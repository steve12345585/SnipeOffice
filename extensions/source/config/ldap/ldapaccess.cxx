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


#include "ldapaccess.hxx"

#include <osl/diagnose.h>
#include <o3tl/char16_t2wchar_t.hxx>

#include <com/sun/star/ldap/LdapConnectionException.hpp>


namespace extensions::config::ldap {


typedef int LdapErrCode;

struct LdapMessageHolder
{
    LdapMessageHolder() : msg(nullptr) {}
    ~LdapMessageHolder()
    {
        if (msg)
            ldap_msgfree(msg);
    }
    LdapMessageHolder(const LdapMessageHolder&) = delete;
    LdapMessageHolder& operator=(const LdapMessageHolder&) = delete;

    LDAPMessage * msg;
};

LdapConnection::~LdapConnection()
{
    if (isValid()) disconnect();
}


void LdapConnection::disconnect()
{
    if (mConnection != nullptr)
    {
        ldap_unbind_s(mConnection) ;
        mConnection = nullptr;
    }
}


static void checkLdapReturnCode(const char *aOperation,
                                LdapErrCode aRetCode)
{
    if (aRetCode == LDAP_SUCCESS) { return ; }

    OUString message;

    if (aOperation != nullptr)
    {
        message += OUString::createFromAscii(aOperation) + ": ";
    }
    message += OUString::createFromAscii(ldap_err2string(aRetCode)) + " (" ;

#ifndef LDAP_OPT_SIZELIMIT // for use with OpenLDAP
    char* stub = nullptr;
    ldap_get_lderrno(aConnection, NULL, &stub) ;
    if (stub != nullptr)
    {
        message += OUString::createFromAscii(stub) ;
        // It would seem the message returned is actually
        // not a copy of a string but rather some static
        // string itself. At any rate freeing it seems to
        // cause some undue problems at least on Windows.
        // This call is thus disabled for the moment.
        //ldap_memfree(stub) ;
    }
    else
#endif
    { message += "No additional information"; }

    message += ")" ;
    throw ldap::LdapGenericException(message, nullptr, aRetCode) ;
}

void  LdapConnection::connectSimple(const LdapDefinition& aDefinition)
{
    OSL_ENSURE(!isValid(), "Re-connecting to an LDAP connection that is already established");
    if (isValid()) disconnect();

    mLdapDefinition = aDefinition;
    connectSimple();
}

void  LdapConnection::connectSimple()
{
    if (isValid())
        return;

    // Connect to the server
    initConnection() ;
    // Set Protocol V3
    int version = LDAP_VERSION3;
    ldap_set_option(mConnection,
                    LDAP_OPT_PROTOCOL_VERSION,
                    &version);

#ifdef LDAP_X_OPT_CONNECT_TIMEOUT // OpenLDAP doesn't support this and the func
    /* timeout is specified in milliseconds -> 4 seconds*/
    int timeout = 4000;
#ifdef _WIN32
    ldap_set_optionW( mConnection,
                    LDAP_X_OPT_CONNECT_TIMEOUT,
                    &timeout );
#else
    ldap_set_option( mConnection,
                    LDAP_X_OPT_CONNECT_TIMEOUT,
                    &timeout );
#endif
#endif

    // Do the bind
#ifdef _WIN32
    LdapErrCode retCode = ldap_simple_bind_sW(mConnection,
                                           const_cast<PWSTR>(o3tl::toW(mLdapDefinition.mAnonUser.getStr())),
                                           const_cast<PWSTR>(o3tl::toW(mLdapDefinition.mAnonCredentials.getStr())) );
#else
    LdapErrCode retCode = ldap_simple_bind_s(mConnection,
                                           OUStringToOString( mLdapDefinition.mAnonUser, RTL_TEXTENCODING_UTF8 ).getStr(),
                                           OUStringToOString( mLdapDefinition.mAnonCredentials, RTL_TEXTENCODING_UTF8 ).getStr()) ;
#endif

    checkLdapReturnCode("SimpleBind", retCode) ;
}

void LdapConnection::initConnection()
{
    if (mLdapDefinition.mServer.isEmpty())
    {
        throw ldap::LdapConnectionException(u"Cannot initialise connection to LDAP: No server specified."_ustr);
    }

    if (mLdapDefinition.mPort == 0) mLdapDefinition.mPort = LDAP_PORT;

#ifdef _WIN32
    mConnection = ldap_initW(const_cast<PWSTR>(o3tl::toW(mLdapDefinition.mServer.getStr())),
                            mLdapDefinition.mPort) ;
#else
    mConnection = ldap_init(OUStringToOString( mLdapDefinition.mServer, RTL_TEXTENCODING_UTF8 ).getStr(),
                            mLdapDefinition.mPort) ;
#endif
    if (mConnection == nullptr)
    {
        throw ldap::LdapConnectionException(
            "Cannot initialise connection to LDAP server "
            + mLdapDefinition.mServer + ":" + OUString::number(mLdapDefinition.mPort));
    }
}

 void LdapConnection::getUserProfile(
     const OUString& aUser, LdapData * data)
{
    OSL_ASSERT(data != nullptr);
    if (!isValid()) { connectSimple(); }

    OUString aUserDn =findUserDn( aUser );

    LdapMessageHolder result;
#ifdef _WIN32
    LdapErrCode retCode = ldap_search_sW(mConnection,
                                      const_cast<PWSTR>(o3tl::toW(aUserDn.getStr())),
                                      LDAP_SCOPE_BASE,
                                      const_cast<PWSTR>( L"(objectclass=*)" ),
                                      nullptr,
                                      0, // Attributes + values
                                      &result.msg) ;
#else
    LdapErrCode retCode = ldap_search_s(mConnection,
                                      OUStringToOString( aUserDn, RTL_TEXTENCODING_UTF8 ).getStr(),
                                      LDAP_SCOPE_BASE,
                                      "(objectclass=*)",
                                      nullptr,
                                      0, // Attributes + values
                                      &result.msg) ;
#endif
    checkLdapReturnCode("getUserProfile", retCode) ;

    BerElement * ptr;
#ifdef _WIN32
    PWCHAR attr = ldap_first_attributeW(mConnection, result.msg, &ptr);
    while (attr) {
        PWCHAR * values = ldap_get_valuesW(mConnection, result.msg, attr);
        if (values) {
            const OUString aAttr( o3tl::toU( attr ) );
            const OUString aValues( o3tl::toU( *values ) );
            data->emplace( aAttr, aValues );
            ldap_value_freeW(values);
        }
        attr = ldap_next_attributeW(mConnection, result.msg, ptr);
#else
    char * attr = ldap_first_attribute(mConnection, result.msg, &ptr);
    while (attr) {
        char ** values = ldap_get_values(mConnection, result.msg, attr);
        if (values) {
            data->emplace(
                    OStringToOUString(attr, RTL_TEXTENCODING_ASCII_US),
                    OStringToOUString(*values, RTL_TEXTENCODING_UTF8));
            ldap_value_free(values);
        }
        attr = ldap_next_attribute(mConnection, result.msg, ptr);
#endif
    }
}

 OUString LdapConnection::findUserDn(const OUString& aUser)
{
    if (!isValid()) { connectSimple(); }

    if (aUser.isEmpty())
    {
        throw lang::IllegalArgumentException(
            u"LdapConnection::findUserDn -User id is empty"_ustr,
                nullptr, 0) ;
    }

    OUString filter = "(&(objectclass="
                    + mLdapDefinition.mUserObjectClass
                    + ")("
                    + mLdapDefinition.mUserUniqueAttr
                    + "="
                    + aUser
                    + "))";

    LdapMessageHolder result;
#ifdef _WIN32
    PWCHAR attributes [2] = { const_cast<PWCHAR>( L"1.1" ), nullptr };
    LdapErrCode retCode = ldap_search_sW(mConnection,
                                      const_cast<PWSTR>(o3tl::toW(mLdapDefinition.mBaseDN.getStr())),
                                      LDAP_SCOPE_SUBTREE,
                                      const_cast<PWSTR>(o3tl::toW(filter.getStr())), attributes, 0, &result.msg) ;
#else
    char * attributes [2] = { const_cast<char *>(LDAP_NO_ATTRS), nullptr };
    LdapErrCode retCode = ldap_search_s(mConnection,
                                      OUStringToOString( mLdapDefinition.mBaseDN, RTL_TEXTENCODING_UTF8 ).getStr(),
                                      LDAP_SCOPE_SUBTREE,
                                      OUStringToOString( filter, RTL_TEXTENCODING_UTF8 ).getStr(), attributes, 0, &result.msg) ;
#endif
    checkLdapReturnCode("FindUserDn", retCode) ;
    OUString userDn ;
    LDAPMessage *entry = ldap_first_entry(mConnection, result.msg) ;

    if (entry != nullptr)
    {
#ifdef _WIN32
        PWCHAR charsDn = ldap_get_dnW(mConnection, entry) ;

        userDn = OUString( o3tl::toU( charsDn ) );
        ldap_memfreeW(charsDn) ;
#else
        char *charsDn = ldap_get_dn(mConnection, entry) ;

        userDn = OStringToOUString( charsDn, RTL_TEXTENCODING_UTF8 );
        ldap_memfree(charsDn) ;
#endif
    }
    else
    {
        OSL_FAIL( "LdapConnection::findUserDn-could not get DN for User ");
    }

    return userDn ;
}


} // extensions::config::ldap

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
