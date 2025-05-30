# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
#
#   Licensed to the Apache Software Foundation (ASF) under one or more
#   contributor license agreements. See the NOTICE file distributed
#   with this work for additional information regarding copyright
#   ownership. The ASF licenses this file to you under the Apache
#   License, Version 2.0 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.apache.org/licenses/LICENSE-2.0 .
#

$(eval $(call gb_Library_Library,svx))

$(eval $(call gb_Library_set_componentfile,svx,svx/util/svx,services))

$(eval $(call gb_Library_add_componentimpls,svx, \
    $(call gb_Helper_optional,BREAKPAD,crashreport) \
    $(if $(ENABLE_WASM_STRIP_RECOVERYUI),,recoveryui) \
))

$(eval $(call gb_Library_set_include,svx,\
    -I$(SRCDIR)/svx/inc \
    -I$(SRCDIR)/svx/source/inc \
    $$(INCLUDE) \
))

$(eval $(call gb_Library_use_sdk_api,svx))

$(eval $(call gb_Library_use_custom_headers,svx,\
        officecfg/registry \
))

$(eval $(call gb_Library_add_defs,svx,\
    -DSVX_DLLIMPLEMENTATION \
))

$(eval $(call gb_Library_set_precompiled_header,svx,svx/inc/pch/precompiled_svx))

$(eval $(call gb_Library_use_libraries,svx,\
    $(call gb_Helper_optional,AVMEDIA,avmedia) \
    basegfx \
    sb \
    comphelper \
    cppuhelper \
    cppu \
    $(call gb_Helper_optional,BREAKPAD, \
		crashreport) \
    $(call gb_Helper_optional,DBCONNECTIVITY, \
        dbtools) \
    docmodel \
    drawinglayercore \
    drawinglayer \
    editeng \
    fwk \
    i18nlangtag \
    i18nutil \
    sal \
    salhelper \
    sfx \
    sot \
    svl \
    svt \
    svxcore \
    tk \
    tl \
    ucbhelper \
    utl \
    vcl \
    xo \
    xmlscript \
))

$(eval $(call gb_Library_use_externals,svx,\
	boost_headers \
    $(call gb_Helper_optional,BREAKPAD, \
		curl) \
	icuuc \
	icu_headers \
))

ifneq ($(ENABLE_WASM_STRIP_RECOVERYUI),TRUE)
$(eval $(call gb_Library_add_exception_objects,svx,\
    svx/source/dialog/docrecovery \
    svx/source/unodraw/recoveryui \
))
endif

ifneq ($(ENABLE_WASM_STRIP_ACCESSIBILITY),TRUE)
$(eval $(call gb_Library_add_exception_objects,svx,\
    svx/source/accessibility/AccessibleControlShape \
    svx/source/accessibility/AccessibleEmptyEditSource \
    svx/source/accessibility/AccessibleGraphicShape \
    svx/source/accessibility/AccessibleOLEShape \
    svx/source/accessibility/AccessibleShape \
    svx/source/accessibility/AccessibleShapeInfo \
    svx/source/accessibility/AccessibleShapeTreeInfo \
    svx/source/accessibility/AccessibleTextEventQueue \
    svx/source/accessibility/AccessibleTextHelper \
    svx/source/accessibility/ChildrenManager \
    svx/source/accessibility/ChildrenManagerImpl \
    svx/source/accessibility/DescriptionGenerator \
    svx/source/accessibility/GraphCtlAccessibleContext \
    svx/source/accessibility/ShapeTypeHandler \
    svx/source/accessibility/SvxShapeTypes \
    svx/source/accessibility/lookupcolorname \
))
endif

$(eval $(call gb_Library_add_exception_objects,svx,\
    svx/source/accessibility/AccessibleFrameSelector \
    svx/source/accessibility/charmapacc \
    svx/source/accessibility/svxpixelctlaccessiblecontext \
    svx/source/accessibility/svxrectctaccessiblecontext \
    svx/source/customshapes/EnhancedCustomShape3d \
    svx/source/customshapes/EnhancedCustomShapeEngine \
    svx/source/customshapes/EnhancedCustomShapeFontWork \
    svx/source/customshapes/EnhancedCustomShapeHandle \
    svx/source/dialog/GenericCheckDialog \
    svx/source/dialog/_bmpmask \
    svx/source/dialog/charmap \
    svx/source/dialog/cuicharmap \
    svx/source/dialog/searchcharmap \
    svx/source/dialog/connctrl \
    svx/source/dialog/_contdlg \
    svx/source/dialog/contwnd \
    svx/source/dialog/gotodlg \
    svx/source/dialog/compressgraphicdialog \
    $(call gb_Helper_optional,BREAKPAD, \
		svx/source/dialog/crashreportdlg \
		svx/source/dialog/crashreportui) \
    svx/source/dialog/ctredlin \
    svx/source/dialog/ClassificationCommon \
    svx/source/dialog/ClassificationDialog \
    svx/source/dialog/ClassificationEditView \
    svx/source/dialog/ClassificationField \
    svx/source/dialog/databaseregistrationui \
    svx/source/dialog/dialcontrol \
    svx/source/dialog/dlgctl3d \
    svx/source/dialog/dlgctrl \
    svx/source/dialog/fntctrl \
    svx/source/dialog/fontwork \
    svx/source/dialog/frmdirlbox \
    svx/source/dialog/frmsel \
    svx/source/dialog/graphctl \
    svx/source/dialog/grfflt \
    svx/source/dialog/hdft \
    svx/source/dialog/hyperdlg \
    svx/source/dialog/imapdlg \
    svx/source/dialog/imapwnd \
    svx/source/dialog/linkwarn \
    svx/source/dialog/measctrl \
    svx/source/dialog/optgrid \
    svx/source/dialog/pagectrl \
    svx/source/dialog/paraprev \
    svx/source/dialog/passwd \
    svx/source/dialog/relfld \
    svx/source/dialog/rlrcitem \
    svx/source/dialog/rubydialog \
    svx/source/dialog/rulritem \
    svx/source/dialog/SafeModeDialog \
    svx/source/dialog/FileExportedDialog \
    svx/source/dialog/SafeModeUI \
    svx/source/dialog/SpellDialogChildWindow \
    svx/source/dialog/srchctrl \
    svx/source/dialog/srchdlg \
    svx/source/dialog/strarray \
    svx/source/dialog/svxbmpnumvalueset \
    svx/source/dialog/svxgraphicitem \
    svx/source/dialog/svxruler \
    svx/source/dialog/swframeexample \
    svx/source/dialog/swframeposstrings \
    svx/source/dialog/ThemeColorValueSet \
    svx/source/dialog/ThemeDialog \
    svx/source/dialog/ThemeColorEditDialog \
    svx/source/dialog/txencbox \
    svx/source/dialog/txenctab \
    svx/source/dialog/weldeditview \
    svx/source/dialog/signaturelinehelper \
    svx/source/engine3d/float3d \
    svx/source/fmcomp/dbaobjectex \
    svx/source/form/databaselocationinput \
    svx/source/form/dbcharsethelper \
    $(call gb_Helper_optional,DBCONNECTIVITY,svx/source/form/filtnav) \
    svx/source/form/fmobjfac \
    svx/source/form/fmPropBrw \
    svx/source/form/fmsrccfg \
    svx/source/form/fmsrcimp \
    svx/source/form/tabwin \
    svx/source/form/tbxform \
    svx/source/items/algitem \
    svx/source/items/autoformathelper \
	svx/source/items/ehdl \
    svx/source/items/hlnkitem \
    svx/source/items/numfmtsh \
    svx/source/items/legacyitem \
    svx/source/items/numinf \
    svx/source/items/ofaitem \
    svx/source/items/pageitem \
    svx/source/items/postattr \
    svx/source/items/rotmodit \
    svx/source/items/SmartTagItem \
    svx/source/items/statusitem \
    svx/source/items/svxerr \
    svx/source/items/viewlayoutitem \
    svx/source/items/zoomslideritem \
    svx/source/mnuctrls/clipboardctl \
    svx/source/mnuctrls/smarttagmenu \
    svx/source/sidebar/ContextChangeEventMultiplexer \
    svx/source/sidebar/EmptyPanel \
    svx/source/sidebar/inspector/InspectorTextPanel \
    svx/source/sidebar/nbdtmg \
    svx/source/sidebar/nbdtmgfact	\
    svx/source/sidebar/PanelFactory \
    svx/source/sidebar/SelectionAnalyzer \
    svx/source/sidebar/SelectionChangeHandler \
    svx/source/sidebar/text/TextCharacterSpacingControl \
    svx/source/sidebar/text/TextCharacterSpacingPopup \
    svx/source/sidebar/text/TextUnderlineControl \
    svx/source/sidebar/text/TextUnderlinePopup \
    svx/source/sidebar/text/TextPropertyPanel \
    svx/source/sidebar/styles/StylesPropertyPanel \
    svx/source/sidebar/lists/ListsPropertyPanel \
    svx/source/sidebar/paragraph/ParaLineSpacingControl \
    svx/source/sidebar/paragraph/ParaLineSpacingPopup \
    svx/source/sidebar/paragraph/ParaPropertyPanel \
    svx/source/sidebar/paragraph/ParaSpacingWindow \
    svx/source/sidebar/paragraph/ParaSpacingControl \
    svx/source/sidebar/area/AreaPropertyPanel \
    svx/source/sidebar/area/AreaPropertyPanelBase \
    svx/source/sidebar/area/AreaTransparencyGradientPopup \
    svx/source/sidebar/effect/EffectPropertyPanel \
    svx/source/sidebar/effect/TextEffectPropertyPanel \
    svx/source/sidebar/fontwork/FontworkPropertyPanel \
    svx/source/sidebar/shadow/ShadowPropertyPanel \
    svx/source/sidebar/graphic/GraphicPropertyPanel \
    svx/source/sidebar/line/LinePropertyPanel \
    svx/source/sidebar/line/LinePropertyPanelBase \
    svx/source/sidebar/line/LineWidthValueSet \
    svx/source/sidebar/line/LineWidthPopup \
    $(call gb_Helper_optional,AVMEDIA,svx/source/sidebar/media/MediaPlaybackPanel) \
    svx/source/sidebar/possize/PosSizePropertyPanel \
    svx/source/sidebar/shapes/DefaultShapesPanel \
    svx/source/sidebar/shapes/ShapesUtil \
    svx/source/sidebar/textcolumns/TextColumnsPropertyPanel \
    svx/source/sidebar/tools/ValueSetWithTextControl \
    svx/source/stbctrls/pszctrl \
    svx/source/stbctrls/insctrl \
    svx/source/stbctrls/selctrl \
    svx/source/stbctrls/xmlsecctrl \
    svx/source/stbctrls/modctrl \
    svx/source/stbctrls/zoomsliderctrl \
    svx/source/stbctrls/zoomctrl \
    svx/source/svdraw/ActionDescriptionProvider \
    svx/source/svdraw/MediaShellHelpers \
    svx/source/smarttags/SmartTagMgr \
    svx/source/table/accessiblecell \
    svx/source/table/accessibletableshape \
    svx/source/table/tabledesign \
    svx/source/table/tablertfexporter \
    svx/source/table/tablertfimporter \
	svx/source/table/tablehtmlimporter \
    svx/source/tbxctrls/bulletsnumbering \
    svx/source/tbxctrls/colrctrl \
    svx/source/tbxctrls/SvxColorChildWindow \
    svx/source/tbxctrls/fillctrl \
    svx/source/tbxctrls/formatpaintbrushctrl \
    svx/source/tbxctrls/grafctrl \
    svx/source/tbxctrls/itemwin \
    svx/source/tbxctrls/layctrl \
    svx/source/tbxctrls/lboxctrl \
    svx/source/tbxctrls/linewidthctrl \
    svx/source/tbxctrls/tbunocontroller \
    svx/source/tbxctrls/tbunosearchcontrollers \
    svx/source/tbxctrls/tbxcolor \
    svx/source/tbxctrls/tbxdrctl \
    svx/source/tbxctrls/verttexttbxctrl \
    svx/source/uitest/uiobject \
    svx/source/unodraw/unoctabl \
    svx/source/unodraw/UnoNamespaceMap \
    svx/source/unodraw/unopool \
    svx/source/unodraw/unoshcol \
    svx/source/unogallery/unogalitem \
    svx/source/unogallery/unogaltheme \
    svx/source/unogallery/unogalthemeprovider \
))

ifeq ($(OS),WNT)
$(eval $(call gb_Library_use_system_win32_libs,svx,\
    advapi32 \
))
endif

# vim: set noet sw=4 ts=4:
