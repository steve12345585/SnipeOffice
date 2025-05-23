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

#include <RptUndo.hxx>
#include <strings.hxx>
#include <rptui_slotid.hrc>
#include <UITools.hxx>
#include <UndoEnv.hxx>

#include <dbaccess/IController.hxx>
#include <com/sun/star/report/XSection.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>

#include <com/sun/star/awt/Point.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <comphelper/propertyvalue.hxx>
#include <comphelper/types.hxx>
#include <svx/unoshape.hxx>
#include <utility>
#include <comphelper/diagnose_ex.hxx>

#include <functional>

namespace rptui
{
    using namespace ::com::sun::star;
    using namespace uno;
    using namespace beans;
    using namespace awt;
    using namespace container;
    using namespace report;


namespace
{
    void lcl_collectElements(const uno::Reference< report::XSection >& _xSection,::std::vector< uno::Reference< drawing::XShape> >& _rControls)
    {
        if ( _xSection.is() )
        {
            sal_Int32 nCount = _xSection->getCount();
            _rControls.reserve(nCount);
            while ( nCount )
            {
                uno::Reference< drawing::XShape> xShape(_xSection->getByIndex(nCount-1),uno::UNO_QUERY);
                _rControls.push_back(xShape);
                _xSection->remove(xShape);
                --nCount;
            }
        }
    }

    void lcl_insertElements(const uno::Reference< report::XSection >& _xSection,const ::std::vector< uno::Reference< drawing::XShape> >& _aControls)
    {
        if ( !_xSection.is() )
            return;

        ::std::vector< uno::Reference< drawing::XShape> >::const_reverse_iterator aIter = _aControls.rbegin();
        ::std::vector< uno::Reference< drawing::XShape> >::const_reverse_iterator aEnd = _aControls.rend();
        for (; aIter != aEnd; ++aIter)
        {
            try
            {
                const awt::Point aPos = (*aIter)->getPosition();
                const awt::Size aSize = (*aIter)->getSize();
                _xSection->add(*aIter);
                (*aIter)->setPosition( aPos );
                (*aIter)->setSize( aSize );
            }
            catch(const uno::Exception&)
            {
                TOOLS_WARN_EXCEPTION( "reportdesign", "lcl_insertElements");
            }
        }
    }

    void lcl_setValues(const uno::Reference< report::XSection >& _xSection,const ::std::vector< ::std::pair< OUString ,uno::Any> >& _aValues)
    {
        if ( !_xSection.is() )
            return;

        for (const auto& [rPropName, rValue] : _aValues)
        {
            try
            {
                _xSection->setPropertyValue(rPropName, rValue);
            }
            catch(const uno::Exception&)
            {
                TOOLS_WARN_EXCEPTION( "reportdesign", "lcl_setValues");
            }
        }
    }
}


OSectionUndo::OSectionUndo(OReportModel& _rMod
                           ,sal_uInt16 _nSlot
                           ,Action _eAction
                           ,TranslateId pCommentID)
: OCommentUndoAction(_rMod,pCommentID)
,m_eAction(_eAction)
,m_nSlot(_nSlot)
,m_bInserted(false)
{
}

OSectionUndo::~OSectionUndo()
{
    if ( m_bInserted )
        return;

    OXUndoEnvironment& rEnv = static_cast< OReportModel& >( m_rMod ).GetUndoEnv();
    for (uno::Reference<drawing::XShape>& xShape : m_aControls)
    {
        rEnv.RemoveElement(xShape);
        try
        {
            comphelper::disposeComponent(xShape);
        }
        catch(const uno::Exception &)
        {
            TOOLS_WARN_EXCEPTION( "reportdesign", "");
        }
    }
}

void OSectionUndo::collectControls(const uno::Reference< report::XSection >& _xSection)
{
    m_aControls.clear();
    try
    {
        // copy all properties for restoring
        uno::Reference< beans::XPropertySetInfo> xInfo = _xSection->getPropertySetInfo();
        const uno::Sequence< beans::Property> aSeq = xInfo->getProperties();
        for(const beans::Property& rProp : aSeq)
        {
            if ( 0 == (rProp.Attributes & beans::PropertyAttribute::READONLY) )
                m_aValues.emplace_back(rProp.Name,_xSection->getPropertyValue(rProp.Name));
        }
        lcl_collectElements(_xSection,m_aControls);
    }
    catch(uno::Exception&)
    {
    }
}

void OSectionUndo::Undo()
{
    try
    {
        switch ( m_eAction )
        {
        case Inserted:
            implReRemove();
            break;

        case Removed:
            implReInsert();
            break;
        }
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "reportdesign", "OSectionUndo::Undo" );
    }
}

void OSectionUndo::Redo()
{
    try
    {
        switch ( m_eAction )
        {
        case Inserted:
            implReInsert();
            break;

        case Removed:
            implReRemove();
            break;
        }
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "reportdesign", "OSectionUndo::Redo" );
    }
}

OReportSectionUndo::OReportSectionUndo(
    OReportModel& _rMod, sal_uInt16 _nSlot,
    ::std::function<uno::Reference<report::XSection>(OReportHelper*)> _pMemberFunction,
    const uno::Reference<report::XReportDefinition>& _xReport, Action _eAction)
    : OSectionUndo(_rMod, _nSlot, _eAction, {})
    , m_aReportHelper(_xReport)
    , m_pMemberFunction(std::move(_pMemberFunction))
{
    if( m_eAction == Removed )
        collectControls(m_pMemberFunction(&m_aReportHelper));
}

OReportSectionUndo::~OReportSectionUndo()
{
}

void OReportSectionUndo::implReInsert( )
{
    const uno::Sequence< beans::PropertyValue > aArgs;
    m_pController->executeChecked(m_nSlot,aArgs);
    uno::Reference< report::XSection > xSection = m_pMemberFunction(&m_aReportHelper);
    lcl_insertElements(xSection,m_aControls);
    lcl_setValues(xSection,m_aValues);
    m_bInserted = true;
}

void OReportSectionUndo::implReRemove( )
{
    if( m_eAction == Removed )
        collectControls(m_pMemberFunction(&m_aReportHelper));
    const uno::Sequence< beans::PropertyValue > aArgs;
    m_pController->executeChecked(m_nSlot,aArgs);
    m_bInserted = false;
}

OGroupSectionUndo::OGroupSectionUndo(
    OReportModel& _rMod, sal_uInt16 _nSlot,
    ::std::function<uno::Reference<report::XSection>(OGroupHelper*)> _pMemberFunction,
    const uno::Reference<report::XGroup>& _xGroup, Action _eAction, TranslateId pCommentID)
    : OSectionUndo(_rMod, _nSlot, _eAction, pCommentID)
    , m_aGroupHelper(_xGroup)
    , m_pMemberFunction(std::move(_pMemberFunction))
{
    if( m_eAction == Removed )
    {
        uno::Reference< report::XSection > xSection = m_pMemberFunction(&m_aGroupHelper);
        if ( xSection.is() )
            m_sName = xSection->getName();
        collectControls(xSection);
    }
}

OUString OGroupSectionUndo::GetComment() const
{
    if ( m_sName.isEmpty() )
    {
        try
        {
            uno::Reference< report::XSection > xSection = const_cast<OGroupSectionUndo*>(this)->m_pMemberFunction(&const_cast<OGroupSectionUndo*>(this)->m_aGroupHelper);

            if ( xSection.is() )
                m_sName = xSection->getName();
        }
        catch (const uno::Exception&)
        {
        }
    }
    return m_strComment + m_sName;
}

void OGroupSectionUndo::implReInsert( )
{
    const OUString aHeaderFooterOnName(SID_GROUPHEADER_WITHOUT_UNDO == m_nSlot? PROPERTY_HEADERON : PROPERTY_FOOTERON);
    uno::Sequence< beans::PropertyValue > aArgs{
        comphelper::makePropertyValue(aHeaderFooterOnName, true),
        comphelper::makePropertyValue(PROPERTY_GROUP, m_aGroupHelper.getGroup())
    };
    m_pController->executeChecked(m_nSlot,aArgs);

    uno::Reference< report::XSection > xSection = m_pMemberFunction(&m_aGroupHelper);
    lcl_insertElements(xSection,m_aControls);
    lcl_setValues(xSection,m_aValues);
    m_bInserted = true;
}

void OGroupSectionUndo::implReRemove( )
{
    if( m_eAction == Removed )
        collectControls(m_pMemberFunction(&m_aGroupHelper));

    const OUString aHeaderFooterOnName(SID_GROUPHEADER_WITHOUT_UNDO == m_nSlot? PROPERTY_HEADERON : PROPERTY_FOOTERON);
    uno::Sequence< beans::PropertyValue > aArgs{
        comphelper::makePropertyValue(aHeaderFooterOnName, false),
        comphelper::makePropertyValue(PROPERTY_GROUP, m_aGroupHelper.getGroup())
    };

    m_pController->executeChecked(m_nSlot,aArgs);
    m_bInserted = false;
}


OGroupUndo::OGroupUndo(OReportModel& _rMod
                       ,TranslateId pCommentID
                       ,Action  _eAction
                       ,uno::Reference< report::XGroup> _xGroup
                       ,uno::Reference< report::XReportDefinition > _xReportDefinition)
: OCommentUndoAction(_rMod,pCommentID)
,m_xGroup(std::move(_xGroup))
,m_xReportDefinition(std::move(_xReportDefinition))
,m_eAction(_eAction)
{
    m_nLastPosition = getPositionInIndexAccess(m_xReportDefinition->getGroups(),m_xGroup);
}

void OGroupUndo::implReInsert( )
{
    try
    {
        m_xReportDefinition->getGroups()->insertByIndex(m_nLastPosition,uno::Any(m_xGroup));
    }
    catch(uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "reportdesign", "Exception caught while undoing remove group");
    }
}

void OGroupUndo::implReRemove( )
{
    try
    {
        m_xReportDefinition->getGroups()->removeByIndex(m_nLastPosition);
    }
    catch(uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION( "reportdesign", "Exception caught while redoing remove group");
    }
}

void OGroupUndo::Undo()
{
    switch ( m_eAction )
    {
    case Inserted:
        implReRemove();
        break;

    case Removed:
        implReInsert();
        break;
    }

}

void OGroupUndo::Redo()
{
    switch ( m_eAction )
    {
    case Inserted:
        implReInsert();
        break;

    case Removed:
        implReRemove();
        break;
    }
}


} // rptui


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
