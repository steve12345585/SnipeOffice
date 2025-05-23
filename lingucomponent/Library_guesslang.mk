# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,guesslang))

$(eval $(call gb_Library_set_componentfile,guesslang,lingucomponent/source/languageguessing/guesslang,services))

ifneq ($(ENABLE_WASM_STRIP_GUESSLANG),TRUE)
$(eval $(call gb_Library_use_externals,guesslang,\
	libexttextcat \
))
endif

$(eval $(call gb_Library_use_externals,guesslang,\
	boost_headers \
))

$(eval $(call gb_Library_use_sdk_api,guesslang))

$(eval $(call gb_Library_use_libraries,guesslang,\
	cppu \
	cppuhelper \
	sal \
	tl \
	utl \
))

$(eval $(call gb_Library_add_exception_objects,guesslang,\
	lingucomponent/source/languageguessing/guess \
	lingucomponent/source/languageguessing/guesslang \
	lingucomponent/source/languageguessing/simpleguesser \
))

# vim: set noet sw=4 ts=4:
