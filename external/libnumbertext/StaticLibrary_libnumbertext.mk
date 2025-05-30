# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_StaticLibrary_StaticLibrary,libnumbertext))

$(eval $(call gb_StaticLibrary_use_unpacked,libnumbertext,libnumbertext))

$(eval $(call gb_StaticLibrary_set_warnings_disabled,libnumbertext))

$(eval $(call gb_StaticLibrary_add_generated_exception_objects,libnumbertext,\
        UnpackedTarball/libnumbertext/src/Soros \
        UnpackedTarball/libnumbertext/src/Numbertext \
))


# vim: set noet sw=4 ts=4:
