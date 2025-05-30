# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,regview))

$(eval $(call gb_Executable_set_include,regview, \
    $$(INCLUDE) \
    -I$(SRCDIR)/registry/inc \
))

$(eval $(call gb_Executable_use_libraries,regview,\
    reg \
    $(if $(filter TRUE,$(DISABLE_DYNLOADING)),store) \
    $(if $(filter TRUE,$(DISABLE_DYNLOADING)),salhelper) \
    sal \
))

$(eval $(call gb_Executable_use_static_libraries,regview,\
    registry_helper \
))

$(eval $(call gb_Executable_add_exception_objects,regview,\
    registry/tools/regview \
))

$(eval $(call gb_Executable_add_default_nativeres,regview))

# vim:set noet sw=4 ts=4:
