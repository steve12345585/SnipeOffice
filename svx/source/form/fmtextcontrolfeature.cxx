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

#include <fmtextcontrolfeature.hxx>
#include <fmtextcontrolshell.hxx>
#include <utility>

#include <osl/diagnose.h>
#include <comphelper/diagnose_ex.hxx>

namespace svx
{


    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::frame;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::util;

    FmTextControlFeature::FmTextControlFeature( const Reference< XDispatch >& _rxDispatcher, URL _aFeatureURL, SfxSlotId _nSlotId, FmTextControlShell* _pInvalidator )
        :m_xDispatcher    ( _rxDispatcher )
        ,m_aFeatureURL    (std::move( _aFeatureURL  ))
        ,m_nSlotId        ( _nSlotId      )
        ,m_pInvalidator   ( _pInvalidator )
        ,m_bFeatureEnabled( false         )
    {
        OSL_ENSURE( _rxDispatcher.is(), "FmTextControlFeature::FmTextControlFeature: invalid dispatcher!"  );
        OSL_ENSURE( m_nSlotId,          "FmTextControlFeature::FmTextControlFeature: invalid slot id!"     );
        OSL_ENSURE( m_pInvalidator,     "FmTextControlFeature::FmTextControlFeature: invalid invalidator!" );

        osl_atomic_increment( &m_refCount );
        try
        {
            m_xDispatcher->addStatusListener( this, m_aFeatureURL );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "svx", "FmTextControlFeature::FmTextControlFeature" );
        }
        osl_atomic_decrement( &m_refCount );
    }


    FmTextControlFeature::~FmTextControlFeature( )
    {
    }


    void FmTextControlFeature::dispatch() const
    {
        dispatch( Sequence< PropertyValue >( ) );
    }


    void FmTextControlFeature::dispatch( const Sequence< PropertyValue >& _rArgs ) const
    {
        try
        {
            if ( m_xDispatcher.is() )
                m_xDispatcher->dispatch( m_aFeatureURL, _rArgs );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "svx", "FmTextControlFeature::dispatch" );
        }
    }


    void SAL_CALL FmTextControlFeature::statusChanged( const FeatureStateEvent& _rState )
    {
        m_aFeatureState   = _rState.State;
        m_bFeatureEnabled = _rState.IsEnabled;

        if ( m_pInvalidator )
            m_pInvalidator->Invalidate( m_nSlotId );
    }


    void SAL_CALL FmTextControlFeature::disposing( const EventObject& /*Source*/ )
    {
        // nothing to do
    }


    void FmTextControlFeature::dispose()
    {
        try
        {
            m_xDispatcher->removeStatusListener( this, m_aFeatureURL );
            m_xDispatcher.clear();
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "svx", "FmTextControlFeature::dispose" );
        }
    }


}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
