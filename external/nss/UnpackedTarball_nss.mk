# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,nss))

$(eval $(call gb_UnpackedTarball_set_tarball,nss,$(NSS_TARBALL)))

$(eval $(call gb_UnpackedTarball_update_autoconf_configs,nss,nspr/build/autoconf))

$(eval $(call gb_UnpackedTarball_set_patchlevel,nss,2))

$(eval $(call gb_UnpackedTarball_add_patches,nss,\
    external/nss/nss.patch \
    external/nss/nss_macosx.patch \
    external/nss/nss-win32-make.patch.1 \
    external/nss/ubsan.patch.0 \
    external/nss/clang-cl.patch.0 \
    external/nss/winnt.patch.0 \
    external/nss/nss.vs2015.patch \
    external/nss/nss.vs2015.pdb.patch \
    external/nss/nss.bzmozilla1238154.patch \
    external/nss/nss-bz1646594.patch.1 \
    external/nss/macos-dlopen.patch.0 \
    external/nss/nss-restore-manual-pre-dependencies.patch.1 \
    external/nss/Wincompatible-function-pointer-types.patch.0 \
    external/nss/nspr-win95-target.patch \
    $(if $(filter LINUX,$(OS)), \
        external/nss/nss.disablefsync.patch \
        external/nss/nss.getrandom.patch) \
    $(if $(filter iOS,$(OS)), \
        external/nss/nss-ios.patch) \
    $(if $(filter ANDROID,$(OS)), \
        external/nss/nss-android.patch.1) \
    $(if $(filter WNT,$(OS)), \
        external/nss/nss.windows.patch \
        external/nss/nss.nowerror.patch \
        external/nss/nss.utf8bom.patch.1) \
))

ifeq ($(COM_IS_CLANG),TRUE)
ifneq ($(filter -fsanitize=%,$(CC)),)
$(eval $(call gb_UnpackedTarball_add_patches,nss,\
    external/nss/asan.patch.1 \
))
endif
endif

# vim: set noet sw=4 ts=4:
