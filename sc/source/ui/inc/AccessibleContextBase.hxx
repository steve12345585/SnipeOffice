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

#pragma once

#include <com/sun/star/accessibility/XAccessible.hpp>
#include <com/sun/star/accessibility/XAccessibleComponent.hpp>
#include <com/sun/star/accessibility/XAccessibleContext.hpp>
#include <com/sun/star/accessibility/XAccessibleEventBroadcaster.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <comphelper/accessiblecomponenthelper.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/interfacecontainer.h>

#include <svl/lstner.hxx>
#include <rtl/ref.hxx>

namespace tools { class Rectangle; }
class AbsoluteScreenPixelRectangle;

class ScAccessibleContextBase
    : public cppu::ImplInheritanceHelper<comphelper::OAccessibleComponentHelper,
                                         css::accessibility::XAccessible, css::lang::XServiceInfo>,
      public SfxListener
{

public:
    //=====  internal  ========================================================
    ScAccessibleContextBase(
        css::uno::Reference<css::accessibility::XAccessible> xParent,
        const sal_Int16 aRole);

    virtual void Init();
    virtual void SAL_CALL disposing() override;
protected:
    virtual ~ScAccessibleContextBase() override;
public:

    /// @throws css::uno::RuntimeException
    bool isShowing();

    /// @throws css::uno::RuntimeException
    virtual bool isVisible();

    ///=====  SfxListener  =====================================================

    virtual void Notify( SfxBroadcaster& rBC, const SfxHint& rHint ) override;

    ///=====  XAccessible  =====================================================

    /// Return the XAccessibleContext.
    virtual css::uno::Reference< css::accessibility::XAccessibleContext> SAL_CALL
        getAccessibleContext() override;

    // OAccessibleComponentHelper
    virtual css::awt::Rectangle implGetBounds() override;

    ///=====  XAccessibleComponent  ============================================

    virtual css::awt::Point SAL_CALL getLocationOnScreen(  ) override;

    virtual void SAL_CALL grabFocus(  ) override;

    virtual sal_Int32 SAL_CALL getForeground(  ) override;

    virtual sal_Int32 SAL_CALL getBackground(  ) override;

    ///=====  XAccessibleContext  ==============================================

    /// Return a reference to the parent.
    virtual css::uno::Reference< css::accessibility::XAccessible> SAL_CALL
        getAccessibleParent() override;

    /// Return this object's role.
    virtual sal_Int16 SAL_CALL
        getAccessibleRole() override;

    /// Return this object's description.
    virtual OUString SAL_CALL
        getAccessibleDescription() override;

    /// Return the object's current name.
    virtual OUString SAL_CALL
        getAccessibleName() override;

    /// Return NULL to indicate that an empty relation set.
    virtual css::uno::Reference<css::accessibility::XAccessibleRelationSet> SAL_CALL
        getAccessibleRelationSet() override;

    /// Return the set of current states.
    virtual sal_Int64 SAL_CALL getAccessibleStateSet() override;

    /** Return the parents locale or throw exception if this object has no
        parent yet/anymore.
    */
    virtual css::lang::Locale SAL_CALL
        getLocale() override;

    ///=====  XServiceInfo  ====================================================

    /** Returns an identifier for the implementation of this object.
    */
    virtual OUString SAL_CALL
        getImplementationName() override;

    /** Return whether the specified service is supported by this class.
    */
    virtual sal_Bool SAL_CALL
        supportsService(const OUString& sServiceName) override;

    /** Returns a list of all supported services.  In this case that is just
        the AccessibleContext and Accessible service.
    */
    virtual css::uno::Sequence< OUString> SAL_CALL
        getSupportedServiceNames() override;

protected:
    /// Return this object's description.
    ///
    /// @throws css::uno::RuntimeException
    virtual OUString
        createAccessibleDescription();

    /// Return the object's current name.
    ///
    /// @throws css::uno::RuntimeException
    virtual OUString
        createAccessibleName();

    /// Return the object's current bounding box relative to the desktop.
    ///
    /// @throws css::uno::RuntimeException
    virtual AbsoluteScreenPixelRectangle GetBoundingBoxOnScreen() const = 0;

    /// Return the object's current bounding box relative to the parent object.
    ///
    /// @throws css::uno::RuntimeException
    virtual tools::Rectangle GetBoundingBox() const = 0;

public:
    /// Calls all listeners to notify them about the change.
    void CommitChange(const sal_Int16 nEventId, const css::uno::Any& rOldValue,
                      const css::uno::Any& rNewValue, sal_Int32 nIndexHint = -1);

    /// Use this method to set initial Name without notification
    void SetName(const OUString& rName) { msName = rName; }

    /// Use this method to set initial Description without notification
    void SetDescription(const OUString& rDesc) { msDescription = rDesc; }

    void SetParent(const css::uno::Reference<css::accessibility::XAccessible>& rParent) { mxParent = rParent; }

protected:
    /// Calls all FocusListener to tell they that the focus is gained.
    void CommitFocusGained();

    /// Calls all FocusListener to tell they that the focus is lost.
    void CommitFocusLost();

    bool IsDefunc() const { return rBHelper.bDisposed; }

    /// Reference to the parent object.
    css::uno::Reference<css::accessibility::XAccessible> mxParent;

private:
    /** Description of this object.  This is not a constant because it can
        be set from the outside.  Furthermore, it changes according to the
        draw page's display mode.
    */
    OUString msDescription;

    /** Name of this object.  It changes according the draw page's
        display mode.
    */
    OUString msName;

    /** This is the role of this object.
    */
    sal_Int16 maRole;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
