# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,desktop_scalc_sh,$(SRCDIR)/desktop/scripts))

$(eval $(call gb_Package_add_file,desktop_scalc_sh,$(LIBO_BIN_FOLDER)/scalc,scalc.sh))

# vim: set ts=4 sw=4 noet:
