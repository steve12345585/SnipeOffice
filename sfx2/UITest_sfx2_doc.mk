# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UITest_UITest,sfx2_doc))

$(eval $(call gb_UITest_add_modules,sfx2_doc,$(SRCDIR)/sfx2/qa/uitest,\
	doc/ \
))

$(eval $(call gb_UITest_set_defs,sfx2_doc, \
    TDOC="$(SRCDIR)/sfx2/qa/uitest/doc/data" \
))

# vim: set noet sw=4 ts=4:
