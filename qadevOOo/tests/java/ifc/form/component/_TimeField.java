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

package ifc.form.component;

import lib.MultiPropertyTest;
import util.utils;

/**
* Testing <code>com.sun.star.form.component.TimeField</code>
* service properties :
* <ul>
*  <li><code> DefaultTime</code></li>
* </ul> <p>
* Properties testing is automated by <code>lib.MultiPropertyTest</code>.
* @see com.sun.star.form.component.TimeField
*/
public class _TimeField extends MultiPropertyTest {

    /**
    * This property can be VOID, and in case if it is so new
    * value must defined.
    */
    public void _DefaultTime() {
        testProperty("DefaultTime", new PropertyTester() {
            @Override
            protected Object getNewValue(String p, Object old) {
                return utils.isVoid(old) ? Integer.valueOf(4321) :
                    super.getNewValue(p, old) ;
            }
        }) ;
    }
}

