# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Jar_Jar,TestExtension))

$(eval $(call gb_Jar_set_manifest,TestExtension,$(SRCDIR)/smoketest/com/sun/star/comp/smoketest/MANIFEST.MF))

$(eval $(call gb_Jar_add_manifest_classpath,TestExtension,\
	libreoffice.jar \
))

$(eval $(call gb_Jar_use_jars,TestExtension,\
	libreoffice \
))

$(eval $(call gb_Jar_set_packageroot,TestExtension,com))

$(eval $(call gb_Jar_add_sourcefiles,TestExtension,\
	smoketest/com/sun/star/comp/smoketest/TestExtension \
))

# vim: set noet sw=4 ts=4:
