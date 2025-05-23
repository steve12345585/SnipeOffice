# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,rasqal))

$(eval $(call gb_ExternalProject_use_external,rasqal,libxml2))

$(eval $(call gb_ExternalProject_use_package,rasqal,raptor))

$(eval $(call gb_ExternalProject_register_targets,rasqal,\
	build \
))

# note: this can intentionally only build against internal raptor (not system)

$(call gb_ExternalProject_get_state_target,rasqal,build):
	$(call gb_Trace_StartRange,rasqal,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		$(if $(filter -fsanitize=undefined,$(CC)),CC='$(CC) -fno-sanitize=function') \
		CFLAGS="$(CFLAGS) $(if $(filter TRUE,$(DISABLE_DYNLOADING)),-fvisibility=hidden) $(call gb_ExternalProject_get_build_flags,rasqal) $(gb_EMSCRIPTEN_CPPFLAGS)" \
		LDFLAGS=" \
			$(if $(filter LINUX FREEBSD,$(OS)),-Wl$(COMMA)-z$(COMMA)origin -Wl$(COMMA)-rpath$(COMMA)\\"\$$\$$ORIGIN") \
			$(if $(SYSBASE),$(if $(filter LINUX SOLARIS,$(OS)),-L$(SYSBASE)/lib -L$(SYSBASE)/usr/lib -lpthread -ldl))" \
		$(if $(SYSBASE),CPPFLAGS="-I$(SYSBASE)/usr/include") \
		PKG_CONFIG="" \
		RAPTOR2_CFLAGS="-I$(gb_UnpackedTarball_workdir)/raptor/src" \
		RAPTOR2_LIBS="-L$(gb_UnpackedTarball_workdir)/raptor/src/.libs -lraptor2" \
		$(gb_RUN_CONFIGURE) ./configure --disable-gtk-doc \
			--with-regex-library=posix \
			--with-decimal=none \
			--with-uuid-library=internal \
			--with-digest-library=internal \
			$(gb_CONFIGURE_PLATFORMS) \
			$(if $(CROSS_COMPILING),$(if $(filter INTEL ARM,$(CPUNAME)),ac_cv_c_bigendian=no)) \
			$(if $(filter MACOSX,$(OS)),--prefix=/@.__________________________________________________OOO) \
			$(if $(DISABLE_DYNLOADING), \
				--enable-static --disable-shared \
			, \
				--enable-shared --disable-static \
			) \
			$(if $(SYSTEM_LIBXML),,--with-xml2-config=$(gb_UnpackedTarball_workdir)/libxml2/xml2-config) \
		&& $(MAKE) \
		$(if $(filter MACOSX,$(OS)),&& $(PERL) \
			$(SRCDIR)/solenv/bin/macosx-change-install-names.pl shl OOO \
			$(EXTERNAL_WORKDIR)/src/.libs/librasqal-lo.$(RASQAL_MAJOR).dylib) \
	)
	$(call gb_Trace_EndRange,rasqal,EXTERNAL)

# vim: set noet sw=4 ts=4:
