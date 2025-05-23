# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,libxslt))

$(eval $(call gb_UnpackedTarball_set_tarball,libxslt,$(LIBXSLT_TARBALL)))

$(eval $(call gb_UnpackedTarball_update_autoconf_configs,libxslt))

$(eval $(call gb_UnpackedTarball_add_patches,libxslt,\
	external/libxslt/libxslt-config.patch.1 \
	external/libxslt/libxslt-internal-symbols.patch.1 \
	$(if $(gb_Module_CURRENTMODULE_SYMBOLS_ENABLED),\
		external/libxslt/libxslt-msvc-sym.patch.2, \
 		external/libxslt/libxslt-msvc.patch.2) \
	external/libxslt/rpath.patch.0 \
))

# vim: set noet sw=4 ts=4:
