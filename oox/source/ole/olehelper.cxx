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

#include <oox/ole/olehelper.hxx>

#include <rtl/ustrbuf.hxx>
#include <sot/storage.hxx>
#include <osl/diagnose.h>
#include <oox/helper/binaryinputstream.hxx>
#include <oox/helper/binaryoutputstream.hxx>
#include <oox/helper/graphichelper.hxx>
#include <oox/token/properties.hxx>
#include <oox/token/tokens.hxx>
#include <oox/ole/axcontrol.hxx>
#include <oox/helper/propertymap.hxx>
#include <oox/helper/propertyset.hxx>

#include <com/sun/star/awt/XControlModel.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/form/FormComponentType.hpp>
#include <com/sun/star/form/XFormComponent.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <tools/globname.hxx>
#include <unotools/streamwrap.hxx>
#include <comphelper/processfactory.hxx>
#include <utility>

namespace oox::ole {

using ::com::sun::star::form::XFormComponent;
using ::com::sun::star::awt::XControlModel;
using ::com::sun::star::awt::Size;
using ::com::sun::star::frame::XModel;
using ::com::sun::star::io::XOutputStream;
using ::com::sun::star::io::XInputStream;
using ::com::sun::star::beans::XPropertySet;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::UNO_QUERY;
using ::com::sun::star::uno::XComponentContext;
using ::com::sun::star::lang::XServiceInfo;

using namespace ::com::sun::star::form;

namespace {

const sal_uInt32 OLE_COLORTYPE_MASK         = 0xFF000000;
const sal_uInt32 OLE_COLORTYPE_CLIENT       = 0x00000000;
const sal_uInt32 OLE_COLORTYPE_PALETTE      = 0x01000000;
const sal_uInt32 OLE_COLORTYPE_BGR          = 0x02000000;
const sal_uInt32 OLE_COLORTYPE_SYSCOLOR     = 0x80000000;

const sal_uInt32 OLE_PALETTECOLOR_MASK      = 0x0000FFFF;
const sal_uInt32 OLE_SYSTEMCOLOR_MASK       = 0x0000FFFF;

/** Swaps the red and blue component of the passed color. */
sal_uInt32 lclSwapRedBlue( sal_uInt32 nColor )
{
    return static_cast< sal_uInt32 >( (nColor & 0xFF00FF00) | ((nColor & 0x0000FF) << 16) | ((nColor & 0xFF0000) >> 16) );
}

/** Returns the UNO RGB color from the passed encoded OLE BGR color. */
::Color lclDecodeBgrColor( sal_uInt32 nOleColor )
{
    return ::Color( ColorTransparency, lclSwapRedBlue( nOleColor ) & 0xFFFFFF );
}

const sal_uInt32 OLE_STDPIC_ID              = 0x0000746C;

struct GUIDCNamePair
{
    const char* sGUID;
    const char* sName;
};

struct IdCntrlData
{
    sal_Int16 nId;
    GUIDCNamePair aData;
};

const sal_Int16 TOGGLEBUTTON = -1;
const sal_Int16 FORMULAFIELD = -2;

typedef std::map< sal_Int16, GUIDCNamePair > GUIDCNamePairMap;
class classIdToGUIDCNamePairMap
{
    GUIDCNamePairMap mnIdToGUIDCNamePairMap;
    classIdToGUIDCNamePairMap();
public:
    static GUIDCNamePairMap& get();
};

classIdToGUIDCNamePairMap::classIdToGUIDCNamePairMap()
{
    static IdCntrlData const initialCntrlData[] =
    {
        // Command button MUST be at index 0
        { FormComponentType::COMMANDBUTTON,
             { AX_GUID_COMMANDBUTTON, "CommandButton"} ,
        },
        // Toggle button MUST be at index 1
        {  TOGGLEBUTTON,
           { AX_GUID_TOGGLEBUTTON, "ToggleButton"},
        },
        { FormComponentType::FIXEDTEXT,
             { AX_GUID_LABEL, "Label"},
        },
        {  FormComponentType::TEXTFIELD,
             { AX_GUID_TEXTBOX, "TextBox"},
        },
        {  FormComponentType::LISTBOX,
             { AX_GUID_LISTBOX, "ListBox"},
        },
        {  FormComponentType::COMBOBOX,
             { AX_GUID_COMBOBOX, "ComboBox"},
        },
        {  FormComponentType::CHECKBOX,
             { AX_GUID_CHECKBOX, "CheckBox"},
        },
        {  FormComponentType::RADIOBUTTON,
             { AX_GUID_OPTIONBUTTON, "OptionButton"},
        },
        {  FormComponentType::IMAGECONTROL,
             { AX_GUID_IMAGE, "Image"},
        },
        {  FormComponentType::DATEFIELD,
             { AX_GUID_TEXTBOX, "TextBox"},
        },
        {  FormComponentType::TIMEFIELD,
             { AX_GUID_TEXTBOX, "TextBox"},
        },
        {  FormComponentType::NUMERICFIELD,
             { AX_GUID_TEXTBOX, "TextBox"},
        },
        {  FormComponentType::CURRENCYFIELD,
             { AX_GUID_TEXTBOX, "TextBox"},
        },
        {  FormComponentType::PATTERNFIELD,
             { AX_GUID_TEXTBOX, "TextBox"},
        },
        {  FORMULAFIELD,
             { AX_GUID_TEXTBOX, "TextBox"},
        },
        {  FormComponentType::IMAGEBUTTON,
             { AX_GUID_COMMANDBUTTON, "CommandButton"},
        },
        {  FormComponentType::SPINBUTTON,
             { AX_GUID_SPINBUTTON, "SpinButton"},
        },
        {  FormComponentType::SCROLLBAR,
             { AX_GUID_SCROLLBAR, "ScrollBar"},
        }
    };
    int const length = std::size( initialCntrlData );
    IdCntrlData const * pData = initialCntrlData;
    for ( int index = 0; index < length; ++index, ++pData )
        mnIdToGUIDCNamePairMap[ pData->nId ] = pData->aData;
};

GUIDCNamePairMap& classIdToGUIDCNamePairMap::get()
{
    static classIdToGUIDCNamePairMap theInst;
    return theInst.mnIdToGUIDCNamePairMap;
}

template< typename Type >
void lclAppendHex( OUStringBuffer& orBuffer, Type nValue )
{
    const sal_Int32 nWidth = 2 * sizeof( Type );
    static const sal_Unicode spcHexChars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };
    orBuffer.setLength( orBuffer.getLength() + nWidth );
    for( sal_Int32 nCharIdx = orBuffer.getLength() - 1, nCharEnd = nCharIdx - nWidth; nCharIdx > nCharEnd; --nCharIdx, nValue >>= 4 )
        orBuffer[nCharIdx] = spcHexChars[ nValue & 0xF ];
}

} // namespace

StdFontInfo::StdFontInfo() :
    mnHeight( 0 ),
    mnWeight( OLE_STDFONT_NORMAL ),
    mnCharSet( WINDOWS_CHARSET_ANSI ),
    mnFlags( 0 )
{
}

StdFontInfo::StdFontInfo( OUString aName, sal_uInt32 nHeight ) :
    maName(std::move( aName )),
    mnHeight( nHeight ),
    mnWeight( OLE_STDFONT_NORMAL ),
    mnCharSet( WINDOWS_CHARSET_ANSI ),
    mnFlags( 0 )
{
}

::Color OleHelper::decodeOleColor(
        const GraphicHelper& rGraphicHelper, sal_uInt32 nOleColor, bool bDefaultColorBgr )
{
    static const sal_Int32 spnSystemColors[] =
    {
        XML_scrollBar,      XML_background,     XML_activeCaption,  XML_inactiveCaption,
        XML_menu,           XML_window,         XML_windowFrame,    XML_menuText,
        XML_windowText,     XML_captionText,    XML_activeBorder,   XML_inactiveBorder,
        XML_appWorkspace,   XML_highlight,      XML_highlightText,  XML_btnFace,
        XML_btnShadow,      XML_grayText,       XML_btnText,        XML_inactiveCaptionText,
        XML_btnHighlight,   XML_3dDkShadow,     XML_3dLight,        XML_infoText,
        XML_infoBk
    };

    switch( nOleColor & OLE_COLORTYPE_MASK )
    {
        case OLE_COLORTYPE_CLIENT:
            return bDefaultColorBgr ? lclDecodeBgrColor( nOleColor ) : rGraphicHelper.getPaletteColor( nOleColor & OLE_PALETTECOLOR_MASK );

        case OLE_COLORTYPE_PALETTE:
            return rGraphicHelper.getPaletteColor( nOleColor & OLE_PALETTECOLOR_MASK );

        case OLE_COLORTYPE_BGR:
            return lclDecodeBgrColor( nOleColor );

        case OLE_COLORTYPE_SYSCOLOR:
            return rGraphicHelper.getSystemColor( STATIC_ARRAY_SELECT( spnSystemColors, nOleColor & OLE_SYSTEMCOLOR_MASK, XML_TOKEN_INVALID ), API_RGB_WHITE );
    }
    OSL_FAIL( "OleHelper::decodeOleColor - unknown color type" );
    return API_RGB_BLACK;
}

sal_uInt32 OleHelper::encodeOleColor( sal_Int32 nRgbColor )
{
    return OLE_COLORTYPE_BGR | lclSwapRedBlue( static_cast< sal_uInt32 >( nRgbColor & 0xFFFFFF ) );
}

void OleHelper::exportGuid( BinaryOutputStream& rOStr, const SvGlobalName& rId )
{
    rOStr.WriteUInt32( rId.GetCLSID().Data1 );
    rOStr.WriteUInt16( rId.GetCLSID().Data2 );
    rOStr.WriteUInt16( rId.GetCLSID().Data3 );
    rOStr.writeArray( rId.GetCLSID().Data4, 8 );
}

OUString OleHelper::importGuid( BinaryInputStream& rInStrm )
{
    OUStringBuffer aBuffer(40);
    aBuffer.append( '{' );
    lclAppendHex( aBuffer, rInStrm.readuInt32() );
    aBuffer.append( '-' );
    lclAppendHex( aBuffer, rInStrm.readuInt16() );
    aBuffer.append( '-' );
    lclAppendHex( aBuffer, rInStrm.readuInt16() );
    aBuffer.append( '-' );
    lclAppendHex( aBuffer, rInStrm.readuInt8() );
    lclAppendHex( aBuffer, rInStrm.readuInt8() );
    aBuffer.append( '-' );
    for( int nIndex = 0; nIndex < 6; ++nIndex )
        lclAppendHex( aBuffer, rInStrm.readuInt8() );
    aBuffer.append( '}' );
    return aBuffer.makeStringAndClear();
}

bool OleHelper::importStdFont( StdFontInfo& orFontInfo, BinaryInputStream& rInStrm, bool bWithGuid )
{
    if( bWithGuid )
    {
        bool bIsStdFont = importGuid( rInStrm ) == OLE_GUID_STDFONT;
        OSL_ENSURE( bIsStdFont, "OleHelper::importStdFont - unexpected header GUID, expected StdFont" );
        if( !bIsStdFont )
            return false;
    }

    sal_uInt8 nVersion, nNameLen;
    nVersion = rInStrm.readuChar();
    orFontInfo.mnCharSet = rInStrm.readuInt16();
    orFontInfo.mnFlags = rInStrm.readuChar();
    orFontInfo.mnWeight = rInStrm.readuInt16();
    orFontInfo.mnHeight = rInStrm.readuInt32();
    nNameLen = rInStrm.readuChar();
    // according to spec the name is ASCII
    orFontInfo.maName = rInStrm.readCharArrayUC( nNameLen, RTL_TEXTENCODING_ASCII_US );
    OSL_ENSURE( nVersion <= 1, "OleHelper::importStdFont - wrong version" );
    return !rInStrm.isEof() && (nVersion <= 1);
}

bool OleHelper::importStdPic( StreamDataSequence& orGraphicData, BinaryInputStream& rInStrm )
{
    bool bIsStdPic = importGuid( rInStrm ) == OLE_GUID_STDPIC;
    OSL_ENSURE( bIsStdPic, "OleHelper::importStdPic - unexpected header GUID, expected StdPic" );
    if( !bIsStdPic )
        return false;

    sal_uInt32 nStdPicId;
    sal_Int32 nBytes;
    nStdPicId = rInStrm.readuInt32();
    nBytes = rInStrm.readInt32();
    OSL_ENSURE( nStdPicId == OLE_STDPIC_ID, "OleHelper::importStdPic - unexpected header version" );
    return !rInStrm.isEof() && (nStdPicId == OLE_STDPIC_ID) && (nBytes > 0) && (rInStrm.readData( orGraphicData, nBytes ) == nBytes);
}

static Reference< css::frame::XFrame > lcl_getFrame( const  Reference< css::frame::XModel >& rxModel )
{
    Reference< css::frame::XFrame > xFrame;
    if ( rxModel.is() )
    {
        Reference< css::frame::XController > xController =  rxModel->getCurrentController();
        xFrame =  xController.is() ? xController->getFrame() : nullptr;
    }
    return xFrame;
}

OleFormCtrlExportHelper::OleFormCtrlExportHelper(  const Reference< XComponentContext >& rxCtx, const Reference< XModel >& rxDocModel, const Reference< XControlModel >& xCntrlModel ) : mpModel( nullptr ), maGrfHelper( rxCtx, lcl_getFrame( rxDocModel ), StorageRef() ), mxDocModel( rxDocModel ), mxControlModel( xCntrlModel )
{
    // try to get the guid
    Reference< css::beans::XPropertySet > xProps( xCntrlModel, UNO_QUERY );
    if ( !xProps.is() )
        return;

    sal_Int16 nClassId = 0;
    PropertySet aPropSet( mxControlModel );
    if ( !aPropSet.getProperty( nClassId, PROP_ClassId ) )
        return;

    /* pseudo ripped from legacy msocximex:
      "There is a truly horrible thing with EditControls and FormattedField
      Controls, they both pretend to have an EDITBOX ClassId for compatibility
      reasons, at some stage in the future hopefully there will be a proper
      FormulaField ClassId rather than this piggybacking two controls onto the
      same ClassId, cmc." - when fixed the fake FORMULAFIELD id entry
      and definition above can be removed/replaced
    */
    if ( nClassId == FormComponentType::TEXTFIELD)
    {
        Reference< XServiceInfo > xInfo( xCntrlModel,
            UNO_QUERY);
        if (xInfo->
            supportsService( u"com.sun.star.form.component.FormattedField"_ustr ) )
            nClassId = FORMULAFIELD;
    }
    else if ( nClassId == FormComponentType::COMMANDBUTTON )
    {
        bool bToggle = false;
        if ( aPropSet.getProperty( bToggle, PROP_Toggle ) && bToggle )
            nClassId = TOGGLEBUTTON;
    }
    else if ( nClassId == FormComponentType::CONTROL )
    {
        Reference< XServiceInfo > xInfo( xCntrlModel,
            UNO_QUERY);
        if (xInfo->supportsService(u"com.sun.star.form.component.ImageControl"_ustr ) )
            nClassId = FormComponentType::IMAGECONTROL;
    }

    GUIDCNamePairMap& cntrlMap = classIdToGUIDCNamePairMap::get();
    GUIDCNamePairMap::iterator it = cntrlMap.find( nClassId );
    if ( it != cntrlMap.end() )
    {
        aPropSet.getProperty(maName, PROP_Name );
        maTypeName = OUString::createFromAscii( it->second.sName );
        maFullName = "Microsoft Forms 2.0 " + maTypeName;
        mpControl.reset(new EmbeddedControl( maName ));
        maGUID = OUString::createFromAscii( it->second.sGUID );
        mpModel = mpControl->createModelFromGuid( maGUID );
    }
}

OleFormCtrlExportHelper::~OleFormCtrlExportHelper()
{
}

void OleFormCtrlExportHelper::exportName( const Reference< XOutputStream >& rxOut )
{
    oox::BinaryXOutputStream aOut( rxOut, false );
    aOut.writeUnicodeArray( maName );
    aOut.WriteInt32(0);
}

void OleFormCtrlExportHelper::exportCompObj( const Reference< XOutputStream >& rxOut )
{
    oox::BinaryXOutputStream aOut( rxOut, false );
    if ( mpModel )
        mpModel->exportCompObj( aOut );
}

void OleFormCtrlExportHelper::exportControl( const Reference< XOutputStream >& rxOut, const Size& rSize, bool bAutoClose )
{
    oox::BinaryXOutputStream aOut( rxOut, bAutoClose );
    if ( mpModel )
    {
        ::oox::ole::ControlConverter aConv(  mxDocModel, maGrfHelper );
        if(mpControl)
            mpControl->convertFromProperties( mxControlModel, aConv );
        mpModel->maSize.first = rSize.Width;
        mpModel->maSize.second = rSize.Height;
        mpModel->exportBinaryModel( aOut );
    }
}

MSConvertOCXControls::MSConvertOCXControls( const Reference< css::frame::XModel >& rxModel ) : SvxMSConvertOCXControls( rxModel ), mxCtx( comphelper::getProcessComponentContext() ), maGrfHelper( mxCtx, lcl_getFrame( rxModel ), StorageRef() )
{
}

MSConvertOCXControls::~MSConvertOCXControls()
{
}

bool
MSConvertOCXControls::importControlFromStream( ::oox::BinaryInputStream& rInStrm, Reference< XFormComponent >& rxFormComp, std::u16string_view rGuidString )
{
    ::oox::ole::EmbeddedControl aControl( u"Unknown"_ustr );
    if( ::oox::ole::ControlModelBase* pModel = aControl.createModelFromGuid( rGuidString  ) )
    {
        pModel->importBinaryModel( rInStrm );
        rxFormComp.set( mxCtx->getServiceManager()->createInstanceWithContext( pModel->getServiceName(), mxCtx ), UNO_QUERY );
        Reference< XControlModel > xCtlModel( rxFormComp, UNO_QUERY );
        ::oox::ole::ControlConverter aConv(  mxModel, maGrfHelper );
        aControl.convertProperties( xCtlModel, aConv );
    }
    return rxFormComp.is();
}

bool
MSConvertOCXControls::ReadOCXCtlsStream( rtl::Reference<SotStorageStream> const & rSrc1, Reference< XFormComponent > & rxFormComp,
                                   sal_Int32 nPos,
                                   sal_Int32 nStreamSize)
{
    if ( rSrc1.is()  )
    {
        BinaryXInputStream aCtlsStrm( Reference< XInputStream >( new utl::OSeekableInputStreamWrapper( *rSrc1 ) ), true );
        aCtlsStrm.seek( nPos );
        OUString aStrmClassId = ::oox::ole::OleHelper::importGuid( aCtlsStrm );
        return  importControlFromStream( aCtlsStrm, rxFormComp, aStrmClassId, nStreamSize );
    }
    return false;
}

bool MSConvertOCXControls::importControlFromStream( ::oox::BinaryInputStream& rInStrm, Reference< XFormComponent >& rxFormComp, const OUString& rStrmClassId,
                                   sal_Int32 nStreamSize)
{
    if ( !rInStrm.isEof() )
    {
        // Special processing for those html controls
        bool bOneOfHtmlControls = false;
        if ( rStrmClassId.toAsciiUpperCase() == HTML_GUID_SELECT
          || rStrmClassId.toAsciiUpperCase() == HTML_GUID_TEXTBOX )
            bOneOfHtmlControls = true;

        if ( bOneOfHtmlControls )
        {
            // html controls don't seem have a handy record length following the GUID
            // in the binary stream.
            // Given the control stream length create a stream of nStreamSize bytes starting from
            // nPos ( offset by the guid already read in )
            if ( !nStreamSize )
                return false;
            const int nGuidSize = 0x10;
            StreamDataSequence aDataSeq;
            sal_Int32 nBytesToRead = nStreamSize - nGuidSize;
            while ( nBytesToRead )
                nBytesToRead -= rInStrm.readData( aDataSeq, nBytesToRead );
            SequenceInputStream aInSeqStream( aDataSeq );
            importControlFromStream( aInSeqStream, rxFormComp, rStrmClassId );
        }
        else
        {
            importControlFromStream( rInStrm, rxFormComp, rStrmClassId );
        }
    }
    return rxFormComp.is();
}

bool MSConvertOCXControls::ReadOCXStorage( rtl::Reference<SotStorage> const & xOleStg,
                                  Reference< XFormComponent > & rxFormComp )
{
    if ( xOleStg.is() )
    {
        rtl::Reference<SotStorageStream> pNameStream = xOleStg->OpenSotStream(u"\3OCXNAME"_ustr, StreamMode::READ);
        BinaryXInputStream aNameStream( Reference< XInputStream >( new utl::OSeekableInputStreamWrapper( *pNameStream ) ), true );

        rtl::Reference<SotStorageStream> pContents = xOleStg->OpenSotStream(u"contents"_ustr, StreamMode::READ);
        BinaryXInputStream aInStrm(  Reference< XInputStream >( new utl::OSeekableInputStreamWrapper( *pContents ) ), true );

        rtl::Reference<SotStorageStream> pClsStrm = xOleStg->OpenSotStream(u"\1CompObj"_ustr, StreamMode::READ);
        BinaryXInputStream aClsStrm( Reference< XInputStream >( new utl::OSeekableInputStreamWrapper(*pClsStrm ) ), true );
        aClsStrm.skip(12);

        OUString aStrmClassId = ::oox::ole::OleHelper::importGuid( aClsStrm );
        if ( importControlFromStream(  aInStrm,  rxFormComp, aStrmClassId, aInStrm.size() ) )
        {
            OUString aName = aNameStream.readNulUnicodeArray();
            Reference< XControlModel > xCtlModel( rxFormComp, UNO_QUERY );
            if ( !aName.isEmpty() && xCtlModel.is() )
            {
                PropertyMap aPropMap;
                aPropMap.setProperty( PROP_Name, aName );
                PropertySet aPropSet( xCtlModel );
                aPropSet.setProperties( aPropMap );
            }
            return rxFormComp.is();
        }
    }
    return  false;
}

bool MSConvertOCXControls::WriteOCXExcelKludgeStream( const css::uno::Reference< css::frame::XModel >& rxModel, const css::uno::Reference< css::io::XOutputStream >& xOutStrm, const css::uno::Reference< css::awt::XControlModel > &rxControlModel, const css::awt::Size& rSize,OUString &rName )
{
    OleFormCtrlExportHelper exportHelper( comphelper::getProcessComponentContext(), rxModel, rxControlModel );
    if ( !exportHelper.isValid() )
        return false;
    rName = exportHelper.getTypeName();
    SvGlobalName aName;
    (void)aName.MakeId(exportHelper.getGUID());
    BinaryXOutputStream aOut( xOutStrm, false );
    OleHelper::exportGuid( aOut, aName );
    exportHelper.exportControl( xOutStrm, rSize );
    return true;
}

bool MSConvertOCXControls::WriteOCXStream( const Reference< XModel >& rxModel, rtl::Reference<SotStorage> const &xOleStg,
    const Reference< XControlModel > &rxControlModel,
    const css::awt::Size& rSize, OUString &rName)
{
    SvGlobalName aName;

    OleFormCtrlExportHelper exportHelper( comphelper::getProcessComponentContext(), rxModel, rxControlModel );

    if ( !exportHelper.isValid() )
        return false;

    aName.MakeId(exportHelper.getGUID());

    OUString sFullName = exportHelper.getFullName();
    rName = exportHelper.getTypeName();
    xOleStg->SetClass( aName, SotClipboardFormatId::EMBEDDED_OBJ_OLE, sFullName);
    {
        rtl::Reference<SotStorageStream> pNameStream = xOleStg->OpenSotStream(u"\3OCXNAME"_ustr);
        Reference< XOutputStream > xOut = new utl::OSeekableOutputStreamWrapper( *pNameStream );
        exportHelper.exportName( xOut );
    }
    {
        rtl::Reference<SotStorageStream> pObjStream = xOleStg->OpenSotStream(u"\1CompObj"_ustr);
        Reference< XOutputStream > xOut = new utl::OSeekableOutputStreamWrapper( *pObjStream );
        exportHelper.exportCompObj( xOut );
    }
    {
        rtl::Reference<SotStorageStream> pContents = xOleStg->OpenSotStream(u"contents"_ustr);
        Reference< XOutputStream > xOut = new utl::OSeekableOutputStreamWrapper( *pContents );
        exportHelper.exportControl( xOut, rSize );
    }
    return true;
}

} // namespace oox

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
