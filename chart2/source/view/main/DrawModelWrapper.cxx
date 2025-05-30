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

#include <chartview/DrawModelWrapper.hxx>
#include <ShapeFactory.hxx>
#include "ChartItemPool.hxx"
#include <ObjectIdentifier.hxx>
#include <svx/unomodel.hxx>
#include <svl/itempool.hxx>
#include <svx/objfac3d.hxx>
#include <svx/svdpage.hxx>
#include <svx/svx3ditems.hxx>
#include <svx/xtable.hxx>
#include <svx/svdoutl.hxx>
#include <editeng/unolingu.hxx>
#include <vcl/svapp.hxx>
#include <vcl/virdev.hxx>
#include <libxml/xmlwriter.h>
#include <osl/diagnose.h>

namespace com::sun::star::linguistic2 { class XHyphenator; }
namespace com::sun::star::linguistic2 { class XSpellChecker1; }

using namespace ::com::sun::star;


namespace chart
{

DrawModelWrapper::DrawModelWrapper()
:   SdrModel()
{
    m_xChartItemPool = ChartItemPool::CreateChartItemPool();

    SetDefaultFontHeight(423);     // 12pt

    SfxItemPool* pMasterPool = &GetItemPool();
    pMasterPool->SetDefaultMetric(MapUnit::Map100thMM);
    pMasterPool->SetUserDefaultItem(SfxBoolItem(EE_PARA_HYPHENATE, true) );
    pMasterPool->SetUserDefaultItem(makeSvx3DPercentDiagonalItem (5));

    // append chart pool to end of pool chain
    pMasterPool->GetLastPoolInChain()->SetSecondaryPool(m_xChartItemPool.get());
    SetTextDefaults();

    //this factory needs to be created before first use of 3D scenes once upon an office runtime
    //@todo in future this should be done by drawing engine itself on demand
    static bool b3dFactoryInitialized = false;
    if(!b3dFactoryInitialized)
    {
        E3dObjFactory aObjFactory;
        b3dFactoryInitialized = true;
    }

    //Hyphenation and spellchecking
    SdrOutliner& rOutliner = GetDrawOutliner();
    try
    {
        uno::Reference< linguistic2::XHyphenator > xHyphenator( LinguMgr::GetHyphenator() );
        if( xHyphenator.is() )
            rOutliner.SetHyphenator( xHyphenator );

        uno::Reference< linguistic2::XSpellChecker1 > xSpellChecker( LinguMgr::GetSpellChecker() );
        if ( xSpellChecker.is() )
            rOutliner.SetSpeller( xSpellChecker );
    }
    catch(...)
    {
        OSL_FAIL("Can't get Hyphenator or SpellChecker for chart");
    }

    //ref device for font rendering
    OutputDevice* pDefaultDevice = rOutliner.GetRefDevice();
    if( !pDefaultDevice )
        pDefaultDevice = Application::GetDefaultDevice();
    m_pRefDevice.disposeAndClear();
    m_pRefDevice = VclPtr<VirtualDevice>::Create(*pDefaultDevice);
    MapMode aMapMode = m_pRefDevice->GetMapMode();
    aMapMode.SetMapUnit(MapUnit::Map100thMM);
    m_pRefDevice->SetMapMode(aMapMode);
    SetRefDevice(m_pRefDevice.get());
    rOutliner.SetRefDevice(m_pRefDevice.get());
}

DrawModelWrapper::~DrawModelWrapper()
{
    // normally call from ~SdrModel, but do it here explicitly before we clear m_xChartItemPool
    implDtorClearModel();

    //remove m_pChartItemPool from pool chain
    if (m_xChartItemPool)
    {
        SfxItemPool* pPool = &GetItemPool();
        for (;;)
        {
            SfxItemPool* pSecondary = pPool->GetSecondaryPool();
            if(pSecondary == m_xChartItemPool.get())
            {
                pPool->SetSecondaryPool (nullptr);
                break;
            }
            pPool = pSecondary;
        }
        m_xChartItemPool.clear();
    }
    m_pRefDevice.disposeAndClear();
}

uno::Reference< frame::XModel > DrawModelWrapper::createUnoModel()
{
    return new SvxUnoDrawingModel( this ); //tell Andreas Schluens if SvxUnoDrawingModel is not needed anymore -> remove export from svx to avoid link problems in writer
}

const uno::Reference< frame::XModel > & DrawModelWrapper::getUnoModel()
{
    return SdrModel::getUnoModel();
}

SdrModel& DrawModelWrapper::getSdrModel()
{
    return *this;
}

uno::Reference< lang::XMultiServiceFactory > DrawModelWrapper::getShapeFactory()
{
    uno::Reference< lang::XMultiServiceFactory > xShapeFactory( getUnoModel(), uno::UNO_QUERY );
    return xShapeFactory;
}

const rtl::Reference<SvxDrawPage> & DrawModelWrapper::getMainDrawPage()
{
    if (m_xMainDrawPage.is())
        return m_xMainDrawPage;

    // Create draw page.
    uno::Reference<drawing::XDrawPagesSupplier> xDrawPagesSuplier(getUnoModel(), uno::UNO_QUERY);
    if (!xDrawPagesSuplier.is())
        return m_xMainDrawPage;

    uno::Reference<drawing::XDrawPages> xDrawPages = xDrawPagesSuplier->getDrawPages();
    if (xDrawPages->getCount() > 1)
    {
        // Take the first page in case of multiple pages.
        uno::Any aPage = xDrawPages->getByIndex(0);
        uno::Reference<drawing::XDrawPage> xTmp;
        aPage >>= xTmp;
        m_xMainDrawPage = dynamic_cast<SvxDrawPage*>(xTmp.get());
        assert(m_xMainDrawPage);
    }

    if (!m_xMainDrawPage.is())
    {
        m_xMainDrawPage = dynamic_cast<SvxDrawPage*>(xDrawPages->insertNewByIndex(0).get());
        assert(m_xMainDrawPage);
    }

    //ensure that additional shapes are in front of the chart objects so create the chart root before
    // let us disable this call for now
    // TODO:moggi
    // ShapeFactory::getOrCreateShapeFactory(getShapeFactory())->getOrCreateChartRootShape( m_xMainDrawPage );
    return m_xMainDrawPage;
}

const rtl::Reference<SvxDrawPage> & DrawModelWrapper::getHiddenDrawPage()
{
    if( !m_xHiddenDrawPage.is() )
    {
        uno::Reference< drawing::XDrawPagesSupplier > xDrawPagesSuplier( getUnoModel(), uno::UNO_QUERY );
        if( xDrawPagesSuplier.is() )
        {
            uno::Reference< drawing::XDrawPages > xDrawPages( xDrawPagesSuplier->getDrawPages () );
            if( xDrawPages->getCount()>1 )
            {
                uno::Any aPage = xDrawPages->getByIndex( 1 ) ;
                uno::Reference<drawing::XDrawPage> xTmp;
                aPage >>= xTmp;
                m_xHiddenDrawPage = dynamic_cast<SvxDrawPage*>(xTmp.get());
                assert(m_xHiddenDrawPage);
            }

            if(!m_xHiddenDrawPage.is())
            {
                if( xDrawPages->getCount()==0 )
                {
                    m_xMainDrawPage = dynamic_cast<SvxDrawPage*>(xDrawPages->insertNewByIndex( 0 ).get());
                    assert(m_xMainDrawPage);
                }
                m_xHiddenDrawPage = dynamic_cast<SvxDrawPage*>(xDrawPages->insertNewByIndex( 1 ).get());
                assert(m_xHiddenDrawPage);
            }
        }
    }
    return m_xHiddenDrawPage;
}
void DrawModelWrapper::clearMainDrawPage()
{
    //uno::Reference<drawing::XShapes> xChartRoot( m_xMainDrawPage, uno::UNO_QUERY );
    rtl::Reference<SvxShapeGroupAnyD> xChartRoot( ShapeFactory::getChartRootShape( m_xMainDrawPage ) );
    if( xChartRoot.is() )
    {
        sal_Int32 nSubCount = xChartRoot->getCount();
        uno::Reference< drawing::XShape > xShape;
        for( sal_Int32 nS = nSubCount; nS--; )
        {
            if( xChartRoot->getByIndex( nS ) >>= xShape )
                xChartRoot->remove( xShape );
        }
    }
}

rtl::Reference<SvxShapeGroupAnyD> DrawModelWrapper::getChartRootShape( const rtl::Reference<SvxDrawPage>& xDrawPage )
{
    return ShapeFactory::getChartRootShape( xDrawPage );
}

void DrawModelWrapper::lockControllers()
{
    uno::Reference< frame::XModel > xDrawModel( getUnoModel() );
    if( xDrawModel.is())
        xDrawModel->lockControllers();
}
void DrawModelWrapper::unlockControllers()
{
    uno::Reference< frame::XModel > xDrawModel( getUnoModel() );
    if( xDrawModel.is())
        xDrawModel->unlockControllers();
}

OutputDevice* DrawModelWrapper::getReferenceDevice() const
{
    return SdrModel::GetRefDevice();
}

SfxItemPool& DrawModelWrapper::GetItemPool()
{
    return SdrModel::GetItemPool();
}
XColorListRef DrawModelWrapper::GetColorList() const
{
    return SdrModel::GetColorList();
}
XDashListRef DrawModelWrapper::GetDashList() const
{
    return SdrModel::GetDashList();
}
XLineEndListRef DrawModelWrapper::GetLineEndList() const
{
    return SdrModel::GetLineEndList();
}
XGradientListRef DrawModelWrapper::GetGradientList() const
{
    return SdrModel::GetGradientList();
}
XHatchListRef DrawModelWrapper::GetHatchList() const
{
    return SdrModel::GetHatchList();
}
XBitmapListRef DrawModelWrapper::GetBitmapList() const
{
    return SdrModel::GetBitmapList();
}

XPatternListRef DrawModelWrapper::GetPatternList() const
{
    return SdrModel::GetPatternList();
}

SdrObject* DrawModelWrapper::getNamedSdrObject( const OUString& rName )
{
    if( rName.isEmpty() )
        return nullptr;
    return getNamedSdrObject( rName, GetPage(0) );
}

SdrObject* DrawModelWrapper::getNamedSdrObject( const OUString& rObjectCID, SdrObjList const * pSearchList )
{
    if(!pSearchList || rObjectCID.isEmpty())
        return nullptr;
    for (const rtl::Reference<SdrObject>& pObj : *pSearchList)
    {
        if( ObjectIdentifier::areIdenticalObjects( rObjectCID, pObj->GetName() ) )
            return pObj.get();
        SdrObject* pNamedObj = DrawModelWrapper::getNamedSdrObject( rObjectCID, pObj->GetSubList() );
        if(pNamedObj)
            return pNamedObj;
    }
    return nullptr;
}

bool DrawModelWrapper::removeShape( const rtl::Reference<SvxShape>& xShape )
{
    uno::Reference<drawing::XShapes> xShapes( xShape->getParent(), uno::UNO_QUERY );
    if( xShapes.is() )
    {
        xShapes->remove(xShape);
        return true;
    }
    return false;
}

void DrawModelWrapper::dumpAsXml(xmlTextWriterPtr pWriter) const
{
    (void)xmlTextWriterStartElement(pWriter, BAD_CAST("DrawModelWrapper"));
    (void)xmlTextWriterWriteFormatAttribute(pWriter, BAD_CAST("ptr"), "%p", this);

    SdrModel::dumpAsXml(pWriter);

    (void)xmlTextWriterEndElement(pWriter);
}

} //namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
