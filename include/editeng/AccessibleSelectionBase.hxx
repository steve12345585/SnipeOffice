/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#ifndef INCLUDED_EDITENG_ACCESSIBLESELECTIONBASE_HXX
#define INCLUDED_EDITENG_ACCESSIBLESELECTIONBASE_HXX

#include <comphelper/accessibleselectionhelper.hxx>
#include <editeng/editengdllapi.h>

namespace accessibility
{

/** @descr
        This base class provides a base implementation of the
        XAccessibleSelection interface.
        The following methods have to be implemented if this
        class is used:

        <method>implGetMutex</method>,
        <method>implGetAccessibleContext</method>,
        <method>implIsSelected</method>,
        <method>implSelect</method>,
*/
class EDITENG_DLLPUBLIC AccessibleSelectionBase : public ::comphelper::OCommonAccessibleSelection,
                                public css::accessibility::XAccessibleSelection
{
protected:

    virtual ::osl::Mutex&   implGetMutex() = 0;

public:

    // XAccessibleSelection - default implementations
    virtual void SAL_CALL selectAccessibleChild( sal_Int64 nChildIndex ) override final;
    virtual sal_Bool SAL_CALL isAccessibleChildSelected( sal_Int64 nChildIndex ) override final;
    virtual void SAL_CALL clearAccessibleSelection(  ) override final;
    virtual void SAL_CALL selectAllAccessibleChildren(  ) override final;
    virtual sal_Int64 SAL_CALL getSelectedAccessibleChildCount(  ) override final;
    virtual css::uno::Reference< css::accessibility::XAccessible > SAL_CALL getSelectedAccessibleChild( sal_Int64 nSelectedChildIndex ) override final;
    virtual void SAL_CALL deselectAccessibleChild( sal_Int64 nSelectedChildIndex ) override final;

public:

                            AccessibleSelectionBase();
    virtual                 ~AccessibleSelectionBase();
};

}

#endif // INCLUDED_EDITENG_ACCESSIBLESELECTIONBASE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
