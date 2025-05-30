/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
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

#include <cassert>

#include <com/sun/star/uno/Exception.hpp>
#include <comphelper/configuration.hxx>
#include <config_folders.h>
#include <officecfg/Setup.hxx>
#include <osl/file.h>
#include <osl/file.hxx>
#if defined ANDROID || defined IOS || defined EMSCRIPTEN
#include <rtl/bootstrap.hxx>
#endif
#include <rtl/ustring.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <unotools/bootstrap.hxx>

#include "userinstall.hxx"

namespace desktop::userinstall {

namespace {

#if !(defined ANDROID || defined IOS || defined EMSCRIPTEN)
osl::FileBase::RC copyRecursive(
    OUString const & srcUri, OUString const & dstUri)
{
    osl::DirectoryItem item;
    osl::FileBase::RC e = osl::DirectoryItem::get(srcUri, item);
    if (e != osl::FileBase::E_None) {
        return e;
    }
    osl::FileStatus stat1(osl_FileStatus_Mask_Type);
    e = item.getFileStatus(stat1);
    if (e != osl::FileBase::E_None) {
        return e;
    }
    if (stat1.getFileType() == osl::FileStatus::Directory) {
        e = osl::Directory::create(dstUri);
        if (e != osl::FileBase::E_None && e != osl::FileBase::E_EXIST) {
            return e;
        }
        osl::Directory dir(srcUri);
        e = dir.open();
        if (e != osl::FileBase::E_None) {
            return e;
        }
        for (;;) {
            e = dir.getNextItem(item);
            if (e == osl::FileBase::E_NOENT) {
                break;
            }
            if (e != osl::FileBase::E_None) {
                return e;
            }
            osl::FileStatus stat2(
                osl_FileStatus_Mask_FileName | osl_FileStatus_Mask_FileURL);
            e = item.getFileStatus(stat2);
            if (e != osl::FileBase::E_None) {
                return e;
            }
            assert(!dstUri.endsWith("/"));
            e = copyRecursive(
                stat2.getFileURL(), dstUri + "/" + stat2.getFileName());
                // assumes that all files under presets/ have names that can be
                // copied unencoded into file URLs
            if (e != osl::FileBase::E_None) {
                return e;
            }
        }
        e = dir.close();
    } else {
        e = osl::File::copy(srcUri, dstUri);
        if (e == osl::FileBase::E_EXIST) {
            // Assume an earlier attempt failed half-way through:
            e = osl::FileBase::E_None;
        }
    }
    return e;
}
#endif

Status create(OUString const & uri) {
    osl::FileBase::RC e = osl::Directory::createPath(uri);
    if (e != osl::FileBase::E_None && e != osl::FileBase::E_EXIST) {
        return ERROR_OTHER;
    }
#if !(defined ANDROID || defined IOS || defined EMSCRIPTEN)
#if defined UNIX
    // Set safer permissions for the user directory by default:
    osl::File::setAttributes(
        uri,
        (osl_File_Attribute_OwnWrite | osl_File_Attribute_OwnRead
         | osl_File_Attribute_OwnExe));
#endif
    // As of now osl_copyFile does not work on Android => don't do this:
    OUString baseUri;
    if (utl::Bootstrap::locateBaseInstallation(baseUri)
        != utl::Bootstrap::PATH_EXISTS)
    {
        return ERROR_OTHER;
    }
    switch (copyRecursive(
                baseUri + "/" LIBO_SHARE_PRESETS_FOLDER, uri + "/user"))
    {
    case osl::FileBase::E_None:
        break;
    case osl::FileBase::E_ACCES:
        return ERROR_CANT_WRITE;
    case osl::FileBase::E_NOSPC:
        return ERROR_NO_SPACE;
    default:
        return ERROR_OTHER;
    }
#else
    // On (Android and) iOS, just create the user directory. Later code fails mysteriously if it
    // doesn't exist.
    OUString userDir("${$BRAND_BASE_DIR/" LIBO_ETC_FOLDER "/" SAL_CONFIGFILE("bootstrap") ":UserInstallation}/user");
    rtl::Bootstrap::expandMacros(userDir);
    osl::Directory::createPath(userDir);
#endif
    std::shared_ptr<comphelper::ConfigurationChanges> batch(
        comphelper::ConfigurationChanges::create());
    officecfg::Setup::Office::ooSetupInstCompleted::set(true, batch);
    batch->commit();
    return CREATED;
}

bool isCreated() {
    try {
        return officecfg::Setup::Office::ooSetupInstCompleted::get();
    } catch (const css::uno::Exception &) {
        TOOLS_WARN_EXCEPTION("desktop.app", "ignoring");
        return false;
    }
}

}

Status finalize() {
    OUString uri;
    switch (utl::Bootstrap::locateUserInstallation(uri)) {
    case utl::Bootstrap::PATH_EXISTS:
        if (isCreated()) {
            return EXISTED;
        }
        [[fallthrough]];
    case utl::Bootstrap::PATH_VALID:
        return create(uri);
    default:
        return ERROR_OTHER;
    }
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
