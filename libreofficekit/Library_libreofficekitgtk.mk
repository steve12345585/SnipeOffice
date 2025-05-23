# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Library_Library,libreofficekitgtk))

$(eval $(call gb_Library_use_sdk_api,libreofficekitgtk))

$(eval $(call gb_Library_add_exception_objects,libreofficekitgtk,\
    libreofficekit/source/gtk/lokdocview \
    libreofficekit/source/gtk/tilebuffer \
))

$(eval $(call gb_Library_use_externals,libreofficekitgtk,\
    boost_headers \
))

$(eval $(call gb_Library_set_include,libreofficekitgtk,\
    $$(INCLUDE) \
    $$(GTK3_CFLAGS) \
))

$(eval $(call gb_Library_add_libs,libreofficekitgtk,\
    $(GTK3_LIBS) \
))

$(eval $(call gb_Library_add_defs,libreofficekitgtk,\
	-DLOK_PATH="\"$(LIBDIR)/libreoffice/$(LIBO_LIB_FOLDER)\"" \
	-DLOK_DOC_VIEW_IMPLEMENTATION \
))

ifeq ($(OS),$(filter LINUX %BSD SOLARIS, $(OS)))
$(eval $(call gb_Library_add_libs,libreofficekitgtk,\
    $(UNIX_DLAPI_LIBS) -lm \
))
endif

$(eval $(call gb_Library_use_packages,libreofficekitgtk, \
    libreofficekit_selectionhandles \
))

# vim: set noet sw=4 ts=4:
