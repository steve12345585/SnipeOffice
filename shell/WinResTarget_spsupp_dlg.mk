# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_WinResTarget_WinResTarget,spsupp_dlg))

$(eval $(call gb_WinResTarget_set_rcfile,spsupp_dlg,shell/source/win32/spsupp/res/spsuppDlg))

# vim: set shiftwidth=4 tabstop=4 noexpandtab:
