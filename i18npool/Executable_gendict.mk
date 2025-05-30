# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,gendict))

$(eval $(call gb_Executable_use_libraries,gendict,\
	sal \
))

ifeq ($(gb_Side),build)
ifneq ($(shell grep -e OS=iOS -e OS=ANDROID $(BUILDDIR)/config_host.mk),)
$(eval $(call gb_Executable_add_cxxflags,gendict,\
	-DDICT_JA_ZH_IN_DATAFILE \
))
endif
endif

$(eval $(call gb_Executable_add_exception_objects,gendict,\
	i18npool/source/breakiterator/gendict \
))

# vim: set noet sw=4 ts=4:
