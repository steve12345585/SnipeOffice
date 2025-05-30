# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Package_Package,scripting_scriptproviderforpython,$(SRCDIR)/scripting/source/pyprov))

$(eval $(call gb_Package_add_file,scripting_scriptproviderforpython,$(LIBO_ETC_FOLDER)/services/scriptproviderforpython.rdb,scriptproviderforpython.rdb))
$(eval $(call gb_Package_add_file,scripting_scriptproviderforpython,$(LIBO_LIB_PYUNO_FOLDER)/pythonscript.py,pythonscript.py))

# vim: set noet sw=4 ts=4:
