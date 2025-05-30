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

#include <ado/AUser.hxx>
#include <ado/ACatalog.hxx>
#include <ado/AGroups.hxx>
#include <comphelper/servicehelper.hxx>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <ado/AConnection.hxx>
#include <ado/Awrapado.hxx>

using namespace connectivity::ado;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;


OAdoUser::OAdoUser(OCatalog* _pParent,bool _bCase, ADOUser* _pUser)
    : OUser_TYPEDEF(_bCase)
    ,m_pCatalog(_pParent)
{
    construct();

    if(_pUser)
        m_aUser.set(_pUser);
    else
        m_aUser.Create();
}

OAdoUser::OAdoUser(OCatalog* _pParent,bool _bCase,   const OUString& Name)
    : OUser_TYPEDEF(Name,_bCase)
    , m_pCatalog(_pParent)
{
    construct();
    m_aUser.Create();
    m_aUser.put_Name(Name);
}

void OAdoUser::refreshGroups()
{
    ::std::vector< OUString> aVector;
    WpADOGroups aGroups(m_aUser.get_Groups());
    aGroups.fillElementNames(aVector);
    if(m_pGroups)
        m_pGroups->reFill(aVector);
    else
        m_pGroups.reset(new OGroups(m_pCatalog, m_aMutex, aVector, aGroups, isCaseSensitive()));
}

void OAdoUser::setFastPropertyValue_NoBroadcast(sal_Int32 nHandle,const Any& rValue)
{
    if(m_aUser.IsValid())
    {

        switch(nHandle)
        {
            case PROPERTY_ID_NAME:
                {
                    OUString aVal;
                    rValue >>= aVal;
                    m_aUser.put_Name(aVal);
                }
                break;
        }
    }
}

void OAdoUser::getFastPropertyValue(Any& rValue,sal_Int32 nHandle) const
{
    if(m_aUser.IsValid())
    {
        switch(nHandle)
        {
            case PROPERTY_ID_NAME:
                rValue <<= m_aUser.get_Name();
                break;
        }
    }
}

OUserExtend::OUserExtend(OCatalog* _pParent,bool _bCase,    ADOUser* _pUser)
    : OAdoUser(_pParent,_bCase,_pUser)
{
}

OUserExtend::OUserExtend(OCatalog* _pParent,bool _bCase, const OUString& Name)
    : OAdoUser(_pParent,_bCase,Name)
{
}


void OUserExtend::construct()
{
    OUser_TYPEDEF::construct();
    registerProperty(OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_PASSWORD),    PROPERTY_ID_PASSWORD,0,&m_Password,::cppu::UnoType<OUString>::get());
}

cppu::IPropertyArrayHelper* OUserExtend::createArrayHelper() const
{
    Sequence< css::beans::Property > aProps;
    describeProperties(aProps);
    return new cppu::OPropertyArrayHelper(aProps);
}

cppu::IPropertyArrayHelper & OUserExtend::getInfoHelper()
{
    return *OUserExtend_PROP::getArrayHelper();
}

sal_Int32 SAL_CALL OAdoUser::getPrivileges( const OUString& objName, sal_Int32 objType )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(OUser_BASE_TYPEDEF::rBHelper.bDisposed);

    return ADOS::mapAdoRights2Sdbc(m_aUser.GetPermissions(objName, ADOS::mapObjectType2Ado(objType)));
}

sal_Int32 SAL_CALL OAdoUser::getGrantablePrivileges( const OUString& objName, sal_Int32 objType )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(OUser_BASE_TYPEDEF::rBHelper.bDisposed);

    sal_Int32 nRights = 0;
    RightsEnum eRights = m_aUser.GetPermissions(objName, ADOS::mapObjectType2Ado(objType));
    if((eRights & adRightWithGrant) == adRightWithGrant)
        nRights = ADOS::mapAdoRights2Sdbc(eRights);
    ADOS::ThrowException(m_pCatalog->getConnection()->getConnection(),*this);
    return nRights;
}

void SAL_CALL OAdoUser::grantPrivileges( const OUString& objName, sal_Int32 objType, sal_Int32 objPrivileges )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(OUser_BASE_TYPEDEF::rBHelper.bDisposed);
    m_aUser.SetPermissions(objName,ADOS::mapObjectType2Ado(objType),adAccessGrant,RightsEnum(ADOS::mapRights2Ado(objPrivileges)));
    ADOS::ThrowException(m_pCatalog->getConnection()->getConnection(),*this);
}

void SAL_CALL OAdoUser::revokePrivileges( const OUString& objName, sal_Int32 objType, sal_Int32 objPrivileges )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(OUser_BASE_TYPEDEF::rBHelper.bDisposed);
    m_aUser.SetPermissions(objName,ADOS::mapObjectType2Ado(objType),adAccessRevoke,RightsEnum(ADOS::mapRights2Ado(objPrivileges)));
    ADOS::ThrowException(m_pCatalog->getConnection()->getConnection(),*this);
}

// XUser
void SAL_CALL OAdoUser::changePassword( const OUString& objPassword, const OUString& newPassword )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkDisposed(OUser_BASE_TYPEDEF::rBHelper.bDisposed);
    m_aUser.ChangePassword(objPassword,newPassword);
    ADOS::ThrowException(m_pCatalog->getConnection()->getConnection(),*this);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
