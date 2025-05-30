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

#include <deque>
#include <mutex>
#include <vector>

#include <osl/conditn.hxx>
#include <rtl/byteseq.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <salhelper/thread.hxx>
#include <typelib/typedescription.hxx>
#include <uno/dispatcher.hxx>

#include "binaryany.hxx"
#include "marshal.hxx"
#include "writerstate.hxx"

namespace binaryurp { class Bridge; }

namespace binaryurp {

class Writer: public salhelper::Thread
{
public:
    explicit Writer(rtl::Reference< Bridge > const & bridge);

    // Only called from Bridge::reader_ thread, and only before Bridge::writer_
    // thread is unblocked:
    void sendDirectRequest(
        rtl::ByteSequence const & tid, OUString const & oid,
        css::uno::TypeDescription const & type,
        css::uno::TypeDescription const & member,
        std::vector< BinaryAny > const & inArguments);

    // Only called from Bridge::reader_ thread, and only before Bridge::writer_
    // thread is unblocked:
    void sendDirectReply(
        rtl::ByteSequence const & tid,
        css::uno::TypeDescription const & member,
        bool exception, BinaryAny const & returnValue,
        std::vector< BinaryAny > const & outArguments);

    void queueRequest(
        rtl::ByteSequence const & tid, OUString const & oid,
        css::uno::TypeDescription const & type,
        css::uno::TypeDescription const & member,
        std::vector< BinaryAny >&& inArguments);

    void queueReply(
        rtl::ByteSequence const & tid,
        css::uno::TypeDescription const & member, bool setter,
        bool exception, BinaryAny const & returnValue,
        std::vector< BinaryAny >&& outArguments,
        bool setCurrentContextMode);

    void unblock();

    void stop();

private:
    virtual ~Writer() override;

    virtual void execute() override;

    void sendRequest(
        rtl::ByteSequence const & tid, OUString const & oid,
        css::uno::TypeDescription const & type,
        css::uno::TypeDescription const & member,
        std::vector< BinaryAny > const & inArguments, bool currentContextMode,
        css::uno::UnoInterfaceReference const & currentContext);

    void sendReply(
        rtl::ByteSequence const & tid,
        css::uno::TypeDescription const & member, bool setter,
        bool exception, BinaryAny const & returnValue,
        std::vector< BinaryAny > const & outArguments);

    void sendMessage(std::vector< unsigned char > const & buffer);

    struct Item {
        Item();

        // Request:
        Item(
            rtl::ByteSequence theTid, OUString theOid,
            css::uno::TypeDescription theType,
            css::uno::TypeDescription theMember,
            std::vector< BinaryAny >&& inArguments,
            css::uno::UnoInterfaceReference theCurrentContext);

        // Reply:
        Item(
            rtl::ByteSequence theTid,
            css::uno::TypeDescription theMember,
            bool theSetter, bool theException, BinaryAny theReturnValue,
            std::vector< BinaryAny >&& outArguments,
            bool theSetCurrentContextMode);

        rtl::ByteSequence tid; // request + reply
        OUString oid; // request
        css::uno::TypeDescription type; // request
        css::uno::TypeDescription member; // request + reply
        css::uno::UnoInterfaceReference currentContext; // request
        BinaryAny returnValue; // reply
        std::vector< BinaryAny > arguments; // request: inArguments; reply: outArguments
        bool request;
        bool setter; // reply
        bool exception; // reply
        bool setCurrentContextMode; // reply
    };

    rtl::Reference< Bridge > bridge_;
    WriterState state_;
    Marshal marshal_;
    css::uno::TypeDescription lastType_;
    OUString lastOid_;
    rtl::ByteSequence lastTid_;
    osl::Condition unblocked_;
    osl::Condition items_;

    std::mutex mutex_;
    std::deque< Item > queue_;
    bool stop_;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
