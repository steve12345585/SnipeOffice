# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UITest_UITest,math_demo))

$(eval $(call gb_UITest_add_modules,math_demo,$(SRCDIR)/uitest,\
	math_tests/ \
))

$(eval $(call gb_UITest_set_defs,math_demo, \
    TDOC="$(SRCDIR)/uitest/math_tests/data" \
))

# vim: set noet sw=4 ts=4:
