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

#include <config_gio.h>

#include <com/sun/star/awt/Toolkit.hpp>
#include <com/sun/star/frame/Desktop.hpp>
#include <com/sun/star/ui/dialogs/ExecutableDialogResults.hpp>
#include <vcl/svapp.hxx>
#include <unx/gtk/gtkinst.hxx>
#include "SalGtkFolderPicker.hxx"
#include <sal/log.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::ui::dialogs;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;

// constructor

SalGtkFolderPicker::SalGtkFolderPicker( const uno::Reference< uno::XComponentContext >& xContext ) :
    SalGtkPicker( xContext )
{
    m_pDialog = gtk_file_chooser_dialog_new(
        OUStringToOString( getResString( FOLDERPICKER_TITLE ), RTL_TEXTENCODING_UTF8 ).getStr(),
        nullptr, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, getCancelText().getStr(), GTK_RESPONSE_CANCEL,
        getOKText().getStr(), GTK_RESPONSE_ACCEPT, nullptr );
    gtk_window_set_modal(GTK_WINDOW(m_pDialog), true);

    gtk_dialog_set_default_response( GTK_DIALOG (m_pDialog), GTK_RESPONSE_ACCEPT );
#if !GTK_CHECK_VERSION(4, 0, 0)
#if ENABLE_GIO
    gtk_file_chooser_set_local_only( GTK_FILE_CHOOSER( m_pDialog ), false );
#endif
#endif
    gtk_file_chooser_set_select_multiple( GTK_FILE_CHOOSER( m_pDialog ), false );
}

void SAL_CALL SalGtkFolderPicker::setDisplayDirectory( const OUString& aDirectory )
{
    SolarMutexGuard g;

    assert( m_pDialog != nullptr );

    OString aTxt = unicodetouri( aDirectory );
    if( aTxt.isEmpty() ){
      aTxt = unicodetouri(u"file:///."_ustr);
    }

    if( aTxt.endsWith("/") )
        aTxt = aTxt.copy( 0, aTxt.getLength() - 1 );

    SAL_INFO( "vcl", "setting path to " << aTxt );

#if GTK_CHECK_VERSION(4, 0, 0)
    GFile* pPath = g_file_new_for_uri(aTxt.getStr());
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(m_pDialog), pPath, nullptr);
    g_object_unref(pPath);
#else
    gtk_file_chooser_set_current_folder_uri(GTK_FILE_CHOOSER(m_pDialog), aTxt.getStr());
#endif
}

OUString SAL_CALL SalGtkFolderPicker::getDisplayDirectory()
{
    SolarMutexGuard g;

    assert( m_pDialog != nullptr );

#if GTK_CHECK_VERSION(4, 0, 0)
    GFile* pPath =
        gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(m_pDialog));
    gchar* pCurrentFolder = g_file_get_uri(pPath);
    g_object_unref(pPath);
#else
    gchar* pCurrentFolder =
        gtk_file_chooser_get_current_folder_uri(GTK_FILE_CHOOSER(m_pDialog));
#endif

    OUString aCurrentFolderName = uritounicode(pCurrentFolder);
    g_free( pCurrentFolder );

    return aCurrentFolderName;
}

OUString SAL_CALL SalGtkFolderPicker::getDirectory()
{
    SolarMutexGuard g;

    assert( m_pDialog != nullptr );

#if GTK_CHECK_VERSION(4, 0, 0)
    GFile* pPath =
        gtk_file_chooser_get_file(GTK_FILE_CHOOSER(m_pDialog));
    gchar* pSelectedFolder = g_file_get_uri(pPath);
    g_object_unref(pPath);
#else
    gchar* pSelectedFolder =
        gtk_file_chooser_get_uri( GTK_FILE_CHOOSER( m_pDialog ) );
#endif
    OUString aSelectedFolderName = uritounicode(pSelectedFolder);
    g_free( pSelectedFolder );

    return aSelectedFolderName;
}

void SAL_CALL SalGtkFolderPicker::setDescription( const OUString& /*rDescription*/ )
{
}

// XExecutableDialog functions

void SAL_CALL SalGtkFolderPicker::setTitle( const OUString& aTitle )
{
    SolarMutexGuard g;

    assert( m_pDialog != nullptr );

    OString aWindowTitle = OUStringToOString( aTitle, RTL_TEXTENCODING_UTF8 );

    gtk_window_set_title( GTK_WINDOW( m_pDialog ), aWindowTitle.getStr() );
}

sal_Int16 SAL_CALL SalGtkFolderPicker::execute()
{
    SolarMutexGuard g;

    assert( m_pDialog != nullptr );

    sal_Int16 retVal = 0;

    uno::Reference< awt::XExtendedToolkit > xToolkit =
        awt::Toolkit::create(m_xContext);

    GtkWindow *pParent = GTK_WINDOW(m_pParentWidget);
    if (!pParent)
    {
        SAL_WARN( "vcl.gtk", "no parent widget set");
        pParent = RunDialog::GetTransientFor();
    }
    if (pParent)
        gtk_window_set_transient_for(GTK_WINDOW(m_pDialog), pParent);
    rtl::Reference<RunDialog> pRunDialog = new RunDialog(m_pDialog, xToolkit, frame::Desktop::create(m_xContext));
    gint nStatus = pRunDialog->run();
    switch( nStatus )
    {
        case GTK_RESPONSE_ACCEPT:
            retVal = ExecutableDialogResults::OK;
            break;
        case GTK_RESPONSE_CANCEL:
            retVal = ExecutableDialogResults::CANCEL;
            break;
        default:
            retVal = 0;
            break;
    }
    gtk_widget_set_visible(m_pDialog, false);

    return retVal;
}

// XInitialization

void SAL_CALL SalGtkFolderPicker::initialize(const uno::Sequence<uno::Any>& aArguments)
{
    m_pParentWidget = GetParentWidget(aArguments);
}

// XCancellable

void SAL_CALL SalGtkFolderPicker::cancel()
{
    SolarMutexGuard g;

    assert( m_pDialog != nullptr );

    // TODO m_pImpl->cancel();
}

uno::Reference< ui::dialogs::XFolderPicker2 >
GtkInstance::createFolderPicker( const uno::Reference< uno::XComponentContext > &xMSF )
{
    return uno::Reference< ui::dialogs::XFolderPicker2 >(
                new SalGtkFolderPicker( xMSF ) );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
