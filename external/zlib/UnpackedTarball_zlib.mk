# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,zlib))

$(eval $(call gb_UnpackedTarball_set_tarball,zlib,$(ZLIB_TARBALL)))

$(eval $(call gb_UnpackedTarball_set_post_action,zlib,\
	mkdir -p x64 && \
	cp $(addsuffix .c,adler32 compress crc32 deflate inffast inflate inftrees trees zutil) x64 \
))

$(eval $(call gb_UnpackedTarball_set_patchlevel,zlib,0))

$(eval $(call gb_UnpackedTarball_add_patches,zlib, \
    external/zlib/missinginclude.patch \
))

# vim: set noet sw=4 ts=4:
