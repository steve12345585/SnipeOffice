# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_StaticLibrary_StaticLibrary,boost_iostreams))

$(eval $(call gb_StaticLibrary_set_warnings_disabled,boost_iostreams))

# disable "auto link" "feature" on MSVC
$(eval $(call gb_StaticLibrary_add_defs,boost_iostreams,\
	-DBOOST_ALL_NO_LIB \
))

$(eval $(call gb_StaticLibrary_use_unpacked,boost_iostreams,boost))


$(eval $(call gb_StaticLibrary_use_externals,boost_iostreams, \
			zlib \
	boost_headers \
))

$(eval $(call gb_StaticLibrary_set_generated_cxx_suffix,boost_iostreams,cpp))

$(eval $(call gb_StaticLibrary_add_generated_exception_objects,boost_iostreams,\
	UnpackedTarball/boost/libs/iostreams/src/zlib \
	UnpackedTarball/boost/libs/iostreams/src/gzip \
	UnpackedTarball/boost/libs/iostreams/src/file_descriptor \
))

# vim: set noet sw=4 ts=4:
