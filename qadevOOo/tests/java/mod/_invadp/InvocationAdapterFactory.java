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

package mod._invadp;

import java.io.PrintWriter;

import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;

import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.uno.XInterface;

/**
* Test for object which is represented by service
* <code>com.sun.star.script.InvocationAdapterFactory</code>. <p>
* Object implements the following interfaces :
* <ul>
*  <li> <code>com::sun::star::script::XInvocationAdapterFactory</code></li>
* </ul>
* This object test <b> is NOT </b> designed to be run in several
* threads concurrently.
* @see com.sun.star.script.XInvocationAdapterFactory
* @see com.sun.star.script.InvocationAdapterFactory
* @see ifc.script._XInvocationAdapterFactory
*/
public class InvocationAdapterFactory extends TestCase {

    /**
    * Creating a TestEnvironment for the interfaces to be tested.
    * Creates an instance of the service
    * <code>com.sun.star.script.InvocationAdapterFactory</code>.
    */
    @Override
    protected TestEnvironment createTestEnvironment(TestParameters tParam, PrintWriter log) throws Exception {
        XMultiServiceFactory xMSF = tParam.getMSF();

        XInterface xInt = (XInterface)xMSF.createInstance(
                "com.sun.star.script.InvocationAdapterFactory");

        TestEnvironment tEnv = new TestEnvironment(xInt);

        return tEnv;
    }
}
