# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalPackage_ExternalPackage,fonts_culmus,font_culmus))

$(eval $(call gb_ExternalPackage_add_unpacked_files,fonts_culmus,$(LIBO_SHARE_FOLDER)/fonts/truetype,\
	DavidCLM-Bold.otf \
	DavidCLM-BoldItalic.otf \
	DavidCLM-Medium.otf \
	DavidCLM-MediumItalic.otf \
	FrankRuehlCLM-Bold.otf \
	FrankRuehlCLM-BoldOblique.otf \
	FrankRuehlCLM-Medium.otf \
	FrankRuehlCLM-MediumOblique.otf \
	MiriamCLM-Bold.otf \
	MiriamCLM-Book.otf \
	MiriamMonoCLM-Bold.ttf \
	MiriamMonoCLM-BoldOblique.ttf \
	MiriamMonoCLM-Book.ttf \
	MiriamMonoCLM-BookOblique.ttf \
	NachlieliCLM-Bold.otf \
	NachlieliCLM-BoldOblique.otf \
	NachlieliCLM-Light.otf \
	NachlieliCLM-LightOblique.otf \
))

# vim: set noet sw=4 ts=4:
