# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,sal_textenc))

$(eval $(call gb_Library_set_plugin_for,sal_textenc,sal))

$(eval $(call gb_Library_add_exception_objects,sal_textenc,\
	sal/textenc/context \
	sal/textenc/convertbig5hkscs \
	sal/textenc/converteuctw \
	sal/textenc/convertgb18030 \
	sal/textenc/convertisciidevangari \
	sal/textenc/convertiso2022cn \
	sal/textenc/convertiso2022jp \
	sal/textenc/convertiso2022kr \
	sal/textenc/convertsinglebytetobmpunicode \
	sal/textenc/tables \
	sal/textenc/tcvtbyte \
	sal/textenc/tcvtmb \
	sal/textenc/tcvtutf7 \
))

# vim: set noet sw=4 ts=4:
