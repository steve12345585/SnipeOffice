# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t; fill-column: 100 -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CustomTarget_CustomTarget,bridges/gcc3_wasm))

$(eval $(call gb_CustomTarget_register_targets,bridges/gcc3_wasm, \
    generated-cxx.cxx \
    generated-asm.s \
    exports \
))

$(gb_CustomTarget_workdir)/bridges/gcc3_wasm/generated-asm.s \
$(gb_CustomTarget_workdir)/bridges/gcc3_wasm/generated-cxx.cxx \
$(gb_CustomTarget_workdir)/bridges/gcc3_wasm/exports: \
        $(call gb_Executable_get_target_for_build,wasmbridgegen) \
        $(call gb_UnoApi_get_target,udkapi) \
        $(call gb_UnoApi_get_target,offapi)
	$(call gb_Executable_get_command,wasmbridgegen) \
        $(gb_CustomTarget_workdir)/bridges/gcc3_wasm/generated-cxx.cxx \
        $(gb_CustomTarget_workdir)/bridges/gcc3_wasm/generated-asm.s \
        $(gb_CustomTarget_workdir)/bridges/gcc3_wasm/exports \
        +$(call gb_UnoApi_get_target,udkapi) +$(call gb_UnoApi_get_target,offapi)

# vim: set noet sw=4 ts=4:
