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

#include <com/sun/star/bridge/UnoUrlResolver.hpp>
#include <com/sun/star/bridge/XUnoUrlResolver.hpp>
#include <com/sun/star/connection/NoConnectException.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/bootstrap.hxx>
#include <cppunit/TestAssert.h>
#include <osl/process.h>
#include <osl/test/uniquepipename.hxx>
#include <osl/time.h>
#include <unotest/getargument.hxx>
#include <unotest/officeconnection.hxx>
#include <unotest/toabsolutefileurl.hxx>

namespace test {

OfficeConnection::OfficeConnection(): process_(nullptr) {}

OfficeConnection::~OfficeConnection() {}

void OfficeConnection::setUp() {
    css::uno::Reference< css::bridge::XUnoUrlResolver > resolver(
        css::bridge::UnoUrlResolver::create(
            cppu::defaultBootstrap_InitialComponentContext()));
    OUString desc;
    OUString argSoffice;
    CPPUNIT_ASSERT(
        getArgument(
            u"soffice",
            &argSoffice));
    if (argSoffice.match("path:")) {
        desc = "pipe,name=" + osl::test::uniquePipeName(u"oootest"_ustr);
        OUString noquickArg(u"--quickstart=no"_ustr);
        OUString norestoreArg(u"--norestore"_ustr);
        OUString nologoArg(u"--nologo"_ustr);
            // disable use of the unix standalone splash screen app for the
            // tests (probably not needed in combination with --headless?)
        OUString headlessArg(u"--headless"_ustr);
        OUString acceptArg("--accept=" + desc + ";urp");
        OUString argUser;
        CPPUNIT_ASSERT(
            getArgument(u"user", &argUser));
        OUString userArg("-env:UserInstallation=" + toAbsoluteFileUrl(argUser));
        OUString jreArg(
            u"-env:UNO_JAVA_JFW_ENV_JREHOME=true"_ustr);
        rtl_uString * args[] = {
            noquickArg.pData, norestoreArg.pData,
            nologoArg.pData, headlessArg.pData, acceptArg.pData, userArg.pData,
            jreArg.pData };
        rtl_uString ** envs = nullptr;
        OUString argEnv;
        if (getArgument(u"env", &argEnv))
        {
            envs = &argEnv.pData;
        }
        // coverity[callee_ptr_arith] - arith is fine
        CPPUNIT_ASSERT_EQUAL(
            osl_Process_E_None,
            osl_executeProcess(
                toAbsoluteFileUrl(
                    argSoffice.copy(RTL_CONSTASCII_LENGTH("path:"))).pData,
                args, std::size(args), 0, nullptr, nullptr, envs, envs == nullptr ? 0 : 1,
                &process_));
    } else if (argSoffice.match("connect:")) {
        desc = argSoffice.copy(RTL_CONSTASCII_LENGTH("connect:"));
    } else {
        CPPUNIT_FAIL(
            "\"soffice\" argument starts with neither \"path:\" nor"
            " \"connect:\"");
    }
    for (;;) {
        try {
            context_ =
                css::uno::Reference< css::uno::XComponentContext >(
                    resolver->resolve(
                        "uno:" + desc + ";urp;StarOffice.ComponentContext"),
                    css::uno::UNO_QUERY_THROW);
            break;
        } catch (css::connection::NoConnectException &) {}
        if (process_ != nullptr) {
            TimeValue delay = { 1, 0 }; // 1 sec
            CPPUNIT_ASSERT_EQUAL(
                osl_Process_E_TimedOut,
                osl_joinProcessWithTimeout(process_, &delay));
        }
    }
}

void OfficeConnection::tearDown() {
    if (process_ == nullptr)
        return;

    if (context_.is()) {
        css::uno::Reference< css::frame::XDesktop2 > desktop = css::frame::Desktop::create( context_ );
        context_.clear();
        try {
            CPPUNIT_ASSERT(desktop->terminate());
            desktop.clear();
        } catch (css::lang::DisposedException &) {}
            // it appears that DisposedExceptions can already happen while
            // receiving the response of the terminate call
    }
    CPPUNIT_ASSERT_EQUAL(osl_Process_E_None, osl_joinProcess(process_));
    oslProcessInfo info;
    info.Size = sizeof info;
    CPPUNIT_ASSERT_EQUAL(
        osl_Process_E_None,
        osl_getProcessInfo(process_, osl_Process_EXITCODE, &info));
    CPPUNIT_ASSERT_EQUAL(oslProcessExitCode(0), info.Code);
    osl_freeProcessHandle(process_);
    process_ = nullptr; // guard against subsequent calls to isStillAlive
}


bool OfficeConnection::isStillAlive() const {
    if (process_ == nullptr) {
        // In case "soffice" argument starts with "connect:" we have no direct
        // control over the liveness of the soffice.bin process (would need to
        // directly monitor the bridge) so can only assume the best here:
        return true;
    }
    TimeValue delay = { 0, 0 }; // 0 sec
    oslProcessError e = osl_joinProcessWithTimeout(process_, &delay);
    CPPUNIT_ASSERT(e == osl_Process_E_None || e == osl_Process_E_TimedOut);
    return e == osl_Process_E_TimedOut;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
