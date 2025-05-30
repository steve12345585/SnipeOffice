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

#include "WrappedResultSet.hxx"
#include <com/sun/star/sdbc/XResultSetUpdate.hpp>

using namespace dbaccess;
using namespace ::connectivity;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdbcx;

void WrappedResultSet::construct(const Reference< XResultSet>& _xDriverSet,const OUString& i_sRowSetFilter)
{
    OCacheSet::construct(_xDriverSet,i_sRowSetFilter);
    m_xUpd.set(_xDriverSet,UNO_QUERY_THROW);
    m_xRowLocate.set(_xDriverSet,UNO_QUERY_THROW);
    m_xUpdRow.set(_xDriverSet,UNO_QUERY_THROW);
}

void WrappedResultSet::reset(const Reference< XResultSet>& _xDriverSet)
{
    construct(_xDriverSet, m_sRowSetFilter);
}

Any WrappedResultSet::getBookmark()
{
    if ( m_xRowLocate.is() )
    {
        return m_xRowLocate->getBookmark( );
    }
    return Any(m_xDriverSet->getRow());
}

bool WrappedResultSet::moveToBookmark( const Any& bookmark )
{
    return m_xRowLocate->moveToBookmark( bookmark );
}

sal_Int32 WrappedResultSet::compareBookmarks( const Any& _first, const Any& _second )
{
    return m_xRowLocate->compareBookmarks( _first,_second );
}

bool WrappedResultSet::hasOrderedBookmarks(  )
{
    return m_xRowLocate->hasOrderedBookmarks();
}

sal_Int32 WrappedResultSet::hashBookmark( const Any& bookmark )
{
    return m_xRowLocate->hashBookmark(bookmark);
}

void WrappedResultSet::insertRow( const ORowSetRow& _rInsertRow,const connectivity::OSQLTable& /*_xTable*/ )
{
    m_xUpd->moveToInsertRow();
    sal_Int32 i = 1;
    connectivity::ORowVector< ORowSetValue > ::Vector::const_iterator aEnd = _rInsertRow->end();
    for(connectivity::ORowVector< ORowSetValue > ::Vector::iterator aIter = _rInsertRow->begin()+1;aIter != aEnd;++aIter,++i)
    {
        aIter->setSigned(m_aSignedFlags[i-1]);
        updateColumn(i,m_xUpdRow,*aIter);
    }
    m_xUpd->insertRow();
    (*_rInsertRow->begin()) = getBookmark();
}

void WrappedResultSet::updateRow(const ORowSetRow& _rInsertRow ,const ORowSetRow& _rOriginalRow,const connectivity::OSQLTable& /*_xTable*/  )
{
    sal_Int32 i = 1;
    connectivity::ORowVector< ORowSetValue > ::Vector::const_iterator aOrgIter = _rOriginalRow->begin()+1;
    connectivity::ORowVector< ORowSetValue > ::Vector::iterator aEnd = _rInsertRow->end();
    for(connectivity::ORowVector< ORowSetValue > ::Vector::iterator aIter = _rInsertRow->begin()+1;aIter != aEnd;++aIter,++i,++aOrgIter)
    {
        aIter->setSigned(aOrgIter->isSigned());
        updateColumn(i,m_xUpdRow,*aIter);
    }
    m_xUpd->updateRow();
}

void WrappedResultSet::deleteRow(const ORowSetRow& /*_rDeleteRow*/ ,const connectivity::OSQLTable& /*_xTable*/  )
{
    m_xUpd->deleteRow();
}

void WrappedResultSet::updateColumn(sal_Int32 nPos, const Reference< XRowUpdate >& _xParameter, const ORowSetValue& _rValue)
{
    if(!(_rValue.isBound() && _rValue.isModified()))
        return;

    if(_rValue.isNull())
        _xParameter->updateNull(nPos);
    else
    {

        switch(_rValue.getTypeKind())
        {
            case DataType::DECIMAL:
            case DataType::NUMERIC:
                _xParameter->updateNumericObject(nPos,_rValue.makeAny(),m_xSetMetaData->getScale(nPos));
                break;
            case DataType::CHAR:
            case DataType::VARCHAR:
                _xParameter->updateString(nPos,_rValue.getString());
                break;
            case DataType::BIGINT:
                if ( _rValue.isSigned() )
                    _xParameter->updateLong(nPos,_rValue.getLong());
                else
                    _xParameter->updateString(nPos,_rValue.getString());
                break;
            case DataType::BIT:
            case DataType::BOOLEAN:
                _xParameter->updateBoolean(nPos,_rValue.getBool());
                break;
            case DataType::TINYINT:
                if ( _rValue.isSigned() )
                    _xParameter->updateByte(nPos,_rValue.getInt8());
                else
                    _xParameter->updateShort(nPos,_rValue.getInt16());
                break;
            case DataType::SMALLINT:
                if ( _rValue.isSigned() )
                    _xParameter->updateShort(nPos,_rValue.getInt16());
                else
                    _xParameter->updateInt(nPos,_rValue.getInt32());
                break;
            case DataType::INTEGER:
                if ( _rValue.isSigned() )
                    _xParameter->updateInt(nPos,_rValue.getInt32());
                else
                    _xParameter->updateLong(nPos,_rValue.getLong());
                break;
            case DataType::FLOAT:
                _xParameter->updateFloat(nPos,_rValue.getFloat());
                break;
            case DataType::DOUBLE:
            case DataType::REAL:
                _xParameter->updateDouble(nPos,_rValue.getDouble());
                break;
            case DataType::DATE:
                _xParameter->updateDate(nPos,_rValue.getDate());
                break;
            case DataType::TIME:
                _xParameter->updateTime(nPos,_rValue.getTime());
                break;
            case DataType::TIMESTAMP:
                _xParameter->updateTimestamp(nPos,_rValue.getDateTime());
                break;
            case DataType::BINARY:
            case DataType::VARBINARY:
            case DataType::LONGVARBINARY:
                _xParameter->updateBytes(nPos,_rValue.getSequence());
                break;
            case DataType::BLOB:
            case DataType::CLOB:
                _xParameter->updateObject(nPos,_rValue.getAny());
                break;
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
