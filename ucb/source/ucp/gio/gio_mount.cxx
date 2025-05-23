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

#include <utility>

#include "gio_mount.hxx"
#include <ucbhelper/simpleauthenticationrequest.hxx>
#include <string.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#if defined __clang__
#if __has_warning("-Wdeprecated-volatile")
#pragma clang diagnostic ignored "-Wdeprecated-volatile"
#endif
#endif
#endif
G_DEFINE_TYPE (OOoMountOperation, ooo_mount_operation, G_TYPE_MOUNT_OPERATION);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static void ooo_mount_operation_ask_password (GMountOperation   *op,
    const char *message, const char *default_user, const char *default_domain,
    GAskPasswordFlags flags);

static void ooo_mount_operation_init (OOoMountOperation *op)
{
    op->m_aPrevPassword.clear();
    op->m_aPrevUsername.clear();
}

void ooo_mount_operation_finalize (GObject *object)
{
    OOoMountOperation *mount_op = OOO_MOUNT_OPERATION (object);

    mount_op->~OOoMountOperation();

    G_OBJECT_CLASS (ooo_mount_operation_parent_class)->finalize (object);
}

static void ooo_mount_operation_class_init (OOoMountOperationClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = ooo_mount_operation_finalize;

    GMountOperationClass *mount_op_class = G_MOUNT_OPERATION_CLASS (klass);
    mount_op_class->ask_password = ooo_mount_operation_ask_password;
}

namespace {

// Temporarily undo the g_main_context_push_thread_default done in the surrounding MountOperation
// ctor (in ucb/source/ucp/gio/gio_content.cxx):
struct GlibThreadDefaultMainContextScope {
public:
    GlibThreadDefaultMainContextScope(GMainContext * context): context_(context)
    { g_main_context_push_thread_default(context_); }

    ~GlibThreadDefaultMainContextScope() { g_main_context_pop_thread_default(context_); }

private:
    GMainContext * context_;
};

}

static void ooo_mount_operation_ask_password (GMountOperation *op,
    const char * /*message*/, const char *default_user,
    const char *default_domain, GAskPasswordFlags flags)
{
    css::uno::Reference< css::task::XInteractionHandler > xIH;

    OOoMountOperation *pThis = reinterpret_cast<OOoMountOperation*>(op);
    GlibThreadDefaultMainContextScope scope(pThis->context.get());

    const css::uno::Reference< css::ucb::XCommandEnvironment > &xEnv = pThis->xEnv;

    if (xEnv.is())
      xIH = xEnv->getInteractionHandler();

    if (!xIH.is())
    {
        g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);
        return;
    }

    OUString aDomain, aUserName, aPassword;

    if (default_user)
        aUserName = OUString(default_user, strlen(default_user), RTL_TEXTENCODING_UTF8);

    ucbhelper::SimpleAuthenticationRequest::EntityType eUserName =
        (flags & G_ASK_PASSWORD_NEED_USERNAME)
          ? ucbhelper::SimpleAuthenticationRequest::ENTITY_MODIFY
          : aUserName.isEmpty() ? ucbhelper::SimpleAuthenticationRequest::ENTITY_NA
                                : ucbhelper::SimpleAuthenticationRequest::ENTITY_FIXED;

    ucbhelper::SimpleAuthenticationRequest::EntityType ePassword =
        (flags & G_ASK_PASSWORD_NEED_PASSWORD)
          ? ucbhelper::SimpleAuthenticationRequest::ENTITY_MODIFY
          : ucbhelper::SimpleAuthenticationRequest::ENTITY_NA;

    OUString aPrevPassword = pThis->m_aPrevUsername;
    OUString aPrevUsername = pThis->m_aPrevPassword;

    //The damn dialog is stupidly broken, so do like webdav, i.e. "#102871#"
    if ( aUserName.isEmpty() )
        aUserName = aPrevUsername;

    if ( aPassword.isEmpty() )
        aPassword = aPrevPassword;

    ucbhelper::SimpleAuthenticationRequest::EntityType eDomain =
        (flags & G_ASK_PASSWORD_NEED_DOMAIN)
          ? ucbhelper::SimpleAuthenticationRequest::ENTITY_MODIFY
          : ucbhelper::SimpleAuthenticationRequest::ENTITY_NA;

    if (default_domain)
        aDomain = OUString(default_domain, strlen(default_domain), RTL_TEXTENCODING_UTF8);

    rtl::Reference< ucbhelper::SimpleAuthenticationRequest > xRequest
        = new ucbhelper::SimpleAuthenticationRequest (OUString() /* FIXME: provide URL here */, OUString(), eDomain, aDomain, eUserName, aUserName, ePassword, aPassword);

    xIH->handle( xRequest );

    rtl::Reference< ucbhelper::InteractionContinuation > xSelection = xRequest->getSelection();

    if ( !xSelection.is() )
    {
        g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);
        return;
    }

    css::uno::Reference< css::task::XInteractionAbort > xAbort(xSelection->getXWeak(), css::uno::UNO_QUERY );
    if ( xAbort.is() )
    {
        g_mount_operation_reply (op, G_MOUNT_OPERATION_ABORTED);
        return;
    }

    const rtl::Reference< ucbhelper::InteractionSupplyAuthentication > & xSupp = xRequest->getAuthenticationSupplier();
    aUserName = xSupp->getUserName();
    aPassword = xSupp->getPassword();

    if (flags & G_ASK_PASSWORD_NEED_USERNAME)
        g_mount_operation_set_username(op, OUStringToOString(aUserName, RTL_TEXTENCODING_UTF8).getStr());

    if (flags & G_ASK_PASSWORD_NEED_PASSWORD)
        g_mount_operation_set_password(op, OUStringToOString(aPassword, RTL_TEXTENCODING_UTF8).getStr());

    if (flags & G_ASK_PASSWORD_NEED_DOMAIN)
        g_mount_operation_set_domain(op, OUStringToOString(xSupp->getRealm(), RTL_TEXTENCODING_UTF8).getStr());

    switch (xSupp->getRememberPasswordMode())
    {
    default:
        case css::ucb::RememberAuthentication_NO:
            g_mount_operation_set_password_save(op, G_PASSWORD_SAVE_NEVER);
            break;
        case css::ucb::RememberAuthentication_SESSION:
            g_mount_operation_set_password_save(op, G_PASSWORD_SAVE_FOR_SESSION);
            break;
        case css::ucb::RememberAuthentication_PERSISTENT:
            g_mount_operation_set_password_save(op, G_PASSWORD_SAVE_PERMANENTLY);
            break;
    }

    pThis->m_aPrevPassword = aPassword;
    pThis->m_aPrevUsername = aUserName;
    g_mount_operation_reply (op, G_MOUNT_OPERATION_HANDLED);
}

GMountOperation *ooo_mount_operation_new(ucb::ucp::gio::glib::MainContextRef && context, const css::uno::Reference< css::ucb::XCommandEnvironment >& rEnv)
{
    void* pMem = g_object_new (OOO_TYPE_MOUNT_OPERATION, nullptr);
    OOoMountOperation *pRet = new (pMem) OOoMountOperation;
    pRet->context = std::move(context);
    pRet->xEnv = rEnv;
    return &pRet->parent_instance;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
