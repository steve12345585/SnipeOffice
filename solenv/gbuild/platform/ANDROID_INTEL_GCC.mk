# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# please make generic modifications to unxgcc.mk or android.mk
gb_CPUDEFS += -DX86
gb_COMPILEROPTFLAGS := -Os

include $(GBUILDDIR)/platform/unxgcc.mk
include $(GBUILDDIR)/platform/android.mk

# vim: set noet sw=4:
