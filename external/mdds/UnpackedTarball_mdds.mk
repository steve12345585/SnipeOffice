# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,mdds))

$(eval $(call gb_UnpackedTarball_set_tarball,mdds,$(MDDS_TARBALL)))

$(eval $(call gb_UnpackedTarball_set_patchlevel,mdds,0))

$(eval $(call gb_UnpackedTarball_add_patches,mdds,\
    external/mdds/gcc-12-silence-use-after-free.patch.1 \
))

# vim: set noet sw=4 ts=4:
