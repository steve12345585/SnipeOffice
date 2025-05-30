# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,libzmf))

$(eval $(call gb_ExternalProject_use_autoconf,libzmf,build))

$(eval $(call gb_ExternalProject_register_targets,libzmf,\
	build \
))

$(eval $(call gb_ExternalProject_use_externals,libzmf,\
	boost_headers \
	icu \
	libpng \
	revenge \
	zlib \
))

$(call gb_ExternalProject_get_state_target,libzmf,build) :
	$(call gb_Trace_StartRange,libzmf,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		export PKG_CONFIG="" \
		&& unset MSYS_NO_PATHCONV && MAKE=$(MAKE) $(gb_RUN_CONFIGURE) ./configure \
			--with-pic \
			--enable-static \
			--disable-shared \
			--without-docs \
			--disable-tests \
			--disable-tools \
			--disable-debug \
			--disable-werror \
			--disable-weffc \
			$(if $(verbose),--disable-silent-rules,--enable-silent-rules) \
			CXXFLAGS="$(gb_CXXFLAGS) $(call gb_ExternalProject_get_build_flags,libzmf)" \
			CPPFLAGS="$(CPPFLAGS) $(BOOST_CPPFLAGS)" \
			LDFLAGS="$(call gb_ExternalProject_get_link_flags,libzmf)" \
			$(gb_CONFIGURE_PLATFORMS) \
		&& $(MAKE) \
	)
	$(call gb_Trace_EndRange,libzmf,EXTERNAL)

# vim: set noet sw=4 ts=4:
