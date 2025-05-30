# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Extension_Extension,test-active,desktop/test/deployment/active,nodeliver))

$(eval $(call gb_Extension_add_files,test-active,, \
    $(SRCDIR)/desktop/test/deployment/active/Addons.xcu \
    $(SRCDIR)/desktop/test/deployment/active/ProtocolHandler.xcu \
    $(SRCDIR)/desktop/test/deployment/active/active_python.py \
    $(call gb_Jar_get_target,active_java) \
))

$(eval $(call gb_Extension_add_libraries,test-active, \
    active_native \
))

# vim: set noet sw=4 ts=4:
