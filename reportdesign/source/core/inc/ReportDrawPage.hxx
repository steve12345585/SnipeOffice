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

#ifndef INCLUDED_REPORTDESIGN_SOURCE_CORE_INC_REPORTDRAWPAGE_HXX
#define INCLUDED_REPORTDESIGN_SOURCE_CORE_INC_REPORTDRAWPAGE_HXX

#include <svx/unopage.hxx>
#include <com/sun/star/report/XSection.hpp>
#include <cppuhelper/weakref.hxx>

namespace reportdesign
{
    class OReportDrawPage : public SvxDrawPage
    {
        css::uno::WeakReference< css::report::XSection > m_xSection;
        OReportDrawPage(const OReportDrawPage&) = delete;
        void operator =(const OReportDrawPage&) = delete;
    protected:
        virtual rtl::Reference<SdrObject> CreateSdrObject_( const css::uno::Reference< css::drawing::XShape > & xShape ) override;
        virtual css::uno::Reference< css::drawing::XShape >  CreateShape( SdrObject *pObj ) const override;
    public:
        OReportDrawPage(SdrPage* pPage,const css::uno::Reference< css::report::XSection >& _xSection);
    };
}
#endif // INCLUDED_REPORTDESIGN_SOURCE_CORE_INC_REPORTDRAWPAGE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
