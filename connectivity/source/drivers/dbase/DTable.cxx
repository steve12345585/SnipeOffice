/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <dbase/DTable.hxx>
#include <com/sun/star/container/ElementExistException.hpp>
#include <com/sun/star/sdbc/ColumnValue.hpp>
#include <com/sun/star/sdbc/DataType.hpp>
#include <com/sun/star/ucb/XContentAccess.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <o3tl/safeint.hxx>
#include <svl/converter.hxx>
#include <dbase/DConnection.hxx>
#include <dbase/DColumns.hxx>
#include <tools/config.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <dbase/DIndex.hxx>
#include <dbase/DIndexes.hxx>
#include <comphelper/processfactory.hxx>
#include <rtl/math.hxx>
#include <ucbhelper/content.hxx>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <connectivity/dbexception.hxx>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <comphelper/property.hxx>
#include <comphelper/servicehelper.hxx>
#include <o3tl/string_view.hxx>
#include <comphelper/string.hxx>
#include <comphelper/configuration.hxx>
#include <unotools/tempfile.hxx>
#include <unotools/ucbhelper.hxx>
#include <comphelper/types.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <connectivity/dbtools.hxx>
#include <connectivity/FValue.hxx>
#include <connectivity/dbconversion.hxx>
#include <connectivity/sdbcx/VColumn.hxx>
#include <strings.hrc>
#include <rtl/strbuf.hxx>
#include <sal/log.hxx>
#include <tools/date.hxx>
#include <i18nutil/calendar.hxx>

#include <algorithm>
#include <cassert>
#include <memory>
#include <string_view>

using namespace ::comphelper;
using namespace connectivity;
using namespace connectivity::sdbcx;
using namespace connectivity::dbase;
using namespace connectivity::file;
using namespace ::ucbhelper;
using namespace ::utl;
using namespace ::cppu;
using namespace ::dbtools;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::i18n;

// stored as the Field Descriptor terminator
#define FIELD_DESCRIPTOR_TERMINATOR 0x0D
#define DBF_EOL                     0x1A

namespace
{
std::size_t lcl_getFileSize(SvStream& _rStream)
{
    std::size_t nFileSize = 0;
    _rStream.Seek(STREAM_SEEK_TO_END);
    _rStream.SeekRel(-1);
    char cEOL;
    _rStream.ReadChar( cEOL );
    nFileSize = _rStream.Tell();
    if ( cEOL == DBF_EOL )
        nFileSize -= 1;
    return nFileSize;
}
/**
    calculates the Julian date
*/
void lcl_CalcJulDate(sal_Int32& _nJulianDate,sal_Int32& _nJulianTime, const css::util::DateTime& rDateTime)
{
    css::util::DateTime aDateTime = rDateTime;
    // weird: months fix
    if (aDateTime.Month > 12)
    {
        aDateTime.Month--;
        sal_uInt16 delta = rDateTime.Month / 12;
        aDateTime.Year += delta;
        aDateTime.Month -= delta * 12;
        aDateTime.Month++;
    }

    _nJulianTime = ((aDateTime.Hours*3600000)+(aDateTime.Minutes*60000)+(aDateTime.Seconds*1000)+(aDateTime.NanoSeconds/1000000));
    /* conversion factors */
    sal_uInt16 iy0;
    sal_uInt16 im0;
    if ( aDateTime.Month <= 2 )
    {
        iy0 = aDateTime.Year - 1;
        im0 = aDateTime.Month + 12;
    }
    else
    {
        iy0 = aDateTime.Year;
        im0 = aDateTime.Month;
    }
    sal_Int32 ia = iy0 / 100;
    sal_Int32 ib = 2 - ia + (ia >> 2);
    /* calculate julian date    */
    if ( aDateTime.Year <= 0 )
    {
        _nJulianDate = static_cast<sal_Int32>((365.25 * iy0) - 0.75)
            + static_cast<sal_Int32>(i18nutil::monthDaysWithoutJanFeb * (im0 + 1) )
            + aDateTime.Day + 1720994;
    } // if ( rDateTime.Year <= 0 )
    else
    {
        _nJulianDate = static_cast<sal_Int32>(365.25 * iy0)
            + static_cast<sal_Int32>(i18nutil::monthDaysWithoutJanFeb * (im0 + 1))
            + aDateTime.Day + 1720994;
    }
    double JD = _nJulianDate + 0.5;
    _nJulianDate = static_cast<sal_Int32>( JD + 0.5);
    const double gyr = aDateTime.Year + (0.01 * aDateTime.Month) + (0.0001 * aDateTime.Day);
    if ( gyr >= 1582.1015 ) /* on or after 15 October 1582  */
        _nJulianDate += ib;
}

/**
    calculates date time from the Julian Date
*/
void lcl_CalDate(sal_Int32 _nJulianDate,sal_Int32 _nJulianTime,css::util::DateTime& _rDateTime)
{
    if ( _nJulianDate )
    {
        sal_Int64 ka = _nJulianDate;
        if ( _nJulianDate >= 2299161 )
        {
            sal_Int64 ialp = static_cast<sal_Int64>( (static_cast<double>(_nJulianDate) - 1867216.25 ) / 36524.25 );
            ka = ka + 1 + ialp - ( ialp >> 2 );
        }
        sal_Int64 kb = ka + 1524;
        sal_Int64 kc = static_cast<sal_Int64>((static_cast<double>(kb) - 122.1) / 365.25);
        sal_Int64 kd = static_cast<sal_Int64>(static_cast<double>(kc) * 365.25);
        sal_Int64 ke = static_cast<sal_Int64>(static_cast<double>(kb - kd) / i18nutil::monthDaysWithoutJanFeb);
        _rDateTime.Day = static_cast<sal_uInt16>(kb - kd - static_cast<sal_Int64>( static_cast<double>(ke) * i18nutil::monthDaysWithoutJanFeb ));
        if ( ke > 13 )
            _rDateTime.Month = static_cast<sal_uInt16>(ke - 13);
        else
            _rDateTime.Month = static_cast<sal_uInt16>(ke - 1);
        if ( (_rDateTime.Month == 2) && (_rDateTime.Day > 28) )
            _rDateTime.Day = 29;
        if ( (_rDateTime.Month == 2) && (_rDateTime.Day == 29) && (ke == 3) )
            _rDateTime.Year = static_cast<sal_uInt16>(kc - 4716);
        else if ( _rDateTime.Month > 2 )
            _rDateTime.Year = static_cast<sal_uInt16>(kc - 4716);
        else
            _rDateTime.Year = static_cast<sal_uInt16>(kc - 4715);
    }

    if ( _nJulianTime )
    {
        double d_s = _nJulianTime / 1000.0;
        double d_m = d_s / 60.0;
        double d_h  = d_m / 60.0;
        _rDateTime.Hours = static_cast<sal_uInt16>(d_h);
        _rDateTime.Minutes = static_cast<sal_uInt16>((d_h - static_cast<double>(_rDateTime.Hours)) * 60.0);
        _rDateTime.Seconds = static_cast<sal_uInt16>(((d_m - static_cast<double>(_rDateTime.Minutes)) * 60.0)
                - (static_cast<double>(_rDateTime.Hours) * 3600.0));
    }
}

}


void ODbaseTable::readHeader()
{
    OSL_ENSURE(m_pFileStream,"No Stream available!");
    if(!m_pFileStream)
        return;
    m_pFileStream->RefreshBuffer(); // Make sure, that the header information actually is read again
    m_pFileStream->Seek(STREAM_SEEK_TO_BEGIN);

    sal_uInt8 nType=0;
    m_pFileStream->ReadUChar( nType );
    if(ERRCODE_NONE != m_pFileStream->GetErrorCode())
        throwInvalidDbaseFormat();

    m_pFileStream->ReadBytes(m_aHeader.dateElems, 3);
    if(ERRCODE_NONE != m_pFileStream->GetErrorCode())
        throwInvalidDbaseFormat();

    m_pFileStream->ReadUInt32( m_aHeader.nbRecords);
    if(ERRCODE_NONE != m_pFileStream->GetErrorCode())
        throwInvalidDbaseFormat();

    m_pFileStream->ReadUInt16( m_aHeader.headerLength);
    if(ERRCODE_NONE != m_pFileStream->GetErrorCode())
        throwInvalidDbaseFormat();

    m_pFileStream->ReadUInt16( m_aHeader.recordLength);
    if(ERRCODE_NONE != m_pFileStream->GetErrorCode())
        throwInvalidDbaseFormat();
    if (m_aHeader.recordLength == 0)
        throwInvalidDbaseFormat();

    m_pFileStream->ReadBytes(m_aHeader.trailer, 20);
    if(ERRCODE_NONE != m_pFileStream->GetErrorCode())
        throwInvalidDbaseFormat();


    if ( ( ( m_aHeader.headerLength - 1 ) / 32 - 1 ) <= 0 ) // number of fields
    {
        // no dBASE file
        throwInvalidDbaseFormat();
    }
    else
    {
        // Consistency check of the header:
        m_aHeader.type = static_cast<DBFType>(nType);
        switch (m_aHeader.type)
        {
            case dBaseIII:
            case dBaseIV:
            case dBaseV:
            case VisualFoxPro:
            case VisualFoxProAuto:
            case dBaseFS:
            case dBaseFSMemo:
            case dBaseIVMemoSQL:
            case dBaseIIIMemo:
            case FoxProMemo:
                m_pFileStream->SetEndian(SvStreamEndian::LITTLE);
                if( getConnection()->isTextEncodingDefaulted() &&
                   !dbfDecodeCharset(m_eEncoding, nType, m_aHeader.trailer[17]))
                {
                    m_eEncoding = RTL_TEXTENCODING_IBM_850;
                }
                break;
            case dBaseIVMemo:
                m_pFileStream->SetEndian(SvStreamEndian::LITTLE);
                break;
            default:
            {
                throwInvalidDbaseFormat();
            }
        }
    }
}

void ODbaseTable::fillColumns()
{
    m_pFileStream->Seek(STREAM_SEEK_TO_BEGIN);
    if (!checkSeek(*m_pFileStream, 32))
    {
        SAL_WARN("connectivity.drivers", "ODbaseTable::fillColumns: bad offset!");
        return;
    }

    if(!m_aColumns.is())
        m_aColumns = new OSQLColumns();
    else
        m_aColumns->clear();

    m_aTypes.clear();
    m_aPrecisions.clear();
    m_aScales.clear();

    // Number of fields:
    sal_Int32 nFieldCount = (m_aHeader.headerLength - 1) / 32 - 1;
    if (nFieldCount <= 0)
    {
        SAL_WARN("connectivity.drivers", "No columns in table!");
        return;
    }

    auto nRemainingsize = m_pFileStream->remainingSize();
    auto nMaxPossibleRecords = nRemainingsize / 32;
    if (o3tl::make_unsigned(nFieldCount) > nMaxPossibleRecords)
    {
        SAL_WARN("connectivity.drivers", "Parsing error: " << nMaxPossibleRecords <<
                 " max possible entries, but " << nFieldCount << " claimed, truncating");
        nFieldCount = nMaxPossibleRecords;
    }

    m_aColumns->reserve(nFieldCount);
    m_aTypes.reserve(nFieldCount);
    m_aPrecisions.reserve(nFieldCount);
    m_aScales.reserve(nFieldCount);

    OUString aTypeName;
    const bool bCase = getConnection()->getMetaData()->supportsMixedCaseQuotedIdentifiers();
    const bool bFoxPro = m_aHeader.type == VisualFoxPro || m_aHeader.type == VisualFoxProAuto || m_aHeader.type == FoxProMemo;

    sal_Int32 i = 0;
    for (; i < nFieldCount; i++)
    {
        DBFColumn aDBFColumn;
        m_pFileStream->ReadBytes(aDBFColumn.db_fnm, 11);
        m_pFileStream->ReadUChar(aDBFColumn.db_typ);
        m_pFileStream->ReadUInt32(aDBFColumn.db_adr);
        m_pFileStream->ReadUChar(aDBFColumn.db_flng);
        m_pFileStream->ReadUChar(aDBFColumn.db_dez);
        m_pFileStream->ReadBytes(aDBFColumn.db_free2, 14);
        if (!m_pFileStream->good())
        {
            SAL_WARN("connectivity.drivers", "ODbaseTable::fillColumns: short read!");
            break;
        }
        if ( FIELD_DESCRIPTOR_TERMINATOR == aDBFColumn.db_fnm[0] ) // 0x0D stored as the Field Descriptor terminator.
            break;

        aDBFColumn.db_fnm[sizeof(aDBFColumn.db_fnm)-1] = 0; //ensure null termination for broken input
        const OUString aColumnName(reinterpret_cast<char *>(aDBFColumn.db_fnm), strlen(reinterpret_cast<char *>(aDBFColumn.db_fnm)), m_eEncoding);

        bool bIsRowVersion = bFoxPro && ( aDBFColumn.db_free2[0] & 0x01 ) == 0x01;

        m_aRealFieldLengths.push_back(aDBFColumn.db_flng);
        sal_Int32 nPrecision = aDBFColumn.db_flng;
        sal_Int32 eType;
        bool bIsCurrency = false;

        char cType[2];
        cType[0] = aDBFColumn.db_typ;
        cType[1] = 0;
        aTypeName = OUString(cType, 1, RTL_TEXTENCODING_ASCII_US);
        SAL_INFO( "connectivity.drivers","column type: " << aDBFColumn.db_typ);

        switch (aDBFColumn.db_typ)
        {
        case 'C':
            eType = DataType::VARCHAR;
            aTypeName = "VARCHAR";
            break;
        case 'F':
        case 'N':
            aTypeName = "DECIMAL";
            if ( aDBFColumn.db_typ == 'N' )
                aTypeName = "NUMERIC";
            eType = DataType::DECIMAL;

            // for numeric fields two characters more are written, then the precision of the column description predescribes,
            // to keep room for the possible sign and the comma. This has to be considered...
            nPrecision = SvDbaseConverter::ConvertPrecisionToOdbc(nPrecision,aDBFColumn.db_dez);
            // This is not true for older versions...
            break;
        case 'L':
            eType = DataType::BIT;
            aTypeName = "BOOLEAN";
            break;
        case 'Y':
            bIsCurrency = true;
            eType = DataType::DOUBLE;
            aTypeName = "DOUBLE";
            break;
        case 'D':
            eType = DataType::DATE;
            aTypeName = "DATE";
            break;
        case 'T':
            eType = DataType::TIMESTAMP;
            aTypeName = "TIMESTAMP";
            break;
        case 'I':
            eType = DataType::INTEGER;
            aTypeName = "INTEGER";
            break;
        case 'M':
            if ( bFoxPro && ( aDBFColumn.db_free2[0] & 0x04 ) == 0x04 )
            {
                eType = DataType::LONGVARBINARY;
                aTypeName = "LONGVARBINARY";
            }
            else
            {
                aTypeName = "LONGVARCHAR";
                eType = DataType::LONGVARCHAR;
            }
            nPrecision = 2147483647;
            break;
        case 'P':
            aTypeName = "LONGVARBINARY";
            eType = DataType::LONGVARBINARY;
            nPrecision = 2147483647;
            break;
        case '0':
        case 'B':
            if ( m_aHeader.type == VisualFoxPro || m_aHeader.type == VisualFoxProAuto )
            {
                aTypeName = "DOUBLE";
                eType = DataType::DOUBLE;
            }
            else
            {
                aTypeName = "LONGVARBINARY";
                eType = DataType::LONGVARBINARY;
                nPrecision = 2147483647;
            }
            break;
        default:
            eType = DataType::OTHER;
        }

        m_aTypes.push_back(eType);
        m_aPrecisions.push_back(nPrecision);
        m_aScales.push_back(aDBFColumn.db_dez);

        Reference< XPropertySet> xCol = new sdbcx::OColumn(aColumnName,
                                                    aTypeName,
                                                    OUString(),
                                                    OUString(),
                                                    ColumnValue::NULLABLE,
                                                    nPrecision,
                                                    aDBFColumn.db_dez,
                                                    eType,
                                                    false,
                                                    bIsRowVersion,
                                                    bIsCurrency,
                                                    bCase,
                                                    m_CatalogName, getSchema(), getName());
        m_aColumns->push_back(xCol);
    } // for (; i < nFieldCount; i++)
    OSL_ENSURE(i,"No columns in table!");
}

ODbaseTable::ODbaseTable(sdbcx::OCollection* _pTables, ODbaseConnection* _pConnection)
    : ODbaseTable_BASE(_pTables,_pConnection)
{
    // initialize the header
    m_aHeader.type = dBaseIII;
    m_eEncoding = getConnection()->getTextEncoding();
}

ODbaseTable::ODbaseTable(sdbcx::OCollection* _pTables, ODbaseConnection* _pConnection,
                         const OUString& Name,
                         const OUString& Type,
                         const OUString& Description ,
                         const OUString& SchemaName,
                         const OUString& CatalogName )
    : ODbaseTable_BASE(_pTables,_pConnection,Name,
                       Type,
                       Description,
                       SchemaName,
                       CatalogName)
{
    m_eEncoding = getConnection()->getTextEncoding();
}

void ODbaseTable::construct()
{
    // initialize the header
    m_aHeader.type = dBaseIII;
    m_aHeader.nbRecords = 0;
    m_aHeader.headerLength = 0;
    m_aHeader.recordLength = 0;
    m_aMemoHeader.db_size = 0;

    OUString sFileName(getEntry(m_pConnection, m_Name));

    INetURLObject aURL;
    aURL.SetURL(sFileName);

    OSL_ENSURE( m_pConnection->matchesExtension( aURL.getExtension() ),
        "ODbaseTable::ODbaseTable: invalid extension!");
        // getEntry is expected to ensure the correct file name

    m_pFileStream = createStream_simpleError( sFileName, StreamMode::READWRITE | StreamMode::NOCREATE | StreamMode::SHARE_DENYWRITE);
    m_bWriteable = ( m_pFileStream != nullptr );

    if ( !m_pFileStream )
    {
        m_bWriteable = false;
        m_pFileStream = createStream_simpleError( sFileName, StreamMode::READ | StreamMode::NOCREATE | StreamMode::SHARE_DENYNONE);
    }

    if (!m_pFileStream)
        return;

    readHeader();

    std::size_t nFileSize = lcl_getFileSize(*m_pFileStream);

    if (m_aHeader.headerLength > nFileSize)
    {
        SAL_WARN("connectivity.drivers", "Parsing error: " << nFileSize <<
                 " max possible size, but " << m_aHeader.headerLength << " claimed, abandoning");
        return;
    }

    if (m_aHeader.recordLength)
    {
        std::size_t nMaxPossibleRecords = (nFileSize - m_aHeader.headerLength) / m_aHeader.recordLength;
        // #i83401# seems to be empty or someone wrote nonsense into the dbase
        // file try and recover if m_aHeader.db_slng is sane
        if (m_aHeader.nbRecords == 0)
        {
            SAL_WARN("connectivity.drivers", "Parsing warning: 0 records claimed, recovering");
            m_aHeader.nbRecords = nMaxPossibleRecords;
        }
        else if (m_aHeader.nbRecords > nMaxPossibleRecords)
        {
            SAL_WARN("connectivity.drivers", "Parsing error: " << nMaxPossibleRecords <<
                     " max possible records, but " << m_aHeader.nbRecords << " claimed, truncating");
            m_aHeader.nbRecords = std::max(nMaxPossibleRecords, static_cast<size_t>(1));
        }
    }

    if (HasMemoFields())
    {
    // Create Memo-Filename (.DBT):
    // nyi: Ugly for Unix and Mac!

        if ( m_aHeader.type == FoxProMemo || m_aHeader.type == VisualFoxPro || m_aHeader.type == VisualFoxProAuto) // foxpro uses another extension
            aURL.SetExtension(u"fpt");
        else
            aURL.SetExtension(u"dbt");

        // If the memo file isn't found, the data will be displayed anyhow.
        // However, updates can't be done
        // but the operation is executed
        m_pMemoStream = createStream_simpleError( aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE), StreamMode::READWRITE | StreamMode::NOCREATE | StreamMode::SHARE_DENYWRITE);
        if ( !m_pMemoStream )
        {
            m_pMemoStream = createStream_simpleError( aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE), StreamMode::READ | StreamMode::NOCREATE | StreamMode::SHARE_DENYNONE);
        }
        if (m_pMemoStream)
            ReadMemoHeader();
    }

    fillColumns();
    m_pFileStream->Seek(STREAM_SEEK_TO_BEGIN);


    // Buffersize dependent on the file size
    m_pFileStream->SetBufferSize(nFileSize > 1000000 ? 32768 :
                              nFileSize > 100000 ? 16384 :
                              nFileSize > 10000 ? 4096 : 1024);

    if (m_pMemoStream)
    {
        // set the buffer exactly to the length of a record
        nFileSize = m_pMemoStream->TellEnd();
        m_pMemoStream->Seek(STREAM_SEEK_TO_BEGIN);

        // Buffersize dependent on the file size
        m_pMemoStream->SetBufferSize(nFileSize > 1000000 ? 32768 :
                                      nFileSize > 100000 ? 16384 :
                                      nFileSize > 10000 ? 4096 :
                                      m_aMemoHeader.db_size);
    }

    AllocBuffer();
}

void ODbaseTable::ReadMemoHeader()
{
    m_pMemoStream->SetEndian(SvStreamEndian::LITTLE);
    m_pMemoStream->RefreshBuffer();         // make sure that the header information is actually read again
    m_pMemoStream->Seek(0);

    (*m_pMemoStream).ReadUInt32( m_aMemoHeader.db_next );
    switch (m_aHeader.type)
    {
        case dBaseIIIMemo:  // dBase III: fixed block size
        case dBaseIVMemo:
            // sometimes dBase3 is attached to dBase4 memo
            m_pMemoStream->Seek(20);
            (*m_pMemoStream).ReadUInt16( m_aMemoHeader.db_size );
            if (m_aMemoHeader.db_size > 1 && m_aMemoHeader.db_size != 512)  // 1 is also for dBase 3
                m_aMemoHeader.db_typ  = MemodBaseIV;
            else if (m_aMemoHeader.db_size == 512)
            {
                // There are files using size specification, though they are dBase-files
                char sHeader[4];
                m_pMemoStream->Seek(m_aMemoHeader.db_size);
                m_pMemoStream->ReadBytes(sHeader, 4);

                if ((m_pMemoStream->GetErrorCode() != ERRCODE_NONE) || static_cast<sal_uInt8>(sHeader[0]) != 0xFF || static_cast<sal_uInt8>(sHeader[1]) != 0xFF || static_cast<sal_uInt8>(sHeader[2]) != 0x08)
                    m_aMemoHeader.db_typ  = MemodBaseIII;
                else
                    m_aMemoHeader.db_typ  = MemodBaseIV;
            }
            else
            {
                m_aMemoHeader.db_typ  = MemodBaseIII;
                m_aMemoHeader.db_size = 512;
            }
            break;
        case VisualFoxPro:
        case VisualFoxProAuto:
        case FoxProMemo:
            m_aMemoHeader.db_typ    = MemoFoxPro;
            m_pMemoStream->Seek(6);
            m_pMemoStream->SetEndian(SvStreamEndian::BIG);
            (*m_pMemoStream).ReadUInt16( m_aMemoHeader.db_size );
            break;
        default:
            SAL_WARN( "connectivity.drivers", "ODbaseTable::ReadMemoHeader: unsupported memo type!" );
            break;
    }
}

OUString ODbaseTable::getEntry(file::OConnection const * _pConnection, std::u16string_view _sName )
{
    OUString sURL;
    try
    {
        Reference< XResultSet > xDir = _pConnection->getDir()->getStaticResultSet();
        Reference< XRow> xRow(xDir,UNO_QUERY);
        OUString sName;
        OUString sExt;
        INetURLObject aURL;
        xDir->beforeFirst();
        while(xDir->next())
        {
            sName = xRow->getString(1);
            aURL.SetSmartProtocol(INetProtocol::File);
            OUString sUrl = _pConnection->getURL() + "/" + sName;
            aURL.SetSmartURL( sUrl );

            // cut the extension
            sExt = aURL.getExtension();

            // name and extension have to coincide
            if ( _pConnection->matchesExtension( sExt ) )
            {
                sName = sName.replaceAt(sName.getLength() - (sExt.getLength() + 1), sExt.getLength() + 1, u"");
                if ( sName == _sName )
                {
                    Reference< XContentAccess > xContentAccess( xDir, UNO_QUERY );
                    sURL = xContentAccess->queryContentIdentifierString();
                    break;
                }
            }
        }
        xDir->beforeFirst(); // move back to before first record
    }
    catch(const Exception&)
    {
        OSL_ASSERT(false);
    }
    return sURL;
}

void ODbaseTable::refreshColumns()
{
    ::osl::MutexGuard aGuard( m_aMutex );

    ::std::vector< OUString> aVector;
    aVector.reserve(m_aColumns->size());

    for (auto const& column : *m_aColumns)
        aVector.push_back(Reference< XNamed>(column,UNO_QUERY_THROW)->getName());

    if(m_xColumns)
        m_xColumns->reFill(aVector);
    else
        m_xColumns.reset(new ODbaseColumns(this,m_aMutex,aVector));
}

void ODbaseTable::refreshIndexes()
{
    ::std::vector< OUString> aVector;
    if(m_pFileStream && (!m_xIndexes || m_xIndexes->getCount() == 0))
    {
        INetURLObject aURL;
        aURL.SetURL(getEntry(m_pConnection,m_Name));

        aURL.setExtension(u"inf");
        Config aInfFile(aURL.getFSysPath(FSysStyle::Detect));
        aInfFile.SetGroup(dBASE_III_GROUP);
        sal_uInt16 nKeyCnt = aInfFile.GetKeyCount();
        OString aKeyName;

        for (sal_uInt16 nKey = 0; nKey < nKeyCnt; nKey++)
        {
            // References the key an index-file?
            aKeyName = aInfFile.GetKeyName( nKey );
            //...if yes, add the index list of the table
            if (aKeyName.startsWith("NDX"))
            {
                OString aIndexName = aInfFile.ReadKey(aKeyName);
                aURL.setName(OStringToOUString(aIndexName, m_eEncoding));
                try
                {
                    Content aCnt(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE),Reference<XCommandEnvironment>(), comphelper::getProcessComponentContext());
                    if (aCnt.isDocument())
                    {
                        aVector.push_back(aURL.getBase());
                    }
                }
                catch(const Exception&) // an exception is thrown when no file exists
                {
                }
            }
        }
    }
    if(m_xIndexes)
        m_xIndexes->reFill(aVector);
    else
        m_xIndexes.reset(new ODbaseIndexes(this,m_aMutex,aVector));
}


void SAL_CALL ODbaseTable::disposing()
{
    OFileTable::disposing();
    ::osl::MutexGuard aGuard(m_aMutex);
    m_aColumns = nullptr;
}

Sequence< Type > SAL_CALL ODbaseTable::getTypes(  )
{
    Sequence< Type > aTypes = OTable_TYPEDEF::getTypes();
    std::vector<Type> aOwnTypes;
    aOwnTypes.reserve(aTypes.getLength());

    for (auto& type : aTypes)
    {
        if(type != cppu::UnoType<XKeysSupplier>::get() &&
           type != cppu::UnoType<XDataDescriptorFactory>::get())
        {
            aOwnTypes.push_back(type);
        }
    }
    aOwnTypes.push_back(cppu::UnoType<css::lang::XUnoTunnel>::get());
    return Sequence< Type >(aOwnTypes.data(), aOwnTypes.size());
}


Any SAL_CALL ODbaseTable::queryInterface( const Type & rType )
{
    if( rType == cppu::UnoType<XKeysSupplier>::get()||
        rType == cppu::UnoType<XDataDescriptorFactory>::get())
        return Any();

    Any aRet = OTable_TYPEDEF::queryInterface(rType);
    return aRet;
}


bool ODbaseTable::fetchRow(OValueRefRow& _rRow, const OSQLColumns & _rCols, bool bRetrieveData)
{
    if (!m_pBuffer)
        return false;

    // Read the data
    bool bIsCurRecordDeleted = m_pBuffer[0] == '*';

    // only read the bookmark

    // Mark record as deleted
    _rRow->setDeleted(bIsCurRecordDeleted);
    *(*_rRow)[0] = m_nFilePos;

    if (!bRetrieveData)
        return true;

    std::size_t nByteOffset = 1;
    // Fields:
    OSQLColumns::const_iterator aIter = _rCols.begin();
    OSQLColumns::const_iterator aEnd  = _rCols.end();
    const std::size_t nCount = _rRow->size();
    for (std::size_t i = 1; aIter != aEnd && nByteOffset <= m_nBufferSize && i < nCount;++aIter, i++)
    {
        // Lengths depending on data type:
        sal_Int32 nLen = m_aPrecisions[i-1];
        sal_Int32 nType = m_aTypes[i-1];

        switch(nType)
        {
            case DataType::INTEGER:
            case DataType::DOUBLE:
            case DataType::TIMESTAMP:
            case DataType::DATE:
            case DataType::BIT:
            case DataType::LONGVARCHAR:
            case DataType::LONGVARBINARY:
                nLen = m_aRealFieldLengths[i-1];
                break;
            case DataType::DECIMAL:
                nLen = SvDbaseConverter::ConvertPrecisionToDbase(nLen,m_aScales[i-1]);
                break;  // the sign and the comma

            case DataType::BINARY:
            case DataType::OTHER:
                nByteOffset += nLen;
                continue;
        }

        // Is the variable bound?
        if ( !(*_rRow)[i]->isBound() )
        {
            // No - next field.
            nByteOffset += nLen;
            OSL_ENSURE( nByteOffset <= m_nBufferSize ,"ByteOffset > m_nBufferSize!");
            continue;
        } // if ( !(_rRow->get())[i]->isBound() )
        if ( ( nByteOffset + nLen) > m_nBufferSize )
            break; // length doesn't match buffer size.

        char *pData = reinterpret_cast<char *>(m_pBuffer.get() + nByteOffset);

        if (nType == DataType::CHAR || nType == DataType::VARCHAR)
        {
            sal_Int32 nLastPos = -1;
            for (sal_Int32 k = 0; k < nLen; ++k)
            {
                if (pData[k] != ' ')
                    // Record last non-empty position.
                    nLastPos = k;
            }
            if (nLastPos < 0)
            {
                // Empty string.  Skip it.
                (*_rRow)[i]->setNull();
            }
            else
            {
                // Commit the string
                *(*_rRow)[i] = OUString(pData, static_cast<sal_Int32>(nLastPos+1), m_eEncoding);
            }
        } // if (nType == DataType::CHAR || nType == DataType::VARCHAR)
        else if ( DataType::TIMESTAMP == nType )
        {
            sal_Int32 nDate = 0,nTime = 0;
            if (o3tl::make_unsigned(nLen) < 8)
            {
                SAL_WARN("connectivity.drivers", "short TIMESTAMP");
                return false;
            }
            memcpy(&nDate, pData, 4);
            memcpy(&nTime, pData + 4, 4);
            if ( !nDate && !nTime )
            {
                (*_rRow)[i]->setNull();
            }
            else
            {
                css::util::DateTime aDateTime;
                lcl_CalDate(nDate,nTime,aDateTime);
                *(*_rRow)[i] = aDateTime;
            }
        }
        else if ( DataType::INTEGER == nType )
        {
            sal_Int32 nValue = 0;
            if (o3tl::make_unsigned(nLen) > sizeof(nValue))
                return false;
            memcpy(&nValue, pData, nLen);
            *(*_rRow)[i] = nValue;
        }
        else if ( DataType::DOUBLE == nType )
        {
            double d = 0.0;
            if (getBOOL((*aIter)->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISCURRENCY)))) // Currency is treated separately
            {
                sal_Int64 nValue = 0;
                if (o3tl::make_unsigned(nLen) > sizeof(nValue))
                    return false;
                memcpy(&nValue, pData, nLen);

                if ( m_aScales[i-1] )
                    d = (nValue / pow(10.0,static_cast<int>(m_aScales[i-1])));
                else
                    d = static_cast<double>(nValue);
            }
            else
            {
                if (o3tl::make_unsigned(nLen) > sizeof(d))
                    return false;
                memcpy(&d, pData, nLen);
            }

            *(*_rRow)[i] = d;
        }
        else
        {
            sal_Int32 nPos1 = -1, nPos2 = -1;
            // If the string contains Nul-characters, then convert them to blanks!
            for (sal_Int32 k = 0; k < nLen; k++)
            {
                if (pData[k] == '\0')
                    pData[k] = ' ';

                if (pData[k] != ' ')
                {
                    if (nPos1 < 0)
                        // first non-empty char position.
                        nPos1 = k;

                    // last non-empty char position.
                    nPos2 = k;
                }
            }

            if (nPos1 < 0)
            {
                // Empty string.  Skip it.
                nByteOffset += nLen;
                (*_rRow)[i]->setNull();   // no values -> done
                continue;
            }

            OUString aStr(pData+nPos1, nPos2-nPos1+1, m_eEncoding);

            switch (nType)
            {
                case DataType::DATE:
                {
                    if (nLen < 8 || aStr.getLength() != nLen)
                    {
                        (*_rRow)[i]->setNull();
                        break;
                    }
                    const sal_uInt16  nYear   = static_cast<sal_uInt16>(o3tl::toInt32(aStr.subView( 0, 4 )));
                    const sal_uInt16  nMonth  = static_cast<sal_uInt16>(o3tl::toInt32(aStr.subView( 4, 2 )));
                    const sal_uInt16  nDay    = static_cast<sal_uInt16>(o3tl::toInt32(aStr.subView( 6, 2 )));

                    const css::util::Date aDate(nDay,nMonth,nYear);
                    *(*_rRow)[i] = aDate;
                }
                break;
                case DataType::DECIMAL:
                    *(*_rRow)[i] = ORowSetValue(aStr);
                break;
                case DataType::BIT:
                {
                    bool b;
                    switch (*pData)
                    {
                        case 'T':
                        case 'Y':
                        case 'J':   b = true; break;
                        default:    b = false; break;
                    }
                    *(*_rRow)[i] = b;
                }
                break;
                case DataType::LONGVARBINARY:
                case DataType::BINARY:
                case DataType::LONGVARCHAR:
                {
                    const tools::Long nBlockNo = aStr.toInt32();   // read blocknumber
                    if (nBlockNo > 0 && m_pMemoStream) // Read data from memo-file, only if
                    {
                        if ( !ReadMemo(nBlockNo, (*_rRow)[i]->get()) )
                            break;
                    }
                    else
                        (*_rRow)[i]->setNull();
                }   break;
                default:
                    SAL_WARN( "connectivity.drivers","Wrong type");
            }
            (*_rRow)[i]->setTypeKind(nType);
        }

        nByteOffset += nLen;
        OSL_ENSURE( nByteOffset <= m_nBufferSize ,"ByteOffset > m_nBufferSize!");
    }
    return true;
}


void ODbaseTable::FileClose()
{
    ::osl::MutexGuard aGuard(m_aMutex);

    m_pMemoStream.reset();

    ODbaseTable_BASE::FileClose();
}

bool ODbaseTable::CreateImpl()
{
    OSL_ENSURE(!m_pFileStream, "SequenceError");

    if ( m_pConnection->isCheckEnabled() && ::dbtools::convertName2SQLName(m_Name, u"") != m_Name )
    {
        const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                STR_SQL_NAME_ERROR,
                "$name$", m_Name
             ) );
        ::dbtools::throwGenericSQLException( sError, *this );
    }

    INetURLObject aURL;
    aURL.SetSmartProtocol(INetProtocol::File);
    OUString aName = getEntry(m_pConnection, m_Name);
    if(aName.isEmpty())
    {
        OUString aIdent = m_pConnection->getContent()->getIdentifier()->getContentIdentifier();
        if ( aIdent.lastIndexOf('/') != (aIdent.getLength()-1) )
            aIdent += "/";
        aIdent += m_Name;
        aName = aIdent;
    }
    aURL.SetURL(aName);

    if ( !m_pConnection->matchesExtension( aURL.getExtension() ) )
        aURL.setExtension(m_pConnection->getExtension());

    try
    {
        Content aContent(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE),Reference<XCommandEnvironment>(), comphelper::getProcessComponentContext());
        if (aContent.isDocument())
        {
            // Only if the file exists with length > 0 raise an error
            std::unique_ptr<SvStream> pFileStream(createStream_simpleError( aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE), StreamMode::READ));

            if (pFileStream && pFileStream->TellEnd())
                return false;
        }
    }
    catch(const Exception&) // an exception is thrown when no file exists
    {
    }

    bool bMemoFile = false;

    bool bOk = CreateFile(aURL, bMemoFile);

    FileClose();

    if (!bOk)
    {
        try
        {
            Content aContent(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE),Reference<XCommandEnvironment>(), comphelper::getProcessComponentContext());
            aContent.executeCommand( u"delete"_ustr, css::uno::Any( true ) );
        }
        catch(const Exception&) // an exception is thrown when no file exists
        {
        }
        return false;
    }

    if (bMemoFile)
    {
        OUString aExt = aURL.getExtension();
        aURL.setExtension(u"dbt");                      // extension for memo file

        bool bMemoAlreadyExists = false;
        try
        {
            Content aMemo1Content(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE),Reference<XCommandEnvironment>(), comphelper::getProcessComponentContext());
            bMemoAlreadyExists = aMemo1Content.isDocument();
        }
        catch(const Exception&) // an exception is thrown when no file exists
        {
        }
        if (bMemoAlreadyExists)
        {
            aURL.setExtension(aExt);      // kill dbf file
            try
            {
                Content aMemoContent(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE),Reference<XCommandEnvironment>(), comphelper::getProcessComponentContext());
                aMemoContent.executeCommand( u"delete"_ustr, css::uno::Any( true ) );
            }
            catch(const Exception&)
            {
                css::uno::Any anyEx = cppu::getCaughtException();
                const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                        STR_COULD_NOT_DELETE_FILE,
                        "$name$", aName
                     ) );
                ::dbtools::throwGenericSQLException( sError, *this, anyEx );
            }
        }
        if (!CreateMemoFile(aURL))
        {
            aURL.setExtension(aExt);      // kill dbf file
            try
            {
                Content aMemoContent(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE),Reference<XCommandEnvironment>(), comphelper::getProcessComponentContext());
                aMemoContent.executeCommand( u"delete"_ustr, css::uno::Any( true ) );
            }
            catch(const ContentCreationException&)
            {
                css::uno::Any anyEx = cppu::getCaughtException();
                const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                        STR_COULD_NOT_DELETE_FILE,
                        "$name$", aName
                     ) );
                ::dbtools::throwGenericSQLException( sError, *this, anyEx );
            }
            return false;
        }
        m_aHeader.type = dBaseIIIMemo;
    }
    else
        m_aHeader.type = dBaseIII;

    return true;
}

void ODbaseTable::throwInvalidColumnType(TranslateId pErrorId, const OUString& _sColumnName)
{
    try
    {
        // we have to drop the file because it is corrupted now
        DropImpl();
    }
    catch(const Exception&)
    {
    }

    const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
            pErrorId,
            "$columnname$", _sColumnName
         ) );
    ::dbtools::throwGenericSQLException( sError, *this );
}

// creates in principle dBase IV file format
bool ODbaseTable::CreateFile(const INetURLObject& aFile, bool& bCreateMemo)
{
    bCreateMemo = false;
    Date aDate( Date::SYSTEM );                     // current date

    m_pFileStream = createStream_simpleError( aFile.GetMainURL(INetURLObject::DecodeMechanism::NONE),StreamMode::READWRITE | StreamMode::SHARE_DENYWRITE | StreamMode::TRUNC );

    if (!m_pFileStream)
        return false;

    sal_uInt8 nDbaseType = dBaseIII;
    Reference<XIndexAccess> xColumns(getColumns(),UNO_QUERY);
    Reference<XPropertySet> xCol;
    const OUString sPropType = OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE);

    try
    {
        const sal_Int32 nCount = xColumns->getCount();
        for(sal_Int32 i=0;i<nCount;++i)
        {
            xColumns->getByIndex(i) >>= xCol;
            OSL_ENSURE(xCol.is(),"This should be a column!");

            switch (getINT32(xCol->getPropertyValue(sPropType)))
            {
                case DataType::DOUBLE:
                case DataType::INTEGER:
                case DataType::TIMESTAMP:
                case DataType::LONGVARBINARY:
                    nDbaseType = VisualFoxPro;
                    i = nCount; // no more columns need to be checked
                    break;
            } // switch (getINT32(xCol->getPropertyValue(sPropType)))
        }
    }
    catch ( const Exception& )
    {
        try
        {
            // we have to drop the file because it is corrupted now
            DropImpl();
        }
        catch(const Exception&) { }
        throw;
    }

    char aBuffer[21] = {}; // write buffer

    m_pFileStream->Seek(0);
    (*m_pFileStream).WriteUChar( nDbaseType );                            // dBase format
    (*m_pFileStream).WriteUChar( aDate.GetYearUnsigned() % 100 );         // current date


    (*m_pFileStream).WriteUChar( aDate.GetMonth() );
    (*m_pFileStream).WriteUChar( aDate.GetDay() );
    (*m_pFileStream).WriteUInt32( 0 );                                    // number of data records
    (*m_pFileStream).WriteUInt16( (m_xColumns->getCount()+1) * 32 + 1 );  // header information,
                                                                          // pColumns contains always an additional column
    (*m_pFileStream).WriteUInt16( 0 );                                     // record length will be determined later
    m_pFileStream->WriteBytes(aBuffer, 20);

    sal_uInt16 nRecLength = 1;                                              // Length 1 for deleted flag
    sal_Int32  nMaxFieldLength = m_pConnection->getMetaData()->getMaxColumnNameLength();
    OUString aName;
    const OUString sPropName = OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME);
    const OUString sPropPrec = OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PRECISION);
    const OUString sPropScale = OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE);

    try
    {
        const sal_Int32 nCount = xColumns->getCount();
        for(sal_Int32 i=0;i<nCount;++i)
        {
            xColumns->getByIndex(i) >>= xCol;
            OSL_ENSURE(xCol.is(),"This should be a column!");

            char cTyp( 'C' );

            xCol->getPropertyValue(sPropName) >>= aName;

            OString aCol;
            if ( DBTypeConversion::convertUnicodeString( aName, aCol, m_eEncoding ) > nMaxFieldLength)
            {
                throwInvalidColumnType( STR_INVALID_COLUMN_NAME_LENGTH, aName );
            }

            m_pFileStream->WriteOString( aCol );
            m_pFileStream->WriteBytes(aBuffer, 11 - aCol.getLength());

            sal_Int32 nPrecision = 0;
            xCol->getPropertyValue(sPropPrec) >>= nPrecision;
            sal_Int32 nScale = 0;
            xCol->getPropertyValue(sPropScale) >>= nScale;

            bool bBinary = false;

            switch (getINT32(xCol->getPropertyValue(sPropType)))
            {
                case DataType::CHAR:
                case DataType::VARCHAR:
                    cTyp = 'C';
                    break;
                case DataType::DOUBLE:
                    if (getBOOL(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISCURRENCY)))) // Currency will be treated separately
                        cTyp = 'Y';
                    else
                        cTyp = 'B';
                    break;
                case DataType::INTEGER:
                    cTyp = 'I';
                    break;
                case DataType::TINYINT:
                case DataType::SMALLINT:
                case DataType::BIGINT:
                case DataType::DECIMAL:
                case DataType::NUMERIC:
                case DataType::REAL:
                    cTyp = 'N';                             // only dBase 3 format
                    break;
                case DataType::TIMESTAMP:
                    cTyp = 'T';
                    break;
                case DataType::DATE:
                    cTyp = 'D';
                    break;
                case DataType::BIT:
                    cTyp = 'L';
                    break;
                case DataType::LONGVARBINARY:
                    bBinary = true;
                    [[fallthrough]];
                case DataType::LONGVARCHAR:
                    cTyp = 'M';
                    break;
                default:
                    {
                        throwInvalidColumnType(STR_INVALID_COLUMN_TYPE, aName);
                    }
            }

            (*m_pFileStream).WriteChar( cTyp );
            if ( nDbaseType == VisualFoxPro )
                (*m_pFileStream).WriteUInt32( nRecLength-1 );
            else
                m_pFileStream->WriteBytes(aBuffer, 4);

            switch(cTyp)
            {
                case 'C':
                    OSL_ENSURE(nPrecision < 255, "ODbaseTable::Create: Column too long!");
                    if (nPrecision > 254)
                    {
                        throwInvalidColumnType(STR_INVALID_COLUMN_PRECISION, aName);
                    }
                    (*m_pFileStream).WriteUChar( std::min(static_cast<unsigned>(nPrecision), 255U) );      // field length
                    nRecLength = nRecLength + static_cast<sal_uInt16>(std::min(static_cast<sal_uInt16>(nPrecision), sal_uInt16(255UL)));
                    (*m_pFileStream).WriteUChar( 0 );                                                                // decimals
                    break;
                case 'F':
                case 'N':
                    OSL_ENSURE(nPrecision >=  nScale,
                            "ODbaseTable::Create: Field length must be larger than decimal places!");
                    if (nPrecision <  nScale)
                    {
                        throwInvalidColumnType(STR_INVALID_PRECISION_SCALE, aName);
                    }
                    if (getBOOL(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISCURRENCY)))) // Currency will be treated separately
                    {
                        (*m_pFileStream).WriteUChar( 10 );          // standard length
                        (*m_pFileStream).WriteUChar( 4 );
                        nRecLength += 10;
                    }
                    else
                    {
                        sal_Int32 nPrec = SvDbaseConverter::ConvertPrecisionToDbase(nPrecision,nScale);

                        (*m_pFileStream).WriteUChar( nPrec );
                        (*m_pFileStream).WriteUChar( nScale );
                        nRecLength += static_cast<sal_uInt16>(nPrec);
                    }
                    break;
                case 'L':
                    (*m_pFileStream).WriteUChar( 1 );
                    (*m_pFileStream).WriteUChar( 0 );
                    ++nRecLength;
                    break;
                case 'I':
                    (*m_pFileStream).WriteUChar( 4 );
                    (*m_pFileStream).WriteUChar( 0 );
                    nRecLength += 4;
                    break;
                case 'Y':
                case 'B':
                case 'T':
                case 'D':
                    (*m_pFileStream).WriteUChar( 8 );
                    (*m_pFileStream).WriteUChar( 0 );
                    nRecLength += 8;
                    break;
                case 'M':
                    bCreateMemo = true;
                    (*m_pFileStream).WriteUChar( 10 );
                    (*m_pFileStream).WriteUChar( 0 );
                    nRecLength += 10;
                    if ( bBinary )
                        aBuffer[0] = 0x06;
                    break;
                default:
                    throwInvalidColumnType(STR_INVALID_COLUMN_TYPE, aName);
            }
            m_pFileStream->WriteBytes(aBuffer, 14);
            aBuffer[0] = 0x00;
        }

        (*m_pFileStream).WriteUChar( FIELD_DESCRIPTOR_TERMINATOR );              // end of header
        (*m_pFileStream).WriteChar( char(DBF_EOL) );
        m_pFileStream->Seek(10);
        (*m_pFileStream).WriteUInt16( nRecLength );                                     // set record length afterwards

        if (bCreateMemo)
        {
            m_pFileStream->Seek(0);
            if (nDbaseType == VisualFoxPro)
                (*m_pFileStream).WriteUChar( FoxProMemo );
            else
                (*m_pFileStream).WriteUChar( dBaseIIIMemo );
        } // if (bCreateMemo)
    }
    catch ( const Exception& )
    {
        try
        {
            // we have to drop the file because it is corrupted now
            DropImpl();
        }
        catch(const Exception&) { }
        throw;
    }
    return true;
}

bool ODbaseTable::HasMemoFields() const
{
    return m_aHeader.type > dBaseIV && !comphelper::IsFuzzing();
}

// creates in principle dBase III file format
bool ODbaseTable::CreateMemoFile(const INetURLObject& aFile)
{
    // filehandling macro for table creation
    m_pMemoStream = createStream_simpleError( aFile.GetMainURL(INetURLObject::DecodeMechanism::NONE),StreamMode::READWRITE | StreamMode::SHARE_DENYWRITE);

    if (!m_pMemoStream)
        return false;

    m_pMemoStream->SetStreamSize(512);

    m_pMemoStream->Seek(0);
    (*m_pMemoStream).WriteUInt32( 1 );                  // pointer to the first free block

    m_pMemoStream.reset();
    return true;
}

bool ODbaseTable::Drop_Static(std::u16string_view _sUrl, bool _bHasMemoFields, OCollection* _pIndexes )
{
    INetURLObject aURL;
    aURL.SetURL(_sUrl);

    bool bDropped = ::utl::UCBContentHelper::Kill(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE));

    if(bDropped)
    {
        if (_bHasMemoFields)
        {  // delete the memo fields
            aURL.setExtension(u"dbt");
            bDropped = ::utl::UCBContentHelper::Kill(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE));
        }

        if(bDropped)
        {
            if(_pIndexes)
            {
                try
                {
                    sal_Int32 i = _pIndexes->getCount();
                    while (i)
                    {
                        _pIndexes->dropByIndex(--i);
                    }
                }
                catch(const SQLException&)
                {
                }
            }
            aURL.setExtension(u"inf");

            // as the inf file does not necessarily exist, we aren't allowed to use UCBContentHelper::Kill
            try
            {
                ::ucbhelper::Content aDeleteContent( aURL.GetMainURL( INetURLObject::DecodeMechanism::NONE ), Reference< XCommandEnvironment >(), comphelper::getProcessComponentContext() );
                aDeleteContent.executeCommand( u"delete"_ustr, Any( true ) );
            }
            catch(const Exception&)
            {
                // silently ignore this...
            }
        }
    }
    return bDropped;
}

bool ODbaseTable::DropImpl()
{
    FileClose();

    if(!m_xIndexes)
        refreshIndexes(); // look for indexes which must be deleted as well

    bool bDropped = Drop_Static(getEntry(m_pConnection,m_Name),HasMemoFields(),m_xIndexes.get());
    if(!bDropped)
    {// we couldn't drop the table so we have to reopen it
        construct();
        if(m_xColumns)
            m_xColumns->refresh();
    }
    return bDropped;
}


bool ODbaseTable::InsertRow(OValueRefVector& rRow, const Reference<XIndexAccess>& _xCols)
{
    // fill buffer with blanks
    if (!AllocBuffer())
        return false;

    memset(m_pBuffer.get(), 0, m_aHeader.recordLength);
    m_pBuffer[0] = ' ';

    // Copy new row completely:
    // ... and add at the end as new Record:
    std::size_t nTempPos = m_nFilePos;

    m_nFilePos = static_cast<std::size_t>(m_aHeader.nbRecords) + 1;
    bool bInsertRow = UpdateBuffer( rRow, nullptr, _xCols, true );
    if ( bInsertRow )
    {
        std::size_t nFileSize = 0, nMemoFileSize = 0;

        nFileSize = lcl_getFileSize(*m_pFileStream);

        if (HasMemoFields() && m_pMemoStream)
        {
            m_pMemoStream->Seek(STREAM_SEEK_TO_END);
            nMemoFileSize = m_pMemoStream->Tell();
        }

        if (!WriteBuffer())
        {
            m_pFileStream->SetStreamSize(nFileSize);                // restore old size

            if (HasMemoFields() && m_pMemoStream)
                m_pMemoStream->SetStreamSize(nMemoFileSize);    // restore old size
            m_nFilePos = nTempPos;              // restore file position
        }
        else
        {
            (*m_pFileStream).WriteChar( char(DBF_EOL) ); // write EOL
            // raise number of datasets in the header:
            m_pFileStream->Seek( 4 );
            (*m_pFileStream).WriteUInt32( m_aHeader.nbRecords + 1 );

            m_pFileStream->Flush();

            // raise number if successfully
            m_aHeader.nbRecords++;
            *rRow[0] = m_nFilePos;                                // set bookmark
            m_nFilePos = nTempPos;
        }
    }
    else
        m_nFilePos = nTempPos;

    return bInsertRow;
}


bool ODbaseTable::UpdateRow(OValueRefVector& rRow, OValueRefRow& pOrgRow, const Reference<XIndexAccess>& _xCols)
{
    // fill buffer with blanks
    if (!AllocBuffer())
        return false;

    // position on desired record:
    std::size_t nPos = m_aHeader.headerLength + static_cast<tools::Long>(m_nFilePos-1) * m_aHeader.recordLength;
    m_pFileStream->Seek(nPos);
    m_pFileStream->ReadBytes(m_pBuffer.get(), m_aHeader.recordLength);

    std::size_t nMemoFileSize( 0 );
    if (HasMemoFields() && m_pMemoStream)
    {
        m_pMemoStream->Seek(STREAM_SEEK_TO_END);
        nMemoFileSize = m_pMemoStream->Tell();
    }
    if (!UpdateBuffer(rRow, pOrgRow, _xCols, false) || !WriteBuffer())
    {
        if (HasMemoFields() && m_pMemoStream)
            m_pMemoStream->SetStreamSize(nMemoFileSize);    // restore old size
    }
    else
    {
        m_pFileStream->Flush();
    }
    return true;
}


bool ODbaseTable::DeleteRow(const OSQLColumns& _rCols)
{
    // Set the Delete-Flag (be it set or not):
    // Position on desired record:
    std::size_t nFilePos = m_aHeader.headerLength + static_cast<tools::Long>(m_nFilePos-1) * m_aHeader.recordLength;
    m_pFileStream->Seek(nFilePos);

    OValueRefRow aRow = new OValueRefVector(_rCols.size());

    if (!fetchRow(aRow,_rCols,true))
        return false;

    Reference<XPropertySet> xCol;
    OUString aColName;
    ::comphelper::UStringMixEqual aCase(isCaseSensitive());
    for (sal_Int32 i = 0; i < m_xColumns->getCount(); i++)
    {
        Reference<XPropertySet> xIndex = isUniqueByColumnName(i);
        if (xIndex.is())
        {
            xCol.set(m_xColumns->getByIndex(i), css::uno::UNO_QUERY);
            OSL_ENSURE(xCol.is(),"ODbaseTable::DeleteRow column is null!");
            if(xCol.is())
            {
                xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)) >>= aColName;

                ODbaseIndex* pIndex = dynamic_cast<ODbaseIndex*>(xIndex.get());
                assert(pIndex && "ODbaseTable::DeleteRow: No Index returned!");

                OSQLColumns::const_iterator aIter = std::find_if(_rCols.begin(), _rCols.end(),
                    [&aCase, &aColName](const OSQLColumns::value_type& rxCol) {
                        return aCase(getString(rxCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_REALNAME))), aColName); });
                if (aIter == _rCols.end())
                    continue;

                auto nPos = static_cast<sal_Int32>(std::distance(_rCols.begin(), aIter)) + 1;
                pIndex->Delete(m_nFilePos,*(*aRow)[nPos]);
            }
        }
    }

    m_pFileStream->Seek(nFilePos);
    (*m_pFileStream).WriteUChar( '*' ); // mark the row in the table as deleted
    m_pFileStream->Flush();
    return true;
}

Reference<XPropertySet> ODbaseTable::isUniqueByColumnName(sal_Int32 _nColumnPos)
{
    if(!m_xIndexes)
        refreshIndexes();
    if(m_xIndexes->hasElements())
    {
        Reference<XPropertySet> xCol;
        m_xColumns->getByIndex(_nColumnPos) >>= xCol;
        OSL_ENSURE(xCol.is(),"ODbaseTable::isUniqueByColumnName column is null!");
        OUString sColName;
        xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)) >>= sColName;

        Reference<XPropertySet> xIndex;
        for(sal_Int32 i=0;i<m_xIndexes->getCount();++i)
        {
            xIndex.set(m_xIndexes->getByIndex(i), css::uno::UNO_QUERY);
            if(xIndex.is() && getBOOL(xIndex->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISUNIQUE))))
            {
                Reference<XNameAccess> xCols(Reference<XColumnsSupplier>(xIndex,UNO_QUERY_THROW)->getColumns());
                if(xCols->hasByName(sColName))
                    return xIndex;

            }
        }
    }
    return Reference<XPropertySet>();
}

static double toDouble(std::string_view rString)
{
    return ::rtl::math::stringToDouble( rString, '.', ',' );
}


bool ODbaseTable::UpdateBuffer(OValueRefVector& rRow, const OValueRefRow& pOrgRow, const Reference<XIndexAccess>& _xCols, const bool bForceAllFields)
{
    OSL_ENSURE(m_pBuffer,"Buffer is NULL!");
    if ( !m_pBuffer )
        return false;
    sal_Int32 nByteOffset  = 1;

    // Update fields:
    Reference<XPropertySet> xCol;
    Reference<XPropertySet> xIndex;
    OUString aColName;
    const sal_Int32 nColumnCount = m_xColumns->getCount();
    std::vector< Reference<XPropertySet> > aIndexedCols(nColumnCount);

    ::comphelper::UStringMixEqual aCase(isCaseSensitive());

    Reference<XIndexAccess> xColumns(m_xColumns.get());
    // first search a key that exist already in the table
    for (sal_Int32 i = 0; i < nColumnCount; ++i)
    {
        sal_Int32 nPos = i;
        if(_xCols != xColumns)
        {
            m_xColumns->getByIndex(i) >>= xCol;
            OSL_ENSURE(xCol.is(),"ODbaseTable::UpdateBuffer column is null!");
            xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)) >>= aColName;

            for(nPos = 0;nPos<_xCols->getCount();++nPos)
            {
                Reference<XPropertySet> xFindCol(
                    _xCols->getByIndex(nPos), css::uno::UNO_QUERY);
                OSL_ENSURE(xFindCol.is(),"ODbaseTable::UpdateBuffer column is null!");
                if(aCase(getString(xFindCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME))),aColName))
                    break;
            }
            if (nPos >= _xCols->getCount())
                continue;
        }

        ++nPos;
        xIndex = isUniqueByColumnName(i);
        aIndexedCols[i] = xIndex;
        if (xIndex.is())
        {
            // first check if the value is different to the old one and when if it conform to the index
            if(pOrgRow.is() && (rRow[nPos]->getValue().isNull() || rRow[nPos] == (*pOrgRow)[nPos]))
                continue;
            else
            {
                ODbaseIndex* pIndex = dynamic_cast<ODbaseIndex*>(xIndex.get());
                assert(pIndex && "ODbaseTable::UpdateBuffer: No Index returned!");

                if (pIndex->Find(0,*rRow[nPos]))
                {
                    // There is no unique value
                    if ( aColName.isEmpty() )
                    {
                        m_xColumns->getByIndex(i) >>= xCol;
                        OSL_ENSURE(xCol.is(),"ODbaseTable::UpdateBuffer column is null!");
                        xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)) >>= aColName;
                        xCol.clear();
                    } // if ( !aColName.getLength() )
                    const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                            STR_DUPLICATE_VALUE_IN_COLUMN
                            ,"$columnname$", aColName
                         ) );
                    ::dbtools::throwGenericSQLException( sError, *this );
                }
            }
        }
    }

    // when we are here there is no double key in the table

    for (sal_Int32 i = 0; i < nColumnCount && nByteOffset <= m_nBufferSize ; ++i)
    {
        // Lengths for each data type:
        assert(i >= 0);
        OSL_ENSURE(o3tl::make_unsigned(i) < m_aPrecisions.size(),"Illegal index!");
        sal_Int32 nLen = 0;
        sal_Int32 nType = 0;
        sal_Int32 nScale = 0;
        if ( o3tl::make_unsigned(i) < m_aPrecisions.size() )
        {
            nLen    = m_aPrecisions[i];
            nType   = m_aTypes[i];
            nScale  = m_aScales[i];
        }
        else
        {
            m_xColumns->getByIndex(i) >>= xCol;
            if ( xCol.is() )
            {
                xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PRECISION)) >>= nLen;
                xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE))      >>= nType;
                xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE))     >>= nScale;
            }
        }

        bool bSetZero = false;
        switch (nType)
        {
            case DataType::INTEGER:
            case DataType::DOUBLE:
            case DataType::TIMESTAMP:
                bSetZero = true;
                [[fallthrough]];
            case DataType::LONGVARBINARY:
            case DataType::DATE:
            case DataType::BIT:
            case DataType::LONGVARCHAR:
                nLen = m_aRealFieldLengths[i];
                break;
            case DataType::DECIMAL:
                nLen = SvDbaseConverter::ConvertPrecisionToDbase(nLen,nScale);
                break;  // The sign and the comma
            default:
                break;

        } // switch (nType)

        sal_Int32 nPos = i;
        if(_xCols != xColumns)
        {
            m_xColumns->getByIndex(i) >>= xCol;
            OSL_ENSURE(xCol.is(),"ODbaseTable::UpdateBuffer column is null!");
            xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)) >>= aColName;
            for(nPos = 0;nPos<_xCols->getCount();++nPos)
            {
                Reference<XPropertySet> xFindCol(
                    _xCols->getByIndex(nPos), css::uno::UNO_QUERY);
                if(aCase(getString(xFindCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME))),aColName))
                    break;
            }
            if (nPos >= _xCols->getCount())
            {
                nByteOffset += nLen;
                continue;
            }
        }


        ++nPos; // the row values start at 1
        const ORowSetValue &thisColVal = rRow[nPos]->get();
        const bool thisColIsBound = thisColVal.isBound();
        const bool thisColIsNull = !thisColIsBound || thisColVal.isNull();
        // don't overwrite non-bound columns
        if ( ! (bForceAllFields || thisColIsBound) )
        {
            // No - don't overwrite this field, it has not changed.
            nByteOffset += nLen;
            continue;
        }
        if (aIndexedCols[i].is())
        {
            ODbaseIndex* pIndex = dynamic_cast<ODbaseIndex*>(aIndexedCols[i].get());
            assert(pIndex && "ODbaseTable::UpdateBuffer: No Index returned!");
            // Update !!
            if (pOrgRow.is() && !thisColIsNull)
                pIndex->Update(m_nFilePos, *(*pOrgRow)[nPos], thisColVal);
            else
                pIndex->Insert(m_nFilePos, thisColVal);
        }

        char* pData = reinterpret_cast<char *>(m_pBuffer.get() + nByteOffset);
        if (thisColIsNull)
        {
            if ( bSetZero )
                memset(pData,0,nLen);   // Clear to NULL char ('\0')
            else
                memset(pData,' ',nLen); // Clear to space/blank ('\0x20')
            nByteOffset += nLen;
            OSL_ENSURE( nByteOffset <= m_nBufferSize ,"ByteOffset > m_nBufferSize!");
            continue;
        }

        try
        {
            switch (nType)
            {
                case DataType::TIMESTAMP:
                    {
                        sal_Int32 nJulianDate = 0, nJulianTime = 0;
                        lcl_CalcJulDate(nJulianDate,nJulianTime, thisColVal.getDateTime());
                        // Exactly 8 bytes to copy:
                        memcpy(pData,&nJulianDate,4);
                        memcpy(pData+4,&nJulianTime,4);
                    }
                    break;
                case DataType::DATE:
                {
                    css::util::Date aDate;
                    if(thisColVal.getTypeKind() == DataType::DOUBLE)
                        aDate = ::dbtools::DBTypeConversion::toDate(thisColVal.getDouble());
                    else
                        aDate = thisColVal.getDate();
                    char s[sizeof("-327686553565535")];
                        // reserve enough space for hypothetical max length
                    snprintf(s,
                        sizeof(s),
                        "%04" SAL_PRIdINT32 "%02" SAL_PRIuUINT32 "%02" SAL_PRIuUINT32,
                        static_cast<sal_Int32>(aDate.Year),
                        static_cast<sal_uInt32>(aDate.Month),
                        static_cast<sal_uInt32>(aDate.Day));

                    // Exactly 8 bytes to copy (even if s could hypothetically be longer):
                    memcpy(pData,s,8);
                } break;
                case DataType::INTEGER:
                    {
                        sal_Int32 nValue = thisColVal.getInt32();
                        if (o3tl::make_unsigned(nLen) > sizeof(nValue))
                            return false;
                        memcpy(pData,&nValue,nLen);
                    }
                    break;
                case DataType::DOUBLE:
                    {
                        const double d = thisColVal.getDouble();
                        m_xColumns->getByIndex(i) >>= xCol;

                        if (getBOOL(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISCURRENCY)))) // Currency is treated separately
                        {
                            sal_Int64 nValue = 0;
                            if ( m_aScales[i] )
                                nValue = static_cast<sal_Int64>(d * pow(10.0,static_cast<int>(m_aScales[i])));
                            else
                                nValue = static_cast<sal_Int64>(d);
                            if (o3tl::make_unsigned(nLen) > sizeof(nValue))
                                return false;
                            memcpy(pData,&nValue,nLen);
                        } // if (getBOOL(xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ISCURRENCY)))) // Currency is treated separately
                        else
                        {
                            if (o3tl::make_unsigned(nLen) > sizeof(d))
                                return false;
                            memcpy(pData,&d,nLen);
                        }
                    }
                    break;
                case DataType::DECIMAL:
                {
                    memset(pData,' ',nLen); // Clear to NULL

                    const double n = thisColVal.getDouble();

                    // one, because const_cast GetFormatPrecision on SvNumberFormat is not constant,
                    // even though it really could and should be
                    const OString aDefaultValue( ::rtl::math::doubleToString( n, rtl_math_StringFormat_F, nScale, '.', nullptr, 0));
                    const sal_Int32 nValueLen = aDefaultValue.getLength();
                    if ( nValueLen <= nLen )
                    {
                        // Write value right-justified, padded with blanks to the left.
                        memcpy(pData+nLen-nValueLen,aDefaultValue.getStr(),nValueLen);
                        // write the resulting double back
                        *rRow[nPos] = toDouble(aDefaultValue);
                    }
                    else
                    {
                        m_xColumns->getByIndex(i) >>= xCol;
                        OSL_ENSURE(xCol.is(),"ODbaseTable::UpdateBuffer column is null!");
                        xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)) >>= aColName;
                        std::vector< std::pair<const char* , OUString > > aStringToSubstitutes
                        {
                            { "$columnname$", aColName },
                            { "$precision$", OUString::number(nLen) },
                            { "$scale$", OUString::number(nScale) },
                            { "$value$", OStringToOUString(aDefaultValue,RTL_TEXTENCODING_UTF8) }
                        };

                        const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                                STR_INVALID_COLUMN_DECIMAL_VALUE
                                ,aStringToSubstitutes
                             ) );
                        ::dbtools::throwGenericSQLException( sError, *this );
                    }
                } break;
                case DataType::BIT:
                    *pData = thisColVal.getBool() ? 'T' : 'F';
                    break;
                case DataType::LONGVARBINARY:
                case DataType::LONGVARCHAR:
                {
                    char cNext = pData[nLen]; // Mark's scratch and replaced by 0
                    pData[nLen] = '\0';       // This is because the buffer is always a sign of greater ...

                    std::size_t nBlockNo = strtol(pData,nullptr,10); // Block number read

                    // Next initial character restore again:
                    pData[nLen] = cNext;
                    if (!m_pMemoStream)
                        break;
                    WriteMemo(thisColVal, nBlockNo);

                    OString aBlock(OString::number(nBlockNo));
                    //align aBlock at the right of a nLen sequence, fill to the left with '0'
                    OStringBuffer aStr;
                    comphelper::string::padToLength(aStr, nLen - aBlock.getLength(), '0');
                    aStr.append(aBlock);

                    // Copy characters:
                    memcpy(pData, aStr.getStr(), nLen);
                }   break;
                default:
                {
                    memset(pData,' ',nLen); // Clear to NULL

                    OUString sStringToWrite( thisColVal.getString() );

                    // convert the string, using the connection's encoding
                    OString sEncoded;

                    DBTypeConversion::convertUnicodeStringToLength( sStringToWrite, sEncoded, nLen, m_eEncoding );
                    memcpy( pData, sEncoded.getStr(), sEncoded.getLength() );

                }
                break;
            }
        }
        catch( const SQLException&  )
        {
            throw;
        }
        catch ( const Exception& )
        {
            m_xColumns->getByIndex(i) >>= xCol;
            OSL_ENSURE( xCol.is(), "ODbaseTable::UpdateBuffer column is null!" );
            if ( xCol.is() )
                xCol->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)) >>= aColName;

            const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                    STR_INVALID_COLUMN_VALUE,
                    "$columnname$", aColName
                 ) );
            ::dbtools::throwGenericSQLException( sError, *this );
        }
        // And more ...
        nByteOffset += nLen;
        OSL_ENSURE( nByteOffset <= m_nBufferSize ,"ByteOffset > m_nBufferSize!");
    }
    return true;
}


void ODbaseTable::WriteMemo(const ORowSetValue& aVariable, std::size_t& rBlockNr)
{
    // if the BlockNo 0 is given, the block will be appended at the end
    std::size_t nSize = 0;
    OString aStr;
    css::uno::Sequence<sal_Int8> aValue;
    sal_uInt8 nHeader[4];
    const bool bBinary = aVariable.getTypeKind() == DataType::LONGVARBINARY && m_aMemoHeader.db_typ == MemoFoxPro;
    if ( bBinary )
    {
        aValue = aVariable.getSequence();
        nSize = aValue.getLength();
    }
    else
    {
        nSize = DBTypeConversion::convertUnicodeString( aVariable.getString(), aStr, m_eEncoding );
    }

    // append or overwrite
    bool bAppend = rBlockNr == 0;

    if (!bAppend)
    {
        switch (m_aMemoHeader.db_typ)
        {
            case MemodBaseIII: // dBase III-Memofield, ends with 2 * Ctrl-Z
                bAppend = nSize > (512 - 2);
                break;
            case MemoFoxPro:
            case MemodBaseIV: // dBase IV-Memofield with length
            {
                char sHeader[4];
                m_pMemoStream->Seek(rBlockNr * m_aMemoHeader.db_size);
                m_pMemoStream->SeekRel(4);
                m_pMemoStream->ReadBytes(sHeader, 4);

                std::size_t nOldSize;
                if (m_aMemoHeader.db_typ == MemoFoxPro)
                    nOldSize = ((static_cast<unsigned char>(sHeader[0]) * 256 +
                                 static_cast<unsigned char>(sHeader[1])) * 256 +
                                 static_cast<unsigned char>(sHeader[2])) * 256 +
                                 static_cast<unsigned char>(sHeader[3]);
                else
                    nOldSize = ((static_cast<unsigned char>(sHeader[3]) * 256 +
                                 static_cast<unsigned char>(sHeader[2])) * 256 +
                                 static_cast<unsigned char>(sHeader[1])) * 256 +
                                 static_cast<unsigned char>(sHeader[0])  - 8;

                // fits the new length in the used blocks
                std::size_t nUsedBlocks = ((nSize + 8) / m_aMemoHeader.db_size) + (((nSize + 8) % m_aMemoHeader.db_size > 0) ? 1 : 0),
                      nOldUsedBlocks = ((nOldSize + 8) / m_aMemoHeader.db_size) + (((nOldSize + 8) % m_aMemoHeader.db_size > 0) ? 1 : 0);
                bAppend = nUsedBlocks > nOldUsedBlocks;
            }
        }
    }

    if (bAppend)
    {
        sal_uInt64 const nStreamSize = m_pMemoStream->TellEnd();
        // fill last block
        rBlockNr = (nStreamSize / m_aMemoHeader.db_size) + ((nStreamSize % m_aMemoHeader.db_size) > 0 ? 1 : 0);

        m_pMemoStream->SetStreamSize(rBlockNr * m_aMemoHeader.db_size);
        m_pMemoStream->Seek(STREAM_SEEK_TO_END);
    }
    else
    {
        m_pMemoStream->Seek(rBlockNr * m_aMemoHeader.db_size);
    }

    switch (m_aMemoHeader.db_typ)
    {
        case MemodBaseIII: // dBase III-Memofield, ends with Ctrl-Z
        {
            const char cEOF = char(DBF_EOL);
            nSize++;
            m_pMemoStream->WriteBytes(aStr.getStr(), aStr.getLength());
            m_pMemoStream->WriteChar( cEOF ).WriteChar( cEOF );
        } break;
        case MemoFoxPro:
        case MemodBaseIV: // dBase IV-Memofield with length
        {
            if ( MemodBaseIV == m_aMemoHeader.db_typ )
                (*m_pMemoStream).WriteUChar( 0xFF )
                                .WriteUChar( 0xFF )
                                .WriteUChar( 0x08 );
            else
                (*m_pMemoStream).WriteUChar( 0x00 )
                                .WriteUChar( 0x00 )
                                .WriteUChar( 0x00 );

            sal_uInt32 nWriteSize = nSize;
            if (m_aMemoHeader.db_typ == MemoFoxPro)
            {
                if ( bBinary )
                    (*m_pMemoStream).WriteUChar( 0x00 ); // Picture
                else
                    (*m_pMemoStream).WriteUChar( 0x01 ); // Memo
                for (int i = 4; i > 0; nWriteSize >>= 8)
                    nHeader[--i] = static_cast<sal_uInt8>(nWriteSize % 256);
            }
            else
            {
                (*m_pMemoStream).WriteUChar( 0x00 );
                nWriteSize += 8;
                for (int i = 0; i < 4; nWriteSize >>= 8)
                    nHeader[i++] = static_cast<sal_uInt8>(nWriteSize % 256);
            }

            m_pMemoStream->WriteBytes(nHeader, 4);
            if ( bBinary )
                m_pMemoStream->WriteBytes(aValue.getConstArray(), aValue.getLength());
            else
                m_pMemoStream->WriteBytes(aStr.getStr(), aStr.getLength());
            m_pMemoStream->Flush();
        }
    }


    // Write the new block number
    if (bAppend)
    {
        sal_uInt64 const nStreamSize = m_pMemoStream->TellEnd();
        m_aMemoHeader.db_next = (nStreamSize / m_aMemoHeader.db_size) + ((nStreamSize % m_aMemoHeader.db_size) > 0 ? 1 : 0);

        // Write the new block number
        m_pMemoStream->Seek(0);
        (*m_pMemoStream).WriteUInt32( m_aMemoHeader.db_next );
        m_pMemoStream->Flush();
    }
}


// XAlterTable
void SAL_CALL ODbaseTable::alterColumnByName( const OUString& colName, const Reference< XPropertySet >& descriptor )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);


    Reference<XDataDescriptorFactory> xOldColumn;
    m_xColumns->getByName(colName) >>= xOldColumn;

    try
    {
        alterColumn(m_xColumns->findColumn(colName)-1,descriptor,xOldColumn);
    }
    catch (const css::lang::IndexOutOfBoundsException&)
    {
        throw NoSuchElementException(colName, *this);
    }
}

void SAL_CALL ODbaseTable::alterColumnByIndex( sal_Int32 index, const Reference< XPropertySet >& descriptor )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);

    if(index < 0 || index >= m_xColumns->getCount())
        throw IndexOutOfBoundsException(OUString::number(index),*this);

    Reference<XDataDescriptorFactory> xOldColumn;
    m_xColumns->getByIndex(index) >>= xOldColumn;
    alterColumn(index,descriptor,xOldColumn);
}

void ODbaseTable::alterColumn(sal_Int32 index,
                              const Reference< XPropertySet >& descriptor ,
                              const Reference< XDataDescriptorFactory >& xOldColumn )
{
    if(index < 0 || index >= m_xColumns->getCount())
        throw IndexOutOfBoundsException(OUString::number(index),*this);

    try
    {
        OSL_ENSURE(descriptor.is(),"ODbaseTable::alterColumn: descriptor can not be null!");
        // creates a copy of the original column and copy all properties from descriptor in xCopyColumn
        Reference<XPropertySet> xCopyColumn;
        if(xOldColumn.is())
            xCopyColumn = xOldColumn->createDataDescriptor();
        else
            xCopyColumn = new OColumn(getConnection()->getMetaData()->supportsMixedCaseQuotedIdentifiers());

        ::comphelper::copyProperties(descriptor,xCopyColumn);

        // creates a temp file

        OUString sTempName = createTempFile();

        rtl::Reference<ODbaseTable> pNewTable = new ODbaseTable(m_pTables,static_cast<ODbaseConnection*>(m_pConnection));
        pNewTable->setPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME),Any(sTempName));
        Reference<XAppend> xAppend(pNewTable->getColumns(),UNO_QUERY);
        OSL_ENSURE(xAppend.is(),"ODbaseTable::alterColumn: No XAppend interface!");

        // copy the structure
        sal_Int32 i=0;
        for(;i < index;++i)
        {
            Reference<XPropertySet> xProp;
            m_xColumns->getByIndex(i) >>= xProp;
            Reference<XDataDescriptorFactory> xColumn(xProp,UNO_QUERY);
            Reference<XPropertySet> xCpy;
            if(xColumn.is())
                xCpy = xColumn->createDataDescriptor();
            else
                xCpy = new OColumn(getConnection()->getMetaData()->supportsMixedCaseQuotedIdentifiers());
            ::comphelper::copyProperties(xProp,xCpy);
            xAppend->appendByDescriptor(xCpy);
        }
        ++i; // now insert our new column
        xAppend->appendByDescriptor(xCopyColumn);

        for(;i < m_xColumns->getCount();++i)
        {
            Reference<XPropertySet> xProp;
            m_xColumns->getByIndex(i) >>= xProp;
            Reference<XDataDescriptorFactory> xColumn(xProp,UNO_QUERY);
            Reference<XPropertySet> xCpy;
            if(xColumn.is())
                xCpy = xColumn->createDataDescriptor();
            else
                xCpy = new OColumn(getConnection()->getMetaData()->supportsMixedCaseQuotedIdentifiers());
            ::comphelper::copyProperties(xProp,xCpy);
            xAppend->appendByDescriptor(xCpy);
        }

        // construct the new table
        if(!pNewTable->CreateImpl())
        {
            const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                    STR_COLUMN_NOT_ALTERABLE,
                    "$columnname$", ::comphelper::getString(descriptor->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)))
                 ) );
            ::dbtools::throwGenericSQLException( sError, *this );
        }

        pNewTable->construct();

        // copy the data
        copyData(pNewTable.get(),0);

        // now drop the old one
        if( DropImpl() ) // we don't want to delete the memo columns too
        {
            try
            {
                // rename the new one to the old one
                pNewTable->renameImpl(m_Name);
            }
            catch(const css::container::ElementExistException&)
            {
                const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                        STR_COULD_NOT_DELETE_FILE,
                        "$filename$", m_Name
                     ) );
                ::dbtools::throwGenericSQLException( sError, *this );
            }
            // release the temp file
            ::comphelper::disposeComponent(pNewTable);
            pNewTable = nullptr;
        }
        else
        {
            pNewTable = nullptr;
        }
        FileClose();
        construct();
        if(m_xColumns)
            m_xColumns->refresh();

    }
    catch(const SQLException&)
    {
        throw;
    }
    catch(const Exception&)
    {
        TOOLS_WARN_EXCEPTION( "connectivity.drivers","");
        throw;
    }
}

Reference< XDatabaseMetaData> ODbaseTable::getMetaData() const
{
    return getConnection()->getMetaData();
}

void SAL_CALL ODbaseTable::rename( const OUString& newName )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(OTableDescriptor_BASE::rBHelper.bDisposed);
    if(m_pTables && m_pTables->hasByName(newName))
        throw ElementExistException(newName,*this);


    renameImpl(newName);

    ODbaseTable_BASE::rename(newName);

    construct();
    if(m_xColumns)
        m_xColumns->refresh();
}
namespace
{
    void renameFile(file::OConnection const * _pConnection,std::u16string_view oldName,
                    const OUString& newName, std::u16string_view _sExtension)
    {
        OUString aName = ODbaseTable::getEntry(_pConnection,oldName);
        if(aName.isEmpty())
        {
            OUString aIdent = _pConnection->getContent()->getIdentifier()->getContentIdentifier();
            if ( aIdent.lastIndexOf('/') != (aIdent.getLength()-1) )
                aIdent += "/";
            aIdent += oldName;
            aName = aIdent;
        }
        INetURLObject aURL;
        aURL.SetURL(aName);

        aURL.setExtension( _sExtension );
        OUString sNewName(newName + "." + _sExtension);

        try
        {
            Content aContent(aURL.GetMainURL(INetURLObject::DecodeMechanism::NONE),Reference<XCommandEnvironment>(), comphelper::getProcessComponentContext());

            Sequence< PropertyValue > aProps{ { u"Title"_ustr,
                                                -1, // n/a
                                                Any(sNewName),
                                                css::beans::PropertyState_DIRECT_VALUE } };
            Sequence< Any > aValues;
            aContent.executeCommand( u"setPropertyValues"_ustr,Any(aProps) ) >>= aValues;
            if(aValues.hasElements() && aValues[0].hasValue())
                throw Exception(u"setPropertyValues returned non-zero"_ustr, nullptr);
        }
        catch(const Exception&)
        {
            throw ElementExistException(newName);
        }
    }
}

void ODbaseTable::renameImpl( const OUString& newName )
{
    ::osl::MutexGuard aGuard(m_aMutex);

    FileClose();


    renameFile(m_pConnection,m_Name,newName,m_pConnection->getExtension());
    if ( HasMemoFields() )
    {  // delete the memo fields
        renameFile(m_pConnection,m_Name,newName,u"dbt");
    }
}

void ODbaseTable::addColumn(const Reference< XPropertySet >& _xNewColumn)
{
    OUString sTempName = createTempFile();

    rtl::Reference xNewTable(new ODbaseTable(m_pTables,static_cast<ODbaseConnection*>(m_pConnection)));
    xNewTable->setPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME),Any(sTempName));
    {
        Reference<XAppend> xAppend(xNewTable->getColumns(),UNO_QUERY);
        bool bCase = getConnection()->getMetaData()->supportsMixedCaseQuotedIdentifiers();
        // copy the structure
        for(sal_Int32 i=0;i < m_xColumns->getCount();++i)
        {
            Reference<XPropertySet> xProp;
            m_xColumns->getByIndex(i) >>= xProp;
            Reference<XDataDescriptorFactory> xColumn(xProp,UNO_QUERY);
            Reference<XPropertySet> xCpy;
            if(xColumn.is())
                xCpy = xColumn->createDataDescriptor();
            else
            {
                xCpy = new OColumn(bCase);
                ::comphelper::copyProperties(xProp,xCpy);
            }

            xAppend->appendByDescriptor(xCpy);
        }
        Reference<XPropertySet> xCpy = new OColumn(bCase);
        ::comphelper::copyProperties(_xNewColumn,xCpy);
        xAppend->appendByDescriptor(xCpy);
    }

    // construct the new table
    if(!xNewTable->CreateImpl())
    {
        const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                STR_COLUMN_NOT_ADDABLE,
                "$columnname$", ::comphelper::getString(_xNewColumn->getPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME)))
             ) );
        ::dbtools::throwGenericSQLException( sError, *this );
    }

    xNewTable->construct();
    // copy the data
    copyData(xNewTable.get(),xNewTable->m_xColumns->getCount());
    // drop the old table
    if(DropImpl())
    {
        xNewTable->renameImpl(m_Name);
        // release the temp file
    }
    xNewTable.clear();

    FileClose();
    construct();
    if(m_xColumns)
        m_xColumns->refresh();
}

void ODbaseTable::dropColumn(sal_Int32 _nPos)
{
    OUString sTempName = createTempFile();

    rtl::Reference xNewTable(new ODbaseTable(m_pTables,static_cast<ODbaseConnection*>(m_pConnection)));
    xNewTable->setPropertyValue(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME),Any(sTempName));
    {
        Reference<XAppend> xAppend(xNewTable->getColumns(),UNO_QUERY);
        bool bCase = getConnection()->getMetaData()->supportsMixedCaseQuotedIdentifiers();
        // copy the structure
        for(sal_Int32 i=0;i < m_xColumns->getCount();++i)
        {
            if(_nPos != i)
            {
                Reference<XPropertySet> xProp;
                m_xColumns->getByIndex(i) >>= xProp;
                Reference<XDataDescriptorFactory> xColumn(xProp,UNO_QUERY);
                Reference<XPropertySet> xCpy;
                if(xColumn.is())
                    xCpy = xColumn->createDataDescriptor();
                else
                {
                    xCpy = new OColumn(bCase);
                    ::comphelper::copyProperties(xProp,xCpy);
                }
                xAppend->appendByDescriptor(xCpy);
            }
        }
    }

    // construct the new table
    if(!xNewTable->CreateImpl())
    {
        const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                STR_COLUMN_NOT_DROP,
                "$position$", OUString::number(_nPos)
             ) );
        ::dbtools::throwGenericSQLException( sError, *this );
    }
    xNewTable->construct();
    // copy the data
    copyData(xNewTable.get(),_nPos);
    // drop the old table
    if(DropImpl())
        xNewTable->renameImpl(m_Name);
        // release the temp file

    xNewTable.clear();

    FileClose();
    construct();
}

OUString ODbaseTable::createTempFile()
{
    OUString aIdent = m_pConnection->getContent()->getIdentifier()->getContentIdentifier();
    if ( aIdent.lastIndexOf('/') != (aIdent.getLength()-1) )
        aIdent += "/";

    OUString sExt("." + m_pConnection->getExtension());
    OUString aTempFileURL = utl::CreateTempURL(m_Name, true, sExt, &aIdent);
    if(aTempFileURL.isEmpty())
        getConnection()->throwGenericSQLException(STR_COULD_NOT_ALTER_TABLE, *this);

    INetURLObject aURL;
    aURL.SetSmartProtocol(INetProtocol::File);
    aURL.SetURL(aTempFileURL);

    OUString sNewName(aURL.getName().copy(0, aURL.getName().getLength() - sExt.getLength()));

    return sNewName;
}

void ODbaseTable::copyData(ODbaseTable* _pNewTable,sal_Int32 _nPos)
{
    sal_Int32 nPos = _nPos + 1; // +1 because we always have the bookmark column as well
    OValueRefRow aRow = new OValueRefVector(m_xColumns->getCount());
    OValueRefRow aInsertRow;
    if(_nPos)
    {
        aInsertRow = new OValueRefVector(_pNewTable->m_xColumns->getCount());
        std::for_each(aInsertRow->begin(),aInsertRow->end(),TSetRefBound(true));
    }
    else
        aInsertRow = aRow;

    // we only have to bind the values which we need to copy into the new table
    std::for_each(aRow->begin(),aRow->end(),TSetRefBound(true));
    if(_nPos && (_nPos < static_cast<sal_Int32>(aRow->size())))
        (*aRow)[nPos]->setBound(false);


    sal_Int32 nCurPos;
    OValueRefVector::const_iterator aIter;
    for(sal_uInt32 nRowPos = 0; nRowPos < m_aHeader.nbRecords;++nRowPos)
    {
        bool bOk = seekRow( IResultSetHelper::BOOKMARK, nRowPos+1, nCurPos );
        if ( bOk )
        {
            bOk = fetchRow( aRow, *m_aColumns, true);
            if ( bOk && !aRow->isDeleted() ) // copy only not deleted rows
            {
                // special handling when pos == 0 then we don't have to distinguish between the two rows
                if(_nPos)
                {
                    aIter = aRow->begin()+1;
                    sal_Int32 nCount = 1;
                    for(OValueRefVector::iterator aInsertIter = aInsertRow->begin()+1; aIter != aRow->end() && aInsertIter != aInsertRow->end();++aIter,++nCount)
                    {
                        if(nPos != nCount)
                        {
                            (*aInsertIter)->setValue( (*aIter)->getValue() );
                            ++aInsertIter;
                        }
                    }
                }
                bOk = _pNewTable->InsertRow(*aInsertRow, _pNewTable->m_xColumns.get());
                SAL_WARN_IF(!bOk, "connectivity.drivers", "Row could not be inserted!");
            }
            else
            {
                SAL_WARN_IF(!bOk, "connectivity.drivers", "Row could not be fetched!");
            }
        }
        else
        {
            OSL_ASSERT(false);
        }
    } // for(sal_uInt32 nRowPos = 0; nRowPos < m_aHeader.db_anz;++nRowPos)
}

void ODbaseTable::throwInvalidDbaseFormat()
{
    FileClose();
    // no dbase file

    const OUString sError( getConnection()->getResources().getResourceStringWithSubstitution(
                STR_INVALID_DBASE_FILE,
                "$filename$", getEntry(m_pConnection,m_Name)
             ) );
    ::dbtools::throwGenericSQLException( sError, *this );
}

void ODbaseTable::refreshHeader()
{
    if ( m_aHeader.nbRecords == 0 )
        readHeader();
}

bool ODbaseTable::seekRow(IResultSetHelper::Movement eCursorPosition, sal_Int32 nOffset, sal_Int32& nCurPos)
{
    // prepare positioning:
    OSL_ENSURE(m_pFileStream,"ODbaseTable::seekRow: FileStream is NULL!");

    sal_uInt32  nNumberOfRecords = m_aHeader.nbRecords;
    sal_uInt32 nTempPos = m_nFilePos;
    m_nFilePos = nCurPos;

    switch(eCursorPosition)
    {
        case IResultSetHelper::NEXT:
            ++m_nFilePos;
            break;
        case IResultSetHelper::PRIOR:
            if (m_nFilePos > 0)
                --m_nFilePos;
            break;
        case IResultSetHelper::FIRST:
            m_nFilePos = 1;
            break;
        case IResultSetHelper::LAST:
            m_nFilePos = nNumberOfRecords;
            break;
        case IResultSetHelper::RELATIVE1:
            m_nFilePos = (m_nFilePos + nOffset < 0) ? 0
                            : static_cast<sal_uInt32>(m_nFilePos + nOffset);
            break;
        case IResultSetHelper::ABSOLUTE1:
        case IResultSetHelper::BOOKMARK:
            m_nFilePos = static_cast<sal_uInt32>(nOffset);
            break;
    }

    if (m_nFilePos > static_cast<sal_Int32>(nNumberOfRecords))
        m_nFilePos = static_cast<sal_Int32>(nNumberOfRecords) + 1;

    if (m_nFilePos == 0 || m_nFilePos == static_cast<sal_Int32>(nNumberOfRecords) + 1)
        goto Error;
    else
    {
        std::size_t nEntryLen = m_aHeader.recordLength;

        OSL_ENSURE(m_nFilePos >= 1,"SdbDBFCursor::FileFetchRow: invalid record position");
        std::size_t nPos = m_aHeader.headerLength + static_cast<std::size_t>(m_nFilePos-1) * nEntryLen;

        m_pFileStream->Seek(nPos);
        if (m_pFileStream->GetError() != ERRCODE_NONE)
            goto Error;

        std::size_t nRead = m_pFileStream->ReadBytes(m_pBuffer.get(), nEntryLen);
        if (nRead != nEntryLen)
        {
            SAL_WARN("connectivity.drivers", "ODbaseTable::seekRow: short read!");
            goto Error;
        }
        if (m_pFileStream->GetError() != ERRCODE_NONE)
            goto Error;
    }
    goto End;

Error:
    switch(eCursorPosition)
    {
        case IResultSetHelper::PRIOR:
        case IResultSetHelper::FIRST:
            m_nFilePos = 0;
            break;
        case IResultSetHelper::LAST:
        case IResultSetHelper::NEXT:
        case IResultSetHelper::ABSOLUTE1:
        case IResultSetHelper::RELATIVE1:
            if (nOffset > 0)
                m_nFilePos = nNumberOfRecords + 1;
            else if (nOffset < 0)
                m_nFilePos = 0;
            break;
        case IResultSetHelper::BOOKMARK:
            m_nFilePos = nTempPos;   // last position
    }
    return false;

End:
    nCurPos = m_nFilePos;
    return true;
}

bool ODbaseTable::ReadMemo(std::size_t nBlockNo, ORowSetValue& aVariable)
{
    m_pMemoStream->Seek(nBlockNo * m_aMemoHeader.db_size);
    switch (m_aMemoHeader.db_typ)
    {
        case MemodBaseIII: // dBase III-Memofield, ends with Ctrl-Z
        {
            const char cEOF = char(DBF_EOL);
            OStringBuffer aBStr;
            static char aBuf[514];
            aBuf[512] = 0;          // avoid random value
            bool bReady = false;

            do
            {
                m_pMemoStream->ReadBytes(&aBuf, 512);

                sal_uInt16 i = 0;
                while (aBuf[i] != cEOF && ++i < 512)
                    ;
                bReady = aBuf[i] == cEOF;

                aBuf[i] = 0;
                aBStr.append(aBuf);

            } while (!bReady && !m_pMemoStream->eof());

            aVariable = OStringToOUString(aBStr,
                m_eEncoding);

        } break;
        case MemoFoxPro:
        case MemodBaseIV: // dBase IV-Memofield with length
        {
            bool bIsText = true;
            char sHeader[4];
            m_pMemoStream->ReadBytes(sHeader, 4);
            // Foxpro stores text and binary data
            if (m_aMemoHeader.db_typ == MemoFoxPro)
            {
                bIsText = sHeader[3] != 0;
            }
            else if (static_cast<sal_uInt8>(sHeader[0]) != 0xFF || static_cast<sal_uInt8>(sHeader[1]) != 0xFF || static_cast<sal_uInt8>(sHeader[2]) != 0x08)
            {
                return false;
            }

            sal_uInt32 nLength(0);
            (*m_pMemoStream).ReadUInt32( nLength );

            if (m_aMemoHeader.db_typ == MemodBaseIV)
            {
                if (nLength < 8)
                {
                    SAL_WARN("connectivity.drivers", "Size too small");
                    return false;
                }
                nLength -= 8;
            }

            if ( nLength )
            {
                if ( bIsText )
                {
                    OStringBuffer aBuffer(read_uInt8s_ToOString(*m_pMemoStream, nLength));
                    //pad it out with ' ' to expected length on short read
                    sal_Int32 nRequested = sal::static_int_cast<sal_Int32>(nLength);
                    comphelper::string::padToLength(aBuffer, nRequested, ' ');
                    aVariable = OStringToOUString(aBuffer, m_eEncoding);
                } // if ( bIsText )
                else
                {
                    css::uno::Sequence< sal_Int8 > aData(nLength);
                    m_pMemoStream->ReadBytes(aData.getArray(), nLength);
                    aVariable = aData;
                }
            } // if ( nLength )
        }
    }
    return true;
}

bool ODbaseTable::AllocBuffer()
{
    sal_uInt16 nSize = m_aHeader.recordLength;
    SAL_WARN_IF(nSize == 0, "connectivity.drivers", "Size too small");

    if (m_nBufferSize != nSize)
    {
        m_pBuffer.reset();
    }

    // if there is no buffer available: allocate:
    if (!m_pBuffer && nSize > 0)
    {
        m_nBufferSize = nSize;
        m_pBuffer.reset(new sal_uInt8[m_nBufferSize+1]);
    }

    return m_pBuffer != nullptr;
}

bool ODbaseTable::WriteBuffer()
{
    OSL_ENSURE(m_nFilePos >= 1,"SdbDBFCursor::FileFetchRow: invalid record position");

    // position on desired record:
    std::size_t nPos = m_aHeader.headerLength + static_cast<tools::Long>(m_nFilePos-1) * m_aHeader.recordLength;
    m_pFileStream->Seek(nPos);
    return m_pFileStream->WriteBytes(m_pBuffer.get(), m_aHeader.recordLength) > 0;
}

sal_Int32 ODbaseTable::getCurrentLastPos() const
{
    return m_aHeader.nbRecords;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
