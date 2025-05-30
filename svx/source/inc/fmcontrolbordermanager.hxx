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

#ifndef INCLUDED_SVX_SOURCE_INC_FMCONTROLBORDERMANAGER_HXX
#define INCLUDED_SVX_SOURCE_INC_FMCONTROLBORDERMANAGER_HXX

#include <com/sun/star/awt/VisualEffect.hpp>
#include <com/sun/star/awt/FontUnderline.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XVclWindowPeer.hpp>
#include <o3tl/typed_flags_set.hxx>
#include <tools/color.hxx>

#include <set>
#include <utility>

namespace com::sun::star::form::validation { class XValidatableFormComponent; }

enum class ControlStatus {
    NONE        = 0x00,
    Focused     = 0x01,
    MouseHover  = 0x02,
    Invalid     = 0x04
};
namespace o3tl {
    template<> struct typed_flags<ControlStatus> : is_typed_flags<ControlStatus, 0x07> {};
}


namespace svxform
{


    struct BorderDescriptor
    {
        sal_Int16   nBorderType;
        Color       nBorderColor;

        BorderDescriptor()
            :nBorderType( css::awt::VisualEffect::FLAT )
        {
        }
    };

    struct UnderlineDescriptor
    {
        sal_Int16 nUnderlineType;
        Color     nUnderlineColor;

        UnderlineDescriptor()
            :nUnderlineType( css::awt::FontUnderline::NONE )
        {
        }

        UnderlineDescriptor( sal_Int16 _nUnderlineType, Color _nUnderlineColor )
            :nUnderlineType( _nUnderlineType )
            ,nUnderlineColor( _nUnderlineColor )
        {
        }
    };

    struct ControlData : public BorderDescriptor, UnderlineDescriptor
    {
        css::uno::Reference< css::awt::XControl > xControl;
        OUString                                                     sOriginalHelpText;

        ControlData() : BorderDescriptor() { }
        ControlData( css::uno::Reference< css::awt::XControl > _xControl )
            :xControl(std::move( _xControl ))
        {
        }
    };


    //= ControlBorderManager

    /** manages the dynamic border color for form controls

        Used by the <type>FormController</type>, this class manages the dynamic changes in the
        border color of form controls. For this a set of events have to be forwarded to the manager
        instance, which then will switch the border color depending on the mouse and focus status
        of the controls.
    */
    class ControlBorderManager
    {
    private:
        struct ControlDataCompare
        {
           bool operator()( const ControlData& _rLHS, const ControlData& _rRHS ) const
           {
               return _rLHS.xControl.get() < _rRHS.xControl.get();
           }
        };

        typedef ::std::set< ControlData, ControlDataCompare > ControlBag;
        typedef ::std::set< css::uno::Reference< css::awt::XVclWindowPeer > >  PeerBag;

        PeerBag     m_aColorableControls;
        PeerBag     m_aNonColorableControls;

        ControlData m_aFocusControl;
        ControlData m_aMouseHoverControl;
        ControlBag  m_aInvalidControls;


        // attributes
        Color       m_nFocusColor;
        Color       m_nMouseHoveColor;
        Color       m_nInvalidColor;
        bool        m_bDynamicBorderColors;

    public:
        ControlBorderManager();
        ~ControlBorderManager();

    public:
        void    focusGained( const css::uno::Reference< css::uno::XInterface >& _rxControl );
        void    focusLost( const css::uno::Reference< css::uno::XInterface >& _rxControl );
        void    mouseEntered( const css::uno::Reference< css::uno::XInterface >& _rxControl );
        void    mouseExited( const css::uno::Reference< css::uno::XInterface >& _rxControl );

        void    validityChanged(
                    const css::uno::Reference< css::awt::XControl >& _rxControl,
                    const css::uno::Reference< css::form::validation::XValidatableFormComponent >& _rxValidatable
                );

        /// enables dynamic border color for the controls
        void    enableDynamicBorderColor( );
        /// disables dynamic border color for the controls
        void    disableDynamicBorderColor( );

        /** sets a color to be used for a given status
            @param _nStatus
                the status which the color should be applied for. Must not be ControlStatus::NONE
            @param _nColor
                the color to apply for the given status
        */
        void    setStatusColor( ControlStatus _nStatus, Color _nColor );

        /** restores all colors of all controls where we possibly changed them
        */
        void    restoreAll();

    private:
        /** called when a control got one of the two possible statuses (focused, and hovered with the mouse)
            @param _rxControl
                the control which gained the status
            @param _rControlData
                the control's status data, as a reference to our respective member
        */
        void    controlStatusGained(
                    const css::uno::Reference< css::uno::XInterface >& _rxControl,
                    ControlData& _rControlData
                );

        /** called when a control lost one of the two possible statuses (focused, and hovered with the mouse)
            @param _rxControl
                the control which lost the status
            @param _rControlData
                the control's status data, as a reference to our respective member
        */
        void    controlStatusLost( const css::uno::Reference< css::uno::XInterface >& _rxControl, ControlData& _rControlData );

        /** determines whether the border of a given peer can be colored
            @param _rxPeer
                the peer to examine. Must not be <NULL/>
        */
        bool    canColorBorder( const css::uno::Reference< css::awt::XVclWindowPeer >& _rxPeer );

        /** determines the status of the given control
        */
        ControlStatus   getControlStatus( const css::uno::Reference< css::awt::XControl >& _rxControl );

        /** retrieves the color associated with a given ControlStatus
            @param _eStatus
                the status of the control. Must not be <member>ControlStatus::none</member>
        */
        Color       getControlColorByStatus( ControlStatus _eStatus ) const;

        /** sets the border color for a given control, depending on its status
            @param _rxControl
                the control to set the border color for. Must not be <NULL/>
            @param _rxPeer
                the peer of the control, to be passed herein for optimization the caller usually needs it, anyway).
                Must not be <NULL/>
            @param _rFallback
                the color/type to use when the control has the status ControlStatus::NONE
        */
        void            updateBorderStyle(
                            const css::uno::Reference< css::awt::XControl >& _rxControl,
                            const css::uno::Reference< css::awt::XVclWindowPeer >& _rxPeer,
                            const BorderDescriptor& _rFallback
                        );

        /** determines the to-be-remembered original border color and type for a control

            The method also takes into account that the control may currently have an overwritten
            border style

            @param _rxControl
                the control to examine. Must not be <NULL/>, and have a non-<NULL/> peer
        */
        void determineOriginalBorderStyle(
                    const css::uno::Reference< css::awt::XControl >& _rxControl,
                    BorderDescriptor& _rData
                ) const;
    };


}


#endif // INCLUDED_SVX_SOURCE_INC_FMCONTROLBORDERMANAGER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
