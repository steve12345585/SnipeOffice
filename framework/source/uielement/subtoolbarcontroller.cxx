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

#include <comphelper/propertysequence.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weakref.hxx>
#include <svtools/popupwindowcontroller.hxx>
#include <svtools/toolbarmenu.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <tools/gen.hxx>
#include <vcl/svapp.hxx>
#include <vcl/toolbox.hxx>
#include <vcl/commandinfoprovider.hxx>
#include <vcl/weldutils.hxx>

#include <com/sun/star/awt/XDockableWindow.hpp>
#include <com/sun/star/frame/XLayoutManager.hpp>
#include <com/sun/star/frame/XSubToolbarController.hpp>
#include <com/sun/star/frame/status/Visibility.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/ui/theUIElementFactoryManager.hpp>
#include <com/sun/star/container/NoSuchElementException.hpp>

typedef cppu::ImplInheritanceHelper< svt::PopupWindowController,
                                     css::frame::XSubToolbarController,
                                     css::awt::XDockableWindowListener> ToolBarBase;

namespace {

class SubToolBarController : public ToolBarBase
{
    OUString m_aSubTbName;
    OUString m_aLastCommand;
    css::uno::Reference< css::ui::XUIElement > m_xUIElement;
    void disposeUIElement();
public:
    explicit SubToolBarController( const rtl::Reference< css::uno::XComponentContext >& rxContext,
                                   const css::uno::Sequence< css::uno::Any >& rxArgs );
    virtual ~SubToolBarController() override;

    void PopoverDestroyed();

    // XInitialization
    virtual void SAL_CALL initialize( const css::uno::Sequence< css::uno::Any >& rxArgs ) override;

    // XStatusListener
    virtual void SAL_CALL statusChanged( const css::frame::FeatureStateEvent& Event ) override;

    // XToolbarController
    virtual void SAL_CALL execute( sal_Int16 nKeyModifier ) override;

    // PopupWindowController
    virtual VclPtr<vcl::Window> createVclPopupWindow(vcl::Window* pParent) override;
    virtual std::unique_ptr<WeldToolbarPopup> weldPopupWindow() override;

    // XSubToolbarController
    virtual sal_Bool SAL_CALL opensSubToolbar() override;
    virtual OUString SAL_CALL getSubToolbarName() override;
    virtual void SAL_CALL functionSelected( const OUString& rCommand ) override;
    virtual void SAL_CALL updateImage() override;

    //  XDockableWindowListener
    virtual void SAL_CALL startDocking( const css::awt::DockingEvent& e ) override;
    virtual css::awt::DockingData SAL_CALL docking( const css::awt::DockingEvent& e ) override;
    virtual void SAL_CALL endDocking( const css::awt::EndDockingEvent& e ) override;
    virtual sal_Bool SAL_CALL prepareToggleFloatingMode( const css::lang::EventObject& e ) override;
    virtual void SAL_CALL toggleFloatingMode( const css::lang::EventObject& e ) override;
    virtual void SAL_CALL closed( const css::lang::EventObject& e ) override;
    virtual void SAL_CALL endPopupMode( const css::awt::EndPopupModeEvent& e ) override;

    // XEventListener
    virtual void SAL_CALL disposing( const css::lang::EventObject& e ) override;

    // XComponent
    virtual void SAL_CALL dispose() override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& rServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
};

}

SubToolBarController::SubToolBarController(
    const rtl::Reference< css::uno::XComponentContext >& rxContext,
    const css::uno::Sequence< css::uno::Any >& rxArgs
)   : ToolBarBase(
        rxContext,
        rtl::Reference< css::frame::XFrame >(),
        ""
    )
{
    for ( css::uno::Any const & arg : rxArgs )
    {
        css::beans::PropertyValue aPropValue;
        arg >>= aPropValue;
        if ( aPropValue.Name == "Value" )
        {
            sal_Int32 nIdx{ 0 };
            OUString aValue;
            aPropValue.Value >>= aValue;
            m_aSubTbName = aValue.getToken(0, ';', nIdx);
            m_aCommandURL = m_aSubTbName;
            m_aLastCommand = aValue.getToken(0, ';', nIdx);
            break;
        }
    }
    if ( !m_aLastCommand.isEmpty() )
        addStatusListener( m_aLastCommand );
}

SubToolBarController::~SubToolBarController()
{
    disposeUIElement();
    m_xUIElement = nullptr;
}

void SubToolBarController::disposeUIElement()
{
    if ( m_xUIElement.is() )
    {
        css::uno::Reference< css::lang::XComponent > xComponent( m_xUIElement, css::uno::UNO_QUERY );
        xComponent->dispose();
    }
}

void SubToolBarController::statusChanged( const css::frame::FeatureStateEvent& Event )
{
    SolarMutexGuard aSolarMutexGuard;

    if ( m_bDisposed )
        return;

    ToolBox* pToolBox = nullptr;
    ToolBoxItemId nId;
    if ( !getToolboxId( nId, &pToolBox ) )
        return;

    ToolBoxItemBits nItemBits = pToolBox->GetItemBits( nId );
    nItemBits &= ~ToolBoxItemBits::CHECKABLE;
    TriState eTri = TRISTATE_FALSE;

    if ( Event.FeatureURL.Complete == m_aCommandURL )
    {
        pToolBox->EnableItem( nId, Event.IsEnabled );

        OUString aStrValue;
        css::frame::status::Visibility aItemVisibility;
        if ( Event.State >>= aStrValue )
        {
            // Enum command, such as the current custom shape,
            // toggle checked state.
            if ( m_aLastCommand == Concat2View( m_aCommandURL + "." + aStrValue ) )
            {
                eTri = TRISTATE_TRUE;
                nItemBits |= ToolBoxItemBits::CHECKABLE;
            }
        }
        else if ( Event.State >>= aItemVisibility )
        {
            pToolBox->ShowItem( nId, aItemVisibility.bVisible );
        }
    }
    else
    {
        bool bValue;
        if ( Event.State >>= bValue )
        {
            // Boolean, treat it as checked/unchecked
            if ( bValue )
                eTri = TRISTATE_TRUE;
            nItemBits |= ToolBoxItemBits::CHECKABLE;
        }
    }

    pToolBox->SetItemState( nId, eTri );
    pToolBox->SetItemBits( nId, nItemBits );
}

void SubToolBarController::execute( sal_Int16 nKeyModifier )
{
    if ( !m_aLastCommand.isEmpty() )
    {
        auto aArgs( comphelper::InitPropertySequence( {
            { "KeyModifier", css::uno::Any( nKeyModifier ) }
        } ) );
        dispatchCommand( m_aLastCommand, aArgs );
    }
}

namespace {
class SubToolbarControl final : public WeldToolbarPopup
{
public:
    explicit SubToolbarControl(SubToolBarController& rController, weld::Widget* pParent);
    virtual ~SubToolbarControl() override;

    virtual void GrabFocus() override;

    weld::Container* GetContainer() { return m_xTargetContainer.get(); }

private:
    SubToolBarController& m_rController;
    std::unique_ptr<weld::Container> m_xTargetContainer;
};
}

SubToolbarControl::SubToolbarControl(SubToolBarController& rController,
                                     weld::Widget* pParent)
    : WeldToolbarPopup(rController.getFrameInterface(), pParent, u"svt/ui/subtoolbar.ui"_ustr, u"subtoolbar"_ustr)
    , m_rController(rController)
    , m_xTargetContainer(m_xBuilder->weld_container(u"container"_ustr))
{
}

void SubToolbarControl::GrabFocus()
{
    // TODO
}

SubToolbarControl::~SubToolbarControl()
{
    m_rController.PopoverDestroyed();
}

std::unique_ptr<WeldToolbarPopup> SubToolBarController::weldPopupWindow()
{
    SolarMutexGuard aGuard;

    auto pPopup = std::make_unique<SubToolbarControl>(*this, m_pToolbar);

    css::uno::Reference< css::frame::XFrame > xFrame ( getFrameInterface() );

    // create element with factory
    static css::uno::WeakReference< css::ui::XUIElementFactoryManager > xWeakUIElementFactory;
    css::uno::Reference< css::ui::XUIElementFactoryManager > xUIElementFactory = xWeakUIElementFactory;
    if ( !xUIElementFactory.is() )
    {
        xUIElementFactory = css::ui::theUIElementFactoryManager::get( m_xContext );
        xWeakUIElementFactory = xUIElementFactory;
    }

    css::uno::Reference< css::awt::XWindow > xParent = new weld::TransportAsXWindow(pPopup->GetContainer());

    auto aPropSeq( comphelper::InitPropertySequence( {
        { "Frame", css::uno::Any( xFrame ) },
        { "ParentWindow", css::uno::Any( xParent ) },
        { "Persistent", css::uno::Any( false ) },
        { "PopupMode", css::uno::Any( true ) }
    } ) );

    try
    {
        m_xUIElement = xUIElementFactory->createUIElement( "private:resource/toolbar/" + m_aSubTbName, aPropSeq );
    }
    catch ( css::container::NoSuchElementException& )
    {}
    catch ( css::lang::IllegalArgumentException& )
    {}

    return pPopup;
}

VclPtr<vcl::Window> SubToolBarController::createVclPopupWindow(vcl::Window* /*pParent*/)
{
    SolarMutexGuard aGuard;

    ToolBox* pToolBox = nullptr;
    ToolBoxItemId nId;
    if ( getToolboxId( nId, &pToolBox ) )
    {
        css::uno::Reference< css::frame::XFrame > xFrame ( getFrameInterface() );

        // create element with factory
        static css::uno::WeakReference< css::ui::XUIElementFactoryManager > xWeakUIElementFactory;
        css::uno::Reference< css::ui::XUIElement > xUIElement;
        css::uno::Reference< css::ui::XUIElementFactoryManager > xUIElementFactory = xWeakUIElementFactory;
        if ( !xUIElementFactory.is() )
        {
            xUIElementFactory = css::ui::theUIElementFactoryManager::get( m_xContext );
            xWeakUIElementFactory = xUIElementFactory;
        }

        auto aPropSeq( comphelper::InitPropertySequence( {
            { "Frame", css::uno::Any( xFrame ) },
            { "ParentWindow", css::uno::Any( m_xParentWindow ) },
            { "Persistent", css::uno::Any( false ) },
            { "PopupMode", css::uno::Any( true ) }
        } ) );

        try
        {
            xUIElement = xUIElementFactory->createUIElement( "private:resource/toolbar/" + m_aSubTbName, aPropSeq );
        }
        catch ( css::container::NoSuchElementException& )
        {}
        catch ( css::lang::IllegalArgumentException& )
        {}

        if ( xUIElement.is() )
        {
            css::uno::Reference< css::awt::XWindow > xSubToolBar( xUIElement->getRealInterface(), css::uno::UNO_QUERY );
            if ( xSubToolBar.is() )
            {
                css::uno::Reference< css::awt::XDockableWindow > xDockWindow( xSubToolBar, css::uno::UNO_QUERY );
                xDockWindow->addDockableWindowListener( css::uno::Reference< css::awt::XDockableWindowListener >(this) );
                xDockWindow->enableDocking( true );

                // keep reference to UIElement to avoid its destruction
                disposeUIElement();
                m_xUIElement = std::move(xUIElement);

                VclPtr<vcl::Window> pTbxWindow = VCLUnoHelper::GetWindow( xSubToolBar );
                if ( pTbxWindow && pTbxWindow->GetType() == WindowType::TOOLBOX )
                {
                    ToolBox* pToolBar = static_cast< ToolBox* >( pTbxWindow.get() );
                    // calc and set size for popup mode
                    Size aSize = pToolBar->CalcPopupWindowSizePixel();
                    pToolBar->SetSizePixel( aSize );
                    // open subtoolbox in popup mode
                    vcl::Window::GetDockingManager()->StartPopupMode( pToolBox, pToolBar );
                }
            }
        }
    }
    return nullptr;
}

sal_Bool SubToolBarController::opensSubToolbar()
{
    return !m_aLastCommand.isEmpty();
}

OUString SubToolBarController::getSubToolbarName()
{
    return m_aSubTbName;
}

void SubToolBarController::functionSelected( const OUString& rCommand )
{
    if ( !m_aLastCommand.isEmpty() && m_aLastCommand != rCommand )
    {
        removeStatusListener( m_aLastCommand );
        m_aLastCommand = rCommand;
        addStatusListener( m_aLastCommand );
        updateImage();
    }
}

void SubToolBarController::updateImage()
{
    SolarMutexGuard aGuard;
    if ( !m_aLastCommand.isEmpty() )
    {
        ToolBox* pToolBox = nullptr;
        ToolBoxItemId nId;
        if ( getToolboxId( nId, &pToolBox ) )
        {
            vcl::ImageType eImageType = pToolBox->GetImageSize();
            Image aImage = vcl::CommandInfoProvider::GetImageForCommand(m_aLastCommand, getFrameInterface(), eImageType);
            if ( !!aImage )
                pToolBox->SetItemImage( nId, aImage );
        }
    }
}

void SubToolBarController::startDocking( const css::awt::DockingEvent& )
{
}

css::awt::DockingData SubToolBarController::docking( const css::awt::DockingEvent& )
{
    return css::awt::DockingData();
}

void SubToolBarController::endDocking( const css::awt::EndDockingEvent& )
{
}

sal_Bool SubToolBarController::prepareToggleFloatingMode( const css::lang::EventObject& )
{
    return false;
}

void SubToolBarController::toggleFloatingMode( const css::lang::EventObject& )
{
}

void SubToolBarController::closed( const css::lang::EventObject& )
{
}

void SubToolBarController::endPopupMode( const css::awt::EndPopupModeEvent& e )
{
    SolarMutexGuard aGuard;

    OUString aSubToolBarResName;
    if ( m_xUIElement.is() )
    {
        css::uno::Reference< css::beans::XPropertySet > xPropSet( m_xUIElement, css::uno::UNO_QUERY );
        if ( xPropSet.is() )
        {
            try
            {
                xPropSet->getPropertyValue(u"ResourceURL"_ustr) >>= aSubToolBarResName;
            }
            catch ( css::beans::UnknownPropertyException& )
            {}
            catch ( css::lang::WrappedTargetException& )
            {}
        }
        disposeUIElement();
    }
    m_xUIElement = nullptr;

    // if the toolbar was teared-off recreate it and place it at the given position
    if( !e.bTearoff )
        return;

    css::uno::Reference< css::ui::XUIElement > xUIElement;
    css::uno::Reference< css::frame::XLayoutManager > xLayoutManager = getLayoutManager();

    if ( !xLayoutManager.is() )
        return;

    xLayoutManager->createElement( aSubToolBarResName );
    xUIElement = xLayoutManager->getElement( aSubToolBarResName );
    if ( !xUIElement.is() )
        return;

    css::uno::Reference< css::awt::XWindow > xSubToolBar( xUIElement->getRealInterface(), css::uno::UNO_QUERY );
    css::uno::Reference< css::beans::XPropertySet > xProp( xUIElement, css::uno::UNO_QUERY );
    if ( !(xSubToolBar.is() && xProp.is()) )
        return;

    try
    {
        VclPtr<vcl::Window> pTbxWindow = VCLUnoHelper::GetWindow( xSubToolBar );
        if ( pTbxWindow && pTbxWindow->GetType() == WindowType::TOOLBOX )
        {
            OUString aPersistentString( u"Persistent"_ustr );
            css::uno::Any a = xProp->getPropertyValue( aPersistentString );
            xProp->setPropertyValue( aPersistentString, css::uno::Any( false ) );

            xLayoutManager->hideElement( aSubToolBarResName );
            xLayoutManager->floatWindow( aSubToolBarResName );

            xLayoutManager->setElementPos( aSubToolBarResName, e.FloatingPosition );
            xLayoutManager->showElement( aSubToolBarResName );

            xProp->setPropertyValue(u"Persistent"_ustr, a );
        }
    }
    catch ( css::uno::RuntimeException& )
    {
        throw;
    }
    catch ( css::uno::Exception& )
    {}
}

void SubToolBarController::disposing( const css::lang::EventObject& e )
{
    svt::ToolboxController::disposing( e );
}

void SubToolBarController::initialize( const css::uno::Sequence< css::uno::Any >& rxArgs )
{
    svt::PopupWindowController::initialize( rxArgs );

    ToolBox* pToolBox = nullptr;
    ToolBoxItemId nId;
    if ( getToolboxId( nId, &pToolBox ) )
    {
        if ( m_aLastCommand.isEmpty() )
            pToolBox->SetItemBits( nId, pToolBox->GetItemBits( nId ) | ToolBoxItemBits::DROPDOWNONLY );
        else
            pToolBox->SetItemBits( nId, pToolBox->GetItemBits( nId ) | ToolBoxItemBits::DROPDOWN );
    }

    if (m_pToolbar)
    {
        mxPopoverContainer.reset(new ToolbarPopupContainer(m_pToolbar));
        m_pToolbar->set_item_popover(m_aCommandURL, mxPopoverContainer->getTopLevel());
    }

    updateImage();
}

void SubToolBarController::PopoverDestroyed()
{
    disposeUIElement();
    m_xUIElement = nullptr;
}

void SubToolBarController::dispose()
{
    if ( m_bDisposed )
        return;

    svt::PopupWindowController::dispose();
    disposeUIElement();
    m_xUIElement = nullptr;
}

OUString SubToolBarController::getImplementationName()
{
    return u"com.sun.star.comp.framework.SubToolBarController"_ustr;
}

sal_Bool SubToolBarController::supportsService( const OUString& rServiceName )
{
    return cppu::supportsService( this, rServiceName );
}

css::uno::Sequence< OUString > SubToolBarController::getSupportedServiceNames()
{
    return {u"com.sun.star.frame.ToolbarController"_ustr};
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_framework_SubToolBarController_get_implementation(
    css::uno::XComponentContext* rxContext,
    css::uno::Sequence<css::uno::Any> const & rxArgs )
{
    return cppu::acquire( new SubToolBarController( rxContext, rxArgs ) );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
