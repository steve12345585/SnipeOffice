# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,fontconfig))

$(eval $(call gb_UnpackedTarball_set_tarball,fontconfig,$(FONTCONFIG_TARBALL),,fontconfig))

$(eval $(call gb_UnpackedTarball_set_patchlevel,fontconfig,0))

$(eval $(call gb_UnpackedTarball_add_patches,fontconfig,\
	external/fontconfig/fontconfig-2.12.1.patch.1 \
	external/fontconfig/libfontconfig-bundled-soname.patch.0 \
))

# vim: set noet sw=4 ts=4:
