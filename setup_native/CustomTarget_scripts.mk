# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

scripts_WORKDIR :=$(gb_CustomTarget_workdir)/setup_native/scripts

$(eval $(call gb_CustomTarget_CustomTarget,setup_native/scripts))

ifeq ($(OS),LINUX)
ifneq ($(filter rpm,$(PKGFORMAT)),)
$(eval $(call gb_CustomTarget_register_targets,setup_native/scripts,\
	install \
	uninstall \
	noarch/fake-db-1.0-0.noarch.rpm \
))

$(scripts_WORKDIR)/noarch/fake-db-1.0-0.noarch.rpm: $(SRCDIR)/setup_native/scripts/fake-db.spec
	mkdir -p $(scripts_WORKDIR)/fake-db-root
	$(RPM) --define "_builddir $(scripts_WORKDIR)/fake-db-root" \
		--define "_rpmdir $(scripts_WORKDIR)" -bb $<
	chmod g+w $(scripts_WORKDIR)/fake-db-root

$(scripts_WORKDIR)/install: $(SRCDIR)/setup_native/scripts/install_linux.sh $(scripts_WORKDIR)/noarch/fake-db-1.0-0.noarch.rpm
	$(PERL) -w $(SRCDIR)/setup_native/scripts/install_create.pl $^ $@
	chmod 775 $@

$(scripts_WORKDIR)/uninstall: $(SRCDIR)/setup_native/scripts/uninstall_linux.sh
	cat $< | tr -d "\015" > $@
	chmod 775 $@
endif
endif

# vim: set noet sw=4 ts=4:
