# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,libwpd))

$(eval $(call gb_UnpackedTarball_set_tarball,libwpd,$(WPD_TARBALL)))

$(eval $(call gb_UnpackedTarball_set_patchlevel,libwpd,0))

$(eval $(call gb_UnpackedTarball_update_autoconf_configs,libwpd))

$(eval $(call gb_UnpackedTarball_add_patches,libwpd,\
	external/libwpd/libwpd-vs2013.patch.1 \
	external/libwpd/tdf153034_3_WrongGreekCharactersWP5Import.patch \
	$(if $(SYSTEM_REVENGE),,external/libwpd/rpath.patch) \
	external/libwpd/include.patch \
))

ifneq ($(OS),MACOSX)
ifneq ($(OS),WNT)
ifneq ($(OS),iOS)
$(eval $(call gb_UnpackedTarball_add_patches,libwpd,\
	external/libwpd/libwpd-bundled-soname.patch.0 \
))
endif
endif
endif

# vim: set noet sw=4 ts=4:
