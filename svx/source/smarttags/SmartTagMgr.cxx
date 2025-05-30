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

// SMARTTAGS

#include <svx/SmartTagMgr.hxx>

#include <utility>
#include <vcl/svapp.hxx>
#include <com/sun/star/smarttags/XSmartTagRecognizer.hpp>
#include <com/sun/star/smarttags/XRangeBasedSmartTagRecognizer.hpp>
#include <com/sun/star/smarttags/XSmartTagAction.hpp>
#include <com/sun/star/deployment/ExtensionManager.hpp>
#include <com/sun/star/smarttags/SmartTagRecognizerMode.hpp>
#include <com/sun/star/i18n/BreakIterator.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/lang/XSingleComponentFactory.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/container/XContentEnumerationAccess.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/util/XChangesNotifier.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <rtl/ustring.hxx>

using namespace com::sun::star;
using namespace com::sun::star::uno;
using namespace com::sun::star::i18n;


SmartTagMgr::SmartTagMgr( OUString aApplicationName )
    : maApplicationName(std::move( aApplicationName )),
      mxContext( ::comphelper::getProcessComponentContext() ),
      mbLabelTextWithSmartTags(true)
{
}

SmartTagMgr::~SmartTagMgr()
{
}

void SmartTagMgr::Init( std::u16string_view rConfigurationGroupName )
{
    PrepareConfiguration( rConfigurationGroupName );
    ReadConfiguration( true, true );
    RegisterListener();
    LoadLibraries();
}

/** Dispatches the recognize call to all installed smart tag recognizers
*/
void SmartTagMgr::RecognizeString( const OUString& rText,
                             const Reference< text::XTextMarkup >& xMarkup,
                             const Reference< frame::XController >& xController,
                             const lang::Locale& rLocale,
                             sal_uInt32 nStart, sal_uInt32 nLen ) const
{
    for (const Reference < smarttags::XSmartTagRecognizer >& xRecognizer : maRecognizerList)
    {
        // if all smart tag types supported by this recognizer have been
        // disabled, we do not have to call the recognizer:
        bool bCallRecognizer = false;
        const sal_uInt32 nSmartTagCount = xRecognizer->getSmartTagCount();
        for ( sal_uInt32 j = 0; j < nSmartTagCount && !bCallRecognizer; ++j )
        {
            const OUString aSmartTagName = xRecognizer->getSmartTagName(j);
            if ( IsSmartTagTypeEnabled( aSmartTagName ) )
                bCallRecognizer = true;
        }

        if ( bCallRecognizer )
        {
            // get the break iterator
            if ( !mxBreakIter.is() )
            {
                mxBreakIter.set( BreakIterator::create(mxContext) );
            }
            xRecognizer->recognize( rText, nStart, nLen,
                                            smarttags::SmartTagRecognizerMode_PARAGRAPH,
                                            rLocale, xMarkup, maApplicationName, xController,
                                            mxBreakIter );
        }
    }
}

void SmartTagMgr::RecognizeTextRange(const Reference< text::XTextRange>& xRange,
                             const Reference< text::XTextMarkup >& xMarkup,
                             const Reference< frame::XController >& xController) const
{
    for (const Reference<smarttags::XSmartTagRecognizer>& xRecognizer : maRecognizerList)
    {
        Reference< smarttags::XRangeBasedSmartTagRecognizer > xRangeBasedRecognizer( xRecognizer, UNO_QUERY);

        if (!xRangeBasedRecognizer.is()) continue;

        // if all smart tag types supported by this recognizer have been
        // disabled, we do not have to call the recognizer:
        bool bCallRecognizer = false;
        const sal_uInt32 nSmartTagCount = xRecognizer->getSmartTagCount();
        for ( sal_uInt32 j = 0; j < nSmartTagCount && !bCallRecognizer; ++j )
        {
            const OUString aSmartTagName = xRecognizer->getSmartTagName(j);
            if ( IsSmartTagTypeEnabled( aSmartTagName ) )
                bCallRecognizer = true;
        }

        if ( bCallRecognizer )
        {
            xRangeBasedRecognizer->recognizeTextRange( xRange,
                                                       smarttags::SmartTagRecognizerMode_PARAGRAPH,
                                                       xMarkup, maApplicationName, xController);
        }
    }

}

void SmartTagMgr::GetActionSequences( std::vector< OUString >& rSmartTagTypes,
                                      Sequence < Sequence< Reference< smarttags::XSmartTagAction > > >& rActionComponentsSequence,
                                      Sequence < Sequence< sal_Int32 > >& rActionIndicesSequence ) const
{
    rActionComponentsSequence.realloc( rSmartTagTypes.size() );
    auto pActionComponentsSequence = rActionComponentsSequence.getArray();
    rActionIndicesSequence.realloc( rSmartTagTypes.size() );
    auto pActionIndicesSequence = rActionIndicesSequence.getArray();

    for ( size_t j = 0; j < rSmartTagTypes.size(); ++j )
    {
        const OUString& rSmartTagType = rSmartTagTypes[j];

        const sal_Int32 nNumberOfActionRefs = maSmartTagMap.count( rSmartTagType );

        Sequence< Reference< smarttags::XSmartTagAction > > aActions( nNumberOfActionRefs );
        auto aActionsRange = asNonConstRange(aActions);
        Sequence< sal_Int32 > aIndices( nNumberOfActionRefs );
        auto aIndicesRange = asNonConstRange(aIndices);

        sal_uInt16 i = 0;
        auto iters = maSmartTagMap.equal_range( rSmartTagType );

        for ( auto aActionsIter = iters.first; aActionsIter != iters.second; ++aActionsIter )
        {
            aActionsRange[ i ] = (*aActionsIter).second.mxSmartTagAction;
            aIndicesRange[ i++ ] = (*aActionsIter).second.mnSmartTagIndex;
        }

        pActionComponentsSequence[ j ] = std::move(aActions);
        pActionIndicesSequence[ j ]  = std::move(aIndices);
    }
}

/** Returns the caption for a smart tag type.
*/
OUString SmartTagMgr::GetSmartTagCaption( const OUString& rSmartTagType, const css::lang::Locale& rLocale ) const
{
    OUString aRet;

    auto aLower = maSmartTagMap.find( rSmartTagType );

    if ( aLower != maSmartTagMap.end() )
    {
        const ActionReference& rActionRef = (*aLower).second;
        Reference< smarttags::XSmartTagAction > xAction = rActionRef.mxSmartTagAction;

        if ( xAction.is() )
        {
            const sal_Int32 nSmartTagIndex = rActionRef.mnSmartTagIndex;
            aRet = xAction->getSmartTagCaption( nSmartTagIndex, rLocale );
        }
    }

    return aRet;
}


/** Returns true if the given smart tag type is enabled.
*/
bool SmartTagMgr::IsSmartTagTypeEnabled( const OUString& rSmartTagType ) const
{
    return maDisabledSmartTagTypes.end() == maDisabledSmartTagTypes.find( rSmartTagType );
}

/** Writes currently disabled smart tag types to configuration.
*/
void SmartTagMgr::WriteConfiguration( const bool* pIsLabelTextWithSmartTags,
                                      const std::vector< OUString >* pDisabledTypes ) const
{
    if ( !mxConfigurationSettings.is() )
        return;

    bool bCommit = false;

    if ( pIsLabelTextWithSmartTags )
    {
        const Any aEnabled( *pIsLabelTextWithSmartTags );

        try
        {
            mxConfigurationSettings->setPropertyValue( u"RecognizeSmartTags"_ustr, aEnabled );
            bCommit = true;
        }
        catch ( css::uno::Exception& )
        {
        }
    }

    if ( pDisabledTypes )
    {
        Sequence< OUString > aTypes = comphelper::containerToSequence(*pDisabledTypes);

        const Any aNewTypes( aTypes );

        try
        {
            mxConfigurationSettings->setPropertyValue( u"ExcludedSmartTagTypes"_ustr, aNewTypes );
            bCommit = true;
        }
        catch ( css::uno::Exception& )
        {
        }
    }

    if ( bCommit )
    {
        try
        {
            Reference< util::XChangesBatch > xChanges( mxConfigurationSettings, UNO_QUERY );
            if (xChanges)
                xChanges->commitChanges();
        }
        catch ( css::uno::Exception& )
        {
        }
    }
}

// css::util::XModifyListener
void SmartTagMgr::modified( const lang::EventObject& )
{
    SolarMutexGuard aGuard;

    maRecognizerList.clear();
    maActionList.clear();
    maSmartTagMap.clear();

    LoadLibraries();
}

// css::lang::XEventListener
void SmartTagMgr::disposing( const lang::EventObject& rEvent )
{
    SolarMutexGuard aGuard;

    uno::Reference< frame::XModel >  xModel( rEvent.Source, uno::UNO_QUERY );
    uno::Reference< util::XModifyBroadcaster >  xMB(xModel, uno::UNO_QUERY);
    uno::Reference< util::XChangesNotifier >  xCN(xModel, uno::UNO_QUERY);

    try
    {
        if( xMB.is() )
        {
            uno::Reference< util::XModifyListener >  xListener( this );
            xMB->removeModifyListener( xListener );
        }
        else if ( xCN.is() )
        {
            uno::Reference< util::XChangesListener >  xListener( this );
            xCN->removeChangesListener( xListener );
        }
    }
    catch(Exception& )
    {
    }
}

// css::util::XChangesListener
void SmartTagMgr::changesOccurred( const util::ChangesEvent& rEvent )
{
    SolarMutexGuard aGuard;

    bool bExcludedTypes = false;
    bool bRecognize = false;

    for( const util::ElementChange& rElementChange : rEvent.Changes)
    {
        OUString sTemp;
        rElementChange.Accessor >>= sTemp;

        if ( sTemp == "ExcludedSmartTagTypes" )
            bExcludedTypes = true;
        else if ( sTemp == "RecognizeSmartTags" )
            bRecognize = true;
    }

    ReadConfiguration( bExcludedTypes, bRecognize );
}

void SmartTagMgr::LoadLibraries()
{
    Reference< container::XContentEnumerationAccess > rContent( mxContext->getServiceManager(), UNO_QUERY_THROW );

    // load recognizers: No recognizers -> nothing to do.
    Reference < container::XEnumeration > rEnum = rContent->createContentEnumeration( u"com.sun.star.smarttags.SmartTagRecognizer"_ustr);
    if ( !rEnum.is() || !rEnum->hasMoreElements() )
        return;

    // iterate over all implementations of the smart tag recognizer service:
    while( rEnum->hasMoreElements())
    {
        const Any a = rEnum->nextElement();
        Reference< lang::XSingleComponentFactory > xSCF;
        Reference< lang::XServiceInfo > xsInfo;

        if (a >>= xsInfo)
            xSCF.set(xsInfo, UNO_QUERY);
        else
            continue;

        Reference< smarttags::XSmartTagRecognizer > xLib ( xSCF->
                   createInstanceWithContext(mxContext), UNO_QUERY );

        if (!xLib.is())
            continue;

        xLib->initialize( Sequence< Any >() );
        maRecognizerList.push_back(xLib);
    }

    // load actions: No actions -> nothing to do.
    rEnum = rContent->createContentEnumeration( u"com.sun.star.smarttags.SmartTagAction"_ustr);
    if ( !rEnum.is() )
        return;

    // iterate over all implementations of the smart tag action service:
    while( rEnum->hasMoreElements())
    {
        const Any a = rEnum->nextElement();
        Reference< lang::XServiceInfo > xsInfo;
        Reference< lang::XSingleComponentFactory > xSCF;

        if (a >>= xsInfo)
            xSCF.set(xsInfo, UNO_QUERY);
        else
            continue;

        Reference< smarttags::XSmartTagAction > xLib ( xSCF->
                    createInstanceWithContext(mxContext), UNO_QUERY );

        if (!xLib.is())
            continue;

        xLib->initialize( Sequence< Any >() );
        maActionList.push_back(xLib);
    }

    AssociateActionsWithRecognizers();

}

void SmartTagMgr::PrepareConfiguration( std::u16string_view rConfigurationGroupName )
{
    beans::PropertyValue aPathArgument;
    aPathArgument.Name = "nodepath";
    aPathArgument.Value <<= OUString::Concat("/org.openoffice.Office.Common/SmartTags/") + rConfigurationGroupName;
    Sequence< Any > aArguments{ Any(aPathArgument) };
    Reference< lang::XMultiServiceFactory > xConfProv = configuration::theDefaultProvider::get( mxContext );

    // try to get read-write access to configuration:
    Reference< XInterface > xConfigurationAccess;
    try
    {
        xConfigurationAccess = xConfProv->createInstanceWithArguments(
            u"com.sun.star.configuration.ConfigurationUpdateAccess"_ustr, aArguments );
    }
    catch ( uno::Exception& )
    {
    }

    // fallback: try read-only access to configuration:
    if ( !xConfigurationAccess.is() )
    {
        try
        {
            xConfigurationAccess = xConfProv->createInstanceWithArguments(
                u"com.sun.star.configuration.ConfigurationAccess"_ustr, aArguments );
        }
        catch ( uno::Exception& )
        {
        }
    }

    if ( xConfigurationAccess.is() )
    {
        mxConfigurationSettings.set( xConfigurationAccess, UNO_QUERY );
    }
}


void SmartTagMgr::ReadConfiguration( bool bExcludedTypes, bool bRecognize )
{
    if ( !mxConfigurationSettings.is() )
        return;

    if ( bExcludedTypes )
    {
        maDisabledSmartTagTypes.clear();

        Any aAny = mxConfigurationSettings->getPropertyValue( u"ExcludedSmartTagTypes"_ustr );
        Sequence< OUString > aValues;
        aAny >>= aValues;

        for (const auto& rValue : aValues)
            maDisabledSmartTagTypes.insert( rValue );
    }

    if ( bRecognize )
    {
        Any aAny = mxConfigurationSettings->getPropertyValue( u"RecognizeSmartTags"_ustr );
        bool bValue = true;
        aAny >>= bValue;

        mbLabelTextWithSmartTags = bValue;
    }
}

void SmartTagMgr::RegisterListener()
{
    // register as listener at package manager
    try
    {
        Reference<deployment::XExtensionManager> xExtensionManager(
                deployment::ExtensionManager::get( mxContext ) );
        if (xExtensionManager)
        {
            Reference< util::XModifyListener > xListener( this );
            xExtensionManager->addModifyListener( xListener );
        }
    }
    catch ( uno::Exception& )
    {
    }

    // register as listener at configuration
    try
    {
        Reference<util::XChangesNotifier> xCN( mxConfigurationSettings, UNO_QUERY );
        if (xCN)
        {
            Reference< util::XChangesListener > xListener( this );
            xCN->addChangesListener( xListener );
        }
    }
    catch ( uno::Exception& )
    {
    }
}

typedef std::pair < const OUString, ActionReference > SmartTagMapElement;

/** Sets up a map that maps smart tag type names to actions references.
*/
void SmartTagMgr::AssociateActionsWithRecognizers()
{
    const sal_uInt32 nActionLibCount = maActionList.size();
    const sal_uInt32 nRecognizerCount = maRecognizerList.size();

    for ( sal_uInt32 i = 0; i < nRecognizerCount; ++i )
    {
        Reference < smarttags::XSmartTagRecognizer > xRecognizer = maRecognizerList[i];
        const sal_uInt32 nSmartTagCount = xRecognizer->getSmartTagCount();
        for ( sal_uInt32 j = 0; j < nSmartTagCount; ++j )
        {
            const OUString aSmartTagName = xRecognizer->getSmartTagName(j);

            // check if smart tag type has already been processed:
            if ( maSmartTagMap.contains( aSmartTagName ) )
                continue;

            bool bFound = false;
            for ( sal_uInt32 k = 0; k < nActionLibCount; ++k )
            {
                Reference< smarttags::XSmartTagAction > xActionLib = maActionList[k];
                const sal_uInt32 nSmartTagCountInActionLib = xActionLib->getSmartTagCount();
                for ( sal_uInt32 l = 0; l < nSmartTagCountInActionLib; ++l )
                {
                    const OUString aSmartTagNameInActionLib = xActionLib->getSmartTagName(l);
                    if ( aSmartTagName == aSmartTagNameInActionLib )
                    {
                        // found actions and recognizer for same smarttag
                        ActionReference aActionRef( xActionLib, l );

                        // add recognizer/action pair to map
                        maSmartTagMap.insert( SmartTagMapElement( aSmartTagName, aActionRef ));

                        bFound = true;
                    }
                }
            }

            if ( !bFound )
            {
                // insert 'empty' action reference if there is no action associated with
                // the current smart tag type:
                ActionReference aActionRef(Reference<smarttags::XSmartTagAction>(), 0);

                // add recognizer/action pair to map
                maSmartTagMap.insert( SmartTagMapElement( aSmartTagName, aActionRef ));
            }
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
