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
package integration.forms;

/** base class for components validating the content of form controls
 */
public abstract class ControlValidator implements com.sun.star.form.validation.XValidator
{

    public void addValidityConstraintListener(com.sun.star.form.validation.XValidityConstraintListener xValidityConstraintListener)
    {
    }

    public void removeValidityConstraintListener(com.sun.star.form.validation.XValidityConstraintListener xValidityConstraintListener)
    {
    }

    protected boolean isVoid( Object Value )
    {
        try
        {
            return ( com.sun.star.uno.AnyConverter.getType(Value).getTypeClass()
                     == com.sun.star.uno.TypeClass.VOID );
        }
        catch( java.lang.ClassCastException e )
        {
        }
        return false;
    }
}
