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

#include <vector>

#include <rtl/byteseq.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <typelib/typedescription.hxx>
#include <uno/dispatcher.hxx>

namespace binaryurp {
    class BinaryAny;
    class Bridge;
}

namespace binaryurp {

class IncomingRequest {
private:
    IncomingRequest(const IncomingRequest&) = delete;
    IncomingRequest& operator=(const IncomingRequest&) = delete;
public:
    IncomingRequest(
        rtl::Reference< Bridge > const & bridge, rtl::ByteSequence tid,
        OUString oid,
        css::uno::UnoInterfaceReference object,
        css::uno::TypeDescription type,
        sal_uInt16 functionId, bool synchronous,
        css::uno::TypeDescription const & member, bool setter,
        std::vector< BinaryAny >&& inArguments, bool currentContextMode,
        css::uno::UnoInterfaceReference currentContext);

    ~IncomingRequest();

    void execute() const;

private:
    bool execute_throw(
        BinaryAny * returnValue, std::vector< BinaryAny > * outArguments) const;

    rtl::Reference< Bridge > bridge_;
    rtl::ByteSequence tid_;
    OUString oid_; // initial object queryInterface; release
    css::uno::UnoInterfaceReference object_;
    css::uno::TypeDescription type_;
    css::uno::TypeDescription member_;
    css::uno::UnoInterfaceReference currentContext_;
    std::vector< BinaryAny > inArguments_;
    sal_uInt16 functionId_;
    bool synchronous_;
    bool setter_;
    bool currentContextMode_;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
