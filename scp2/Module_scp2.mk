# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Module_Module,scp2))

$(eval $(call gb_Module_add_targets,scp2,\
	AutoInstall \
	CustomTarget_langmacros \
	InstallModule_base \
	InstallModule_calc \
	InstallModule_draw \
	InstallModule_extensions \
	InstallModule_graphicfilter \
	InstallModule_impress \
	InstallModule_math \
	InstallModule_onlineupdate \
	InstallModule_ooo \
	InstallModule_python \
	InstallModule_spsupp \
	InstallModule_ure \
	InstallModule_writer \
	InstallModule_xsltfilter \
	InstallScript_setup_osl \
	$(if $(filter ODK,$(BUILD_TYPE)), \
		InstallModule_sdkoo \
		InstallScript_sdkoo \
	) \
	$(if $(filter WNT,$(OS)),\
		InstallModule_activex \
		InstallModule_quickstart \
		InstallModule_windows \
		InstallModule_winexplorerext \
	) \
	$(if $(filter TRUE,$(ENABLE_EVOAB2) $(ENABLE_GIO) $(ENABLE_GTK3)),\
		InstallModule_gnome \
	) \
	$(if $(filter TRUE,$(ENABLE_QT5) $(ENABLE_QT6) $(ENABLE_KF5) $(ENABLE_KF6) $(ENABLE_GTK3_KDE5)),\
		InstallModule_kde \
	) \
))

# vim: set shiftwidth=4 tabstop=4 noexpandtab:
