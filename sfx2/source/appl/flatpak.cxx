/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <osl/file.hxx>
#include <osl/thread.h>
#include <rtl/textcvt.h>
#include <rtl/ustring.h>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <sfx2/flatpak.hxx>
#include <tools/debug.hxx>
#include <unotools/tempfile.hxx>
#include <unotools/ucbhelper.hxx>

bool flatpak::isFlatpak() {
    static auto const flatpak = [] { return std::getenv("LIBO_FLATPAK") != nullptr; }();
    return flatpak;
}

namespace {

// Must only be accessed with SolarMutex locked:
struct {
    bool created = false;
    OUString url;
} temporaryHtmlDirectoryStatus;

}

bool flatpak::createTemporaryHtmlDirectory(OUString ** url) {
    assert(url != nullptr);
    DBG_TESTSOLARMUTEX();
    if (!temporaryHtmlDirectoryStatus.created) {
        // coverity[tainted_return_value] - we trust the contents of this variable
        auto const env = std::getenv("XDG_CACHE_HOME");
        if (env == nullptr) {
            SAL_WARN("sfx.appl", "LIBO_FLATPAK mode but unset XDG_CACHE_HOME");
            return false;
        }
        OUString path;
        if (!rtl_convertStringToUString(
                &path.pData, env, std::strlen(env), osl_getThreadTextEncoding(),
                (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
                 | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR)))
        {
            SAL_WARN(
                "sfx.appl",
                "LIBO_FLATPAK mode failure converting XDG_CACHE_HOME \"" << env << "\" encoding");
            return false;
        }
        OUString parent;
        auto const err = osl::FileBase::getFileURLFromSystemPath(path, parent);
        if (err != osl::FileBase::E_None) {
            SAL_WARN(
                "sfx.appl",
                "LIBO_FLATPAK mode failure converting XDG_CACHE_HOME \"" << path << "\" to URL: "
                    << err);
            return false;
        }
        if (!parent.endsWith("/")) {
            parent += "/";
        }
        temporaryHtmlDirectoryStatus.url = utl::CreateTempURL(&parent, true);
        if (temporaryHtmlDirectoryStatus.url.isEmpty()) {
            SAL_WARN(
                "sfx.appl", "LIBO_FLATPAK mode failure creating temp dir at <" << parent << ">");
            return false;
        }
        temporaryHtmlDirectoryStatus.created = true;
    }
    *url = &temporaryHtmlDirectoryStatus.url;
    return true;
}

void flatpak::removeTemporaryHtmlDirectory() {
    DBG_TESTSOLARMUTEX();
    if (temporaryHtmlDirectoryStatus.created) {
        if (!utl::UCBContentHelper::Kill(temporaryHtmlDirectoryStatus.url)) {
            SAL_INFO(
                "sfx.appl",
                "LIBO_FLATPAK mode failure removing directory <"
                    << temporaryHtmlDirectoryStatus.url << ">");
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
