# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

include $(SRCDIR)/vcl/commonfuzzer.mk

$(eval $(call gb_Executable_Executable,diffuzzer))

$(eval $(call gb_Executable_use_api,diffuzzer,\
    offapi \
    udkapi \
))

$(eval $(call gb_Executable_use_externals,diffuzzer,\
	$(fuzzer_externals) \
))

$(eval $(call gb_Executable_set_include,diffuzzer,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl/inc \
))

$(eval $(call gb_Executable_use_libraries,diffuzzer,\
    $(fuzzer_calc_libraries) \
    $(fuzzer_core_libraries) \
))

$(eval $(call gb_Executable_use_static_libraries,diffuzzer,\
    $(fuzzer_statics) \
))

$(eval $(call gb_Executable_add_exception_objects,diffuzzer,\
	vcl/workben/diffuzzer \
))

$(eval $(call gb_Executable_add_libs,diffuzzer,\
	$(LIB_FUZZING_ENGINE) \
))

# vim: set noet sw=4 ts=4:
