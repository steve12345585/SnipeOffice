# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
#
#   Licensed to the Apache Software Foundation (ASF) under one or more
#   contributor license agreements. See the NOTICE file distributed
#   with this work for additional information regarding copyright
#   ownership. The ASF licenses this file to you under the Apache
#   License, Version 2.0 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.apache.org/licenses/LICENSE-2.0 .
#

$(eval $(call gb_Library_Library,tl))

$(eval $(call gb_Library_set_include,tl,\
    -I$(SRCDIR)/tools/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_Library_set_precompiled_header,tl,tools/inc/pch/precompiled_tl))

$(eval $(call gb_Library_add_defs,tl,\
    -DTOOLS_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_use_sdk_api,tl))

$(eval $(call gb_Library_use_libraries,tl,\
    basegfx \
    comphelper \
    i18nlangtag \
    cppu \
    cppuhelper \
    sal \
))


$(eval $(call gb_Library_add_exception_objects,tl,\
    tools/source/datetime/datetime \
    tools/source/datetime/datetimeutils \
    tools/source/datetime/duration \
    tools/source/datetime/systemdatetime \
    tools/source/datetime/tdate \
    tools/source/datetime/ttime \
    tools/source/debug/debug \
    tools/source/fsys/fileutil \
    tools/source/fsys/urlobj \
    tools/source/fsys/wldcrd \
    tools/source/generic/b3dtrans \
    tools/source/generic/bigint \
    tools/source/generic/color \
    tools/source/generic/config \
    tools/source/generic/fract \
    tools/source/generic/gen \
    tools/source/generic/line \
    tools/source/generic/point \
    tools/source/generic/poly \
    tools/source/generic/poly2 \
    tools/source/generic/svborder \
    tools/source/inet/inetmime \
    tools/source/inet/inetmsg \
    tools/source/inet/inetstrm \
    tools/source/inet/hostfilter \
    tools/source/memtools/multisel \
    tools/source/misc/cpuid \
    tools/source/misc/extendapplicationenvironment \
    tools/source/misc/json_writer \
    tools/source/misc/lazydelete \
    tools/source/misc/UniqueID \
    tools/source/ref/globname \
    tools/source/ref/ref \
    tools/source/stream/stream \
    tools/source/stream/vcompat \
    tools/source/stream/GenericTypeSerializer \
    tools/source/string/tenccvt \
    tools/source/zcodec/zcodec \
    tools/source/xml/XmlWriter \
    tools/source/xml/XmlWalker \
))

ifneq ($(SYSTEM_LIBFIXMATH),TRUE)
$(eval $(call gb_Library_add_exception_objects,tl,\
    tools/source/misc/fix16 \
))
endif

ifeq ($(OS),WNT)
$(eval $(call gb_Library_add_exception_objects,tl, \
    tools/source/stream/strmwnt \
))
else
$(eval $(call gb_Library_add_exception_objects,tl, \
    tools/source/stream/strmunx \
))
endif

$(eval $(call gb_Library_add_generated_exception_objects,tl,\
    CustomTarget/tools/string/reversemap \
))

$(eval $(call gb_Library_use_externals,tl,\
	boost_headers \
	zlib \
	libxml2 \
))

ifeq ($(OS),LINUX)
$(eval $(call gb_Library_add_libs,tl,\
        -lrt \
))
endif

ifeq ($(SYSTEM_LIBFIXMATH),TRUE)
$(eval $(call gb_Library_add_libs,tl,\
	$(LIBFIXMATH_LIBS) \
))
endif

ifeq ($(OS),WNT)

$(eval $(call gb_Library_use_system_win32_libs,tl,\
	mpr \
	netapi32 \
	ole32 \
	shell32 \
	uuid \
	winmm \
))

endif

# vim: set noet sw=4 ts=4:
