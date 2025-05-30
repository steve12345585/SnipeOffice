/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <comphelper/compbase.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ui/test/XUIObject.hpp>

#include <memory>

#include <vcl/uitest/uiobject.hxx>

typedef ::comphelper::WeakComponentImplHelper <
    css::ui::test::XUIObject, css::lang::XServiceInfo
    > UIObjectBase;

class UIObjectUnoObj : public UIObjectBase
{
private:
    std::unique_ptr<UIObject> mpObj;

public:

    explicit UIObjectUnoObj(std::unique_ptr<UIObject> pObj);
    virtual ~UIObjectUnoObj() override;

    css::uno::Reference<css::ui::test::XUIObject> SAL_CALL getChild(const OUString& rID) override;

    void SAL_CALL executeAction(const OUString& rAction, const css::uno::Sequence<css::beans::PropertyValue>& xPropValues) override;

    css::uno::Sequence<css::beans::PropertyValue> SAL_CALL getState() override;

    css::uno::Sequence<OUString> SAL_CALL getChildren() override;

    OUString SAL_CALL getType() override;

    OUString SAL_CALL getImplementationName() override;

    sal_Bool SAL_CALL supportsService(OUString const & ServiceName) override;

    css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

    OUString SAL_CALL getHierarchy() override;

    sal_Bool SAL_CALL equals(const css::uno::Reference<css::ui::test::XUIObject>& rOther) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
