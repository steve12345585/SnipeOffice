# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CustomTarget_CustomTarget,jvmfwk/jreproperties))

$(call gb_CustomTarget_get_target,jvmfwk/jreproperties) : $(gb_CustomTarget_workdir)/jvmfwk/jreproperties/JREProperties.class

$(gb_CustomTarget_workdir)/jvmfwk/jreproperties/JREProperties.class : \
		$(SRCDIR)/jvmfwk/plugins/sunmajor/pluginlib/JREProperties.java \
		| $(gb_CustomTarget_workdir)/jvmfwk/jreproperties/.dir
	$(call gb_Output_announce,$(subst $(WORKDIR)/,,$@),$(true),JCS,1)
	$(call gb_Trace_StartRange,$(subst $(WORKDIR)/,,$@),JCS)
	$(call gb_Helper_abbreviate_dirs, \
	cd $(dir $@) && $(call gb_JavaClassSet_JAVACCOMMAND,$(JAVA_TARGET_VER))  $(if $(call gb_target_symbols_enabled,CustomTarget_jvmfwk/jreproperties),$(gb_JavaClassSet_JAVACDEBUG)) -d $(dir $@) $^)
	$(call gb_Trace_EndRange,$(subst $(WORKDIR)/,,$@),JCS)

# vim:set shiftwidth=4 tabstop=4 noexpandtab:
