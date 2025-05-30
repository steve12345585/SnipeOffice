# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Pagein_Pagein,draw))

$(eval $(call gb_Pagein_add_objects,draw,\
    sd \
    sdui \
    svx \
    svxcore \
))

# vim: set ts=4 sw=4 et:
