# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_InstallScript_InstallScript,setup_osl))

$(eval $(call gb_InstallScript_use_modules,setup_osl,\
	scp2/base \
	scp2/calc \
	scp2/draw \
	scp2/graphicfilter \
	scp2/impress \
	scp2/math \
	scp2/ooo \
	scp2/python \
	scp2/ure \
	scp2/writer \
	scp2/xsltfilter \
	$(if $(filter WNT,$(OS)),\
		scp2/activex \
		scp2/quickstart \
		scp2/spsupp \
		scp2/windows \
		$(if $(filter MSC,$(COM)),\
			scp2/winexplorerext \
		) \
	) \
	$(if $(WITH_EXTENSION_INTEGRATION),\
		scp2/extensions \
	) \
	$(if $(filter TRUE,$(ENABLE_EVOAB2) $(ENABLE_GIO) $(ENABLE_GTK3)),\
		scp2/gnome \
	) \
	$(if $(filter TRUE,$(ENABLE_QT5) $(ENABLE_QT6) $(ENABLE_KF5) $(ENABLE_KF6) $(ENABLE_GTK3_KDE5)),\
		scp2/kde \
	) \
	$(if $(filter TRUE,$(ENABLE_ONLINE_UPDATE)),\
		scp2/onlineupdate \
	) \
))

# vim: set shiftwidth=4 tabstop=4 noexpandtab:
