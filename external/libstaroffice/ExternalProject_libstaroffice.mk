# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,libstaroffice))

$(eval $(call gb_ExternalProject_use_autoconf,libstaroffice,build))

$(eval $(call gb_ExternalProject_register_targets,libstaroffice,\
	build \
))

$(eval $(call gb_ExternalProject_use_externals,libstaroffice,\
	revenge \
))

$(call gb_ExternalProject_get_state_target,libstaroffice,build) :
	$(call gb_Trace_StartRange,libstaroffice,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		export PKG_CONFIG="" \
		&& $(gb_RUN_CONFIGURE) ./configure \
			--with-pic \
			$(if $(DISABLE_DYNLOADING), \
				--enable-static --disable-shared \
			, \
				--enable-shared --disable-static \
			) \
			--with-sharedptr=c++11 \
			--without-docs \
			--disable-tools \
			--disable-zip \
			$(if $(ENABLE_DEBUG),--enable-debug,--disable-debug) \
			$(if $(verbose),--disable-silent-rules,--enable-silent-rules) \
			--disable-werror \
			CXXFLAGS="$(gb_CXXFLAGS) $(call gb_ExternalProject_get_build_flags,libstaroffice)" \
			$(if $(filter LINUX,$(OS)),$(if $(SYSTEM_REVENGE),, \
				'LDFLAGS=-Wl$(COMMA)-z$(COMMA)origin \
					-Wl$(COMMA)-rpath$(COMMA)\$$$$ORIGIN')) \
			$(gb_CONFIGURE_PLATFORMS) \
			$(if $(filter MACOSX,$(OS)),--prefix=/@.__________________________________________________OOO) \
		&& $(MAKE) \
		$(if $(filter MACOSX,$(OS)),\
			&& $(PERL) $(SRCDIR)/solenv/bin/macosx-change-install-names.pl shl OOO \
				$(EXTERNAL_WORKDIR)/src/lib/.libs/libstaroffice-0.0.0.dylib \
		) \
	)
	$(call gb_Trace_EndRange,libstaroffice,EXTERNAL)

# vim: set noet sw=4 ts=4:
