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


#include <ado/AGroup.hxx>
#include <ado/AUsers.hxx>
#include <comphelper/servicehelper.hxx>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <ado/AConnection.hxx>
#include <TConnection.hxx>

using namespace connectivity::ado;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;
using namespace com::sun::star::sdbcx;


void WpADOGroup::Create()
{
    pInterface.CoCreateInstance(ADOS::CLSID_ADOGROUP_25, nullptr, CLSCTX_INPROC_SERVER);
}

OAdoGroup::OAdoGroup(OCatalog* _pParent,bool _bCase,    ADOGroup* _pGroup) : OGroup_ADO(_bCase),m_pCatalog(_pParent)
{
    construct();
    if(_pGroup)
        m_aGroup.set(_pGroup);
    else
        m_aGroup.Create();

}

OAdoGroup::OAdoGroup(OCatalog* _pParent,bool _bCase, const OUString& Name) : OGroup_ADO(Name,_bCase),m_pCatalog(_pParent)
{
    construct();
    m_aGroup.Create();
    m_aGroup.put_Name(Name);
}

void OAdoGroup::refreshUsers()
{
    ::std::vector< OUString> aVector;

    WpADOUsers aUsers = m_aGroup.get_Users();
    aUsers.fillElementNames(aVector);

    if(m_pUsers)
        m_pUsers->reFill(aVector);
    else
        m_pUsers.reset(new OUsers(m_pCatalog, m_aMutex, aVector, aUsers, isCaseSensitive()));
}

void OAdoGroup::setFastPropertyValue_NoBroadcast(sal_Int32 nHandle,const Any& rValue)
{
    if(m_aGroup.IsValid())
    {

        switch(nHandle)
        {
            case PROPERTY_ID_NAME:
                {
                    OUString aVal;
                    rValue >>= aVal;
                    m_aGroup.put_Name(aVal);
                }
                break;
        }
    }
}

void OAdoGroup::getFastPropertyValue(Any& rValue,sal_Int32 nHandle) const
{
    if(m_aGroup.IsValid())
    {
        switch(nHandle)
        {
            case PROPERTY_ID_NAME:
                rValue <<= m_aGroup.get_Name();
                break;
        }
    }
}


sal_Int32 SAL_CALL OAdoGroup::getPrivileges( const OUString& objName, sal_Int32 objType )
{
    return MapRight(m_aGroup.GetPermissions(objName,MapObjectType(objType)));
}

sal_Int32 SAL_CALL OAdoGroup::getGrantablePrivileges( const OUString& objName, sal_Int32 objType )
{
    RightsEnum eNum = m_aGroup.GetPermissions(objName,MapObjectType(objType));
    if(eNum & adRightWithGrant)
        return MapRight(eNum);
    return 0;
}

void SAL_CALL OAdoGroup::grantPrivileges( const OUString& objName, sal_Int32 objType, sal_Int32 objPrivileges )
{
    m_aGroup.SetPermissions(objName,MapObjectType(objType),adAccessGrant,Map2Right(objPrivileges));
}

void SAL_CALL OAdoGroup::revokePrivileges( const OUString& objName, sal_Int32 objType, sal_Int32 objPrivileges )
{
    m_aGroup.SetPermissions(objName,MapObjectType(objType),adAccessDeny,Map2Right(objPrivileges));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
