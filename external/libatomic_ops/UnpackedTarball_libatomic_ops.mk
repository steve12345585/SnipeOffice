# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UnpackedTarball_UnpackedTarball,libatomic_ops))

$(eval $(call gb_UnpackedTarball_set_tarball,libatomic_ops,$(LIBATOMIC_OPS_TARBALL)))

$(eval $(call gb_UnpackedTarball_update_autoconf_configs,libatomic_ops))

# vim: set noet sw=4 ts=4:
