# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Jar_Jar,ScriptProviderForJavaScript))

$(eval $(call gb_Jar_use_jars,ScriptProviderForJavaScript,\
	libreoffice \
	ScriptFramework \
))

$(eval $(call gb_Jar_use_externals,ScriptProviderForJavaScript,\
	rhino \
))

$(eval $(call gb_Jar_set_manifest,ScriptProviderForJavaScript,$(SRCDIR)/scripting/java/com/sun/star/script/framework/provider/javascript/MANIFEST.MF))

$(eval $(call gb_Jar_set_componentfile,ScriptProviderForJavaScript,scripting/java/ScriptProviderForJavaScript,OOO,scriptproviderforjavascript))

$(eval $(call gb_Jar_set_packageroot,ScriptProviderForJavaScript,com))

$(eval $(call gb_Jar_add_sourcefiles,ScriptProviderForJavaScript,\
	scripting/java/com/sun/star/script/framework/provider/javascript/ScriptProviderForJavaScript \
))

# vim: set noet sw=4 ts=4:
