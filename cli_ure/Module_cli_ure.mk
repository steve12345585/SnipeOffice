# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,cli_ure))

ifeq ($(ENABLE_CLI),TRUE)
ifeq ($(COM),MSC)
ifneq ($(CPUNAME)_$(CROSS_COMPILING),AARCH64_TRUE)
$(eval $(call gb_Module_add_targets,cli_ure,\
	CliLibrary_cli_basetypes \
	CliLibrary_cli_ure \
	CliNativeLibrary_cli_cppuhelper \
	CliUnoApi_cli_uretypes \
	CustomTarget_cli_ure_assemblies \
	Executable_climaker \
	Library_cli_cppuhelper_native \
	Library_cli_uno \
	Package_cli_basetypes_copy \
))
endif
endif
endif

# vim: set noet sw=4 ts=4:
