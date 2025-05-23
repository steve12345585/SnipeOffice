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

#ifndef INCLUDED_VBAHELPER_SOURCE_MSFORMS_VBASCROLLBAR_HXX
#define INCLUDED_VBAHELPER_SOURCE_MSFORMS_VBASCROLLBAR_HXX

#include <cppuhelper/implbase.hxx>
#include <ooo/vba/msforms/XScrollBar.hpp>

#include "vbacontrol.hxx"
#include <vbahelper/vbahelper.hxx>

typedef cppu::ImplInheritanceHelper< ScVbaControl, ov::msforms::XScrollBar > ScrollBarImpl_BASE;

class ScVbaScrollBar : public ScrollBarImpl_BASE
{
public:
    ScVbaScrollBar( const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext, const css::uno::Reference< css::uno::XInterface >& xControl, const css::uno::Reference< css::frame::XModel >& xModel, std::unique_ptr<ov::AbstractGeometryAttributes> pGeomHelper  );
   // Attributes
    virtual css::uno::Any SAL_CALL getValue() override;
    virtual void SAL_CALL setValue( const css::uno::Any& _value ) override;
    virtual ::sal_Int32 SAL_CALL getMax() override;
    virtual void SAL_CALL setMax( ::sal_Int32 _max ) override;
    virtual ::sal_Int32 SAL_CALL getMin() override;
    virtual void SAL_CALL setMin( ::sal_Int32 _min ) override;
    virtual ::sal_Int32 SAL_CALL getLargeChange() override;
    virtual void SAL_CALL setLargeChange( ::sal_Int32 _largechange ) override;
    virtual ::sal_Int32 SAL_CALL getSmallChange() override;
    virtual void SAL_CALL setSmallChange( ::sal_Int32 _smallchange ) override;


    //XHelperInterface
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;
};

#endif // INCLUDED_VBAHELPER_SOURCE_MSFORMS_VBASCROLLBAR_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
