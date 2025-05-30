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

$(eval $(call gb_Module_Module,framework))

$(eval $(call gb_Module_add_targets,framework,\
    Library_fwk \
    Package_dtd \
    UIConfig_startmodule \
))

$(eval $(call gb_Module_add_slowcheck_targets,framework,\
    CppunitTest_framework_dispatch \
    CppunitTest_framework_loadenv \
	CppunitTest_framework_CheckXTitle \
))

# Not sure why this is not stable on macOS.
ifneq ($(OS),MACOSX)
$(eval $(call gb_Module_add_slowcheck_targets,framework,\
    CppunitTest_framework_services \
))
endif

$(eval $(call gb_Module_add_l10n_targets,framework,\
    AllLangMoTarget_fwk \
))

$(eval $(call gb_Module_add_subsequentcheck_targets,framework,\
    JunitTest_framework_complex \
    JunitTest_framework_unoapi \
))

# vim: set noet sw=4 ts=4:
