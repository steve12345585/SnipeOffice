# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,epm))

$(eval $(call gb_ExternalProject_register_targets,epm,\
	build \
))

$(call gb_ExternalProject_get_state_target,epm,build) :
	$(call gb_Trace_StartRange,epm,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		$(gb_RUN_CONFIGURE) ./configure --disable-fltk \
			$(if $(filter MACOSX,$(OS)),--prefix=/@.__________________________________________________NONE) \
		&& $(MAKE) \
		&& touch $@ \
	)
	$(call gb_Trace_EndRange,epm,EXTERNAL)

# vim: set noet sw=4 ts=4:
