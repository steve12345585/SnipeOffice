# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,liblangtag))

$(eval $(call gb_Module_add_targets,liblangtag,\
	UnpackedTarball_liblangtag \
	ExternalPackage_liblangtag_data \
	ExternalProject_liblangtag \
))


ifneq ($(COM),MSC)
$(eval $(call gb_Module_add_targets,liblangtag,\
	ExternalPackage_liblangtag \
))
endif

# vim: set noet sw=4 ts=4:
