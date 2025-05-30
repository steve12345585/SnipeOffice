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

package ifc.style;

import lib.MultiMethodTest;

import com.sun.star.style.XStyleFamiliesSupplier;

/**
* Testing <code>com.sun.star.style.XStyleFamiliesSupplier</code>
* interface methods :
* <ul>
*  <li><code> getStyleFamilies()</code></li>
* </ul> <p>
* Test is multithread compliant. <p>
* @see com.sun.star.style.XStyleFamiliesSupplier
*/
public class _XStyleFamiliesSupplier extends MultiMethodTest {

    public XStyleFamiliesSupplier oObj = null;

    /**
    * Test calls the method. <p>
    * Has <b> OK </b> status if the method returns not null value.
    */
    public void _getStyleFamilies() {
         tRes.tested("getStyleFamilies()",oObj.getStyleFamilies() != null);
    }
}

