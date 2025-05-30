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

#include "clickableimage.hxx"
#include <com/sun/star/awt/XMouseListener.hpp>


namespace frm
{

class OImageButtonModel
        :public OClickableImageBaseModel
{
public:
    OImageButtonModel(
        const css::uno::Reference< css::uno::XComponentContext>& _rxFactory
    );
    OImageButtonModel(
        const OImageButtonModel* _pOriginal,
        const css::uno::Reference< css::uno::XComponentContext>& _rxFactory
    );
    virtual ~OImageButtonModel() override;

// css::lang::XServiceInfo
    OUString SAL_CALL getImplementationName() override
    { return u"com.sun.star.form.OImageButtonModel"_ustr; }

    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

// css::io::XPersistObject
    virtual OUString SAL_CALL getServiceName() override;
    virtual void SAL_CALL write(const css::uno::Reference< css::io::XObjectOutputStream>& _rxOutStream) override;
    virtual void SAL_CALL read(const css::uno::Reference< css::io::XObjectInputStream>& _rxInStream) override;

    // OControlModel's property handling
    virtual void describeFixedProperties(
        css::uno::Sequence< css::beans::Property >& /* [out] */ _rProps
    ) const override;

protected:
    virtual css::uno::Reference< css::util::XCloneable > SAL_CALL createClone(  ) override;
};

typedef ::cppu::ImplHelper1< css::awt::XMouseListener> OImageButtonControl_BASE;
class OImageButtonControl : public OClickableImageBaseControl,
                            public OImageButtonControl_BASE
{
protected:
    // UNO Binding
    virtual css::uno::Sequence< css::uno::Type> _getTypes() override;

public:
    explicit OImageButtonControl(const css::uno::Reference< css::uno::XComponentContext>& _rxFactory);

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override
    { return u"com.sun.star.form.OImageButtonControl"_ustr; }

    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    // UNO Binding
    DECLARE_UNO3_AGG_DEFAULTS(OImageButtonControl, OClickableImageBaseControl)
    virtual css::uno::Any SAL_CALL queryAggregation(const css::uno::Type& _rType) override;

    // XEventListener
    virtual void SAL_CALL disposing(const css::lang::EventObject& _rSource) override
        { OControl::disposing(_rSource); }

    // XMouseListener
    virtual void SAL_CALL mousePressed(const css::awt::MouseEvent& e) override;
    virtual void SAL_CALL mouseReleased(const css::awt::MouseEvent& e) override;
    virtual void SAL_CALL mouseEntered(const css::awt::MouseEvent& e) override;
    virtual void SAL_CALL mouseExited(const css::awt::MouseEvent& e) override;

    // prevent method hiding
    using OClickableImageBaseControl::disposing;
};


}   // namespace frm


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
