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

#include "controlwizard.hxx"
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/sdbcx/XTablesSupplier.hpp>
#include <com/sun/star/sdb/DatabaseContext.hpp>
#include <com/sun/star/sdb/XQueriesSupplier.hpp>
#include <com/sun/star/sdbc/XPreparedStatement.hpp>
#include <com/sun/star/container/XChild.hpp>
#include <com/sun/star/sdbcx/XColumnsSupplier.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/sheet/XSpreadsheetView.hpp>
#include <com/sun/star/drawing/XDrawView.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>
#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/sdbc/SQLWarning.hpp>
#include <com/sun/star/sdb/SQLContext.hpp>
#include <com/sun/star/task/InteractionHandler.hpp>
#include <comphelper/types.hxx>
#include <connectivity/dbtools.hxx>
#include <comphelper/interaction.hxx>
#include <vcl/stdtext.hxx>
#include <connectivity/conncleanup.hxx>
#include <com/sun/star/sdbc/DataType.hpp>
#include <tools/urlobj.hxx>

#define WIZARD_SIZE_X   60
#define WIZARD_SIZE_Y   23

namespace dbp
{
    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::awt;
    using namespace ::com::sun::star::sdb;
    using namespace ::com::sun::star::sdbc;
    using namespace ::com::sun::star::sdbcx;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::container;
    using namespace ::com::sun::star::drawing;
    using namespace ::com::sun::star::frame;
    using namespace ::com::sun::star::sheet;
    using namespace ::com::sun::star::form;
    using namespace ::com::sun::star::task;
    using namespace ::comphelper;
    using namespace ::dbtools;

    struct OAccessRegulator
    {
        friend class OControlWizardPage;

    protected:
        OAccessRegulator() { }
    };

    OControlWizardPage::OControlWizardPage(weld::Container* pPage, OControlWizard* pWizard, const OUString& rUIXMLDescription, const OUString& rID)
        : OControlWizardPage_Base(pPage, pWizard, rUIXMLDescription, rID)
        , m_pDialog(pWizard)
    {
        m_xContainer->set_size_request(m_xContainer->get_approximate_digit_width() * WIZARD_SIZE_X,
                                       m_xContainer->get_text_height() * WIZARD_SIZE_Y);
    }

    OControlWizardPage::~OControlWizardPage()
    {
    }

    OControlWizard* OControlWizardPage::getDialog()
    {
        return m_pDialog;
    }

    const OControlWizard* OControlWizardPage::getDialog() const
    {
        return m_pDialog;
    }

    bool OControlWizardPage::updateContext()
    {
        return m_pDialog->updateContext(OAccessRegulator());
    }

    Reference< XConnection > OControlWizardPage::getFormConnection() const
    {
        return m_pDialog->getFormConnection(OAccessRegulator());
    }

    void OControlWizardPage::setFormConnection( const Reference< XConnection >& _rxConn, bool _bAutoDispose )
    {
        m_pDialog->setFormConnection( OAccessRegulator(), _rxConn, _bAutoDispose );
    }

    const OControlWizardContext& OControlWizardPage::getContext() const
    {
        return m_pDialog->getContext();
    }

    void OControlWizardPage::fillListBox(weld::TreeView& _rList, const Sequence< OUString >& _rItems)
    {
        _rList.clear();
        for (sal_Int32 nIndex = 0; nIndex < _rItems.getLength(); ++nIndex)
        {
            _rList.append(OUString::number(nIndex), _rItems[nIndex]);
        }
    }

    void OControlWizardPage::fillListBox(weld::ComboBox& _rList, const Sequence< OUString >& _rItems)
    {
        _rList.clear();
        for (auto& item : _rItems)
        {
            _rList.append_text(item);
        }
    }

    void OControlWizardPage::enableFormDatasourceDisplay()
    {
        if (m_xFormContentType)
            // nothing to do
            return;

        m_xFrame = m_xBuilder->weld_frame(u"sourceframe"_ustr);
        m_xFrame->show();
        m_xFormContentType = m_xBuilder->weld_label(u"contenttype"_ustr);
        m_xFormContentTypeLabel = m_xBuilder->weld_label(u"contenttypelabel"_ustr);
        m_xFormDatasource = m_xBuilder->weld_label(u"datasource"_ustr);
        m_xFormDatasourceLabel = m_xBuilder->weld_label(u"datasourcelabel"_ustr);
        m_xFormTable = m_xBuilder->weld_label(u"formtable"_ustr);

        const OControlWizardContext& rContext = getContext();
        if ( rContext.bEmbedded )
        {
            m_xFormDatasourceLabel->hide();
            m_xFormDatasource->hide();
        }
    }

    void OControlWizardPage::initializePage()
    {
        if (m_xFormDatasource && m_xFormContentTypeLabel && m_xFormTable)
        {
            const OControlWizardContext& rContext = getContext();
            OUString sDataSource;
            OUString sCommand;
            sal_Int32 nCommandType = CommandType::COMMAND;
            try
            {
                rContext.xForm->getPropertyValue(u"DataSourceName"_ustr) >>= sDataSource;
                rContext.xForm->getPropertyValue(u"Command"_ustr) >>= sCommand;
                rContext.xForm->getPropertyValue(u"CommandType"_ustr) >>= nCommandType;
            }
            catch(const Exception&)
            {
                TOOLS_WARN_EXCEPTION("extensions.dbpilots", "OControlWizardPage::initializePage");
            }

            INetURLObject aURL( sDataSource );
            if( aURL.GetProtocol() != INetProtocol::NotValid )
                sDataSource = aURL.GetLastName(INetURLObject::DecodeMechanism::WithCharset);
            m_xFormDatasource->set_label(sDataSource);
            m_xFormTable->set_label(sCommand);

            TranslateId pCommandTypeResourceId;
            switch (nCommandType)
            {
                case CommandType::TABLE:
                    pCommandTypeResourceId = RID_STR_TYPE_TABLE;
                    break;

                case CommandType::QUERY:
                    pCommandTypeResourceId = RID_STR_TYPE_QUERY;
                    break;

                default:
                    pCommandTypeResourceId = RID_STR_TYPE_COMMAND;
                    break;
            }
            m_xFormContentType->set_label(compmodule::ModuleRes(pCommandTypeResourceId));
        }

        OControlWizardPage_Base::initializePage();
    }

    OControlWizard::OControlWizard(weld::Window* _pParent,
            const Reference< XPropertySet >& _rxObjectModel, const Reference< XComponentContext >& _rxContext )
        : WizardMachine(_pParent, WizardButtonFlags::CANCEL | WizardButtonFlags::PREVIOUS | WizardButtonFlags::NEXT | WizardButtonFlags::FINISH)
        , m_xContext(_rxContext)
    {
        m_aContext.xObjectModel = _rxObjectModel;
        initContext();

        defaultButton(WizardButtonFlags::NEXT);
        enableButtons(WizardButtonFlags::FINISH, false);
    }

    OControlWizard::~OControlWizard()
    {
    }

    short OControlWizard::run()
    {
        // get the class id of the control we're dealing with
        sal_Int16 nClassId = FormComponentType::CONTROL;
        try
        {
            getContext().xObjectModel->getPropertyValue(u"ClassId"_ustr) >>= nClassId;
        }
        catch(const Exception&)
        {
            OSL_FAIL("OControlWizard::activate: could not obtain the class id!");
        }
        if (!approveControl(nClassId))
        {
            // TODO: MessageBox or exception
            return RET_CANCEL;
        }

        ActivatePage();

        m_xAssistant->set_current_page(0);

        return OControlWizard_Base::run();
    }

    void OControlWizard::implDetermineShape()
    {
        Reference< XIndexAccess > xPageObjects = m_aContext.xDrawPage;
        DBG_ASSERT(xPageObjects.is(), "OControlWizard::implDetermineShape: invalid page!");

        // for comparing the model
        Reference< XControlModel > xModelCompare(m_aContext.xObjectModel, UNO_QUERY);

        if (!xPageObjects.is())
            return;

        // loop through all objects of the page
        sal_Int32 nObjects = xPageObjects->getCount();
        Reference< XControlShape > xControlShape;
        Reference< XControlModel > xControlModel;
        for (sal_Int32 i=0; i<nObjects; ++i)
        {
            if (xPageObjects->getByIndex(i) >>= xControlShape)
            {   // it _is_ a control shape
                xControlModel = xControlShape->getControl();
                DBG_ASSERT(xControlModel.is(), "OControlWizard::implDetermineShape: control shape without model!");
                if (xModelCompare.get() == xControlModel.get())
                {
                    m_aContext.xObjectShape = xControlShape;
                    break;
                }
            }
        }
    }


    void OControlWizard::implDetermineForm()
    {
        Reference< XChild > xModelAsChild(m_aContext.xObjectModel, UNO_QUERY);
        Reference< XInterface > xControlParent;
        if (xModelAsChild.is())
            xControlParent = xModelAsChild->getParent();

        m_aContext.xForm.set(xControlParent, UNO_QUERY);
        m_aContext.xRowSet.set(xControlParent, UNO_QUERY);
        DBG_ASSERT(m_aContext.xForm.is() && m_aContext.xRowSet.is(),
            "OControlWizard::implDetermineForm: missing some interfaces of the control parent!");

    }


    void OControlWizard::implDeterminePage()
    {
        try
        {
            // get the document model
            Reference< XChild > xControlAsChild(m_aContext.xObjectModel, UNO_QUERY);
            Reference< XChild > xModelSearch(xControlAsChild->getParent(), UNO_QUERY);

            Reference< XModel > xModel(xModelSearch, UNO_QUERY);
            while (xModelSearch.is() && !xModel.is())
            {
                xModelSearch.set(xModelSearch->getParent(), UNO_QUERY);
                xModel.set(xModelSearch, UNO_QUERY);
            }

            Reference< XDrawPage > xPage;
            if (xModel.is())
            {
                m_aContext.xDocumentModel = xModel;

                Reference< XDrawPageSupplier > xPageSupp(xModel, UNO_QUERY);
                if (xPageSupp.is())
                {   // it's a document with only one page -> Writer
                    xPage = xPageSupp->getDrawPage();
                }
                else
                {
                    // get the controller currently working on this model
                    Reference< XController > xController = xModel->getCurrentController();
                    DBG_ASSERT(xController.is(), "OControlWizard::implDeterminePage: no current controller!");

                    // maybe it's a spreadsheet
                    Reference< XSpreadsheetView > xView(xController, UNO_QUERY);
                    if (xView.is())
                    {   // okay, it is one
                        Reference< XSpreadsheet > xSheet = xView->getActiveSheet();
                        xPageSupp.set(xSheet, UNO_QUERY);
                        DBG_ASSERT(xPageSupp.is(), "OControlWizard::implDeterminePage: a spreadsheet which is no page supplier!");
                        if (xPageSupp.is())
                            xPage = xPageSupp->getDrawPage();
                    }
                    else
                    {   // can be a draw/impress doc only
                        Reference< XDrawView > xDrawView(xController, UNO_QUERY);
                        DBG_ASSERT(xDrawView.is(), "OControlWizard::implDeterminePage: no alternatives left ... can't determine the page!");
                        if (xDrawView.is())
                            xPage = xDrawView->getCurrentPage();
                    }
                }
            }
            else
            {
                DBG_ASSERT(xPage.is(), "OControlWizard::implDeterminePage: can't determine the page (no model)!");
            }
            m_aContext.xDrawPage = std::move(xPage);
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION("extensions.dbpilots", "OControlWizard::implDeterminePage");
        }
    }


    void OControlWizard::implGetDSContext()
    {
        try
        {
            DBG_ASSERT(m_xContext.is(), "OControlWizard::implGetDSContext: invalid service factory!");

            m_aContext.xDatasourceContext = DatabaseContext::create(m_xContext);
        }
        catch(const Exception&)
        {
            OSL_FAIL("OControlWizard::implGetDSContext: invalid database context!");
        }
    }


    Reference< XConnection > OControlWizard::getFormConnection(const OAccessRegulator&) const
    {
        return getFormConnection();
    }

    Reference< XConnection > OControlWizard::getFormConnection() const
    {
        Reference< XConnection > xConn;
        try
        {
            if ( !::dbtools::isEmbeddedInDatabase(m_aContext.xForm,xConn) )
                m_aContext.xForm->getPropertyValue(u"ActiveConnection"_ustr) >>= xConn;
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION("extensions.dbpilots", "OControlWizard::getFormConnection");
        }
        return xConn;
    }


    void OControlWizard::setFormConnection( const OAccessRegulator& _rAccess, const Reference< XConnection >& _rxConn, bool _bAutoDispose )
    {
        try
        {
            Reference< XConnection > xOldConn = getFormConnection(_rAccess);
            if (xOldConn.get() == _rxConn.get())
                return;

            disposeComponent(xOldConn);

            // set the new connection
            if ( _bAutoDispose )
            {
                // for this, use an AutoDisposer (so the conn is cleaned up when the form dies or gets another connection)
                Reference< XRowSet > xFormRowSet( m_aContext.xForm, UNO_QUERY );
                new OAutoConnectionDisposer( xFormRowSet, _rxConn );
            }
            else
            {
                m_aContext.xForm->setPropertyValue(u"ActiveConnection"_ustr, Any( _rxConn ) );
            }
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION( "extensions.dbpilots", "OControlWizard::setFormConnection");
        }
    }


    bool OControlWizard::updateContext(const OAccessRegulator&)
    {
        return initContext();
    }

    Reference< XInteractionHandler > OControlWizard::getInteractionHandler(weld::Window* _pWindow) const
    {
        Reference< XInteractionHandler > xHandler;
        try
        {
            xHandler.set( InteractionHandler::createWithParent(m_xContext, nullptr), UNO_QUERY_THROW );
        }
        catch(const Exception&) { }
        if (!xHandler.is())
        {
            ShowServiceNotAvailableError(_pWindow, u"com.sun.star.task.InteractionHandler", true);
        }
        return xHandler;
    }

    bool OControlWizard::initContext()
    {
        DBG_ASSERT(m_aContext.xObjectModel.is(), "OGroupBoxWizard::initContext: have no control model to work with!");
        if (!m_aContext.xObjectModel.is())
            return false;

        // reset the context
        m_aContext.xForm.clear();
        m_aContext.xRowSet.clear();
        m_aContext.xDocumentModel.clear();
        m_aContext.xDrawPage.clear();
        m_aContext.xObjectShape.clear();
        m_aContext.aFieldNames.realloc(0);

        m_aContext.xObjectContainer.clear();
        m_aContext.aTypes.clear();
        m_aContext.bEmbedded = false;

        Any aSQLException;
        Reference< XPreparedStatement >  xStatement;
        try
        {
            // get the datasource context
            implGetDSContext();

            // first, determine the form the control belongs to
            implDetermineForm();

            // need the page, too
            implDeterminePage();

            // the shape of the control
            implDetermineShape();

            // get the columns of the object the settings refer to
            Reference< XNameAccess >  xColumns;

            if (m_aContext.xForm.is())
            {
                // collect some properties of the form
                OUString sObjectName = ::comphelper::getString(m_aContext.xForm->getPropertyValue(u"Command"_ustr));
                sal_Int32 nObjectType = ::comphelper::getINT32(m_aContext.xForm->getPropertyValue(u"CommandType"_ustr));

                // calculate the connection the rowset is working with
                Reference< XConnection > xConnection;
                m_aContext.bEmbedded = ::dbtools::isEmbeddedInDatabase( m_aContext.xForm, xConnection );
                if ( !m_aContext.bEmbedded )
                    xConnection = ::dbtools::connectRowset( m_aContext.xRowSet, m_xContext, nullptr );

                // get the fields
                if (xConnection.is())
                {
                    switch (nObjectType)
                    {
                        case 0:
                        {
                            Reference< XTablesSupplier >  xSupplyTables(xConnection, UNO_QUERY);
                            if (xSupplyTables.is() && xSupplyTables->getTables().is() && xSupplyTables->getTables()->hasByName(sObjectName))
                            {
                                Reference< XColumnsSupplier >  xSupplyColumns;
                                m_aContext.xObjectContainer = xSupplyTables->getTables();
                                m_aContext.xObjectContainer->getByName(sObjectName) >>= xSupplyColumns;
                                DBG_ASSERT(xSupplyColumns.is(), "OControlWizard::initContext: invalid table columns!");
                                xColumns = xSupplyColumns->getColumns();
                            }
                        }
                        break;
                        case 1:
                        {
                            Reference< XQueriesSupplier >  xSupplyQueries(xConnection, UNO_QUERY);
                            if (xSupplyQueries.is() && xSupplyQueries->getQueries().is() && xSupplyQueries->getQueries()->hasByName(sObjectName))
                            {
                                Reference< XColumnsSupplier >  xSupplyColumns;
                                m_aContext.xObjectContainer = xSupplyQueries->getQueries();
                                m_aContext.xObjectContainer->getByName(sObjectName) >>= xSupplyColumns;
                                DBG_ASSERT(xSupplyColumns.is(), "OControlWizard::initContext: invalid query columns!");
                                xColumns  = xSupplyColumns->getColumns();
                            }
                        }
                        break;
                        default:
                        {
                            xStatement = xConnection->prepareStatement(sObjectName);

                            // not interested in any results, only in the fields
                            Reference< XPropertySet > xStatementProps(xStatement, UNO_QUERY);
                            xStatementProps->setPropertyValue(u"MaxRows"_ustr, Any(sal_Int32(0)));

                            // TODO: think about handling local SQLExceptions here ...
                            Reference< XColumnsSupplier >  xSupplyCols(xStatement->executeQuery(), UNO_QUERY);
                            if (xSupplyCols.is())
                                xColumns = xSupplyCols->getColumns();
                        }
                    }
                }
            }

            if (xColumns.is())
            {
                m_aContext.aFieldNames = xColumns->getElementNames();
                for (auto& name : m_aContext.aFieldNames)
                {
                    sal_Int32 nFieldType = DataType::OTHER;
                    try
                    {
                        Reference< XPropertySet > xColumn;
                        xColumns->getByName(name) >>= xColumn;
                        xColumn->getPropertyValue(u"Type"_ustr) >>= nFieldType;
                    }
                    catch(const Exception&)
                    {
                        TOOLS_WARN_EXCEPTION(
                            "extensions.dbpilots",
                            "unexpected exception while gathering column information!");
                    }
                    m_aContext.aTypes.emplace(name, nFieldType);
                }
            }
        }
        catch(const SQLContext& e) { aSQLException <<= e; }
        catch(const SQLWarning& e) { aSQLException <<= e; }
        catch(const SQLException& e) { aSQLException <<= e; }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION( "extensions.dbpilots", "OControlWizard::initContext: could not retrieve the control context");
        }

        ::comphelper::disposeComponent(xStatement);

        if (aSQLException.hasValue())
        {   // an SQLException (or derivee) was thrown ...

            // prepend an extra SQLContext explaining what we were doing
            SQLContext aContext(compmodule::ModuleRes(RID_STR_COULDNOTOPENTABLE), {}, {}, 0,
                                aSQLException, {});

            // create an interaction handler to display this exception
            Reference< XInteractionHandler > xHandler = getInteractionHandler(m_xAssistant.get());
            if ( !xHandler.is() )
                return false;

            Reference< XInteractionRequest > xRequest = new OInteractionRequest(Any(aContext));
            try
            {
                xHandler->handle(xRequest);
            }
            catch(const Exception&) { }
            return false;
        }

        return m_aContext.aFieldNames.hasElements();
    }


    void OControlWizard::commitControlSettings(OControlWizardSettings const * _pSettings)
    {
        DBG_ASSERT(m_aContext.xObjectModel.is(), "OControlWizard::commitControlSettings: have no control model to work with!");
        if (!m_aContext.xObjectModel.is())
            return;

        // the only thing we have at the moment is the label
        try
        {
            Reference< XPropertySetInfo > xInfo = m_aContext.xObjectModel->getPropertySetInfo();
            if (xInfo.is() && xInfo->hasPropertyByName(u"Label"_ustr))
            {
                OUString sControlLabel(_pSettings->sControlLabel);
                m_aContext.xObjectModel->setPropertyValue(
                    u"Label"_ustr,
                    Any(sControlLabel)
                );
            }
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION( "extensions.dbpilots", "OControlWizard::commitControlSettings: could not commit the basic control settings!");
        }
    }


    void OControlWizard::initControlSettings(OControlWizardSettings* _pSettings)
    {
        DBG_ASSERT(m_aContext.xObjectModel.is(), "OControlWizard::initControlSettings: have no control model to work with!");
        if (!m_aContext.xObjectModel.is())
            return;

        // initialize some settings from the control model give
        try
        {
            OUString sLabelPropertyName(u"Label"_ustr);
            Reference< XPropertySetInfo > xInfo = m_aContext.xObjectModel->getPropertySetInfo();
            if (xInfo.is() && xInfo->hasPropertyByName(sLabelPropertyName))
            {
                OUString sControlLabel;
                m_aContext.xObjectModel->getPropertyValue(sLabelPropertyName) >>= sControlLabel;
                _pSettings->sControlLabel = sControlLabel;
            }
        }
        catch(const Exception&)
        {
            TOOLS_WARN_EXCEPTION( "extensions.dbpilots", "OControlWizard::initControlSettings: could not retrieve the basic control settings!");
        }
    }


    bool OControlWizard::needDatasourceSelection()
    {
        // lemme see ...
        return !getContext().aFieldNames.hasElements();
            // if we got fields, the data source is valid ...
    }


}   // namespace dbp


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
