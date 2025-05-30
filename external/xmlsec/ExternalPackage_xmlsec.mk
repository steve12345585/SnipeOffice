# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalPackage_ExternalPackage,xmlsec,xmlsec))

$(eval $(call gb_ExternalPackage_use_external_project,xmlsec,xmlsec))

ifeq ($(OS),WNT)
$(eval $(call gb_ExternalPackage_add_file,xmlsec,$(LIBO_LIB_FOLDER)/libxmlsec-mscng.dll,win32/binaries/libxmlsec-mscng.dll))
$(eval $(call gb_ExternalPackage_add_file,xmlsec,$(LIBO_LIB_FOLDER)/libxmlsec.dll,win32/binaries/libxmlsec.dll))
endif

# vim: set noet sw=4 ts=4:
