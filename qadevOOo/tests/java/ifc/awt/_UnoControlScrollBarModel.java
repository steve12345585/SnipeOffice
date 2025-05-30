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

package ifc.awt;

import lib.MultiPropertyTest;
import util.utils;

/**
* Testing <code>com.sun.star.awt.UnoControlScrollBarModel</code>
* service properties :
* <ul>
*  <li><code> BlockIncrement</code></li>
*  <li><code> Border</code></li>
*  <li><code> DefaultControl</code></li>
*  <li><code> Enabled</code></li>
*  <li><code> HelpText</code></li>
*  <li><code> HelpURL</code></li>
*  <li><code> LineIncrement</code></li>
*  <li><code> Orientation</code></li>
*  <li><code> Printable</code></li>
*  <li><code> ScrollValue</code></li>
*  <li><code> ScrollValueMax</code></li>
*  <li><code> VisibleSize</code></li>
* </ul> <p>
* Properties testing is automated by <code>lib.MultiPropertyTest</code>.
* @see com.sun.star.awt.UnoControlScrollBarModel
*/
public class _UnoControlScrollBarModel extends MultiPropertyTest {

    /**
    * This property can be VOID, and in case if it is so new
    * value must defined.
    */
    public void _BackgroundColor() {
        testProperty("BackgroundColor", new PropertyTester() {
            @Override
            protected Object getNewValue(String p, Object old) {
                return utils.isVoid(old) ? Integer.valueOf(32768) : null ;
            }
        }) ;
    }

  /**
    * This property can be VOID, and in case if it is so new
    * value must defined.
    */
    public void _BorderColor() {
        testProperty("BorderColor", new PropertyTester() {
            @Override
            protected Object getNewValue(String p, Object old) {
                return utils.isVoid(old) ? Integer.valueOf(1234) : null ;
            }
        }) ;
    }

    /**
    * This property can be VOID, and in case if it is so new
    * value must defined.
    */
    public void _SymbolColor() {
        testProperty("SymbolColor", new PropertyTester() {
            @Override
            protected Object getNewValue(String p, Object old) {
                return utils.isVoid(old) ? Integer.valueOf(65324) : null ;
            }
        }) ;
    }

    /**
    * This property can be VOID, and in case if it is so new
    * value must defined.
    */
    public void _ScrollValue() {
        testProperty("ScrollValue", new PropertyTester() {
            @Override
            protected Object getNewValue(String p, Object old) {
                return utils.isVoid(old) ? Integer.valueOf(10) : Integer.valueOf(15) ;
            }
        }) ;
    }

    /**
    * This property can be VOID, and in case if it is so new
    * value must defined.
    */
    public void _VisibleSize() {
        testProperty("VisibleSize", new PropertyTester() {
            @Override
            protected Object getNewValue(String p, Object old) {
                return utils.isVoid(old) ? Integer.valueOf(10) : null ;
            }
        }) ;
    }
}

