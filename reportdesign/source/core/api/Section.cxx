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
#include <Section.hxx>
#include <Group.hxx>
#include <Groups.hxx>
#include <comphelper/enumhelper.hxx>
#include <comphelper/servicehelper.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <com/sun/star/report/XReportComponent.hpp>
#include <com/sun/star/report/ForceNewPage.hpp>
#include <com/sun/star/lang/NoSupportException.hpp>
#include <strings.hxx>
#include <Tools.hxx>
#include <RptModel.hxx>
#include <RptPage.hxx>
#include <ReportControlModel.hxx>
#include <ReportDefinition.hxx>
#include <vcl/svapp.hxx>

namespace reportdesign
{

    using namespace com::sun::star;
    using namespace comphelper;


static uno::Sequence< OUString> lcl_getGroupAbsent()
{
    const OUString pProps[] = {
                PROPERTY_CANGROW
                ,PROPERTY_CANSHRINK
        };

    return uno::Sequence< OUString >(pProps,SAL_N_ELEMENTS(pProps));
}


static uno::Sequence< OUString> lcl_getAbsent(bool _bPageSection)
{
    if ( _bPageSection )
    {
        const OUString pProps[] = {
                PROPERTY_FORCENEWPAGE
                ,PROPERTY_NEWROWORCOL
                ,PROPERTY_KEEPTOGETHER
                ,PROPERTY_CANGROW
                ,PROPERTY_CANSHRINK
                ,PROPERTY_REPEATSECTION
        };
        return uno::Sequence< OUString >(pProps,SAL_N_ELEMENTS(pProps));
    }

    const OUString pProps[] = {
                PROPERTY_CANGROW
                ,PROPERTY_CANSHRINK
                ,PROPERTY_REPEATSECTION
        };

    return uno::Sequence< OUString >(pProps,SAL_N_ELEMENTS(pProps));
}

rtl::Reference<OSection> OSection::createOSection(
    const rtl::Reference< OReportDefinition >& xParentDef,
    const uno::Reference< uno::XComponentContext >& context,
    bool const bPageSection)
{
    rtl::Reference<OSection> pNew =
        new OSection(xParentDef, nullptr, context, lcl_getAbsent(bPageSection));
    pNew->init();
    return pNew;
}

rtl::Reference<OSection> OSection::createOSection(
    const rtl::Reference< OGroup >& xParentGroup,
    const uno::Reference< uno::XComponentContext >& context)
{
    rtl::Reference<OSection> pNew =
        new OSection(nullptr, xParentGroup, context, lcl_getGroupAbsent());
    pNew->init();
    return pNew;
}


OSection::OSection(const rtl::Reference< OReportDefinition >& xParentDef
                   ,const rtl::Reference< OGroup >& xParentGroup
                   ,const uno::Reference< uno::XComponentContext >& context
                   ,uno::Sequence< OUString> const& rStrings)
:SectionBase(m_aMutex)
,SectionPropertySet(context,SectionPropertySet::IMPLEMENTS_PROPERTY_SET,rStrings)
,m_aContainerListeners(m_aMutex)
,m_xGroup(xParentGroup)
,m_xReportDefinition(xParentDef)
,m_nHeight(3000)
,m_nBackgroundColor(COL_TRANSPARENT)
,m_nForceNewPage(report::ForceNewPage::NONE)
,m_nNewRowOrCol(report::ForceNewPage::NONE)
,m_bKeepTogether(false)
,m_bRepeatSection(false)
,m_bVisible(true)
,m_bBacktransparent(true)
,m_bInRemoveNotify(false)
,m_bInInsertNotify(false)
{
}

// TODO: VirtualFunctionFinder: This is virtual function!

OSection::~OSection()
{
}

//IMPLEMENT_FORWARD_XINTERFACE2(OSection,SectionBase,SectionPropertySet)
IMPLEMENT_FORWARD_REFCOUNT( OSection, SectionBase )

uno::Any SAL_CALL OSection::queryInterface( const uno::Type& _rType )
{
    uno::Any aReturn = SectionBase::queryInterface(_rType);
    if ( !aReturn.hasValue() )
        aReturn = SectionPropertySet::queryInterface(_rType);

    if ( !aReturn.hasValue() && OReportControlModel::isInterfaceForbidden(_rType) )
        return aReturn;

    return aReturn;
}


void SAL_CALL OSection::dispose()
{
    OSL_ENSURE(!rBHelper.bDisposed,"Already disposed!");
    SectionPropertySet::dispose();
    uno::Reference<lang::XComponent> const xPageComponent(m_xDrawPage,
            uno::UNO_QUERY);
    if (xPageComponent.is())
    {
        xPageComponent->dispose();
    }
    cppu::WeakComponentImplHelperBase::dispose();

}

// TODO: VirtualFunctionFinder: This is virtual function!

void SAL_CALL OSection::disposing()
{
    lang::EventObject aDisposeEvent( getXWeak() );
    m_aContainerListeners.disposeAndClear( aDisposeEvent );
}

OUString SAL_CALL OSection::getImplementationName(  )
{
    return u"com.sun.star.comp.report.Section"_ustr;
}

uno::Sequence< OUString> OSection::getSupportedServiceNames_Static()
{
    uno::Sequence<OUString> aSupported { SERVICE_SECTION };
    return aSupported;
}

uno::Sequence< OUString> SAL_CALL OSection::getSupportedServiceNames()
{
    return getSupportedServiceNames_Static();
}

sal_Bool SAL_CALL OSection::supportsService( const OUString& _rServiceName )
{
    return cppu::supportsService(this, _rServiceName);
}

void OSection::init()
{
    SolarMutexGuard g; // lock while manipulating SdrModel
    uno::Reference< report::XReportDefinition> xReport = getReportDefinition();
    std::shared_ptr<rptui::OReportModel> pModel = OReportDefinition::getSdrModel(xReport);
    assert(pModel && "No model set at the report definition!");
    if ( !pModel )
        return;

    uno::Reference<report::XSection> const xSection(this);
    SdrPage & rSdrPage(*pModel->createNewPage(xSection));
    m_xDrawPage.set(rSdrPage.getUnoPage(), uno::UNO_QUERY_THROW);
    m_xDrawPage_ShapeGrouper.set(m_xDrawPage, uno::UNO_QUERY_THROW);
    // apparently we may also get OReportDrawPage which doesn't support this
    m_xDrawPage_FormSupplier.set(m_xDrawPage, uno::UNO_QUERY);
    m_xDrawPage_Tunnel.set(m_xDrawPage, uno::UNO_QUERY_THROW);
    // fdo#53872: now also exchange the XDrawPage in the SdrPage so that
    // rSdrPage.getUnoPage returns this
    rSdrPage.SetUnoPage(this);
    // createNewPage _should_ have stored away 2 uno::References to this,
    // so our ref count cannot be 1 here, so this isn't destroyed here
    assert(m_refCount > 1);
}

// XSection

sal_Bool SAL_CALL OSection::getVisible()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_bVisible;
}

void SAL_CALL OSection::setVisible( sal_Bool _visible )
{
    set(PROPERTY_VISIBLE,_visible,m_bVisible);
}

OUString SAL_CALL OSection::getName()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_sName;
}

void SAL_CALL OSection::setName( const OUString& _name )
{
    set(PROPERTY_NAME,_name,m_sName);
}

::sal_uInt32 SAL_CALL OSection::getHeight()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_nHeight;
}

void SAL_CALL OSection::setHeight( ::sal_uInt32 _height )
{
    set(PROPERTY_HEIGHT,_height,m_nHeight);
}

::sal_Int32 SAL_CALL OSection::getBackColor()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_bBacktransparent ? static_cast<sal_Int32>(COL_TRANSPARENT) : m_nBackgroundColor;
}

void SAL_CALL OSection::setBackColor( ::sal_Int32 _backgroundcolor )
{
    bool bTransparent = _backgroundcolor == static_cast<sal_Int32>(COL_TRANSPARENT);
    setBackTransparent(bTransparent);
    if ( !bTransparent )
        set(PROPERTY_BACKCOLOR,_backgroundcolor,m_nBackgroundColor);
}

sal_Bool SAL_CALL OSection::getBackTransparent()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_bBacktransparent;
}

void SAL_CALL OSection::setBackTransparent( sal_Bool _backtransparent )
{
    set(PROPERTY_BACKTRANSPARENT,_backtransparent,m_bBacktransparent);
    if ( _backtransparent )
        set(PROPERTY_BACKCOLOR,static_cast<sal_Int32>(COL_TRANSPARENT),m_nBackgroundColor);
}

OUString SAL_CALL OSection::getConditionalPrintExpression()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_sConditionalPrintExpression;
}

void SAL_CALL OSection::setConditionalPrintExpression( const OUString& _conditionalprintexpression )
{
    set(PROPERTY_CONDITIONALPRINTEXPRESSION,_conditionalprintexpression,m_sConditionalPrintExpression);
}

void OSection::checkNotPageHeaderFooter()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    rtl::Reference< OReportDefinition > xRet = m_xReportDefinition;
    if ( xRet.is() )
    {
        if ( xRet->getPageHeaderOn() && xRet->getPageHeader() == *this )
            throw beans::UnknownPropertyException();
        if ( xRet->getPageFooterOn() && xRet->getPageFooter() == *this )
            throw beans::UnknownPropertyException();
    }
}

::sal_Int16 SAL_CALL OSection::getForceNewPage()
{
    ::osl::MutexGuard aGuard(m_aMutex);

    checkNotPageHeaderFooter();
    return m_nForceNewPage;
}

void SAL_CALL OSection::setForceNewPage( ::sal_Int16 _forcenewpage )
{
    if ( _forcenewpage < report::ForceNewPage::NONE || _forcenewpage > report::ForceNewPage::BEFORE_AFTER_SECTION )
        throwIllegallArgumentException(u"css::report::ForceNewPage"
                        ,*this
                        ,1);
    checkNotPageHeaderFooter();
    set(PROPERTY_FORCENEWPAGE,_forcenewpage,m_nForceNewPage);
}

::sal_Int16 SAL_CALL OSection::getNewRowOrCol()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkNotPageHeaderFooter();
    return m_nNewRowOrCol;
}

void SAL_CALL OSection::setNewRowOrCol( ::sal_Int16 _newroworcol )
{
    if ( _newroworcol < report::ForceNewPage::NONE || _newroworcol > report::ForceNewPage::BEFORE_AFTER_SECTION )
        throwIllegallArgumentException(u"css::report::ForceNewPage"
                        ,*this
                        ,1);
    checkNotPageHeaderFooter();

    set(PROPERTY_NEWROWORCOL,_newroworcol,m_nNewRowOrCol);
}

sal_Bool SAL_CALL OSection::getKeepTogether()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    checkNotPageHeaderFooter();
    return m_bKeepTogether;
}

void SAL_CALL OSection::setKeepTogether( sal_Bool _keeptogether )
{
    {
        ::osl::MutexGuard aGuard(m_aMutex);
        checkNotPageHeaderFooter();
    }

    set(PROPERTY_KEEPTOGETHER,_keeptogether,m_bKeepTogether);
}

sal_Bool SAL_CALL OSection::getCanGrow()
{
    throw beans::UnknownPropertyException(); ///TODO: unsupported at the moment
}

void SAL_CALL OSection::setCanGrow( sal_Bool /*_cangrow*/ )
{
    throw beans::UnknownPropertyException(); ///TODO: unsupported at the moment
}

sal_Bool SAL_CALL OSection::getCanShrink()
{
    throw beans::UnknownPropertyException(); ///TODO: unsupported at the moment
}

void SAL_CALL OSection::setCanShrink( sal_Bool /*_canshrink*/ )
{
    throw beans::UnknownPropertyException(); ///TODO: unsupported at the moment
}

sal_Bool SAL_CALL OSection::getRepeatSection()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    rtl::Reference< OGroup > xGroup = m_xGroup;
    if ( !xGroup.is() )
        throw beans::UnknownPropertyException();
    return m_bRepeatSection;
}

void SAL_CALL OSection::setRepeatSection( sal_Bool _repeatsection )
{
    {
        ::osl::MutexGuard aGuard(m_aMutex);
        rtl::Reference< OGroup > xGroup = m_xGroup;
        if ( !xGroup.is() )
            throw beans::UnknownPropertyException();
    }
    set(PROPERTY_REPEATSECTION,_repeatsection,m_bRepeatSection);
}

uno::Reference< report::XGroup > SAL_CALL OSection::getGroup()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_xGroup.get();
}

uno::Reference< report::XReportDefinition > SAL_CALL OSection::getReportDefinition()
{
    ::osl::MutexGuard aGuard(m_aMutex);
    rtl::Reference< OReportDefinition > xRet = m_xReportDefinition;
    if (xRet.is())
        return xRet;
    rtl::Reference< OGroup > xGroup = m_xGroup;
    if ( xGroup.is() )
    {
        rtl::Reference< OGroups> xGroups(xGroup->getOGroups());
        if ( xGroups.is() )
            return xGroups->getReportDefinition();
    }
    return {};
}

// XChild
uno::Reference< uno::XInterface > SAL_CALL OSection::getParent(  )
{
    uno::Reference< uno::XInterface > xRet;
    {
        ::osl::MutexGuard aGuard(m_aMutex);
        xRet = m_xReportDefinition;
        if ( !xRet.is() )
            xRet = m_xGroup;
    }
    return  xRet;
}

void SAL_CALL OSection::setParent( const uno::Reference< uno::XInterface >& /*Parent*/ )
{
    throw lang::NoSupportException();
}

// XContainer
void SAL_CALL OSection::addContainerListener( const uno::Reference< container::XContainerListener >& xListener )
{
    m_aContainerListeners.addInterface(xListener);
}

void SAL_CALL OSection::removeContainerListener( const uno::Reference< container::XContainerListener >& xListener )
{
    m_aContainerListeners.removeInterface(xListener);
}

// XElementAccess
uno::Type SAL_CALL OSection::getElementType(  )
{
    return cppu::UnoType<report::XReportComponent>::get();
}

sal_Bool SAL_CALL OSection::hasElements(  )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_xDrawPage.is() && m_xDrawPage->hasElements();
}

// XIndexAccess
::sal_Int32 SAL_CALL OSection::getCount(  )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_xDrawPage.is() ? m_xDrawPage->getCount() : 0;
}

uno::Any SAL_CALL OSection::getByIndex( ::sal_Int32 Index )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return m_xDrawPage.is() ? m_xDrawPage->getByIndex(Index) : uno::Any();
}

// XEnumerationAccess
uno::Reference< container::XEnumeration > SAL_CALL OSection::createEnumeration(  )
{
    ::osl::MutexGuard aGuard(m_aMutex);
    return new ::comphelper::OEnumerationByIndex(static_cast<XSection*>(this));
}

uno::Reference< beans::XPropertySetInfo > SAL_CALL OSection::getPropertySetInfo(  )
{
    return SectionPropertySet::getPropertySetInfo();
}

void SAL_CALL OSection::setPropertyValue( const OUString& aPropertyName, const uno::Any& aValue )
{
    SectionPropertySet::setPropertyValue( aPropertyName, aValue );
}

uno::Any SAL_CALL OSection::getPropertyValue( const OUString& PropertyName )
{
    return SectionPropertySet::getPropertyValue( PropertyName);
}

void SAL_CALL OSection::addPropertyChangeListener( const OUString& aPropertyName, const uno::Reference< beans::XPropertyChangeListener >& xListener )
{
    SectionPropertySet::addPropertyChangeListener( aPropertyName, xListener );
}

void SAL_CALL OSection::removePropertyChangeListener( const OUString& aPropertyName, const uno::Reference< beans::XPropertyChangeListener >& aListener )
{
    SectionPropertySet::removePropertyChangeListener( aPropertyName, aListener );
}

void SAL_CALL OSection::addVetoableChangeListener( const OUString& PropertyName, const uno::Reference< beans::XVetoableChangeListener >& aListener )
{
    SectionPropertySet::addVetoableChangeListener( PropertyName, aListener );
}

void SAL_CALL OSection::removeVetoableChangeListener( const OUString& PropertyName, const uno::Reference< beans::XVetoableChangeListener >& aListener )
{
    SectionPropertySet::removeVetoableChangeListener( PropertyName, aListener );
}

void SAL_CALL OSection::add( const uno::Reference< drawing::XShape >& xShape )
{
    {
        ::osl::MutexGuard aGuard(m_aMutex);
        m_bInInsertNotify = true;
        OSL_ENSURE(m_xDrawPage.is(),"No DrawPage!");
        m_xDrawPage->add(xShape);
        m_bInInsertNotify = false;
    }
    notifyElementAdded(xShape);
}

void SAL_CALL OSection::remove( const uno::Reference< drawing::XShape >& xShape )
{
    {
        ::osl::MutexGuard aGuard(m_aMutex);
        m_bInRemoveNotify = true;
        OSL_ENSURE(m_xDrawPage.is(),"No DrawPage!");
        m_xDrawPage->remove(xShape);
        m_bInRemoveNotify = false;
    }
    notifyElementRemoved(xShape);
}

// XShapeGrouper
uno::Reference< drawing::XShapeGroup > SAL_CALL
OSection::group(uno::Reference< drawing::XShapes > const& xShapes)
{
    // no lock because m_xDrawPage_ShapeGrouper is const
    return (m_xDrawPage_ShapeGrouper.is())
        ? m_xDrawPage_ShapeGrouper->group(xShapes)
        : nullptr;
}
void SAL_CALL
OSection::ungroup(uno::Reference<drawing::XShapeGroup> const& xGroup)
{
    // no lock because m_xDrawPage_ShapeGrouper is const
    if (m_xDrawPage_ShapeGrouper.is()) {
        m_xDrawPage_ShapeGrouper->ungroup(xGroup);
    }
}

// XFormsSupplier
uno::Reference<container::XNameContainer> SAL_CALL OSection::getForms()
{
    // no lock because m_xDrawPage_FormSupplier is const
    return (m_xDrawPage_FormSupplier.is())
        ? m_xDrawPage_FormSupplier->getForms()
        : nullptr;
}
// XFormsSupplier2
sal_Bool SAL_CALL OSection::hasForms()
{
    // no lock because m_xDrawPage_FormSupplier is const
    return (m_xDrawPage_FormSupplier.is())
        && m_xDrawPage_FormSupplier->hasForms();
}


// css::lang::XUnoTunnel

sal_Int64 OSection::getSomething( const uno::Sequence< sal_Int8 > & rId )
{
    if (comphelper::isUnoTunnelId<OSection>(rId) )
        return comphelper::getSomething_cast(this);
    return (m_xDrawPage_Tunnel.is()) ? m_xDrawPage_Tunnel->getSomething(rId) : 0;
}

const uno::Sequence< sal_Int8 > & OSection::getUnoTunnelId()
{
    static const comphelper::UnoIdInit implId;
    return implId.getSeq();
}

void OSection::notifyElementAdded(const uno::Reference< drawing::XShape >& xShape )
{
    if ( !m_bInInsertNotify )
    {
        container::ContainerEvent aEvent(static_cast<container::XContainer*>(this), uno::Any(), uno::Any(xShape), uno::Any());
        m_aContainerListeners.notifyEach(&container::XContainerListener::elementInserted,aEvent);
    }
}

void OSection::notifyElementRemoved(const uno::Reference< drawing::XShape >& xShape)
{
    if ( !m_bInRemoveNotify )
    {
        // notify our container listeners
        container::ContainerEvent aEvent(static_cast<container::XContainer*>(this), uno::Any(), uno::Any(xShape), uno::Any());
        m_aContainerListeners.notifyEach(&container::XContainerListener::elementRemoved,aEvent);
    }
}

} // namespace reportdesign


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
