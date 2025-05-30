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

#include <string_view>
#include <vector>

#include <uiconfiguration/globalsettings.hxx>
#include <framework/addonsoptions.hxx>
#include <uielement/uielement.hxx>
#include <services/layoutmanager.hxx>

#include <com/sun/star/ui/XUIConfigurationManager.hpp>
#include <com/sun/star/awt/XWindowListener.hpp>
#include <com/sun/star/ui/XUIElementFactory.hpp>
#include <com/sun/star/ui/DockingArea.hpp>
#include <com/sun/star/awt/XWindow2.hpp>
#include <com/sun/star/awt/XDockableWindow.hpp>
#include <com/sun/star/awt/XDockableWindowListener.hpp>

#include <cppuhelper/implbase.hxx>

class VclSimpleEvent;

namespace framework
{

class ToolbarLayoutManager : public ::cppu::WeakImplHelper< css::awt::XDockableWindowListener,
                                                            css::ui::XUIConfigurationListener,
                                                            css::awt::XWindowListener >
{
    public:
        enum { DOCKINGAREAS_COUNT = 4 };

        enum PreviewFrameDetection
        {
            PREVIEWFRAME_UNKNOWN,
            PREVIEWFRAME_NO,
            PREVIEWFRAME_YES
        };

        ToolbarLayoutManager( css::uno::Reference< css::uno::XComponentContext > xContext,
                              css::uno::Reference< css::ui::XUIElementFactory >  xUIElementFactory,
                              LayoutManager* pParentLayouter );
        virtual ~ToolbarLayoutManager() override;

        void reset();
        void attach( const css::uno::Reference< css::frame::XFrame >& xFrame,
                     const css::uno::Reference< css::ui::XUIConfigurationManager >& xModuleCfgMgr,
                     const css::uno::Reference< css::ui::XUIConfigurationManager >& xDocCfgMgr,
                     const css::uno::Reference< css::container::XNameAccess >& xPersistentWindowState );

        void setParentWindow( const css::uno::Reference< css::awt::XVclWindowPeer >& xParentWindow );
        void setDockingAreaOffsets(const ::tools::Rectangle& rOffsets);

        void resetDockingArea();

        css::awt::Rectangle getDockingArea();
        void setDockingArea( const css::awt::Rectangle& rDockingArea );

        bool isPreviewFrame();

        // layouting
        bool isLayoutDirty() const { return m_bLayoutDirty;}
        void doLayout(const ::Size& aContainerSize);

        // creation/destruction
        void createStaticToolbars();
        void destroyToolbars();

        bool requestToolbar( const OUString& rResourceURL );
        bool createToolbar( const OUString& rResourceURL );
        bool destroyToolbar( std::u16string_view rResourceURL );

        // visibility
        bool showToolbar( std::u16string_view rResourceURL );
        bool hideToolbar( std::u16string_view rResourceURL );

        void refreshToolbarsVisibility( bool bAutomaticToolbars );
        void setFloatingToolbarsVisibility( bool bVisible );
        void setVisible(bool bVisible);

        // docking and further functions
        bool dockToolbar( std::u16string_view rResourceURL, css::ui::DockingArea eDockingArea, const css::awt::Point& aPos );
        bool dockAllToolbars();
        bool floatToolbar( std::u16string_view rResourceURL );
        bool lockToolbar( std::u16string_view rResourceURL );
        bool unlockToolbar( std::u16string_view rResourceURL );
        void setToolbarPos( std::u16string_view rResourceURL, const css::awt::Point& aPos );
        void setToolbarSize( std::u16string_view rResourceURL, const css::awt::Size& aSize );
        void setToolbarPosSize( std::u16string_view rResourceURL, const css::awt::Point& aPos, const css::awt::Size& aSize );
        bool isToolbarVisible( std::u16string_view rResourceURL );
        bool isToolbarFloating( std::u16string_view rResourceURL );
        bool isToolbarDocked( std::u16string_view rResourceURL );
        bool isToolbarLocked( std::u16string_view rResourceURL );
        css::awt::Point getToolbarPos( std::u16string_view rResourceURL );
        css::awt::Size getToolbarSize( std::u16string_view rResourceURL );
        css::uno::Reference< css::ui::XUIElement > getToolbar( std::u16string_view aName );
        css::uno::Sequence< css::uno::Reference< css::ui::XUIElement > > getToolbars();

        void updateToolbarsTips();

        // child window notifications
        void childWindowEvent( VclSimpleEvent const * pEvent );

        // XInterface

        virtual void SAL_CALL acquire() noexcept override;
        virtual void SAL_CALL release() noexcept override;
        virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;

        // XEventListener
        virtual void SAL_CALL disposing( const css::lang::EventObject& aEvent ) override;

        // XWindowListener
        virtual void SAL_CALL windowResized( const css::awt::WindowEvent& aEvent ) override;
        virtual void SAL_CALL windowMoved( const css::awt::WindowEvent& aEvent ) override;
        virtual void SAL_CALL windowShown( const css::lang::EventObject& aEvent ) override;
        virtual void SAL_CALL windowHidden( const css::lang::EventObject& aEvent ) override;

        // XDockableWindowListener
        virtual void SAL_CALL startDocking( const css::awt::DockingEvent& e ) override;
        virtual css::awt::DockingData SAL_CALL docking( const css::awt::DockingEvent& e ) override;
        virtual void SAL_CALL endDocking( const css::awt::EndDockingEvent& e ) override;
        virtual sal_Bool SAL_CALL prepareToggleFloatingMode( const css::lang::EventObject& e ) override;
        virtual void SAL_CALL toggleFloatingMode( const css::lang::EventObject& e ) override;
        virtual void SAL_CALL closed( const css::lang::EventObject& e ) override;
        virtual void SAL_CALL endPopupMode( const css::awt::EndPopupModeEvent& e ) override;

        // XUIConfigurationListener
        virtual void SAL_CALL elementInserted( const css::ui::ConfigurationEvent& Event ) override;
        virtual void SAL_CALL elementRemoved( const css::ui::ConfigurationEvent& Event ) override;
        virtual void SAL_CALL elementReplaced( const css::ui::ConfigurationEvent& Event ) override;

    private:
        enum DockingOperation
        {
            DOCKOP_BEFORE_COLROW,
            DOCKOP_ON_COLROW,
            DOCKOP_AFTER_COLROW
        };

        typedef std::vector< UIElement > UIElementVector;
        struct SingleRowColumnWindowData
        {
            SingleRowColumnWindowData()
                : nVarSize(0)
                , nStaticSize(0)
                , nSpace(0)
                , nRowColumn(0)
            {}

            std::vector< OUString >                                   aUIElementNames;
            std::vector< css::uno::Reference< css::awt::XWindow > >   aRowColumnWindows;
            std::vector< css::awt::Rectangle >                        aRowColumnWindowSizes;
            std::vector< sal_Int32 >                                  aRowColumnSpace;
            css::awt::Rectangle                                       aRowColumnRect;
            sal_Int32                                                 nVarSize;
            sal_Int32                                                 nStaticSize;
            sal_Int32                                                 nSpace;
            sal_Int32                                                 nRowColumn;
        };

        // internal helper methods

        bool             implts_isParentWindowVisible() const;
        ::tools::Rectangle      implts_calcDockingArea();
        void             implts_sortUIElements();
        void             implts_reparentToolbars();
        static OUString  implts_generateGenericAddonToolbarTitle( sal_Int32 nNumber );
        void             implts_setElementData( UIElement& rUIElement, const css::uno::Reference< css::awt::XDockableWindow >& rDockWindow );
        void             implts_destroyDockingAreaWindows();

        // layout methods

        void             implts_setDockingAreaWindowSizes( const css::awt::Rectangle& rBorderSpace );
        css::awt::Point  implts_findNextCascadeFloatingPos();
        void             implts_renumberRowColumnData( css::ui::DockingArea eDockingArea, const UIElement& rUIElement );
        void             implts_calcWindowPosSizeOnSingleRowColumn( sal_Int32 nDockingArea,
                                                                    sal_Int32 nOffset,
                                                                    SingleRowColumnWindowData& rRowColumnWindowData,
                                                                    const ::Size& rContainerSize );
        void             implts_setLayoutDirty();
        void             implts_setLayoutInProgress( bool bInProgress = true );

        // lookup/container methods

        UIElement        implts_findToolbar( std::u16string_view aName );
        UIElement        implts_findToolbar( const css::uno::Reference< css::uno::XInterface >& xToolbar );
        UIElement&       impl_findToolbar( std::u16string_view aName );
        css::uno::Reference< css::awt::XWindow > implts_getXWindow( std::u16string_view aName );
        vcl::Window*     implts_getWindow( std::u16string_view aName );
        bool             implts_insertToolbar( const UIElement& rUIElement );
        void             implts_setToolbar( const UIElement& rUIElement );
        ::Size           implts_getTopBottomDockingAreaSizes();
        void             implts_getUIElementVectorCopy( UIElementVector& rCopy );

        // internal docking methods

        static ::tools::Rectangle implts_calcHotZoneRect( const ::tools::Rectangle& rRect, sal_Int32 nHotZoneOffset );
        void             implts_calcDockingPosSize( UIElement& aUIElement, DockingOperation& eDockOperation, ::tools::Rectangle& rTrackingRect, const Point& rMousePos );
        static DockingOperation implts_determineDockingOperation( css::ui::DockingArea DockingArea, const ::tools::Rectangle& rRowColRect, const Point& rMousePos );
        ::tools::Rectangle      implts_getWindowRectFromRowColumn( css::ui::DockingArea DockingArea, const SingleRowColumnWindowData& rRowColumnWindowData, const ::Point& rMousePos, std::u16string_view rExcludeElementName );
        ::tools::Rectangle      implts_determineFrontDockingRect( css::ui::DockingArea eDockingArea,
                                                           sal_Int32 nRowCol,
                                                           const ::tools::Rectangle& rDockedElementRect,
                                                           std::u16string_view rMovedElementName,
                                                           const ::tools::Rectangle& rMovedElementRect );
        ::tools::Rectangle      implts_calcTrackingAndElementRect( css::ui::DockingArea eDockingArea,
                                                            sal_Int32 nRowCol,
                                                            UIElement& rUIElement,
                                                            const ::tools::Rectangle& rTrackingRect,
                                                            const ::tools::Rectangle& rRowColumnRect,
                                                            const ::Size& rContainerWinSize );

        void             implts_getDockingAreaElementInfos( css::ui::DockingArea DockingArea, std::vector< SingleRowColumnWindowData >& rRowColumnsWindowData );
        void             implts_getDockingAreaElementInfoOnSingleRowCol( css::ui::DockingArea, sal_Int32 nRowCol, SingleRowColumnWindowData& rRowColumnWindowData );
        void             implts_findNextDockingPos( css::ui::DockingArea DockingArea, const ::Size& aUIElementSize, css::awt::Point& rVirtualPos, ::Point& rPixelPos );
        static void      implts_setTrackingRect( css::ui::DockingArea eDockingArea, const ::Point& rMousePos, ::tools::Rectangle& rTrackingRect );

        // creation methods

        void             implts_createAddonsToolBars();
        void             implts_createCustomToolBars();
        void             implts_createNonContextSensitiveToolBars();
        void             implts_createCustomToolBars( const css::uno::Sequence< css::uno::Sequence< css::beans::PropertyValue > >& aCustomTbxSeq );
        void             implts_createCustomToolBar( const OUString& aTbxResName, const OUString& aTitle );
        void             implts_setToolbarCreation( bool bStart = true );
        bool             implts_isToolbarCreationActive();

        // persistence methods

        bool         implts_readWindowStateData( const OUString& aName, UIElement& rElementData );
        void             implts_writeWindowStateData( const UIElement& rElementData );

        // members

        css::uno::Reference< css::uno::XComponentContext >                   m_xContext;
        css::uno::Reference< css::frame::XFrame >                            m_xFrame;
        css::uno::Reference< css::awt::XWindow2 >                            m_xContainerWindow;
        css::uno::Reference< css::awt::XWindow >                             m_xDockAreaWindows[DOCKINGAREAS_COUNT];
        css::uno::Reference< css::ui::XUIElementFactory >                    m_xUIElementFactoryManager;
        css::uno::Reference< css::ui::XUIConfigurationManager >              m_xModuleCfgMgr;
        css::uno::Reference< css::ui::XUIConfigurationManager >              m_xDocCfgMgr;
        css::uno::Reference< css::container::XNameAccess >                   m_xPersistentWindowState;
        LayoutManager*                                                       m_pParentLayouter;

        UIElementVector                                                      m_aUIElements;
        UIElement                                                            m_aDockUIElement;
        tools::Rectangle                                                            m_aDockingArea;
        tools::Rectangle                                                            m_aDockingAreaOffsets;
        DockingOperation                                                     m_eDockOperation;
        PreviewFrameDetection                                                m_ePreviewDetection;

        std::unique_ptr<AddonsOptions>                                       m_pAddonOptions;
        std::unique_ptr<GlobalSettings>                                      m_pGlobalSettings;

        bool                                                                 m_bComponentAttached;
        bool                                                                 m_bLayoutDirty;
        bool                                                                 m_bGlobalSettings;
        bool                                                                 m_bDockingInProgress;
        bool                                                                 m_bLayoutInProgress;
        bool                                                                 m_bToolbarCreation;
};

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
