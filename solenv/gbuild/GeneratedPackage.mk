# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# class GeneratedPackage

# Enables to deliver whole directories (of generated files) to instdir.
#
# GeneratedPackage shall be used as a substitution for Package when the
# names of the produced files are not known in advance (in older times,
# we used Zip in these places). It shall only be used to deliver files
# for installation.
#
# If you know the filenames in advance, use Package. Laziness is not an
# excuse.

gb_GeneratedPackage__get_srcdir = $(lastword $(subst <>, ,$(1)))
gb_GeneratedPackage__get_destdir = $(firstword $(subst <>, ,$(1)))

$(dir $(call gb_GeneratedPackage_get_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_GeneratedPackage_get_target,%))%.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

# require all added directories to exist
$(call gb_GeneratedPackage_get_target,%) :| $(foreach pair,$(PACKAGE_DIRS),$(PACKAGE_SOURCEDIR)/$(call gb_GeneratedPackage__get_srcdir,$(pair)))

# split in two commands to avoid running into commandline/environment size limits
# on windows with all languages the processing of help can truncate the find command otherwise
$(call gb_GeneratedPackage_get_target,%) :
	$(call gb_Output_announce,$*,$(true),GPK,2)
	$(call gb_Trace_StartRange,$*,GPK)
	$(if $(PACKAGE_DIRS),,$(call gb_Output_error,no dirs were added))
	$(call gb_Helper_abbreviate_dirs,\
		rm -rf $(foreach pair,$(PACKAGE_DIRS),$(call gb_GeneratedPackage__get_destdir,$(pair))) \
		&& mkdir -p $(foreach pair,$(PACKAGE_DIRS),$(dir $(call gb_GeneratedPackage__get_destdir,$(pair)))) \
		$(foreach pair,$(PACKAGE_DIRS),&& cp -R $(PACKAGE_SOURCEDIR)/$(call gb_GeneratedPackage__get_srcdir,$(pair)) $(call gb_GeneratedPackage__get_destdir,$(pair))) \
	)
	$(call gb_Helper_abbreviate_dirs,\
		$(FIND) $(foreach pair,$(PACKAGE_DIRS),$(call gb_GeneratedPackage__get_destdir,$(pair))) \( -type f -o -type l \) -print | LC_ALL=C $(SORT) > $@ \
	)
	$(call gb_Trace_EndRange,$*,GPK)

.PHONY : $(call gb_GeneratedPackage_get_clean_target,%)
$(call gb_GeneratedPackage_get_clean_target,%) :
	$(call gb_Output_announce,$*,$(false),GPK,2)
	rm -rf $(call gb_GeneratedPackage_get_target,$*) $(PACKAGE_DIRS)

# Create a generated package.
#
# gb_GeneratedPackage_GeneratedPackage package srcdir
define gb_GeneratedPackage_GeneratedPackage
$(call gb_GeneratedPackage_get_target,$(1)) : PACKAGE_DIRS :=
$(call gb_GeneratedPackage_get_target,$(1)) : PACKAGE_SOURCEDIR := $(2)
$(call gb_GeneratedPackage_get_clean_target,$(1)) : PACKAGE_DIRS :=

$(call gb_GeneratedPackage_get_target,$(1)) : $(gb_Module_CURRENTMAKEFILE)
$(call gb_GeneratedPackage_get_target,$(1)) :| $(dir $(call gb_GeneratedPackage_get_target,$(1))).dir

$$(eval $$(call gb_Module_register_target,$(call gb_GeneratedPackage_get_target,$(1)),$(call gb_GeneratedPackage_get_clean_target,$(1))))
$(call gb_Helper_make_userfriendly_targets,$(1),GeneratedPackage)

endef

# Depend on a custom target.
#
# gb_GeneratedPackage_use_customtarget package custom-target
define gb_GeneratedPackage_use_customtarget
$(call gb_GeneratedPackage_get_target,$(1)) : $(call gb_CustomTarget_get_target,$(2))

endef

# Depend on an unpacked tarball.
#
# gb_GeneratedPackage_use_unpacked package unpacked
define gb_GeneratedPackage_use_unpacked
$(call gb_GeneratedPackage_get_target,$(1)) : $(call gb_UnpackedTarball_get_target,$(2))

endef

# Depend on an external project.
#
# gb_GeneratedPackage_use_external_project package project
define gb_GeneratedPackage_use_external_project
$(call gb_GeneratedPackage_get_target,$(1)) : $(call gb_ExternalProject_get_target,$(2))

endef

# Add a dir to the package.
#
# The srcdir will be copied to instdir as destdir.
#
# gb_GeneratedPackage_add_dir package destdir srcdir
define gb_GeneratedPackage_add_dir
$(call gb_GeneratedPackage_get_target,$(1)) : PACKAGE_DIRS += $(strip $(2))<>$(strip $(3))
$(call gb_GeneratedPackage_get_clean_target,$(1)) : PACKAGE_DIRS += $(2)

endef

# vim: set noet sw=4 ts=4:
