# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CustomTarget_CustomTarget,bridges/source/cpp_uno/gcc3_linux_arm))

$(call gb_CustomTarget_get_target,bridges/source/cpp_uno/gcc3_linux_arm) : \
	$(gb_CustomTarget_workdir)/bridges/source/cpp_uno/gcc3_linux_arm/armhelper.objectlist

$(gb_CustomTarget_workdir)/bridges/source/cpp_uno/gcc3_linux_arm/armhelper.o : \
		$(SRCDIR)/bridges/source/cpp_uno/gcc3_linux_arm/armhelper.S \
		| $(gb_CustomTarget_workdir)/bridges/source/cpp_uno/gcc3_linux_arm/.dir
	$(gb_CXX) -c -o $@ $< -fPIC

$(gb_CustomTarget_workdir)/bridges/source/cpp_uno/gcc3_linux_arm/armhelper.objectlist : \
		$(gb_CustomTarget_workdir)/bridges/source/cpp_uno/gcc3_linux_arm/armhelper.o \
		| $(gb_CustomTarget_workdir)/bridges/source/cpp_uno/gcc3_linux_arm/.dir
	echo $< > $@

# vim: set noet sw=4 ts=4:
