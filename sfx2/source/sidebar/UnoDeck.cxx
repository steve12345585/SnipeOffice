/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#include <sidebar/UnoDeck.hxx>

#include <sidebar/UnoPanels.hxx>

#include <sfx2/sidebar/ResourceManager.hxx>
#include <sfx2/sidebar/SidebarController.hxx>
#include <sidebar/DeckTitleBar.hxx>
#include <sfx2/sidebar/Deck.hxx>
#include <sidebar/DeckDescriptor.hxx>

#include <utility>
#include <vcl/svapp.hxx>

using namespace css;
using namespace ::sfx2::sidebar;

SfxUnoDeck::SfxUnoDeck(uno::Reference<frame::XFrame> _xFrame, OUString deckId):
xFrame(std::move(_xFrame)),
mDeckId(std::move(deckId))
{

}
SidebarController* SfxUnoDeck::getSidebarController()
{
    return SidebarController::GetSidebarControllerForFrame(xFrame);
}

OUString SAL_CALL SfxUnoDeck::getId()
{
    return mDeckId;
}

OUString SAL_CALL  SfxUnoDeck::getTitle()
{
    SolarMutexGuard aGuard;

    SidebarController* pSidebarController = getSidebarController();
    VclPtr<Deck> pDeck = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId)->mpDeck;

    if (!pDeck)
    {
        pSidebarController->CreateDeck(mDeckId);
        pDeck = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId)->mpDeck;
    }

    DeckTitleBar* pTitleBar = pDeck->GetTitleBar();
    return pTitleBar->GetTitle();
}

void SAL_CALL SfxUnoDeck::setTitle( const OUString& newTitle )
{
    SolarMutexGuard aGuard;

    SidebarController* pSidebarController = getSidebarController();
    pSidebarController->CreateDeck(mDeckId);

    std::shared_ptr<DeckDescriptor> xDeckDescriptor = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId);

    if (xDeckDescriptor)
    {
        Deck* pDeck = xDeckDescriptor->mpDeck;
        DeckTitleBar* pTitleBar = pDeck->GetTitleBar();
        pTitleBar->SetTitle(newTitle);

        xDeckDescriptor->msTitle = newTitle;
        xDeckDescriptor->msHelpText = newTitle;

        pSidebarController->notifyDeckTitle(mDeckId);
    }
}

sal_Bool SAL_CALL SfxUnoDeck::isActive()
{
    SolarMutexGuard aGuard;

    SidebarController* pSidebarController = getSidebarController();
    return pSidebarController->IsDeckVisible(mDeckId);
}


void SAL_CALL SfxUnoDeck::activate( const sal_Bool bActivate )
{
    SolarMutexGuard aGuard;

    SidebarController* pSidebarController = getSidebarController();

    // tdf#138160: OpenThenToggleDeck takes care of minimal width
    if (bActivate)
        pSidebarController->OpenThenToggleDeck(mDeckId);
    else
    {
        pSidebarController->SwitchToDefaultDeck();
        // update the sidebar
        pSidebarController->NotifyResize();
    }

}

uno::Reference<ui::XPanels> SAL_CALL SfxUnoDeck::getPanels()
{
    SolarMutexGuard aGuard;

    uno::Reference<ui::XPanels> panels = new SfxUnoPanels(xFrame, mDeckId);
    return panels;
}

sal_Int32 SAL_CALL SfxUnoDeck::getOrderIndex()
{
    SolarMutexGuard aGuard;
    SidebarController* pSidebarController = getSidebarController();

    sal_Int32 index = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId)->mnOrderIndex;
    return index;
}

void SAL_CALL SfxUnoDeck::setOrderIndex( const sal_Int32 newOrderIndex )
{
    SolarMutexGuard aGuard;
    SidebarController* pSidebarController = getSidebarController();

    std::shared_ptr<DeckDescriptor> xDeckDescriptor = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId);

    if (xDeckDescriptor)
    {
        xDeckDescriptor->mnOrderIndex = newOrderIndex;
        // update the sidebar
        pSidebarController->NotifyResize();
    }
}

void SAL_CALL SfxUnoDeck::moveFirst()
{
    SolarMutexGuard aGuard;
    SidebarController* pSidebarController = getSidebarController();

    ResourceManager::DeckContextDescriptorContainer aDecks = pSidebarController->GetMatchingDecks();

    sal_Int32 minIndex = GetMinOrderIndex(aDecks);
    sal_Int32 curOrderIndex = getOrderIndex();

    if (curOrderIndex != minIndex) // is deck already in place ?
    {
        minIndex -= 1;
        std::shared_ptr<DeckDescriptor> xDeckDescriptor = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId);
        if (xDeckDescriptor)
        {
            xDeckDescriptor->mnOrderIndex = minIndex;
            // update the sidebar
            pSidebarController->NotifyResize();
        }
    }
}

void SAL_CALL SfxUnoDeck::moveLast()
{
    SolarMutexGuard aGuard;
    SidebarController* pSidebarController = getSidebarController();

    ResourceManager::DeckContextDescriptorContainer aDecks = pSidebarController->GetMatchingDecks();

    sal_Int32 maxIndex = GetMaxOrderIndex(aDecks);
    sal_Int32 curOrderIndex = getOrderIndex();

    if (curOrderIndex != maxIndex) // is deck already in place ?
    {
        maxIndex += 1;
        std::shared_ptr<DeckDescriptor> xDeckDescriptor = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId);
        if (xDeckDescriptor)
        {
            xDeckDescriptor->mnOrderIndex = maxIndex;
            // update the sidebar
            pSidebarController->NotifyResize();
        }
    }
}

void SAL_CALL SfxUnoDeck::moveUp()
{
    SolarMutexGuard aGuard;
    SidebarController* pSidebarController = getSidebarController();

    // Search for previous deck OrderIndex
    ResourceManager::DeckContextDescriptorContainer aDecks = pSidebarController->GetMatchingDecks();

    sal_Int32 curOrderIndex = getOrderIndex();
    sal_Int32 previousIndex = GetMinOrderIndex(aDecks);

    for (auto const& deck : aDecks)
    {
        sal_Int32 index = pSidebarController->GetResourceManager()->GetDeckDescriptor(deck.msId)->mnOrderIndex;
        if( index < curOrderIndex && index > previousIndex)
            previousIndex = index;
    }

    if (curOrderIndex != previousIndex) // is deck already in place ?
    {
        previousIndex -= 1;
        std::shared_ptr<DeckDescriptor> xDeckDescriptor = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId);
        if (xDeckDescriptor)
        {
            xDeckDescriptor->mnOrderIndex = previousIndex;
            // update the sidebar
            pSidebarController->NotifyResize();
        }
    }
}

void SAL_CALL SfxUnoDeck::moveDown()
{
    SolarMutexGuard aGuard;
    SidebarController* pSidebarController = getSidebarController();

    ResourceManager::DeckContextDescriptorContainer aDecks = pSidebarController->GetMatchingDecks();

    // Search for next deck OrderIndex
    sal_Int32 curOrderIndex = getOrderIndex();
    sal_Int32 nextIndex = GetMaxOrderIndex(aDecks);

    for (auto const& deck : aDecks)
    {
        sal_Int32 index = pSidebarController->GetResourceManager()->GetDeckDescriptor(deck.msId)->mnOrderIndex;
        if( index > curOrderIndex && index < nextIndex)
            nextIndex = index;
    }

    if (curOrderIndex != nextIndex) // is deck already in place ?
    {
        nextIndex += 1;
        std::shared_ptr<DeckDescriptor> xDeckDescriptor = pSidebarController->GetResourceManager()->GetDeckDescriptor(mDeckId);
        if (xDeckDescriptor)
        {
            xDeckDescriptor->mnOrderIndex = nextIndex;
            // update the sidebar
            pSidebarController->NotifyResize();
        }
    }
}

sal_Int32 SfxUnoDeck::GetMinOrderIndex(const ResourceManager::DeckContextDescriptorContainer& rDecks)
{
    SidebarController* pSidebarController = getSidebarController();

    ResourceManager::DeckContextDescriptorContainer::const_iterator iDeck = rDecks.begin();
    sal_Int32 minIndex = pSidebarController->GetResourceManager()->GetDeckDescriptor(iDeck->msId)->mnOrderIndex;

    for (auto const& deck : rDecks)
    {
        sal_Int32 index = pSidebarController->GetResourceManager()->GetDeckDescriptor(deck.msId)->mnOrderIndex;
        if(minIndex > index)
            minIndex = index;
    }
    return minIndex;
}

sal_Int32 SfxUnoDeck::GetMaxOrderIndex(const ResourceManager::DeckContextDescriptorContainer& rDecks)
{
    SidebarController* pSidebarController = getSidebarController();

    sal_Int32 maxIndex = pSidebarController->GetResourceManager()->GetDeckDescriptor(rDecks.begin()->msId)->mnOrderIndex;

    for (auto const& deck : rDecks)
    {
        sal_Int32 index = pSidebarController->GetResourceManager()->GetDeckDescriptor(deck.msId)->mnOrderIndex;
        if(maxIndex < index)
            maxIndex = index;
    }
    return maxIndex;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
