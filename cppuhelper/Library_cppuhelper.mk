# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,cppuhelper))

$(eval $(call gb_Library_set_soversion_script,cppuhelper,$(SRCDIR)/cppuhelper/source/gcc3.map))

$(eval $(call gb_Library_set_precompiled_header,cppuhelper,cppuhelper/inc/pch/precompiled_cppuhelper))

$(eval $(call gb_Library_use_internal_comprehensive_api,cppuhelper,\
	cppuhelper \
	udkapi \
	offapi \
))

$(eval $(call gb_Library_set_is_ure_library_or_dependency,cppuhelper))

$(eval $(call gb_Library_add_defs,cppuhelper,\
	-DCPPUHELPER_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_use_libraries,cppuhelper,\
	cppu \
	reg \
	sal \
	salhelper \
	unoidl \
	xmlreader \
))

$(eval $(call gb_Library_use_static_libraries,cppuhelper,\
	findsofficepath \
))

$(eval $(call gb_Library_use_externals,cppuhelper,\
    boost_headers \
))

ifeq ($(OS),iOS)
$(eval $(call gb_Library_add_cxxflags,cppuhelper,\
    $(gb_OBJCXXFLAGS) \
))
endif

$(eval $(call gb_Library_set_include,cppuhelper,\
    -I$(SRCDIR)/cppuhelper/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_Library_add_exception_objects,cppuhelper,\
	cppuhelper/source/access_control \
	cppuhelper/source/bootstrap \
	cppuhelper/source/compat \
	cppuhelper/source/compbase \
	cppuhelper/source/component_context \
	cppuhelper/source/component \
	cppuhelper/source/defaultbootstrap \
	cppuhelper/source/exc_thrower \
	cppuhelper/source/factory \
	cppuhelper/source/implbase \
	cppuhelper/source/implbase_ex \
	cppuhelper/source/implementationentry \
	cppuhelper/source/interfacecontainer \
	cppuhelper/source/macro_expander \
	cppuhelper/source/paths \
	cppuhelper/source/propertysetmixin \
	cppuhelper/source/propshlp \
	cppuhelper/source/servicemanager \
	cppuhelper/source/shlib \
	cppuhelper/source/supportsservice \
	cppuhelper/source/tdmgr \
	cppuhelper/source/typemanager \
	cppuhelper/source/typeprovider \
	cppuhelper/source/unoimplbase \
	cppuhelper/source/unourl \
	cppuhelper/source/weak \
))

# vim: set noet sw=4 ts=4:
