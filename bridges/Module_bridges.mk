# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,bridges))

$(eval $(call gb_Module_add_targets,bridges,\
	Library_cpp_uno \
	$(if $(ENABLE_DOTNET),Library_net_uno) \
	$(if $(ENABLE_JAVA),\
		Jar_java_uno \
		Library_java_uno \
		$(if $(filter MACOSX,$(OS)),Package_jnilib_java_uno) \
	) \
	$(if $(filter ARM,$(CPUNAME)),\
		$(if $(filter ANDROID LINUX,$(OS)),\
			CustomTarget_gcc3_linux_arm) \
	) \
	$(if $(filter EMSCRIPTEN,$(OS)), \
	    CustomTarget_gcc3_wasm \
	    StaticLibrary_emscriptencxxabi \
	) \
))

ifeq (,$(filter build,$(gb_Module_SKIPTARGETS)))
ifeq ($(strip $(bridges_SELECTED_BRIDGE)),)
$(call gb_Output_error,no bridge selected for build: bailing out)
else ifneq ($(words $(bridges_SELECTED_BRIDGE)),1)
$(call gb_Output_error,multiple bridges selected for build: $(bridges_SELECTED_BRIDGE))
endif
endif

# vim: set noet sw=4 ts=4:
