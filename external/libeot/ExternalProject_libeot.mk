# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,libeot))

$(eval $(call gb_ExternalProject_register_targets,libeot,\
	build \
))

$(call gb_ExternalProject_get_state_target,libeot,build) :
	$(call gb_Trace_StartRange,libeot,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		touch Makefile.in \
		&& export PKG_CONFIG="" \
		&& $(gb_RUN_CONFIGURE) ./configure \
			--with-pic \
			--enable-static \
			--disable-shared \
			--disable-debug \
		&& $(MAKE) $(if $(verbose),V=1) \
	)
	$(call gb_Trace_EndRange,libeot,EXTERNAL)

# vim: set noet sw=4 ts=4:
