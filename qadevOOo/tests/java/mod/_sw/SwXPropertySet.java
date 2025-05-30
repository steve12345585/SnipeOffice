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

package mod._sw;

import java.io.PrintWriter;

import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.SOfficeFactory;

import com.sun.star.text.XTextDocument;

/**
 * Test for object which is represented by text document.<p>
 * Object implements the following interfaces :
 * <ul>
 *  <li> <code>com::sun::star::beans::XPropertySet</code></li>
 * </ul> <p>
 * This object test <b> is NOT </b> designed to be run in several
 * threads concurrently.
 * @see com.sun.star.beans.XPropertySet
 * @see ifc.beans._XPropertySet
 */
public class SwXPropertySet extends TestCase {
    XTextDocument xTextDoc;

    /**
    * Disposes text document.
    */
    @Override
    protected void cleanup( TestParameters Param, PrintWriter log) {
        log.println( "    disposing xTextDoc " );
        util.DesktopTools.closeDoc(xTextDoc);
    }

    /**
    * Creating a TestEnvironment for the interfaces to be tested.
    * Method creates text document as a test component.
    */
    @Override
    public TestEnvironment createTestEnvironment(
            TestParameters Param, PrintWriter log ) throws Exception {

        log.println( "creating a test environment" );
        SOfficeFactory SOF = SOfficeFactory.getFactory( Param.getMSF() );
        log.println( "creating a text document" );
        xTextDoc = SOF.createTextDoc( null );

        TestEnvironment tEnv = new TestEnvironment( xTextDoc );
        return tEnv;
    } // finish method getTestEnvironment

}    // finish class SwXPropertySet

