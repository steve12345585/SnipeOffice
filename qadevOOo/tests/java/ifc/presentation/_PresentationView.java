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

package ifc.presentation;

import lib.MultiPropertyTest;

public class _PresentationView extends MultiPropertyTest {

    /**
     * Property tester which changes DrawPage.
     */
    protected PropertyTester PageTester = new PropertyTester() {
        @Override
        protected Object getNewValue(String propName, Object oldValue)
                throws java.lang.IllegalArgumentException {
            if (oldValue.equals(tEnv.getObjRelation("FirstPage")))
                return tEnv.getObjRelation("SecondPage"); else
                return tEnv.getObjRelation("FirstPage");
        }
    } ;

    /**
     * This property must be an XDrawPage
     */
    public void _CurrentPage() {
        log.println("Testing with custom Property tester") ;
        testProperty("CurrentPage", PageTester) ;
    }

}  // finish class _PresentationView


