# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_UIConfig_UIConfig,modules/simpress))

$(eval $(call gb_UIConfig_add_menubarfiles,modules/simpress,\
	sd/uiconfig/simpress/menubar/menubar \
))

$(eval $(call gb_UIConfig_add_popupmenufiles,modules/simpress,\
	sd/uiconfig/simpress/popupmenu/3dobject \
	sd/uiconfig/simpress/popupmenu/3dscene2 \
	sd/uiconfig/simpress/popupmenu/3dscene \
	sd/uiconfig/simpress/popupmenu/annotation \
	sd/uiconfig/simpress/popupmenu/bezier \
	sd/uiconfig/simpress/popupmenu/connector \
	sd/uiconfig/simpress/popupmenu/curve \
	sd/uiconfig/simpress/popupmenu/drawtext \
	sd/uiconfig/simpress/popupmenu/draw \
	sd/uiconfig/simpress/popupmenu/form \
	sd/uiconfig/simpress/popupmenu/formrichtext \
	sd/uiconfig/simpress/popupmenu/gluepoint \
	sd/uiconfig/simpress/popupmenu/graphic \
	sd/uiconfig/simpress/popupmenu/group \
	sd/uiconfig/simpress/popupmenu/line \
	sd/uiconfig/simpress/popupmenu/measure \
	sd/uiconfig/simpress/popupmenu/media \
	sd/uiconfig/simpress/popupmenu/multiselect \
	sd/uiconfig/simpress/popupmenu/notebookbar \
	sd/uiconfig/simpress/popupmenu/objectalign \
	sd/uiconfig/simpress/popupmenu/oleobject \
	sd/uiconfig/simpress/popupmenu/outline \
	sd/uiconfig/simpress/popupmenu/pagepanemaster \
	sd/uiconfig/simpress/popupmenu/pagepanenoselmaster \
	sd/uiconfig/simpress/popupmenu/pagepanenosel \
	sd/uiconfig/simpress/popupmenu/pagepane \
	sd/uiconfig/simpress/popupmenu/pagetab \
	sd/uiconfig/simpress/popupmenu/page \
	sd/uiconfig/simpress/popupmenu/table \
	sd/uiconfig/simpress/popupmenu/textbox \
))

$(eval $(call gb_UIConfig_add_statusbarfiles,modules/simpress,\
	sd/uiconfig/simpress/statusbar/statusbar \
))

$(eval $(call gb_UIConfig_add_toolbarfiles,modules/simpress,\
	sd/uiconfig/simpress/toolbar/3dobjectsbar \
	sd/uiconfig/simpress/toolbar/alignmentbar \
	sd/uiconfig/simpress/toolbar/arrowsbar \
	sd/uiconfig/simpress/toolbar/arrowshapes \
	sd/uiconfig/simpress/toolbar/basicshapes \
	sd/uiconfig/simpress/toolbar/bezierobjectbar \
	sd/uiconfig/simpress/toolbar/calloutshapes \
	sd/uiconfig/simpress/toolbar/choosemodebar \
	sd/uiconfig/simpress/toolbar/classificationbar \
	sd/uiconfig/simpress/toolbar/colorbar \
	sd/uiconfig/simpress/toolbar/commentsbar \
	sd/uiconfig/simpress/toolbar/commontaskbar \
	sd/uiconfig/simpress/toolbar/connectorsbar \
	sd/uiconfig/simpress/toolbar/drawingobjectbar \
	sd/uiconfig/simpress/toolbar/distributebar \
	sd/uiconfig/simpress/toolbar/ellipsesbar \
	sd/uiconfig/simpress/toolbar/extrusionobjectbar \
	sd/uiconfig/simpress/toolbar/findbar \
	sd/uiconfig/simpress/toolbar/flowchartshapes \
	sd/uiconfig/simpress/toolbar/fontworkobjectbar \
	sd/uiconfig/simpress/toolbar/fontworkshapetype \
	sd/uiconfig/simpress/toolbar/formcontrols \
	sd/uiconfig/simpress/toolbar/formdesign \
	sd/uiconfig/simpress/toolbar/formsfilterbar \
	sd/uiconfig/simpress/toolbar/formsnavigationbar \
	sd/uiconfig/simpress/toolbar/formtextobjectbar \
	sd/uiconfig/simpress/toolbar/fullscreenbar \
	sd/uiconfig/simpress/toolbar/gluepointsobjectbar \
	sd/uiconfig/simpress/toolbar/graffilterbar \
	sd/uiconfig/simpress/toolbar/graphicobjectbar \
	sd/uiconfig/simpress/toolbar/insertbar \
	sd/uiconfig/simpress/toolbar/linesbar \
	sd/uiconfig/simpress/toolbar/masterviewtoolbar \
	sd/uiconfig/simpress/toolbar/mediaobjectbar \
	sd/uiconfig/simpress/toolbar/notebookbarshortcuts \
	sd/uiconfig/simpress/toolbar/optimizetablebar \
	sd/uiconfig/simpress/toolbar/optionsbar \
	sd/uiconfig/simpress/toolbar/outlinetoolbar \
	sd/uiconfig/simpress/toolbar/positionbar \
	sd/uiconfig/simpress/toolbar/rectanglesbar \
	sd/uiconfig/simpress/toolbar/slideviewobjectbar \
	sd/uiconfig/simpress/toolbar/slideviewtoolbar \
	sd/uiconfig/simpress/toolbar/singlemode \
	sd/uiconfig/simpress/toolbar/standardbar \
	sd/uiconfig/simpress/toolbar/starshapes \
	sd/uiconfig/simpress/toolbar/symbolshapes \
	sd/uiconfig/simpress/toolbar/tableobjectbar \
	sd/uiconfig/simpress/toolbar/textbar \
	sd/uiconfig/simpress/toolbar/textobjectbar \
	sd/uiconfig/simpress/toolbar/toolbar \
	sd/uiconfig/simpress/toolbar/viewerbar \
	sd/uiconfig/simpress/toolbar/zoombar \
))

$(eval $(call gb_UIConfig_add_uifiles,modules/simpress,\
	sd/uiconfig/simpress/ui/annotation \
	sd/uiconfig/simpress/ui/annotationtagmenu \
	sd/uiconfig/simpress/ui/clientboxfragment \
	sd/uiconfig/simpress/ui/currentmastermenu \
	sd/uiconfig/simpress/ui/customanimationspanel \
	sd/uiconfig/simpress/ui/customanimationproperties \
	sd/uiconfig/simpress/ui/customanimationeffecttab \
	sd/uiconfig/simpress/ui/customanimationfragment \
	sd/uiconfig/simpress/ui/customanimationtimingtab \
	sd/uiconfig/simpress/ui/customanimationtexttab \
	sd/uiconfig/simpress/ui/customslideshows \
	sd/uiconfig/simpress/ui/definecustomslideshow \
	sd/uiconfig/simpress/ui/displaywindow \
	sd/uiconfig/simpress/ui/dlgfield \
	sd/uiconfig/simpress/ui/dockinganimation \
	sd/uiconfig/simpress/ui/effectmenu \
	sd/uiconfig/simpress/ui/fieldmenu \
	sd/uiconfig/simpress/ui/fontsizemenu \
	sd/uiconfig/simpress/ui/fontstylemenu \
	sd/uiconfig/simpress/ui/gluebox \
	sd/uiconfig/simpress/ui/headerfooterdialog \
	sd/uiconfig/simpress/ui/headerfootertab \
	sd/uiconfig/simpress/ui/impressprinteroptions \
	sd/uiconfig/simpress/ui/insertslides \
	sd/uiconfig/simpress/ui/interactiondialog \
	sd/uiconfig/simpress/ui/interactionpage \
	sd/uiconfig/simpress/ui/layoutmenu \
	sd/uiconfig/simpress/ui/layoutpanel \
	sd/uiconfig/simpress/ui/layoutwindow \
	sd/uiconfig/simpress/ui/masterlayoutdlg \
	sd/uiconfig/simpress/ui/mastermenu \
	sd/uiconfig/simpress/ui/masterpagemenu \
	sd/uiconfig/simpress/ui/masterpagepanel \
	sd/uiconfig/simpress/ui/masterpagepanelall \
	sd/uiconfig/simpress/ui/masterpagepanelrecent \
	sd/uiconfig/simpress/ui/navigatorpanel \
	sd/uiconfig/simpress/ui/notebookbar \
	sd/uiconfig/simpress/ui/notebookbar_compact \
	sd/uiconfig/simpress/ui/notebookbar_single \
	sd/uiconfig/simpress/ui/notebookbar_groups \
	sd/uiconfig/simpress/ui/notebookbar_groupedbar_full \
	sd/uiconfig/simpress/ui/notebookbar_groupedbar_compact \
	sd/uiconfig/simpress/ui/notebookbar_online \
	sd/uiconfig/simpress/ui/noteschildwindow \
	sd/uiconfig/simpress/ui/notespanelcontextmenu \
	sd/uiconfig/simpress/ui/optimpressgeneralpage \
	sd/uiconfig/simpress/ui/pagesfieldbox \
	sd/uiconfig/simpress/ui/photoalbum \
	sd/uiconfig/simpress/ui/pmimagespage \
	sd/uiconfig/simpress/ui/pminfodialog \
	sd/uiconfig/simpress/ui/pmintropage \
	sd/uiconfig/simpress/ui/pmobjectspage \
	sd/uiconfig/simpress/ui/pmslidespage \
	sd/uiconfig/simpress/ui/pmsummarypage \
	sd/uiconfig/simpress/ui/presentationdialog \
	sd/uiconfig/simpress/ui/prntopts \
	sd/uiconfig/simpress/ui/remotedialog \
	sd/uiconfig/simpress/ui/rotatemenu \
	sd/uiconfig/simpress/ui/scalemenu \
	sd/uiconfig/simpress/ui/sdviewpage \
	sd/uiconfig/simpress/ui/sidebarslidebackground \
	sd/uiconfig/simpress/ui/slidecontextmenu \
	sd/uiconfig/simpress/ui/slidedesigndialog \
	sd/uiconfig/simpress/ui/slidetransitionspanel \
	sd/uiconfig/simpress/ui/snapmenu \
	sd/uiconfig/simpress/ui/tabviewbar \
	sd/uiconfig/simpress/ui/tabledesignpanel \
	sd/uiconfig/simpress/ui/templatedialog \
))

# vim: set noet sw=4 ts=4:
