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

#include "sqlcommanddesign.hxx"
#include "formstrings.hxx"
#include <command.hrc>
#include "modulepcr.hxx"
#include "unourl.hxx"

#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/awt/XTopWindow.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/frame/XTitle.hpp>
#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/lang/NullPointerException.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/frame/FrameSearchFlag.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/sdb/CommandType.hpp>

#include <comphelper/propertyvalue.hxx>
#include <utility>
#include <comphelper/diagnose_ex.hxx>
#include <osl/diagnose.h>


namespace pcr
{


    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::beans::PropertyChangeEvent;
    using ::com::sun::star::uno::RuntimeException;
    using ::com::sun::star::frame::XFrame;
    using ::com::sun::star::awt::XTopWindow;
    using ::com::sun::star::awt::XWindow;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::uno::UNO_QUERY_THROW;
    using ::com::sun::star::uno::UNO_QUERY;
    using ::com::sun::star::beans::PropertyValue;
    using ::com::sun::star::uno::Sequence;
    using ::com::sun::star::lang::XComponent;
    using ::com::sun::star::frame::XComponentLoader;
    using ::com::sun::star::beans::XPropertySet;
    using ::com::sun::star::frame::XTitle;
    using ::com::sun::star::lang::EventObject;
    using ::com::sun::star::lang::NullPointerException;
    using ::com::sun::star::lang::DisposedException;
    using ::com::sun::star::uno::XComponentContext;
    using ::com::sun::star::frame::XFrames;
    using ::com::sun::star::util::XCloseable;
    using ::com::sun::star::lang::XMultiServiceFactory;
    using ::com::sun::star::frame::XDispatchProvider;
    using ::com::sun::star::frame::XDispatch;
    using ::com::sun::star::frame::Desktop;
    using ::com::sun::star::frame::XDesktop2;

    namespace FrameSearchFlag = ::com::sun::star::frame::FrameSearchFlag;
    namespace CommandType = ::com::sun::star::sdb::CommandType;


    //= ISQLCommandAdapter


    ISQLCommandAdapter::~ISQLCommandAdapter()
    {
    }


    //= SQLCommandDesigner


    SQLCommandDesigner::SQLCommandDesigner( const Reference< XComponentContext >& _rxContext,
            const ::rtl::Reference< ISQLCommandAdapter >& _rxPropertyAdapter,
            ::dbtools::SharedConnection _aConnection, const Link<SQLCommandDesigner&,void>& _rCloseLink )
        :m_xContext( _rxContext )
        ,m_xConnection(std::move( _aConnection ))
        ,m_xObjectAdapter( _rxPropertyAdapter )
        ,m_aCloseLink( _rCloseLink )
    {
        if ( m_xContext.is() )
            m_xORB = m_xContext->getServiceManager();
        if ( !m_xORB.is() || !_rxPropertyAdapter.is() || !m_xConnection.is() )
            throw NullPointerException();

        impl_doOpenDesignerFrame_nothrow();
    }


    SQLCommandDesigner::~SQLCommandDesigner()
    {
    }


    void SAL_CALL SQLCommandDesigner::propertyChange( const PropertyChangeEvent& Event )
    {
        OSL_ENSURE( m_xDesigner.is() && ( Event.Source == m_xDesigner ), "SQLCommandDesigner::propertyChange: where did this come from?" );

        if ( !(m_xDesigner.is() && ( Event.Source == m_xDesigner )) )
            return;

        try
        {
            if ( PROPERTY_ACTIVECOMMAND == Event.PropertyName )
            {
                OUString sCommand;
                OSL_VERIFY( Event.NewValue >>= sCommand );
                m_xObjectAdapter->setSQLCommand( sCommand );
            }
            else if ( PROPERTY_ESCAPE_PROCESSING == Event.PropertyName )
            {
                bool bEscapeProcessing( false );
                OSL_VERIFY( Event.NewValue >>= bEscapeProcessing );
                m_xObjectAdapter->setEscapeProcessing( bEscapeProcessing );
            }
        }
        catch( const RuntimeException& ) { throw; }
        catch( const Exception& )
        {
            // not allowed to leave, so silence it
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }
    }


    void SAL_CALL SQLCommandDesigner::disposing( const EventObject& Source )
    {
        if ( m_xDesigner.is() && ( Source.Source == m_xDesigner ) )
        {
            m_aCloseLink.Call( *this );
            m_xDesigner.clear();
        }
    }


    void SQLCommandDesigner::dispose()
    {
        if ( impl_isDisposed() )
            return;

        if ( isActive() )
            impl_closeDesigner_nothrow();

        m_xConnection.clear();
        m_xContext.clear();
        m_xORB.clear();
        m_xDesigner.clear();
        m_xObjectAdapter.clear();
    }


    void SQLCommandDesigner::impl_checkDisposed_throw() const
    {
        if ( impl_isDisposed() )
            throw DisposedException();
    }


    void SQLCommandDesigner::raise() const
    {
        impl_checkDisposed_throw();
        impl_raise_nothrow();
    }


    bool SQLCommandDesigner::suspend() const
    {
        impl_checkDisposed_throw();
        return impl_trySuspendDesigner_nothrow();
    }


    void SQLCommandDesigner::impl_raise_nothrow() const
    {
        OSL_PRECOND( isActive(), "SQLCommandDesigner::impl_raise_nothrow: not active!" );
        if ( !isActive() )
            return;

        try
        {
            // activate the frame for this component
            Reference< XFrame >     xFrame( m_xDesigner->getFrame(), css::uno::UNO_SET_THROW );
            Reference< XWindow >    xWindow( xFrame->getContainerWindow(), css::uno::UNO_SET_THROW );
            Reference< XTopWindow > xTopWindow( xWindow, UNO_QUERY_THROW );

            xTopWindow->toFront();
            xWindow->setFocus();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }
    }


    void SQLCommandDesigner::impl_doOpenDesignerFrame_nothrow()
    {
        OSL_PRECOND( !isActive(),
            "SQLCommandDesigner::impl_doOpenDesignerFrame_nothrow: already active!" );
        OSL_PRECOND( m_xConnection.is(), "SQLCommandDesigner::impl_doOpenDesignerFrame_nothrow: this will crash!" );
        osl_atomic_increment(&m_refCount);

        try
        {
            // for various reasons, we don't want the new frame to appear in the desktop's frame list
            // thus, we create a blank frame at the desktop, remove it from the desktop's frame list
            // immediately, and then load the component into this blank (and now parent-less) frame
            Reference< XComponentLoader > xLoader( impl_createEmptyParentlessTask_nothrow(), UNO_QUERY_THROW );
            const bool bEscapeProcessing = m_xObjectAdapter->getEscapeProcessing();
            Sequence< PropertyValue > aArgs{
                comphelper::makePropertyValue(PROPERTY_ACTIVE_CONNECTION, m_xConnection.getTyped()),
                comphelper::makePropertyValue(PROPERTY_COMMAND, m_xObjectAdapter->getSQLCommand()),
                comphelper::makePropertyValue(PROPERTY_COMMANDTYPE, CommandType::COMMAND),
                comphelper::makePropertyValue(PROPERTY_ESCAPE_PROCESSING, bEscapeProcessing),
                comphelper::makePropertyValue(u"GraphicalDesign"_ustr, bEscapeProcessing)
            };

            Reference< XComponent > xQueryDesign = xLoader->loadComponentFromURL(
                u".component:DB/QueryDesign"_ustr,
                u"_self"_ustr,
                FrameSearchFlag::TASKS | FrameSearchFlag::CREATE,
                aArgs
            );

            // remember this newly loaded component - we need to care for it e.g. when we're suspended
            m_xDesigner.set(xQueryDesign, css::uno::UNO_QUERY);
            OSL_ENSURE( m_xDesigner.is() || !xQueryDesign.is(), "SQLCommandDesigner::impl_doOpenDesignerFrame_nothrow: the component is expected to be a controller!" );
            if ( m_xDesigner.is() )
            {
                Reference< XPropertySet > xQueryDesignProps( m_xDesigner, UNO_QUERY );
                OSL_ENSURE( xQueryDesignProps.is(), "SQLCommandDesigner::impl_doOpenDesignerFrame_nothrow: the controller should have properties!" );
                if ( xQueryDesignProps.is() )
                {
                    xQueryDesignProps->addPropertyChangeListener( PROPERTY_ACTIVECOMMAND, this );
                    xQueryDesignProps->addPropertyChangeListener( PROPERTY_ESCAPE_PROCESSING, this );
                }
            }

            // get the frame which we just opened and set its title
            Reference< XTitle> xTitle(xQueryDesign,UNO_QUERY);
            if ( xTitle.is() )
            {
                OUString sDisplayName = PcrRes(RID_RSC_ENUM_COMMAND_TYPE[CommandType::COMMAND]);
                xTitle->setTitle(sDisplayName);
            }
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
            m_xDesigner.clear();
        }
        osl_atomic_decrement(&m_refCount);
    }


    Reference< XFrame > SQLCommandDesigner::impl_createEmptyParentlessTask_nothrow( ) const
    {
        OSL_PRECOND( m_xORB.is(), "SQLCommandDesigner::impl_createEmptyParentlessTask_nothrow: this will crash!" );

        Reference< XFrame > xFrame;
        try
        {
            Reference< XDesktop2 > xDesktop = Desktop::create(m_xContext);

            Reference< XFrames > xDesktopFramesCollection( xDesktop->getFrames(), css::uno::UNO_SET_THROW );
            xFrame = xDesktop->findFrame( u"_blank"_ustr, FrameSearchFlag::CREATE );
            OSL_ENSURE( xFrame.is(), "SQLCommandDesigner::impl_createEmptyParentlessTask_nothrow: could not create an empty frame!" );
            xDesktopFramesCollection->remove( xFrame );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }
        return xFrame;
    }


    void SQLCommandDesigner::impl_closeDesigner_nothrow()
    {
        OSL_PRECOND( isActive(), "SQLCommandDesigner::impl_closeDesigner_nothrow: invalid call!" );
        // close it
        try
        {
            // do not listen anymore...
            Reference< XPropertySet > xProps( m_xDesigner, UNO_QUERY );
            if ( xProps.is() )
                xProps->removePropertyChangeListener( PROPERTY_ACTIVECOMMAND, this );

            // we need to close the frame via the "user interface", by dispatching a close command,
            // instead of calling XCloseable::close directly. The latter method would also close
            // the frame, but not care for things like shutting down the office when the last
            // frame is gone ...
            const UnoURL aCloseURL( u".uno:CloseDoc"_ustr,
                Reference< XMultiServiceFactory >( m_xORB, UNO_QUERY ) );

            Reference< XDispatchProvider > xProvider( m_xDesigner->getFrame(), UNO_QUERY_THROW );
            Reference< XDispatch > xDispatch( xProvider->queryDispatch( aCloseURL, u"_top"_ustr, FrameSearchFlag::SELF ) );
            OSL_ENSURE( xDispatch.is(), "SQLCommandDesigner::impl_closeDesigner_nothrow: no dispatcher for the CloseDoc command!" );
            if ( xDispatch.is() )
            {
                xDispatch->dispatch( aCloseURL, Sequence< PropertyValue >( ) );
            }
            else
            {
                // fallback: use the XCloseable::close (with all possible disadvantages)
                Reference< XCloseable > xClose( m_xDesigner->getFrame(), UNO_QUERY );
                if ( xClose.is() )
                    xClose->close( true );
            }
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }

        m_xDesigner.clear();
    }


    bool SQLCommandDesigner::impl_trySuspendDesigner_nothrow() const
    {
        OSL_PRECOND( isActive(), "SQLCommandDesigner::impl_trySuspendDesigner_nothrow: no active designer, this will crash!" );
        bool bAllow = true;
        try
        {
            bAllow = m_xDesigner->suspend( true );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }
        return bAllow;
    }


} // namespace pcr


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
