# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,starmath))

$(eval $(call gb_Module_add_targets,starmath,\
    Library_sm \
    Library_smd \
    UIConfig_smath \
))

$(eval $(call gb_Module_add_l10n_targets,starmath,\
    AllLangMoTarget_sm \
))

$(eval $(call gb_Module_add_check_targets,starmath,\
    CppunitTest_starmath_export \
    CppunitTest_starmath_import \
    CppunitTest_starmath_qa_cppunit \
))

$(eval $(call gb_Module_add_subsequentcheck_targets,starmath,\
    JunitTest_starmath_unoapi \
))

# screenshots
$(eval $(call gb_Module_add_screenshot_targets,starmath,\
    CppunitTest_starmath_dialogs_test \
))

# vim: set noet sw=4 ts=4:
