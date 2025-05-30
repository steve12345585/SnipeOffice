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

package mod._ucb;

import java.io.PrintWriter;

import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;

import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.ucb.XContentIdentifierFactory;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;

/**
* Test for object which is represented by service
* <code>com.sun.star.ucb.UniversalContentBroker</code>. <p>
*/
public class UniversalContentBroker extends TestCase {

    /**
    * Creating a TestEnvironment for the interfaces to be tested.
    * Creates an instance of the service
    * <code>com.sun.star.ucb.UniversalContentBroker</code>.
    */
    @Override
    public TestEnvironment createTestEnvironment
            ( TestParameters Param, PrintWriter log ) throws Exception {

        XInterface oObj = null;
        Object oInterface = null;

        XMultiServiceFactory xMSF = Param.getMSF();
        oInterface = xMSF.createInstance
            ( "com.sun.star.comp.ucb.UniversalContentBroker" );

        oObj = (XInterface) oInterface;

        XContentIdentifierFactory CIF = UnoRuntime.queryInterface(XContentIdentifierFactory.class,oObj);

        System.out.println("ImplementationName: "+util.utils.getImplName(oObj));

        TestEnvironment tEnv = new TestEnvironment( oObj );

        //Adding ObjRelation for XContentProvider
        tEnv.addObjRelation("FACTORY",CIF);

        tEnv.addObjRelation("CONTENT1",
            "vnd.sun.star.webdav://localhost/davhome/");

        return tEnv;

    } // finish method getTestEnvironment

}    // finish class UniversalContentBroker

