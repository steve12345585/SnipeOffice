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

#include <unoparagraph.hxx>

#include <cppuhelper/exc_hlp.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <osl/diagnose.h>
#include <comphelper/diagnose_ex.hxx>

#include <cmdid.h>
#include <fmtautofmt.hxx>
#include <unomid.h>
#include <unoparaframeenum.hxx>
#include <unotext.hxx>
#include <unotextrange.hxx>
#include <unoport.hxx>
#include <unomap.hxx>
#include <unocrsr.hxx>
#include <unoprnms.hxx>
#include <unocrsrhelper.hxx>
#include <doc.hxx>
#include <ndtxt.hxx>
#include <algorithm>
#include <utility>
#include <vcl/svapp.hxx>
#include <docsh.hxx>
#include <swunohelper.hxx>
#include <names.hxx>

#include <com/sun/star/beans/SetPropertyTolerantFailed.hpp>
#include <com/sun/star/beans/GetPropertyTolerantResult.hpp>
#include <com/sun/star/beans/TolerantPropertySetResultType.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/text/WrapTextMode.hpp>
#include <com/sun/star/text/TextContentAnchorType.hpp>

#include <com/sun/star/drawing/BitmapMode.hpp>
#include <comphelper/propertyvalue.hxx>
#include <comphelper/scopeguard.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/servicehelper.hxx>
#include <editeng/unoipset.hxx>
#include <svx/unobrushitemhelper.hxx>
#include <svx/xflbmtit.hxx>
#include <svx/xflbstit.hxx>

using namespace ::com::sun::star;

namespace {

class SwParaSelection
{
    SwCursor & m_rCursor;
public:
    explicit SwParaSelection(SwCursor & rCursor);
    ~SwParaSelection();
};

}

SwParaSelection::SwParaSelection(SwCursor & rCursor)
    : m_rCursor(rCursor)
{
    if (m_rCursor.HasMark())
    {
        m_rCursor.DeleteMark();
    }
    // is it at the start?
    if (m_rCursor.GetPoint()->GetContentIndex() != 0)
    {
        m_rCursor.MovePara(GoCurrPara, fnParaStart);
    }
    // or at the end already?
    if (m_rCursor.GetPoint()->GetContentIndex() != m_rCursor.GetPointContentNode()->Len())
    {
        m_rCursor.SetMark();
        m_rCursor.MovePara(GoCurrPara, fnParaEnd);
    }
}

SwParaSelection::~SwParaSelection()
{
    if (m_rCursor.GetPoint()->GetContentIndex() != 0)
    {
        m_rCursor.DeleteMark();
        m_rCursor.MovePara(GoCurrPara, fnParaStart);
    }
}

/// @throws beans::UnknownPropertyException
/// @throws uno::RuntimeException
static beans::PropertyState lcl_SwXParagraph_getPropertyState(
                            const SwTextNode& rTextNode,
                            const SwAttrSet** ppSet,
                            const SfxItemPropertyMapEntry& rEntry,
                            bool &rAttrSetFetched );

SwTextNode& SwXParagraph::GetTextNodeOrThrow()
{
    if (!m_pTextNode) {
        throw uno::RuntimeException(u"SwXParagraph: disposed or invalid"_ustr, nullptr);
    }
    return *m_pTextNode;
}

void SwXParagraph::MySvtListener::Notify(const SfxHint& rHint)
{
    if(rHint.GetId() == SfxHintId::Dying)
    {
        m_rThis.m_pTextNode = nullptr;
        std::unique_lock aGuard(m_rThis.m_Mutex);
        if (m_rThis.m_EventListeners.getLength(aGuard) != 0)
        {
            // fdo#72695: if UNO object is already dead, don't revive it with event
            // The specific pattern we are guarding against is this:
            // [1] Thread1 takes the SolarMutex
            // [2] Thread2 decrements the SwXParagraph reference count, and calls the
            //     SwXParagraph destructor, which tries to take the SolarMutex, and blocks
            // [3] Thread1 destroys a SwTextNode, which calls this Notify event, which results
            //     in a double-free if we construct the xThis object.
            if (m_rThis.m_refCount == 0)
            {   // fdo#72695: if UNO object is already dead, don't revive it with event
                return;
            }
            lang::EventObject const ev(static_cast<cppu::OWeakObject*>(&m_rThis));
            m_rThis.m_EventListeners.disposeAndClear(aGuard, ev);
        }
    }
}

SwXParagraph::SwXParagraph()
    : m_rPropSet(*aSwMapProvider.GetPropertySet(PROPERTY_MAP_PARAGRAPH))
    , m_bIsDescriptor(true)
    , m_nSelectionStartPos(-1)
    , m_nSelectionEndPos(-1)
    , m_pTextNode(nullptr)
    , moSvtListener(std::in_place, *this)
{
}

SwXParagraph::SwXParagraph(
        css::uno::Reference< SwXText > const & xParent,
        SwTextNode & rTextNode,
        const sal_Int32 nSelStart, const sal_Int32 nSelEnd)
    : m_rPropSet(*aSwMapProvider.GetPropertySet(PROPERTY_MAP_PARAGRAPH))
    , m_bIsDescriptor(false)
    , m_nSelectionStartPos(nSelStart)
    , m_nSelectionEndPos(nSelEnd)
    , m_xParentText(xParent)
    , m_pTextNode(&rTextNode)
    , moSvtListener(std::in_place, *this)
{
    moSvtListener->StartListening(rTextNode.GetNotifier());
}

SwXParagraph::~SwXParagraph()
{
    // need to hold solar mutex while destructing SvtListener
    SolarMutexGuard aGuard;
    moSvtListener.reset();
}

rtl::Reference<SwXParagraph>
SwXParagraph::CreateXParagraph(SwDoc & rDoc, SwTextNode *const pTextNode,
        css::uno::Reference< SwXText> const& i_xParent,
        const sal_Int32 nSelStart, const sal_Int32 nSelEnd)
{
    // re-use existing SwXParagraph
    // #i105557#: do not iterate over the registered clients: race condition
    rtl::Reference<SwXParagraph> xParagraph;
    if (pTextNode && (-1 == nSelStart) && (-1 == nSelEnd))
    {   // only use cache if no selection!
        xParagraph = pTextNode->GetXParagraph();
    }
    if (xParagraph.is())
    {
        return xParagraph;
    }

    // create new SwXParagraph
    css::uno::Reference<SwXText> xParentText(i_xParent);
    if (!xParentText.is() && pTextNode)
    {
        SwPosition Pos(*pTextNode);
        xParentText = ::sw::CreateParentXText( rDoc, Pos );
    }
    SwXParagraph *const pXPara( pTextNode
            ? new SwXParagraph(xParentText, *pTextNode, nSelStart, nSelEnd)
            : new SwXParagraph);
    // this is why the constructor is private: need to acquire pXPara here
    xParagraph.set(pXPara);
    // in order to initialize the weak pointer cache in the core object
    if (pTextNode && (-1 == nSelStart) && (-1 == nSelEnd))
    {
        pTextNode->SetXParagraph(xParagraph);
    }
    return xParagraph;
}

bool SwXParagraph::SelectPaM(SwPaM & rPaM)
{
    SwTextNode const*const pTextNode( GetTextNode() );

    if (!pTextNode)
    {
        return false;
    }

    rPaM.GetPoint()->Assign( *pTextNode );
    // set selection to the whole paragraph
    rPaM.SetMark();
    rPaM.GetMark()->SetContent( pTextNode->GetText().getLength() );
    return true;
}

OUString SAL_CALL
SwXParagraph::getImplementationName()
{
    return u"SwXParagraph"_ustr;
}

sal_Bool SAL_CALL
SwXParagraph::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

uno::Sequence< OUString > SAL_CALL
SwXParagraph::getSupportedServiceNames()
{
    return {
        u"com.sun.star.text.TextContent"_ustr,
        u"com.sun.star.text.Paragraph"_ustr,
        u"com.sun.star.style.CharacterProperties"_ustr,
        u"com.sun.star.style.CharacterPropertiesAsian"_ustr,
        u"com.sun.star.style.CharacterPropertiesComplex"_ustr,
        u"com.sun.star.style.ParagraphProperties"_ustr,
        u"com.sun.star.style.ParagraphPropertiesAsian"_ustr,
        u"com.sun.star.style.ParagraphPropertiesComplex"_ustr
    };
}

void
SwXParagraph::attachToText(SwXText & rParent, SwTextNode & rTextNode)
{
    OSL_ENSURE(m_bIsDescriptor, "Paragraph is not a descriptor");
    if (!m_bIsDescriptor)
        return;

    m_bIsDescriptor = false;
    moSvtListener->EndListeningAll();
    moSvtListener->StartListening(rTextNode.GetNotifier());
    rTextNode.SetXParagraph(this);
    m_xParentText = &rParent;
    if (!m_sText.isEmpty())
    {
        try { setString(m_sText); }
        catch(...){}
        m_sText.clear();
    }
}

uno::Reference< beans::XPropertySetInfo > SAL_CALL
SwXParagraph::getPropertySetInfo()
{
    SolarMutexGuard g;

    static uno::Reference< beans::XPropertySetInfo > xRef = m_rPropSet.getPropertySetInfo();
    return xRef;
}

void SAL_CALL
SwXParagraph::setPropertyValue(const OUString& rPropertyName,
        const uno::Any& rValue)
{
    SolarMutexGuard aGuard;
    // See XMLTextImportHelper::DeleteParagraph
    if (rPropertyName == "DeleteWithoutCorrection")
    {
        m_bDeleteWithoutCorrection = true;
        return;
    }
    SetPropertyValues_Impl( { rPropertyName }, { rValue } );
}

uno::Any
SwXParagraph::getPropertyValue(const OUString& rPropertyName)
{
    SolarMutexGuard aGuard;
    uno::Sequence<OUString> aPropertyNames { rPropertyName };
    const uno::Sequence< uno::Any > aRet = GetPropertyValues_Impl(aPropertyNames);
    return aRet.getConstArray()[0];
}

void SwXParagraph::SetPropertyValues_Impl(
    const uno::Sequence< OUString >& rPropertyNames,
    const uno::Sequence< uno::Any >& rValues )
{
    SwTextNode & rTextNode(GetTextNodeOrThrow());

    SwPosition aPos( rTextNode );
    SwCursor aCursor( aPos, nullptr );
    SwParaSelection aParaSel( aCursor );

    uno::Sequence< beans::PropertyValue > aValues( rPropertyNames.getLength() );
    std::transform(
        rPropertyNames.begin(), rPropertyNames.end(), rValues.begin(), aValues.getArray(),
        [&rMap = m_rPropSet.getPropertyMap(), this](const OUString& name, const uno::Any& value)
        {
            if (SfxItemPropertyMapEntry const* const pEntry = rMap.getByName(name); !pEntry)
            {
                throw beans::UnknownPropertyException("Unknown property: " + name, getXWeak());
            }
            else if (pEntry->nFlags & beans::PropertyAttribute::READONLY)
            {
                throw beans::PropertyVetoException("Property is read-only: " + name, getXWeak());
            }
            return comphelper::makePropertyValue(name, value);
        });
    SwUnoCursorHelper::SetPropertyValues(aCursor, m_rPropSet, aValues);
}

void SAL_CALL SwXParagraph::setPropertyValues(
    const uno::Sequence< OUString >& rPropertyNames,
    const uno::Sequence< uno::Any >& rValues )
{
    if (rPropertyNames.getLength() != rValues.getLength())
        throw lang::IllegalArgumentException(u"lengths do not match"_ustr,
                                             getXWeak(), -1);

    SolarMutexGuard aGuard;

    // workaround for bad designed API
    try
    {
        SetPropertyValues_Impl( rPropertyNames, rValues );
    }
    catch (const beans::UnknownPropertyException &rException)
    {
        // wrap the original (here not allowed) exception in
        // a lang::WrappedTargetException that gets thrown instead.
        lang::WrappedTargetException aWExc;
        aWExc.TargetException <<= rException;
        throw aWExc;
    }
}

// Support for DrawingLayer FillStyles for GetPropertyValue() usages
// static
void SwXParagraph::GetSinglePropertyValue_Impl(
    const SfxItemPropertyMapEntry& rEntry,
    const SfxItemSet& rSet,
    uno::Any& rAny )
{
    bool bDone(false);

    switch(rEntry.nWID)
    {
        case RES_BACKGROUND:
        {
            const std::unique_ptr<SvxBrushItem> aOriginalBrushItem(getSvxBrushItemFromSourceSet(rSet, RES_BACKGROUND));

            if(!aOriginalBrushItem->QueryValue(rAny, rEntry.nMemberId))
            {
                OSL_ENSURE(false, "Error getting attribute from RES_BACKGROUND (!)");
            }

            bDone = true;
            break;
        }
        case OWN_ATTR_FILLBMP_MODE:
        {
            if (rSet.Get(XATTR_FILLBMP_TILE).GetValue())
            {
                rAny <<= drawing::BitmapMode_REPEAT;
            }
            else if (rSet.Get(XATTR_FILLBMP_STRETCH).GetValue())
            {
                rAny <<= drawing::BitmapMode_STRETCH;
            }
            else
            {
                rAny <<= drawing::BitmapMode_NO_REPEAT;
            }

            bDone = true;
            break;
        }
        default: break;
    }

    if(bDone)
        return;

    // fallback to standard get value implementation used before this helper was created
    SfxItemPropertySet::getPropertyValue(rEntry, rSet, rAny);

    if(rEntry.aType == cppu::UnoType<sal_Int16>::get() && rEntry.aType != rAny.getValueType())
    {
        // since the sfx uInt16 item now exports a sal_Int32, we may have to fix this here
        sal_Int32 nValue(0);

        if (rAny >>= nValue)
        {
            rAny <<= static_cast<sal_Int16>(nValue);
        }
    }

    // check for needed metric translation
    if(!(rEntry.nMoreFlags & PropertyMoreFlags::METRIC_ITEM))
        return;

    bool bDoIt(true);

    if(XATTR_FILLBMP_SIZEX == rEntry.nWID || XATTR_FILLBMP_SIZEY == rEntry.nWID)
    {
        // exception: If these ItemTypes are used, do not convert when these are negative
        // since this means they are intended as percent values
        sal_Int32 nValue = 0;

        if(rAny >>= nValue)
        {
            bDoIt = nValue > 0;
        }
    }

    if(bDoIt)
    {
        const MapUnit eMapUnit(rSet.GetPool()->GetMetric(rEntry.nWID));

        if(eMapUnit != MapUnit::Map100thMM)
        {
            SvxUnoConvertToMM(eMapUnit, rAny);
        }
    }
}

uno::Sequence< uno::Any > SwXParagraph::GetPropertyValues_Impl(
        const uno::Sequence< OUString > & rPropertyNames )
{
    SwTextNode & rTextNode(GetTextNodeOrThrow());

    uno::Sequence< uno::Any > aValues(rPropertyNames.getLength());
    SwPaM aPam( rTextNode );
    uno::Any* pValues = aValues.getArray();
    const OUString* pPropertyNames = rPropertyNames.getConstArray();
    const SfxItemPropertyMap &rMap = m_rPropSet.getPropertyMap();
    const SwAttrSet& rAttrSet( rTextNode.GetSwAttrSet() );
    for (sal_Int32 nProp = 0; nProp < rPropertyNames.getLength(); nProp++)
    {
        if (pPropertyNames[nProp] == "ParaMarkerAutoStyleSpan")
        {
            // A hack to tunnel the fake text span to ODF export
            // see XMLTextParagraphExport::exportParagraph
            if (rTextNode.GetAttr(RES_PARATR_LIST_AUTOFMT).GetStyleHandle())
            {
                SwUnoCursor aEndCursor(*aPam.GetMark());
                css::uno::Reference<css::beans::XPropertySet> xFakeSpan(
                    new SwXTextPortion(&aEndCursor, {}, PORTION_LIST_AUTOFMT));
                pValues[nProp] <<= xFakeSpan;
            }
            continue;
        }

        if (pPropertyNames[nProp] == "ODFExport_NodeIndex")
        {
            // A hack to avoid writing random list ids to ODF when they are not referred later
            // see XMLTextParagraphExport::DocumentListNodes::ShouldSkipListId
            pValues[nProp] <<= rTextNode.GetIndex().get();
            continue;
        }

        if (pPropertyNames[nProp] == "OOXMLImport_AnchoredShapes")
        {
            // A hack to provide list of anchored objects fast
            // See reanchorObjects in writerfilter/source/dmapper/DomainMapper_Impl.cxx
            FrameClientSortList_t aFrames;
            CollectFrameAtNode(rTextNode, aFrames, false); // Frames anchored to paragraph
            CollectFrameAtNode(rTextNode, aFrames, true); // Frames anchored to character
            std::vector<uno::Reference<text::XTextContent>> aRet;
            aRet.reserve(aFrames.size());
            for (const auto& rFrame : aFrames)
                if (auto xContent = FrameClientToXTextContent(rFrame.pFrameClient.get()))
                    aRet.push_back(xContent);

            pValues[nProp] <<= comphelper::containerToSequence(aRet);
            continue;
        }

        SfxItemPropertyMapEntry const*const pEntry =
            rMap.getByName( pPropertyNames[nProp] );
        if (!pEntry)
        {
            throw beans::UnknownPropertyException(
                "Unknown property: " + pPropertyNames[nProp], getXWeak());
        }
        if (! ::sw::GetDefaultTextContentValue(
                pValues[nProp], pPropertyNames[nProp], pEntry->nWID))
        {
            beans::PropertyState eTemp;
            const bool bDone = SwUnoCursorHelper::getCursorPropertyValue(
                *pEntry, aPam, &(pValues[nProp]), eTemp, &rTextNode );
            if (!bDone)
            {
                GetSinglePropertyValue_Impl(*pEntry, rAttrSet, pValues[nProp]);
            }
        }
    }
    return aValues;
}

uno::Sequence< uno::Any > SAL_CALL
SwXParagraph::getPropertyValues(const uno::Sequence< OUString >& rPropertyNames)
{
    SolarMutexGuard aGuard;
    uno::Sequence< uno::Any > aValues;

    // workaround for bad designed API
    try
    {
        aValues = GetPropertyValues_Impl( rPropertyNames );
    }
    catch (beans::UnknownPropertyException &)
    {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw css::lang::WrappedTargetRuntimeException(u"Unknown property exception caught"_ustr,
                getXWeak(), anyEx );
    }
    catch (lang::WrappedTargetException &)
    {
        css::uno::Any anyEx = cppu::getCaughtException();
        throw css::lang::WrappedTargetRuntimeException(u"WrappedTargetException caught"_ustr,
                getXWeak(), anyEx );
    }

    return aValues;
}

void SAL_CALL SwXParagraph::addPropertiesChangeListener(
    const uno::Sequence< OUString >& /*aPropertyNames*/,
    const uno::Reference< beans::XPropertiesChangeListener >& /*xListener*/ )
{
    OSL_FAIL("SwXParagraph::addPropertiesChangeListener(): not implemented");
}

void SAL_CALL SwXParagraph::removePropertiesChangeListener(
    const uno::Reference< beans::XPropertiesChangeListener >& /*xListener*/ )
{
    OSL_FAIL("SwXParagraph::removePropertiesChangeListener(): not implemented");
}

void SAL_CALL SwXParagraph::firePropertiesChangeEvent(
    const uno::Sequence< OUString >& /*aPropertyNames*/,
    const uno::Reference< beans::XPropertiesChangeListener >& /*xListener*/ )
{
    OSL_FAIL("SwXParagraph::firePropertiesChangeEvent(): not implemented");
}

/* disabled for #i46921# */

uno::Sequence< beans::SetPropertyTolerantFailed > SAL_CALL
SwXParagraph::setPropertyValuesTolerant(
        const uno::Sequence< OUString >& rPropertyNames,
        const uno::Sequence< uno::Any >& rValues )
{
    SolarMutexGuard aGuard;

    if (rPropertyNames.getLength() != rValues.getLength())
    {
        throw lang::IllegalArgumentException();
    }

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    //SwNode& rTextNode = pUnoCursor->GetPoint()->GetNode();
    //const SwAttrSet& rAttrSet = static_cast<SwTextNode&>(rTextNode).GetSwAttrSet();
    //sal_uInt16 nAttrCount = rAttrSet.Count();

    const sal_Int32 nProps = rPropertyNames.getLength();
    const OUString *pProp = rPropertyNames.getConstArray();

    //sal_Int32 nVals = rValues.getLength();
    const uno::Any *pValue = rValues.getConstArray();

    sal_Int32 nFailed = 0;
    uno::Sequence< beans::SetPropertyTolerantFailed > aFailed( nProps );
    beans::SetPropertyTolerantFailed *pFailed = aFailed.getArray();

    // get entry to start with
    const SfxItemPropertyMap &rPropMap = m_rPropSet.getPropertyMap();

    SwPosition aPos( rTextNode );
    SwCursor aCursor( aPos, nullptr );
    SwParaSelection aParaSel( aCursor );
    for (sal_Int32 i = 0;  i < nProps;  ++i)
    {
        try
        {
            pFailed[ nFailed ].Name = pProp[i];

            SfxItemPropertyMapEntry const*const pEntry =
                rPropMap.getByName( pProp[i] );
            if (!pEntry)
            {
                pFailed[ nFailed++ ].Result  =
                    beans::TolerantPropertySetResultType::UNKNOWN_PROPERTY;
            }
            else
            {
                // set property value
                // (compare to SwXParagraph::setPropertyValues)
                if (pEntry->nFlags & beans::PropertyAttribute::READONLY)
                {
                    pFailed[ nFailed++ ].Result  =
                        beans::TolerantPropertySetResultType::PROPERTY_VETO;
                }
                else
                {
                    SwUnoCursorHelper::SetPropertyValue(
                        aCursor, m_rPropSet, pProp[i], pValue[i]);
                }
            }
        }
        catch (beans::UnknownPropertyException &)
        {
            // should not occur because property was searched for before
            TOOLS_WARN_EXCEPTION( "sw", "unexpected exception caught" );
            pFailed[ nFailed++ ].Result =
                beans::TolerantPropertySetResultType::UNKNOWN_PROPERTY;
        }
        catch (lang::IllegalArgumentException &)
        {
            pFailed[ nFailed++ ].Result =
                beans::TolerantPropertySetResultType::ILLEGAL_ARGUMENT;
        }
        catch (beans::PropertyVetoException &)
        {
            pFailed[ nFailed++ ].Result =
                beans::TolerantPropertySetResultType::PROPERTY_VETO;
        }
        catch (lang::WrappedTargetException &)
        {
            pFailed[ nFailed++ ].Result =
                beans::TolerantPropertySetResultType::WRAPPED_TARGET;
        }
    }

    aFailed.realloc( nFailed );
    return aFailed;
}

uno::Sequence< beans::GetPropertyTolerantResult > SAL_CALL
SwXParagraph::getPropertyValuesTolerant(
        const uno::Sequence< OUString >& rPropertyNames )
{
    SolarMutexGuard aGuard;

    const uno::Sequence< beans::GetDirectPropertyTolerantResult > aTmpRes(
        GetPropertyValuesTolerant_Impl( rPropertyNames, false ) );

    // copy temporary result to final result type
    const sal_Int32 nLen = aTmpRes.getLength();
    uno::Sequence< beans::GetPropertyTolerantResult > aRes( nLen );
    std::copy(aTmpRes.begin(), aTmpRes.end(), aRes.getArray());
    return aRes;
}

uno::Sequence< beans::GetDirectPropertyTolerantResult > SAL_CALL
SwXParagraph::getDirectPropertyValuesTolerant(
        const uno::Sequence< OUString >& rPropertyNames )
{
    SolarMutexGuard aGuard;

    return GetPropertyValuesTolerant_Impl( rPropertyNames, true );
}

uno::Sequence< beans::GetDirectPropertyTolerantResult >
SwXParagraph::GetPropertyValuesTolerant_Impl(
        const uno::Sequence< OUString >& rPropertyNames,
        bool bDirectValuesOnly )
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    // #i46786# Use SwAttrSet pointer for determining the state.
    //          Use the value SwAttrSet (from the paragraph OR the style)
    //          for determining the actual value(s).
    const SwAttrSet* pAttrSet = rTextNode.GetpSwAttrSet();
    const SwAttrSet& rValueAttrSet = rTextNode.GetSwAttrSet();

    sal_Int32 nProps = rPropertyNames.getLength();

    uno::Sequence< beans::GetDirectPropertyTolerantResult > aResult( nProps );
    beans::GetDirectPropertyTolerantResult *pResult = aResult.getArray();
    sal_Int32 nIdx = 0;

    // get entry to start with
    const SfxItemPropertyMap &rPropMap = m_rPropSet.getPropertyMap();

    for (const OUString& rProp : rPropertyNames)
    {
        OSL_ENSURE( nIdx < nProps, "index out of bounds" );
        beans::GetDirectPropertyTolerantResult &rResult = pResult[nIdx];

        try
        {
            rResult.Name = rProp;

            SfxItemPropertyMapEntry const*const pEntry =
                rPropMap.getByName( rProp );
            if (!pEntry)  // property available?
            {
                rResult.Result =
                    beans::TolerantPropertySetResultType::UNKNOWN_PROPERTY;
            }
            else
            {
                // get property state
                // (compare to SwXParagraph::getPropertyState)
                bool bAttrSetFetched = true;
                beans::PropertyState eState = lcl_SwXParagraph_getPropertyState(
                            rTextNode, &pAttrSet, *pEntry, bAttrSetFetched );
                rResult.State  = eState;

                rResult.Result = beans::TolerantPropertySetResultType::UNKNOWN_FAILURE;
                if (!bDirectValuesOnly ||
                    (beans::PropertyState_DIRECT_VALUE == eState))
                {
                    // get property value
                    // (compare to SwXParagraph::getPropertyValue(s))
                    uno::Any aValue;
                    if (! ::sw::GetDefaultTextContentValue(
                                aValue, rProp, pEntry->nWID ) )
                    {
                        SwPaM aPam( rTextNode );
                        // handle properties that are not part of the attribute
                        // and thus only pretending to be paragraph attributes
                        beans::PropertyState eTemp;
                        const bool bDone =
                            SwUnoCursorHelper::getCursorPropertyValue(
                                    *pEntry, aPam, &aValue, eTemp, &rTextNode );

                        // if not found try the real paragraph attributes...
                        if (!bDone)
                        {
                            GetSinglePropertyValue_Impl(*pEntry, rValueAttrSet, aValue);
                        }
                    }

                    rResult.Value  = std::move(aValue);
                    rResult.Result = beans::TolerantPropertySetResultType::SUCCESS;

                    nIdx++;
                }
                // this assertion should never occur!
                OSL_ENSURE( nIdx < 1  ||  pResult[nIdx - 1].Result != beans::TolerantPropertySetResultType::UNKNOWN_FAILURE,
                        "unknown failure while retrieving property" );

            }
        }
        catch (beans::UnknownPropertyException &)
        {
            // should not occur because property was searched for before
            TOOLS_WARN_EXCEPTION( "sw", "unexpected exception caught" );
            rResult.Result = beans::TolerantPropertySetResultType::UNKNOWN_PROPERTY;
        }
        catch (lang::IllegalArgumentException &)
        {
            rResult.Result = beans::TolerantPropertySetResultType::ILLEGAL_ARGUMENT;
        }
        catch (beans::PropertyVetoException &)
        {
            rResult.Result = beans::TolerantPropertySetResultType::PROPERTY_VETO;
        }
        catch (lang::WrappedTargetException &)
        {
            rResult.Result = beans::TolerantPropertySetResultType::WRAPPED_TARGET;
        }
    }

    // resize to actually used size
    aResult.realloc( nIdx );

    return aResult;
}

bool ::sw::GetDefaultTextContentValue(
        uno::Any& rAny, std::u16string_view rPropertyName, sal_uInt16 nWID)
{
    if(!nWID)
    {
        if(rPropertyName == UNO_NAME_ANCHOR_TYPE)
            nWID = FN_UNO_ANCHOR_TYPE;
        else if(rPropertyName == UNO_NAME_ANCHOR_TYPES)
            nWID = FN_UNO_ANCHOR_TYPES;
        else if(rPropertyName == UNO_NAME_TEXT_WRAP)
            nWID = FN_UNO_TEXT_WRAP;
        else
            return false;
    }

    switch(nWID)
    {
        case FN_UNO_TEXT_WRAP:  rAny <<= text::WrapTextMode_NONE; break;
        case FN_UNO_ANCHOR_TYPE: rAny <<= text::TextContentAnchorType_AT_PARAGRAPH; break;
        case FN_UNO_ANCHOR_TYPES:
        {   uno::Sequence<text::TextContentAnchorType> aTypes { text::TextContentAnchorType_AT_PARAGRAPH };
            rAny <<= aTypes;
        }
        break;
        default:
            return false;
    }
    return true;
}

void SAL_CALL
SwXParagraph::addPropertyChangeListener(
        const OUString& /*rPropertyName*/,
        const uno::Reference< beans::XPropertyChangeListener >& /*xListener*/)
{
    OSL_FAIL("SwXParagraph::addPropertyChangeListener(): not implemented");
}

void SAL_CALL
SwXParagraph::removePropertyChangeListener(
        const OUString& /*rPropertyName*/,
        const uno::Reference< beans::XPropertyChangeListener >& /*xListener*/)
{
    OSL_FAIL("SwXParagraph::removePropertyChangeListener(): not implemented");
}

void SAL_CALL
SwXParagraph::addVetoableChangeListener(
        const OUString& /*rPropertyName*/,
        const uno::Reference< beans::XVetoableChangeListener >& /*xListener*/)
{
    OSL_FAIL("SwXParagraph::addVetoableChangeListener(): not implemented");
}

void SAL_CALL
SwXParagraph::removeVetoableChangeListener(
        const OUString& /*rPropertyName*/,
        const uno::Reference< beans::XVetoableChangeListener >& /*xListener*/)
{
    OSL_FAIL("SwXParagraph::removeVetoableChangeListener(): not implemented");
}

static beans::PropertyState lcl_SwXParagraph_getPropertyState(
    const SwTextNode& rTextNode,
    const SwAttrSet** ppSet,
    const SfxItemPropertyMapEntry& rEntry,
    bool &rAttrSetFetched)
{
    beans::PropertyState eRet(beans::PropertyState_DEFAULT_VALUE);

    if(!(*ppSet) && !rAttrSetFetched)
    {
        (*ppSet) = rTextNode.GetpSwAttrSet();
        rAttrSetFetched = true;
    }

    SwPosition aPos(rTextNode);
    SwPaM aPam(aPos);
    bool bDone(false);

    switch(rEntry.nWID)
    {
        case FN_UNO_NUM_RULES:
        {
            // if numbering is set, return it; else do nothing
            SwUnoCursorHelper::getNumberingProperty(aPam,eRet,nullptr);
            bDone = true;
            break;
        }
        case FN_UNO_ANCHOR_TYPES:
        {
            bDone = true;
            break;
        }
        case RES_ANCHOR:
        {
            bDone = (MID_SURROUND_SURROUNDTYPE == rEntry.nMemberId);
            break;
        }
        case RES_SURROUND:
        {
            bDone = (MID_ANCHOR_ANCHORTYPE == rEntry.nMemberId);
            break;
        }
        case FN_UNO_PARA_STYLE:
        case FN_UNO_PARA_CONDITIONAL_STYLE_NAME:
        {
            SwFormatColl* pFormat = SwUnoCursorHelper::GetCurTextFormatColl(aPam,rEntry.nWID == FN_UNO_PARA_CONDITIONAL_STYLE_NAME);
            eRet = pFormat ? beans::PropertyState_DIRECT_VALUE : beans::PropertyState_AMBIGUOUS_VALUE;
            bDone = true;
            break;
        }
        case FN_UNO_PAGE_STYLE:
        {
            ProgName sVal;
            SwUnoCursorHelper::GetCurPageStyle( aPam, sVal );
            eRet = !sVal.isEmpty() ? beans::PropertyState_DIRECT_VALUE
                              : beans::PropertyState_AMBIGUOUS_VALUE;
            bDone = true;
            break;
        }

        // DrawingLayer PropertyStyle support
        case OWN_ATTR_FILLBMP_MODE:
        {
            if(*ppSet)
            {
                if(SfxItemState::SET == (*ppSet)->GetItemState(XATTR_FILLBMP_STRETCH, false)
                    || SfxItemState::SET == (*ppSet)->GetItemState(XATTR_FILLBMP_TILE, false))
                {
                    eRet = beans::PropertyState_DIRECT_VALUE;
                }
                else
                {
                    eRet = beans::PropertyState_AMBIGUOUS_VALUE;
                }

                bDone = true;
            }
            break;
        }
        case RES_BACKGROUND:
        {
            if(*ppSet)
            {
                if (SWUnoHelper::needToMapFillItemsToSvxBrushItemTypes(**ppSet,
                        rEntry.nMemberId))
                {
                    eRet = beans::PropertyState_DIRECT_VALUE;
                }
                bDone = true;
            }
            break;
        }
    }

    if(!bDone)
    {
        if((*ppSet) && SfxItemState::SET == (*ppSet)->GetItemState(rEntry.nWID, false))
        {
            eRet = beans::PropertyState_DIRECT_VALUE;
        }
    }

    return eRet;
}

beans::PropertyState SAL_CALL
SwXParagraph::getPropertyState(const OUString& rPropertyName)
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    const SwAttrSet* pSet = nullptr;
    SfxItemPropertyMapEntry const*const pEntry =
        m_rPropSet.getPropertyMap().getByName(rPropertyName);
    if (!pEntry)
    {
        throw beans::UnknownPropertyException(
            "Unknown property: " + rPropertyName,
            getXWeak());
    }
    bool bDummy = false;
    const beans::PropertyState eRet =
        lcl_SwXParagraph_getPropertyState(rTextNode, &pSet, *pEntry, bDummy);
    return eRet;
}

uno::Sequence< beans::PropertyState > SAL_CALL
SwXParagraph::getPropertyStates(
        const uno::Sequence< OUString >& PropertyNames)
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    const OUString* pNames = PropertyNames.getConstArray();
    uno::Sequence< beans::PropertyState > aRet(PropertyNames.getLength());
    beans::PropertyState* pStates = aRet.getArray();
    const SfxItemPropertyMap &rMap = m_rPropSet.getPropertyMap();
    const SwAttrSet* pSet = nullptr;
    bool bAttrSetFetched = false;

    for (sal_Int32 i = 0, nEnd = PropertyNames.getLength(); i < nEnd;
            ++i, ++pStates, ++pNames)
    {
        SfxItemPropertyMapEntry const*const pEntry =
            rMap.getByName( *pNames );
        if (!pEntry)
        {
            throw beans::UnknownPropertyException(
                "Unknown property: " + *pNames,
                getXWeak());
        }

        if (bAttrSetFetched && !pSet && isATR(pEntry->nWID))
        {
            *pStates = beans::PropertyState_DEFAULT_VALUE;
        }
        else
        {
            *pStates = lcl_SwXParagraph_getPropertyState(
                rTextNode, &pSet, *pEntry, bAttrSetFetched );
        }
    }

    return aRet;
}

void SAL_CALL
SwXParagraph::setPropertyToDefault(const OUString& rPropertyName)
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    SwPosition aPos( rTextNode );
    SwCursor aCursor( aPos, nullptr );
    if (rPropertyName == UNO_NAME_ANCHOR_TYPE  ||
        rPropertyName == UNO_NAME_ANCHOR_TYPES ||
        rPropertyName == UNO_NAME_TEXT_WRAP)
    {
        return;
    }

    // select paragraph
    SwParaSelection aParaSel( aCursor );
    SfxItemPropertyMapEntry const*const pEntry =
        m_rPropSet.getPropertyMap().getByName( rPropertyName );
    if (!pEntry)
    {
        throw beans::UnknownPropertyException(
            "Unknown property: " + rPropertyName,
            getXWeak());
    }

    if (pEntry->nFlags & beans::PropertyAttribute::READONLY)
    {
        throw uno::RuntimeException(
            "Property is read-only: " + rPropertyName,
            getXWeak());
    }

    const bool bBelowFrameAtrEnd(pEntry->nWID < RES_FRMATR_END);
    const bool bDrawingLayerRange(XATTR_FILL_FIRST <= pEntry->nWID && XATTR_FILL_LAST >= pEntry->nWID);

    if(bBelowFrameAtrEnd || bDrawingLayerRange)
    {
        o3tl::sorted_vector<sal_uInt16> aWhichIds;

        // For FillBitmapMode two IDs have to be reset (!)
        if(OWN_ATTR_FILLBMP_MODE == pEntry->nWID)
        {
            aWhichIds.insert(XATTR_FILLBMP_STRETCH);
            aWhichIds.insert(XATTR_FILLBMP_TILE);
        }
        else
        {
            aWhichIds.insert(pEntry->nWID);
        }

        if (pEntry->nWID < RES_PARATR_BEGIN)
        {
            aCursor.GetDoc().ResetAttrs(aCursor, true, aWhichIds);
        }
        else
        {
            // for paragraph attributes the selection must be extended
            // to paragraph boundaries
            SwPosition aStart( *aCursor.Start() );
            SwPosition aEnd  ( *aCursor.End()   );
            auto pTemp( aCursor.GetDoc().CreateUnoCursor(aStart) );
            if(!SwUnoCursorHelper::IsStartOfPara(*pTemp))
            {
                pTemp->MovePara(GoCurrPara, fnParaStart);
            }

            pTemp->SetMark();
            *pTemp->GetPoint() = std::move(aEnd);

            SwUnoCursorHelper::SelectPam(*pTemp, true);

            if (!SwUnoCursorHelper::IsEndOfPara(*pTemp))
            {
                pTemp->MovePara(GoCurrPara, fnParaEnd);
            }


            pTemp->GetDoc().ResetAttrs(*pTemp, true, aWhichIds);
        }
    }
    else
    {
        SwUnoCursorHelper::resetCursorPropertyValue(*pEntry, aCursor);
    }
}

uno::Any SAL_CALL
SwXParagraph::getPropertyDefault(const OUString& rPropertyName)
{
    SolarMutexGuard g;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    uno::Any aRet;
    if (::sw::GetDefaultTextContentValue(aRet, rPropertyName))
    {
        return aRet;
    }

    SfxItemPropertyMapEntry const*const pEntry =
        m_rPropSet.getPropertyMap().getByName(rPropertyName);
    if (!pEntry)
    {
        throw beans::UnknownPropertyException(
            "Unknown property: " + rPropertyName,
            getXWeak());
    }

    const bool bBelowFrameAtrEnd(pEntry->nWID < RES_FRMATR_END);
    const bool bDrawingLayerRange(XATTR_FILL_FIRST <= pEntry->nWID && XATTR_FILL_LAST >= pEntry->nWID);

    if(bBelowFrameAtrEnd || bDrawingLayerRange)
    {
        const SfxPoolItem& rDefItem = rTextNode.GetDoc().GetAttrPool().GetUserOrPoolDefaultItem(pEntry->nWID);

        rDefItem.QueryValue(aRet, pEntry->nMemberId);
    }

    return aRet;
}

void SAL_CALL
SwXParagraph::attach(const uno::Reference< text::XTextRange > & /*xTextRange*/)
{
    // SwXParagraph will only created in order to be inserted by
    // 'insertTextContentBefore' or 'insertTextContentAfter' therefore
    // they cannot be attached
    throw uno::RuntimeException();
}

uno::Reference< text::XTextRange > SAL_CALL
SwXParagraph::getAnchor()
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    SwPosition aPos( rTextNode );
    SwCursor aCursor( aPos, nullptr );
    // select paragraph
    SwParaSelection aParaSel( aCursor );
    const uno::Reference< text::XTextRange >  xRet =
        new SwXTextRange(aCursor, m_xParentText);
    return xRet;
}

void SAL_CALL SwXParagraph::dispose()
{
    SolarMutexGuard aGuard;

    SwTextNode *const pTextNode( m_pTextNode );
    if (pTextNode)
    {
        SwCursor aCursor( SwPosition( *pTextNode ), nullptr );
        {
            auto& rDoc = pTextNode->GetDoc();
            comphelper::ScopeGuard aGuard2(
                [&rDoc, restore = rDoc.SetDontCorrectBookmarks(m_bDeleteWithoutCorrection)]()
                { rDoc.SetDontCorrectBookmarks(restore); });
            rDoc.getIDocumentContentOperations().DelFullPara(aCursor);
        }
        lang::EventObject const ev(getXWeak());
        std::unique_lock aGuard2(m_Mutex);
        m_EventListeners.disposeAndClear(aGuard2, ev);
    }
}

void SAL_CALL SwXParagraph::addEventListener(
        const uno::Reference< lang::XEventListener > & xListener)
{
    // no need to lock here as m_pImpl is const and container threadsafe
    std::unique_lock aGuard(m_Mutex);
    m_EventListeners.addInterface(aGuard, xListener);
}

void SAL_CALL SwXParagraph::removeEventListener(
        const uno::Reference< lang::XEventListener > & xListener)
{
    // no need to lock here as m_pImpl is const and container threadsafe
    std::unique_lock aGuard(m_Mutex);
    m_EventListeners.removeInterface(aGuard, xListener);
}

uno::Reference< container::XEnumeration >  SAL_CALL
SwXParagraph::createEnumeration()
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    SwPaM aPam ( rTextNode );
    const uno::Reference< container::XEnumeration > xRef =
        new SwXTextPortionEnumeration(aPam, m_xParentText,
            m_nSelectionStartPos, m_nSelectionEndPos);
    return xRef;
}

 /// tries to return less data, but may return more than just text fields
rtl::Reference< SwXTextPortionEnumeration >
SwXParagraph::createTextFieldsEnumeration()
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());
    SwPaM aPam ( rTextNode );

    return new SwXTextPortionEnumeration(aPam, m_xParentText,
            m_nSelectionStartPos, m_nSelectionEndPos, /*bOnlyTextFields*/true);
}

uno::Type SAL_CALL SwXParagraph::getElementType()
{
    return cppu::UnoType<text::XTextRange>::get();
}

sal_Bool SAL_CALL SwXParagraph::hasElements()
{
    SolarMutexGuard aGuard;
    return GetTextNode() != nullptr;
}

uno::Reference< text::XText > SAL_CALL
SwXParagraph::getText()
{
    SolarMutexGuard g;

    return m_xParentText;
}

uno::Reference< text::XTextRange > SAL_CALL
SwXParagraph::getStart()
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    SwPosition aPos( rTextNode );
    SwCursor aCursor( aPos, nullptr );
    SwParaSelection aParaSel( aCursor );
    SwPaM aPam( *aCursor.Start() );
    uno::Reference< text::XText >  xParent = getText();
    const uno::Reference< text::XTextRange > xRet =
        new SwXTextRange(aPam, xParent);
    return xRet;
}

uno::Reference< text::XTextRange > SAL_CALL
SwXParagraph::getEnd()
{
    SolarMutexGuard aGuard;

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    SwPosition aPos( rTextNode );
    SwCursor aCursor( aPos, nullptr );
    SwParaSelection aParaSel( aCursor );
    SwPaM aPam( *aCursor.End() );
    uno::Reference< text::XText >  xParent = getText();
    const uno::Reference< text::XTextRange > xRet =
        new SwXTextRange(aPam, xParent);
    return xRet;
}

OUString SAL_CALL SwXParagraph::getString()
{
    SolarMutexGuard aGuard;
    OUString aRet;
    SwTextNode const*const pTextNode( GetTextNode() );
    if (pTextNode)
    {
        SwPosition aPos( *pTextNode );
        SwCursor aCursor( aPos, nullptr );
        SwParaSelection aParaSel( aCursor );
        SwUnoCursorHelper::GetTextFromPam(aCursor, aRet);
    }
    else if (IsDescriptor())
    {
        aRet = m_sText;
    }
    else
    {
        // Seems object is being disposed or some other problem occurs.
        // Anyway from user point of view object still exist, so on that level this is not an error
        SAL_WARN("sw.uno", "getString() for invalid paragraph called. Returning empty string.");
    }
    return aRet;
}

void SAL_CALL SwXParagraph::setString(const OUString& aString)
{
    SolarMutexGuard aGuard;

    SwTextNode const*const pTextNode( GetTextNode() );
    if (pTextNode)
    {
        SwPosition aPos( *pTextNode );
        SwCursor aCursor( aPos, nullptr );
        if (!SwUnoCursorHelper::IsStartOfPara(aCursor)) {
            aCursor.MovePara(GoCurrPara, fnParaStart);
        }
        SwUnoCursorHelper::SelectPam(aCursor, true);
        if (pTextNode->GetText().getLength()) {
            aCursor.MovePara(GoCurrPara, fnParaEnd);
        }
        SwUnoCursorHelper::SetString(aCursor, aString);
        SwUnoCursorHelper::SelectPam(aCursor, false);
    }
    else if (IsDescriptor())
    {
        m_sText = aString;
    }
    else
    {
        throw uno::RuntimeException();
    }
}

uno::Reference< container::XEnumeration > SAL_CALL
SwXParagraph::createContentEnumeration(const OUString& rServiceName)
{
    SolarMutexGuard g;

    if ( rServiceName != "com.sun.star.text.TextContent" )
    {
        throw uno::RuntimeException();
    }

    SwTextNode & rTextNode(GetTextNodeOrThrow());

    SwPaM aPam( rTextNode );
    rtl::Reference< SwXParaFrameEnumeration > xRet =
        SwXParaFrameEnumeration::Create(aPam, PARAFRAME_PORTION_PARAGRAPH);
    return xRet;
}

uno::Sequence< OUString > SAL_CALL
SwXParagraph::getAvailableServiceNames()
{
    uno::Sequence<OUString> aRet { u"com.sun.star.text.TextContent"_ustr };
    return aRet;
}

// MetadatableMixin
::sfx2::Metadatable* SwXParagraph::GetCoreObject()
{
    return m_pTextNode;
}

uno::Reference<frame::XModel> SwXParagraph::GetModel()
{
    SwTextNode *const pTextNode( m_pTextNode );
    if (pTextNode)
    {
        SwDocShell const*const pShell( pTextNode->GetDoc().GetDocShell() );
        return pShell ? pShell->GetModel() : nullptr;
    }
    return nullptr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
