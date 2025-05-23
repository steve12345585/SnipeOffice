# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,libpagemaker))

$(eval $(call gb_ExternalProject_use_autoconf,libpagemaker,build))

$(eval $(call gb_ExternalProject_register_targets,libpagemaker,\
	build \
))

$(eval $(call gb_ExternalProject_use_externals,libpagemaker,\
	boost_headers \
	revenge \
))

$(call gb_ExternalProject_get_state_target,libpagemaker,build) :
	$(call gb_Trace_StartRange,libpagemaker,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		export PKG_CONFIG="" \
		&& MAKE=$(MAKE) $(gb_RUN_CONFIGURE) ./configure \
			--with-pic \
			--enable-static \
			--disable-shared \
			--without-docs \
			--disable-tools \
			--disable-debug \
			--disable-werror \
			--disable-weffc \
			$(if $(verbose),--disable-silent-rules,--enable-silent-rules) \
			CXXFLAGS="$(gb_CXXFLAGS) $(call gb_ExternalProject_get_build_flags,libpagemaker)" \
			CPPFLAGS="$(CPPFLAGS) $(BOOST_CPPFLAGS)" \
			LDFLAGS="$(call gb_ExternalProject_get_link_flags,libpagemaker)" \
			$(gb_CONFIGURE_PLATFORMS) \
		&& $(MAKE) \
	)
	$(call gb_Trace_EndRange,libpagemaker,EXTERNAL)

# vim: set noet sw=4 ts=4:
