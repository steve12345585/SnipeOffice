# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call 	gb_UITest_UITest,impress_tests))

$(eval $(call gb_UITest_add_modules,impress_tests,$(SRCDIR)/sd/qa/uitest,\
	impress_tests/ \
))

$(eval $(call gb_UITest_set_defs,impress_tests, \
    TDOC="$(SRCDIR)/sd/qa/uitest/data" \
))

$(eval $(call gb_UITest_avoid_oneprocess,impress_tests))

# vim: set noet sw=4 ts=4:
