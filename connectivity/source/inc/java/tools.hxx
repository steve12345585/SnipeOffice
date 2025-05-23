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

#include <sal/config.h>

#include <memory>

#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/uno/Sequence.h>

#if defined __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-attributes"
#endif
#include <jni.h>
#if defined __clang__
#pragma clang diagnostic pop
#endif

#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/io/XInputStream.hpp>


namespace connectivity
{

    jstring convertwchar_tToJavaString(JNIEnv *pEnv,const OUString& Temp);
    OUString JavaString2String(JNIEnv *pEnv,jstring Str);
    class java_util_Properties;

    /// @throws css::sdbc::SQLException
    /// @throws css::uno::RuntimeException
    std::unique_ptr<java_util_Properties> createStringPropertyArray(const css::uno::Sequence< css::beans::PropertyValue >& info );

    jobject convertTypeMapToJavaMap(const css::uno::Reference< css::container::XNameAccess > & _rMap);

    /** return if an exception occurred
        the exception will be cleared.
        @param  pEnv
            The native java env
    */
    bool isExceptionOccurred(JNIEnv *pEnv);

    jobject createByteInputStream(const css::uno::Reference< css::io::XInputStream >& x,sal_Int32 length);
    jobject createCharArrayReader(const css::uno::Reference< css::io::XInputStream >& x,sal_Int32 length);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
