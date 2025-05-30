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


#include <sdr/contact/viewobjectcontactofunocontrol.hxx>
#include <sdr/contact/viewcontactofunocontrol.hxx>
#include <svx/sdr/contact/displayinfo.hxx>
#include <svx/sdr/contact/objectcontactofpageview.hxx>
#include <svx/sdr/primitive2d/svx_primitivetypes2d.hxx>
#include <svx/svdouno.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdview.hxx>
#include <svx/sdrpagewindow.hxx>

#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XControlContainer.hpp>
#include <com/sun/star/awt/XWindow2.hpp>
#include <com/sun/star/awt/PosSize.hpp>
#include <com/sun/star/awt/XView.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/awt/InvalidateStyle.hpp>
#include <com/sun/star/util/XModeChangeListener.hpp>
#include <com/sun/star/util/XModeChangeBroadcaster.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/container/XContainerListener.hpp>
#include <com/sun/star/container/XContainer.hpp>

#include <vcl/canvastools.hxx>
#include <vcl/svapp.hxx>
#include <vcl/unohelp.hxx>
#include <vcl/window.hxx>
#include <comphelper/lok.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/scopeguard.hxx>
#include <cppuhelper/implbase.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/debug.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/matrix/b2dhommatrixtools.hxx>
#include <drawinglayer/primitive2d/controlprimitive2d.hxx>
#include <drawinglayer/primitive2d/groupprimitive2d.hxx>

#include <utility>
/*

Form controls (more precise: UNO Controls) in the drawing layer are ... prone to breakage, since they have some
specialities which the drawing layer currently doesn't capture too well. In particular, having a living VCL
window as child of the document window, and coupling this Window to a drawing layer object, makes things
difficult sometimes.

Below is a list of issues which existed in the past. Whenever you change code here, you're encouraged to
verify those issues are still fixed. (Whenever you have some additional time, you're encouraged to write
an automatic test for one or more of those issues for which this is possible :)

https://bz.apache.org/ooo/show_bug.cgi?id=105992
zooming documents containing (alive) form controls improperly positions the controls

https://bz.apache.org/ooo/show_bug.cgi?id=104362
crash when copy a control

https://bz.apache.org/ooo/show_bug.cgi?id=104544
Gridcontrol duplicated after design view on/off

https://bz.apache.org/ooo/show_bug.cgi?id=102089
print preview shows control elements with property printable=false

https://bz.apache.org/ooo/show_bug.cgi?id=102090
problem with setVisible on TextControl

https://bz.apache.org/ooo/show_bug.cgi?id=103138
loop when insert a control in draw

https://bz.apache.org/ooo/show_bug.cgi?id=101398
initially-displaying a document with many controls is very slow

https://bz.apache.org/ooo/show_bug.cgi?id=72429
repaint error in form wizard in bugdoc database

https://bz.apache.org/ooo/show_bug.cgi?id=72694
form control artifacts when scrolling a text fast

*/


namespace sdr::contact {


    using namespace ::com::sun::star::awt::InvalidateStyle;
    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::uno::XInterface;
    using ::com::sun::star::uno::UNO_QUERY;
    using ::com::sun::star::uno::UNO_QUERY_THROW;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::awt::XControl;
    using ::com::sun::star::awt::XControlModel;
    using ::com::sun::star::awt::XControlContainer;
    using ::com::sun::star::awt::XWindow2;
    using ::com::sun::star::awt::XWindowListener;
    using ::com::sun::star::awt::PosSize::POSSIZE;
    using ::com::sun::star::awt::XView;
    using ::com::sun::star::awt::WindowEvent;
    using ::com::sun::star::beans::XPropertySet;
    using ::com::sun::star::beans::XPropertySetInfo;
    using ::com::sun::star::lang::XComponent;
    using ::com::sun::star::awt::XWindowPeer;
    using ::com::sun::star::beans::XPropertyChangeListener;
    using ::com::sun::star::util::XModeChangeListener;
    using ::com::sun::star::util::XModeChangeBroadcaster;
    using ::com::sun::star::util::ModeChangeEvent;
    using ::com::sun::star::lang::EventObject;
    using ::com::sun::star::beans::PropertyChangeEvent;
    using ::com::sun::star::container::XContainerListener;
    using ::com::sun::star::container::XContainer;
    using ::com::sun::star::container::ContainerEvent;
    using ::com::sun::star::uno::Any;

    namespace {

    class ControlHolder
    {
    private:
        Reference< XControl >   m_xControl;
        Reference< XWindow2 >   m_xControlWindow;
        Reference< XView    >   m_xControlView;

    public:
        ControlHolder()
        {
        }

        explicit ControlHolder( const Reference< XControl >& _rxControl )
        {
            *this = _rxControl;
        }

        ControlHolder& operator=( const Reference< XControl >& _rxControl )
        {
            clear();

            m_xControl = _rxControl;
            if ( m_xControl.is() )
            {
                m_xControlWindow.set( m_xControl, UNO_QUERY );
                m_xControlView.set( m_xControl, UNO_QUERY );
                if ( !m_xControlWindow.is() || !m_xControlView.is() )
                {
                    OSL_FAIL( "ControlHolder::operator=: invalid XControl, missing required interfaces!" );
                    clear();
                }
            }

            return *this;
        }

    public:
        bool    is() const { return m_xControl.is() && m_xControlWindow.is() && m_xControlView.is(); }
        void    clear() { m_xControl.clear(); m_xControlWindow.clear(); m_xControlView.clear(); }

        // delegators for the methods of the UNO interfaces
        // Note all those will crash if called for a NULL object.
        bool     isDesignMode() const                        { return m_xControl->isDesignMode();         }
        void     setDesignMode( const bool _bDesign ) const  { m_xControl->setDesignMode( _bDesign );     }
        bool     isVisible() const                           { return m_xControlWindow->isVisible();      }
        void     setVisible( const bool _bVisible ) const    { m_xControlWindow->setVisible( _bVisible ); }
        Reference< XControlModel >
                        getModel() const { return m_xControl->getModel(); }
        void     setModel( const Reference< XControlModel >& _m ) const { m_xControl->setModel( _m ); }

        void     addWindowListener( const Reference< XWindowListener >& _l ) const    { m_xControlWindow->addWindowListener( _l );    }
        void     removeWindowListener( const Reference< XWindowListener >& _l ) const { m_xControlWindow->removeWindowListener( _l ); }
               void     setPosSize( const tools::Rectangle& _rPosSize ) const;
               tools::Rectangle
                        getPosSize() const;
               void     setZoom( const ::basegfx::B2DVector& _rScale ) const;
               ::basegfx::B2DVector
                        getZoom() const;

               void     invalidate() const;

    public:
        const Reference< XControl >&    getControl() const  { return m_xControl; }
    };

    bool operator==( const ControlHolder& _rControl, const Reference< XInterface >& _rxCompare )
    {
        return _rControl.getControl() == _rxCompare;
    }

    bool operator==( const ControlHolder& _rControl, const Any& _rxCompare )
    {
        return _rControl == Reference< XInterface >( _rxCompare, UNO_QUERY );
    }

    }

    void ControlHolder::setPosSize( const tools::Rectangle& _rPosSize ) const
    {
        // no check whether we're valid, this is the responsibility of the caller

        // don't call setPosSize when pos/size did not change #i104181#
        ::tools::Rectangle aCurrentRect( getPosSize() );
        if ( aCurrentRect != _rPosSize )
        {
            m_xControlWindow->setPosSize(
                _rPosSize.Left(), _rPosSize.Top(), _rPosSize.GetWidth(), _rPosSize.GetHeight(),
                POSSIZE
            );
        }
    }


    ::tools::Rectangle ControlHolder::getPosSize() const
    {
        // no check whether we're valid, this is the responsibility of the caller
        return vcl::unohelper::ConvertToVCLRect( m_xControlWindow->getPosSize() );
    }


    void ControlHolder::setZoom( const ::basegfx::B2DVector& _rScale ) const
    {
        // no check whether we're valid, this is the responsibility of the caller
        m_xControlView->setZoom( static_cast<float>(_rScale.getX()), static_cast<float>(_rScale.getY()) );
    }


    void ControlHolder::invalidate() const
    {
        Reference< XWindowPeer > xPeer( m_xControl->getPeer() );
        if ( xPeer.is() )
        {
            VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow( xPeer );
            OSL_ENSURE( pWindow, "ControlHolder::invalidate: no implementation access!" );
            if ( pWindow )
                pWindow->Invalidate();
        }
    }


    ::basegfx::B2DVector ControlHolder::getZoom() const
    {
        // no check whether we're valid, this is the responsibility of the caller

        // Argh. Why does XView have a setZoom only, but not a getZoom?
        VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow( m_xControl->getPeer() );
        OSL_ENSURE( pWindow, "ControlHolder::getZoom: no implementation access!" );

        ::basegfx::B2DVector aZoom( 1, 1 );
        if ( pWindow )
        {
            const Fraction& rZoom( pWindow->GetZoom() );
            aZoom.setX( static_cast<double>(rZoom) );
            aZoom.setY( static_cast<double>(rZoom) );
        }
        return aZoom;
    }

    namespace UnoControlContactHelper {

    /** positions a control, and sets its zoom mode, using a given transformation and output device
     */
    static void adjustControlGeometry_throw( const ControlHolder& _rControl, const tools::Rectangle& _rLogicBoundingRect,
        const basegfx::B2DHomMatrix& _rViewTransformation, const ::basegfx::B2DHomMatrix& _rZoomLevelNormalization )
    {
        // In the LOK case, control geometry is handled by LokControlHandler
        if (comphelper::LibreOfficeKit::isActive())
            return;

        OSL_PRECOND( _rControl.is(), "UnoControlContactHelper::adjustControlGeometry_throw: illegal control!" );
        if ( !_rControl.is() )
            return;

    #if OSL_DEBUG_LEVEL > 0
        ::basegfx::B2DTuple aViewScale, aViewTranslate;
        double nViewRotate(0), nViewShearX(0);
        _rViewTransformation.decompose( aViewScale, aViewTranslate, nViewRotate, nViewShearX );

        ::basegfx::B2DTuple aZoomScale, aZoomTranslate;
        double nZoomRotate(0), nZoomShearX(0);
        _rZoomLevelNormalization.decompose( aZoomScale, aZoomTranslate, nZoomRotate, nZoomShearX );
    #endif

        // transform the logic bound rect, using the view transformation, to pixel coordinates
        ::basegfx::B2DPoint aTopLeft( _rLogicBoundingRect.Left(), _rLogicBoundingRect.Top() );
        aTopLeft *= _rViewTransformation;
        ::basegfx::B2DPoint aBottomRight( _rLogicBoundingRect.Right(), _rLogicBoundingRect.Bottom() );
        aBottomRight *= _rViewTransformation;

        const tools::Rectangle aPaintRectPixel(static_cast<tools::Long>(std::round(aTopLeft.getX())),
                                               static_cast<tools::Long>(std::round(aTopLeft.getY())),
                                               static_cast<tools::Long>(std::round(aBottomRight.getX())),
                                               static_cast<tools::Long>(std::round(aBottomRight.getY())));
        _rControl.setPosSize( aPaintRectPixel );

        // determine the scale from the current view transformation, and the normalization matrix
        ::basegfx::B2DHomMatrix aObtainResolutionDependentScale( _rViewTransformation * _rZoomLevelNormalization );
        ::basegfx::B2DVector aScale, aTranslate;
        double fRotate, fShearX;
        aObtainResolutionDependentScale.decompose( aScale, aTranslate, fRotate, fShearX );
        _rControl.setZoom( aScale );
    }

    /** disposes the given control
     */
    static void disposeAndClearControl_nothrow( ControlHolder& _rControl )
    {
        try
        {
            Reference< XComponent > xControlComp = _rControl.getControl();
            if ( xControlComp.is() )
                xControlComp->dispose();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
        _rControl.clear();
    }

    }

    namespace {

    /** interface encapsulating access to an SdrPageView, stripped down to the methods we really need
     */
    class IPageViewAccess
    {
    public:
        /** determines whether the view is currently in design mode
         */
        virtual bool    isDesignMode() const = 0;

        /** retrieves the control container for a given output device
         */
        virtual Reference< XControlContainer >
                        getControlContainer( const OutputDevice& _rDevice ) const = 0;

        /** determines whether a given layer is visible
         */
        virtual bool    isLayerVisible( SdrLayerID _nLayerID ) const = 0;

    protected:
        ~IPageViewAccess() {}
    };

    /** is a ->IPageViewAccess implementation based on a real ->SdrPageView instance
     */
    class SdrPageViewAccess : public IPageViewAccess
    {
        const SdrPageView&  m_rPageView;
    public:
        explicit SdrPageViewAccess( const SdrPageView& _rPageView ) : m_rPageView( _rPageView ) { }

        virtual ~SdrPageViewAccess() {}

        virtual bool    isDesignMode() const override;
        virtual Reference< XControlContainer >
                        getControlContainer( const OutputDevice& _rDevice ) const override;
        virtual bool    isLayerVisible( SdrLayerID _nLayerID ) const override;
    };

    }

    bool SdrPageViewAccess::isDesignMode() const
    {
        return m_rPageView.GetView().IsDesignMode();
    }


    Reference< XControlContainer > SdrPageViewAccess::getControlContainer( const OutputDevice& _rDevice ) const
    {
        Reference< XControlContainer > xControlContainer = m_rPageView.GetControlContainer( _rDevice );
        DBG_ASSERT( xControlContainer.is() || nullptr == m_rPageView.FindPageWindow( _rDevice ),
            "SdrPageViewAccess::getControlContainer: the output device is known, but there is no control container for it?" );
        return xControlContainer;
    }


    bool SdrPageViewAccess::isLayerVisible( SdrLayerID _nLayerID ) const
    {
        return m_rPageView.GetVisibleLayers().IsSet( _nLayerID );
    }

    namespace {

    /** is a ->IPageViewAccess implementation which can be used to create an invisible control for
        an arbitrary window
     */
    class InvisibleControlViewAccess : public IPageViewAccess
    {
    private:
        Reference< XControlContainer >& m_rControlContainer;
    public:
        explicit InvisibleControlViewAccess( Reference< XControlContainer >& _inout_ControlContainer )
            :m_rControlContainer( _inout_ControlContainer )
        {
        }

        virtual ~InvisibleControlViewAccess() {}

        virtual bool    isDesignMode() const override;
        virtual Reference< XControlContainer >
                        getControlContainer( const OutputDevice& _rDevice ) const override;
        virtual bool    isLayerVisible( SdrLayerID _nLayerID ) const override;
    };

    }

    bool InvisibleControlViewAccess::isDesignMode() const
    {
        return true;
    }


    Reference< XControlContainer > InvisibleControlViewAccess::getControlContainer( const OutputDevice& _rDevice ) const
    {
        if ( !m_rControlContainer.is() )
        {
            const vcl::Window* pWindow = _rDevice.GetOwnerWindow();
            OSL_ENSURE( pWindow, "InvisibleControlViewAccess::getControlContainer: expected to be called for a window only!" );
            if ( pWindow )
                m_rControlContainer = VCLUnoHelper::CreateControlContainer( const_cast< vcl::Window* >( pWindow ) );
        }
        return m_rControlContainer;
    }


    bool InvisibleControlViewAccess::isLayerVisible( SdrLayerID /*_nLayerID*/ ) const
    {
        return false;
    }

    namespace {

    //= DummyPageViewAccess

    /** is a ->IPageViewAccess implementation which can be used to create a control for an arbitrary
        non-Window device

        The implementation will report the "PageView" as being in design mode, all layers to be visible,
        and will not return any ControlContainer, so all control container related features (notifications etc)
        are not available.
     */
    class DummyPageViewAccess : public IPageViewAccess
    {
    public:
        DummyPageViewAccess()
        {
        }

        virtual ~DummyPageViewAccess() {}

        virtual bool    isDesignMode() const override;
        virtual Reference< XControlContainer >
                        getControlContainer( const OutputDevice& _rDevice ) const override;
        virtual bool    isLayerVisible( SdrLayerID _nLayerID ) const override;
    };

    }

    bool DummyPageViewAccess::isDesignMode() const
    {
        return true;
    }


    Reference< XControlContainer > DummyPageViewAccess::getControlContainer( const OutputDevice& /*_rDevice*/ ) const
    {
        return nullptr;
    }


    bool DummyPageViewAccess::isLayerVisible( SdrLayerID /*_nLayerID*/ ) const
    {
        return true;
    }


    //= ViewObjectContactOfUnoControl_Impl

    typedef     ::cppu::WeakImplHelper <   XWindowListener
                                        ,   XPropertyChangeListener
                                        ,   XContainerListener
                                        ,   XModeChangeListener
                                        >   ViewObjectContactOfUnoControl_Impl_Base;

    class ViewObjectContactOfUnoControl_Impl:
        public ViewObjectContactOfUnoControl_Impl_Base
    {
    private:
        // tdf#41935 note that access to members is protected with SolarMutex;
        // the class previously had its own mutex but that is prone to deadlock

        /// the instance whose IMPL we are
        ViewObjectContactOfUnoControl*  m_pAntiImpl;

        /// are we currently inside impl_ensureControl_nothrow?
        bool                            m_bCreatingControl;

        /// the control we're responsible for
        ControlHolder                   m_aControl;

        /// the ControlContainer where we inserted our control
        Reference< XContainer >         m_xContainer;

        /// the output device for which the control was created
        VclPtr<OutputDevice>            m_pOutputDeviceForWindow;

        /// flag indicating whether the control is currently visible
        bool                            m_bControlIsVisible;

        /// are we currently listening at a design mode control?
        bool                            m_bIsDesignModeListening;

        enum ViewControlMode
        {
            eDesign,
            eAlive,
            eUnknown
        };
        /// is the control currently in design mode?
        mutable ViewControlMode         m_eControlDesignMode;

        ::basegfx::B2DHomMatrix         m_aZoomLevelNormalization;

    public:
        explicit ViewObjectContactOfUnoControl_Impl( ViewObjectContactOfUnoControl* _pAntiImpl );
        ViewObjectContactOfUnoControl_Impl(const ViewObjectContactOfUnoControl_Impl&) = delete;
        ViewObjectContactOfUnoControl_Impl& operator=(const ViewObjectContactOfUnoControl_Impl&) = delete;

        /** disposes the instance, which is nonfunctional afterwards
        */
        void dispose();

        /** determines whether the instance is disposed
        */
        bool isDisposed() const { return impl_isDisposed_nofail(); }

        /** returns the SdrUnoObject associated with the ViewContact

            @precond
                We're not disposed.
        */
        SdrUnoObj*    getUnoObject() const;

        /** ensures that we have an ->XControl

            Must only be called if a control is needed when no DisplayInfo is present, yet.

            For creating a control, an ->OutputDevice is needed, and an ->SdrPageView. Both will be obtained
            from a ->ObjectContactOfPageView. So, if our (anti-impl's) object contact is not a ->ObjectContactOfPageView,
            this method fill fail.

            Failure of this method will be reported via an assertion in a non-product version.
        */
        void    ensureControl( const basegfx::B2DHomMatrix* _pInitialViewTransformationOrNULL );

        /** returns our XControl, if it already has been created

            If you want to ensure that the control exists before accessing it, use ->ensureControl
        */
        const ControlHolder&
                getExistentControl() const { return m_aControl; }

        bool
                hasControl() const { return m_aControl.is(); }

        /** positions our XControl according to the geometry settings in the SdrUnoObj, modified by the given
            transformation, and sets proper zoom settings according to our device

            @precond
                ->m_pOutputDeviceForWindow and ->m_aControl are not <NULL/>
        */
        void    positionAndZoomControl( const basegfx::B2DHomMatrix& _rViewTransformation ) const;

        /** determines whether or not our control is printable

            Effectively, this method returns the value of the "Printable" property
            of the control's model. If we have no control, <FALSE/> is returned.
        */
        bool    isPrintableControl() const;

        /** sets the design mode on the control, or at least remembers the flag for the
            time the control is created
        */
        void    setControlDesignMode( bool _bDesignMode ) const;

        /** determines whether our control is currently visible
            @nofail
        */
        bool    isControlVisible() const { return m_bControlIsVisible; }

        /// creates an XControl for the given device and SdrUnoObj
        static bool
                createControlForDevice(
                    IPageViewAccess const & _rPageView,
                    const OutputDevice& _rDevice,
                    const SdrUnoObj& _rUnoObject,
                    const basegfx::B2DHomMatrix& _rInitialViewTransformation,
                    const basegfx::B2DHomMatrix& _rInitialZoomNormalization,
                    ControlHolder& _out_rControl
                );

        const ViewContactOfUnoControl&
                getViewContact() const
        {
            ENSURE_OR_THROW( !impl_isDisposed_nofail(), "already disposed" );
            return static_cast< const ViewContactOfUnoControl& >( m_pAntiImpl->GetViewContact() );
        }

    protected:
        virtual ~ViewObjectContactOfUnoControl_Impl() override;

        // XEventListener
        virtual void SAL_CALL disposing( const EventObject& Source ) override;

        // XWindowListener
        virtual void SAL_CALL windowResized( const WindowEvent& e ) override;
        virtual void SAL_CALL windowMoved( const WindowEvent& e ) override;
        virtual void SAL_CALL windowShown( const EventObject& e ) override;
        virtual void SAL_CALL windowHidden( const EventObject& e ) override;

        // XPropertyChangeListener
        virtual void SAL_CALL propertyChange( const PropertyChangeEvent& evt ) override;

        // XModeChangeListener
        virtual void SAL_CALL modeChanged( const ModeChangeEvent& _rSource ) override;

        // XContainerListener
        virtual void SAL_CALL elementInserted( const css::container::ContainerEvent& Event ) override;
        virtual void SAL_CALL elementRemoved( const css::container::ContainerEvent& Event ) override;
        virtual void SAL_CALL elementReplaced( const css::container::ContainerEvent& Event ) override;

    private:
        /** retrieves the SdrPageView which our associated SdrPageViewWindow belongs to

            @param out_rpPageView
                a reference to a pointer holding, upon return, the desired SdrPageView

            @return
                <TRUE/> if and only if a ->SdrPageView could be obtained

            @precond
                We really belong to an SdrPageViewWindow. Perhaps (I'm not sure ATM :)
                there are instance for which this might not be true, but those instances
                should never have a need to call this method.

            @precond
                We're not disposed.

            @postcond
                The method expects success, if it returns with <FALSE/>, this will have been
                asserted.

            @nothrow
        */
        bool    impl_getPageView_nothrow( SdrPageView*& _out_rpPageView );

        /** adjusts the control visibility so it respects its layer's visibility

            @precond
                ->m_aControl is not <NULL/>

            @precond
                We're not disposed.

            @precond
                We really belong to an SdrPageViewWindow. There are instance for which this
                might not be true, but those instances should never have a need to call
                this method.
        */
        void impl_adjustControlVisibilityToLayerVisibility_throw();

        /** adjusts the control visibility so it respects its layer's visibility

            The control must never be visible if it's in design mode.
            In alive mode, it must be visibility if and only it's on a visible layer.

            @param _rxControl
                the control whose visibility is to be adjusted

            @param _rPageView
                provides access to the attributes of the SdrPageView which the control finally belongs to

            @param _rUnoObject
                our SdrUnoObj

            @param _bIsCurrentlyVisible
                determines whether the control is currently visible. Note that this is only a shortcut for
                querying _rxControl for the XWindow2 interface, and calling isVisible at this interface.
                This shortcut has been chosen since the caller usually already has this information.
                If _bForce is <TRUE/>, _bIsCurrentlyVisible is ignored.

            @param _bForce
                set to <TRUE/> if you want to force a ->XWindow::setVisible call,
                no matter if the control visibility is already correct

            @precond
                We're not disposed.
        */
        static void impl_adjustControlVisibilityToLayerVisibility_throw( const ControlHolder& _rxControl, const SdrUnoObj& _rUnoObject,
            IPageViewAccess const & _rPageView, bool _bIsCurrentlyVisible, bool _bForce );

        /** starts or stops listening at various aspects of our control

            @precond
                ->m_aControl is not <NULL/>
        */
        void impl_switchControlListening_nothrow( bool _bStart );

        /** starts or stops listening at our control container

            @precond
                ->m_xContainer is not <NULL/>
        */
        void impl_switchContainerListening_nothrow( bool _bStart );

        /** starts or stops listening at the control for design-mode relevant facets
        */
        void impl_switchDesignModeListening_nothrow( bool _bStart );

        /** starts or stops listening for all properties at our control

            @param _bStart
                determines whether to start or to stop listening

            @precond
                ->m_aControl is not <NULL/>
        */
        void impl_switchPropertyListening_nothrow( bool _bStart );

        /** disposes the instance
            @param _bAlsoDisposeControl
                determines whether the XControl should be disposed, too
        */
        void impl_dispose_nothrow( bool _bAlsoDisposeControl );

        /** determines whether the instance is disposed
            @nofail
        */
        bool    impl_isDisposed_nofail() const { return m_pAntiImpl == nullptr; }

        /** determines whether the control currently is in design mode

            @precond
                The design mode must already be known. It is known when we first had access to
                an SdrPageView (which carries this flag), or somebody explicitly set it from
                outside.
        */
        bool impl_isControlDesignMode_nothrow() const
        {
            DBG_ASSERT( m_eControlDesignMode != eUnknown, "ViewObjectContactOfUnoControl_Impl::impl_isControlDesignMode_nothrow: mode is still unknown!" );
            return m_eControlDesignMode == eDesign;
        }

        /** ensures that we have a control for the given PageView/OutputDevice
        */
        bool impl_ensureControl_nothrow(
                IPageViewAccess const & _rPageView,
                const OutputDevice& _rDevice,
                const basegfx::B2DHomMatrix& _rInitialViewTransformation
             );

        const OutputDevice& impl_getOutputDevice_throw() const;
    };

    namespace {

    class LazyControlCreationPrimitive2D : public ::drawinglayer::primitive2d::BufferedDecompositionPrimitive2D
    {
    private:
        typedef ::drawinglayer::primitive2d::BufferedDecompositionPrimitive2D  BufferedDecompositionPrimitive2D;

    protected:
        virtual void
            get2DDecomposition(
                ::drawinglayer::primitive2d::Primitive2DDecompositionVisitor& rVisitor,
                const ::drawinglayer::geometry::ViewInformation2D& rViewInformation
            ) const override;

        virtual ::drawinglayer::primitive2d::Primitive2DReference create2DDecomposition(
                const ::drawinglayer::geometry::ViewInformation2D& rViewInformation
            ) const override;

        virtual ::basegfx::B2DRange
            getB2DRange(
                const ::drawinglayer::geometry::ViewInformation2D& rViewInformation
            ) const override;

    public:
        explicit LazyControlCreationPrimitive2D( ::rtl::Reference< ViewObjectContactOfUnoControl_Impl > _pVOCImpl )
            :m_pVOCImpl(std::move( _pVOCImpl ))
        {
            ENSURE_OR_THROW( m_pVOCImpl.is(), "Illegal argument." );
            getTransformation( m_pVOCImpl->getViewContact(), m_aTransformation );
        }

        virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

        // declare unique ID for this primitive class
        virtual sal_uInt32 getPrimitive2DID() const override;

        static void getTransformation( const ViewContactOfUnoControl& _rVOC, ::basegfx::B2DHomMatrix& _out_Transformation );

    private:
        void impl_positionAndZoomControl( const ::drawinglayer::geometry::ViewInformation2D& _rViewInformation ) const
        {
            if ( !_rViewInformation.getViewport().isEmpty() )
                m_pVOCImpl->positionAndZoomControl( _rViewInformation.getObjectToViewTransformation() );
        }

    private:
        ::rtl::Reference< ViewObjectContactOfUnoControl_Impl >  m_pVOCImpl;
        /** The geometry is part of the identity of a primitive, so we cannot calculate it on demand
            (since the data the calculation is based on might have changed then), but need to calc
            it at construction time, and remember it.
        */
        ::basegfx::B2DHomMatrix                                 m_aTransformation;
    };

    }

    ViewObjectContactOfUnoControl_Impl::ViewObjectContactOfUnoControl_Impl( ViewObjectContactOfUnoControl* _pAntiImpl )
        :m_pAntiImpl( _pAntiImpl )
        ,m_bCreatingControl( false )
        ,m_pOutputDeviceForWindow( nullptr )
        ,m_bControlIsVisible( false )
        ,m_bIsDesignModeListening( false )
        ,m_eControlDesignMode( eUnknown )
    {
        DBG_ASSERT( m_pAntiImpl, "ViewObjectContactOfUnoControl_Impl::ViewObjectContactOfUnoControl_Impl: invalid AntiImpl!" );

        const OutputDevice& rPageViewDevice( impl_getOutputDevice_throw() );
        m_aZoomLevelNormalization = rPageViewDevice.GetInverseViewTransformation();

    #if OSL_DEBUG_LEVEL > 0
        ::basegfx::B2DVector aScale, aTranslate;
        double fRotate, fShearX;
        m_aZoomLevelNormalization.decompose( aScale, aTranslate, fRotate, fShearX );
    #endif

        ::basegfx::B2DHomMatrix aScaleNormalization;
        const MapMode& aCurrentDeviceMapMode( rPageViewDevice.GetMapMode() );
        aScaleNormalization.set( 0, 0, static_cast<double>(aCurrentDeviceMapMode.GetScaleX()) );
        aScaleNormalization.set( 1, 1, static_cast<double>(aCurrentDeviceMapMode.GetScaleY()) );
        m_aZoomLevelNormalization *= aScaleNormalization;

    #if OSL_DEBUG_LEVEL > 0
        m_aZoomLevelNormalization.decompose( aScale, aTranslate, fRotate, fShearX );
    #endif
   }


    ViewObjectContactOfUnoControl_Impl::~ViewObjectContactOfUnoControl_Impl()
    {
        if ( !impl_isDisposed_nofail() )
        {
            acquire();
            dispose();
        }

    }


    void ViewObjectContactOfUnoControl_Impl::impl_dispose_nothrow( bool _bAlsoDisposeControl )
    {
        if ( impl_isDisposed_nofail() )
            return;

        if ( m_aControl.is() )
            impl_switchControlListening_nothrow( false );

        if ( m_xContainer.is() )
            impl_switchContainerListening_nothrow( false );

        // dispose the control
        if ( _bAlsoDisposeControl )
            UnoControlContactHelper::disposeAndClearControl_nothrow( m_aControl );

        m_aControl.clear();
        m_xContainer.clear();
        m_pOutputDeviceForWindow = nullptr;
        m_bControlIsVisible = false;

        m_pAntiImpl = nullptr;
    }


    void ViewObjectContactOfUnoControl_Impl::dispose()
    {
        SolarMutexGuard aSolarGuard;
        impl_dispose_nothrow( true );
    }


    SdrUnoObj* ViewObjectContactOfUnoControl_Impl::getUnoObject() const
    {
        OSL_PRECOND( !impl_isDisposed_nofail(), "ViewObjectContactOfUnoControl_Impl::getUnoObject: already disposed()" );
        if ( impl_isDisposed_nofail() )
            return nullptr;
        auto pRet = dynamic_cast< SdrUnoObj* >( m_pAntiImpl->GetViewContact().TryToGetSdrObject() );
        DBG_ASSERT( pRet || !m_pAntiImpl->GetViewContact().TryToGetSdrObject(),
            "ViewObjectContactOfUnoControl_Impl::getUnoObject: invalid SdrObject!" );
        return pRet;
    }


    void ViewObjectContactOfUnoControl_Impl::positionAndZoomControl( const basegfx::B2DHomMatrix& _rViewTransformation ) const
    {
        OSL_PRECOND( m_aControl.is(), "ViewObjectContactOfUnoControl_Impl::positionAndZoomControl: no output device or no control!" );
        if ( !m_aControl.is() )
            return;

        try
        {
            SdrUnoObj* pUnoObject = getUnoObject();
            if ( pUnoObject )
            {
                const tools::Rectangle aRect( pUnoObject->GetLogicRect() );
                UnoControlContactHelper::adjustControlGeometry_throw( m_aControl, aRect, _rViewTransformation, m_aZoomLevelNormalization );
            }
            else
                OSL_FAIL( "ViewObjectContactOfUnoControl_Impl::positionAndZoomControl: no SdrUnoObj!" );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }


    void ViewObjectContactOfUnoControl_Impl::ensureControl( const basegfx::B2DHomMatrix* _pInitialViewTransformationOrNULL )
    {
        OSL_PRECOND( !impl_isDisposed_nofail(), "ViewObjectContactOfUnoControl_Impl::ensureControl: already disposed()" );
        if ( impl_isDisposed_nofail() )
            return;

        ObjectContactOfPageView* pPageViewContact = dynamic_cast< ObjectContactOfPageView* >( &m_pAntiImpl->GetObjectContact() );
        if ( pPageViewContact )
        {
            SdrPageViewAccess aPVAccess( pPageViewContact->GetPageWindow().GetPageView() );
            const OutputDevice& rDevice( *m_pAntiImpl->getPageViewOutputDevice() );
            impl_ensureControl_nothrow(
                aPVAccess,
                rDevice,
                _pInitialViewTransformationOrNULL ? *_pInitialViewTransformationOrNULL : rDevice.GetViewTransformation()
            );
            return;
        }

        DummyPageViewAccess aNoPageView;
        const OutputDevice& rDevice( impl_getOutputDevice_throw() );
        impl_ensureControl_nothrow(
            aNoPageView,
            rDevice,
            _pInitialViewTransformationOrNULL ? *_pInitialViewTransformationOrNULL : rDevice.GetViewTransformation()
        );
    }


    const OutputDevice& ViewObjectContactOfUnoControl_Impl::impl_getOutputDevice_throw() const
    {
        // do not use ObjectContact::TryToGetOutputDevice, it would not care for the PageWindow's
        // OriginalPaintWindow
        const OutputDevice* oPageOutputDev = m_pAntiImpl->getPageViewOutputDevice();
        if( oPageOutputDev )
            return *oPageOutputDev;

        const OutputDevice* pDevice = m_pAntiImpl->GetObjectContact().TryToGetOutputDevice();
        ENSURE_OR_THROW( pDevice, "no output device -> no control" );
        return *pDevice;
    }


    namespace
    {
        void lcl_resetFlag( bool& rbFlag )
        {
            rbFlag = false;
        }
    }


    bool ViewObjectContactOfUnoControl_Impl::impl_ensureControl_nothrow( IPageViewAccess const & _rPageView, const OutputDevice& _rDevice,
        const basegfx::B2DHomMatrix& _rInitialViewTransformation )
    {
        if ( m_bCreatingControl )
        {
            OSL_FAIL( "ViewObjectContactOfUnoControl_Impl::impl_ensureControl_nothrow: reentrance is not really good here!" );
            // We once had a situation where this was called reentrantly, which lead to all kind of strange effects. All
            // those affected the grid control, which is the only control so far which is visible in design mode (and
            // not only in alive mode).
            // Creating the control triggered a Window::Update on some of its child windows, which triggered a
            // Paint on parent of the grid control (e.g. the SwEditWin), which triggered a reentrant call to this method,
            // which it is not really prepared for.

            // /me thinks that re-entrance should be caught on a higher level, i.e. the Drawing Layer should not allow
            // reentrant paint requests. For the moment, until /me can discuss this with AW, catch it here. #i104544#
            return false;
        }

        m_bCreatingControl = true;
        ::comphelper::ScopeGuard aGuard([&] () { lcl_resetFlag(m_bCreatingControl); });

        if ( m_aControl.is() )
        {
            if ( m_pOutputDeviceForWindow.get() == &_rDevice )
                return true;

            // Somebody requested a control for a new device, which means either of
            // - our PageView's paint window changed since we were last here
            // - we don't belong to a page view, and are simply painted onto different devices
            // The first sounds strange (doesn't it?), the second means we could perhaps
            // optimize this in the future - there is no need to re-create the control every time,
            // is it? #i74523#
            if ( m_xContainer.is() )
                impl_switchContainerListening_nothrow( false );
            impl_switchControlListening_nothrow( false );
            UnoControlContactHelper::disposeAndClearControl_nothrow( m_aControl );
        }

        SdrUnoObj* pUnoObject = getUnoObject();
        if ( !pUnoObject )
            return false;

        ControlHolder aControl;
        if ( !createControlForDevice( _rPageView, _rDevice, *pUnoObject, _rInitialViewTransformation, m_aZoomLevelNormalization, aControl ) )
            return false;

        m_pOutputDeviceForWindow = const_cast< OutputDevice * >( &_rDevice );
        m_aControl = std::move(aControl);
        m_xContainer.set(_rPageView.getControlContainer( _rDevice ), css::uno::UNO_QUERY);
        DBG_ASSERT( (   m_xContainer.is()                                           // either have a XControlContainer
                    ||  (   ( !_rPageView.getControlContainer( _rDevice ).is() )    // or don't have any container,
                        &&  ( _rDevice.GetOwnerWindow() == nullptr )  // which is allowed for non-Window instances only
                        )
                    ),
            "ViewObjectContactOfUnoControl_Impl::impl_ensureControl_nothrow: no XContainer at the ControlContainer!" );

        try
        {
            m_eControlDesignMode = m_aControl.isDesignMode() ? eDesign : eAlive;
            m_bControlIsVisible = m_aControl.isVisible();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }

        // start listening at all aspects of the control which are interesting to us ...
        impl_switchControlListening_nothrow( true );

        // start listening at the control container, in case somebody tampers with our control
        if ( m_xContainer.is() )
            impl_switchContainerListening_nothrow( true );

        return m_aControl.is();
    }


    bool ViewObjectContactOfUnoControl_Impl::createControlForDevice( IPageViewAccess const & _rPageView,
        const OutputDevice& _rDevice, const SdrUnoObj& _rUnoObject, const basegfx::B2DHomMatrix& _rInitialViewTransformation,
        const basegfx::B2DHomMatrix& _rInitialZoomNormalization, ControlHolder& _out_rControl )
    {
        _out_rControl.clear();

        const Reference< XControlModel >& xControlModel( _rUnoObject.GetUnoControlModel() );
        DBG_ASSERT( xControlModel.is(), "ViewObjectContactOfUnoControl_Impl::createControlForDevice: no control model at the SdrUnoObject!?" );
        if ( !xControlModel.is() )
            return false;

        bool bSuccess = false;
        try
        {
            const OUString& sControlServiceName( _rUnoObject.GetUnoControlTypeName() );

            const Reference< css::uno::XComponentContext >& xContext = ::comphelper::getProcessComponentContext();
            _out_rControl = Reference<XControl>( xContext->getServiceManager()->createInstanceWithContext(sControlServiceName, xContext), UNO_QUERY_THROW );

            // tdf#150886 for calc/writer/impress make forms ignore the platform theme
            Reference<XPropertySet> xModelProperties(xControlModel, UNO_QUERY);
            Reference<XPropertySetInfo> xInfo = xModelProperties ? xModelProperties->getPropertySetInfo() : nullptr;
            if (xInfo && xInfo->hasPropertyByName(u"StandardTheme"_ustr))
                xModelProperties->setPropertyValue(u"StandardTheme"_ustr, Any(!_rUnoObject.getSdrModelFromSdrObject().AreControlsThemed()));

            // knit the model and the control
            _out_rControl.setModel( xControlModel );
            const tools::Rectangle aRect( _rUnoObject.GetLogicRect() );

            // proper geometry
            UnoControlContactHelper::adjustControlGeometry_throw(
                _out_rControl,
                aRect,
                _rInitialViewTransformation,
                _rInitialZoomNormalization
            );

            // set design mode before peer is created,
            // this is also needed for accessibility
            _out_rControl.setDesignMode( _rPageView.isDesignMode() );

            // adjust the initial visibility according to the visibility of the layer
            impl_adjustControlVisibilityToLayerVisibility_throw( _out_rControl, _rUnoObject, _rPageView, false, true );

            // add the control to the respective control container
            // do this last
            Reference< XControlContainer > xControlContainer( _rPageView.getControlContainer( _rDevice ) );
            if ( xControlContainer.is() )
                xControlContainer->addControl( sControlServiceName, _out_rControl.getControl() );

            bSuccess = true;
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }

        if ( !bSuccess )
        {
            // delete the control which might have been created already
            UnoControlContactHelper::disposeAndClearControl_nothrow( _out_rControl );
        }

        return _out_rControl.is();
    }


    bool ViewObjectContactOfUnoControl_Impl::impl_getPageView_nothrow( SdrPageView*& _out_rpPageView )
    {
        OSL_PRECOND( !impl_isDisposed_nofail(), "ViewObjectContactOfUnoControl_Impl::impl_getPageView_nothrow: already disposed!" );

        _out_rpPageView = nullptr;
        if ( impl_isDisposed_nofail() )
            return false;

        ObjectContactOfPageView* pPageViewContact = dynamic_cast< ObjectContactOfPageView* >( &m_pAntiImpl->GetObjectContact() );
        if ( pPageViewContact )
            _out_rpPageView = &pPageViewContact->GetPageWindow().GetPageView();

        DBG_ASSERT( _out_rpPageView != nullptr, "ViewObjectContactOfUnoControl_Impl::impl_getPageView_nothrow: this method is expected to always have success!" );
        return ( _out_rpPageView != nullptr );
    }


    void ViewObjectContactOfUnoControl_Impl::impl_adjustControlVisibilityToLayerVisibility_throw()
    {
        OSL_PRECOND( m_aControl.is(),
            "ViewObjectContactOfUnoControl_Impl::impl_adjustControlVisibilityToLayerVisibility_throw: only valid if we have a control!" );

        SdrPageView* pPageView( nullptr );
        if ( !impl_getPageView_nothrow( pPageView ) )
            return;

        SdrUnoObj* pUnoObject = getUnoObject();
        if ( !pUnoObject )
            return;

        SdrPageViewAccess aPVAccess( *pPageView );
        impl_adjustControlVisibilityToLayerVisibility_throw( m_aControl, *pUnoObject, aPVAccess, m_bControlIsVisible, false/*_bForce*/ );
    }


    void ViewObjectContactOfUnoControl_Impl::impl_adjustControlVisibilityToLayerVisibility_throw( const ControlHolder& _rControl,
        const SdrUnoObj& _rUnoObject, IPageViewAccess const & _rPageView, bool _bIsCurrentlyVisible, bool _bForce )
    {
        // in design mode, there is no problem with the visibility: The XControl is hidden by
        // default, and the Drawing Layer will simply not call our paint routine, if we're in
        // a hidden layer. So, only alive mode matters.
        if ( !_rControl.isDesignMode() )
        {
            // the layer of our object
            SdrLayerID nObjectLayer = _rUnoObject.GetLayer();
            // is the object we're residing in visible in this view?
            bool bIsObjectVisible = _rUnoObject.IsVisible() && _rPageView.isLayerVisible( nObjectLayer );

            if ( _bForce || ( bIsObjectVisible != _bIsCurrentlyVisible ) )
            {
                _rControl.setVisible( bIsObjectVisible );
            }
        }
    }


    void ViewObjectContactOfUnoControl_Impl::impl_switchContainerListening_nothrow( bool _bStart )
    {
        OSL_PRECOND( m_xContainer.is(), "ViewObjectContactOfUnoControl_Impl::impl_switchContainerListening_nothrow: no control container!" );
        if ( !m_xContainer.is() )
            return;

        try
        {
            if ( _bStart )
                m_xContainer->addContainerListener( this );
            else
                m_xContainer->removeContainerListener( this );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }


    void ViewObjectContactOfUnoControl_Impl::impl_switchControlListening_nothrow( bool _bStart )
    {
        OSL_PRECOND( m_aControl.is(), "ViewObjectContactOfUnoControl_Impl::impl_switchControlListening_nothrow: invalid control!" );
        if ( !m_aControl.is() )
            return;

        try
        {
            // listen for visibility changes
            if ( _bStart )
                m_aControl.addWindowListener( this );
            else
                m_aControl.removeWindowListener( this );

            // in design mode, listen for some more aspects
            impl_switchDesignModeListening_nothrow( impl_isControlDesignMode_nothrow() && _bStart );

            // listen for design mode changes
            Reference< XModeChangeBroadcaster > xDesignModeChanges( m_aControl.getControl(), UNO_QUERY_THROW );
            if ( _bStart )
                xDesignModeChanges->addModeChangeListener( this );
            else
                xDesignModeChanges->removeModeChangeListener( this );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }


    void ViewObjectContactOfUnoControl_Impl::impl_switchDesignModeListening_nothrow( bool _bStart )
    {
        if ( m_bIsDesignModeListening != _bStart )
        {
            m_bIsDesignModeListening = _bStart;
            impl_switchPropertyListening_nothrow( _bStart );
        }
    }


    void ViewObjectContactOfUnoControl_Impl::impl_switchPropertyListening_nothrow( bool _bStart )
    {
        OSL_PRECOND( m_aControl.is(), "ViewObjectContactOfUnoControl_Impl::impl_switchPropertyListening_nothrow: no control!" );
        if ( !m_aControl.is() )
            return;

        try
        {
            Reference< XPropertySet > xModelProperties( m_aControl.getModel(), UNO_QUERY_THROW );
            if ( _bStart )
                xModelProperties->addPropertyChangeListener( OUString(), this );
            else
                xModelProperties->removePropertyChangeListener( OUString(), this );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }


    bool ViewObjectContactOfUnoControl_Impl::isPrintableControl() const
    {
        SdrUnoObj* pUnoObject = getUnoObject();
        if ( !pUnoObject )
            return false;

        bool bIsPrintable = false;
        try
        {
            Reference< XPropertySet > xModelProperties( pUnoObject->GetUnoControlModel(), UNO_QUERY_THROW );
            OSL_VERIFY( xModelProperties->getPropertyValue( u"Printable"_ustr ) >>= bIsPrintable );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
        return bIsPrintable;
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::disposing( const EventObject& Source )
    {
        SolarMutexGuard aSolarGuard;
            // some code below - in particular our disposal - might trigger actions which require the
            // SolarMutex. In particular, in our disposal, we remove ourself as listener from the control,
            // which alone needs the SolarMutex. Of course this - a removeFooListener needed the SolarMutex -
            // is the real bug. Toolkit really is infested with solar mutex usage ... :( #i82169#

        if ( !m_aControl.is() )
            return;

        if  (   ( m_aControl            == Source.Source )
            ||  ( m_aControl.getModel() == Source.Source )
            )
        {
            // the model or the control is dying ... hmm, not much sense in that we ourself continue
            // living
            impl_dispose_nothrow( false );
            return;
        }

        DBG_ASSERT( Source.Source == m_xContainer, "ViewObjectContactOfUnoControl_Impl::disposing: Who's this?" );
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::windowResized( const WindowEvent& /*e*/ )
    {
        // not interested in
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::windowMoved( const WindowEvent& /*e*/ )
    {
        // not interested in
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::windowShown( const EventObject& /*e*/ )
    {
        SolarMutexGuard aSolarGuard;
        m_bControlIsVisible = true;
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::windowHidden( const EventObject& /*e*/ )
    {
        SolarMutexGuard aSolarGuard;
        m_bControlIsVisible = false;
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::propertyChange( const PropertyChangeEvent& /*_rEvent*/ )
    {
        SolarMutexGuard aSolarGuard;
            // (re)painting might require VCL operations, which need the SolarMutex

        OSL_PRECOND( !impl_isDisposed_nofail(), "ViewObjectContactOfUnoControl_Impl::propertyChange: already disposed()" );
        if ( impl_isDisposed_nofail() )
            return;

        DBG_ASSERT( m_aControl.is(), "ViewObjectContactOfUnoControl_Impl::propertyChange: " );
        if ( !m_aControl.is() )
            return;

        // a generic property changed. If we're in design mode, we need to repaint the control
        if ( impl_isControlDesignMode_nothrow() )
        {
            m_pAntiImpl->propertyChange();
        }
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::modeChanged( const ModeChangeEvent& _rSource )
    {
        SolarMutexGuard aSolarGuard;

        DBG_ASSERT( _rSource.NewMode == "design" || _rSource.NewMode == "alive", "ViewObjectContactOfUnoControl_Impl::modeChanged: unexpected mode!" );

        m_eControlDesignMode = _rSource.NewMode == "design" ? eDesign : eAlive;

        impl_switchDesignModeListening_nothrow( impl_isControlDesignMode_nothrow() );

        try
        {
            // if the control is part of an invisible layer, we need to explicitly hide it in alive mode
            impl_adjustControlVisibilityToLayerVisibility_throw();
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::elementInserted( const ContainerEvent& /*_Event*/ )
    {
        // not interested in
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::elementRemoved( const ContainerEvent& Event )
    {
        SolarMutexGuard aSolarGuard;
            // some code below - in particular our disposal - might trigger actions which require the
            // SolarMutex. In particular, in our disposal, we remove ourself as listener from the control,
            // which alone needs the SolarMutex. Of course this - a removeFooListener needed the SolarMutex -
            // is the real bug. Toolkit really is infested with solar mutex usage ... :( #i82169#
        DBG_ASSERT( Event.Source == m_xContainer, "ViewObjectContactOfUnoControl_Impl::elementRemoved: where did this come from?" );

        if ( m_aControl == Event.Element )
            impl_dispose_nothrow( false );
    }


    void SAL_CALL ViewObjectContactOfUnoControl_Impl::elementReplaced( const ContainerEvent& Event )
    {
        SolarMutexGuard aSolarGuard;
        DBG_ASSERT( Event.Source == m_xContainer, "ViewObjectContactOfUnoControl_Impl::elementReplaced: where did this come from?" );

        if ( ! ( m_aControl == Event.ReplacedElement ) )
            return;

        Reference< XControl > xNewControl( Event.Element, UNO_QUERY );
        DBG_ASSERT( xNewControl.is(), "ViewObjectContactOfUnoControl_Impl::elementReplaced: invalid new control!" );
        if ( !xNewControl.is() )
            return;

        ENSURE_OR_THROW( m_pOutputDeviceForWindow, "calling this without /me having an output device should be impossible." );

        DBG_ASSERT( xNewControl->getModel() == m_aControl.getModel(), "ViewObjectContactOfUnoControl_Impl::elementReplaced: another model at the new control?" );
        // another model should - in the drawing layer - also imply another SdrUnoObj, which
        // should also result in new ViewContact, and thus in new ViewObjectContacts

        impl_switchControlListening_nothrow( false );

        ControlHolder aNewControl( xNewControl );
        aNewControl.setZoom( m_aControl.getZoom() );
        aNewControl.setPosSize( m_aControl.getPosSize() );
        aNewControl.setDesignMode( impl_isControlDesignMode_nothrow() );

        m_aControl = xNewControl;
        m_bControlIsVisible = m_aControl.isVisible();

        impl_switchControlListening_nothrow( true );

        m_pAntiImpl->onControlChangedOrModified( ViewObjectContactOfUnoControl::ImplAccess() );
    }


    void ViewObjectContactOfUnoControl_Impl::setControlDesignMode( bool _bDesignMode ) const
    {
        if ( ( m_eControlDesignMode != eUnknown ) && ( _bDesignMode == impl_isControlDesignMode_nothrow() ) )
            // nothing to do
            return;
        m_eControlDesignMode = _bDesignMode ? eDesign : eAlive;

        if ( !m_aControl.is() )
            // nothing to do, the setting will be respected as soon as the control
            // is created
            return;

        try
        {
            m_aControl.setDesignMode( _bDesignMode );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }


    //= LazyControlCreationPrimitive2D


    bool LazyControlCreationPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
    {
        if ( !BufferedDecompositionPrimitive2D::operator==( rPrimitive ) )
            return false;

        const LazyControlCreationPrimitive2D* pRHS = dynamic_cast< const LazyControlCreationPrimitive2D* >( &rPrimitive );
        if ( !pRHS )
            return false;

        if ( m_pVOCImpl != pRHS->m_pVOCImpl )
            return false;

        if ( m_aTransformation != pRHS->m_aTransformation )
            return false;

        return true;
    }


    void LazyControlCreationPrimitive2D::getTransformation( const ViewContactOfUnoControl& _rVOC, ::basegfx::B2DHomMatrix& _out_Transformation )
    {
        // Do use model data directly to create the correct geometry. Do NOT
        // use getBoundRect()/getSnapRect() here; these will use the sequence of
        // primitives themselves in the long run.
        const tools::Rectangle aSdrGeoData( _rVOC.GetSdrUnoObj().GetGeoRect() );
        const basegfx::B2DRange aRange = vcl::unotools::b2DRectangleFromRectangle(aSdrGeoData);

        _out_Transformation.identity();
        _out_Transformation.set( 0, 0, aRange.getWidth() );
        _out_Transformation.set( 1, 1, aRange.getHeight() );
        _out_Transformation.set( 0, 2, aRange.getMinX() );
        _out_Transformation.set( 1, 2, aRange.getMinY() );
    }


    ::basegfx::B2DRange LazyControlCreationPrimitive2D::getB2DRange( const ::drawinglayer::geometry::ViewInformation2D& /*rViewInformation*/ ) const
    {
        ::basegfx::B2DRange aRange( 0.0, 0.0, 1.0, 1.0 );
        aRange.transform( m_aTransformation );
        return aRange;
    }


    void LazyControlCreationPrimitive2D::get2DDecomposition( ::drawinglayer::primitive2d::Primitive2DDecompositionVisitor& rVisitor, const ::drawinglayer::geometry::ViewInformation2D& _rViewInformation ) const
    {
    #if OSL_DEBUG_LEVEL > 0
        ::basegfx::B2DVector aScale, aTranslate;
        double fRotate, fShearX;
        _rViewInformation.getObjectToViewTransformation().decompose( aScale, aTranslate, fRotate, fShearX );
    #endif
        if ( m_pVOCImpl->hasControl() )
            impl_positionAndZoomControl( _rViewInformation );
        BufferedDecompositionPrimitive2D::get2DDecomposition( rVisitor, _rViewInformation );
    }


    ::drawinglayer::primitive2d::Primitive2DReference LazyControlCreationPrimitive2D::create2DDecomposition( const ::drawinglayer::geometry::ViewInformation2D& _rViewInformation ) const
    {
    #if OSL_DEBUG_LEVEL > 0
        ::basegfx::B2DVector aScale, aTranslate;
        double fRotate, fShearX;
        _rViewInformation.getObjectToViewTransformation().decompose( aScale, aTranslate, fRotate, fShearX );
    #endif
        const bool bHadControl = m_pVOCImpl->getExistentControl().is();

        // force control here to make it a VCL ChildWindow. Will be fetched
        // and used below by getExistentControl()
        m_pVOCImpl->ensureControl( &_rViewInformation.getObjectToViewTransformation() );
        impl_positionAndZoomControl( _rViewInformation );

        // get needed data
        const ViewContactOfUnoControl& rViewContactOfUnoControl( m_pVOCImpl->getViewContact() );
        Reference< XControlModel > xControlModel( rViewContactOfUnoControl.GetSdrUnoObj().GetUnoControlModel() );
        const ControlHolder& rControl( m_pVOCImpl->getExistentControl() );

        if ( !bHadControl && rControl.is() && rControl.isVisible() )
            rControl.invalidate();

        // check if we already have an XControl.
        if ( !xControlModel.is() || !rControl.is() )
        {
            // use the default mechanism. This will create a ControlPrimitive2D without
            // handing over a XControl. If not even a XControlModel exists, it will
            // create the SdrObject fallback visualisation
            ::drawinglayer::primitive2d::Primitive2DContainer aContainer;
            rViewContactOfUnoControl.getViewIndependentPrimitive2DContainer(aContainer);
            return new drawinglayer::primitive2d::GroupPrimitive2D(std::move(aContainer));
        }

        SdrObject const& rSdrObj(m_pVOCImpl->getViewContact().GetSdrObject());
        void const* pAnchorKey(nullptr);
        if (auto const pUserCall = rSdrObj.GetUserCall())
        {
            pAnchorKey = pUserCall->GetPDFAnchorStructureElementKey(rSdrObj);
        }

        // create a primitive and hand over the existing xControl. This will
        // allow the primitive to not need to create another one on demand.
        return new ::drawinglayer::primitive2d::ControlPrimitive2D(
            m_aTransformation, xControlModel, rControl.getControl(),
            rSdrObj.GetTitle(), rSdrObj.GetDescription(), pAnchorKey);
    }

    sal_uInt32 LazyControlCreationPrimitive2D::getPrimitive2DID() const
    {
        return PRIMITIVE2D_ID_SDRCONTROLPRIMITIVE2D;
    }

    ViewObjectContactOfUnoControl::ViewObjectContactOfUnoControl( ObjectContact& _rObjectContact, ViewContactOfUnoControl& _rViewContact )
        :ViewObjectContactOfSdrObj( _rObjectContact, _rViewContact )
        ,m_pImpl( new ViewObjectContactOfUnoControl_Impl( this ) )
    {
    }


    ViewObjectContactOfUnoControl::~ViewObjectContactOfUnoControl()
    {
        m_pImpl->dispose();
        m_pImpl = nullptr;

    }


    Reference< XControl > ViewObjectContactOfUnoControl::getControl()
    {
        SolarMutexGuard aSolarGuard;
        m_pImpl->ensureControl( nullptr );
        return m_pImpl->getExistentControl().getControl();
    }


    Reference< XControl > ViewObjectContactOfUnoControl::getTemporaryControlForWindow(
        const vcl::Window& _rWindow, Reference< XControlContainer >& _inout_ControlContainer, const SdrUnoObj& _rUnoObject )
    {
        ControlHolder aControl;

        InvisibleControlViewAccess aSimulatePageView( _inout_ControlContainer );
        OSL_VERIFY( ViewObjectContactOfUnoControl_Impl::createControlForDevice( aSimulatePageView, *_rWindow.GetOutDev(), _rUnoObject,
            _rWindow.GetOutDev()->GetViewTransformation(), _rWindow.GetOutDev()->GetInverseViewTransformation(), aControl ) );
        return aControl.getControl();
    }


    void ViewObjectContactOfUnoControl::ensureControlVisibility( bool _bVisible ) const
    {
        SolarMutexGuard aSolarGuard;

        try
        {
            const ControlHolder& rControl( m_pImpl->getExistentControl() );
            if ( !rControl.is() )
                return;

            // only need to care for alive mode
            if ( rControl.isDesignMode() )
                return;

            // is the visibility correct?
            if ( m_pImpl->isControlVisible() == _bVisible )
                return;

            // no -> adjust it
            rControl.setVisible( _bVisible );
            DBG_ASSERT( m_pImpl->isControlVisible() == _bVisible, "ViewObjectContactOfUnoControl::ensureControlVisibility: this didn't work!" );
                // now this would mean that either isControlVisible is not reliable,
                // or that showing/hiding the window did not work as intended.
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }


    void ViewObjectContactOfUnoControl::setControlDesignMode( bool _bDesignMode ) const
    {
        SolarMutexGuard aSolarGuard;
        m_pImpl->setControlDesignMode( _bDesignMode );

        if(!_bDesignMode)
        {
            // when live mode is switched on, a refresh is needed. The edit mode visualisation
            // needs to be repainted and the now used VCL-Window needs to be positioned and
            // sized. Both is done from the repaint refresh.
            const_cast< ViewObjectContactOfUnoControl* >(this)->ActionChanged();
        }
    }


    void ViewObjectContactOfUnoControl::createPrimitive2DSequence(const DisplayInfo& /*rDisplayInfo*/, drawinglayer::primitive2d::Primitive2DDecompositionVisitor& rVisitor) const
    {
        if ( m_pImpl->isDisposed() )
            // our control already died.
            // TODO: Is it worth re-creating the control? Finally, this is a pathological situation, it means some instance
            // disposed the control though it doesn't own it. So, /me thinks we should not bother here.
            return;

        if ( GetObjectContact().getViewInformation2D().getViewTransformation().isIdentity() )
            // remove this when #i115754# is fixed
            return;

        // ignore existing controls which are in alive mode and manually switched to "invisible" #i102090#
        const ControlHolder& rControl( m_pImpl->getExistentControl() );
        if ( rControl.is() && !rControl.isDesignMode() && !rControl.isVisible() )
            return;

        rVisitor.visit( new LazyControlCreationPrimitive2D( m_pImpl ) );
    }


    bool ViewObjectContactOfUnoControl::isPrimitiveVisible( const DisplayInfo& _rDisplayInfo ) const
    {
        SolarMutexGuard aSolarGuard;

        if ( m_pImpl->hasControl() )
        {
            const ::drawinglayer::geometry::ViewInformation2D& rViewInformation( GetObjectContact().getViewInformation2D() );
        #if OSL_DEBUG_LEVEL > 0
            ::basegfx::B2DVector aScale, aTranslate;
            double fRotate, fShearX;
            rViewInformation.getObjectToViewTransformation().decompose( aScale, aTranslate, fRotate, fShearX );
        #endif

            if ( !rViewInformation.getViewport().isEmpty() )
            {
                // tdf#121963 check and eventually pre-multiply ViewTransformation
                // with GridOffset transformation to avoid alternating positions of
                // FormControls which are victims of the non-linear calc ViewTransformation
                // aka GridOffset. For other paths (e.g. repaint) this is included already
                // as part of the object's sequence of B2DPrimitive - representation
                // (see ViewObjectContact::getPrimitive2DSequence and how getGridOffset is used there)
                basegfx::B2DHomMatrix aViewTransformation(rViewInformation.getObjectToViewTransformation());

                if(GetObjectContact().supportsGridOffsets())
                {
                    const basegfx::B2DVector& rGridOffset(getGridOffset());

                    if(0.0 != rGridOffset.getX() || 0.0 != rGridOffset.getY())
                    {
                        // pre-multiply: GridOffset needs to be applied directly to logic model data
                        // of object coordinates, so multiply GridOffset from right to make it
                        // work as 1st change - these objects may still be part of groups/hierarchies
                        aViewTransformation = aViewTransformation * basegfx::utils::createTranslateB2DHomMatrix(rGridOffset);
                    }
                }

                m_pImpl->positionAndZoomControl(aViewTransformation);
            }
        }

        return ViewObjectContactOfSdrObj::isPrimitiveVisible( _rDisplayInfo );
    }


    void ViewObjectContactOfUnoControl::propertyChange()
    {
        impl_onControlChangedOrModified();
    }


    void ViewObjectContactOfUnoControl::ActionChanged()
    {
        // call parent
        ViewObjectContactOfSdrObj::ActionChanged();
        const ControlHolder& rControl(m_pImpl->getExistentControl());

        if(!rControl.is() || rControl.isDesignMode())
            return;

        // #i93180# if layer visibility has changed and control is in live mode, it is necessary
        // to correct visibility to make those control vanish on SdrObject LayerID changes
        const SdrPageView* pSdrPageView = GetObjectContact().TryToGetSdrPageView();

        if(pSdrPageView)
        {
            const SdrObject& rObject = getSdrObject();
            const bool bIsLayerVisible( rObject.IsVisible() && pSdrPageView->GetVisibleLayers().IsSet(rObject.GetLayer()));

            if(rControl.isVisible() != bIsLayerVisible)
            {
                rControl.setVisible(bIsLayerVisible);
            }
        }
    }


    void ViewObjectContactOfUnoControl::impl_onControlChangedOrModified()
    {
        // graphical invalidate at all views
        ActionChanged();

        // #i93318# flush Primitive2DContainer to force recreation with updated XControlModel
        // since e.g. background color has changed and existing decompositions are possibly no
        // longer valid. Unfortunately this is not detected from ControlPrimitive2D::operator==
        // since it only has a uno reference to the XControlModel
        flushPrimitive2DSequence();
    }

    UnoControlPrintOrPreviewContact::UnoControlPrintOrPreviewContact( ObjectContactOfPageView& _rObjectContact, ViewContactOfUnoControl& _rViewContact )
        :ViewObjectContactOfUnoControl( _rObjectContact, _rViewContact )
    {
    }


    UnoControlPrintOrPreviewContact::~UnoControlPrintOrPreviewContact()
    {
    }


    void UnoControlPrintOrPreviewContact::createPrimitive2DSequence(const DisplayInfo& rDisplayInfo, drawinglayer::primitive2d::Primitive2DDecompositionVisitor& rVisitor ) const
    {
        if ( !m_pImpl->isPrintableControl() )
            return;
        ViewObjectContactOfUnoControl::createPrimitive2DSequence( rDisplayInfo, rVisitor );
    }


} // namespace sdr::contact


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
