# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UIConfig_UIConfig,modules/dbtdata))

$(eval $(call gb_UIConfig_add_menubarfiles,modules/dbtdata,\
	dbaccess/uiconfig/dbtdata/menubar/menubar \
))

$(eval $(call gb_UIConfig_add_popupmenufiles,modules/dbtdata,\
	dbaccess/uiconfig/dbtdata/popupmenu/refreshdata \
))

$(eval $(call gb_UIConfig_add_toolbarfiles,modules/dbtdata,\
	dbaccess/uiconfig/dbtdata/toolbar/toolbar \
))

# vim: set noet sw=4 ts=4:
