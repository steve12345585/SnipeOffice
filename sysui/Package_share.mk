# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,sysui_share,$(share_WORKDIR)/libreoffice))

$(eval $(call gb_Package_add_files,sysui_share,share/xdg,\
	$(addsuffix .desktop,$(LAUNCHERLIST)) \
))

$(eval $(call gb_Package_use_custom_target,sysui_share,sysui/share))

# vim: set noet sw=4 ts=4:
