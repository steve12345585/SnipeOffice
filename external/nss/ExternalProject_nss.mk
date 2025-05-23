# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,nss))

# nss build calls configure for nspr itself - if for some reason the configure step should be split out,
# make sure to create config.status (aka run configure) in dir specified with OBJDIR_NAME (nspr/out)
$(eval $(call gb_ExternalProject_register_targets,nss,\
	build \
))

$(eval $(call gb_ExternalProject_use_externals,nss,\
	zlib \
))

ifeq ($(OS),WNT)
$(call gb_ExternalProject_get_state_target,nss,build): \
		$(call gb_ExternalExecutable_get_dependencies,python) \
		$(SRCDIR)/external/nss/nsinstall.py
	$(call gb_Trace_StartRange,nss,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		$(if $(MSVC_USE_DEBUG_RUNTIME),USE_DEBUG_RTL=1,BUILD_OPT=1) \
		$(if $(gb_Module_CURRENTMODULE_SYMBOLS_ENABLED), \
			MOZ_DEBUG_SYMBOLS=1 \
			MOZ_DEBUG_FLAGS=" " \
			OPT_CODE_SIZE=0) \
		OS_TARGET=WINNT \
		USE_SYSTEM_ZLIB=1 \
		$(if $(filter X86_64,$(CPUNAME)),USE_64=1) \
		$(if $(filter AARCH64,$(CPUNAME)),USE_64=1 CPU_ARCH=aarch64) \
		LIB="$(ILIB)" \
		XCFLAGS="$(SOLARINC) $(ZLIB_CFLAGS)" \
		NSPR_CONFIGURE_OPTS="$(gb_CONFIGURE_PLATFORMS)" \
		$(if $(CROSS_COMPILING),\
			CROSS_COMPILE=1 \
			$(if $(filter AARCH64,$(CPUNAME)),CPU_ARCH=aarch64)) \
		$(MAKE) nss_build_all RC="rc.exe $(SOLARINC)" \
			NSINSTALL='$(call gb_ExternalExecutable_get_command,python) $(SRCDIR)/external/nss/nsinstall.py' \
			NSS_DISABLE_GTESTS=1 \
			CCC="$(CXX)" \
	,nss)
	$(call gb_Trace_EndRange,nss,EXTERNAL)

else # OS!=WNT
# make sure to specify NSPR_CONFIGURE_OPTS as env (before make command), so nss can append it's own defaults
# OTOH specify e.g. CC and NSINSTALL as arguments (after make command), so they will overrule nss makefile values
$(call gb_ExternalProject_get_state_target,nss,build): \
		$(call gb_ExternalExecutable_get_dependencies,python) \
		$(SRCDIR)/external/nss/nsinstall.py
	$(call gb_Trace_StartRange,nss,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		$(if $(filter ANDROID FREEBSD LINUX MACOSX,$(OS)),$(if $(filter X86_64,$(CPUNAME)),USE_64=1)) \
		$(if $(filter AARCH64,$(CPUNAME)),USE_64=1 CPU_ARCH=aarch64) \
		$(if $(filter POWERPC64,$(CPUNAME)),USE_64=1 CPU_ARCH=ppc64le) \
		$(if $(filter MACOSX,$(OS)),\
			MACOS_SDK_DIR=$(MACOSX_SDK_PATH) \
			NSS_USE_SYSTEM_SQLITE=1) \
		$(if $(filter LINUX,$(OS)),$(if $(ENABLE_DBGUTIL),,BUILD_OPT=1)) \
		$(if $(filter SOLARIS,$(OS)),NS_USE_GCC=1) \
		$(if $(filter ARM,$(CPUNAME)),NSS_DISABLE_ARM32_NEON=1) \
		NSPR_CONFIGURE_OPTS="$(gb_CONFIGURE_PLATFORMS)" \
		$(if $(CROSS_COMPILING),CROSS_COMPILE=1) \
		$(if $(filter MACOSX-X86_64-arm64,$(OS)-$(CPUNAME)-$(shell uname -m)), \
			CPU_ARCH=x86_64) \
		NSDISTMODE=copy \
		$(MAKE) \
			AR="$(AR)" \
			RANLIB="$(RANLIB)" \
			NMEDIT="$(NM)edit" \
			COMMA=$(COMMA) \
			CC="$(CC) $(gb_DEBUGINFO_FLAGS) \
				$(if $(filter -fsanitize=undefined,$(CC)),-fno-sanitize=function) \
				$(if $(filter iOS,$(OS)), -DNSS_STATIC_SOFTOKEN=1 -DNSS_STATIC_FREEBL=1 -DNSS_STATIC_PKCS11=1) \
				$(if $(filter ANDROID,$(OS)), -DSQLITE_DISABLE_LFS=1)" \
			CCC="$(CXX) $(gb_DEBUGINFO_FLAGS)" \
			$(if $(CROSS_COMPILING),NSINSTALL="$(if $(filter MACOSX,$(OS_FOR_BUILD)),xcrun python3,$(call gb_ExternalExecutable_get_command,python)) $(SRCDIR)/external/nss/nsinstall.py") \
			$(if $(filter ANDROID,$(OS)),OS_TARGET=Android OS_TARGET_RELEASE=$(ANDROID_API_LEVEL) ARCHFLAG="" DEFAULT_COMPILER=clang ANDROID_NDK=$(ANDROID_NDK_DIR) ANDROID_TOOLCHAIN_VERSION=obsolete ANDROID_PREFIX=$(HOST_PLATFORM) ANDROID_SYSROOT=$(ANDROID_NDK_DIR)/sysroot) \
			NSS_DISABLE_GTESTS=1 \
			nss_build_all \
		&& rm -f $(gb_UnpackedTarball_workdir)/nss/dist/out/lib/*.a \
		$(if $(filter MACOSX,$(OS)),\
			&& chmod u+w $(gb_UnpackedTarball_workdir)/nss/dist/out/lib/*.dylib \
			&& $(PERL) \
				$(SRCDIR)/solenv/bin/macosx-change-install-names.pl shl OOO \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libfreebl3.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libnspr4.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libnss3.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libnssckbi.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libnssdbm3.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libnssutil3.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libplc4.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libplds4.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libsmime3.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libsoftokn3.dylib \
				$(EXTERNAL_WORKDIR)/dist/out/lib/libssl3.dylib) \
	,nss)
	$(call gb_Trace_EndRange,nss,EXTERNAL)

endif

# vim: set noet sw=4 ts=4:
