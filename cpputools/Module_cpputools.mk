# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,cpputools))

$(eval $(call gb_Module_add_targets,cpputools,\
    $(call gb_CondExeSp2bv,Executable_sp2bv) \
    $(call gb_CondExeUno, \
        Executable_uno \
        Package_uno_sh \
    ) \
))

# vim:set noet sw=4 ts=4:
