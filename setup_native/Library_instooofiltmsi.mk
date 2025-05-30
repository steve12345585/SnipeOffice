# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,instooofiltmsi))

$(eval $(call gb_Library_add_defs,instooofiltmsi,\
	-U_DLL \
))

$(eval $(call gb_Library_add_cxxflags,instooofiltmsi,\
	$(if $(MSVC_USE_DEBUG_RUNTIME),/MTd,/MT) \
    $(if $(filter -fsanitize=%,$(CC)),,/fno-sanitize-address-vcasan-lib) \
))

$(eval $(call gb_Library_add_ldflags,instooofiltmsi,\
	/DEF:$(SRCDIR)/setup_native/source/win32/customactions/indexingfilter/instooofiltmsi.def \
	/NODEFAULTLIB \
))

$(eval $(call gb_Library_add_exception_objects,instooofiltmsi,\
    setup_native/source/win32/customactions/indexingfilter/restartindexingservice \
))

$(eval $(call gb_Library_use_system_win32_libs,instooofiltmsi,\
	libcmt \
	libcpmt \
	libucrt \
	libvcruntime \
	kernel32 \
	advapi32 \
))

# vim: set noet sw=4 ts=4:
