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

#include "commoncontrol.hxx"
#include <comphelper/diagnose_ex.hxx>


namespace pcr
{


    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::inspection::XPropertyControlContext;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::inspection::XPropertyControl;

    CommonBehaviourControlHelper::CommonBehaviourControlHelper( sal_Int16 _nControlType, XPropertyControl& _rAntiImpl )
        :m_nControlType( _nControlType )
        ,m_rAntiImpl( _rAntiImpl )
        ,m_bModified( false )
    {
    }


    CommonBehaviourControlHelper::~CommonBehaviourControlHelper()
    {
    }

    void CommonBehaviourControlHelper::setControlContext( const Reference< XPropertyControlContext >& _controlcontext )
    {
        m_xContext = _controlcontext;
    }

    void CommonBehaviourControlHelper::notifyModifiedValue(  )
    {
        if ( isModified() && m_xContext.is() )
        {
            try
            {
                m_xContext->valueChanged( &m_rAntiImpl );
                m_bModified = false;
            }
            catch( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
            }
        }
    }

    void CommonBehaviourControlHelper::editChanged()
    {
        setModified();
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, EditModifiedHdl, weld::Entry&, void )
    {
        editChanged();
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, ModifiedHdl, weld::ComboBox&, void )
    {
        setModified();
        // notify as soon as the Data source is changed, don't wait until we lose focus
        // because the Content dropdown cannot be populated after it is popped up
        // and going from Data source direct to Content may give focus-lost to
        // Content after the popup attempt is made
        notifyModifiedValue();
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, MetricModifiedHdl, weld::MetricSpinButton&, void )
    {
        setModified();
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, FormattedModifiedHdl, weld::FormattedSpinButton&, void )
    {
        setModified();
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, TimeModifiedHdl, weld::FormattedSpinButton&, void )
    {
        setModified();
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, DateModifiedHdl, SvtCalendarBox&, void )
    {
        setModified();
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, ColorModifiedHdl, ColorListBox&, void )
    {
        setModified();
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, GetFocusHdl, weld::Widget&, void )
    {
        try
        {
            if ( m_xContext.is() )
                m_xContext->focusGained( &m_rAntiImpl );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.propctrlr");
        }
    }

    IMPL_LINK_NOARG( CommonBehaviourControlHelper, LoseFocusHdl, weld::Widget&, void )
    {
        // TODO/UNOize: should this be outside the default control's implementations? If somebody
        // has an own control implementation, which does *not* do this - would this be allowed?
        // If not, then we must move this logic out of here.
        notifyModifiedValue();
    }

} // namespace pcr

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
