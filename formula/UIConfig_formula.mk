# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UIConfig_UIConfig,formula))

$(eval $(call gb_UIConfig_add_uifiles,formula,\
	formula/uiconfig/ui/formuladialog \
	formula/uiconfig/ui/functionpage \
	formula/uiconfig/ui/structpage \
	formula/uiconfig/ui/parameter \
))

# vim: set noet sw=4 ts=4:
