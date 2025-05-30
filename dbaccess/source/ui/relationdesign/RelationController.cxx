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

#include <sal/config.h>

#include <map>

#include <strings.hrc>
#include <strings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <browserids.hxx>
#include <comphelper/types.hxx>
#include <core_resource.hxx>
#include <connectivity/dbtools.hxx>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/sdbcx/KeyType.hpp>
#include <com/sun/star/sdbcx/XKeysSupplier.hpp>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/sdbc/SQLException.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <connectivity/dbexception.hxx>
#include <connectivity/dbmetadata.hxx>
#include <sqlmessage.hxx>
#include <RelationController.hxx>
#include <TableWindowData.hxx>
#include <UITools.hxx>
#include <RTableConnectionData.hxx>
#include <RelationTableView.hxx>
#include <RelationDesignView.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <osl/thread.hxx>
#include <osl/mutex.hxx>

#define MAX_THREADS 10

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
org_openoffice_comp_dbu_ORelationDesign_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const& )
{
    return cppu::acquire(new ::dbaui::ORelationController(context));
}

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::sdbcx;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::ui::dialogs;
using namespace ::dbtools;
using namespace ::dbaui;
using namespace ::comphelper;
using namespace ::osl;

OUString SAL_CALL ORelationController::getImplementationName()
{
    return u"org.openoffice.comp.dbu.ORelationDesign"_ustr;
}

Sequence< OUString> SAL_CALL ORelationController::getSupportedServiceNames()
{
    return { u"com.sun.star.sdb.RelationDesign"_ustr };
}

ORelationController::ORelationController(const Reference< XComponentContext >& _rM)
    : OJoinController(_rM)
    ,m_nThreadEvent(0)
    ,m_bRelationsPossible(true)
{
    InvalidateAll();
}

ORelationController::~ORelationController()
{
}

FeatureState ORelationController::GetState(sal_uInt16 _nId) const
{
    FeatureState aReturn;
    aReturn.bEnabled = m_bRelationsPossible;
    switch (_nId)
    {
        case SID_RELATION_ADD_RELATION:
            aReturn.bEnabled = !m_vTableData.empty() && isConnected() && isEditable();
            aReturn.bChecked = false;
            break;
        case ID_BROWSER_SAVEDOC:
            aReturn.bEnabled = haveDataSource() && impl_isModified();
            break;
        default:
            aReturn = OJoinController::GetState(_nId);
            break;
    }
    return aReturn;
}

void ORelationController::Execute(sal_uInt16 _nId, const Sequence< PropertyValue >& aArgs)
{
    switch(_nId)
    {
        case ID_BROWSER_SAVEDOC:
            {
                OSL_ENSURE(isEditable(),"Slot ID_BROWSER_SAVEDOC should not be enabled!");
                if(!::dbaui::checkDataSourceAvailable(::comphelper::getString(getDataSource()->getPropertyValue(PROPERTY_NAME)), getORB()))
                {
                    OUString aMessage(DBA_RES(STR_DATASOURCE_DELETED));
                    OSQLWarningBox aWarning(getFrameWeld(), aMessage);
                    aWarning.run();
                }
                else
                {
                    // now we save the layout information
                    //  create the output stream
                    try
                    {
                        if ( haveDataSource() && getDataSource()->getPropertySetInfo()->hasPropertyByName(PROPERTY_LAYOUTINFORMATION) )
                        {
                            ::comphelper::NamedValueCollection aWindowsData;
                            saveTableWindows( aWindowsData );
                            getDataSource()->setPropertyValue( PROPERTY_LAYOUTINFORMATION, Any( aWindowsData.getPropertyValues() ) );
                            setModified(false);
                        }
                    }
                    catch ( const Exception& )
                    {
                        DBG_UNHANDLED_EXCEPTION("dbaccess");
                    }
                }
            }
            break;
        case SID_RELATION_ADD_RELATION:
            static_cast<ORelationTableView*>(static_cast<ORelationDesignView*>( getView() )->getTableView())->AddNewRelation();
            break;
        default:
            OJoinController::Execute(_nId,aArgs);
            return;
    }
    InvalidateFeature(_nId);
}

void ORelationController::impl_initialize(const ::comphelper::NamedValueCollection& rArguments)
{
    OJoinController::impl_initialize(rArguments);

    if( !getSdbMetaData().supportsRelations() )
    {// check if this database supports relations

        setEditable(false);
        m_bRelationsPossible    = false;
        {
            OUString sTitle(DBA_RES(STR_RELATIONDESIGN));
            sTitle = sTitle.copy(3);
            OSQLMessageBox aDlg(getFrameWeld(), sTitle, DBA_RES(STR_RELATIONDESIGN_NOT_AVAILABLE));
            aDlg.run();
        }
        disconnect();
        throw SQLException();
    }

    if(!m_bRelationsPossible)
        InvalidateAll();

    // we need a datasource
    OSL_ENSURE(haveDataSource(),"ORelationController::initialize: need a datasource!");

    Reference<XTablesSupplier> xSup(getConnection(),UNO_QUERY);
    OSL_ENSURE(xSup.is(),"Connection isn't a XTablesSupplier!");
    if(xSup.is())
        m_xTables = xSup->getTables();
    // load the layoutInformation
    loadLayoutInformation();
    try
    {
        loadData();
        if ( !m_nThreadEvent )
            Application::PostUserEvent(LINK(this, ORelationController, OnThreadFinished));
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("dbaccess");
    }

}

OUString ORelationController::getPrivateTitle( ) const
{
    OUString sName = getDataSourceName();
    return ::dbaui::getStrippedDatabaseName(getDataSource(),sName);
}

bool ORelationController::Construct(vcl::Window* pParent)
{
    setView( VclPtr<ORelationDesignView>::Create( pParent, *this, getORB() ) );
    OJoinController::Construct(pParent);
    return true;
}

short ORelationController::saveModified()
{
    short nSaved = RET_YES;
    if(haveDataSource() && isModified())
    {
        std::unique_ptr<weld::Builder> xBuilder(Application::CreateBuilder(getFrameWeld(), u"dbaccess/ui/designsavemodifieddialog.ui"_ustr));
        std::unique_ptr<weld::MessageDialog> xQuery(xBuilder->weld_message_dialog(u"DesignSaveModifiedDialog"_ustr));
        nSaved = xQuery->run();
        if(nSaved == RET_YES)
            Execute(ID_BROWSER_SAVEDOC,Sequence<PropertyValue>());
    }
    return nSaved;
}

void ORelationController::describeSupportedFeatures()
{
    OJoinController::describeSupportedFeatures();
    implDescribeSupportedFeature( u".uno:DBAddRelation"_ustr, SID_RELATION_ADD_RELATION, CommandGroup::EDIT );
}

namespace
{
    class RelationLoader : public ::osl::Thread
    {
        typedef std::map<OUString, std::shared_ptr<OTableWindowData>, ::comphelper::UStringMixLess> TTableDataHelper;
        TTableDataHelper                    m_aTableData;
        TTableConnectionData                m_vTableConnectionData;
        const Sequence< OUString>    m_aTableList;
        ORelationController*                m_pParent;
        const Reference< XDatabaseMetaData> m_xMetaData;
        const Reference< XNameAccess >      m_xTables;
        const sal_Int32                     m_nStartIndex;
        const sal_Int32                     m_nEndIndex;

    public:
        RelationLoader(ORelationController* _pParent
                        ,const Reference< XDatabaseMetaData>& _xMetaData
                        ,const Reference< XNameAccess >& _xTables
                        ,const Sequence< OUString>& _aTableList
                        ,const sal_Int32 _nStartIndex
                        ,const sal_Int32 _nEndIndex)
            :m_aTableData(comphelper::UStringMixLess(_xMetaData.is() && _xMetaData->supportsMixedCaseQuotedIdentifiers()))
            ,m_aTableList(_aTableList)
            ,m_pParent(_pParent)
            ,m_xMetaData(_xMetaData)
            ,m_xTables(_xTables)
            ,m_nStartIndex(_nStartIndex)
            ,m_nEndIndex(_nEndIndex)
        {
        }

        /// Working method which should be overridden.
        virtual void SAL_CALL run() override;
        virtual void SAL_CALL onTerminated() override;
    protected:
        virtual ~RelationLoader() override {}

        void loadTableData(const Any& _aTable);
    };

    void SAL_CALL RelationLoader::run()
    {
        osl_setThreadName("RelationLoader");

        for(sal_Int32 i = m_nStartIndex; i < m_nEndIndex; ++i)
        {
            OUString sCatalog,sSchema,sTable;
            ::dbtools::qualifiedNameComponents(m_xMetaData,
                                                m_aTableList[i],
                                                sCatalog,
                                                sSchema,
                                                sTable,
                                                ::dbtools::EComposeRule::InDataManipulation);
            Any aCatalog;
            if ( !sCatalog.isEmpty() )
                aCatalog <<= sCatalog;

            try
            {
                Reference< XResultSet > xResult = m_xMetaData->getImportedKeys(aCatalog, sSchema,sTable);
                if ( xResult.is() && xResult->next() )
                {
                    ::comphelper::disposeComponent(xResult);
                    loadTableData(m_xTables->getByName(m_aTableList[i]));
                }
            }
            catch( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("dbaccess");
            }
        }
    }
    void SAL_CALL RelationLoader::onTerminated()
    {
        m_pParent->mergeData(m_vTableConnectionData);
        delete this;
    }

    void RelationLoader::loadTableData(const Any& _aTable)
    {
        Reference<XPropertySet> xTableProp(_aTable,UNO_QUERY);
        const OUString sSourceName = ::dbtools::composeTableName( m_xMetaData, xTableProp, ::dbtools::EComposeRule::InTableDefinitions, false );
        TTableDataHelper::const_iterator aFind = m_aTableData.find(sSourceName);
        if ( aFind == m_aTableData.end() )
        {
            aFind = m_aTableData.emplace(sSourceName,std::make_shared<OTableWindowData>(xTableProp,sSourceName, sSourceName, OUString())).first;
            aFind->second->ShowAll(false);
        }
        TTableWindowData::value_type pReferencingTable = aFind->second;
        Reference<XIndexAccess> xKeys = pReferencingTable->getKeys();
        const Reference<XKeysSupplier> xKeySup(xTableProp,UNO_QUERY);

        if ( !xKeys.is() && xKeySup.is() )
        {
            xKeys = xKeySup->getKeys();
        }

        if ( !xKeys.is() )
            return;

        Reference<XPropertySet> xKey;
        const sal_Int32 nCount = xKeys->getCount();
        for(sal_Int32 i = 0 ; i < nCount ; ++i)
        {
            xKeys->getByIndex(i) >>= xKey;
            sal_Int32 nKeyType = 0;
            xKey->getPropertyValue(PROPERTY_TYPE) >>= nKeyType;
            if ( KeyType::FOREIGN == nKeyType )
            {
                OUString sReferencedTable;
                xKey->getPropertyValue(PROPERTY_REFERENCEDTABLE) >>= sReferencedTable;

                // insert windows
                TTableDataHelper::const_iterator aRefFind = m_aTableData.find(sReferencedTable);
                if ( aRefFind == m_aTableData.end() )
                {
                    if ( m_xTables->hasByName(sReferencedTable) )
                    {
                        Reference<XPropertySet>  xReferencedTable(m_xTables->getByName(sReferencedTable),UNO_QUERY);
                        aRefFind = m_aTableData.emplace(sReferencedTable,std::make_shared<OTableWindowData>(xReferencedTable,sReferencedTable, sReferencedTable, OUString())).first;
                        aRefFind->second->ShowAll(false);
                    }
                    else
                        continue; // table name could not be found so we do not show this table relation
                }
                TTableWindowData::value_type pReferencedTable = aRefFind->second;

                OUString sKeyName;
                xKey->getPropertyValue(PROPERTY_NAME) >>= sKeyName;
                // insert connection
                auto xTabConnData = std::make_shared<ORelationTableConnectionData>( pReferencingTable, pReferencedTable, sKeyName );
                m_vTableConnectionData.push_back(xTabConnData);
                // insert columns
                const Reference<XColumnsSupplier> xColsSup(xKey,UNO_QUERY);
                OSL_ENSURE(xColsSup.is(),"Key is no XColumnsSupplier!");
                const Reference<XNameAccess> xColumns = xColsSup->getColumns();
                const Sequence< OUString> aNames = xColumns->getElementNames();
                OUString sColumnName,sRelatedName;
                for(sal_Int32 j=0;j<aNames.getLength();++j)
                {
                    const Reference<XPropertySet> xPropSet(xColumns->getByName(aNames[j]),UNO_QUERY);
                    OSL_ENSURE(xPropSet.is(),"Invalid column found in KeyColumns!");
                    if ( xPropSet.is() )
                    {
                        xPropSet->getPropertyValue(PROPERTY_NAME)           >>= sColumnName;
                        xPropSet->getPropertyValue(PROPERTY_RELATEDCOLUMN)  >>= sRelatedName;
                    }
                    xTabConnData->SetConnLine( j, sColumnName, sRelatedName );
                }
                // set update/del flags
                sal_Int32   nUpdateRule = 0;
                sal_Int32   nDeleteRule = 0;
                xKey->getPropertyValue(PROPERTY_UPDATERULE) >>= nUpdateRule;
                xKey->getPropertyValue(PROPERTY_DELETERULE) >>= nDeleteRule;

                xTabConnData->SetUpdateRules( nUpdateRule );
                xTabConnData->SetDeleteRules( nDeleteRule );

                // set cardinality
                xTabConnData->SetCardinality();
            }
        }
    }
}

void ORelationController::mergeData(const TTableConnectionData& _aConnectionData)
{
    ::osl::MutexGuard aGuard( getMutex() );

    m_vTableConnectionData.insert( m_vTableConnectionData.end(), _aConnectionData.begin(), _aConnectionData.end() );
    // here we are finished, so we can collect the table from connection data
    for (auto const& elem : m_vTableConnectionData)
    {
        if ( !existsTable(elem->getReferencingTable()->GetComposedName()) )
        {
            m_vTableData.push_back(elem->getReferencingTable());
        }
        if ( !existsTable(elem->getReferencedTable()->GetComposedName()) )
        {
            m_vTableData.push_back(elem->getReferencedTable());
        }
    }
    if ( m_nThreadEvent )
    {
        --m_nThreadEvent;
        if ( !m_nThreadEvent )
            Application::PostUserEvent(LINK(this, ORelationController, OnThreadFinished));
    }
}

IMPL_LINK_NOARG( ORelationController, OnThreadFinished, void*, void )
{
    ::SolarMutexGuard aSolarGuard;
    ::osl::MutexGuard aGuard( getMutex() );
    try
    {
        getView()->initialize();    // show the windows and fill with our information
        getView()->Invalidate(InvalidateFlags::NoErase);
        ClearUndoManager();
        setModified(false);     // and we are not modified yet

        if(m_vTableData.empty())
            Execute(ID_BROWSER_ADDTABLE,Sequence<PropertyValue>());
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("dbaccess");
    }
    m_xWaitObject.reset();
}

void ORelationController::loadData()
{
    m_xWaitObject.reset(new weld::WaitObject(getFrameWeld()));
    try
    {
        if ( !m_xTables.is() )
            return;
        DatabaseMetaData aMeta(getConnection());
        // this may take some time
        const Reference< XDatabaseMetaData> xMetaData = getConnection()->getMetaData();
        const Sequence< OUString> aNames = m_xTables->getElementNames();
        const sal_Int32 nCount = aNames.getLength();
        if ( aMeta.supportsThreads() )
        {
            const sal_Int32 nMaxElements = (nCount / MAX_THREADS) +1;
            sal_Int32 nStart = 0,nEnd = std::min(nMaxElements,nCount);
            while(nStart != nEnd)
            {
                ++m_nThreadEvent;
                RelationLoader* pThread = new RelationLoader(this,xMetaData,m_xTables,aNames,nStart,nEnd);
                pThread->createSuspended();
                pThread->setPriority(osl_Thread_PriorityBelowNormal);
                pThread->resume();
                nStart = nEnd;
                nEnd += nMaxElements;
                nEnd = std::min(nEnd,nCount);
            }
        }
        else
        {
            RelationLoader* pThread = new RelationLoader(this,xMetaData,m_xTables,aNames,0,nCount);
            pThread->run();
            pThread->onTerminated();
        }
    }
    catch(SQLException& e)
    {
        showError(SQLExceptionInfo(e));
    }
    catch(const Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("dbaccess");
    }
}

TTableWindowData::value_type ORelationController::existsTable(std::u16string_view _rComposedTableName)  const
{
    ::comphelper::UStringMixEqual bCase(true);
    for (auto const& elem : m_vTableData)
    {
        if(bCase(elem->GetComposedName(),_rComposedTableName))
            return elem;
    }
    return TTableWindowData::value_type();
}

void ORelationController::loadLayoutInformation()
{
    try
    {
        OSL_ENSURE(haveDataSource(),"We need a datasource from our connection!");
        if ( haveDataSource() )
        {
            if ( getDataSource()->getPropertySetInfo()->hasPropertyByName(PROPERTY_LAYOUTINFORMATION) )
            {
                Sequence<PropertyValue> aWindows;
                getDataSource()->getPropertyValue(PROPERTY_LAYOUTINFORMATION) >>= aWindows;
                loadTableWindows(aWindows);
            }
        }
    }
    catch(Exception&)
    {
    }
}

void ORelationController::reset()
{
    loadLayoutInformation();
    ODataView* pView = getView();
    OSL_ENSURE(pView,"No current view!");
    if(pView)
    {
        pView->initialize();
        pView->Invalidate(InvalidateFlags::NoErase);
    }
}

bool ORelationController::allowViews() const
{
    return false;
}

bool ORelationController::allowQueries() const
{
    return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
