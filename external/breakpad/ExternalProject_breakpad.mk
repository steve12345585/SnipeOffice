# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,breakpad))

$(eval $(call gb_ExternalProject_register_targets,breakpad,\
	build \
))


ifeq ($(COM),MSC)

$(call gb_ExternalProject_get_state_target,breakpad,build) :
	$(call gb_Trace_StartRange,breakpad,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		MSBuild.exe src/tools/windows/dump_syms/dump_syms.sln -p:Configuration=Release \
			/p:PlatformToolset=$(VCTOOLSET) /p:VisualStudioVersion=$(VCVER) /ToolsVersion:Current \
	)
	$(call gb_Trace_EndRange,breakpad,EXTERNAL)

else # !ifeq($(COM),MSC)

$(call gb_ExternalProject_get_state_target,breakpad,build) :
	$(call gb_Trace_StartRange,breakpad,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		$(gb_RUN_CONFIGURE) ./configure CXXFLAGS="-O2 $(gb_VISIBILITY_FLAGS)" \
		&& $(MAKE) \
	)
	$(call gb_Trace_EndRange,breakpad,EXTERNAL)

endif

# vim: set noet sw=4 ts=4:
