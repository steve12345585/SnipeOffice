# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,stoc))

$(eval $(call gb_Module_add_targets,stoc,\
    Library_bootstrap \
    Library_introspection \
    Library_invocadapt \
    Library_invocation \
    Library_namingservice \
    Library_proxyfac \
    Library_reflection \
    Library_stocservices \
))

ifeq ($(ENABLE_JAVA),TRUE)

$(eval $(call gb_Module_add_targets,stoc,\
    Library_javaloader \
    Library_javavm \
))

endif

$(eval $(call gb_Module_add_check_targets,stoc, \
    CppunitTest_stoc_dump \
    CppunitTest_stoc_uriproc \
))

# vim:set noet sw=4 ts=4:
