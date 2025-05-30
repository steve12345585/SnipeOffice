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

#include "abspilot.hxx"
#include <helpids.h>
#include <strings.hrc>
#include <componentmodule.hxx>
#include <tools/debug.hxx>
#include "typeselectionpage.hxx"
#include "admininvokationpage.hxx"
#include "tableselectionpage.hxx"
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <osl/diagnose.h>
#include "abpfinalpage.hxx"
#include "fieldmappingpage.hxx"
#include "fieldmappingimpl.hxx"

using vcl::RoadmapWizardTypes::PathId;

namespace abp
{


#define STATE_SELECT_ABTYPE         0
#define STATE_INVOKE_ADMIN_DIALOG   1
#define STATE_TABLE_SELECTION       2
#define STATE_MANUAL_FIELD_MAPPING  3
#define STATE_FINAL_CONFIRM         4



    using namespace ::com::sun::star::uno;

    OAddressBookSourcePilot::OAddressBookSourcePilot(weld::Window* _pParent, const Reference< XComponentContext >& _rxORB)
        :OAddressBookSourcePilot_Base( _pParent )
        ,m_xORB(_rxORB)
        ,m_aNewDataSource(_rxORB)
        ,m_eNewDataSourceType( AST_INVALID )
    {
        declarePath(PathId::COMPLETE,
            {STATE_SELECT_ABTYPE,
            STATE_INVOKE_ADMIN_DIALOG,
            STATE_TABLE_SELECTION,
            STATE_MANUAL_FIELD_MAPPING,
            STATE_FINAL_CONFIRM}
        );
        declarePath(PathId::NO_SETTINGS,
            {STATE_SELECT_ABTYPE,
            STATE_TABLE_SELECTION,
            STATE_MANUAL_FIELD_MAPPING,
            STATE_FINAL_CONFIRM}
        );
        declarePath(PathId::NO_FIELDS,
            {STATE_SELECT_ABTYPE,
            STATE_INVOKE_ADMIN_DIALOG,
            STATE_TABLE_SELECTION,
            STATE_FINAL_CONFIRM}
        );
        declarePath(PathId::NO_SETTINGS_NO_FIELDS,
            {STATE_SELECT_ABTYPE,
            STATE_TABLE_SELECTION,
            STATE_FINAL_CONFIRM}
        );

        m_xPrevPage->set_help_id(HID_ABSPILOT_PREVIOUS);
        m_xNextPage->set_help_id(HID_ABSPILOT_NEXT);
        m_xCancel->set_help_id(HID_ABSPILOT_CANCEL);
        m_xFinish->set_help_id(HID_ABSPILOT_FINISH);
        m_xHelp->set_help_id(UID_ABSPILOT_HELP);

        // some initial settings
#ifdef UNX
#ifdef MACOSX
        m_aSettings.eType = AST_MACAB;
#else
// FIXME: if KDE use KAB instead
        m_aSettings.eType = AST_EVOLUTION;
#endif
#else
        m_aSettings.eType = AST_OTHER;
#endif
        m_aSettings.sDataSourceName = compmodule::ModuleRes(RID_STR_DEFAULT_NAME);
        m_aSettings.bRegisterDataSource = false;
        m_aSettings.bEmbedDataSource = false;
        m_aSettings.bIgnoreNoTable = false;

        defaultButton(WizardButtonFlags::NEXT);
        enableButtons(WizardButtonFlags::FINISH, false);
        ActivatePage();
        m_xAssistant->set_current_page(0);

        typeSelectionChanged( m_aSettings.eType );

        OUString sDialogTitle = compmodule::ModuleRes(RID_STR_ABSOURCEDIALOGTITLE);
        setTitleBase(sDialogTitle);
        m_xAssistant->set_help_id(HID_ABSPILOT);
    }

    OUString OAddressBookSourcePilot::getStateDisplayName( WizardState _nState ) const
    {
        TranslateId pResId;
        switch ( _nState )
        {
            case STATE_SELECT_ABTYPE:        pResId = RID_STR_SELECT_ABTYPE; break;
            case STATE_INVOKE_ADMIN_DIALOG:  pResId = RID_STR_INVOKE_ADMIN_DIALOG; break;
            case STATE_TABLE_SELECTION:      pResId = RID_STR_TABLE_SELECTION; break;
            case STATE_MANUAL_FIELD_MAPPING: pResId = RID_STR_MANUAL_FIELD_MAPPING; break;
            case STATE_FINAL_CONFIRM:        pResId = RID_STR_FINAL_CONFIRM; break;
        }
        DBG_ASSERT( pResId, "OAddressBookSourcePilot::getStateDisplayName: don't know this state!" );

        OUString sDisplayName;
        if (pResId)
        {
            sDisplayName = compmodule::ModuleRes(pResId);
        }

        return sDisplayName;
    }

    void OAddressBookSourcePilot::implCommitAll()
    {
        // in real, the data source already exists in the data source context
        // Thus, if the user changed the name, we have to rename the data source
        if ( m_aSettings.sDataSourceName != m_aNewDataSource.getName() )
            m_aNewDataSource.rename( m_aSettings.sDataSourceName );

        // 1. the data source
        m_aNewDataSource.store(m_aSettings);

        // 2. check if we need to register the data source
        if ( m_aSettings.bRegisterDataSource )
            m_aNewDataSource.registerDataSource(m_aSettings.sRegisteredDataSourceName);

        // 3. write the data source / table names into the configuration
        addressconfig::writeTemplateAddressSource( getORB(), m_aSettings.bRegisterDataSource ? m_aSettings.sRegisteredDataSourceName : m_aSettings.sDataSourceName, m_aSettings.sSelectedTable );

        // 4. write the field mapping
        fieldmapping::writeTemplateAddressFieldMapping( getORB(), std::map(m_aSettings.aFieldMapping) );
    }

    void OAddressBookSourcePilot::implCleanup()
    {
        if ( m_aNewDataSource.isValid() )
            m_aNewDataSource.remove();
    }

    short OAddressBookSourcePilot::run()
    {
        short nRet = OAddressBookSourcePilot_Base::run();

        implCleanup();

        return nRet;
    }

    bool OAddressBookSourcePilot::onFinish()
    {
        if ( !OAddressBookSourcePilot_Base::onFinish() )
            return false;

        implCommitAll();

        addressconfig::markPilotSuccess( getORB() );

        return true;
    }

    void OAddressBookSourcePilot::enterState( WizardState _nState )
    {
        switch ( _nState )
        {
            case STATE_SELECT_ABTYPE:
                impl_updateRoadmap( static_cast< TypeSelectionPage* >( GetPage( STATE_SELECT_ABTYPE ) )->getSelectedType() );
                break;

            case STATE_FINAL_CONFIRM:
                if ( !needManualFieldMapping( ) )
                    implDoAutoFieldMapping();
                break;

            case STATE_TABLE_SELECTION:
                implDefaultTableName();
                break;
        }

        OAddressBookSourcePilot_Base::enterState(_nState);
    }


    bool OAddressBookSourcePilot::prepareLeaveCurrentState( CommitPageReason _eReason )
    {
        if ( !OAddressBookSourcePilot_Base::prepareLeaveCurrentState( _eReason ) )
            return false;

        if ( _eReason == vcl::WizardTypes::eTravelBackward )
            return true;

        bool bAllow = true;

        switch ( getCurrentState() )
        {
        case STATE_SELECT_ABTYPE:
            implCreateDataSource();
            if ( needAdminInvokationPage() )
                break;
            [[fallthrough]];

        case STATE_INVOKE_ADMIN_DIALOG:
            if ( !connectToDataSource( false ) )
            {
                // connecting did not succeed -> do not allow proceeding
                bAllow = false;
                break;
            }


            // now that we connected to the data source, check whether we need the "table selection" page
            const StringBag& aTables = m_aNewDataSource.getTableNames();

            if ( aTables.empty() )
            {
                std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(m_xAssistant.get(),
                                                          VclMessageType::Question, VclButtonsType::YesNo,
                                                          compmodule::ModuleRes(getSettings().eType == AST_EVOLUTION_GROUPWISE ? RID_STR_QRY_NO_EVO_GW : RID_STR_QRY_NOTABLES)));

                if (RET_YES != xBox->run())
                {
                    // cannot ask the user, or the user chose to use this data source, though there are no tables
                    bAllow = false;
                    break;
                }

                m_aSettings.bIgnoreNoTable = true;
            }

            if ( aTables.size() == 1 )
                // remember the one and only table we have
                m_aSettings.sSelectedTable = *aTables.begin();

            break;
        }

        impl_updateRoadmap( m_aSettings.eType );
        return bAllow;
    }

    void OAddressBookSourcePilot::implDefaultTableName()
    {
        const StringBag& rTableNames = getDataSource().getTableNames();
        if ( rTableNames.end() != rTableNames.find( getSettings().sSelectedTable ) )
            // already a valid table selected
            return;

        const char* pGuess = nullptr;
        switch ( getSettings().eType )
        {
            case AST_THUNDERBIRD        : pGuess = "Personal Address book"; break;
            case AST_EVOLUTION          :
            case AST_EVOLUTION_GROUPWISE:
            case AST_EVOLUTION_LDAP     : pGuess = "Personal"; break;
            default:
                OSL_FAIL( "OAddressBookSourcePilot::implDefaultTableName: unhandled case!" );
                return;
        }
        const OUString sGuess = OUString::createFromAscii( pGuess );
        if ( rTableNames.end() != rTableNames.find( sGuess ) )
            getSettings().sSelectedTable = sGuess;
    }

    void OAddressBookSourcePilot::implDoAutoFieldMapping()
    {
        DBG_ASSERT( !needManualFieldMapping( ), "OAddressBookSourcePilot::implDoAutoFieldMapping: invalid call!" );

        fieldmapping::defaultMapping( getORB(), m_aSettings.aFieldMapping );
    }

    void OAddressBookSourcePilot::implCreateDataSource()
    {
        if (m_aNewDataSource.isValid())
        {   // we already have a data source object
            if ( m_aSettings.eType == m_eNewDataSourceType )
                // and it already has the correct type
                return;

            // it has a wrong type -> remove it
            m_aNewDataSource.remove();
        }

        ODataSourceContext aContext( getORB() );
        aContext.disambiguate( m_aSettings.sDataSourceName );

        switch (m_aSettings.eType)
        {
            case AST_THUNDERBIRD:
                m_aNewDataSource = aContext.createNewThunderbird( m_aSettings.sDataSourceName );
                break;

            case AST_EVOLUTION:
                m_aNewDataSource = aContext.createNewEvolution( m_aSettings.sDataSourceName );
                break;

            case AST_EVOLUTION_GROUPWISE:
                m_aNewDataSource = aContext.createNewEvolutionGroupwise( m_aSettings.sDataSourceName );
                break;

            case AST_EVOLUTION_LDAP:
                m_aNewDataSource = aContext.createNewEvolutionLdap( m_aSettings.sDataSourceName );
                break;

            case AST_KAB:
                m_aNewDataSource = aContext.createNewKab( m_aSettings.sDataSourceName );
                break;

            case AST_MACAB:
                m_aNewDataSource = aContext.createNewMacab( m_aSettings.sDataSourceName );
                break;

            case AST_OTHER:
                m_aNewDataSource = aContext.createNewOther( m_aSettings.sDataSourceName );
                break;

            case AST_INVALID:
                OSL_FAIL( "OAddressBookSourcePilot::implCreateDataSource: illegal data source type!" );
                break;
        }
        m_eNewDataSourceType = m_aSettings.eType;
    }

    bool OAddressBookSourcePilot::connectToDataSource( bool _bForceReConnect )
    {
        DBG_ASSERT( m_aNewDataSource.isValid(), "OAddressBookSourcePilot::implConnect: invalid current data source!" );

        weld::WaitObject aWaitCursor(m_xAssistant.get());
        if ( _bForceReConnect && m_aNewDataSource.isConnected( ) )
            m_aNewDataSource.disconnect( );

        return m_aNewDataSource.connect(m_xAssistant.get());
    }

    std::unique_ptr<BuilderPage> OAddressBookSourcePilot::createPage(WizardState _nState)
    {
        OUString sIdent(OUString::number(_nState));
        weld::Container* pPageContainer = m_xAssistant->append_page(sIdent);

        std::unique_ptr<vcl::OWizardPage> xRet;

        switch (_nState)
        {
            case STATE_SELECT_ABTYPE:
                xRet = std::make_unique<TypeSelectionPage>(pPageContainer, this);
                break;
            case STATE_INVOKE_ADMIN_DIALOG:
                xRet = std::make_unique<AdminDialogInvokationPage>(pPageContainer, this);
                break;
            case STATE_TABLE_SELECTION:
                xRet = std::make_unique<TableSelectionPage>(pPageContainer, this);
                break;
            case STATE_MANUAL_FIELD_MAPPING:
                xRet = std::make_unique<FieldMappingPage>(pPageContainer, this);
                break;
            case STATE_FINAL_CONFIRM:
                xRet = std::make_unique<FinalPage>(pPageContainer, this);
                break;
            default:
                assert(false && "OAddressBookSourcePilot::createPage: invalid state!");
                break;
        }

        m_xAssistant->set_page_title(sIdent, getStateDisplayName(_nState));

        return xRet;
    }

    void OAddressBookSourcePilot::impl_updateRoadmap( AddressSourceType _eType )
    {
        bool bSettingsPage = needAdminInvokationPage( _eType );
        bool bTablesPage   = needTableSelection( _eType );
        bool bFieldsPage   = needManualFieldMapping( _eType );

        bool bConnected = m_aNewDataSource.isConnected();
        bool bCanSkipTables =
                (   m_aNewDataSource.hasTable( m_aSettings.sSelectedTable )
                ||  m_aSettings.bIgnoreNoTable
                );

        enableState( STATE_INVOKE_ADMIN_DIALOG, bSettingsPage );

        enableState( STATE_TABLE_SELECTION,
            bTablesPage &&  ( bConnected ? !bCanSkipTables : !bSettingsPage )
            // if we do not need a settings page, we connect upon "Next" on the first page
        );

        enableState( STATE_MANUAL_FIELD_MAPPING,
                bFieldsPage && bConnected && m_aNewDataSource.hasTable( m_aSettings.sSelectedTable )
        );

        enableState( STATE_FINAL_CONFIRM,
            bConnected && bCanSkipTables
        );
    }

    void OAddressBookSourcePilot::typeSelectionChanged( AddressSourceType _eType )
    {
        PathId nCurrentPathID( PathId::COMPLETE );
        bool bSettingsPage = needAdminInvokationPage( _eType );
        bool bFieldsPage = needManualFieldMapping( _eType );
        if ( !bSettingsPage )
            if ( !bFieldsPage )
                nCurrentPathID = PathId::NO_SETTINGS_NO_FIELDS;
            else
                nCurrentPathID = PathId::NO_SETTINGS;
        else
            if ( !bFieldsPage )
                nCurrentPathID = PathId::NO_FIELDS;
            else
                nCurrentPathID = PathId::COMPLETE;
        activatePath( nCurrentPathID, true );

        m_aNewDataSource.disconnect();
        m_aSettings.bIgnoreNoTable = false;
        impl_updateRoadmap( _eType );
    }

}   // namespace abp


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
