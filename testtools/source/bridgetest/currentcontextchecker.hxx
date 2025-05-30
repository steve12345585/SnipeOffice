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

#ifndef INCLUDED_TESTTOOLS_SOURCE_BRIDGETEST_CURRENTCONTEXTCHECKER_HXX
#define INCLUDED_TESTTOOLS_SOURCE_BRIDGETEST_CURRENTCONTEXTCHECKER_HXX

#include <sal/config.h>
#include <com/sun/star/uno/Reference.hxx>
#include <cppuhelper/implbase.hxx>
#include <osl/diagnose.hxx>
#include <sal/types.h>
#include <test/testtools/bridgetest/XCurrentContextChecker.hpp>

#include "dllapi.hxx"

namespace testtools::bridgetest {

class LO_DLLPUBLIC_TESTTOOLS CurrentContextChecker :
    public ::osl::DebugBase< CurrentContextChecker >,
    public ::cppu::WeakImplHelper<
        ::test::testtools::bridgetest::XCurrentContextChecker >
{
public:
    CurrentContextChecker();

    virtual ~CurrentContextChecker() override;

    virtual sal_Bool SAL_CALL perform(
        css::uno::Reference< ::test::testtools::bridgetest::XCurrentContextChecker > const & other,
        ::sal_Int32 setSteps, ::sal_Int32 checkSteps) override;

private:
    CurrentContextChecker(CurrentContextChecker const &) = delete;
    void operator =(CurrentContextChecker const &) = delete;

    SAL_DLLPRIVATE bool performCheck(
        css::uno::Reference< ::test::testtools::bridgetest::XCurrentContextChecker > const & other,
        ::sal_Int32 setSteps, ::sal_Int32 checkSteps);
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
