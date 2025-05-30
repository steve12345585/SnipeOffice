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

#include <connectivity/parameters.hxx>

#include <com/sun/star/form/DatabaseParameterEvent.hpp>
#include <com/sun/star/form/XDatabaseParameterListener.hpp>
#include <com/sun/star/sdbc/XParameters.hpp>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/sdb/XParametersSupplier.hpp>
#include <com/sun/star/sdb/ParametersRequest.hpp>
#include <com/sun/star/sdbc/SQLException.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>

#include <connectivity/dbtools.hxx>
#include <connectivity/filtermanager.hxx>
#include <TConnection.hxx>

#include <comphelper/diagnose_ex.hxx>

#include <ParameterCont.hxx>
#include <o3tl/safeint.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>

namespace dbtools
{
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::sdb;
    using namespace ::com::sun::star::sdbc;
    using namespace ::com::sun::star::sdbcx;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::task;
    using namespace ::com::sun::star::form;
    using namespace ::com::sun::star::container;

    using namespace ::comphelper;
    using namespace ::connectivity;

    ParameterManager::ParameterManager( ::osl::Mutex& _rMutex, const Reference< XComponentContext >& _rxContext )
        :m_rMutex             ( _rMutex )
        ,m_aParameterListeners( _rMutex )
        ,m_xContext           ( _rxContext  )
        ,m_nInnerCount        ( 0       )
        ,m_bUpToDate          ( false   )
    {
        OSL_ENSURE( m_xContext.is(), "ParameterManager::ParameterManager: no service factory!" );
    }


    void ParameterManager::initialize( const Reference< XPropertySet >& _rxComponent, const Reference< XAggregation >& _rxComponentAggregate )
    {
        OSL_ENSURE( !m_xComponent.get().is(), "ParameterManager::initialize: already initialized!" );

        m_xComponent        = _rxComponent;
        m_xAggregatedRowSet = _rxComponentAggregate;
        if ( m_xAggregatedRowSet.is() )
            m_xAggregatedRowSet->queryAggregation( cppu::UnoType<decltype(m_xInnerParamUpdate)>::get() ) >>= m_xInnerParamUpdate;
        OSL_ENSURE( m_xComponent.get().is() && m_xInnerParamUpdate.is(), "ParameterManager::initialize: invalid arguments!" );
        if ( !m_xComponent.get().is() || !m_xInnerParamUpdate.is() )
            return;
    }


    void ParameterManager::dispose( )
    {
        clearAllParameterInformation();

        m_xComposer.clear();
        m_xParentComposer.clear();
        //m_xComponent.clear();
        m_xInnerParamUpdate.clear();
        m_xAggregatedRowSet.clear();
    }


    void ParameterManager::clearAllParameterInformation()
    {
        m_xInnerParamColumns.clear();
        if ( m_pOuterParameters.is() )
            m_pOuterParameters->dispose();
        m_pOuterParameters   = nullptr;
        m_nInnerCount        = 0;
        ParameterInformation().swap(m_aParameterInformation);
        m_aMasterFields.clear();
        m_aDetailFields.clear();
        m_sIdentifierQuoteString.clear();
        m_sSpecialCharacters.clear();
        m_xConnectionMetadata.clear();
        std::vector< bool >().swap(m_aParametersVisited);
        m_bUpToDate = false;
    }


    void ParameterManager::setAllParametersNull()
    {
        OSL_PRECOND( isAlive(), "ParameterManager::setAllParametersNull: not initialized, or already disposed!" );
        if ( !isAlive() )
            return;

        for ( sal_Int32 i = 1; i <= m_nInnerCount; ++i )
            m_xInnerParamUpdate->setNull( i, DataType::VARCHAR );
    }


    bool ParameterManager::initializeComposerByComponent( const Reference< XPropertySet >& _rxComponent )
    {
        OSL_PRECOND( _rxComponent.is(), "ParameterManager::initializeComposerByComponent: invalid !" );

        m_xComposer.clear();
        m_xInnerParamColumns.clear();
        m_nInnerCount = 0;

        // create and fill a composer
        try
        {
            // get a query composer for the 's settings
            m_xComposer.reset( getCurrentSettingsComposer( _rxComponent, m_xContext, nullptr ), SharedQueryComposer::TakeOwnership );

            // see if the composer found parameters
            Reference< XParametersSupplier > xParamSupp( m_xComposer, UNO_QUERY );
            if ( xParamSupp.is() )
                m_xInnerParamColumns = xParamSupp->getParameters();

            if ( m_xInnerParamColumns.is() )
                m_nInnerCount = m_xInnerParamColumns->getCount();
        }
        catch( const SQLException& )
        {
        }

        return m_xInnerParamColumns.is();
    }


    void ParameterManager::collectInnerParameters( bool _bSecondRun )
    {
        OSL_PRECOND( m_xInnerParamColumns.is(), "ParameterManager::collectInnerParameters: missing some internal data!" );
        if ( !m_xInnerParamColumns.is() )
            return;

        // strip previous index information
        if ( _bSecondRun )
        {
            for (auto & paramInfo : m_aParameterInformation)
            {
                paramInfo.second.aInnerIndexes.clear();
            }
        }

        // we need to map the parameter names (which is all we get from the 's
        // MasterFields property) to indices, which are needed by the XParameters
        // interface of the row set)
        Reference<XPropertySet> xParam;
        for ( sal_Int32 i = 0; i < m_nInnerCount; ++i )
        {
            try
            {
                xParam.clear();
                m_xInnerParamColumns->getByIndex( i ) >>= xParam;

                OUString sName;
                xParam->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME) ) >>= sName;

                // only append additional parameters when they are not already in the list
                ParameterInformation::iterator aExistentPos = m_aParameterInformation.find( sName );
                OSL_ENSURE( !_bSecondRun || ( aExistentPos != m_aParameterInformation.end() ),
                    "ParameterManager::collectInnerParameters: the parameter information should already exist in the second run!" );

                if ( aExistentPos == m_aParameterInformation.end() )
                {
                    aExistentPos = m_aParameterInformation.emplace(
                        sName, xParam ).first;
                }
                else
                    aExistentPos->second.xComposerColumn = xParam;

                aExistentPos->second.aInnerIndexes.push_back( i );
            }
            catch( const Exception& )
            {
                TOOLS_WARN_EXCEPTION( "connectivity.commontools", "ParameterManager::collectInnerParameters" );
            }
        }
    }


    OUString ParameterManager::createFilterConditionFromColumnLink(
        const OUString &_rMasterColumn, const Reference < XPropertySet > &xDetailField, OUString &o_rNewParamName )
    {
        OUString sFilter;
        // format is:
        // <detail_column> = :<new_param_name>
        {
            OUString tblName;
            xDetailField->getPropertyValue(u"TableName"_ustr) >>= tblName;
            if (!tblName.isEmpty())
                sFilter = ::dbtools::quoteTableName( m_xConnectionMetadata, tblName, ::dbtools::EComposeRule::InDataManipulation ) + ".";
        }
        {
            OUString colName;
            xDetailField->getPropertyValue(u"RealName"_ustr) >>= colName;
            bool isFunction(false);
            xDetailField->getPropertyValue(u"Function"_ustr) >>= isFunction;
            if (isFunction)
                sFilter += colName;
            else
                sFilter += quoteName( m_sIdentifierQuoteString, colName );
        }

        // generate a parameter name which is not already used
        o_rNewParamName = "link_from_";
        o_rNewParamName += convertName2SQLName( _rMasterColumn, m_sSpecialCharacters );
        while ( m_aParameterInformation.find( o_rNewParamName ) != m_aParameterInformation.end() )
        {
            o_rNewParamName += "_";
        }

        return sFilter + " =:" + o_rNewParamName;
    }


    void ParameterManager::classifyLinks( const Reference< XNameAccess >& _rxParentColumns,
        const Reference< XNameAccess >& _rxColumns,
        std::vector< OUString >& _out_rAdditionalFilterComponents,
        std::vector< OUString >& _out_rAdditionalHavingComponents )
    {
        OSL_PRECOND( m_aMasterFields.size() == m_aDetailFields.size(),
            "ParameterManager::classifyLinks: master and detail fields should have the same length!" );
        OSL_ENSURE( _rxColumns.is(), "ParameterManager::classifyLinks: invalid columns!" );

        if ( !_rxColumns.is() )
            return;

        // we may need to strip any links which are invalid, so here go the containers
        // for temporarily holding the new pairs
        std::vector< OUString > aStrippedMasterFields;
        std::vector< OUString > aStrippedDetailFields;

        bool bNeedExchangeLinks = false;

        // classify the links
        auto pMasterFields = m_aMasterFields.begin();
        auto pDetailFields = m_aDetailFields.begin();
        auto pDetailFieldsEnd = m_aDetailFields.end();
        for ( ; pDetailFields != pDetailFieldsEnd; ++pDetailFields, ++pMasterFields )
        {
            if ( pMasterFields->isEmpty() || pDetailFields->isEmpty() )
                continue;

            // if not even the master part of the relationship exists in the parent, the
            // link is invalid as a whole #i63674#
            if ( !_rxParentColumns->hasByName( *pMasterFields ) )
            {
                bNeedExchangeLinks = true;
                continue;
            }

            bool bValidLink = true;

            // is there an inner parameter with this name? That is, a parameter which is already part of
            // the very original statement (not the one we create ourselves, with the additional parameters)
            ParameterInformation::iterator aPos = m_aParameterInformation.find( *pDetailFields );
            if ( aPos != m_aParameterInformation.end() )
            {   // there is an inner parameter with this name
                aPos->second.eType = ParameterClassification::LinkedByParamName;
                aStrippedDetailFields.push_back( *pDetailFields );
            }
            else
            {
                // does the detail name denote a column?
                if ( _rxColumns->hasByName( *pDetailFields ) )
                {
                    Reference< XPropertySet > xDetailField(_rxColumns->getByName( *pDetailFields ), UNO_QUERY);
                    assert(xDetailField.is());

                    OUString sNewParamName;
                    const OUString sFilterCondition = createFilterConditionFromColumnLink( *pMasterFields, xDetailField, sNewParamName );
                    OSL_PRECOND( !sNewParamName.isEmpty(), "ParameterManager::classifyLinks: createFilterConditionFromColumnLink returned nonsense!" );

                    // remember meta information about this new parameter
                    std::pair< ParameterInformation::iterator, bool > aInsertionPos =
                        m_aParameterInformation.emplace(
                            sNewParamName, ParameterMetaData( nullptr )
                        );
                    OSL_ENSURE( aInsertionPos.second, "ParameterManager::classifyLinks: there already was a parameter with this name!" );
                    aInsertionPos.first->second.eType = ParameterClassification::LinkedByColumnName;

                    // remember the filter component
                    if (isAggregateColumn(xDetailField))
                        _out_rAdditionalHavingComponents.push_back( sFilterCondition );
                    else
                        _out_rAdditionalFilterComponents.push_back( sFilterCondition );

                    // remember the new "detail field" for this link
                    aStrippedDetailFields.push_back( sNewParamName );
                    bNeedExchangeLinks = true;
                }
                else
                {
                    // the detail field neither denotes a column name, nor a parameter name
                    bValidLink = false;
                    bNeedExchangeLinks = true;
                }
            }

            if ( bValidLink )
                aStrippedMasterFields.push_back( *pMasterFields );
        }
        SAL_WARN_IF( aStrippedMasterFields.size() != aStrippedDetailFields.size(),
            "connectivity.commontools",
            "ParameterManager::classifyLinks: inconsistency in new link pairs!" );

        if ( bNeedExchangeLinks )
        {
            m_aMasterFields.swap(aStrippedMasterFields);
            m_aDetailFields.swap(aStrippedDetailFields);
        }
    }


    void ParameterManager::analyzeFieldLinks( FilterManager& _rFilterManager, bool& /* [out] */ _rColumnsInLinkDetails )
    {
        OSL_PRECOND( isAlive(), "ParameterManager::analyzeFieldLinks: not initialized, or already disposed!" );
        if ( !isAlive() )
            return;

        _rColumnsInLinkDetails = false;
        try
        {
            // the links as determined by the  properties
            Reference< XPropertySet > xProp = m_xComponent;
            OSL_ENSURE(xProp.is(),"Someone already released my component!");
            if ( xProp.is() )
            {
                Sequence<OUString> aTmp;
                if (xProp->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_MASTERFIELDS) ) >>= aTmp)
                     comphelper::sequenceToContainer(m_aMasterFields, aTmp);
                if (xProp->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_DETAILFIELDS) ) >>= aTmp)
                    comphelper::sequenceToContainer(m_aDetailFields, aTmp);
            }

            {
                // normalize to equal length
                sal_Int32 nMasterLength = m_aMasterFields.size();
                sal_Int32 nDetailLength = m_aDetailFields.size();

                if ( nMasterLength > nDetailLength )
                    m_aMasterFields.resize( nDetailLength );
                else if ( nDetailLength > nMasterLength )
                    m_aDetailFields.resize( nMasterLength );
            }

            Reference< XNameAccess > xColumns;
            if ( !getColumns( xColumns, true ) )
                // already asserted in getColumns
                return;

            Reference< XNameAccess > xParentColumns;
            if ( !getParentColumns( xParentColumns, true ) )
                return;

            // classify the links - depending on what the detail fields in each link pair denotes
            std::vector< OUString > aAdditionalFilterComponents;
            std::vector< OUString > aAdditionalHavingComponents;
            classifyLinks( xParentColumns, xColumns, aAdditionalFilterComponents, aAdditionalHavingComponents );

            // did we find links where the detail field refers to a detail column (instead of a parameter name)?
            if ( !aAdditionalFilterComponents.empty() )
            {
                // build a conjunction of all the filter components
                OUStringBuffer sAdditionalFilter;
                for (auto const& elem : aAdditionalFilterComponents)
                {
                    if ( !sAdditionalFilter.isEmpty() )
                        sAdditionalFilter.append(" AND ");

                    sAdditionalFilter.append("( " + elem + " )");
                }

                // now set this filter at the filter manager
                _rFilterManager.setFilterComponent( FilterManager::FilterComponent::LinkFilter, sAdditionalFilter.makeStringAndClear() );

                _rColumnsInLinkDetails = true;
            }

            if ( !aAdditionalHavingComponents.empty() )
            {
                // build a conjunction of all the filter components
                OUStringBuffer sAdditionalHaving;
                for (auto const& elem : aAdditionalHavingComponents)
                {
                    if ( !sAdditionalHaving.isEmpty() )
                        sAdditionalHaving.append(" AND ");

                    sAdditionalHaving.append("( " + elem + " )");
                }

                // now set this having clause at the filter manager
                _rFilterManager.setFilterComponent( FilterManager::FilterComponent::LinkHaving, sAdditionalHaving.makeStringAndClear() );

                _rColumnsInLinkDetails = true;
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "connectivity.commontools", "ParameterManager::analyzeFieldLinks" );
        }
    }


    void ParameterManager::createOuterParameters()
    {
        OSL_PRECOND( !m_pOuterParameters.is(), "ParameterManager::createOuterParameters: outer parameters not initialized!" );
        OSL_PRECOND( m_xInnerParamUpdate.is(), "ParameterManager::createOuterParameters: no write access to the inner parameters!" );
        if ( !m_xInnerParamUpdate.is() )
            return;

        m_pOuterParameters = new param::ParameterWrapperContainer;

#if OSL_DEBUG_LEVEL > 0
        sal_Int32 nSmallestIndexLinkedByColumnName = -1;
        sal_Int32 nLargestIndexNotLinkedByColumnName = -1;
#endif
        for (auto & aParam : m_aParameterInformation)
        {
#if OSL_DEBUG_LEVEL > 0
            if ( aParam.second.aInnerIndexes.size() )
            {
                if ( aParam.second.eType == ParameterClassification::LinkedByColumnName )
                {
                    if ( nSmallestIndexLinkedByColumnName == -1 )
                        nSmallestIndexLinkedByColumnName = aParam.second.aInnerIndexes[ 0 ];
                }
                else
                {
                    nLargestIndexNotLinkedByColumnName = aParam.second.aInnerIndexes[ aParam.second.aInnerIndexes.size() - 1 ];
                }
            }
#endif
            if ( aParam.second.eType != ParameterClassification::FilledExternally )
                continue;

            // check which of the parameters have already been visited (e.g. filled via XParameters)
            size_t nAlreadyVisited = 0;
            for (auto & aIndex : aParam.second.aInnerIndexes)
            {
                if ( ( m_aParametersVisited.size() > o3tl::make_unsigned(aIndex) ) && m_aParametersVisited[ aIndex ] )
                {   // exclude this index
                    aIndex = -1;
                    ++nAlreadyVisited;
                }
            }
            if ( nAlreadyVisited == aParam.second.aInnerIndexes.size() )
                continue;

            // need a wrapper for this... the "inner parameters" as supplied by a result set don't have a "Value"
            // property, but the parameter listeners expect such a property. So we need an object "aggregating"
            // xParam and supplying an additional property ("Value")
            // (it's no real aggregation of course...)
            m_pOuterParameters->push_back( new param::ParameterWrapper( aParam.second.xComposerColumn, m_xInnerParamUpdate, std::vector(aParam.second.aInnerIndexes) ) );
        }

#if OSL_DEBUG_LEVEL > 0
        OSL_ENSURE( ( nSmallestIndexLinkedByColumnName == -1 ) || ( nLargestIndexNotLinkedByColumnName == -1 ) ||
            ( nSmallestIndexLinkedByColumnName > nLargestIndexNotLinkedByColumnName ),
            "ParameterManager::createOuterParameters: inconsistency!" );

        // for the master-detail links, where the detail field denoted a column name, we created an additional ("artificial")
        // filter, and *appended* it to all other (potentially) existing filters of the row set. This means that the indexes
        // for the parameters resulting from the artificial filter should be larger than any other parameter index, and this
        // is what the assertion checks.
        // If the assertion fails, then we would need another handling for the "parameters visited" flags, since they're based
        // on parameter indexes *without* the artificial filter (because this filter is not visible from the outside).
#endif
    }


    void ParameterManager::updateParameterInfo( FilterManager& _rFilterManager )
    {
        OSL_PRECOND( isAlive(), "ParameterManager::updateParameterInfo: not initialized, or already disposed!" );
        if ( !isAlive() )
            return;

        clearAllParameterInformation();
        cacheConnectionInfo();

        // check whether the  is based on a statement/query which requires parameters
        Reference< XPropertySet > xProp = m_xComponent;
        OSL_ENSURE(xProp.is(),"Some already released my component!");
        if ( xProp.is() )
        {
            if ( !initializeComposerByComponent( xProp ) )
            {   // okay, nothing to do
                m_bUpToDate = true;
                return;
            } // if ( !initializeComposerByComponent( m_xComponent ) )
        }
        SAL_WARN_IF( !m_xInnerParamColumns.is(),
            "connectivity.commontools",
            "ParameterManager::updateParameterInfo: initializeComposerByComponent did nonsense (1)!" );

        // collect all parameters which are defined by the "inner parameters"
        collectInnerParameters( false );

        // analyze the master-detail relationships
        bool bColumnsInLinkDetails = false;
        analyzeFieldLinks( _rFilterManager, bColumnsInLinkDetails );

        if ( bColumnsInLinkDetails )
        {
            // okay, in this case, analyzeFieldLinks modified the "real" filter at the RowSet, to contain
            // an additional restriction (which we created ourself)
            // So we need to update all information about our inner parameter columns
            Reference< XPropertySet > xDirectRowSetProps;
            m_xAggregatedRowSet->queryAggregation( cppu::UnoType<decltype(xDirectRowSetProps)>::get() ) >>= xDirectRowSetProps;
            OSL_VERIFY( initializeComposerByComponent( xDirectRowSetProps ) );
            collectInnerParameters( true );
        }

        if ( !m_nInnerCount )
        {   // no parameters at all
            m_bUpToDate = true;
            return;
        }

        // for what now remains as outer parameters, create the wrappers for the single
        // parameter columns
        createOuterParameters();

        m_bUpToDate = true;
    }


    void ParameterManager::fillLinkedParameters( const Reference< XNameAccess >& _rxParentColumns )
    {
        OSL_PRECOND( isAlive(), "ParameterManager::fillLinkedParameters: not initialized, or already disposed!" );
        if ( !isAlive() )
            return;
        OSL_PRECOND( m_xInnerParamColumns.is(), "ParameterManager::fillLinkedParameters: no inner parameters found!"                 );
        OSL_ENSURE ( _rxParentColumns.is(),     "ParameterManager::fillLinkedParameters: invalid parent columns!"                    );

        try
        {
            // the master and detail field( name)s of the
            auto pMasterFields = m_aMasterFields.begin();
            auto pDetailFields = m_aDetailFields.begin();

            sal_Int32 nMasterLen = m_aMasterFields.size();

            // loop through all master fields. For each of them, get the respective column from the
            // parent , and forward its current value as parameter value to the (inner) row set
            for ( sal_Int32 i = 0; i < nMasterLen; ++i, ++pMasterFields, ++pDetailFields )
            {
                // does the name denote a valid column in the parent?
                if ( !_rxParentColumns->hasByName( *pMasterFields ) )
                {
                    SAL_WARN( "connectivity.commontools", "ParameterManager::fillLinkedParameters: invalid master names should have been stripped long before!" );
                    continue;
                }

                // do we, for this name, know where to place the values?
                ParameterInformation::const_iterator aParamInfo = m_aParameterInformation.find( *pDetailFields );
                if  (  ( aParamInfo == m_aParameterInformation.end() )
                    || ( aParamInfo->second.aInnerIndexes.empty() )
                    )
                {
                    SAL_WARN( "connectivity.commontools", "ParameterManager::fillLinkedParameters: nothing known about this detail field!" );
                    continue;
                }

                // the concrete master field
                Reference< XPropertySet >  xMasterField(_rxParentColumns->getByName( *pMasterFields ),UNO_QUERY);

                // the positions where we have to fill in values for the current parameter name
                for (auto const& aPosition : aParamInfo->second.aInnerIndexes)
                {
                    // the concrete detail field
                    Reference< XPropertySet >  xDetailField(m_xInnerParamColumns->getByIndex(aPosition),UNO_QUERY);
                    OSL_ENSURE( xDetailField.is(), "ParameterManager::fillLinkedParameters: invalid detail field!" );
                    if ( !xDetailField.is() )
                        continue;

                    // type and scale of the parameter field
                    sal_Int32 nParamType = DataType::VARCHAR;
                    OSL_VERIFY( xDetailField->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_TYPE) ) >>= nParamType );

                    sal_Int32 nScale = 0;
                    if ( xDetailField->getPropertySetInfo()->hasPropertyByName( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE) ) )
                        OSL_VERIFY( xDetailField->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_SCALE) ) >>= nScale );

                    // transfer the param value
                    try
                    {
                        m_xInnerParamUpdate->setObjectWithInfo(
                            aPosition + 1,                     // parameters are based at 1
                            xMasterField->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_VALUE) ),
                            nParamType,
                            nScale
                        );
                    }
                    catch( const Exception& )
                    {
                        DBG_UNHANDLED_EXCEPTION("connectivity.commontools");
                        SAL_WARN( "connectivity.commontools", "ParameterManager::fillLinkedParameters: master-detail parameter number " <<
                                  sal_Int32( aPosition + 1 ) << " could not be filled!" );
                    }
                }
            }
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("connectivity.commontools");
        }
    }


    bool ParameterManager::completeParameters( const Reference< XInteractionHandler >& _rxCompletionHandler, const Reference< XConnection >& _rxConnection )
    {
        OSL_PRECOND( isAlive(), "ParameterManager::completeParameters: not initialized, or already disposed!" );
        OSL_ENSURE ( _rxCompletionHandler.is(), "ParameterManager::completeParameters: invalid interaction handler!" );

        // two continuations (Ok and Cancel)
        rtl::Reference<OInteractionAbort> pAbort = new OInteractionAbort;
        rtl::Reference<OParameterContinuation> pParams = new OParameterContinuation;

        // the request
        ParametersRequest aRequest;
        aRequest.Parameters = m_pOuterParameters.get();
        aRequest.Connection = _rxConnection;
        rtl::Reference<OInteractionRequest> pRequest = new OInteractionRequest( Any( aRequest ) );

        // some knittings
        pRequest->addContinuation( pAbort );
        pRequest->addContinuation( pParams );

        // execute the request
        try
        {
            _rxCompletionHandler->handle( pRequest );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "connectivity.commontools", "ParameterManager::completeParameters: caught an exception while calling the handler" );
        }

        if ( !pParams->wasSelected() )
            // canceled by the user (i.e. (s)he canceled the dialog)
            return false;

        try
        {
            // transfer the values from the continuation object to the parameter columns
            const Sequence< PropertyValue >& aFinalValues = pParams->getValues();
            for (sal_Int32 i = 0; i < aFinalValues.getLength(); ++i)
            {
                Reference< XPropertySet > xParamColumn(aRequest.Parameters->getByIndex( i ),UNO_QUERY);
                if ( xParamColumn.is() )
                {
            #ifdef DBG_UTIL
                    OUString sName;
                    xParamColumn->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_NAME) ) >>= sName;
                    OSL_ENSURE( sName == aFinalValues[i].Name, "ParameterManager::completeParameters: inconsistent parameter names!" );
            #endif
                    xParamColumn->setPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_VALUE), aFinalValues[i].Value );
                        // the property sets are wrapper classes, translating the Value property into a call to
                        // the appropriate XParameters interface
                }
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "connectivity.commontools", "ParameterManager::completeParameters: caught an exception while propagating the values" );
        }
        return true;
    }


    bool ParameterManager::consultParameterListeners( ::osl::ResettableMutexGuard& _rClearForNotifies )
    {
        bool bCanceled = false;

        sal_Int32 nParamsLeft = m_pOuterParameters->getParameters().size();
            // TODO: shouldn't we subtract all the parameters which were already visited?
        if ( nParamsLeft )
        {
            ::comphelper::OInterfaceIteratorHelper3 aIter( m_aParameterListeners );
            Reference< XPropertySet > xProp = m_xComponent;
            OSL_ENSURE(xProp.is(),"Some already released my component!");
            DatabaseParameterEvent aEvent( xProp, m_pOuterParameters );

            _rClearForNotifies.clear();
            while ( aIter.hasMoreElements() && !bCanceled )
                bCanceled = !aIter.next()->approveParameter( aEvent );
            _rClearForNotifies.reset();
        }

        return !bCanceled;
    }


    bool ParameterManager::fillParameterValues( const Reference< XInteractionHandler >& _rxCompletionHandler, ::osl::ResettableMutexGuard& _rClearForNotifies )
    {
        OSL_PRECOND( isAlive(), "ParameterManager::fillParameterValues: not initialized, or already disposed!" );
        if ( !isAlive() )
            return true;

        if ( m_nInnerCount == 0 )
            // no parameters at all
            return true;

        // fill the parameters from the master-detail relationship
        Reference< XNameAccess > xParentColumns;
        if ( getParentColumns( xParentColumns, false ) && xParentColumns->hasElements() && !m_aMasterFields.empty() )
            fillLinkedParameters( xParentColumns );

        // let the user (via the interaction handler) fill all remaining parameters
        Reference< XConnection > xConnection;
        getConnection( xConnection );

        if ( _rxCompletionHandler.is() )
            return completeParameters( _rxCompletionHandler, xConnection );

        return consultParameterListeners( _rClearForNotifies );
    }


    void ParameterManager::getConnection( Reference< XConnection >& /* [out] */ _rxConnection )
    {
        OSL_PRECOND( isAlive(), "ParameterManager::getConnection: not initialized, or already disposed!" );
        if ( !isAlive() )
            return;

        _rxConnection.clear();
        try
        {
            Reference< XPropertySet > xProp = m_xComponent;
            OSL_ENSURE(xProp.is(),"Some already released my component!");
            if ( xProp.is() )
                xProp->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_ACTIVE_CONNECTION) ) >>= _rxConnection;
        }
        catch( const Exception& )
        {
            SAL_WARN( "connectivity.commontools", "ParameterManager::getConnection: could not retrieve the connection of the !" );
        }
    }


    void ParameterManager::cacheConnectionInfo()
    {
        try
        {
            Reference< XConnection > xConnection;
            getConnection( xConnection );
            Reference< XDatabaseMetaData > xMeta;
            if ( xConnection.is() )
                xMeta = xConnection->getMetaData();
            if ( xMeta.is() )
            {
                m_xConnectionMetadata = xMeta;
                m_sIdentifierQuoteString = xMeta->getIdentifierQuoteString();
                m_sSpecialCharacters = xMeta->getExtraNameCharacters();
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "connectivity.commontools", "ParameterManager::cacheConnectionInfo: caught an exception" );
        }
    }


    bool ParameterManager::getColumns( Reference< XNameAccess >& /* [out] */ _rxColumns, bool _bFromComposer )
    {
        _rxColumns.clear();

        Reference< XColumnsSupplier > xColumnSupp;
        if ( _bFromComposer )
            xColumnSupp.set(m_xComposer, css::uno::UNO_QUERY);
        else
            xColumnSupp.set( m_xComponent.get(),UNO_QUERY);
        if ( xColumnSupp.is() )
            _rxColumns = xColumnSupp->getColumns();
        OSL_ENSURE( _rxColumns.is(), "ParameterManager::getColumns: could not retrieve the columns for the detail !" );

        return _rxColumns.is();
    }


    bool ParameterManager::getParentColumns( Reference< XNameAccess >& /* [out] */ _out_rxParentColumns, bool _bFromComposer )
    {
        OSL_PRECOND( isAlive(), "ParameterManager::getParentColumns: not initialized, or already disposed!" );

        _out_rxParentColumns.clear();
        try
        {
            // get the parent of the component we're working for
            Reference< XChild > xAsChild( m_xComponent.get(), UNO_QUERY_THROW );
            Reference< XPropertySet > xParent( xAsChild->getParent(), UNO_QUERY );
            if ( !xParent.is() )
                return false;

            // the columns supplier: either from a composer, or directly from the
            Reference< XColumnsSupplier > xParentColSupp;
            if ( _bFromComposer )
            {
                // re-create the parent composer all the time. Else, we'd have to bother with
                // being a listener at its properties, its loaded state, and event the parent-relationship.
                m_xParentComposer.reset(
                    getCurrentSettingsComposer( xParent, m_xContext, nullptr ),
                    SharedQueryComposer::TakeOwnership
                );
                xParentColSupp.set(m_xParentComposer, css::uno::UNO_QUERY);
            }
            else
                xParentColSupp.set(xParent, css::uno::UNO_QUERY);

            // get the columns of the parent
            if ( xParentColSupp.is() )
                _out_rxParentColumns = xParentColSupp->getColumns();
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "connectivity.commontools", "ParameterManager::getParentColumns" );
        }
        return _out_rxParentColumns.is();
    }


    void ParameterManager::addParameterListener( const Reference< XDatabaseParameterListener >& _rxListener )
    {
        if ( _rxListener.is() )
            m_aParameterListeners.addInterface( _rxListener );
    }


    void ParameterManager::removeParameterListener( const Reference< XDatabaseParameterListener >& _rxListener )
    {
        m_aParameterListeners.removeInterface( _rxListener );
    }


    void ParameterManager::resetParameterValues( )
    {
        OSL_PRECOND( isAlive(), "ParameterManager::resetParameterValues: not initialized, or already disposed!" );
        if ( !isAlive() )
            return;

        if ( !m_nInnerCount )
            // no parameters at all
            return;

        try
        {
            Reference< XNameAccess > xColumns;
            if ( !getColumns( xColumns, false ) )
                // already asserted in getColumns
                return;

            Reference< XNameAccess > xParentColumns;
            if ( !getParentColumns( xParentColumns, false ) )
                return;

            // loop through all links pairs
            auto pMasterFields = m_aMasterFields.begin();
            auto pDetailFields = m_aDetailFields.begin();

            Reference< XPropertySet > xMasterField;
            Reference< XPropertySet > xDetailField;

            // now really ....
            auto pDetailFieldsEnd = m_aDetailFields.end();
            for ( ; pDetailFields != pDetailFieldsEnd; ++pDetailFields, ++pMasterFields )
            {
                if ( !xParentColumns->hasByName( *pMasterFields ) )
                {
                    // if this name is unknown in the parent columns, then we don't have a source
                    // for copying the value to the detail columns
                    SAL_WARN( "connectivity.commontools", "ParameterManager::resetParameterValues: this should have been stripped long before!" );
                    continue;
                }

                // for all inner parameters which are bound to the name as specified by the
                // slave element of the link, propagate the value from the master column to this
                // parameter column
                ParameterInformation::const_iterator aParamInfo = m_aParameterInformation.find( *pDetailFields );
                if  (  ( aParamInfo == m_aParameterInformation.end() )
                    || ( aParamInfo->second.aInnerIndexes.empty() )
                    )
                {
                    SAL_WARN( "connectivity.commontools", "ParameterManager::resetParameterValues: nothing known about this detail field!" );
                    continue;
                }

                xParentColumns->getByName( *pMasterFields ) >>= xMasterField;
                if ( !xMasterField.is() )
                    continue;

                for (auto const& aPosition : aParamInfo->second.aInnerIndexes)
                {
                    Reference< XPropertySet > xInnerParameter;
                    m_xInnerParamColumns->getByIndex(aPosition) >>= xInnerParameter;
                    if ( !xInnerParameter.is() )
                        continue;

                    OUString sParamColumnRealName;
                    xInnerParameter->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_REALNAME) ) >>= sParamColumnRealName;
                    if ( xColumns->hasByName( sParamColumnRealName ) )
                    {   // our own columns have a column which's name equals the real name of the param column
                        // -> transfer the value property
                        xColumns->getByName( sParamColumnRealName ) >>= xDetailField;
                        if ( xDetailField.is() )
                            xDetailField->setPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_VALUE), xMasterField->getPropertyValue( OMetaConnection::getPropMap().getNameByIndex(PROPERTY_ID_VALUE) ) );
                    }
                }
            }
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "connectivity.commontools", "ParameterManager::resetParameterValues" );
        }

    }


    void ParameterManager::externalParameterVisited( sal_Int32 _nIndex )
    {
        if ( m_aParametersVisited.size() < o3tl::make_unsigned(_nIndex) )
        {
            m_aParametersVisited.reserve( _nIndex );
            for ( sal_Int32 i = m_aParametersVisited.size(); i < _nIndex; ++i )
                m_aParametersVisited.push_back( false );
        }
        m_aParametersVisited[ _nIndex - 1 ] = true;
    }

    void ParameterManager::setNull( sal_Int32 _nIndex, sal_Int32 sqlType )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setNull(_nIndex, sqlType);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setObjectNull( sal_Int32 _nIndex, sal_Int32 sqlType, const OUString& typeName )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setObjectNull(_nIndex, sqlType, typeName);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setBoolean( sal_Int32 _nIndex, bool x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setBoolean(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setByte( sal_Int32 _nIndex, sal_Int8 x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setByte(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setShort( sal_Int32 _nIndex, sal_Int16 x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setShort(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setInt( sal_Int32 _nIndex, sal_Int32 x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setInt(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setLong( sal_Int32 _nIndex, sal_Int64 x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setLong(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setFloat( sal_Int32 _nIndex, float x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setFloat(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setDouble( sal_Int32 _nIndex, double x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setDouble(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setString( sal_Int32 _nIndex, const OUString& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setString(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setBytes( sal_Int32 _nIndex, const css::uno::Sequence< sal_Int8 >& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setBytes(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setDate( sal_Int32 _nIndex, const css::util::Date& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setDate(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setTime( sal_Int32 _nIndex, const css::util::Time& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setTime(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setTimestamp( sal_Int32 _nIndex, const css::util::DateTime& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setTimestamp(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setBinaryStream( sal_Int32 _nIndex, const css::uno::Reference< css::io::XInputStream>& x, sal_Int32 length )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setBinaryStream(_nIndex, x, length);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setCharacterStream( sal_Int32 _nIndex, const css::uno::Reference< css::io::XInputStream>& x, sal_Int32 length )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setCharacterStream(_nIndex, x, length);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setObject( sal_Int32 _nIndex, const css::uno::Any& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setObject(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setObjectWithInfo( sal_Int32 _nIndex, const css::uno::Any& x, sal_Int32 targetSqlType, sal_Int32 scale )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setObjectWithInfo(_nIndex, x, targetSqlType, scale);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setRef( sal_Int32 _nIndex, const css::uno::Reference< css::sdbc::XRef>& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setRef(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setBlob( sal_Int32 _nIndex, const css::uno::Reference< css::sdbc::XBlob>& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setBlob(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setClob( sal_Int32 _nIndex, const css::uno::Reference< css::sdbc::XClob>& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setClob(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::setArray( sal_Int32 _nIndex, const css::uno::Reference< css::sdbc::XArray>& x )
    {
        ::osl::MutexGuard aGuard(m_rMutex);
        OSL_ENSURE(m_xInnerParamUpdate.is(), "ParameterManager::XParameters::setXXX: no XParameters access to the RowSet!");
        if (!m_xInnerParamUpdate.is())
            return;
        m_xInnerParamUpdate->setArray(_nIndex, x);
        externalParameterVisited(_nIndex);
    }


    void ParameterManager::clearParameters( )
    {
        if ( m_xInnerParamUpdate.is() )
            m_xInnerParamUpdate->clearParameters( );
    }

    void SAL_CALL OParameterContinuation::setParameters( const Sequence< PropertyValue >& _rValues )
    {
        m_aValues = _rValues;
    }


}   // namespace frm


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
