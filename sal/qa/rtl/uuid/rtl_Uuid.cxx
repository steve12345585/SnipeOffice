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

#include <string.h>

#include <rtl/uuid.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>

#ifdef _WIN32
#if !defined WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif


namespace rtl_Uuid
{
class createUuid : public CppUnit::TestFixture
{
public:
#define TEST_UUID 20
    void createUuid_001()
    {
    sal_uInt8 aNode[TEST_UUID][16];
    sal_Int32 i,i2;
    for( i = 0 ; i < TEST_UUID ; i ++ )
    {
        rtl_createUuid( aNode[i], nullptr, false );
    }
    bool bRes = true;
    for( i = 0 ; i < TEST_UUID ; i ++ )
    {
        for( i2 = i+1 ; i2 < TEST_UUID ; i2 ++ )
        {
            if ( rtl_compareUuid( aNode[i] , aNode[i2] ) == 0  )
            {
                bRes = false;
                break;
            }
        }
        if ( !bRes )
            break;
    }
    CPPUNIT_ASSERT_MESSAGE("createUuid: every uuid must be different.", bRes);
    }
   /*
    void createUuid_002()
    {
    sal_uInt8 pNode[16];
    sal_uInt8 aNode[TEST_UUID][16];
    sal_Int32 i,i2;
    for( i = 0 ; i < TEST_UUID ; i ++ )
    {
        rtl_createUuid( aNode[i], pNode, sal_True );
    }
    sal_Bool bRes = sal_True;
    for( i = 0 ; i < TEST_UUID ; i ++ )
    {
        //printUuid( aNode[i] );
        for( i2 = i+1 ; i2 < TEST_UUID ; i2 ++ )
        {
            if ( rtl_compareUuid( aNode[i] , aNode[i2] ) == 0  )
            {
                bRes = sal_False;
                break;
            }
        }
        if ( bRes == sal_False )
            break;
    }
    CPPUNIT_ASSERT_MESSAGE("createUuid: every uuid must be different.", bRes == sal_True );
    }*/

    CPPUNIT_TEST_SUITE(createUuid);
    CPPUNIT_TEST(createUuid_001);
    //CPPUNIT_TEST(createUuid_002);
    CPPUNIT_TEST_SUITE_END();
}; // class createUuid

class createNamedUuid : public CppUnit::TestFixture
{
public:
    void createNamedUuid_001()
    {
        sal_uInt8 NameSpace_DNS[16] = RTL_UUID_NAMESPACE_DNS;
        sal_uInt8 NameSpace_URL[16] = RTL_UUID_NAMESPACE_URL;
        sal_uInt8 pPriorCalculatedUUID[16] = {
            0x52,0xc9,0x30,0xa5,
            0xd1,0x61,0x3b,0x16,
            0x98,0xc5,0xf8,0xd1,
            0x10,0x10,0x6d,0x4d };

        sal_uInt8 pNamedUUID[16], pNamedUUID2[16];

        // Same name does generate the same uuid
        rtl_String *pName = nullptr;
        rtl_string_newFromStr( &pName , "this is a bla.blubs.DNS-Name" );
        rtl_createNamedUuid( pNamedUUID , NameSpace_DNS , pName );
        rtl_createNamedUuid( pNamedUUID2 , NameSpace_DNS , pName );
        CPPUNIT_ASSERT_MESSAGE( "Same name should generate the same uuid", ! memcmp( pNamedUUID , pNamedUUID2 , 16 ));
        CPPUNIT_ASSERT_EQUAL_MESSAGE( "Same name should generate the same uuid", sal_Int32(0), rtl_compareUuid( pNamedUUID , pNamedUUID2 ) );
        CPPUNIT_ASSERT_MESSAGE( "Same name should generate the same uuid", ! memcmp( pNamedUUID  , pPriorCalculatedUUID , 16 ) );

        // Different names does not generate the same uuid
        rtl_string_newFromStr( &pName , "this is a bla.blubs.DNS-Namf" );
        rtl_createNamedUuid( pNamedUUID2 , NameSpace_DNS , pName );
        CPPUNIT_ASSERT_MESSAGE("Different names does not generate the same uuid.", memcmp( pNamedUUID , pNamedUUID2 , 16 ) );

        // the same name with different namespace uuid produces different uuids
        rtl_createNamedUuid( pNamedUUID , NameSpace_URL , pName );
        CPPUNIT_ASSERT_MESSAGE( " same name with different namespace uuid produces different uuids", memcmp( pNamedUUID , pNamedUUID2 , 16 ));
        CPPUNIT_ASSERT_MESSAGE( " same name with different namespace uuid produces different uuids", rtl_compareUuid( pNamedUUID , pNamedUUID2 ) != 0);

        //test compareUuid
        if ( rtl_compareUuid( pNamedUUID , pNamedUUID2 ) > 0 )
        {   CPPUNIT_ASSERT_MESSAGE( " compare uuids", rtl_compareUuid( pNamedUUID2 , pNamedUUID ) < 0);
        }
        else
            CPPUNIT_ASSERT_MESSAGE( " compare uuids", rtl_compareUuid( pNamedUUID2 , pNamedUUID ) > 0);

        rtl_string_release( pName );
    }

    CPPUNIT_TEST_SUITE(createNamedUuid);
    CPPUNIT_TEST(createNamedUuid_001);
    CPPUNIT_TEST_SUITE_END();
}; // class createNamedUuid

CPPUNIT_TEST_SUITE_REGISTRATION(rtl_Uuid::createUuid);
CPPUNIT_TEST_SUITE_REGISTRATION(rtl_Uuid::createNamedUuid);
} // namespace rtl_Uuid

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
