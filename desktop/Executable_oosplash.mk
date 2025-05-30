# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,oosplash))

$(eval $(call gb_Executable_set_targettype_gui,oosplash,YES))

$(eval $(call gb_Executable_use_libraries,oosplash,\
    sal \
))

$(eval $(call gb_Executable_add_cobjects,oosplash,\
    desktop/unx/source/args \
    desktop/unx/source/file_image_unx \
    $(if $(ENABLE_PAGEIN),desktop/unx/source/pagein) \
    desktop/unx/source/splashx \
    desktop/unx/source/start \
))

ifneq ($(USING_X11),)
$(eval $(call gb_Executable_add_libs,oosplash,\
    -lXinerama \
))
endif

ifneq ($(DISABLE_GUI),TRUE)

ifneq ($(OS),WNT)
$(eval $(call gb_Executable_add_libs,oosplash,\
    -lX11 \
))
endif

$(eval $(call gb_Executable_add_defs,oosplash,\
    -DENABLE_QUICKSTART_LIBPNG \
))

$(eval $(call gb_Executable_use_externals,oosplash,\
    libpng \
))

endif

ifeq ($(OS),LINUX)
$(eval $(call gb_Executable_add_libs,oosplash,\
    -lm \
))
endif

ifeq ($(OS),SOLARIS)

$(eval $(call gb_Executable_add_libs,oosplash,\
    -lsocket \
))

endif

# vim: set ts=4 sw=4 et:
