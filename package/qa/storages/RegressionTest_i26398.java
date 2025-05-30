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

package complex.storages;

import com.sun.star.uno.XInterface;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.lang.XSingleServiceFactory;

import com.sun.star.bridge.XUnoUrlResolver;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.io.XStream;
import com.sun.star.io.XInputStream;

import com.sun.star.embed.*;

import share.LogWriter;
import complex.storages.TestHelper;
import complex.storages.StorageTest;

public class RegressionTest_i26398 implements StorageTest {

    XMultiServiceFactory m_xMSF;
    XSingleServiceFactory m_xStorageFactory;
    TestHelper m_aTestHelper;

    public RegressionTest_i26398( XMultiServiceFactory xMSF, XSingleServiceFactory xStorageFactory, LogWriter aLogWriter )
    {
        m_xMSF = xMSF;
        m_xStorageFactory = xStorageFactory;
        m_aTestHelper = new TestHelper( aLogWriter, "RegressionTest_i26398: " );
    }

    public boolean test()
    {
        try
        {
            XStream xTempFileStream = m_aTestHelper.CreateTempFileStream( m_xMSF );
            if ( xTempFileStream == null )
                return false;

            // create storage based on the temporary stream
            Object pArgs[] = new Object[2];
            pArgs[0] = (Object) xTempFileStream;
            pArgs[1] = Integer.valueOf( ElementModes.WRITE );

            Object oTempStorage = m_xStorageFactory.createInstanceWithArguments( pArgs );
            XStorage xTempStorage = (XStorage) UnoRuntime.queryInterface( XStorage.class, oTempStorage );
            if ( xTempStorage == null )
            {
                m_aTestHelper.Error( "Can't create temporary storage representation!" );
                return false;
            }

            // open a new substorage
            XStorage xTempSubStorage = m_aTestHelper.openSubStorage( xTempStorage,
                                                                    "SubStorage1",
                                                                    ElementModes.WRITE );
            if ( xTempSubStorage == null )
            {
                m_aTestHelper.Error( "Can't create substorage!" );
                return false;
            }

            byte pBytes1[] = { 1, 1, 1, 1, 1 };

            // open a new substream, set "MediaType" and "Compressed" properties to it and write some bytes
            if ( !m_aTestHelper.WriteBytesToSubstream( xTempSubStorage, "SubStream1", "MediaType1", true, pBytes1 ) )
                return false;

            // set "MediaType" property for storages and check that "IsRoot" and "OpenMode" properties are set correctly
            if ( !m_aTestHelper.setStorageTypeAndCheckProps( xTempStorage,
                                                            "MediaType2",
                                                            true,
                                                            ElementModes.WRITE ) )
                return false;

            // set "MediaType" property for storages and check that "IsRoot" and "OpenMode" properties are set correctly
            if ( !m_aTestHelper.setStorageTypeAndCheckProps( xTempSubStorage,
                                                            "MediaType3",
                                                            false,
                                                            ElementModes.WRITE ) )
                return false;



            // commit the substorage, dispose it, reopen readonly
            // and dispose the reopened substorage


            // commit substorage
            if ( !m_aTestHelper.commitStorage( xTempSubStorage ) )
                return false;

            // dispose substorage
            if ( !m_aTestHelper.disposeStorage( xTempSubStorage ) )
                return false;

            // open a new substorage
            xTempSubStorage = m_aTestHelper.openSubStorage( xTempStorage,
                                                                    "SubStorage1",
                                                                    ElementModes.READ );
            if ( xTempSubStorage == null )
            {
                m_aTestHelper.Error( "Can't create substorage!" );
                return false;
            }

            // dispose substorage
            if ( !m_aTestHelper.disposeStorage( xTempSubStorage ) )
                return false;


            // reopen the substorage in readwrite mode and check contents


            // open a new substorage
            xTempSubStorage = m_aTestHelper.openSubStorage( xTempStorage,
                                                                    "SubStorage1",
                                                                    ElementModes.WRITE );
            if ( xTempSubStorage == null )
            {
                m_aTestHelper.Error( "Can't create substorage!" );
                return false;
            }

            if ( !m_aTestHelper.checkStorageProperties( xTempSubStorage, "MediaType3", false, ElementModes.WRITE ) )
                return false;

            if ( !m_aTestHelper.checkStorageProperties( xTempStorage, "MediaType2", true, ElementModes.WRITE ) )
                return false;

            if ( !m_aTestHelper.checkStream( xTempSubStorage, "SubStream1", "MediaType1", true, pBytes1 ) )
                return false;

            // the root storage is based on the temporary stream so it can be left undisposed, since it does not lock
            // any resource, later the garbage collector will release the object and it must die by refcount

            return true;
        }
        catch( Exception e )
        {
            m_aTestHelper.Error( "Exception: " + e );
            return false;
        }
    }
}

