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

#include <memory>
#include <sal/config.h>

#include <sfx2/sidebar/AsynchronousCall.hxx>
#include <sfx2/sidebar/Context.hxx>
#include <sfx2/sidebar/Deck.hxx>
#include <sfx2/sidebar/FocusManager.hxx>
#include <sfx2/sidebar/ResourceManager.hxx>
#include <sfx2/sidebar/TabBar.hxx>
#include <sfx2/viewfrm.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <com/sun/star/frame/XStatusListener.hpp>
#include <com/sun/star/frame/XFrameActionListener.hpp>
#include <com/sun/star/ui/XContextChangeEventListener.hpp>
#include <com/sun/star/ui/XSidebar.hpp>

#include <optional>
#include <comphelper/compbase.hxx>

namespace com::sun::star::awt { class XWindow; }
namespace com::sun::star::frame { class XDispatch; }
namespace com::sun::star::ui { class XUIElement; }

typedef comphelper::WeakComponentImplHelper <
    css::ui::XContextChangeEventListener,
    css::beans::XPropertyChangeListener,
    css::ui::XSidebar,
    css::frame::XStatusListener,
    css::frame::XFrameActionListener
    > SidebarControllerInterfaceBase;

class SfxSplitWindow;
class SfxViewShell;

namespace sfx2::sidebar {

class DeckDescriptor;
class SidebarDockingWindow;

class SFX2_DLLPUBLIC SidebarController final
    : public SidebarControllerInterfaceBase
{
public:
    static rtl::Reference<SidebarController> create(SidebarDockingWindow* pParentWindow,
                                                    const SfxViewFrame* pViewFrame);
    virtual ~SidebarController() override;
    SidebarController(const SidebarController&) = delete;
    SidebarController& operator=( const SidebarController& ) = delete;

    /** Return the SidebarController object that is associated with
        the given XFrame.
        @return
            When there is no SidebarController object for the given
            XFrame then <NULL/> is returned.
    */
    static SidebarController* GetSidebarControllerForFrame (
        const css::uno::Reference<css::frame::XFrame>& rxFrame);

    void registerSidebarForFrame(const css::uno::Reference<css::frame::XController>& xFrame);

    void unregisterSidebarForFrame(const css::uno::Reference<css::frame::XController>& xFrame);

    // ui::XContextChangeEventListener
    virtual void SAL_CALL notifyContextChangeEvent (const css::ui::ContextChangeEventObject& rEvent) override;

    // XEventListener
    virtual void SAL_CALL disposing (const css::lang::EventObject& rEventObject) override;

    // beans::XPropertyChangeListener
    virtual void SAL_CALL propertyChange (const css::beans::PropertyChangeEvent& rEvent) override;

    // frame::XStatusListener
    virtual void SAL_CALL statusChanged (const css::frame::FeatureStateEvent& rEvent) override;

    // frame::XFrameActionListener
    virtual void SAL_CALL frameAction (const css::frame::FrameActionEvent& rEvent) override;

    // ui::XSidebar
    virtual void SAL_CALL requestLayout() override;

    void NotifyResize();

    /** In some situations it is necessary to force an update of the
        current deck and its panels.  One reason is a change of the
        view scale.  Some panels can handle this only when
        constructed.  In this case we have to a context change and
        also force that all panels are destroyed and created new.
    */
    const static sal_Int32 SwitchFlag_NoForce = 0x00;
    const static sal_Int32 SwitchFlag_ForceSwitch = 0x01;
    const static sal_Int32 SwitchFlag_ForceNewDeck = 0x02;
    const static sal_Int32 SwitchFlag_ForceNewPanels = 0x02;

    bool IsDocked() const;

    void OpenThenSwitchToDeck(std::u16string_view rsDeckId);
    void OpenThenToggleDeck(const OUString& rsDeckId);

    /** Show only the tab bar, not the deck.
    */
    void RequestCloseDeck();

    /** Open the deck area and restore the parent window to its old width.
    */
    void RequestOpenDeck();

    /** Returns true when the given deck is the currently visible deck
     */
    bool IsDeckVisible(std::u16string_view rsDeckId);

    bool IsDeckOpen(const sal_Int32 nIndex = -1);

    FocusManager& GetFocusManager() { return maFocusManager;}

    ResourceManager* GetResourceManager() { return mpResourceManager.get();}

   // std::unique_ptr<ResourceManager> GetResourceManager() { return mpResourceManager;}

    const Context& GetCurrentContext() const { return maCurrentContext;}
    bool IsDocumentReadOnly (void) const { return mbIsDocumentReadOnly;}

    void SwitchToDeck(std::u16string_view rsDeckId);
    void SwitchToDefaultDeck();
    bool WasFloatingDeckClosed() const { return mbFloatingDeckClosed; }
    void SetFloatingDeckClosed(bool bWasClosed) { mbFloatingDeckClosed = bWasClosed; }

    void CreateDeck(std::u16string_view rDeckId);
    void CreateDeck(std::u16string_view rDeckId, const Context& rContext, bool bForceCreate = false);

    ResourceManager::DeckContextDescriptorContainer GetMatchingDecks();
    ResourceManager::PanelContextDescriptorContainer GetMatchingPanels(std::u16string_view rDeckId);

    void notifyDeckTitle(std::u16string_view targetDeckId);

    void updateModel(const css::uno::Reference<css::frame::XModel>& xModel);

    void disposeDecks();

    void FadeIn();
    void FadeOut();

    tools::Rectangle GetDeckDragArea() const;

    css::uno::Reference<css::frame::XFrame> const & getXFrame() const {return mxFrame;}

    sal_Int32 getMaximumWidth() const { return mnMaximumSidebarWidth; }
    void setMaximumWidth(sal_Int32 nMaximumWidth) { mnMaximumSidebarWidth = nMaximumWidth; }

    void saveDeckState();

    void SyncUpdate();

    // Used to avoid wrong context update when an embedded object activation is in progress
    bool hasChartOrMathContextCurrently() const;

    static SidebarController* GetSidebarControllerForView(const SfxViewShell* pViewShell);

private:
    SidebarController(SidebarDockingWindow* pParentWindow, const SfxViewFrame* pViewFrame);

    VclPtr<Deck> mpCurrentDeck;
    VclPtr<SidebarDockingWindow> mpParentWindow;
    const SfxViewFrame* mpViewFrame;
    css::uno::Reference<css::frame::XFrame> mxFrame;
    VclPtr<TabBar> mpTabBar;
    Context maCurrentContext;
    Context maRequestedContext;
    css::uno::Reference<css::frame::XController> mxCurrentController;
    /// Use a combination of SwitchFlag_* as value.
    sal_Int32 mnRequestedForceFlags;
    sal_Int32 mnMaximumSidebarWidth;
    bool mbMinimumSidebarWidth;
    OUString msCurrentDeckId;
    AsynchronousCall maPropertyChangeForwarder;
    AsynchronousCall maContextChangeUpdate;
    css::uno::Reference<css::beans::XPropertySet> mxThemePropertySet;

    /** Two flags control whether the deck is displayed or if only the
        tab bar remains visible.
        The mbIsDeckOpen flag stores the current state while
        mbIsDeckRequestedOpen stores how this state should be.  User
        actions like clicking on the deck closer affect the
        mbIsDeckRequestedOpen.  Normally both flags have the same
        value.  A document being read-only can prevent the deck from opening.
    */
    ::std::optional<bool> mbIsDeckRequestedOpen;
    ::std::optional<bool> mbIsDeckOpen;

    bool mbFloatingDeckClosed;

    /** Before the deck is closed the sidebar width is saved into this variable,
        so that it can be restored when the deck is reopened.
    */
    sal_Int32 mnSavedSidebarWidth;
    FocusManager maFocusManager;
    css::uno::Reference<css::frame::XDispatch> mxReadOnlyModeDispatch;
    bool mbIsDocumentReadOnly;
    VclPtr<SfxSplitWindow> mpSplitWindow;
    /** When the user moves the splitter then we remember the
        width at that time.
    */
    sal_Int32 mnWidthOnSplitterButtonDown;
    /** Control that is temporarily used as replacement for the deck
        to indicate that when the current mouse drag operation ends, the
        sidebar will only show the tab bar.
    */
    VclPtr<vcl::Window> mpCloseIndicator;

    DECL_DLLPRIVATE_LINK(WindowEventHandler, VclWindowEvent&, void);
    /** Make maRequestedContext the current context.
    */
    void UpdateConfigurations();

    css::uno::Reference<css::ui::XUIElement> CreateUIElement (
        const css::uno::Reference<css::awt::XWindow>& rxWindow,
        const OUString& rsImplementationURL,
        const bool bWantsCanvas,
        const Context& rContext);

    void CreatePanels(
        std::u16string_view rDeckId,
        const Context& rContext);
    std::shared_ptr<Panel> CreatePanel (
        std::u16string_view rsPanelId,
        weld::Widget* pParentWindow,
        const bool bIsInitiallyExpanded,
        const Context& rContext,
        const VclPtr<Deck>& pDeck);

    void SwitchToDeck (
        const DeckDescriptor& rDeckDescriptor,
        const Context& rContext);

    void ConnectMenuActivateHandlers(weld::Menu& rMainMenu, weld::Menu& rSubMenu) const;

    DECL_DLLPRIVATE_LINK(OnMenuItemSelected, const OUString&, void);
    DECL_DLLPRIVATE_LINK(OnSubMenuItemSelected, const OUString&, void);
    void BroadcastPropertyChange();

    /** The close of the deck changes the width of the child window.
        That is only possible if there is no other docking window docked above or below the sidebar.
        Return whether the width of the child window can be modified.
    */
    bool CanModifyChildWindowWidth();

    /** Set the child window container to a new width.
        Return the old width.
    */
    sal_Int32 SetChildWindowWidth (const sal_Int32 nNewWidth);

    /** Update the icons displayed in the title bars of the deck and
        the panels.  This is called once when a deck is created and
        every time when a data change event is processed.
    */
    void UpdateTitleBarIcons();

    void UpdateDeckOpenState();
    void RestrictWidth (sal_Int32 nWidth);
    SfxSplitWindow* GetSplitWindow();
    void ProcessNewWidth (const sal_Int32 nNewWidth);
    void UpdateCloseIndicator (const bool bIsIndicatorVisible);

    /** Typically called when a panel is focused via keyboard.
        Tries to scroll the deck up or down to make the given panel
        completely visible.
    */
    void ShowPanel (const Panel& rPanel);

    virtual void disposing(std::unique_lock<std::mutex>&) override;

    std::unique_ptr<ResourceManager> mpResourceManager;

};

} // end of namespace sfx2::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
