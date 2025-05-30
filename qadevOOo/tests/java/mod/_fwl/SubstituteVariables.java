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

package mod._fwl;

import java.io.PrintWriter;

import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.utils;

import com.sun.star.uno.XInterface;

/**
 * Test for object which is represented by service
 * <code>com.sun.star.frame.FrameLoaderFactory</code>. <p>
 *
 * Object implements the following interfaces :
 * <ul>
 *  <li> <code>com::sun::star::container::XNameAccess</code></li>
 *  <li> <code>com::sun::star::container::XElementAccess</code></li>
 *  <li> <code>com::sun::star::lang::XMultiServiceFactory</code></li>
 * </ul> <p>
 *
 * @see com.sun.star.util.XStringSubstitution
 * @see ifc.util._XStringSubstitution
 */
public class SubstituteVariables extends TestCase {

    /**
    * Creating a TestEnvironment for the interfaces to be tested.
    * Creates an instance of the service
    * <code>com.sun.star.util.PathSubstitution</code>. <p>
    */
    @Override
    protected TestEnvironment createTestEnvironment
            (TestParameters Param, PrintWriter log) throws Exception {
        //now get the OButtonControl
        Object oInterface = Param.getMSF().createInstance
            ("com.sun.star.util.PathSubstitution") ;
        XInterface oObj = (XInterface) oInterface ;
        log.println("ImplName: "+utils.getImplName(oObj));

        log.println( "creating a new environment for object" );
        TestEnvironment tEnv = new TestEnvironment( oObj );


        return tEnv;
    } // finish method getTestEnvironment

}

