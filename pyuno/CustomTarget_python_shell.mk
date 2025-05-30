# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CustomTarget_CustomTarget,pyuno/python_shell))

$(eval $(call gb_CustomTarget_register_targets,pyuno/python_shell,\
	os.sh \
	python.sh \
))

ifeq ($(OS),MACOSX)
pyuno_PYTHON_SHELL_VERSION:=$(PYTHON_VERSION_MAJOR).$(PYTHON_VERSION_MINOR)
else
pyuno_PYTHON_SHELL_VERSION:=$(PYTHON_VERSION)
endif

$(gb_CustomTarget_workdir)/pyuno/python_shell/python.sh : \
		$(SRCDIR)/pyuno/zipcore/python.sh \
		$(gb_CustomTarget_workdir)/pyuno/python_shell/os.sh
	$(call gb_Output_announce,$(subst $(WORKDIR)/,,$@),$(true),CAT,1)
	$(call gb_Trace_StartRange,$(subst $(WORKDIR)/,,$@),CAT)
	cat $^ > $@ && chmod +x $@
	$(call gb_Trace_EndRange,$(subst $(WORKDIR)/,,$@),CAT)

$(gb_CustomTarget_workdir)/pyuno/python_shell/os.sh : \
		$(SRCDIR)/pyuno/zipcore/$(if $(filter MACOSX,$(OS)),mac,nonmac).sh \
		$(BUILDDIR)/config_$(gb_Side)/config_python.h
	$(call gb_Output_announce,$(subst $(WORKDIR)/,,$@),$(true),SED,1)
	$(call gb_Trace_StartRange,$(subst $(WORKDIR)/,,$@),SED)
	sed -e "s/%%PYVERSION%%/$(pyuno_PYTHON_SHELL_VERSION)/g" \
		$< > $@
	$(call gb_Trace_EndRange,$(subst $(WORKDIR)/,,$@),SED)

# vim: set noet sw=4 ts=4:
