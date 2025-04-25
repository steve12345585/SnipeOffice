# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
# 

$(eval $(call 	gb_UITest_UITest,writer_macro_tests))

$(eval $(call gb_UITest_add_modules,writer_macro_tests,$(SRCDIR)/sw/qa/uitest,\
	macro_tests/ \
))

$(eval $(call gb_UITest_set_defs,writer_macro_tests, \
    TDOC="$(SRCDIR)/sw/qa/uitest/data" \
))

$(eval $(call gb_UITest_avoid_oneprocess,writer_macro_tests))

# vim: set noet sw=4 ts=4:
