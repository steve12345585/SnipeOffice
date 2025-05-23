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


#include "propertycontrolextender.hxx"

#include <com/sun/star/awt/KeyFunction.hpp>

#include <comphelper/diagnose_ex.hxx>


namespace pcr
{


    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::uno::UNO_SET_THROW;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::uno::Any;
    using ::com::sun::star::awt::KeyEvent;
    using ::com::sun::star::inspection::XPropertyControl;
    using ::com::sun::star::lang::EventObject;
    using ::com::sun::star::inspection::XPropertyControlContext;

    namespace KeyFunction = ::com::sun::star::awt::KeyFunction;


    //= PropertyControlExtender


    PropertyControlExtender::PropertyControlExtender( const Reference< XPropertyControl >& _rxObservedControl )
    {
        try
        {
            mxControl.set( _rxObservedControl, UNO_SET_THROW );
            mxControlWindow.set( mxControl->getControlWindow(), UNO_SET_THROW );
            mxControlWindow->addKeyListener( this );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }
    }


    PropertyControlExtender::~PropertyControlExtender()
    {
    }


    void SAL_CALL PropertyControlExtender::keyPressed( const KeyEvent& _event )
    {
        OSL_ENSURE( _event.Source == mxControlWindow, "PropertyControlExtender::keyPressed: where does this come from?" );
        if  ( ( _event.KeyFunc != KeyFunction::DELETE )
            ||  ( _event.Modifiers != 0 )
            )
            return;

        try
        {
            Reference< XPropertyControl > xControl( mxControl, UNO_SET_THROW );

            // reset the value
            xControl->setValue( Any() );

            // and notify the change
            // don't use XPropertyControl::notifyModifiedValue. It only notifies when the control content
            // is recognized as being modified by the user, which is not the case, since we just modified
            // it programmatically.
            Reference< XPropertyControlContext > xControlContext( xControl->getControlContext(), UNO_SET_THROW );
            xControlContext->valueChanged( xControl );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }
    }


    void SAL_CALL PropertyControlExtender::keyReleased( const KeyEvent& /*_event*/ )
    {
        // not interested in
    }


    void SAL_CALL PropertyControlExtender::disposing( const EventObject& Source )
    {
        OSL_ENSURE( Source.Source == mxControlWindow, "PropertyControlExtender::disposing: where does this come from?" );
        (void)Source.Source;
        mxControlWindow.clear();
        mxControl.clear();
    }


} // namespace pcr


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
