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

#include <framework/documentundoguard.hxx>

#include <com/sun/star/document/XUndoManagerSupplier.hpp>

#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>
#include <comphelper/diagnose_ex.hxx>

namespace framework
{

    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::uno::XInterface;
    using ::com::sun::star::uno::UNO_QUERY;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::document::XUndoManagerSupplier;
    using ::com::sun::star::document::XUndoManager;
    using ::com::sun::star::document::XUndoManagerListener;
    using ::com::sun::star::document::UndoManagerEvent;
    using ::com::sun::star::lang::EventObject;

    //= UndoManagerContextListener

    typedef ::cppu::WeakImplHelper <   XUndoManagerListener
                                    >   UndoManagerContextListener_Base;

    class UndoManagerContextListener : public UndoManagerContextListener_Base
    {
    public:
        explicit UndoManagerContextListener( const Reference< XUndoManager >& i_undoManager )
            :m_xUndoManager( i_undoManager )
            ,m_nRelativeContextDepth( 0 )
            ,m_documentDisposed( false )
        {
            osl_atomic_increment( &m_refCount );
            {
                m_xUndoManager->addUndoManagerListener( this );
            }
            osl_atomic_decrement( &m_refCount );
        }

        void finish()
        {
            OSL_ENSURE( m_nRelativeContextDepth >= 0, "UndoManagerContextListener: more contexts left than entered?" );

            if ( m_documentDisposed )
                return;

            // work with a copy of m_nRelativeContextDepth, to be independent from possible bugs in the
            // listener notifications (where it would be decremented with every leaveUndoContext)
            sal_Int32 nDepth = m_nRelativeContextDepth;
            while ( nDepth-- > 0 )
            {
                m_xUndoManager->leaveUndoContext();
            }
            m_xUndoManager->removeUndoManagerListener( this );
        }

        // XUndoManagerListener
        virtual void SAL_CALL undoActionAdded( const UndoManagerEvent& i_event ) override;
        virtual void SAL_CALL actionUndone( const UndoManagerEvent& i_event ) override;
        virtual void SAL_CALL actionRedone( const UndoManagerEvent& i_event ) override;
        virtual void SAL_CALL allActionsCleared( const EventObject& i_event ) override;
        virtual void SAL_CALL redoActionsCleared( const EventObject& i_event ) override;
        virtual void SAL_CALL resetAll( const EventObject& i_event ) override;
        virtual void SAL_CALL enteredContext( const UndoManagerEvent& i_event ) override;
        virtual void SAL_CALL enteredHiddenContext( const UndoManagerEvent& i_event ) override;
        virtual void SAL_CALL leftContext( const UndoManagerEvent& i_event ) override;
        virtual void SAL_CALL leftHiddenContext( const UndoManagerEvent& i_event ) override;
        virtual void SAL_CALL cancelledContext( const UndoManagerEvent& i_event ) override;

        // XEventListener
        virtual void SAL_CALL disposing( const EventObject& i_event ) override;

    private:
        Reference< XUndoManager > const m_xUndoManager;
        oslInterlockedCount             m_nRelativeContextDepth;
        bool                            m_documentDisposed;
    };

    void SAL_CALL UndoManagerContextListener::undoActionAdded( const UndoManagerEvent& )
    {
        // not interested in
    }

    void SAL_CALL UndoManagerContextListener::actionUndone( const UndoManagerEvent& )
    {
        // not interested in
    }

    void SAL_CALL UndoManagerContextListener::actionRedone( const UndoManagerEvent& )
    {
        // not interested in
    }

    void SAL_CALL UndoManagerContextListener::allActionsCleared( const EventObject& )
    {
        // not interested in
    }

    void SAL_CALL UndoManagerContextListener::redoActionsCleared( const EventObject& )
    {
        // not interested in
    }

    void SAL_CALL UndoManagerContextListener::resetAll( const EventObject& )
    {
        m_nRelativeContextDepth = 0;
    }

    void SAL_CALL UndoManagerContextListener::enteredContext( const UndoManagerEvent& )
    {
        osl_atomic_increment( &m_nRelativeContextDepth );
    }

    void SAL_CALL UndoManagerContextListener::enteredHiddenContext( const UndoManagerEvent& )
    {
        osl_atomic_increment( &m_nRelativeContextDepth );
    }

    void SAL_CALL UndoManagerContextListener::leftContext( const UndoManagerEvent& )
    {
        osl_atomic_decrement( &m_nRelativeContextDepth );
    }

    void SAL_CALL UndoManagerContextListener::leftHiddenContext( const UndoManagerEvent& )
    {
        osl_atomic_decrement( &m_nRelativeContextDepth );
    }

    void SAL_CALL UndoManagerContextListener::cancelledContext( const UndoManagerEvent& )
    {
        osl_atomic_decrement( &m_nRelativeContextDepth );
    }

    void SAL_CALL UndoManagerContextListener::disposing( const EventObject& )
    {
        m_documentDisposed = true;
    }

    //= DocumentUndoGuard

    DocumentUndoGuard::DocumentUndoGuard( const Reference< XInterface >& i_undoSupplierComponent )
    {
        try
        {
            Reference< XUndoManagerSupplier > xUndoSupplier( i_undoSupplierComponent, UNO_QUERY );
            if ( xUndoSupplier.is() )
                mxUndoManager.set( xUndoSupplier->getUndoManager(), css::uno::UNO_SET_THROW );

            if ( mxUndoManager.is() )
                mxContextListener.set( new UndoManagerContextListener( mxUndoManager ) );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("fwk");
        }
    }

    DocumentUndoGuard::~DocumentUndoGuard()
    {
        try
        {
            if ( mxContextListener.is() )
                mxContextListener->finish();
            mxContextListener.clear();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("fwk");
        }
    }

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
