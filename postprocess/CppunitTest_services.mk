# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CppunitTest_CppunitTest,services))

$(eval $(call gb_CppunitTest_add_exception_objects,services, \
    postprocess/qa/services \
))

$(eval $(call gb_CppunitTest_use_externals,services, \
	boost_headers \
))

$(eval $(call gb_CppunitTest_use_libraries,services, \
	comphelper \
	cppu \
	cppuhelper \
	sal \
	test \
	unotest \
	vcl \
))

$(eval $(call gb_CppunitTest_use_sdk_api,services))
$(eval $(call gb_CppunitTest_use_api,services,oovbaapi))

$(eval $(call gb_CppunitTest_use_ure,services))
$(eval $(call gb_CppunitTest_use_vcl,services))

$(eval $(call gb_CppunitTest_use_rdb,services,services))
ifneq ($(DISABLE_PYTHON),TRUE)
$(eval $(call gb_CppunitTest_use_python_ure,services))

$(eval $(call gb_CppunitTest_use_rdb,services,pyuno))

$(eval $(call gb_CppunitTest_use_packages,services, \
   pyuno_pythonloader_ini \
))

$(call gb_CppunitTest_get_target,services): $(call gb_Pyuno_get_target,commonwizards)
endif

$(eval $(call gb_CppunitTest_use_configuration,services))

ifeq ($(ENABLE_JAVA),TRUE)
$(eval $(call gb_CppunitTest_use_jars,services,\
	ScriptProviderForJava \
	smoketest \
))
endif

$(eval $(call gb_CppunitTest_use_packages,services,\
	autotextshare_en-US \
	instsetoo_native_setup \
))

# vim: set noet sw=4 ts=4:
