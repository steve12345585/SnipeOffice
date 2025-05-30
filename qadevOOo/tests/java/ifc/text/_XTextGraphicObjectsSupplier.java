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

package ifc.text;

import lib.MultiMethodTest;

import com.sun.star.container.XNameAccess;
import com.sun.star.text.XTextGraphicObjectsSupplier;

/**
 * Testing <code>com.sun.star.text.XTextGraphicObjectsSupplier</code>
 * interface methods :
 * <ul>
 *  <li><code> getGraphicObjects()</code></li>
 * </ul> <p>
 *
 * The component <b>must have</b> the graphic object with
 * name  'SwXTextDocument'. <p>
 *
 * Test is <b> NOT </b> multithread compliant. <p>
 * @see com.sun.star.text.XTextGraphicObjectsSupplier
 */
public class _XTextGraphicObjectsSupplier extends MultiMethodTest {

    public static XTextGraphicObjectsSupplier oObj = null;

    /**
     * Gets graphic objects collection from the component, and checks
     * if the object with name 'SwXTextDocument' exists. <p>
     * Has <b>OK</b> status if the object exists.
     */
    public void _getGraphicObjects() {
        boolean res = false;

        XNameAccess the_graphics = oObj.getGraphicObjects();
        res = the_graphics.hasByName("SwXTextDocument");

        tRes.tested("getGraphicObjects()",res);
    }

}  // finish class _XTextGraphicObjectsSupplier

