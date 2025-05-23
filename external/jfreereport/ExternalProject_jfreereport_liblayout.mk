# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_ExternalProject_ExternalProject,jfreereport_liblayout))

$(eval $(call gb_ExternalProject_use_external_projects,jfreereport_liblayout,\
	jfreereport_sac \
	jfreereport_libbase \
	jfreereport_flute \
	jfreereport_libloader \
	jfreereport_libxml \
	jfreereport_libformula \
	jfreereport_libfonts \
	jfreereport_librepository \
	jfreereport_libserializer \
))

$(eval $(call gb_ExternalProject_register_targets,jfreereport_liblayout,\
	build \
))

$(call gb_ExternalProject_get_state_target,jfreereport_liblayout,build) :
	$(call gb_Trace_StartRange,jfreereport_liblayout,EXTERNAL)
	$(call gb_ExternalProject_run,build,\
		unset MSYS_NO_PATHCONV && JAVA_HOME=$(JAVA_HOME_FOR_BUILD) \
		$(ICECREAM_RUN) "$(ANT)" \
			$(if $(verbose),-v,-q) \
			-f build.xml \
			-Dbuild.label="build-$(LIBO_VERSION_MAJOR).$(LIBO_VERSION_MINOR).$(LIBO_VERSION_MICRO).$(LIBO_VERSION_PATCH)" \
			-Dflute.jar=$(gb_UnpackedTarball_workdir)/jfreereport_flute/dist/flute-$(FLUTE_VERSION).jar \
			-Dlibbase.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libbase/dist/libbase-$(LIBBASE_VERSION).jar \
			-Dlibformula.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libformula/dist/libformula-$(LIBFORMULA_VERSION).jar \
			-Dlibfonts.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libfonts/dist/libfonts-$(LIBFONTS_VERSION).jar \
			-Dlibloader.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libloader/dist/libloader-$(LIBLOADER_VERSION).jar \
			-Dlibrepository.jar=$(gb_UnpackedTarball_workdir)/jfreereport_librepository/dist/librepository-$(LIBREPOSITORY_VERSION).jar \
			-Dlibserializer.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libserializer/dist/libserializer-$(LIBBASE_VERSION).jar \
			-Dlibxml.jar=$(gb_UnpackedTarball_workdir)/jfreereport_libxml/dist/libxml-$(LIBXML_VERSION).jar \
			-Dsac.jar=$(gb_UnpackedTarball_workdir)/jfreereport_sac/build/lib/sac.jar \
			-Dant.build.javac.source=$(JAVA_SOURCE_VER) \
			-Dant.build.javac.target=$(JAVA_TARGET_VER) \
			-Dantcontrib.available="true" \
			-Dbuild.id="10682" \
			$(if $(debug),-Dbuild.debug="on") jar \
	)
	$(call gb_Trace_EndRange,jfreereport_liblayout,EXTERNAL)

# vim: set noet sw=4 ts=4:
