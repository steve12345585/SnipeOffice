# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Extension_Extension,test-crashextension,desktop/test/deployment/crashextension,nodeliver))

$(eval $(call gb_Extension_add_file,test-crashextension,platform.components,$(call gb_Rdb_get_target,crashextension)))

$(eval $(call gb_Extension_add_files,test-crashextension,, \
    $(SRCDIR)/desktop/test/deployment/crashextension/Addons.xcu \
    $(SRCDIR)/desktop/test/deployment/crashextension/ProtocolHandler.xcu \
    $(SRCDIR)/desktop/test/deployment/crashextension/crash.png \
))

$(eval $(call gb_Extension_add_libraries,test-crashextension, \
    crashextension \
))

# vim: set noet sw=4 ts=4:
