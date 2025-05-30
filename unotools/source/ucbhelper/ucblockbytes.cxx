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

#include "ucblockbytes.hxx"

#include <sal/log.hxx>
#include <comphelper/processfactory.hxx>
#include <salhelper/condition.hxx>
#include <osl/thread.hxx>
#include <osl/diagnose.h>
#include <tools/urlobj.hxx>
#include <tools/solar.h>
#include <ucbhelper/interactionrequest.hxx>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/task/XInteractionAbort.hpp>
#include <com/sun/star/ucb/InteractiveNetworkConnectException.hpp>
#include <com/sun/star/ucb/CommandFailedException.hpp>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <com/sun/star/ucb/CommandAbortedException.hpp>
#include <com/sun/star/ucb/UnsupportedDataSinkException.hpp>
#include <com/sun/star/ucb/InteractiveIOException.hpp>
#include <com/sun/star/ucb/XContentIdentifier.hpp>
#include <com/sun/star/ucb/XContent.hpp>
#include <com/sun/star/io/IOException.hpp>
#include <com/sun/star/io/XActiveDataStreamer.hpp>
#include <com/sun/star/io/TempFile.hpp>
#include <com/sun/star/ucb/XCommandProcessor.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/ucb/OpenCommandArgument2.hpp>
#include <com/sun/star/ucb/PostCommandArgument2.hpp>
#include <com/sun/star/ucb/OpenMode.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertiesChangeNotifier.hpp>
#include <com/sun/star/beans/XPropertiesChangeListener.hpp>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XActiveDataControl.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <cppuhelper/implbase.hxx>
#include <tools/debug.hxx>
#include <com/sun/star/io/XTruncate.hpp>
#include <com/sun/star/lang/IllegalArgumentException.hpp>

#include <comphelper/bytereader.hxx>
#include <comphelper/storagehelper.hxx>
#include <ucbhelper/content.hxx>
#include <unotools/tempfile.hxx>
#include <mutex>
#include <utility>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;

namespace utl
{

namespace {

/**
    Helper class for getting a XInputStream when opening a content
 */
class UcbDataSink_Impl : public ::cppu::WeakImplHelper< XActiveDataControl, XActiveDataSink >
{
    UcbLockBytesRef         m_xLockBytes;

public:
    explicit UcbDataSink_Impl( UcbLockBytes* pLockBytes )
        : m_xLockBytes( pLockBytes )
    {}

    // XActiveDataControl.
    virtual void SAL_CALL   addListener ( const Reference<XStreamListener> &/*rxListener*/) override {}
    virtual void SAL_CALL   removeListener ( const Reference<XStreamListener> &/*rxListener*/) override {}
    virtual void SAL_CALL   start() override {}
    virtual void SAL_CALL   terminate() override
                            { m_xLockBytes->terminate(); }

    // XActiveDataSink.
    virtual void SAL_CALL   setInputStream ( const Reference<XInputStream> &rxInputStream) override
                            { m_xLockBytes->setInputStream(rxInputStream); }
    virtual Reference<XInputStream> SAL_CALL getInputStream() override
                            { return m_xLockBytes->getInputStream(); }
};

/**
    Helper class for getting a XStream when opening a content
 */
class UcbStreamer_Impl : public ::cppu::WeakImplHelper< XActiveDataStreamer, XActiveDataControl >
{
    Reference < XStream >   m_xStream;
    UcbLockBytesRef         m_xLockBytes;

public:
    explicit UcbStreamer_Impl( UcbLockBytes* pLockBytes )
        : m_xLockBytes( pLockBytes )
    {}

    // XActiveDataControl.
    virtual void SAL_CALL   addListener ( const Reference<XStreamListener> &/*rxListener*/) override {}
    virtual void SAL_CALL   removeListener ( const Reference<XStreamListener> &/*rxListener*/) override {}
    virtual void SAL_CALL   start() override {}
    virtual void SAL_CALL   terminate() override
                            { m_xLockBytes->terminate(); }

    // XActiveDataStreamer
    virtual void SAL_CALL   setStream( const Reference< XStream >& aStream ) override
                            { m_xStream = aStream; m_xLockBytes->setStream( aStream ); }
    virtual Reference< XStream > SAL_CALL getStream() override
                            { return m_xStream; }
};

/**
    Helper class for managing interactions and progress when executing UCB commands
 */
class UcbTaskEnvironment : public ::cppu::WeakImplHelper< XCommandEnvironment >
{
    Reference< XInteractionHandler >                m_xInteractionHandler;
    Reference< XProgressHandler >                   m_xProgressHandler;

public:
                            UcbTaskEnvironment( const Reference< XInteractionHandler>& rxInteractionHandler,
                                                const Reference< XProgressHandler>& rxProgressHandler )
                                : m_xInteractionHandler( rxInteractionHandler )
                                , m_xProgressHandler( rxProgressHandler )
                            {}

    virtual Reference<XInteractionHandler> SAL_CALL getInteractionHandler() override
    { return m_xInteractionHandler; }

    virtual Reference<XProgressHandler> SAL_CALL    getProgressHandler() override
    { return m_xProgressHandler; }
};

/**
    Helper class for property change notifies when executing UCB commands
*/
class UcbPropertiesChangeListener_Impl : public ::cppu::WeakImplHelper< XPropertiesChangeListener >
{
public:
    UcbLockBytesRef         m_xLockBytes;

    explicit UcbPropertiesChangeListener_Impl( UcbLockBytesRef xRef )
        : m_xLockBytes(std::move( xRef ))
    {}

    virtual void SAL_CALL   disposing ( const EventObject &/*rEvent*/) override {}
    virtual void SAL_CALL   propertiesChange ( const Sequence<PropertyChangeEvent> &rEvent) override;
};

}

void SAL_CALL UcbPropertiesChangeListener_Impl::propertiesChange ( const Sequence<PropertyChangeEvent> &rEvent)
{
    for (const auto& rPropChangeEvent : rEvent)
    {
        if (rPropChangeEvent.PropertyName == "DocumentHeader")
        {
            m_xLockBytes->SetStreamValid();
        }
    }
}

namespace {

class Moderator
    : public osl::Thread
{
    // usage restriction:
    // It might be possible, that the call to the interactionhandler and/or
    // progresshandler is done asynchronously, while the 'execute' simply
    // returns. This would imply that these class must be refcounted!!!

public:
    /// @throws ContentCreationException
    /// @throws RuntimeException
    Moderator(
        Reference < XContent > const & xContent,
        Reference < XInteractionHandler > const & xInteract,
        Command aArg
    );

    enum class ResultType {
        NORESULT,

        INTERACTIONREQUEST,    // reply expected

        INPUTSTREAM,
        STREAM,

        RESULT,
        TIMEDOUT,
        COMMANDABORTED,
        COMMANDFAILED,
        INTERACTIVEIO,
        UNSUPPORTED,
        GENERAL
    };

    class ConditionRes
        : public salhelper::Condition
    {
    public:
        ConditionRes(osl::Mutex& aMutex,Moderator& aModerator)
            : salhelper::Condition(aMutex),
              m_aModerator(aModerator)
        {
        }

    protected:
        bool applies() const override {
            return m_aModerator.m_aResultType != ResultType::NORESULT;
        }

    private:
        Moderator& m_aModerator;
    };

    struct Result {
        ResultType        type;
        Any               result;
        IOErrorCode       ioErrorCode;
    };

    Result getResult(const sal_uInt32 milliSec);

    enum ReplyType {
        NOREPLY,
        EXIT,
        REQUESTHANDLED
    };

    class ConditionRep
        : public salhelper::Condition
    {
    public:
        ConditionRep(osl::Mutex& aMutex,Moderator& aModerator)
            : salhelper::Condition(aMutex),
              m_aModerator(aModerator)
        {
        }

    protected:
        bool applies() const override {
            return m_aModerator.m_aReplyType != NOREPLY;
        }

    private:
        Moderator& m_aModerator;
    };

    void setReply(ReplyType);

    void handle( const Reference<XInteractionRequest >& Request );

    void setStream(const Reference< XStream >& aStream);
    void setInputStream(const Reference<XInputStream> &rxInputStream);

protected:
    virtual void SAL_CALL run() override;
    virtual void SAL_CALL onTerminated() override;

private:
    osl::Mutex        m_aMutex;

    friend class ConditionRes;

    ConditionRes      m_aRes;
    ResultType        m_aResultType;
    IOErrorCode       m_nIOErrorCode;
    Any               m_aResult;

    friend class ConditionRep;

    ConditionRep      m_aRep;
    ReplyType         m_aReplyType;

    Command                           m_aArg;
    ::ucbhelper::Content              m_aContent;
};

class ModeratorsActiveDataStreamer
    : public ::cppu::WeakImplHelper<XActiveDataStreamer>
{
public:

    explicit ModeratorsActiveDataStreamer(Moderator &theModerator);

    // XActiveDataStreamer
    virtual void SAL_CALL
    setStream(
        const Reference< XStream >& aStream
    ) override;

    virtual Reference<XStream> SAL_CALL getStream () override
    {
        std::scoped_lock aGuard(m_aMutex);
        return m_xStream;
    }

private:
    Moderator& m_aModerator;

    std::mutex m_aMutex;
    Reference<XStream> m_xStream;
};

class ModeratorsActiveDataSink
    : public ::cppu::WeakImplHelper<XActiveDataSink>
{
public:

    explicit ModeratorsActiveDataSink(Moderator &theModerator);

    // XActiveDataSink.
    virtual void SAL_CALL
    setInputStream (
        const Reference<XInputStream> &rxInputStream
    ) override;

    virtual Reference<XInputStream> SAL_CALL getInputStream() override
    {
        std::scoped_lock aGuard(m_aMutex);
        return m_xStream;
    }

private:
    Moderator& m_aModerator;
    std::mutex m_aMutex;
    Reference<XInputStream> m_xStream;
};

}

ModeratorsActiveDataSink::ModeratorsActiveDataSink(Moderator &theModerator)
    : m_aModerator(theModerator)
{
}

// XActiveDataSink.
void SAL_CALL
ModeratorsActiveDataSink::setInputStream (
    const Reference<XInputStream> &rxInputStream
)
{
    m_aModerator.setInputStream(rxInputStream);
    std::scoped_lock aGuard(m_aMutex);
    m_xStream = rxInputStream;
}

ModeratorsActiveDataStreamer::ModeratorsActiveDataStreamer(
    Moderator &theModerator
)
    : m_aModerator(theModerator)
{
}

// XActiveDataStreamer.
void SAL_CALL
ModeratorsActiveDataStreamer::setStream (
    const Reference<XStream> &rxStream
)
{
    m_aModerator.setStream(rxStream);
    std::scoped_lock aGuard(m_aMutex);
    m_xStream = rxStream;
}

namespace {

class ModeratorsInteractionHandler
    : public ::cppu::WeakImplHelper<XInteractionHandler>
{
public:

    explicit ModeratorsInteractionHandler(Moderator &theModerator);

    virtual void SAL_CALL
    handle( const Reference<XInteractionRequest >& Request ) override;

private:

    Moderator& m_aModerator;
};

}

ModeratorsInteractionHandler::ModeratorsInteractionHandler(
    Moderator &aModerator)
    : m_aModerator(aModerator)
{
}

void SAL_CALL
ModeratorsInteractionHandler::handle(
    const Reference<XInteractionRequest >& Request
)
{
    // wakes up the mainthread
    m_aModerator.handle(Request);
}

Moderator::Moderator(
    Reference < XContent > const & xContent,
    Reference < XInteractionHandler > const & xInteract,
    Command aArg
)
    : m_aRes(m_aMutex,*this),
      m_aResultType(ResultType::NORESULT),
      m_nIOErrorCode(IOErrorCode_ABORT),
      m_aRep(m_aMutex,*this),
      m_aReplyType(NOREPLY),
      m_aArg(std::move(aArg)),
      m_aContent(
          xContent,
          new UcbTaskEnvironment(
              xInteract.is() ? new ModeratorsInteractionHandler(*this) : nullptr,
              nullptr),
          comphelper::getProcessComponentContext())
{
    // now exchange the whole data sink stuff
    // with a thread safe version

    Reference<XInterface> *pxSink = nullptr;

    PostCommandArgument2 aPostArg;
    OpenCommandArgument2 aOpenArg;

    int dec(2);
    if(m_aArg.Argument >>= aPostArg) {
        pxSink = &aPostArg.Sink;
        dec = 0;
    }
    else if(m_aArg.Argument >>= aOpenArg) {
        pxSink = &aOpenArg.Sink;
        dec = 1;
    }

    if(dec ==2)
        throw ContentCreationException();

    Reference < XActiveDataSink > xActiveSink(*pxSink,UNO_QUERY);
    if(xActiveSink.is())
        pxSink->set(getXWeak(new ModeratorsActiveDataSink(*this)));

    Reference<XActiveDataStreamer> xStreamer( *pxSink, UNO_QUERY );
    if ( xStreamer.is() )
        pxSink->set(getXWeak(new ModeratorsActiveDataStreamer(*this)));

    if(dec == 0)
        m_aArg.Argument <<= aPostArg;
    else if(dec == 1)
        m_aArg.Argument <<= aOpenArg;
}

Moderator::Result Moderator::getResult(const sal_uInt32 milliSec)
{
    Result ret;
    try {
        salhelper::ConditionWaiter aWaiter(m_aRes,milliSec);
        ret.type = m_aResultType;
        ret.result = m_aResult;
        ret.ioErrorCode = m_nIOErrorCode;

        // reset
        m_aResultType = ResultType::NORESULT;
    }
    catch (const salhelper::ConditionWaiter::timedout&)
    {
        ret.type = ResultType::TIMEDOUT;
    }

    return ret;
}

void Moderator::setReply(ReplyType aReplyType )
{
    salhelper::ConditionModifier aMod(m_aRep);
    m_aReplyType = aReplyType;
}

void Moderator::handle( const Reference<XInteractionRequest >& Request )
{
    ReplyType aReplyType;

    do {
        {
            salhelper::ConditionModifier aMod(m_aRes);
            m_aResultType = ResultType::INTERACTIONREQUEST;
            m_aResult <<= Request;
        }

        {
            salhelper::ConditionWaiter aWait(m_aRep);
            aReplyType = m_aReplyType;

            // reset
            m_aReplyType = NOREPLY;
        }

        if(aReplyType == EXIT) {
            const Sequence<Reference<XInteractionContinuation> > aSeq(
                Request->getContinuations());
            for(const auto& rContinuation : aSeq) {
                Reference<XInteractionAbort> aRef(rContinuation,UNO_QUERY);
                if(aRef.is()) {
                    aRef->select();
                }
            }

            // resignal the exit condition
            setReply(EXIT);
            break;
        }
    } while(aReplyType != REQUESTHANDLED);
}

void Moderator::setStream(const Reference< XStream >& aStream)
{
    {
        salhelper::ConditionModifier aMod(m_aRes);
        m_aResultType = ResultType::STREAM;
        m_aResult <<= aStream;
    }
    ReplyType aReplyType;
    {
        salhelper::ConditionWaiter aWait(m_aRep);
        aReplyType = m_aReplyType;
        m_aReplyType = NOREPLY;
    }
    if(aReplyType == EXIT)
        setReply(EXIT);
}

void Moderator::setInputStream(const Reference<XInputStream> &rxInputStream)
{
    {
        salhelper::ConditionModifier aMod(m_aRes);
        m_aResultType = ResultType::INPUTSTREAM;
        m_aResult <<= rxInputStream;
    }
    ReplyType aReplyType;
    {
        salhelper::ConditionWaiter aWait(m_aRep);
        aReplyType = m_aReplyType;
        m_aReplyType = NOREPLY;
    }
    if(aReplyType == EXIT)
        setReply(EXIT);
}

void SAL_CALL Moderator::run()
{
    osl_setThreadName("utl::Moderator");

    ResultType  aResultType;
    Any         aResult;
    IOErrorCode nIOErrorCode = IOErrorCode_ABORT;

    try
    {
        aResult = m_aContent.executeCommand(m_aArg.Name,m_aArg.Argument);
        aResultType = ResultType::RESULT;
    }
    catch (const CommandAbortedException&)
    {
        aResultType = ResultType::COMMANDABORTED;
    }
    catch (const CommandFailedException&)
    {
        aResultType = ResultType::COMMANDFAILED;
    }
    catch (const InteractiveIOException& r)
    {
        nIOErrorCode = r.Code;
        aResultType = ResultType::INTERACTIVEIO;
    }
    catch (const UnsupportedDataSinkException &)
    {
        aResultType = ResultType::UNSUPPORTED;
    }
    catch (const Exception&)
    {
        aResultType = ResultType::GENERAL;
    }

    {
        salhelper::ConditionModifier aMod(m_aRes);
        m_aResultType = aResultType;
        m_aResult = std::move(aResult);
        m_nIOErrorCode = nIOErrorCode;
    }
}

void SAL_CALL Moderator::onTerminated()
{
    {
        salhelper::ConditionWaiter aWaiter(m_aRep);
    }
    delete this;
}

/**
   Function for opening UCB contents synchronously,
   but with handled timeout;
*/
static bool UCBOpenContentSync_(
    const UcbLockBytesRef& xLockBytes,
    const Reference < XContent >& xContent,
    const Command& rArg,
    const Reference < XInterface >& xSink,
    const Reference < XInteractionHandler >& xInteract );

static bool UCBOpenContentSync(
    const UcbLockBytesRef& xLockBytes,
    Reference < XContent > const & xContent,
    const Command& rArg,
    const Reference < XInterface >& xSink,
    Reference < XInteractionHandler > const & xInteract )
{
    // http protocol must be handled in a special way:
    //        during the opening process the input stream may change
    //        only the last inputstream after notifying the document
    //        headers is valid

    Reference<XContentIdentifier> xContId(
        xContent.is() ? xContent->getIdentifier() : nullptr );

    OUString aScheme;
    if(xContId.is())
        aScheme = xContId->getContentProviderScheme();

    // now determine whether we use a timeout or not;
    if( ! aScheme.equalsIgnoreAsciiCase("http")                &&
        ! aScheme.equalsIgnoreAsciiCase("https")               &&
        ! aScheme.equalsIgnoreAsciiCase("vnd.sun.star.webdav") &&
        ! aScheme.equalsIgnoreAsciiCase("vnd.sun.star.webdavs") &&
        ! aScheme.equalsIgnoreAsciiCase("ftp"))
        return UCBOpenContentSync_(
            xLockBytes,xContent,rArg,xSink,xInteract);

    if ( !aScheme.equalsIgnoreAsciiCase( "http" ) &&
         !aScheme.equalsIgnoreAsciiCase( "https" ) )
        xLockBytes->SetStreamValid();

    rtl::Reference< UcbPropertiesChangeListener_Impl > xListener;
    Reference< XPropertiesChangeNotifier > xProps(xContent,UNO_QUERY);
    if(xProps.is()) {
        xListener =
            new UcbPropertiesChangeListener_Impl(xLockBytes);
        xProps->addPropertiesChangeListener(
            Sequence< OUString >(),
            xListener);
    }

    bool bException(false);
    bool bAborted(false);
    bool bResultAchieved(false);

    Moderator* pMod = nullptr;
    try
    {
        pMod = new Moderator(xContent,xInteract,rArg);
        pMod->create();
            //TODO: a protocol is missing how to join with the launched thread before exit(3), to
            // ensure the thread is no longer relying on any infrastructure while that
            // infrastructure is being shut down in atexit handlers
    }
    catch (const ContentCreationException&)
    {
        bResultAchieved = bException = true;
        xLockBytes->SetError( ERRCODE_IO_GENERAL );
    }

    sal_uInt32 nTimeout(5000); // initially 5000 milliSec
    while(!bResultAchieved) {

        // try to get the result for with timeout
        Moderator::Result res = pMod->getResult(nTimeout);

        switch(res.type) {
        case Moderator::ResultType::STREAM:
            {
                Reference<XStream> result;
                if(res.result >>= result) {
                    Reference < XActiveDataStreamer > xStreamer(
                        xSink, UNO_QUERY
                    );

                    if(xStreamer.is())
                        xStreamer->setStream(result);
                }
                pMod->setReply(Moderator::REQUESTHANDLED);
                break;
            }
        case Moderator::ResultType::INPUTSTREAM:
            {
                Reference<XInputStream> result;
                res.result >>= result;
                Reference < XActiveDataSink > xActiveSink(
                    xSink, UNO_QUERY
                );

                if(xActiveSink.is())
                    xActiveSink->setInputStream(result);
                pMod->setReply(Moderator::REQUESTHANDLED);
                break;
            }
        case Moderator::ResultType::TIMEDOUT:
            {
                Reference<XInteractionRetry> xRet;
                if(xInteract.is()) {
                    INetURLObject aURL(
                        xContId.is() ?
                        xContId->getContentIdentifier() :
                        OUString() );
                    InteractiveNetworkConnectException aExcep(
                        u"server not responding after five seconds"_ustr, {},
                        InteractionClassification_ERROR, aURL.GetHost());
                    Any request;
                    request <<= aExcep;
                    rtl::Reference<ucbhelper::InteractionRequest> xIR =
                        new ucbhelper::InteractionRequest(request);
                    rtl::Reference<ucbhelper::InteractionRetry> retryP =
                        new ucbhelper::InteractionRetry(xIR.get());
                    rtl::Reference<ucbhelper::InteractionAbort> abortP =
                        new ucbhelper::InteractionAbort(xIR.get());
                    Sequence<Reference<XInteractionContinuation> > aSeq { retryP, abortP };

                    xIR->setContinuations(aSeq);
                    xInteract->handle(xIR);
                    rtl::Reference< ucbhelper::InteractionContinuation > ref
                        = xIR->getSelection();
                    if(ref.is()) {
                        xRet.set(ref->getXWeak(), UNO_QUERY);
                    }
                }

                if(!xRet.is()) {
                    bAborted = true;
                    xLockBytes->SetError(ERRCODE_ABORT);
                }

                break;
            }
        case Moderator::ResultType::INTERACTIONREQUEST:
            {
                Reference<XInteractionRequest> Request;
                res.result >>= Request;
                xInteract->handle(Request);
                pMod->setReply(Moderator::REQUESTHANDLED);
                break;
            }
        case Moderator::ResultType::RESULT:
            {
                bResultAchieved = true;
                break;
            }
        case Moderator::ResultType::COMMANDABORTED:
        case Moderator::ResultType::COMMANDFAILED:
            {
                bAborted = true;
                xLockBytes->SetError( ERRCODE_ABORT );
                break;
            }
        case Moderator::ResultType::INTERACTIVEIO:
            {
                bException = true;
                if ( res.ioErrorCode == IOErrorCode_ACCESS_DENIED ||
                     res.ioErrorCode == IOErrorCode_LOCKING_VIOLATION )
                    xLockBytes->SetError( ERRCODE_IO_ACCESSDENIED );
                else if ( res.ioErrorCode == IOErrorCode_NOT_EXISTING )
                    xLockBytes->SetError( ERRCODE_IO_NOTEXISTS );
                else if ( res.ioErrorCode == IOErrorCode_CANT_READ )
                    xLockBytes->SetError( ERRCODE_IO_CANTREAD );
                else
                    xLockBytes->SetError( ERRCODE_IO_GENERAL );
                break;
            }
        case Moderator::ResultType::UNSUPPORTED:
            {
                bException = true;
                xLockBytes->SetError( ERRCODE_IO_NOTSUPPORTED );
                break;
            }
        default:
            {
                bException = true;
                xLockBytes->SetError( ERRCODE_IO_GENERAL );
                break;
            }
        }

        bResultAchieved |= bException;
        bResultAchieved |= bAborted;
        if(nTimeout == 5000) nTimeout *= 2;
    }

    if(pMod) pMod->setReply(Moderator::EXIT);

    if ( bAborted || bException )
    {
        Reference < XActiveDataSink > xActiveSink( xSink, UNO_QUERY );
        if ( xActiveSink.is() )
            xActiveSink->setInputStream( Reference < XInputStream >() );

        Reference < XActiveDataStreamer > xStreamer( xSink, UNO_QUERY );
        if ( xStreamer.is() )
            xStreamer->setStream( Reference < XStream >() );
    }

    Reference < XActiveDataControl > xControl( xSink, UNO_QUERY );
    if ( xControl.is() )
        xControl->terminate();

    if ( xProps.is() )
        xProps->removePropertiesChangeListener(
            Sequence< OUString >(),
            xListener );

    return ( bAborted || bException );
}

/**
    Function for opening UCB contents synchronously
 */
static bool UCBOpenContentSync_(
    const UcbLockBytesRef& xLockBytes,
    const Reference < XContent >& xContent,
    const Command& rArg,
    const Reference < XInterface >& xSink,
    const Reference < XInteractionHandler >& xInteract )
{
    ::ucbhelper::Content aContent(
        xContent, new UcbTaskEnvironment( xInteract, nullptr ),
        comphelper::getProcessComponentContext() );
    Reference < XContentIdentifier > xIdent = xContent->getIdentifier();
    OUString aScheme = xIdent->getContentProviderScheme();

    // http protocol must be handled in a special way: during the opening process the input stream may change
    // only the last inputstream after notifying the document headers is valid
    if ( !aScheme.equalsIgnoreAsciiCase("http") )
        xLockBytes->SetStreamValid();

    Reference< XPropertiesChangeListener > xListener = new UcbPropertiesChangeListener_Impl( xLockBytes );
    Reference< XPropertiesChangeNotifier > xProps ( xContent, UNO_QUERY );
    if ( xProps.is() )
        xProps->addPropertiesChangeListener( Sequence< OUString >(), xListener );

    bool bException = false;
    bool bAborted = false;

    try
    {
        aContent.executeCommand( rArg.Name, rArg.Argument );
    }
    catch (const CommandAbortedException&)
    {
        bAborted = true;
        xLockBytes->SetError( ERRCODE_ABORT );
    }
    catch (const CommandFailedException&)
    {
        bAborted = true;
        xLockBytes->SetError( ERRCODE_ABORT );
    }
    catch (const InteractiveIOException& r)
    {
        bException = true;
        if ( r.Code == IOErrorCode_ACCESS_DENIED || r.Code == IOErrorCode_LOCKING_VIOLATION )
            xLockBytes->SetError( ERRCODE_IO_ACCESSDENIED );
        else if ( r.Code == IOErrorCode_NOT_EXISTING )
            xLockBytes->SetError( ERRCODE_IO_NOTEXISTS );
        else if ( r.Code == IOErrorCode_CANT_READ )
            xLockBytes->SetError( ERRCODE_IO_CANTREAD );
        else
            xLockBytes->SetError( ERRCODE_IO_GENERAL );
    }
    catch (const UnsupportedDataSinkException&)
    {
        bException = true;
        xLockBytes->SetError( ERRCODE_IO_NOTSUPPORTED );
    }
    catch (const Exception&)
    {
        bException = true;
        xLockBytes->SetError( ERRCODE_IO_GENERAL );
    }

    if ( bAborted || bException )
    {
        Reference < XActiveDataSink > xActiveSink( xSink, UNO_QUERY );
        if ( xActiveSink.is() )
            xActiveSink->setInputStream( Reference < XInputStream >() );

        Reference < XActiveDataStreamer > xStreamer( xSink, UNO_QUERY );
        if ( xStreamer.is() )
            xStreamer->setStream( Reference < XStream >() );
    }

    Reference < XActiveDataControl > xControl( xSink, UNO_QUERY );
    if ( xControl.is() )
        xControl->terminate();

    if ( xProps.is() )
        xProps->removePropertiesChangeListener( Sequence< OUString >(), xListener );

    return ( bAborted || bException );
}

UcbLockBytes::UcbLockBytes()
    : m_nError( ERRCODE_NONE )
    , m_bTerminated  (false)
    , m_bDontClose( false )
    , m_bStreamValid  (false)
{
    SetSynchronMode();
}

UcbLockBytes::~UcbLockBytes()
{
    if ( !m_bDontClose )
    {
        if ( m_xInputStream.is() )
        {
            try
            {
                m_xInputStream->closeInput();
            }
            catch (const RuntimeException&)
            {
            }
            catch (const IOException&)
            {
            }
        }
    }

    if ( m_xInputStream.is() || !m_xOutputStream.is() )
        return;

    try
    {
        m_xOutputStream->closeOutput();
    }
    catch (const RuntimeException&)
    {
    }
    catch (const IOException&)
    {
    }
}

Reference < XInputStream > UcbLockBytes::getInputStream()
{
    std::unique_lock aGuard( m_aMutex );
    m_bDontClose = true;
    return m_xInputStream;
}

void UcbLockBytes::setStream( const Reference<XStream>& aStream )
{
    std::unique_lock aGuard( m_aMutex );
    if ( aStream.is() )
    {
        m_xOutputStream = aStream->getOutputStream();
        setInputStreamImpl( aGuard, aStream->getInputStream(), false );
        m_xSeekable.set( aStream, UNO_QUERY );
    }
    else
    {
        m_xOutputStream.clear();
        setInputStreamImpl( aGuard, Reference < XInputStream >() );
    }
}

bool UcbLockBytes::setInputStream( const Reference<XInputStream> &rxInputStream )
{
    std::unique_lock aGuard( m_aMutex );
    return setInputStreamImpl(aGuard, rxInputStream, /*bSetXSeekable*/true);
}

bool UcbLockBytes::setInputStreamImpl( std::unique_lock<std::mutex>& /*rGuard*/, const Reference<XInputStream> &rxInputStream, bool bSetXSeekable )
{
    bool bRet = false;

    try
    {
        if ( !m_bDontClose && m_xInputStream.is() )
            m_xInputStream->closeInput();

        m_xInputStream = rxInputStream;

        if( bSetXSeekable )
        {
            m_xSeekable.set( rxInputStream, UNO_QUERY );
            if( !m_xSeekable.is() && rxInputStream.is() )
            {
                rtl::Reference< utl::TempFileFastService > rxTempOut( new utl::TempFileFastService );
                ::comphelper::OStorageHelper::CopyInputToOutput( rxInputStream, rxTempOut );
                m_xInputStream.set( rxTempOut );
                m_xSeekable.set( rxTempOut );
            }
        }

        bRet = m_xInputStream.is();
    }
    catch (const Exception&)
    {
    }

    if ( m_bStreamValid && m_xInputStream.is() )
        m_aInitialized.set();

    return bRet;
}

void UcbLockBytes::SetStreamValid()
{
    m_bStreamValid = true;
    if ( m_xInputStream.is() )
        m_aInitialized.set();
}

void UcbLockBytes::terminate()
{
    m_bTerminated = true;
    m_aInitialized.set();
    m_aTerminated.set();

    if ( GetError() == ERRCODE_NONE && !m_xInputStream.is() )
    {
        OSL_FAIL("No InputStream, but no error set!" );
        SetError( ERRCODE_IO_NOTEXISTS );
    }
}

ErrCode UcbLockBytes::ReadAt(sal_uInt64 const nPos,
        void *pBuffer, std::size_t nCount, std::size_t *pRead) const
{
    if ( IsSynchronMode() )
    {
        UcbLockBytes* pThis = const_cast < UcbLockBytes* >( this );
        pThis->m_aInitialized.wait();
    }

    Reference <XInputStream> xStream = getInputStream();
    if ( !xStream.is() )
    {
        if ( m_bTerminated )
            return ERRCODE_IO_CANTREAD;
        else
            return ERRCODE_IO_PENDING;
    }

    if ( pRead )
        *pRead = 0;

    Reference <XSeekable> xSeekable = getSeekable();
    if ( !xSeekable.is() )
        return ERRCODE_IO_CANTREAD;

    try
    {
        xSeekable->seek( nPos );
    }
    catch (const IOException&)
    {
        return ERRCODE_IO_CANTSEEK;
    }
    catch (const css::lang::IllegalArgumentException&)
    {
        return ERRCODE_IO_CANTSEEK;
    }

    sal_Int32          nSize;

    if(nCount > 0x7FFFFFFF)
    {
        nCount = 0x7FFFFFFF;
    }
    try
    {
        if ( !m_bTerminated && !IsSynchronMode() )
        {
            sal_uInt64 nLen = xSeekable->getLength();
            if ( nPos + nCount > nLen )
                return ERRCODE_IO_PENDING;
        }

        comphelper::ByteReader* pByteReader = dynamic_cast< comphelper::ByteReader* >(xStream.get());
        if (pByteReader)
        {
            nSize = pByteReader->readSomeBytes( static_cast<sal_Int8*>(pBuffer), sal_Int32(nCount) );
        }
        else
        {
            Sequence<sal_Int8> aData;
            nSize = xStream->readBytes( aData, sal_Int32(nCount) );
            memcpy (pBuffer, aData.getConstArray(), nSize);
        }
    }
    catch (const IOException&)
    {
        return ERRCODE_IO_CANTREAD;
    }

    if (pRead)
        *pRead = static_cast<std::size_t>(nSize);

    return ERRCODE_NONE;
}

ErrCode UcbLockBytes::WriteAt(sal_uInt64 const nPos, const void *pBuffer,
        std::size_t nCount, std::size_t *pWritten)
{
    if ( pWritten )
        *pWritten = 0;

    DBG_ASSERT( IsSynchronMode(), "Writing is only possible in SynchronMode!" );
    DBG_ASSERT( m_aInitialized.check(), "Writing bevor stream is ready!" );

    Reference <XSeekable> xSeekable = getSeekable();
    Reference <XOutputStream> xOutputStream = getOutputStream();
    if ( !xOutputStream.is() || !xSeekable.is() )
        return ERRCODE_IO_CANTWRITE;

    try
    {
        xSeekable->seek( nPos );
    }
    catch (const IOException&)
    {
        return ERRCODE_IO_CANTSEEK;
    }

    sal_Int8 const * pData = static_cast<sal_Int8 const *>(pBuffer);
    Sequence<sal_Int8> aData( pData, nCount );
    try
    {
        xOutputStream->writeBytes( aData );
        if ( pWritten )
            *pWritten = nCount;
    }
    catch (const Exception&)
    {
        return ERRCODE_IO_CANTWRITE;
    }

    return ERRCODE_NONE;
}

ErrCode UcbLockBytes::Flush() const
{
    Reference <XOutputStream > xOutputStream = getOutputStream();
    if ( !xOutputStream.is() )
        return ERRCODE_IO_CANTWRITE;

    try
    {
        xOutputStream->flush();
    }
    catch (const Exception&)
    {
        return ERRCODE_IO_CANTWRITE;
    }

    return ERRCODE_NONE;
}

ErrCode UcbLockBytes::SetSize (sal_uInt64 const nNewSize)
{
    SvLockBytesStat aStat;
    Stat( &aStat );
    std::size_t nSize = aStat.nSize;

    if ( nSize > nNewSize )
    {
        Reference < XTruncate > xTrunc( getOutputStream(), UNO_QUERY );
        if ( xTrunc.is() )
        {
            xTrunc->truncate();
            nSize = 0;
        }
        else {
            SAL_INFO("unotools.ucbhelper", "Not truncable!");
        }
    }

    if ( nSize < nNewSize )
    {
        std::size_t nDiff = nNewSize-nSize, nCount=0;
        std::unique_ptr<sal_uInt8[]> pBuffer(new sal_uInt8[ nDiff ]);
        memset(pBuffer.get(), 0, nDiff); // initialize for enhanced security
        WriteAt( nSize, pBuffer.get(), nDiff, &nCount );
        if ( nCount != nDiff )
            return ERRCODE_IO_CANTWRITE;
    }

    return ERRCODE_NONE;
}

ErrCode UcbLockBytes::Stat( SvLockBytesStat *pStat ) const
{
    if ( IsSynchronMode() )
    {
        UcbLockBytes* pThis = const_cast < UcbLockBytes* >( this );
        pThis->m_aInitialized.wait();
    }

    if (!pStat)
        return ERRCODE_IO_INVALIDPARAMETER;

    Reference <XInputStream> xStream = getInputStream();
    Reference <XSeekable> xSeekable = getSeekable();

    if ( !xStream.is() )
    {
        if ( m_bTerminated )
            return ERRCODE_IO_INVALIDACCESS;
        else
            return ERRCODE_IO_PENDING;
    }
    else if( !xSeekable.is() )
        return ERRCODE_IO_CANTTELL;

    try
    {
        pStat->nSize = xSeekable->getLength();
    }
    catch (const IOException&)
    {
        return ERRCODE_IO_CANTTELL;
    }

    return ERRCODE_NONE;
}

UcbLockBytesRef UcbLockBytes::CreateInputLockBytes( const Reference< XInputStream >& xInputStream )
{
    if( !xInputStream.is() )
        return nullptr;

    UcbLockBytesRef xLockBytes = new UcbLockBytes;
    xLockBytes->setDontClose();
    xLockBytes->setInputStream( xInputStream );
    xLockBytes->terminate();
    return xLockBytes;
}

UcbLockBytesRef UcbLockBytes::CreateLockBytes( const Reference< XStream >& xStream )
{
    if( !xStream.is() )
        return nullptr;

    UcbLockBytesRef xLockBytes = new UcbLockBytes;
    xLockBytes->setDontClose();
    xLockBytes->setStream( xStream );
    xLockBytes->terminate();
    return xLockBytes;
}

UcbLockBytesRef UcbLockBytes::CreateLockBytes( const Reference < XContent >& xContent, const Sequence < PropertyValue >& rProps,
        StreamMode eOpenMode, const Reference < XInteractionHandler >& xInteractionHandler )
{
    if( !xContent.is() )
        return nullptr;

    UcbLockBytesRef xLockBytes = new UcbLockBytes;
    xLockBytes->SetSynchronMode();
    Reference< XActiveDataControl > xSink;
    if ( eOpenMode & StreamMode::WRITE )
        xSink = new UcbStreamer_Impl(xLockBytes.get());
    else
        xSink = new UcbDataSink_Impl(xLockBytes.get());

    if ( rProps.hasElements() )
    {
        Reference < XCommandProcessor > xProcessor( xContent, UNO_QUERY );
        Command aCommand;
        aCommand.Name     = "setPropertyValues";
        aCommand.Handle   = -1; /* unknown */
        aCommand.Argument <<= rProps;
        xProcessor->execute( aCommand, 0, Reference < XCommandEnvironment >() );
    }

    OpenCommandArgument2 aArgument;
    aArgument.Sink = xSink;
    aArgument.Mode = OpenMode::DOCUMENT;

    Command aCommand;
    aCommand.Name = "open";
    aCommand.Argument <<= aArgument;

    bool bError = UCBOpenContentSync( xLockBytes,
                                      xContent,
                                      aCommand,
                                      xSink,
                                      xInteractionHandler );

    if ( xLockBytes->GetError() == ERRCODE_NONE && ( bError || !xLockBytes->getInputStream().is() ) )
    {
        OSL_FAIL("No InputStream, but no error set!" );
        xLockBytes->SetError( ERRCODE_IO_GENERAL );
    }

    return xLockBytes;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
