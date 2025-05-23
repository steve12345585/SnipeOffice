# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,javaunohelper))

ifneq ($(ENABLE_JAVA),)

$(eval $(call gb_Module_add_targets,javaunohelper,\
    Library_juh \
    Jar_juh \
))

$(eval $(call gb_Module_add_subsequentcheck_targets,javaunohelper,\
    JunitTest_juh \
))

ifneq ($(DISABLE_DYNLOADING),TRUE)
$(eval $(call gb_Module_add_targets,javaunohelper,\
    Library_juhx \
))
endif

endif

# vim:set noet sw=4 ts=4:
