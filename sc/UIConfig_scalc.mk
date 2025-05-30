# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UIConfig_UIConfig,modules/scalc))

$(eval $(call gb_UIConfig_add_menubarfiles,modules/scalc,\
	sc/uiconfig/scalc/menubar/menubar \
))

$(eval $(call gb_UIConfig_add_popupmenufiles,modules/scalc,\
	sc/uiconfig/scalc/popupmenu/anchor \
	sc/uiconfig/scalc/popupmenu/audit \
	sc/uiconfig/scalc/popupmenu/cell \
	sc/uiconfig/scalc/popupmenu/celledit \
	sc/uiconfig/scalc/popupmenu/colheader \
	sc/uiconfig/scalc/popupmenu/column_operations \
	sc/uiconfig/scalc/popupmenu/conditional \
	sc/uiconfig/scalc/popupmenu/conditional_easy \
	sc/uiconfig/scalc/popupmenu/draw \
	sc/uiconfig/scalc/popupmenu/drawtext \
	sc/uiconfig/scalc/popupmenu/form \
	sc/uiconfig/scalc/popupmenu/formrichtext \
	sc/uiconfig/scalc/popupmenu/formulabar \
	sc/uiconfig/scalc/popupmenu/freezepanes \
	sc/uiconfig/scalc/popupmenu/graphic \
	sc/uiconfig/scalc/popupmenu/media \
	sc/uiconfig/scalc/popupmenu/notebookbar \
	sc/uiconfig/scalc/popupmenu/oleobject \
	sc/uiconfig/scalc/popupmenu/pagebreak \
	sc/uiconfig/scalc/popupmenu/pivot \
	sc/uiconfig/scalc/popupmenu/preview \
	sc/uiconfig/scalc/popupmenu/printranges \
	sc/uiconfig/scalc/popupmenu/rowheader \
	sc/uiconfig/scalc/popupmenu/row_operations \
	sc/uiconfig/scalc/popupmenu/sheettab \
	sc/uiconfig/scalc/popupmenu/sparkline \
	sc/uiconfig/scalc/popupmenu/sparklinemenu \
	sc/uiconfig/scalc/popupmenu/statisticsmenu \
))

$(eval $(call gb_UIConfig_add_statusbarfiles,modules/scalc,\
	sc/uiconfig/scalc/statusbar/statusbar \
))

$(eval $(call gb_UIConfig_add_toolbarfiles,modules/scalc,\
	sc/uiconfig/scalc/toolbar/alignmentbar \
	sc/uiconfig/scalc/toolbar/arrowsbar \
	sc/uiconfig/scalc/toolbar/arrowshapes \
	sc/uiconfig/scalc/toolbar/basicshapes \
	sc/uiconfig/scalc/toolbar/calloutshapes \
	sc/uiconfig/scalc/toolbar/classificationbar \
	sc/uiconfig/scalc/toolbar/colorbar \
	sc/uiconfig/scalc/toolbar/datastreams \
	sc/uiconfig/scalc/toolbar/drawbar \
	sc/uiconfig/scalc/toolbar/drawobjectbar \
	sc/uiconfig/scalc/toolbar/extrusionobjectbar \
	sc/uiconfig/scalc/toolbar/findbar \
	sc/uiconfig/scalc/toolbar/flowchartshapes \
	sc/uiconfig/scalc/toolbar/fontworkobjectbar \
	sc/uiconfig/scalc/toolbar/fontworkshapetype \
	sc/uiconfig/scalc/toolbar/formatobjectbar \
	sc/uiconfig/scalc/toolbar/formcontrols \
	sc/uiconfig/scalc/toolbar/formdesign \
	sc/uiconfig/scalc/toolbar/formsfilterbar \
	sc/uiconfig/scalc/toolbar/formsnavigationbar \
	sc/uiconfig/scalc/toolbar/formtextobjectbar \
	sc/uiconfig/scalc/toolbar/fullscreenbar \
	sc/uiconfig/scalc/toolbar/graffilterbar \
	sc/uiconfig/scalc/toolbar/graphicobjectbar \
	sc/uiconfig/scalc/toolbar/insertbar \
	sc/uiconfig/scalc/toolbar/insertcellsbar \
	sc/uiconfig/scalc/toolbar/linesbar \
	sc/uiconfig/scalc/toolbar/mediaobjectbar \
	sc/uiconfig/scalc/toolbar/notebookbarshortcuts \
	sc/uiconfig/scalc/toolbar/previewbar \
	sc/uiconfig/scalc/toolbar/singlemode \
	sc/uiconfig/scalc/toolbar/standardbar \
	sc/uiconfig/scalc/toolbar/starshapes \
	sc/uiconfig/scalc/toolbar/symbolshapes \
	sc/uiconfig/scalc/toolbar/textobjectbar \
	sc/uiconfig/scalc/toolbar/toolbar \
	sc/uiconfig/scalc/toolbar/viewerbar \
))

$(eval $(call gb_UIConfig_add_uifiles,modules/scalc,\
	sc/uiconfig/scalc/ui/advancedfilterdialog \
	sc/uiconfig/scalc/ui/allheaderfooterdialog \
	sc/uiconfig/scalc/ui/analysisofvariancedialog \
	sc/uiconfig/scalc/ui/autoformattable \
	sc/uiconfig/scalc/ui/autosum \
	sc/uiconfig/scalc/ui/cellprotectionpage \
	sc/uiconfig/scalc/ui/duplicaterecordsdlg \
	sc/uiconfig/scalc/ui/changesourcedialog \
	sc/uiconfig/scalc/ui/chardialog \
	sc/uiconfig/scalc/ui/checkwarningdialog \
	sc/uiconfig/scalc/ui/chisquaretestdialog \
	sc/uiconfig/scalc/ui/colormenu \
	sc/uiconfig/scalc/ui/colorrowdialog \
	sc/uiconfig/scalc/ui/colwidthdialog \
	sc/uiconfig/scalc/ui/condformatmanager \
	sc/uiconfig/scalc/ui/conditionaleasydialog \
	sc/uiconfig/scalc/ui/conditionalformatdialog \
	sc/uiconfig/scalc/ui/conditionalentry \
	sc/uiconfig/scalc/ui/conditionaliconset \
	sc/uiconfig/scalc/ui/conflictsdialog \
	sc/uiconfig/scalc/ui/consolidatedialog \
	sc/uiconfig/scalc/ui/correlationdialog \
	sc/uiconfig/scalc/ui/covariancedialog \
	sc/uiconfig/scalc/ui/createnamesdialog \
	sc/uiconfig/scalc/ui/dapiservicedialog \
	sc/uiconfig/scalc/ui/databaroptions \
	sc/uiconfig/scalc/ui/datafielddialog \
	sc/uiconfig/scalc/ui/datafieldoptionsdialog \
	sc/uiconfig/scalc/ui/dataform \
	sc/uiconfig/scalc/ui/dataformfragment \
	sc/uiconfig/scalc/ui/datastreams \
	sc/uiconfig/scalc/ui/dataproviderdlg \
	sc/uiconfig/scalc/ui/definedatabaserangedialog \
	sc/uiconfig/scalc/ui/definename \
	sc/uiconfig/scalc/ui/deletecells \
	sc/uiconfig/scalc/ui/deletecolumnentry \
	sc/uiconfig/scalc/ui/deletecontents \
	sc/uiconfig/scalc/ui/descriptivestatisticsdialog \
	sc/uiconfig/scalc/ui/drawtemplatedialog \
	sc/uiconfig/scalc/ui/dropmenu \
	sc/uiconfig/scalc/ui/doubledialog \
	sc/uiconfig/scalc/ui/erroralerttabpage \
	sc/uiconfig/scalc/ui/externaldata \
	sc/uiconfig/scalc/ui/exponentialsmoothingdialog \
	sc/uiconfig/scalc/ui/filldlg \
	sc/uiconfig/scalc/ui/filterlist \
	sc/uiconfig/scalc/ui/filterdropdown \
	sc/uiconfig/scalc/ui/filtersubdropdown \
	sc/uiconfig/scalc/ui/footerdialog \
	sc/uiconfig/scalc/ui/formatcellsdialog \
	sc/uiconfig/scalc/ui/formulacalculationoptions \
	sc/uiconfig/scalc/ui/fourieranalysisdialog \
	sc/uiconfig/scalc/ui/floatingborderstyle \
	sc/uiconfig/scalc/ui/floatinglinestyle \
	sc/uiconfig/scalc/ui/functionpanel \
	sc/uiconfig/scalc/ui/goalseekdlg \
	sc/uiconfig/scalc/ui/gotosheetdialog \
	sc/uiconfig/scalc/ui/groupdialog \
	sc/uiconfig/scalc/ui/groupbydate \
	sc/uiconfig/scalc/ui/groupbynumber \
	sc/uiconfig/scalc/ui/headerdialog \
	sc/uiconfig/scalc/ui/headerfootercontent \
	sc/uiconfig/scalc/ui/headerfooterdialog \
	sc/uiconfig/scalc/ui/imoptdialog \
	sc/uiconfig/scalc/ui/inputbar \
	sc/uiconfig/scalc/ui/inputstringdialog \
	sc/uiconfig/scalc/ui/insertcells \
	sc/uiconfig/scalc/ui/insertname \
	sc/uiconfig/scalc/ui/insertsheet \
	sc/uiconfig/scalc/ui/integerdialog \
	sc/uiconfig/scalc/ui/leftfooterdialog \
	sc/uiconfig/scalc/ui/leftheaderdialog \
	sc/uiconfig/scalc/ui/listmenu \
	sc/uiconfig/scalc/ui/namerangesdialog \
	sc/uiconfig/scalc/ui/notebookbar \
	sc/uiconfig/scalc/ui/notebookbar_compact \
	sc/uiconfig/scalc/ui/notebookbar_groups \
	sc/uiconfig/scalc/ui/notebookbar_groupedbar_full \
	sc/uiconfig/scalc/ui/notebookbar_groupedbar_compact \
	sc/uiconfig/scalc/ui/notebookbar_online \
	sc/uiconfig/scalc/ui/numberbox \
	sc/uiconfig/scalc/ui/managenamesdialog \
	sc/uiconfig/scalc/ui/mergecellsdialog \
	sc/uiconfig/scalc/ui/mergecolumnentry \
	sc/uiconfig/scalc/ui/texttransformationentry \
	sc/uiconfig/scalc/ui/sorttransformationentry \
	sc/uiconfig/scalc/ui/aggregatefunctionentry \
	sc/uiconfig/scalc/ui/numbertransformationentry \
	sc/uiconfig/scalc/ui/replacenulltransformationentry \
	sc/uiconfig/scalc/ui/datetimetransformationentry \
	sc/uiconfig/scalc/ui/findreplaceentry \
	sc/uiconfig/scalc/ui/deleterowentry \
	sc/uiconfig/scalc/ui/swaprowsentry \
	sc/uiconfig/scalc/ui/movecopysheet \
	sc/uiconfig/scalc/ui/movingaveragedialog \
	sc/uiconfig/scalc/ui/multipleoperationsdialog \
	sc/uiconfig/scalc/ui/navigatorpanel \
	sc/uiconfig/scalc/ui/nosolutiondialog \
	sc/uiconfig/scalc/ui/onlyactivesheetsaveddialog \
	sc/uiconfig/scalc/ui/optcalculatepage \
	sc/uiconfig/scalc/ui/optchangespage \
	sc/uiconfig/scalc/ui/optcompatibilitypage \
	sc/uiconfig/scalc/ui/optdefaultpage \
	sc/uiconfig/scalc/ui/optdlg \
	sc/uiconfig/scalc/ui/optformula \
	sc/uiconfig/scalc/ui/optimalcolwidthdialog \
	sc/uiconfig/scalc/ui/optimalrowheightdialog \
	sc/uiconfig/scalc/ui/optsortlists \
	sc/uiconfig/scalc/ui/pagelistmenu \
	sc/uiconfig/scalc/ui/pagetemplatedialog \
	sc/uiconfig/scalc/ui/pastespecial \
	sc/uiconfig/scalc/ui/paradialog \
	sc/uiconfig/scalc/ui/paratemplatedialog \
	sc/uiconfig/scalc/ui/passfragment \
	sc/uiconfig/scalc/ui/pivotfielddialog \
	sc/uiconfig/scalc/ui/pivotfilterdialog \
	sc/uiconfig/scalc/ui/pivottablelayoutdialog \
	sc/uiconfig/scalc/ui/posbox \
	sc/uiconfig/scalc/ui/printareasdialog \
	sc/uiconfig/scalc/ui/printeroptions \
	sc/uiconfig/scalc/ui/protectsheetdlg \
	sc/uiconfig/scalc/ui/queryrunstreamscriptdialog \
	sc/uiconfig/scalc/ui/randomnumbergenerator \
	sc/uiconfig/scalc/ui/recalcquerydialog \
	sc/uiconfig/scalc/ui/regressiondialog \
	sc/uiconfig/scalc/ui/retypepassdialog \
	sc/uiconfig/scalc/ui/retypepassworddialog \
	sc/uiconfig/scalc/ui/rightfooterdialog \
	sc/uiconfig/scalc/ui/rightheaderdialog \
	sc/uiconfig/scalc/ui/rowheightdialog \
	sc/uiconfig/scalc/ui/samplingdialog \
	sc/uiconfig/scalc/ui/standardfilterdialog \
	sc/uiconfig/scalc/ui/scenariodialog \
	sc/uiconfig/scalc/ui/scenariomenu \
	sc/uiconfig/scalc/ui/scgeneralpage \
	sc/uiconfig/scalc/ui/searchresults \
	sc/uiconfig/scalc/ui/selectdatasource \
	sc/uiconfig/scalc/ui/selectrange \
	sc/uiconfig/scalc/ui/selectsource \
	sc/uiconfig/scalc/ui/sheetprintpage \
	sc/uiconfig/scalc/ui/sharedocumentdlg \
	sc/uiconfig/scalc/ui/sharedfirstfooterdialog \
	sc/uiconfig/scalc/ui/sharedfirstheaderdialog \
	sc/uiconfig/scalc/ui/sharedleftfooterdialog \
	sc/uiconfig/scalc/ui/sharedleftheaderdialog \
	sc/uiconfig/scalc/ui/sharedfooterdialog \
	sc/uiconfig/scalc/ui/sharedheaderdialog \
	sc/uiconfig/scalc/ui/sharedwarningdialog \
	sc/uiconfig/scalc/ui/showchangesdialog \
	sc/uiconfig/scalc/ui/showdetaildialog \
	sc/uiconfig/scalc/ui/showsheetdialog \
	sc/uiconfig/scalc/ui/sidebaralignment \
	sc/uiconfig/scalc/ui/sidebarnumberformat \
	sc/uiconfig/scalc/ui/sidebarcellappearance \
	sc/uiconfig/scalc/ui/simplerefdialog \
	sc/uiconfig/scalc/ui/solverdlg \
	sc/uiconfig/scalc/ui/solveroptionsdialog \
	sc/uiconfig/scalc/ui/solverprogressdialog \
	sc/uiconfig/scalc/ui/solversuccessdialog \
	sc/uiconfig/scalc/ui/sortcriteriapage \
	sc/uiconfig/scalc/ui/sortdialog \
	sc/uiconfig/scalc/ui/sortkey \
	sc/uiconfig/scalc/ui/sortoptionspage \
	sc/uiconfig/scalc/ui/sortwarning \
	sc/uiconfig/scalc/ui/sparklinedialog \
	sc/uiconfig/scalc/ui/sparklinedatarangedialog \
	sc/uiconfig/scalc/ui/splitcolumnentry \
	sc/uiconfig/scalc/ui/subtotaldialog \
	sc/uiconfig/scalc/ui/subtotaloptionspage \
	sc/uiconfig/scalc/ui/subtotalgrppage \
	sc/uiconfig/scalc/ui/statisticsinfopage \
	sc/uiconfig/scalc/ui/tabcolordialog \
	sc/uiconfig/scalc/ui/textimportoptions \
	sc/uiconfig/scalc/ui/textimportcsv \
	sc/uiconfig/scalc/ui/tpviewpage \
	sc/uiconfig/scalc/ui/ttestdialog \
	sc/uiconfig/scalc/ui/ungroupdialog \
	sc/uiconfig/scalc/ui/validationdialog \
	sc/uiconfig/scalc/ui/validationcriteriapage \
	sc/uiconfig/scalc/ui/validationhelptabpage \
	sc/uiconfig/scalc/ui/warnautocorrect \
	sc/uiconfig/scalc/ui/xmlsourcedialog \
	sc/uiconfig/scalc/ui/zoombox \
	sc/uiconfig/scalc/ui/ztestdialog \
))

# vim: set noet sw=4 ts=4:
