# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalPackage_ExternalPackage,cairo,cairo))

$(eval $(call gb_ExternalPackage_use_external_project,cairo,cairo))

ifneq ($(DISABLE_DYNLOADING),TRUE)
$(eval $(call gb_ExternalPackage_add_file,cairo,$(LIBO_LIB_FOLDER)/libcairo-lo.so.2,src/.libs/libcairo-lo.so.2.1170$(CAIRO_VERSION_MICRO).0))
endif

# vim: set noet sw=4 ts=4:
