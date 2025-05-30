# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,jvmfwk_javavendors,$(SRCDIR)/jvmfwk))

ifeq ($(OS),WNT)
$(eval $(call gb_Package_add_file,jvmfwk_javavendors,$(LIBO_URE_MISC_FOLDER)/javavendors.xml,distributions/OpenOfficeorg/javavendors_wnt.xml))
else ifeq ($(OS),FREEBSD)
$(eval $(call gb_Package_add_file,jvmfwk_javavendors,$(LIBO_URE_MISC_FOLDER)/javavendors.xml,distributions/OpenOfficeorg/javavendors_freebsd.xml))
else ifeq ($(OS),MACOSX)
ifeq ($(CPUNAME),AARCH64)
$(eval $(call gb_Package_add_file,jvmfwk_javavendors,$(LIBO_URE_MISC_FOLDER)/javavendors.xml,distributions/OpenOfficeorg/javavendors_macosx_aarch64.xml))
else
$(eval $(call gb_Package_add_file,jvmfwk_javavendors,$(LIBO_URE_MISC_FOLDER)/javavendors.xml,distributions/OpenOfficeorg/javavendors_macosx.xml))
endif
else ifeq ($(OS),LINUX)
$(eval $(call gb_Package_add_file,jvmfwk_javavendors,$(LIBO_URE_MISC_FOLDER)/javavendors.xml,distributions/OpenOfficeorg/javavendors_linux.xml))
else
$(eval $(call gb_Package_add_file,jvmfwk_javavendors,$(LIBO_URE_MISC_FOLDER)/javavendors.xml,distributions/OpenOfficeorg/javavendors_unx.xml))
endif

# vim:set noet sw=4 ts=4:
