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


#include <com/sun/star/graphic/GraphicProvider.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/view/SelectionType.hpp>
#include <com/sun/star/util/VetoException.hpp>
#include <o3tl/any.hxx>
#include <helper/property.hxx>

#include <com/sun/star/awt/tree/XMutableTreeNode.hpp>
#include <controls/treecontrolpeer.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>

#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>
#include <vcl/graph.hxx>
#include <vcl/svapp.hxx>
#include <vcl/toolkit/treelistbox.hxx>
#include <vcl/toolkit/treelistentry.hxx>
#include <vcl/toolkit/viewdataentry.hxx>
#include <vcl/toolkit/svlbitm.hxx>
#include <vcl/unohelp.hxx>

#include <map>
#include <memory>
#include <list>

using namespace ::com::sun::star;
using namespace css::uno;
using namespace css::lang;
using namespace css::awt::tree;
using namespace css::beans;
using namespace css::view;
using namespace css::container;
using namespace css::util;
using namespace css::graphic;

namespace {

struct LockGuard
{
public:
    explicit LockGuard( sal_Int32& rLock )
    : mrLock( rLock )
    {
        rLock++;
    }

    ~LockGuard()
    {
        mrLock--;
    }

    sal_Int32& mrLock;
};


class ImplContextGraphicItem : public SvLBoxContextBmp
{
public:
    ImplContextGraphicItem( Image const & rI1, Image const & rI2, bool bExpanded)
        : SvLBoxContextBmp(rI1, rI2, bExpanded) {}

    OUString msExpandedGraphicURL;
    OUString msCollapsedGraphicURL;
};


}

class UnoTreeListBoxImpl : public SvTreeListBox
{
public:
    UnoTreeListBoxImpl( TreeControlPeer* pPeer, vcl::Window* pParent, WinBits nWinStyle );
    virtual ~UnoTreeListBoxImpl() override;
    virtual void dispose() override;

    void            insert( SvTreeListEntry* pEntry, SvTreeListEntry* pParent, sal_uInt32 nPos );

    virtual void    RequestingChildren( SvTreeListEntry* pParent ) override;

    virtual bool    EditingEntry( SvTreeListEntry* pEntry ) override;
    virtual bool    EditedEntry( SvTreeListEntry* pEntry, const OUString& rNewText ) override;

    DECL_LINK(OnSelectionChangeHdl, SvTreeListBox*, void);
    DECL_LINK(OnExpandingHdl, SvTreeListBox*, bool);
    DECL_LINK(OnExpandedHdl, SvTreeListBox*, void);

private:
    rtl::Reference< TreeControlPeer > mxPeer;
};


namespace {

class UnoTreeListItem : public SvLBoxString
{
public:
                    UnoTreeListItem();

    void            InitViewData( SvTreeListBox*,SvTreeListEntry*,SvViewDataItem * = nullptr ) override;
    void            SetImage( const Image& rImage );
    const OUString& GetGraphicURL() const { return maGraphicURL;}
    void            SetGraphicURL( const OUString& rGraphicURL );
    virtual void    Paint(const Point& rPos, SvTreeListBox& rOutDev, vcl::RenderContext& rRenderContext,
                          const SvViewDataEntry* pView, const SvTreeListEntry& rEntry) override;
    std::unique_ptr<SvLBoxItem> Clone( SvLBoxItem const * pSource ) const override;

private:
    OUString        maGraphicURL;
    Image           maImage;
};

}

class UnoTreeListEntry : public SvTreeListEntry
{
public:
    UnoTreeListEntry( const Reference< XTreeNode >& xNode, TreeControlPeer* pPeer );
    virtual ~UnoTreeListEntry() override;

    Reference< XTreeNode > mxNode;
    TreeControlPeer* mpPeer;
};

TreeControlPeer::TreeControlPeer()
    : maSelectionListeners( *this )
    , maTreeExpansionListeners( *this )
    , maTreeEditListeners( *this )
    , mbIsRootDisplayed(false)
    , mpTreeImpl( nullptr )
    , mnEditLock( 0 )
{
}


TreeControlPeer::~TreeControlPeer()
{
    if( mpTreeImpl )
        mpTreeImpl->Clear();
}


void TreeControlPeer::addEntry( UnoTreeListEntry* pEntry )
{
    if( pEntry && pEntry->mxNode.is() )
    {
        if( !mpTreeNodeMap )
        {
            mpTreeNodeMap.reset( new TreeNodeMap );
        }

        (*mpTreeNodeMap)[ pEntry->mxNode ] = pEntry;
    }
}


void TreeControlPeer::removeEntry( UnoTreeListEntry const * pEntry )
{
    if( mpTreeNodeMap && pEntry && pEntry->mxNode.is() )
    {
        TreeNodeMap::iterator aIter( mpTreeNodeMap->find( pEntry->mxNode ) );
        if( aIter != mpTreeNodeMap->end() )
        {
            mpTreeNodeMap->erase( aIter );
        }
    }
}


UnoTreeListEntry* TreeControlPeer::getEntry( const Reference< XTreeNode >& xNode, bool bThrow /* = true */ )
{
    if( mpTreeNodeMap )
    {
        TreeNodeMap::iterator aIter( mpTreeNodeMap->find( xNode ) );
        if( aIter != mpTreeNodeMap->end() )
            return (*aIter).second;
    }

    if( bThrow )
        throw IllegalArgumentException();

    return nullptr;
}


vcl::Window* TreeControlPeer::createVclControl( vcl::Window* pParent, sal_Int64 nWinStyle )
{
    mpTreeImpl = VclPtr<UnoTreeListBoxImpl>::Create( this, pParent, nWinStyle );
    return mpTreeImpl;
}


/** called from the UnoTreeListBoxImpl when it gets deleted */
void TreeControlPeer::disposeControl()
{
    mpTreeNodeMap.reset();
    mpTreeImpl = nullptr;
}


UnoTreeListEntry* TreeControlPeer::createEntry( const Reference< XTreeNode >& xNode, UnoTreeListEntry* pParent, sal_uInt32 nPos /* = TREELIST_APPEND */ )
{
    UnoTreeListEntry* pEntry = nullptr;
    if( mpTreeImpl )
    {
        Image aImage;
        pEntry = new UnoTreeListEntry( xNode, this );
        pEntry->AddItem(std::make_unique<ImplContextGraphicItem>(aImage, aImage, true));

        std::unique_ptr<UnoTreeListItem> pUnoItem(new UnoTreeListItem);

        if( !xNode->getNodeGraphicURL().isEmpty() )
        {
            pUnoItem->SetGraphicURL( xNode->getNodeGraphicURL() );
            Image aNodeImage;
            loadImage( xNode->getNodeGraphicURL(), aNodeImage );
            pUnoItem->SetImage( aNodeImage );
            mpTreeImpl->AdjustEntryHeight( aNodeImage );
        }

        pEntry->AddItem(std::move(pUnoItem));

        mpTreeImpl->insert( pEntry, pParent, nPos );

        if( !msDefaultExpandedGraphicURL.isEmpty() )
            mpTreeImpl->SetExpandedEntryBmp( pEntry, maDefaultExpandedImage );

        if( !msDefaultCollapsedGraphicURL.isEmpty() )
            mpTreeImpl->SetCollapsedEntryBmp( pEntry, maDefaultCollapsedImage );

        updateEntry( pEntry );
    }
    return pEntry;
}


void TreeControlPeer::updateEntry( UnoTreeListEntry* pEntry )
{
    bool bChanged = false;
    if( !(pEntry && pEntry->mxNode.is() && mpTreeImpl) )
        return;

    const OUString aValue( getEntryString( pEntry->mxNode->getDisplayValue() ) );
    UnoTreeListItem* pUnoItem = dynamic_cast< UnoTreeListItem* >( &pEntry->GetItem( 1 ) );
    if( pUnoItem )
    {
        if( aValue != pUnoItem->GetText() )
        {
            pUnoItem->SetText( aValue );
            bChanged = true;
        }

        if( pUnoItem->GetGraphicURL() != pEntry->mxNode->getNodeGraphicURL() )
        {
            Image aImage;
            if( loadImage( pEntry->mxNode->getNodeGraphicURL(), aImage ) )
            {
                pUnoItem->SetGraphicURL( pEntry->mxNode->getNodeGraphicURL() );
                pUnoItem->SetImage( aImage );
                mpTreeImpl->AdjustEntryHeight( aImage );
                bChanged = true;
            }
        }
    }

    if( bool(pEntry->mxNode->hasChildrenOnDemand()) != pEntry->HasChildrenOnDemand() )
    {
        pEntry->EnableChildrenOnDemand( pEntry->mxNode->hasChildrenOnDemand() );
        bChanged = true;
    }

    ImplContextGraphicItem* pContextGraphicItem = dynamic_cast< ImplContextGraphicItem* >( &pEntry->GetItem( 0 ) );
    if( pContextGraphicItem )
    {
        if( pContextGraphicItem->msExpandedGraphicURL != pEntry->mxNode->getExpandedGraphicURL() )
        {
            Image aImage;
            if( loadImage( pEntry->mxNode->getExpandedGraphicURL(), aImage ) )
            {
                pContextGraphicItem->msExpandedGraphicURL = pEntry->mxNode->getExpandedGraphicURL();
                mpTreeImpl->SetExpandedEntryBmp( pEntry, aImage );
                bChanged = true;
            }
        }
        if( pContextGraphicItem->msCollapsedGraphicURL != pEntry->mxNode->getCollapsedGraphicURL() )
        {
            Image aImage;
            if( loadImage( pEntry->mxNode->getCollapsedGraphicURL(), aImage ) )
            {
                pContextGraphicItem->msCollapsedGraphicURL = pEntry->mxNode->getCollapsedGraphicURL();
                mpTreeImpl->SetCollapsedEntryBmp( pEntry, aImage );
                bChanged = true;
            }
        }
    }

    if( bChanged )
        mpTreeImpl->GetModel()->InvalidateEntry( pEntry );
}


void TreeControlPeer::onSelectionChanged()
{
    Reference< XInterface > xSource( getXWeak() );
    EventObject aEvent( xSource );
    maSelectionListeners.selectionChanged( aEvent );
}


void TreeControlPeer::onRequestChildNodes( const Reference< XTreeNode >& xNode )
{
    try
    {
        Reference< XInterface > xSource( getXWeak() );
        TreeExpansionEvent aEvent( xSource, xNode );
        maTreeExpansionListeners.requestChildNodes( aEvent );
    }
    catch( Exception& )
    {
    }
}


bool TreeControlPeer::onExpanding( const Reference< XTreeNode >& xNode, bool bExpanding )
{
    try
    {
        Reference< XInterface > xSource( getXWeak() );
        TreeExpansionEvent aEvent( xSource, xNode );
        if( bExpanding )
        {
            maTreeExpansionListeners.treeExpanding( aEvent );
        }
        else
        {
            maTreeExpansionListeners.treeCollapsing( aEvent );
        }
    }
    catch( Exception& )
    {
        return false;
    }
    return true;
}


void TreeControlPeer::onExpanded( const Reference< XTreeNode >& xNode, bool bExpanding )
{
    try
    {
        Reference< XInterface > xSource( getXWeak() );
        TreeExpansionEvent aEvent( xSource, xNode );

        if( bExpanding )
        {
            maTreeExpansionListeners.treeExpanded( aEvent );
        }
        else
        {
            maTreeExpansionListeners.treeCollapsed( aEvent );
        }
    }
    catch( Exception& )
    {
    }
}


void TreeControlPeer::fillTree( UnoTreeListBoxImpl& rTree, const Reference< XTreeDataModel >& xDataModel )
{
    rTree.Clear();

    if( !xDataModel.is() )
        return;

    Reference< XTreeNode > xRootNode( xDataModel->getRoot() );
    if( !xRootNode.is() )
        return;

    if( mbIsRootDisplayed )
    {
        addNode( rTree, xRootNode, nullptr );
    }
    else
    {
        const sal_Int32 nChildCount = xRootNode->getChildCount();
        for( sal_Int32 nChild = 0; nChild < nChildCount; nChild++ )
            addNode( rTree, xRootNode->getChildAt( nChild ), nullptr );
    }
}


void TreeControlPeer::addNode( UnoTreeListBoxImpl& rTree, const Reference< XTreeNode >& xNode, UnoTreeListEntry* pParentEntry )
{
    if( xNode.is() )
    {
        UnoTreeListEntry* pEntry = createEntry( xNode, pParentEntry, TREELIST_APPEND );
        const sal_Int32 nChildCount = xNode->getChildCount();
        for( sal_Int32 nChild = 0; nChild < nChildCount; nChild++ )
            addNode( rTree, xNode->getChildAt( nChild ), pEntry );
    }
}


UnoTreeListBoxImpl& TreeControlPeer::getTreeListBoxOrThrow() const
{
    if( !mpTreeImpl )
        throw DisposedException();
    return *mpTreeImpl;
}


void TreeControlPeer::ChangeNodesSelection( const Any& rSelection, bool bSelect, bool bSetSelection )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    Reference< XTreeNode > xTempNode;

    Sequence<Reference<XTreeNode>> pNodes;
    sal_Int32 nCount = 0;

    if( rSelection.hasValue() )
    {
        switch( rSelection.getValueTypeClass() )
        {
        case TypeClass_INTERFACE:
            {
                rSelection >>= xTempNode;
                if( xTempNode.is() )
                {
                    nCount = 1;
                    pNodes = {xTempNode};
                }
                break;
            }
        case TypeClass_SEQUENCE:
            {
                if( auto rSeq = o3tl::tryAccess<Sequence<Reference<XTreeNode>>>(
                        rSelection) )
                {
                    nCount = rSeq->getLength();
                    pNodes = *rSeq;
                }
                break;
            }
        default:
            break;
        }

        if( nCount == 0 )
            throw IllegalArgumentException();
    }

    if( bSetSelection )
        rTree.SelectAll( false );

    for( sal_Int32 i = 0; i != nCount; ++i )
    {
        UnoTreeListEntry* pEntry = getEntry( pNodes[i] );
        rTree.Select( pEntry, bSelect );
    }
}


// css::view::XSelectionSupplier


sal_Bool SAL_CALL TreeControlPeer::select( const Any& rSelection )
{
    SolarMutexGuard aGuard;
    ChangeNodesSelection( rSelection, true, true );
    return true;
}


Any SAL_CALL TreeControlPeer::getSelection()
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    Any aRet;

    sal_uInt32 nSelectionCount = rTree.GetSelectionCount();
    if( nSelectionCount == 1 )
    {
        UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.FirstSelected() );
        if( pEntry && pEntry->mxNode.is() )
            aRet <<= pEntry->mxNode;
    }
    else if( nSelectionCount > 1 )
    {
        Sequence< Reference< XTreeNode > > aSelection( nSelectionCount );
        Reference< XTreeNode >* pNodes = aSelection.getArray();
        UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.FirstSelected() );
        while( pEntry && nSelectionCount )
        {
            *pNodes++ = pEntry->mxNode;
            pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.NextSelected( pEntry ) );
            --nSelectionCount;
        }

        OSL_ASSERT( (pEntry == nullptr) && (nSelectionCount == 0) );
        aRet <<= aSelection;
    }

    return aRet;
}


void SAL_CALL TreeControlPeer::addSelectionChangeListener( const Reference< XSelectionChangeListener >& xListener )
{
    maSelectionListeners.addInterface( xListener );
}


void SAL_CALL TreeControlPeer::removeSelectionChangeListener( const Reference< XSelectionChangeListener >& xListener )
{
    maSelectionListeners.removeInterface( xListener );
}


// css::view::XMultiSelectionSupplier


sal_Bool SAL_CALL TreeControlPeer::addSelection( const Any& rSelection )
{
    ChangeNodesSelection( rSelection, true, false );
    return true;
}


void SAL_CALL TreeControlPeer::removeSelection( const Any& rSelection )
{
    ChangeNodesSelection( rSelection, false, false );
}


void SAL_CALL TreeControlPeer::clearSelection()
{
    SolarMutexGuard aGuard;
    getTreeListBoxOrThrow().SelectAll( false );
}


sal_Int32 SAL_CALL TreeControlPeer::getSelectionCount()
{
    SolarMutexGuard aGuard;
    return getTreeListBoxOrThrow().GetSelectionCount();
}

namespace {

class TreeSelectionEnumeration : public ::cppu::WeakImplHelper< XEnumeration >
{
public:
    explicit TreeSelectionEnumeration( std::list< Any >& rSelection );
    virtual sal_Bool SAL_CALL hasMoreElements() override;
    virtual Any SAL_CALL nextElement() override;

    std::list< Any > maSelection;
    std::list< Any >::iterator maIter;
};

}

TreeSelectionEnumeration::TreeSelectionEnumeration( std::list< Any >& rSelection )
{
    maSelection.swap( rSelection );
    maIter = maSelection.begin();
}


sal_Bool SAL_CALL TreeSelectionEnumeration::hasMoreElements()
{
    return maIter != maSelection.end();
}


Any SAL_CALL TreeSelectionEnumeration::nextElement()
{
    if( maIter == maSelection.end() )
        throw NoSuchElementException();

    return (*maIter++);
}


Reference< XEnumeration > SAL_CALL TreeControlPeer::createSelectionEnumeration()
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    sal_uInt32 nSelectionCount = rTree.GetSelectionCount();
    std::list< Any > aSelection( nSelectionCount );

    UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.FirstSelected() );
    while( pEntry && nSelectionCount )
    {
        aSelection.emplace_back( pEntry->mxNode );
        pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.NextSelected( pEntry ) );
        --nSelectionCount;
    }

    OSL_ASSERT( (pEntry == nullptr) && (nSelectionCount == 0) );

    return Reference< XEnumeration >( new TreeSelectionEnumeration( aSelection ) );
}


Reference< XEnumeration > SAL_CALL TreeControlPeer::createReverseSelectionEnumeration()
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    sal_uInt32 nSelectionCount = rTree.GetSelectionCount();
    std::list< Any > aSelection;

    UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.FirstSelected() );
    while( pEntry && nSelectionCount )
    {
        aSelection.push_front( Any( pEntry->mxNode ) );
        pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.NextSelected( pEntry ) );
        --nSelectionCount;
    }

    OSL_ASSERT( (pEntry == nullptr) && (nSelectionCount == 0) );

    return Reference< XEnumeration >( new TreeSelectionEnumeration( aSelection ) );
}


// css::awt::XTreeControl


OUString SAL_CALL TreeControlPeer::getDefaultExpandedGraphicURL()
{
    SolarMutexGuard aGuard;
    return msDefaultExpandedGraphicURL;
}


void SAL_CALL TreeControlPeer::setDefaultExpandedGraphicURL( const OUString& sDefaultExpandedGraphicURL )
{
    SolarMutexGuard aGuard;
    if( msDefaultExpandedGraphicURL == sDefaultExpandedGraphicURL )
        return;

    if( !sDefaultExpandedGraphicURL.isEmpty() )
        loadImage( sDefaultExpandedGraphicURL, maDefaultExpandedImage );
    else
        maDefaultExpandedImage = Image();

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    SvTreeListEntry* pEntry = rTree.First();
    while( pEntry )
    {
        ImplContextGraphicItem* pContextGraphicItem = dynamic_cast< ImplContextGraphicItem* >( &pEntry->GetItem( 0 ) );
        if( pContextGraphicItem )
        {
            if( pContextGraphicItem->msExpandedGraphicURL.isEmpty() )
                rTree.SetExpandedEntryBmp( pEntry, maDefaultExpandedImage );
        }
        pEntry = rTree.Next( pEntry );
    }

    msDefaultExpandedGraphicURL = sDefaultExpandedGraphicURL;
}


OUString SAL_CALL TreeControlPeer::getDefaultCollapsedGraphicURL()
{
    SolarMutexGuard aGuard;
    return msDefaultCollapsedGraphicURL;
}


void SAL_CALL TreeControlPeer::setDefaultCollapsedGraphicURL( const OUString& sDefaultCollapsedGraphicURL )
{
    SolarMutexGuard aGuard;
    if( msDefaultCollapsedGraphicURL == sDefaultCollapsedGraphicURL )
        return;

    if( !sDefaultCollapsedGraphicURL.isEmpty() )
        loadImage( sDefaultCollapsedGraphicURL, maDefaultCollapsedImage );
    else
        maDefaultCollapsedImage = Image();

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    SvTreeListEntry* pEntry = rTree.First();
    while( pEntry )
    {
        ImplContextGraphicItem* pContextGraphicItem = dynamic_cast< ImplContextGraphicItem* >( &pEntry->GetItem( 0 ) );
        if( pContextGraphicItem )
        {
            if( pContextGraphicItem->msCollapsedGraphicURL.isEmpty() )
                rTree.SetCollapsedEntryBmp( pEntry, maDefaultCollapsedImage );
        }
        pEntry = rTree.Next( pEntry );
    }

    msDefaultCollapsedGraphicURL = sDefaultCollapsedGraphicURL;
}


sal_Bool SAL_CALL TreeControlPeer::isNodeExpanded( const Reference< XTreeNode >& xNode )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    UnoTreeListEntry* pEntry = getEntry( xNode );
    return pEntry && rTree.IsExpanded( pEntry );
}


sal_Bool SAL_CALL TreeControlPeer::isNodeCollapsed( const Reference< XTreeNode >& xNode )
{
    SolarMutexGuard aGuard;
    return !isNodeExpanded( xNode );
}


void SAL_CALL TreeControlPeer::makeNodeVisible( const Reference< XTreeNode >& xNode )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    UnoTreeListEntry* pEntry = getEntry( xNode );
    if( pEntry )
        rTree.MakeVisible( pEntry );
}


sal_Bool SAL_CALL TreeControlPeer::isNodeVisible( const Reference< XTreeNode >& xNode )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    UnoTreeListEntry* pEntry = getEntry( xNode );
    return pEntry && rTree.IsEntryVisible( pEntry );
}


void SAL_CALL TreeControlPeer::expandNode( const Reference< XTreeNode >& xNode )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    UnoTreeListEntry* pEntry = getEntry( xNode );
    if( pEntry )
        rTree.Expand( pEntry );
}


void SAL_CALL TreeControlPeer::collapseNode( const Reference< XTreeNode >& xNode )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    UnoTreeListEntry* pEntry = getEntry( xNode );
    if( pEntry )
        rTree.Collapse( pEntry );
}


void SAL_CALL TreeControlPeer::addTreeExpansionListener( const Reference< XTreeExpansionListener >& xListener )
{
    maTreeExpansionListeners.addInterface( xListener );
}


void SAL_CALL TreeControlPeer::removeTreeExpansionListener( const Reference< XTreeExpansionListener >& xListener )
{
    maTreeExpansionListeners.removeInterface( xListener );
}


Reference< XTreeNode > SAL_CALL TreeControlPeer::getNodeForLocation( sal_Int32 x, sal_Int32 y )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    Reference< XTreeNode > xNode;

    const Point aPos( x, y );
    UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.GetEntry( aPos, true ) );
    if( pEntry )
        xNode = pEntry->mxNode;

    return xNode;
}


Reference< XTreeNode > SAL_CALL TreeControlPeer::getClosestNodeForLocation( sal_Int32 x, sal_Int32 y )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    Reference< XTreeNode > xNode;

    const Point aPos( x, y );
    UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( rTree.GetEntry( aPos, true ) );
    if( pEntry )
        xNode = pEntry->mxNode;

    return xNode;
}


awt::Rectangle SAL_CALL TreeControlPeer::getNodeRect( const Reference< XTreeNode >& i_Node )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    UnoTreeListEntry* pEntry = getEntry( i_Node );

    ::tools::Rectangle aEntryRect( rTree.GetFocusRect( pEntry, rTree.GetEntryPosition( pEntry ).Y() ) );
    return vcl::unohelper::ConvertToAWTRect( aEntryRect );
}


sal_Bool SAL_CALL TreeControlPeer::isEditing(  )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    return rTree.IsEditingActive();
}


sal_Bool SAL_CALL TreeControlPeer::stopEditing()
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    if( rTree.IsEditingActive() )
    {
        rTree.EndEditing();
        return true;
    }
    else
    {
        return false;
    }
}


void SAL_CALL TreeControlPeer::cancelEditing(  )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    rTree.EndEditing();
}


void SAL_CALL TreeControlPeer::startEditingAtNode( const Reference< XTreeNode >& xNode )
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    UnoTreeListEntry* pEntry = getEntry( xNode );
    rTree.EditEntry( pEntry );
}

void SAL_CALL TreeControlPeer::addTreeEditListener( const Reference< XTreeEditListener >& xListener )
{
    maTreeEditListeners.addInterface( xListener );
}

void SAL_CALL TreeControlPeer::removeTreeEditListener( const Reference< XTreeEditListener >& xListener )
{
    maTreeEditListeners.removeInterface( xListener );
}

bool TreeControlPeer::onEditingEntry( UnoTreeListEntry const * pEntry )
{
    if( mpTreeImpl && pEntry && pEntry->mxNode.is() && (maTreeEditListeners.getLength() > 0)  )
    {
        try
        {
            maTreeEditListeners.nodeEditing( pEntry->mxNode );
        }
        catch( VetoException& )
        {
            return false;
        }
        catch( Exception& )
        {
        }
    }
    return true;
}

bool TreeControlPeer::onEditedEntry( UnoTreeListEntry const * pEntry, const OUString& rNewText )
{
    if( mpTreeImpl && pEntry && pEntry->mxNode.is() ) try
    {
        LockGuard aLockGuard( mnEditLock );
        if( maTreeEditListeners.getLength() > 0 )
        {
            maTreeEditListeners.nodeEdited( pEntry->mxNode, rNewText );
            return false;
        }
        else
        {
            Reference< XMutableTreeNode > xMutableNode( pEntry->mxNode, UNO_QUERY );
            if( xMutableNode.is() )
                xMutableNode->setDisplayValue( Any( rNewText ) );
            else
                return false;
        }

    }
    catch( Exception& )
    {
    }

    return true;
}


// css::awt::tree::TreeDataModelListener


void SAL_CALL TreeControlPeer::treeNodesChanged( const css::awt::tree::TreeDataModelEvent& rEvent )
{
    SolarMutexGuard aGuard;

    if( mnEditLock != 0 )
        return;

    updateTree( rEvent );
}

void SAL_CALL TreeControlPeer::treeNodesInserted( const css::awt::tree::TreeDataModelEvent& rEvent )
{
    SolarMutexGuard aGuard;

    if( mnEditLock != 0 )
        return;

    updateTree( rEvent );
}

void SAL_CALL TreeControlPeer::treeNodesRemoved( const css::awt::tree::TreeDataModelEvent& rEvent )
{
    SolarMutexGuard aGuard;

    if( mnEditLock != 0 )
        return;

    updateTree( rEvent );
}

void SAL_CALL TreeControlPeer::treeStructureChanged( const css::awt::tree::TreeDataModelEvent& rEvent )
{
    SolarMutexGuard aGuard;

    if( mnEditLock != 0 )
        return;

    updateTree( rEvent );
}

void TreeControlPeer::updateTree( const css::awt::tree::TreeDataModelEvent& rEvent )
{
    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    Sequence< Reference< XTreeNode > > Nodes;
    Reference< XTreeNode > xNode( rEvent.ParentNode );
    if( !xNode.is() && Nodes.hasElements() )
    {
        xNode = Nodes[0];
    }

    if( xNode.is() )
        updateNode( rTree, xNode );
}

void TreeControlPeer::updateNode( UnoTreeListBoxImpl const & rTree, const Reference< XTreeNode >& xNode )
{
    if( !xNode.is() )
        return;

    UnoTreeListEntry* pNodeEntry = getEntry( xNode, false );

    if( !pNodeEntry )
    {
        Reference< XTreeNode > xParentNode( xNode->getParent() );
        UnoTreeListEntry* pParentEntry = nullptr;
        sal_uInt32 nChild = TREELIST_APPEND;

        if( xParentNode.is() )
        {
            pParentEntry = getEntry( xParentNode  );
            nChild = xParentNode->getIndex( xNode );
        }

        pNodeEntry = createEntry( xNode, pParentEntry, nChild );
    }

    updateChildNodes( rTree, xNode, pNodeEntry );
}

void TreeControlPeer::updateChildNodes( UnoTreeListBoxImpl const & rTree, const Reference< XTreeNode >& xParentNode, UnoTreeListEntry* pParentEntry )
{
    if( !(xParentNode.is() && pParentEntry) )
        return;

    UnoTreeListEntry* pCurrentChild = dynamic_cast< UnoTreeListEntry* >( rTree.FirstChild( pParentEntry ) );

    const sal_Int32 nChildCount = xParentNode->getChildCount();
    for( sal_Int32 nChild = 0; nChild < nChildCount; nChild++ )
    {
        Reference< XTreeNode > xNode( xParentNode->getChildAt( nChild ) );
        if( !pCurrentChild || ( pCurrentChild->mxNode != xNode ) )
        {
            UnoTreeListEntry* pNodeEntry = getEntry( xNode, false );
            if( pNodeEntry == nullptr )
            {
                // child node is not yet part of the tree, add it
                pCurrentChild = createEntry( xNode, pParentEntry, nChild );
            }
            else if( pNodeEntry != pCurrentChild )
            {
                // node is already part of the tree, but not on the correct position
                rTree.GetModel()->Move( pNodeEntry, pParentEntry, nChild );
                pCurrentChild = pNodeEntry;
                updateEntry( pCurrentChild );
            }
        }
        else
        {
            // child node has entry and entry is equal to current entry,
            // so no structural changes happened
            updateEntry( pCurrentChild );
        }

        pCurrentChild = dynamic_cast< UnoTreeListEntry* >( pCurrentChild->NextSibling() );
    }

    // check if we have entries without nodes left, we need to remove them
    while( pCurrentChild )
    {
        UnoTreeListEntry* pNextChild = dynamic_cast< UnoTreeListEntry* >( pCurrentChild->NextSibling() );
        rTree.GetModel()->Remove( pCurrentChild );
        pCurrentChild = pNextChild;
    }
}

OUString TreeControlPeer::getEntryString( const Any& rValue )
{
    OUString sValue;
    if( rValue.hasValue() )
    {
        switch( rValue.getValueTypeClass() )
        {
        case TypeClass_SHORT:
        case TypeClass_LONG:
            {
                sal_Int32 nValue = 0;
                if( rValue >>= nValue )
                    sValue = OUString::number( nValue );
                break;
            }
        case TypeClass_BYTE:
        case TypeClass_UNSIGNED_SHORT:
        case TypeClass_UNSIGNED_LONG:
            {
                sal_uInt32 nValue = 0;
                if( rValue >>= nValue )
                    sValue = OUString::number( nValue );
                break;
            }
        case TypeClass_HYPER:
            {
                sal_Int64 nValue = 0;
                if( rValue >>= nValue )
                    sValue = OUString::number( nValue );
                break;
            }
        case TypeClass_UNSIGNED_HYPER:
            {
                sal_uInt64 nValue = 0;
                if( rValue >>= nValue )
                    sValue = OUString::number( nValue );
                break;
            }
        case TypeClass_FLOAT:
        case TypeClass_DOUBLE:
            {
                double fValue = 0.0;
                if( rValue >>= fValue )
                    sValue = OUString::number( fValue );
                break;
            }
        case TypeClass_STRING:
            rValue >>= sValue;
            break;
    /*
        case TypeClass_INTERFACE:
            // @todo
            break;
        case TypeClass_SEQUENCE:
            {
                Sequence< Any > aValues;
                if( aValue >>= aValues )
                {
                    updateEntry( SvTreeListEntry& rEntry, aValues );
                    return;
                }
            }
            break;
    */
        default:
            break;
        }
    }
    return sValue;
}

// XEventListener
void SAL_CALL TreeControlPeer::disposing( const css::lang::EventObject& )
{
    // model is disposed, so we clear our tree
    SolarMutexGuard aGuard;
    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
    rTree.Clear();
    mxDataModel.clear();
}

void TreeControlPeer::onChangeDataModel( UnoTreeListBoxImpl& rTree, const Reference< XTreeDataModel >& xDataModel )
{
    if( xDataModel.is() && (mxDataModel == xDataModel) )
        return; // do nothing

    Reference< XTreeDataModelListener > xListener( this );

    if( mxDataModel.is() )
        mxDataModel->removeTreeDataModelListener( xListener );

    mxDataModel = xDataModel;

    fillTree( rTree, mxDataModel );

    if( mxDataModel.is() )
        mxDataModel->addTreeDataModelListener( xListener );
}


// css::awt::XLayoutConstrains


css::awt::Size TreeControlPeer::getMinimumSize()
{
    SolarMutexGuard aGuard;

    css::awt::Size aSz;
/* todo
    MultiLineEdit* pEdit = (MultiLineEdit*) GetWindow();
    if ( pEdit )
        aSz = vcl::unohelper::ConvertToAWTSize(pEdit->CalcMinimumSize());
*/
    return aSz;
}

css::awt::Size TreeControlPeer::getPreferredSize()
{
    return getMinimumSize();
}

css::awt::Size TreeControlPeer::calcAdjustedSize( const css::awt::Size& rNewSize )
{
    SolarMutexGuard aGuard;

    css::awt::Size aSz = rNewSize;
/* todo
    MultiLineEdit* pEdit = (MultiLineEdit*) GetWindow();
    if ( pEdit )
        aSz = vcl::unohelper::ConvertToAWTSize(pEdit->CalcAdjustedSize(vcl::unohelper::ConvertToVCLSize(rNewSize)));
*/
    return aSz;
}


// css::awt::XVclWindowPeer


void TreeControlPeer::setProperty( const OUString& PropertyName, const Any& aValue)
{
    SolarMutexGuard aGuard;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    switch( GetPropertyId( PropertyName ) )
    {
        case BASEPROPERTY_HIDEINACTIVESELECTION:
        {
            bool bEnabled = false;
            if ( aValue >>= bEnabled )
            {
                WinBits nStyle = rTree.GetStyle();
                if ( bEnabled )
                    nStyle |= WB_HIDESELECTION;
                else
                    nStyle &= ~WB_HIDESELECTION;
                rTree.SetStyle( nStyle );
            }
        }
        break;

        case BASEPROPERTY_TREE_SELECTIONTYPE:
        {
            SelectionType eSelectionType;
            if( aValue >>= eSelectionType )
            {
                SelectionMode eSelMode;
                switch( eSelectionType )
                {
                case SelectionType_SINGLE:  eSelMode = SelectionMode::Single; break;
                case SelectionType_RANGE:   eSelMode = SelectionMode::Range; break;
                case SelectionType_MULTI:   eSelMode = SelectionMode::Multiple; break;
    //          case SelectionType_NONE:
                default:                    eSelMode = SelectionMode::NONE; break;
                }
                if( rTree.GetSelectionMode() != eSelMode )
                    rTree.SetSelectionMode( eSelMode );
            }
            break;
        }

        case BASEPROPERTY_TREE_DATAMODEL:
            onChangeDataModel( rTree, Reference< XTreeDataModel >( aValue, UNO_QUERY ) );
            break;
        case BASEPROPERTY_ROW_HEIGHT:
        {
            sal_Int32 nHeight = 0;
            if( aValue >>= nHeight )
                rTree.SetEntryHeight( static_cast<short>(nHeight) );
            break;
        }
        case BASEPROPERTY_TREE_EDITABLE:
        {
            bool bEnabled = false;
            if( aValue >>= bEnabled )
                rTree.EnableInplaceEditing( bEnabled );
            break;
        }
        case BASEPROPERTY_TREE_INVOKESSTOPNODEEDITING:
            break; // @todo
        case BASEPROPERTY_TREE_ROOTDISPLAYED:
        {
            bool bDisplayed = false;
            if( (aValue >>= bDisplayed) && ( bDisplayed != mbIsRootDisplayed) )
            {
                onChangeRootDisplayed(bDisplayed);
            }
            break;
        }
        case BASEPROPERTY_TREE_SHOWSHANDLES:
        {
            bool bEnabled = false;
            if( aValue >>= bEnabled )
            {
                WinBits nBits = rTree.GetStyle() & (~WB_HASLINES);
                if( bEnabled )
                    nBits |= WB_HASLINES;
                if( nBits != rTree.GetStyle() )
                    rTree.SetStyle( nBits );
            }
            break;
        }
        case BASEPROPERTY_TREE_SHOWSROOTHANDLES:
        {
            bool bEnabled = false;
            if( aValue >>= bEnabled )
            {
                WinBits nBits = rTree.GetStyle() & (~WB_HASLINESATROOT);
                if( bEnabled )
                    nBits |= WB_HASLINESATROOT;
                if( nBits != rTree.GetStyle() )
                    rTree.SetStyle( nBits );
            }
            break;
        }
        default:
        VCLXWindow::setProperty( PropertyName, aValue );
        break;
    }
}

Any TreeControlPeer::getProperty( const OUString& PropertyName )
{
    SolarMutexGuard aGuard;

    const sal_uInt16 nPropId = GetPropertyId( PropertyName );
    if( (nPropId >= BASEPROPERTY_TREE_START) && (nPropId <= BASEPROPERTY_TREE_END) )
    {
        UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();
        switch(nPropId)
        {
        case BASEPROPERTY_TREE_SELECTIONTYPE:
        {
            SelectionType eSelectionType;

            SelectionMode eSelMode = rTree.GetSelectionMode();
            switch( eSelMode )
            {
            case SelectionMode::Single:  eSelectionType = SelectionType_SINGLE; break;
            case SelectionMode::Range:   eSelectionType = SelectionType_RANGE; break;
            case SelectionMode::Multiple:eSelectionType = SelectionType_MULTI; break;
//          case SelectionMode::NONE:
            default:                eSelectionType = SelectionType_NONE; break;
            }
            return Any( eSelectionType );
        }
        case BASEPROPERTY_ROW_HEIGHT:
            return Any( static_cast<sal_Int32>(rTree.GetEntryHeight()) );
        case BASEPROPERTY_TREE_DATAMODEL:
            return Any( mxDataModel );
        case BASEPROPERTY_TREE_EDITABLE:
            return Any( rTree.IsInplaceEditingEnabled() );
        case BASEPROPERTY_TREE_INVOKESSTOPNODEEDITING:
            return Any( true ); // @todo
        case BASEPROPERTY_TREE_ROOTDISPLAYED:
            return Any( mbIsRootDisplayed );
        case BASEPROPERTY_TREE_SHOWSHANDLES:
            return Any( (rTree.GetStyle() & WB_HASLINES) != 0 );
        case BASEPROPERTY_TREE_SHOWSROOTHANDLES:
            return Any( (rTree.GetStyle() & WB_HASLINESATROOT) != 0 );
        }
    }
    return VCLXWindow::getProperty( PropertyName );
}

void TreeControlPeer::onChangeRootDisplayed( bool bIsRootDisplayed )
{
    if( mbIsRootDisplayed == bIsRootDisplayed )
        return;

    mbIsRootDisplayed = bIsRootDisplayed;

    UnoTreeListBoxImpl& rTree = getTreeListBoxOrThrow();

    if( rTree.GetEntryCount() == 0 )
        return;

    // todo
    fillTree( rTree, mxDataModel );
}

bool TreeControlPeer::loadImage( const OUString& rURL, Image& rImage )
{
    if( !mxGraphicProvider.is() )
    {
        mxGraphicProvider = graphic::GraphicProvider::create(
            comphelper::getProcessComponentContext());
    }

    try
    {
        css::beans::PropertyValues aProps{ comphelper::makePropertyValue(u"URL"_ustr, rURL) };
        Reference< XGraphic > xGraphic( mxGraphicProvider->queryGraphic( aProps ) );

        Graphic aGraphic( xGraphic );
        rImage = Image(aGraphic.GetBitmapEx());
        return true;
    }
    catch( Exception& )
    {
    }

    return false;
}




UnoTreeListBoxImpl::UnoTreeListBoxImpl( TreeControlPeer* pPeer, vcl::Window* pParent, WinBits nWinStyle )
: SvTreeListBox( pParent, nWinStyle )
, mxPeer( pPeer )
{
    SetStyle( WB_BORDER | WB_HASLINES |WB_HASBUTTONS | WB_HASLINESATROOT | WB_HASBUTTONSATROOT | WB_HSCROLL );
    SetNodeDefaultImages();
    SetSelectHdl( LINK(this, UnoTreeListBoxImpl, OnSelectionChangeHdl) );
    SetDeselectHdl( LINK(this, UnoTreeListBoxImpl, OnSelectionChangeHdl) );

    SetExpandingHdl( LINK(this, UnoTreeListBoxImpl, OnExpandingHdl) );
    SetExpandedHdl( LINK(this, UnoTreeListBoxImpl, OnExpandedHdl) );

}


UnoTreeListBoxImpl::~UnoTreeListBoxImpl()
{
    disposeOnce();
}

void UnoTreeListBoxImpl::dispose()
{
    if( mxPeer.is() )
        mxPeer->disposeControl();
    mxPeer.clear();
    SvTreeListBox::dispose();
}


IMPL_LINK_NOARG(UnoTreeListBoxImpl, OnSelectionChangeHdl, SvTreeListBox*, void)
{
    if( mxPeer.is() )
        mxPeer->onSelectionChanged();
}


IMPL_LINK_NOARG(UnoTreeListBoxImpl, OnExpandingHdl, SvTreeListBox*, bool)
{
    UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( GetHdlEntry() );

    if( pEntry && mxPeer.is() )
    {
        return mxPeer->onExpanding( pEntry->mxNode, !IsExpanded( pEntry ) );
    }
    return false;
}


IMPL_LINK_NOARG(UnoTreeListBoxImpl, OnExpandedHdl, SvTreeListBox*, void)
{
    UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( GetHdlEntry() );
    if( pEntry && mxPeer.is() )
    {
        mxPeer->onExpanded( pEntry->mxNode, IsExpanded( pEntry ) );
    }
}


void UnoTreeListBoxImpl::insert( SvTreeListEntry* pEntry,SvTreeListEntry* pParent,sal_uInt32 nPos )
{
    if( pParent )
        SvTreeListBox::Insert( pEntry, pParent, nPos );
    else
        SvTreeListBox::Insert( pEntry, nPos );
}


void UnoTreeListBoxImpl::RequestingChildren( SvTreeListEntry* pParent )
{
    UnoTreeListEntry* pEntry = dynamic_cast< UnoTreeListEntry* >( pParent );
    if( pEntry && pEntry->mxNode.is() && mxPeer.is() )
        mxPeer->onRequestChildNodes( pEntry->mxNode );
}


bool UnoTreeListBoxImpl::EditingEntry( SvTreeListEntry* pEntry )
{
    return mxPeer.is() && mxPeer->onEditingEntry( dynamic_cast< UnoTreeListEntry* >( pEntry ) );
}


bool UnoTreeListBoxImpl::EditedEntry( SvTreeListEntry* pEntry, const OUString& rNewText )
{
    return mxPeer.is() && mxPeer->onEditedEntry( dynamic_cast< UnoTreeListEntry* >( pEntry ), rNewText );
}




UnoTreeListItem::UnoTreeListItem()
: SvLBoxString(OUString())
{
}

void UnoTreeListItem::Paint(
    const Point& rPos, SvTreeListBox& rDev, vcl::RenderContext& rRenderContext, const SvViewDataEntry* /*pView*/, const SvTreeListEntry& rEntry)
{
    Point aPos(rPos);
    Size aSize(GetWidth(&rDev, &rEntry), GetHeight(&rDev, &rEntry));
    if (!!maImage)
    {
        rRenderContext.DrawImage(aPos, maImage, rDev.IsEnabled() ? DrawImageFlags::NONE : DrawImageFlags::Disable);
        int nWidth = maImage.GetSizePixel().Width() + 6;
        aPos.AdjustX(nWidth );
        aSize.AdjustWidth( -nWidth );
    }
    rRenderContext.DrawText(tools::Rectangle(aPos,aSize),maText, rDev.IsEnabled() ? DrawTextFlags::NONE : DrawTextFlags::Disable);
}


std::unique_ptr<SvLBoxItem> UnoTreeListItem::Clone(SvLBoxItem const * pSource) const
{
    std::unique_ptr<UnoTreeListItem> pNew(new UnoTreeListItem);
    UnoTreeListItem const * pSourceItem = static_cast< UnoTreeListItem const * >( pSource );
    pNew->maText = pSourceItem->maText;
    pNew->maImage = pSourceItem->maImage;
    return std::unique_ptr<SvLBoxItem>(pNew.release());
}


void UnoTreeListItem::SetImage( const Image& rImage )
{
    maImage = rImage;
}


void UnoTreeListItem::SetGraphicURL( const OUString& rGraphicURL )
{
    maGraphicURL = rGraphicURL;
}


void UnoTreeListItem::InitViewData( SvTreeListBox* pView,SvTreeListEntry* pEntry, SvViewDataItem* pViewData)
{
    if( !pViewData )
        pViewData = pView->GetViewDataItem( pEntry, this );

    Size aSize(maImage.GetSizePixel());
    pViewData->mnWidth = aSize.Width();
    pViewData->mnHeight = aSize.Height();

    const Size aTextSize(pView->GetTextWidth( maText ), pView->GetTextHeight());
    if( pViewData->mnWidth )
    {
        pViewData->mnWidth += (6 + aTextSize.Width());
        if( pViewData->mnHeight < aTextSize.Height() )
            pViewData->mnHeight = aTextSize.Height();
    }
    else
    {
        pViewData->mnWidth = aTextSize.Width();
        pViewData->mnHeight = aTextSize.Height();
    }
}


UnoTreeListEntry::UnoTreeListEntry( const Reference< XTreeNode >& xNode, TreeControlPeer* pPeer )
: mxNode( xNode )
, mpPeer( pPeer )
{
    if( mpPeer )
        mpPeer->addEntry( this );
}


UnoTreeListEntry::~UnoTreeListEntry()
{
    if( mpPeer )
        mpPeer->removeEntry( this );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
