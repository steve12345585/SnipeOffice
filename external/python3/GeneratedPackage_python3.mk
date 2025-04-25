# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_GeneratedPackage_GeneratedPackage,python3,$(gb_UnpackedTarball_workdir)/python3/python-inst/@__________________________________________________OOO))

$(eval $(call gb_GeneratedPackage_use_unpacked,python3,python3))

$(eval $(call gb_GeneratedPackage_use_external_project,python3,python3))

$(eval $(call gb_GeneratedPackage_add_dir,python3,$(INSTROOT)/Frameworks/LibreOfficePython.framework,LibreOfficePython.framework))

# vim: set noet sw=4 ts=4:
