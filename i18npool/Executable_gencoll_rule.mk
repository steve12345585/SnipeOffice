# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,gencoll_rule))

$(eval $(call gb_Executable_set_include,gencoll_rule,\
	$$(INCLUDE) \
	-I$(SRCDIR)/i18npool/inc \
))

$(eval $(call gb_Executable_use_libraries,gencoll_rule,\
	sal \
))

$(eval $(call gb_Executable_use_externals,gencoll_rule,\
	icui18n \
	icuuc \
	$(if $(filter TRUE,$(DISABLE_DYNLOADING)),icudata) \
	icu_headers \
))

$(eval $(call gb_Executable_add_exception_objects,gencoll_rule,\
	i18npool/source/collator/gencoll_rule \
))

# vim: set noet sw=4 ts=4:
