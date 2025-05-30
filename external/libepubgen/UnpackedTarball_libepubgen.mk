# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

epubgen_patches :=
epubgen_patches += tdf-120491.patch

$(eval $(call gb_UnpackedTarball_UnpackedTarball,libepubgen))

$(eval $(call gb_UnpackedTarball_set_tarball,libepubgen,$(EPUBGEN_TARBALL)))

$(eval $(call gb_UnpackedTarball_update_autoconf_configs,libepubgen))

$(eval $(call gb_UnpackedTarball_set_patchlevel,libepubgen,0))

$(eval $(call gb_UnpackedTarball_add_patches,libepubgen,\
	$(foreach patch,$(epubgen_patches),external/libepubgen/$(patch)) \
))

# vim: set noet sw=4 ts=4:
