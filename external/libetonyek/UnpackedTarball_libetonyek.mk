# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,libetonyek))

$(eval $(call gb_UnpackedTarball_set_tarball,libetonyek,$(ETONYEK_TARBALL)))

$(eval $(call gb_UnpackedTarball_set_patchlevel,libetonyek,0))

$(eval $(call gb_UnpackedTarball_update_autoconf_configs,libetonyek))

$(eval $(call gb_UnpackedTarball_add_patches,libetonyek,\
	external/libetonyek/win_build.patch.1 \
	external/libetonyek/ubsan.patch \
	external/libetonyek/rpath.patch \
	external/libetonyek/enumarith.patch \
	external/libetonyek/mdds3.0.patch.1 \
))

ifneq ($(OS),MACOSX)
ifneq ($(OS),WNT)
ifneq ($(OS),iOS)
$(eval $(call gb_UnpackedTarball_add_patches,libetonyek,\
	external/libetonyek/libetonyek-bundled-soname.patch.0 \
))
endif
endif
endif

# vim: set noet sw=4 ts=4:
