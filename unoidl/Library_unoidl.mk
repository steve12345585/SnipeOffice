# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,unoidl))

$(eval $(call gb_Library_add_defs,unoidl,-DLO_DLLIMPLEMENTATION_UNOIDL))

$(eval $(call gb_Library_add_exception_objects,unoidl, \
    unoidl/source/sourcefileprovider \
    unoidl/source/sourcetreeprovider \
    unoidl/source/unoidl \
    unoidl/source/unoidlprovider \
))

# drop legacy provider for fuzzing
ifeq (,$(filter FUZZERS,$(BUILD_TYPE)))
$(eval $(call gb_Library_add_exception_objects,unoidl, \
    unoidl/source/legacyprovider \
))
endif

$(eval $(call gb_Library_add_grammars,unoidl, \
    unoidl/source/sourceprovider-parser \
))

$(eval $(call gb_Library_add_scanners,unoidl, \
    unoidl/source/sourceprovider-scanner \
))

$(eval $(call gb_Library_set_include,unoidl, \
    $$(INCLUDE) \
    -I$(SRCDIR)/unoidl/source \
))

$(eval $(call gb_Library_set_is_ure_library_or_dependency,unoidl))

$(eval $(call gb_Library_use_libraries,unoidl, \
    reg \
    sal \
    salhelper \
))

# vim: set noet sw=4 ts=4:
