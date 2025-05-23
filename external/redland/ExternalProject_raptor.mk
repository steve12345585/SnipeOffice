# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,raptor))

$(eval $(call gb_ExternalProject_use_externals,raptor,\
	icu \
	libxml2 \
))

ifeq ($(SYSTEM_ICU),)
$(eval $(call gb_ExternalProject_use_package,raptor,icu_ure))
endif

$(eval $(call gb_ExternalProject_register_targets,raptor,\
	build \
))

$(call gb_ExternalProject_get_state_target,raptor,build):
	$(call gb_Trace_StartRange,raptor,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		$(if $(filter iOS,$(OS)),LIBS="-liconv") \
		$(if $(filter -fsanitize=undefined,$(CC)),CC='$(CC) -fno-sanitize=function') \
		CFLAGS="$(CFLAGS) \
			$(call gb_ExternalProject_get_build_flags,raptor) \
			$(if $(filter TRUE,$(DISABLE_DYNLOADING)),-fvisibility=hidden) \
			$(if $(filter GCCLINUXPOWERPC64,$(COM)$(OS)$(CPUNAME)),-mminimal-toc)" \
		LDFLAGS='$(strip \
		    $(if $(filter LINUX FREEBSD,$(OS)),$(strip -Wl,-z,origin -Wl,-rpath,\$$$$ORIGIN -Wl,-rpath-link,$(INSTROOT)/$(LIBO_URE_LIB_FOLDER))) \
		    $(if $(SYSBASE),$(if $(filter LINUX SOLARIS,$(OS)),-L$(SYSBASE)/lib -L$(SYSBASE)/usr/lib -lpthread -ldl)))' \
		CPPFLAGS="$(if $(SYSBASE),-I$(SYSBASE)/usr/include) $(gb_EMSCRIPTEN_CPPFLAGS)" \
		ICU_LIBS='$(if $(filter-out MACOSX,$(OS)),$(ICU_LIBS))' \
		$(gb_RUN_CONFIGURE) ./configure --disable-gtk-doc \
			--enable-parsers="rdfxml" \
			--without-www \
			--without-xslt-config \
			$(gb_CONFIGURE_PLATFORMS) \
			$(if $(CROSS_COMPILING),$(if $(filter INTEL ARM,$(CPUNAME)),ac_cv_c_bigendian=no)) \
			$(if $(filter MACOSX,$(OS)),--prefix=/@.__________________________________________________OOO) \
			$(if $(DISABLE_DYNLOADING), \
				--enable-static --disable-shared \
			, \
				--enable-shared --disable-static \
			) \
			$(if $(SYSTEM_LIBXML),$(if $(filter-out MACOSX,$(OS)),--without-xml2-config),--with-xml2-config=$(gb_UnpackedTarball_workdir)/libxml2/xml2-config) \
		&& $(MAKE) \
	)
	$(call gb_Trace_EndRange,raptor,EXTERNAL)

# vim: set noet sw=4 ts=4:
