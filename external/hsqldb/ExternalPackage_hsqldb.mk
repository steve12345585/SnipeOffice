# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalPackage_ExternalPackage,hsqldb_project,hsqldb))

$(eval $(call gb_ExternalPackage_use_external_project,hsqldb_project,hsqldb))

$(eval $(call gb_ExternalPackage_add_file,hsqldb_project,$(LIBO_SHARE_JAVA_FOLDER)/hsqldb.jar,lib/hsqldb.jar))

# vim: set noet sw=4 ts=4:
