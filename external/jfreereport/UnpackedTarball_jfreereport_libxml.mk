# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,jfreereport_libxml))

$(eval $(call gb_UnpackedTarball_set_tarball,jfreereport_libxml,$(JFREEREPORT_LIBXML_TARBALL),0))

$(eval $(call gb_UnpackedTarball_set_patchlevel,jfreereport_libxml,2))

$(eval $(call gb_UnpackedTarball_fix_end_of_line,jfreereport_libxml,\
	common_build.xml \
))

$(eval $(call gb_UnpackedTarball_add_patches,jfreereport_libxml,\
	external/jfreereport/patches/common_build.patch \
	external/jfreereport/patches/libxml-1.1.3-remove-commons-logging.patch.1 \
))

# vim: set noet sw=4 ts=4:
