# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,jfreereport_flow_engine))

$(eval $(call gb_ExternalProject_use_external_projects,jfreereport_flow_engine,\
	jfreereport_liblayout \
))

$(eval $(call gb_ExternalProject_register_targets,jfreereport_flow_engine,\
	build \
))

$(call gb_ExternalProject_get_state_target,jfreereport_flow_engine,build) :
	$(call gb_Trace_StartRange,jfreereport_flow_engine,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		unset MSYS_NO_PATHCONV && JAVA_HOME=$(JAVA_HOME_FOR_BUILD) \
		$(ICECREAM_RUN) "$(ANT)" \
			$(if $(verbose),-v,-q) \
			-f build.xml \
			-Dbuild.label="build-$(LIBO_VERSION_MAJOR).$(LIBO_VERSION_MINOR).$(LIBO_VERSION_MICRO).$(LIBO_VERSION_PATCH)" \
			-Dlibbase.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libbase/dist/libbase-$(LIBBASE_VERSION).jar \
			-Dlibformula.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libformula/dist/libformula-$(LIBFORMULA_VERSION).jar \
			-Dliblayout.jar=$(gb_UnpackedTarball_workdir)/jfreereport_liblayout/build/lib/liblayout.jar \
			-Dlibloader.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libloader/dist/libloader-$(LIBLOADER_VERSION).jar \
			-Dlibserializer.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libserializer/dist/libserializer-$(LIBBASE_VERSION).jar \
			-Dlibxml.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libxml/dist/libxml-$(LIBXML_VERSION).jar \
			-Dant.build.javac.source=$(JAVA_SOURCE_VER) \
			-Dant.build.javac.target=$(JAVA_TARGET_VER) \
			$(if $(debug),-Dbuild.debug="on") jar \
	)
	$(call gb_Trace_EndRange,jfreereport_flow_engine,EXTERNAL)

# vim: set noet sw=4 ts=4:
