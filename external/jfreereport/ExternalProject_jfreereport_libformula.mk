# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# for VERSION
include $(SRCDIR)/external/jfreereport/version.mk

$(eval $(call gb_ExternalProject_ExternalProject,jfreereport_libformula))

$(eval $(call gb_ExternalProject_register_targets,jfreereport_libformula,\
	build \
))

$(call gb_ExternalProject_get_state_target,jfreereport_libformula,build) :
	$(call gb_Trace_StartRange,jfreereport_libformula,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		unset MSYS_NO_PATHCONV && JAVA_HOME=$(JAVA_HOME_FOR_BUILD) \
		$(ICECREAM_RUN) "$(ANT)" \
			$(if $(verbose),-v,-q) \
			-f build.xml \
			-Dbuild.label="build-$(LIBO_VERSION_MAJOR).$(LIBO_VERSION_MINOR).$(LIBO_VERSION_MICRO).$(LIBO_VERSION_PATCH)" \
			-Dant.build.javac.source=$(JAVA_SOURCE_VER) \
			-Dant.build.javac.target=$(JAVA_TARGET_VER) \
			-Dantcontrib.available="true" \
			-Dbuild.id="10682" \
			-Dproject.revision="$(LIBFORMULA_VERSION)" \
			$(if $(debug),-Dbuild.debug="on") jar \
	)
	$(call gb_Trace_EndRange,jfreereport_libformula,EXTERNAL)

# vim: set noet sw=4 ts=4:
