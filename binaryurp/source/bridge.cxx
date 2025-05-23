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

#include <sal/config.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include <com/sun/star/bridge/InvalidProtocolChangeException.hpp>
#include <com/sun/star/bridge/XBridge.hpp>
#include <com/sun/star/bridge/XInstanceProvider.hpp>
#include <com/sun/star/bridge/XProtocolProperties.hpp>
#include <com/sun/star/connection/XConnection.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/EventObject.hpp>
#include <com/sun/star/lang/XEventListener.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/XInterface.hpp>
#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/weak.hxx>
#include <osl/mutex.hxx>
#include <osl/thread.hxx>
#include <rtl/byteseq.hxx>
#include <rtl/random.h>
#include <rtl/ref.hxx>
#include <rtl/string.h>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <sal/types.h>
#include <typelib/typeclass.h>
#include <typelib/typedescription.h>
#include <typelib/typedescription.hxx>
#include <uno/dispatcher.hxx>
#include <uno/environment.hxx>
#include <uno/lbnames.h>

#include "binaryany.hxx"
#include "bridge.hxx"
#include "bridgefactory.hxx"
#include "incomingreply.hxx"
#include "lessoperators.hxx"
#include "outgoingrequest.hxx"
#include "outgoingrequests.hxx"
#include "proxy.hxx"
#include "reader.hxx"

namespace binaryurp {

namespace {

sal_Int32 random() {
    sal_Int32 n;
    (void)rtl_random_getBytes(nullptr, &n, sizeof n);
    return n;
}

OUString toString(css::uno::TypeDescription const & type) {
    typelib_TypeDescription * d = type.get();
    assert(d != nullptr && d->pTypeName != nullptr);
    return OUString(d->pTypeName);
}

extern "C" void freeProxyCallback(
    SAL_UNUSED_PARAMETER uno_ExtEnvironment *, void * pProxy)
{
    assert(pProxy != nullptr);
    static_cast< Proxy * >(pProxy)->do_free();
}

bool isThread(salhelper::Thread * thread) {
    assert(thread != nullptr);
    return osl::Thread::getCurrentIdentifier() == thread->getIdentifier();
}

class AttachThread {
public:
    explicit AttachThread(uno_ThreadPool threadPool);

    ~AttachThread();

    const rtl::ByteSequence& getTid() const noexcept { return tid_;}

private:
    AttachThread(const AttachThread&) = delete;
    AttachThread& operator=(const AttachThread&) = delete;

    uno_ThreadPool threadPool_;
    rtl::ByteSequence tid_;
};

AttachThread::AttachThread(uno_ThreadPool threadPool): threadPool_(threadPool) {
    sal_Sequence * s = nullptr;
    uno_getIdOfCurrentThread(&s);
    tid_ = rtl::ByteSequence(s, rtl::BYTESEQ_NOACQUIRE);
    uno_threadpool_attach(threadPool_);
}

AttachThread::~AttachThread() {
    uno_threadpool_detach(threadPool_);
    uno_releaseIdFromCurrentThread();
}


class PopOutgoingRequest {
public:
    PopOutgoingRequest(
        OutgoingRequests & requests, rtl::ByteSequence tid,
        OutgoingRequest const & request);

    ~PopOutgoingRequest();

    void clear();

private:
    PopOutgoingRequest(const PopOutgoingRequest&) = delete;
    PopOutgoingRequest& operator=(const PopOutgoingRequest&) = delete;

    OutgoingRequests & requests_;
    rtl::ByteSequence tid_;
    bool cleared_;
};

PopOutgoingRequest::PopOutgoingRequest(
    OutgoingRequests & requests, rtl::ByteSequence tid,
    OutgoingRequest const & request):
    requests_(requests), tid_(std::move(tid)), cleared_(false)
{
    requests_.push(tid_, request);
}

PopOutgoingRequest::~PopOutgoingRequest() {
    if (!cleared_) {
        requests_.pop(tid_);
    }
}

void PopOutgoingRequest::clear() {
    cleared_ = true;
}

}

struct Bridge::SubStub {
    css::uno::UnoInterfaceReference object;

    sal_uInt32 references;
};

Bridge::Bridge(
    rtl::Reference< BridgeFactory > const & factory, OUString name,
    css::uno::Reference< css::connection::XConnection > const & connection,
    css::uno::Reference< css::bridge::XInstanceProvider > provider):
    factory_(factory), name_(std::move(name)), connection_(connection),
    provider_(std::move(provider)),
    binaryUno_(u"" UNO_LB_UNO ""_ustr),
    cppToBinaryMapping_(CPPU_CURRENT_LANGUAGE_BINDING_NAME, u"" UNO_LB_UNO ""_ustr),
    binaryToCppMapping_(u"" UNO_LB_UNO ""_ustr, CPPU_CURRENT_LANGUAGE_BINDING_NAME),
    protPropTid_(
        reinterpret_cast< sal_Int8 const * >(".UrpProtocolPropertiesTid"),
        RTL_CONSTASCII_LENGTH(".UrpProtocolPropertiesTid")),
    protPropOid_(u"UrpProtocolProperties"_ustr),
    protPropType_(
        cppu::UnoType<
            css::uno::Reference< css::bridge::XProtocolProperties > >::get()),
    protPropRequest_(u"com.sun.star.bridge.XProtocolProperties::requestChange"_ustr),
    protPropCommit_(u"com.sun.star.bridge.XProtocolProperties::commitChange"_ustr),
    state_(STATE_INITIAL), threadPool_(nullptr), currentContextMode_(false),
    proxies_(0), calls_(0), normalCall_(false), activeCalls_(0),
    mode_(MODE_REQUESTED)
{
    assert(factory.is() && connection.is());
    if (!binaryUno_.is()) {
        throw css::uno::RuntimeException(u"URP: no binary UNO environment"_ustr);
    }
    if (!(cppToBinaryMapping_.is() && binaryToCppMapping_.is())) {
        throw css::uno::RuntimeException(u"URP: no C++ UNO mapping"_ustr);
    }
    passive_.set();
    // coverity[uninit_member] - random_ is set in due course by the reader_ thread's state machine
}

void Bridge::start() {
    rtl::Reference r(new Reader(this));
    rtl::Reference w(new Writer(this));
    {
        std::lock_guard g(mutex_);
        assert(
            state_ == STATE_INITIAL && threadPool_ == nullptr && !writer_.is() &&
            !reader_.is());
        threadPool_ = uno_threadpool_create();
        assert(threadPool_ != nullptr);
        reader_ = r;
        writer_ = w;
        state_ = STATE_STARTED;
    }
    // It is important to call reader_->launch() last here; both
    // Writer::execute and Reader::execute can call Bridge::terminate, but
    // Writer::execute is initially blocked in unblocked_.wait() until
    // Reader::execute has called bridge_->sendRequestChangeRequest(), so
    // effectively only reader_->launch() can lead to an early call to
    // Bridge::terminate
    w->launch();
    r->launch();
}

void Bridge::terminate(bool final) {
    uno_ThreadPool tp;
    // Make sure function-local variables (Stubs s, etc.) are destroyed before
    // the final uno_threadpool_destroy/threadPool_ = 0:
    {
        rtl::Reference< Reader > r;
        rtl::Reference< Writer > w;
        bool joinW;
        Listeners ls;
        {
            std::unique_lock g(mutex_);
            switch (state_) {
            case STATE_INITIAL: // via ~Bridge -> dispose -> terminate
            case STATE_FINAL:
                return;
            case STATE_STARTED:
                break;
            case STATE_TERMINATED:
                if (final) {
                    g.unlock();
                    terminated_.wait();
                    {
                        std::lock_guard g2(mutex_);
                        tp = threadPool_;
                        threadPool_ = nullptr;
                        if (reader_.is()) {
                            if (!isThread(reader_.get())) {
                                r = reader_;
                            }
                            reader_.clear();
                        }
                        if (writer_.is()) {
                            if (!isThread(writer_.get())) {
                                w = writer_;
                            }
                            writer_.clear();
                        }
                        state_ = STATE_FINAL;
                    }
                    assert(!(r.is() && w.is()));
                    if (r.is()) {
                        r->join();
                    } else if (w.is()) {
                        w->join();
                    }
                    if (tp != nullptr) {
                        uno_threadpool_destroy(tp);
                    }
                }
                return;
            }
            tp = threadPool_;
            assert(!(final && isThread(reader_.get())));
            if (!isThread(reader_.get())) {
                std::swap(reader_, r);
            }
            w = writer_;
            joinW = !isThread(writer_.get());
            assert(!final || joinW);
            if (joinW) {
                writer_.clear();
            }
            ls.swap(listeners_);
            state_ = final ? STATE_FINAL : STATE_TERMINATED;
        }
        try {
            connection_->close();
        } catch (const css::io::IOException & e) {
            SAL_INFO("binaryurp", "caught IO exception '" << e << '\'');
        }
        assert(w.is());
        w->stop();
        if (r.is()) {
            r->join();
        }
        if (joinW) {
            w->join();
        }
        assert(tp != nullptr);
        uno_threadpool_dispose(tp);
        Stubs s;
        {
            std::lock_guard g(mutex_);
            s.swap(stubs_);
        }
        for (auto & stub : s)
        {
            for (auto & item : stub.second)
            {
                SAL_INFO(
                    "binaryurp",
                    "stub '" << stub.first << "', '" << toString(item.first)
                        << "' still mapped at Bridge::terminate");
                binaryUno_.get()->pExtEnv->revokeInterface(
                    binaryUno_.get()->pExtEnv, item.second.object.get());
            }
        }
        factory_->removeBridge(this);
        for (auto const& listener : ls)
        {
            try {
                listener->disposing(
                    css::lang::EventObject(
                        getXWeak()));
            } catch (const css::uno::RuntimeException & e) {
                SAL_WARN("binaryurp", "caught " << e);
            }
        }
    }
    if (final) {
        uno_threadpool_destroy(tp);
    }
    {
        std::lock_guard g(mutex_);
        if (final) {
            threadPool_ = nullptr;
        }
    }
    terminated_.set();
}


BinaryAny Bridge::mapCppToBinaryAny(css::uno::Any const & cppAny) {
    css::uno::Any in(cppAny);
    BinaryAny out;
    out.~BinaryAny();
    uno_copyAndConvertData(
        &out.get(), &in,
        css::uno::TypeDescription(cppu::UnoType< css::uno::Any >::get()).get(),
        cppToBinaryMapping_.get());
    return out;
}

uno_ThreadPool Bridge::getThreadPool() {
    std::lock_guard g(mutex_);
    checkDisposed();
    assert(threadPool_ != nullptr);
    return threadPool_;
}

rtl::Reference< Writer > Bridge::getWriter() {
    std::lock_guard g(mutex_);
    checkDisposed();
    assert(writer_.is());
    return writer_;
}

css::uno::UnoInterfaceReference Bridge::registerIncomingInterface(
    OUString const & oid, css::uno::TypeDescription const & type)
{
    assert(type.is());
    if (oid.isEmpty()) {
        return css::uno::UnoInterfaceReference();
    }
    css::uno::UnoInterfaceReference obj(findStub(oid, type));
    if (!obj.is()) {
        binaryUno_.get()->pExtEnv->getRegisteredInterface(
            binaryUno_.get()->pExtEnv,
            reinterpret_cast< void ** >(&obj.m_pUnoI), oid.pData,
            reinterpret_cast< typelib_InterfaceTypeDescription * >(type.get()));
        if (obj.is()) {
            makeReleaseCall(oid, type);
        } else {
            obj.set(new Proxy(this, oid, type), SAL_NO_ACQUIRE);
            {
                std::lock_guard g(mutex_);
                assert(proxies_ < std::numeric_limits< std::size_t >::max());
                ++proxies_;
            }
            binaryUno_.get()->pExtEnv->registerProxyInterface(
                binaryUno_.get()->pExtEnv,
                reinterpret_cast< void ** >(&obj.m_pUnoI), &freeProxyCallback,
                oid.pData,
                reinterpret_cast< typelib_InterfaceTypeDescription * >(
                    type.get()));
        }
    }
    return obj;
}

OUString Bridge::registerOutgoingInterface(
    css::uno::UnoInterfaceReference const & object,
    css::uno::TypeDescription const & type)
{
    assert(type.is());
    if (!object.is()) {
        return OUString();
    }
    OUString oid;
    if (!Proxy::isProxy(this, object, &oid)) {
        binaryUno_.get()->pExtEnv->getObjectIdentifier(
            binaryUno_.get()->pExtEnv, &oid.pData, object.get());
        std::lock_guard g(mutex_);
        Stubs::iterator i(stubs_.find(oid));
        Stub newStub;
        Stub * stub = i == stubs_.end() ? &newStub : &i->second;
        Stub::iterator j(stub->find(type));
        //TODO: Release sub-stub if it is not successfully sent to remote side
        // (otherwise, stub will leak until terminate()):
        if (j == stub->end()) {
            j = stub->emplace(type, SubStub()).first;
            if (stub == &newStub) {
                i = stubs_.emplace(oid, Stub()).first;
                std::swap(i->second, newStub);
                j = i->second.find(type);
                assert(j !=  i->second.end());
            }
            j->second.object = object;
            j->second.references = 1;
            binaryUno_.get()->pExtEnv->registerInterface(
                binaryUno_.get()->pExtEnv,
                reinterpret_cast< void ** >(&j->second.object.m_pUnoI),
                oid.pData,
                reinterpret_cast< typelib_InterfaceTypeDescription * >(
                    type.get()));
        } else {
            assert(stub != &newStub);
            if (j->second.references == SAL_MAX_UINT32) {
                throw css::uno::RuntimeException(
                    u"URP: stub reference count overflow"_ustr);
            }
            ++j->second.references;
        }
    }
    return oid;
}

css::uno::UnoInterfaceReference Bridge::findStub(
    OUString const & oid, css::uno::TypeDescription const & type)
{
    assert(!oid.isEmpty() && type.is());
    std::lock_guard g(mutex_);
    Stubs::iterator i(stubs_.find(oid));
    if (i != stubs_.end()) {
        Stub::iterator j(i->second.find(type));
        if (j != i->second.end()) {
            return j->second.object;
        }
        for (auto const& item : i->second)
        {
            if (typelib_typedescription_isAssignableFrom(
                    type.get(), item.first.get()))
            {
                return item.second.object;
            }
        }
    }
    return css::uno::UnoInterfaceReference();
}

void Bridge::releaseStub(
    OUString const & oid, css::uno::TypeDescription const & type)
{
    assert(!oid.isEmpty() && type.is());
    css::uno::UnoInterfaceReference obj;
    bool unused;
    {
        std::lock_guard g(mutex_);
        Stubs::iterator i(stubs_.find(oid));
        if (i == stubs_.end()) {
            throw css::uno::RuntimeException(u"URP: release unknown stub"_ustr);
        }
        Stub::iterator j(i->second.find(type));
        if (j == i->second.end()) {
            throw css::uno::RuntimeException(u"URP: release unknown stub"_ustr);
        }
        assert(j->second.references > 0);
        --j->second.references;
        if (j->second.references == 0) {
            obj = j->second.object;
            i->second.erase(j);
            if (i->second.empty()) {
                stubs_.erase(i);
            }
        }
        unused = becameUnused();
    }
    if (obj.is()) {
        binaryUno_.get()->pExtEnv->revokeInterface(
            binaryUno_.get()->pExtEnv, obj.get());
    }
    terminateWhenUnused(unused);
}

void Bridge::resurrectProxy(Proxy & proxy) {
    uno_Interface * p = &proxy;
    binaryUno_.get()->pExtEnv->registerProxyInterface(
        binaryUno_.get()->pExtEnv,
        reinterpret_cast< void ** >(&p), &freeProxyCallback,
        proxy.getOid().pData,
        reinterpret_cast< typelib_InterfaceTypeDescription * >(
            proxy.getType().get()));
    assert(p == &proxy);
}

void Bridge::revokeProxy(Proxy & proxy) {
    binaryUno_.get()->pExtEnv->revokeInterface(
        binaryUno_.get()->pExtEnv, &proxy);
}

void Bridge::freeProxy(Proxy & proxy) {
    try {
        makeReleaseCall(proxy.getOid(), proxy.getType());
    } catch (const css::uno::RuntimeException & e) {
        SAL_INFO(
            "binaryurp", "caught runtime exception '" << e << '\'');
    } catch (const std::exception & e) {
        SAL_WARN("binaryurp", "caught C++ exception '" << e.what() << '\'');
    }
    bool unused;
    {
        std::lock_guard g(mutex_);
        assert(proxies_ > 0);
        --proxies_;
        unused = becameUnused();
    }
    terminateWhenUnused(unused);
}

void Bridge::incrementCalls(bool normalCall) noexcept {
    std::lock_guard g(mutex_);
    assert(calls_ < std::numeric_limits< std::size_t >::max());
    ++calls_;
    normalCall_ |= normalCall;
}

void Bridge::decrementCalls() {
    bool unused;
    {
        std::lock_guard g(mutex_);
        assert(calls_ > 0);
        --calls_;
        unused = becameUnused();
    }
    terminateWhenUnused(unused);
}

void Bridge::incrementActiveCalls() noexcept {
    std::lock_guard g(mutex_);
    assert(
        activeCalls_ <= calls_ &&
        activeCalls_ < std::numeric_limits< std::size_t >::max());
    ++activeCalls_;
    passive_.reset();
}

void Bridge::decrementActiveCalls() noexcept {
    std::lock_guard g(mutex_);
    assert(activeCalls_ <= calls_ && activeCalls_ > 0);
    --activeCalls_;
    if (activeCalls_ == 0) {
        passive_.set();
    }
}

bool Bridge::makeCall(
    OUString const & oid, css::uno::TypeDescription const & member,
    bool setter, std::vector< BinaryAny >&& inArguments,
    BinaryAny * returnValue, std::vector< BinaryAny > * outArguments)
{
    std::unique_ptr< IncomingReply > resp;
    {
        uno_ThreadPool tp = getThreadPool();
        AttachThread att(tp);
        PopOutgoingRequest pop(
            outgoingRequests_, att.getTid(),
            OutgoingRequest(OutgoingRequest::KIND_NORMAL, member, setter));
        sendRequest(
            att.getTid(), oid, css::uno::TypeDescription(), member,
            std::move(inArguments));
        pop.clear();
        incrementCalls(true);
        incrementActiveCalls();
        void * job;
        uno_threadpool_enter(tp, &job);
        resp.reset(static_cast< IncomingReply * >(job));
        decrementActiveCalls();
        decrementCalls();
    }
    if (resp == nullptr)
    {
        throw css::lang::DisposedException(
            u"Binary URP bridge disposed during call"_ustr,
            getXWeak());
    }
    *returnValue = resp->returnValue;
    if (!resp->exception) {
        *outArguments = resp->outArguments;
    }
    return resp->exception;
}

void Bridge::sendRequestChangeRequest() {
    assert(mode_ == MODE_REQUESTED);
    random_ = random();
    std::vector< BinaryAny > a;
    a.emplace_back(
            css::uno::TypeDescription(cppu::UnoType< sal_Int32 >::get()),
            &random_);
    sendProtPropRequest(OutgoingRequest::KIND_REQUEST_CHANGE, a);
}

void Bridge::handleRequestChangeReply(
    bool exception, BinaryAny const & returnValue)
{
    try {
        throwException(exception, returnValue);
    } catch (css::uno::RuntimeException & e) {
        // Before OOo 2.2, Java URP would throw a RuntimeException when
        // receiving a requestChange message (see i#35277 "Java URP: Support
        // Manipulation of Protocol Properties"):
        if (mode_ != MODE_REQUESTED) {
            throw;
        }
        SAL_WARN(
            "binaryurp",
            "requestChange caught " << e << " in state 'requested'");
        mode_ = MODE_NORMAL;
        getWriter()->unblock();
        decrementCalls();
        return;
    }
    sal_Int32 n = *static_cast< sal_Int32 * >(
        returnValue.getValue(
            css::uno::TypeDescription(cppu::UnoType< sal_Int32 >::get())));
    sal_Int32 exp = 0;
    switch (mode_) {
    case MODE_REQUESTED:
    case MODE_REPLY_1:
        exp = 1;
        break;
    case MODE_REPLY_MINUS1:
        exp = -1;
        mode_ = MODE_REQUESTED;
        break;
    case MODE_REPLY_0:
        exp = 0;
        mode_ = MODE_WAIT;
        break;
    default:
        assert(false); // this cannot happen
        break;
    }
    if (n != exp) {
        throw css::uno::RuntimeException(
            u"URP: requestChange reply with unexpected return value received"_ustr,
            getXWeak());
    }
    decrementCalls();
    switch (exp) {
    case -1:
        sendRequestChangeRequest();
        break;
    case 0:
        break;
    case 1:
        sendCommitChangeRequest();
        break;
    default:
        assert(false); // this cannot happen
        break;
    }
}

void Bridge::handleCommitChangeReply(
    bool exception, BinaryAny const & returnValue)
{
    bool bCcMode = true;
    try {
        throwException(exception, returnValue);
    } catch (const css::bridge::InvalidProtocolChangeException &) {
        bCcMode = false;
    }
    if (bCcMode) {
        setCurrentContextMode();
    }
    assert(mode_ == MODE_REQUESTED || mode_ == MODE_REPLY_1);
    mode_ = MODE_NORMAL;
    getWriter()->unblock();
    decrementCalls();
}

void Bridge::handleRequestChangeRequest(
    rtl::ByteSequence const & tid, std::vector< BinaryAny > const & inArguments)
{
    assert(inArguments.size() == 1);
    switch (mode_) {
    case MODE_REQUESTED:
        {
            sal_Int32 n2 = *static_cast< sal_Int32 * >(
                inArguments[0].getValue(
                    css::uno::TypeDescription(
                        cppu::UnoType< sal_Int32 >::get())));
            sal_Int32 ret;
            if (n2 > random_) {
                ret = 1;
                mode_ = MODE_REPLY_0;
            } else if (n2 == random_) {
                ret = -1;
                mode_ = MODE_REPLY_MINUS1;
            } else {
                ret = 0;
                mode_ = MODE_REPLY_1;
            }
            getWriter()->sendDirectReply(
                tid, protPropRequest_, false,
                BinaryAny(
                    css::uno::TypeDescription(
                        cppu::UnoType< sal_Int32 >::get()),
                    &ret),
            std::vector< BinaryAny >());
            break;
        }
    case MODE_NORMAL:
        {
            mode_ = MODE_NORMAL_WAIT;
            sal_Int32 ret = 1;
            getWriter()->queueReply(
                tid, protPropRequest_, false, false,
                BinaryAny(
                    css::uno::TypeDescription(
                        cppu::UnoType< sal_Int32 >::get()),
                    &ret),
            std::vector< BinaryAny >(), false);
            break;
        }
    default:
        throw css::uno::RuntimeException(
            u"URP: unexpected requestChange request received"_ustr,
            getXWeak());
    }
}

void Bridge::handleCommitChangeRequest(
    rtl::ByteSequence const & tid, std::vector< BinaryAny > const & inArguments)
{
    bool bCcMode = false;
    bool bExc = false;
    BinaryAny ret;
    assert(inArguments.size() == 1);
    css::uno::Sequence< css::bridge::ProtocolProperty > s;
    [[maybe_unused]] bool ok = (mapBinaryToCppAny(inArguments[0]) >>= s);
    assert(ok);
    for (const auto & pp : s) {
        if (pp.Name == "CurrentContext") {
            bCcMode = true;
        } else {
            bCcMode = false;
            bExc = true;
            ret = mapCppToBinaryAny(
                css::uno::Any(
                    css::bridge::InvalidProtocolChangeException(
                        u"InvalidProtocolChangeException"_ustr,
                        css::uno::Reference< css::uno::XInterface >(), pp,
                        1)));
            break;
        }
    }
    switch (mode_) {
    case MODE_WAIT:
        getWriter()->sendDirectReply(
            tid, protPropCommit_, bExc, ret, std::vector< BinaryAny >());
        if (bCcMode) {
            setCurrentContextMode();
            mode_ = MODE_NORMAL;
            getWriter()->unblock();
        } else {
            mode_ = MODE_REQUESTED;
            sendRequestChangeRequest();
        }
        break;
    case MODE_NORMAL_WAIT:
        getWriter()->queueReply(
            tid, protPropCommit_, false, false, ret, std::vector< BinaryAny >(),
            bCcMode);
        mode_ = MODE_NORMAL;
        break;
    default:
        throw css::uno::RuntimeException(
            u"URP: unexpected commitChange request received"_ustr,
            getXWeak());
    }
}

OutgoingRequest Bridge::lastOutgoingRequest(rtl::ByteSequence const & tid) {
    OutgoingRequest req(outgoingRequests_.top(tid));
    outgoingRequests_.pop(tid);
    return req;
}

bool Bridge::isProtocolPropertiesRequest(
    std::u16string_view oid, css::uno::TypeDescription const & type) const
{
    return oid == protPropOid_ && type.equals(protPropType_);
}

void Bridge::setCurrentContextMode() {
    std::lock_guard g(mutex_);
    currentContextMode_ = true;
}

bool Bridge::isCurrentContextMode() {
    std::lock_guard g(mutex_);
    return currentContextMode_;
}

Bridge::~Bridge() {
#if OSL_DEBUG_LEVEL > 0
    {
        std::lock_guard g(mutex_);
        SAL_WARN_IF(
            state_ == STATE_STARTED || state_ == STATE_TERMINATED, "binaryurp",
            "undisposed bridge \"" << name_ <<"\" in state " << state_
                << ", potential deadlock ahead");
    }
#endif
    dispose();
}

css::uno::Reference< css::uno::XInterface > Bridge::getInstance(
    OUString const & sInstanceName)
{
    if (sInstanceName.isEmpty()) {
        throw css::uno::RuntimeException(
            u"XBridge::getInstance sInstanceName must be non-empty"_ustr,
            getXWeak());
    }
    for (sal_Int32 i = 0; i != sInstanceName.getLength(); ++i) {
        if (sInstanceName[i] > 0x7F) {
            throw css::uno::RuntimeException(
                u"XBridge::getInstance sInstanceName contains non-ASCII"
                " character"_ustr);
        }
    }
    css::uno::TypeDescription ifc(cppu::UnoType<css::uno::XInterface>::get());
    typelib_TypeDescription * p = ifc.get();
    std::vector< BinaryAny > inArgs;
    inArgs.emplace_back(
            css::uno::TypeDescription(cppu::UnoType< css::uno::Type >::get()),
            &p);
    BinaryAny ret;
    std::vector< BinaryAny> outArgs;
    bool bExc = makeCall(
        sInstanceName,
        css::uno::TypeDescription(
            u"com.sun.star.uno.XInterface::queryInterface"_ustr),
        false, std::move(inArgs), &ret, &outArgs);
    throwException(bExc, ret);
    auto const t = ret.getType();
    if (t.get()->eTypeClass == typelib_TypeClass_VOID) {
        return {};
    }
    if (!t.equals(ifc)) {
        throw css::uno::RuntimeException(
            "initial object queryInterface for OID \"" + sInstanceName + "\" returned ANY of type "
            + OUString::unacquired(&t.get()->pTypeName));
    }
    auto const val = *static_cast< uno_Interface ** >(ret.getValue(ifc));
    if (val == nullptr) {
        throw css::uno::RuntimeException(
            "initial object queryInterface for OID \"" + sInstanceName
            + "\" returned null css.uno.XInterface ANY");
    }
    return css::uno::Reference< css::uno::XInterface >(
        static_cast< css::uno::XInterface * >(
            binaryToCppMapping_.mapInterface(
                val,
                ifc.get())),
        SAL_NO_ACQUIRE);
}

OUString Bridge::getName() {
    return name_;
}

OUString Bridge::getDescription() {
    OUString b = name_ + ":" + connection_->getDescription();
    return b;
}

void Bridge::dispose() {
    // For terminate(true) not to deadlock, an external protocol must ensure
    // that dispose is not called from a thread pool worker thread (that dispose
    // is never called from the reader or writer thread is already ensured
    // internally):
    terminate(true);
    // OOo expects dispose to not return while there are still remote calls in
    // progress; an external protocol must ensure that dispose is not called
    // from within an incoming or outgoing remote call, as passive_.wait() would
    // otherwise deadlock:
    passive_.wait();
}

void Bridge::addEventListener(
    css::uno::Reference< css::lang::XEventListener > const & xListener)
{
    assert(xListener.is());
    {
        std::lock_guard g(mutex_);
        assert(state_ != STATE_INITIAL);
        if (state_ == STATE_STARTED) {
            listeners_.push_back(xListener);
            return;
        }
    }
    xListener->disposing(
        css::lang::EventObject(getXWeak()));
}

void Bridge::removeEventListener(
    css::uno::Reference< css::lang::XEventListener > const & aListener)
{
    std::lock_guard g(mutex_);
    Listeners::iterator i(
        std::find(listeners_.begin(), listeners_.end(), aListener));
    if (i != listeners_.end()) {
        listeners_.erase(i);
    }
}

void Bridge::sendCommitChangeRequest() {
    assert(mode_ == MODE_REQUESTED || mode_ == MODE_REPLY_1);
    css::uno::Sequence< css::bridge::ProtocolProperty > s(1);
    s.getArray()[0].Name = "CurrentContext";
    std::vector< BinaryAny > a { mapCppToBinaryAny(css::uno::Any(s)) };
    sendProtPropRequest(OutgoingRequest::KIND_COMMIT_CHANGE, a);
}

void Bridge::sendProtPropRequest(
    OutgoingRequest::Kind kind, std::vector< BinaryAny > const & inArguments)
{
    assert(
        kind == OutgoingRequest::KIND_REQUEST_CHANGE ||
        kind == OutgoingRequest::KIND_COMMIT_CHANGE);
    incrementCalls(false);
    css::uno::TypeDescription member(
        kind == OutgoingRequest::KIND_REQUEST_CHANGE
        ? protPropRequest_ : protPropCommit_);
    PopOutgoingRequest pop(
        outgoingRequests_, protPropTid_, OutgoingRequest(kind, member, false));
    getWriter()->sendDirectRequest(
        protPropTid_, protPropOid_, protPropType_, member, inArguments);
    pop.clear();
}

void Bridge::makeReleaseCall(
    OUString const & oid, css::uno::TypeDescription const & type)
{
    //HACK to decouple the processing of release calls from all other threads.  Normally, sending
    // the release request should use the current thread's TID (via AttachThread), which would cause
    // that asynchronous request to be processed by a physical thread that is paired with the
    // physical thread processing the normal synchronous call stack (see ThreadIdHashMap in
    // cppu/source/threadpool/threadpool.hxx).  However, that can lead to deadlock when a thread
    // illegally makes a synchronous UNO call with the SolarMutex locked (e.g.,
    // SfxBaseModel::postEvent_Impl in sfx2/source/doc/sfxbasemodel.cxx doing documentEventOccurred
    // and notifyEvent calls), and while that call is on the stack the remote side sends back some
    // release request on the same logical UNO thread for an object that wants to acquire the
    // SolarMutex in its destructor (e.g., SwXTextDocument in sw/inc/unotxdoc.hxx holding its
    // m_pImpl via an sw::UnoImplPtr).  While the correct approach would be to not make UNO calls
    // with the SolarMutex (or any other mutex) locked, fixing that would probably be a heroic
    // effort.  So for now live with this hack, hoping that it does not introduce any new issues of
    // its own:
    static auto const tid = [] {
            static sal_Int8 const id[] = {'r', 'e', 'l', 'e', 'a', 's', 'e', 'h', 'a', 'c', 'k'};
            return rtl::ByteSequence(id, std::size(id));
        }();
    sendRequest(
        tid, oid, type,
        css::uno::TypeDescription(u"com.sun.star.uno.XInterface::release"_ustr),
        std::vector< BinaryAny >());
}

void Bridge::sendRequest(
    rtl::ByteSequence const & tid, OUString const & oid,
    css::uno::TypeDescription const & type,
    css::uno::TypeDescription const & member,
    std::vector< BinaryAny >&& inArguments)
{
    getWriter()->queueRequest(tid, oid, type, member, std::move(inArguments));
}

void Bridge::throwException(bool exception, BinaryAny const & value) {
    if (exception) {
        cppu::throwException(mapBinaryToCppAny(value));
    }
}

css::uno::Any Bridge::mapBinaryToCppAny(BinaryAny const & binaryAny) {
    BinaryAny in(binaryAny);
    css::uno::Any out;
    out.~Any();
    uno_copyAndConvertData(
        &out, &in.get(),
        css::uno::TypeDescription(cppu::UnoType< css::uno::Any >::get()).get(),
        binaryToCppMapping_.get());
    return out;
}

bool Bridge::becameUnused() const {
    return stubs_.empty() && proxies_ == 0 && calls_ == 0 && normalCall_;
}

void Bridge::terminateWhenUnused(bool unused) {
    if (unused) {
        // That the current thread considers the bridge unused implies that it
        // is not within an incoming or outgoing remote call (so calling
        // terminate cannot lead to deadlock):
        terminate(false);
    }
}

void Bridge::checkDisposed() {
    assert(state_ != STATE_INITIAL);
    if (state_ != STATE_STARTED) {
        throw css::lang::DisposedException(
            u"Binary URP bridge already disposed"_ustr,
            getXWeak());
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
