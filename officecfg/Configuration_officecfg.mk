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

# The name "registry" needs to match gb_Configuration_PRIMARY_REGISTRY_NAME in
# solenv/gbuild/Configuration.mk:
$(eval $(call gb_Configuration_Configuration,registry))

$(eval $(call gb_Configuration_add_schemas,registry,officecfg/registry/schema,\
	$(addprefix org/openoffice/,$(addsuffix .xcs,$(officecfg_XCSFILES))) \
))

$(eval $(call gb_Configuration_add_datas,registry,officecfg/registry/data,\
	org/openoffice/Inet.xcu \
	org/openoffice/System.xcu \
	org/openoffice/UserProfile.xcu \
	org/openoffice/VCL.xcu \
	org/openoffice/Interaction.xcu \
	org/openoffice/Office/Calc.xcu \
	org/openoffice/Office/BasicIDE.xcu \
	org/openoffice/Office/Canvas.xcu \
	org/openoffice/Office/Compatibility.xcu \
	org/openoffice/Office/ExtensionDependencies.xcu \
	org/openoffice/Office/ExtensionManager.xcu \
	org/openoffice/Office/Impress.xcu \
	org/openoffice/Office/Jobs.xcu \
	org/openoffice/Office/Logging.xcu \
	org/openoffice/Office/Math.xcu \
	org/openoffice/Office/ProtocolHandler.xcu \
	org/openoffice/Office/Security.xcu \
	org/openoffice/Office/Views.xcu \
	org/openoffice/Office/Paths.xcu \
	org/openoffice/Office/Histories.xcu \
	org/openoffice/Office/ReportDesign.xcu \
	org/openoffice/Office/UI/Controller.xcu \
	org/openoffice/Office/UI/Factories.xcu \
	org/openoffice/TypeDetection/UISort.xcu \
	org/openoffice/ucb/Configuration.xcu \
))

$(eval $(call gb_Configuration_add_spool_modules,registry,officecfg/registry/data,\
	org/openoffice/Inet-macosx.xcu \
	org/openoffice/Inet-unixdesktop.xcu \
	org/openoffice/Inet-wnt.xcu \
	org/openoffice/Setup-writer.xcu \
	org/openoffice/Setup-calc.xcu \
	org/openoffice/Setup-draw.xcu \
	org/openoffice/Setup-impress.xcu \
	org/openoffice/Setup-base.xcu \
	org/openoffice/Setup-math.xcu \
	org/openoffice/Setup-report.xcu \
	$(call gb_Helper_optional,REPORTBUILDER, \
		org/openoffice/Setup-reportbuilder.xcu \
	) \
	org/openoffice/Setup-start.xcu \
	org/openoffice/UserProfile-unixdesktop.xcu \
	org/openoffice/VCL-unixdesktop.xcu \
	org/openoffice/Office/Accelerators-macosx.xcu \
	$(call gb_Helper_optional,REPORTBUILDER, \
		org/openoffice/Office/Accelerators-reportbuilder.xcu \
	) \
	org/openoffice/Office/Accelerators-unxwnt.xcu \
	$(call gb_Helper_optional,LIBRELOGO, \
		org/openoffice/Office/Addons-librelogo.xcu \
	) \
	org/openoffice/Office/Common-writer.xcu \
	org/openoffice/Office/Common-calc.xcu \
	org/openoffice/Office/Common-draw.xcu \
	org/openoffice/Office/Common-impress.xcu \
	org/openoffice/Office/Common-base.xcu \
	org/openoffice/Office/Common-math.xcu \
	org/openoffice/Office/Common-unx.xcu \
	org/openoffice/Office/Common-unixdesktop.xcu \
	org/openoffice/Office/Common-macosx.xcu \
	org/openoffice/Office/Common-macosxsandbox.xcu \
	org/openoffice/Office/Common-wnt.xcu \
	org/openoffice/Office/Common-UseOOoFileDialogs.xcu \
	org/openoffice/Office/Common-32bit.xcu \
	org/openoffice/Office/Jobs-impress.xcu \
	org/openoffice/Office/ProtocolHandler-impress.xcu \
	org/openoffice/Office/Common-cjk.xcu \
	org/openoffice/Office/Common-ctl.xcu \
	org/openoffice/Office/Common-ctlseqcheck.xcu \
	org/openoffice/Office/DataAccess-evoab2.xcu \
	$(call gb_Helper_optional,REPORTBUILDER, \
		org/openoffice/Office/DataAccess-reportbuilder.xcu \
	) \
	org/openoffice/Office/Paths-macosx.xcu \
	org/openoffice/Office/Paths-unxwnt.xcu \
	org/openoffice/Office/Paths-unixdesktop.xcu \
	org/openoffice/Office/Paths-internallibexttextcatdata.xcu \
	org/openoffice/Office/Paths-externallibexttextcatdata.xcu \
	org/openoffice/Office/Paths-internallibnumbertextdata.xcu \
	org/openoffice/Office/Paths-externallibnumbertextdata.xcu \
	org/openoffice/Office/Writer-cjk.xcu \
	org/openoffice/Office/Impress-ogltrans.xcu \
	org/openoffice/Office/Embedding-calc.xcu \
	org/openoffice/Office/Embedding-chart.xcu \
	org/openoffice/Office/Embedding-draw.xcu \
	org/openoffice/Office/Embedding-impress.xcu \
	org/openoffice/Office/Embedding-math.xcu \
	org/openoffice/Office/Embedding-base.xcu \
	$(call gb_Helper_optional,REPORTBUILDER, \
		org/openoffice/Office/Embedding-reportbuilder.xcu \
	) \
	org/openoffice/Office/Embedding-writer.xcu \
	$(call gb_Helper_optional,LIBRELOGO, \
		org/openoffice/Office/UI/WriterCommands-librelogo.xcu \
		org/openoffice/Office/UI/WriterWindowState-librelogo.xcu \
	) \
	$(call gb_Helper_optional,REPORTBUILDER, \
		org/openoffice/Office/UI/Controller-reportbuilder.xcu \
	) \
	org/openoffice/Office/UI/Infobar-macosxsandbox.xcu \
	org/openoffice/TypeDetection/UISort-writer.xcu \
	org/openoffice/TypeDetection/UISort-calc.xcu \
	org/openoffice/TypeDetection/UISort-draw.xcu \
	org/openoffice/TypeDetection/UISort-impress.xcu \
	org/openoffice/TypeDetection/UISort-math.xcu \
	org/openoffice/ucb/Configuration-gio.xcu \
	org/openoffice/ucb/Configuration-webdav.xcu \
	org/openoffice/ucb/Configuration-win.xcu \
))

# perhaps this file should be moved 2 levels up?
$(eval $(call gb_Configuration_add_spool_langpack,registry,officecfg/registry/data/org/openoffice,\
	Langpack.xcu \
))

$(eval $(call gb_Configuration_add_localized_datas,registry,officecfg/registry/data,\
	org/openoffice/Setup.xcu \
	org/openoffice/Office/Accelerators.xcu \
	$(call gb_Helper_optional,LIBRELOGO, \
		org/openoffice/Office/Addons.xcu \
	) \
	org/openoffice/Office/Common.xcu \
	org/openoffice/Office/DataAccess.xcu \
	$(if $(ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS),, \
	    org/openoffice/Office/PresentationMinimizer.xcu \
	    org/openoffice/Office/PresenterScreen.xcu) \
	org/openoffice/Office/TableWizard.xcu \
	org/openoffice/Office/UI.xcu \
	org/openoffice/Office/Embedding.xcu \
	$(call gb_Helper_optional,REPORTBUILDER, \
		org/openoffice/Office/ExtendedColorScheme.xcu \
	) \
	org/openoffice/Office/FormWizard.xcu \
	org/openoffice/Office/Writer.xcu \
	org/openoffice/Office/UI/BasicIDECommands.xcu \
	org/openoffice/Office/UI/BibliographyCommands.xcu \
	org/openoffice/Office/UI/CalcCommands.xcu \
	org/openoffice/Office/UI/ChartCommands.xcu \
	org/openoffice/Office/UI/ChartWindowState.xcu \
	org/openoffice/Office/UI/DbuCommands.xcu \
	org/openoffice/Office/UI/BaseWindowState.xcu \
	org/openoffice/Office/UI/WriterFormWindowState.xcu \
	org/openoffice/Office/UI/WriterReportWindowState.xcu \
	org/openoffice/Office/UI/DbQueryWindowState.xcu \
	org/openoffice/Office/UI/DbTableWindowState.xcu \
	org/openoffice/Office/UI/DbRelationWindowState.xcu \
	$(call gb_Helper_optional,REPORTBUILDER, \
		org/openoffice/Office/UI/DbReportWindowState.xcu \
	) \
	org/openoffice/Office/UI/DbBrowserWindowState.xcu \
	org/openoffice/Office/UI/DbTableDataWindowState.xcu \
	org/openoffice/Office/UI/DrawImpressCommands.xcu \
	$(if $(ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS),, \
	    org/openoffice/Office/UI/Effects.xcu) \
	org/openoffice/Office/UI/GenericCommands.xcu \
	$(if $(ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS),, \
	    org/openoffice/Office/UI/MathCommands.xcu) \
	org/openoffice/Office/UI/BasicIDEWindowState.xcu \
	org/openoffice/Office/UI/CalcWindowState.xcu \
	$(if $(ENABLE_WASM_STRIP_BASIC_DRAW_MATH_IMPRESS),, \
	    org/openoffice/Office/UI/DrawWindowState.xcu \
	    org/openoffice/Office/UI/ImpressWindowState.xcu \
	    org/openoffice/Office/UI/MathWindowState.xcu) \
	$(call gb_Helper_optional,REPORTBUILDER, \
		org/openoffice/Office/UI/ReportCommands.xcu \
	) \
	org/openoffice/Office/UI/Sidebar.xcu \
	org/openoffice/Office/UI/StartModuleWindowState.xcu \
	org/openoffice/Office/UI/WriterWindowState.xcu \
	org/openoffice/Office/UI/XFormsWindowState.xcu \
	org/openoffice/Office/UI/WriterGlobalWindowState.xcu \
	org/openoffice/Office/UI/WriterWebWindowState.xcu \
	org/openoffice/Office/UI/WriterCommands.xcu \
	org/openoffice/Office/UI/GenericCategories.xcu \
	org/openoffice/Office/UI/ToolbarMode.xcu \
))

