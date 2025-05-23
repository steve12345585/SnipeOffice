# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
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

#the following user-defined variables are supported:
# YACCFLAGS
# LEXFLAGS
# CPPFLAGS
# CFLAGS
# CXXFLAGS
# OBJCFLAGS
# OBJCXXFLAGS
# LDFLAGS

# defined by platform
#  gb_AsmObject_get_source (.asm on Windows, .s elsewhere)
#  gb_AsmObject__command
#  gb_CObject__command_pattern
#  gb_LinkTarget_CXXFLAGS
#  gb_LinkTarget_LDFLAGS
#  gb_LinkTarget_INCLUDE

# Same happens for the gb_LinkTarget_add_libs calls from RepositoryExternal.mk. But we have no real
# way to separate for gbuild internal and external gb_LinkTarget_add_libs calls.
# So this flags these call parameters with an @ postfix. But now these must be filtered out, so this
# defines that filter, just in case we need to change it because it conflicts with some parameters.
# FYI: on Windows, gb_Library_use_system_win32_libs also calls gb_LinkTarget_add_libs; easy to miss.
gb_LinkTarget__syslib = %@

# debug flags, if the LinkTarget is named in the list of libraries of ENABLE_SYMBOLS_FOR
gb_LinkTarget__get_debugflags= \
    $(if $(ENABLE_OPTIMIZED),$(gb_COMPILEROPTFLAGS), \
        $(if $(ENABLE_OPTIMIZED_DEBUG),$(gb_COMPILERDEBUGOPTFLAGS),$(gb_COMPILERNOOPTFLAGS))) \
    $(if $(call gb_target_symbols_enabled,$(1)),$(gb_DEBUGINFO_FLAGS))

# T_LDFLAGS is just expanded once. Override the flags here, so that the linker and compiler use the same.
ifeq (EMSCRIPTEN,$(OS))
gb_LinkTarget__get_debugldflags=$(call gb_LinkTarget__get_debugflags,$1) \
    $(gb_LinkTarget__emscripten_warnings_ldflags)
else
# similar for LDFLAGS, use linker optimization flags in non-debug case,
# but moreover strip debug from libraries for which debuginfo is not wanted
# (some libraries reuse .o files from other libraries, notably unittests)
gb_LinkTarget__get_stripldflags=$(if $(strip $(CFLAGS)$(CXXFLAGS)$(OBJCFLAGS)$(OBJCXXFLAGS)$(LDFLAGS)),,$(gb_LINKERSTRIPDEBUGFLAGS))
gb_LinkTarget__get_debugldflags=$(if $(call gb_target_symbols_enabled,$(1)),$(gb_LINKER_DEBUGINFO_FLAGS),$(gb_LINKEROPTFLAGS) $(call gb_LinkTarget__get_stripldflags,$(1)))
endif

# generic cflags/cxxflags to use (optimization flags, debug flags)
# user supplied CFLAGS/CXXFLAGS override default debug/optimization flags
# call gb_LinkTarget__get_cflags,linktargetmakefilename
gb_LinkTarget__get_cflags=$(if $(CFLAGS),$(CFLAGS),$(call gb_LinkTarget__get_debugflags,$(1)))
gb_LinkTarget__get_objcflags=$(if $(OBJCFLAGS),$(OBJCFLAGS),$(call gb_LinkTarget__get_debugflags,$(1)))
gb_LinkTarget__get_cxxflags=$(if $(CXXFLAGS),$(CXXFLAGS),$(call gb_LinkTarget__get_debugflags,$(1)))
gb_LinkTarget__get_objcxxflags=$(if $(OBJCXXFLAGS),$(OBJCXXFLAGS),$(call gb_LinkTarget__get_debugflags,$(1)))
gb_LinkTarget__get_cxxclrflags=$(call gb_LinkTarget__get_debugflags,$(1))
# call gb_LinkTarget__get_ldflags,linktargetmakefilename
gb_LinkTarget__get_ldflags=$(if $(LDFLAGS),$(LDFLAGS),$(call gb_LinkTarget__get_debugldflags,$(1)))

gb_LinkTarget_LAYER_LINKPATHS := \
	URELIB:URELIB. \
	UREBIN:URELIB. \
	SDKBIN:URELIB. \
	OOO:URELIB+OOO. \
	SHLXTHDL:. \
	OXT:OXT. \
	NONE:URELIB+OOO+NONE. \


# Used to run a compiler plugin tool.
#
# At least for now, these definitions are generic enough so that they can be
# shared across all current use cases (COMPILER_EXTERNAL_TOOL,
# COMPILER_PLUGIN_TOOL) on all relevant toolchains (GCC?, Clang, clang-cl).  If
# it ever becomes necessary, they can be moved to e.g.
# platform/com_{GCC,MSC}_class.mk and made different there.
#
# $(call gb_CObject__tool_command,relative-source,source,compiler-plugins)
define gb_CObject__tool_command
$(call gb_Helper_abbreviate_dirs,\
        ICECC=no CCACHE_DISABLE=1 \
	$(gb_CC) \
		$(DEFS) \
		$(T_LTOFLAGS) \
		$(if $(VISIBILITY),,$(gb_VISIBILITY_FLAGS)) \
		$(if $(WARNINGS_NOT_ERRORS),$(if $(ENABLE_WERROR),$(if $(PLUGIN_WARNINGS_AS_ERRORS),$(gb_COMPILER_PLUGINS_WARNINGS_AS_ERRORS))),$(gb_CFLAGS_WERROR)) \
		$(if $(3),$(gb_COMPILER_PLUGINS_TOOL)) \
		$(T_CFLAGS) $(T_CFLAGS_APPEND) \
		$(if $(EXTERNAL_CODE),$(gb_CXXFLAGS_Wundef),$(gb_DEFS_INTERNAL)) \
		-c $(2) \
		$(INCLUDE) \
		)
endef
define gb_ObjCObject__tool_command
$(call gb_Helper_abbreviate_dirs,\
        ICECC=no CCACHE_DISABLE=1 \
	$(gb_CC) \
		$(DEFS) \
		$(T_LTOFLAGS) \
		$(if $(VISIBILITY),,$(gb_VISIBILITY_FLAGS)) \
		$(if $(WARNINGS_NOT_ERRORS),$(if $(ENABLE_WERROR),$(if $(PLUGIN_WARNINGS_AS_ERRORS),$(gb_COMPILER_PLUGINS_WARNINGS_AS_ERRORS))),$(gb_CFLAGS_WERROR)) \
		$(if $(3),$(gb_COMPILER_PLUGINS_TOOL)) \
		$(T_OBJCFLAGS) $(T_OBJCFLAGS_APPEND) \
		$(if $(EXTERNAL_CODE),$(gb_CXXFLAGS_Wundef),$(gb_DEFS_INTERNAL)) \
		-c $(2) \
		$(INCLUDE) \
		)
endef
define gb_CxxObject__tool_command
$(call gb_Helper_abbreviate_dirs,\
        ICECC=no CCACHE_DISABLE=1 \
	$(gb_CXX) \
		$(DEFS) \
		$(T_LTOFLAGS) \
		$(if $(VISIBILITY),,$(gb_VISIBILITY_FLAGS)) \
		$(if $(WARNINGS_NOT_ERRORS),$(if $(ENABLE_WERROR),$(if $(PLUGIN_WARNINGS_AS_ERRORS),$(gb_COMPILER_PLUGINS_WARNINGS_AS_ERRORS))),$(gb_CFLAGS_WERROR)) \
		$(if $(3),$(gb_COMPILER_PLUGINS_TOOL)) \
		$(T_CXXFLAGS) $(T_CXXFLAGS_APPEND) \
		$(if $(EXTERNAL_CODE),$(gb_CXXFLAGS_Wundef),$(gb_DEFS_INTERNAL)) \
		-c $(2) \
		$(INCLUDE) \
		)
endef
define gb_ObjCxxObject__tool_command
$(call gb_Helper_abbreviate_dirs,\
        ICECC=no CCACHE_DISABLE=1 \
	$(gb_CXX) \
		$(DEFS) \
		$(T_LTOFLAGS) \
		$(if $(VISIBILITY),,$(gb_VISIBILITY_FLAGS)) \
		$(if $(WARNINGS_NOT_ERRORS),$(if $(ENABLE_WERROR),$(if $(PLUGIN_WARNINGS_AS_ERRORS),$(gb_COMPILER_PLUGINS_WARNINGS_AS_ERRORS))),$(gb_CFLAGS_WERROR)) \
		$(if $(3),$(gb_COMPILER_PLUGINS_TOOL)) \
		$(T_OBJCXXFLAGS) $(T_OBJCXXFLAGS_APPEND) \
		$(if $(EXTERNAL_CODE),$(gb_CXXFLAGS_Wundef),$(gb_DEFS_INTERNAL)) \
		-c $(2) \
		$(INCLUDE) \
		)
endef
define gb_CxxClrObject__tool_command
$(call gb_Helper_abbreviate_dirs,\
        ICECC=no CCACHE_DISABLE=1 \
	$(gb_CXX) \
		$(DEFS) \
		$(T_LTOFLAGS) \
		$(if $(VISIBILITY),,$(gb_VISIBILITY_FLAGS)) \
		$(if $(WARNINGS_NOT_ERRORS),$(if $(ENABLE_WERROR),$(if $(PLUGIN_WARNINGS_AS_ERRORS),$(gb_COMPILER_PLUGINS_WARNINGS_AS_ERRORS))),$(gb_CFLAGS_WERROR)) \
		$(if $(3),$(gb_COMPILER_PLUGINS_TOOL)) \
		$(T_CXXCLRFLAGS) $(T_CXXCLRFLAGS_APPEND) \
		$(if $(EXTERNAL_CODE),$(gb_CXXFLAGS_Wundef),$(gb_DEFS_INTERNAL)) \
		-c $(2) \
		$(INCLUDE) \
		)
endef


# Overview of dependencies and tasks of LinkTarget
#
# target                      task                         depends on
# LinkTarget                  linking                      AsmObject CObject CxxObject GenCObject GenCxxObject ObjCObject ObjCxxObject GenObjCObject GenObjCxxObject GenAsmObject GenNasmObject CxxClrObject
#                                                          LinkTarget/headers
# LinkTarget/dep              joined dep file              AsmObject/dep CObject/dep CxxObject/dep GenCObject/dep GenCxxObject/dep ObjCObject/dep ObjCxxObject/dep GenObjCObject/dep GenObjCxxObject/dep GenAsmObject/dep GenNasmObject/dep CxxClrObject/dep GenCxxClrObject/dep
#                                                          | LinkTarget/headers
# LinkTarget/headers          all headers available
#                             including own generated
# PCH                         precompiled headers          LinkTarget/headers
# CObject                     plain c compile              | LinkTarget/headers
# CxxObject                   c++ compile                  | LinkTarget/headers PCH
# GenCObject                  plain c compile from         | LinkTarget/headers
#                              generated source
# GenCxxObject                C++ compile from             | LinkTarget/headers PCH
#                              generated source
# ObjCObject                  objective c compile          | LinkTarget/headers
# ObjCxxObject                objective c++ compile        | LinkTarget/headers
# GenObjCObject               objective c compile from     | LinkTarget/headers
#                              generated source
# GenObjCxxObject             objective c++ compile from   | LinkTarget/headers
#                              generated source
# GenAsmObject                asm compile from             | LinkTarget/headers
#                              generated source
# GenNasmObject               nasm compile from            | LinkTarget/headers
#                              generated source
# CxxClrObject                C++ CLR compile              | LinkTarget/headers
# GenCxxClrObject             C++ CLR compile from         | LinkTarget/headers
#                              generated source
#
# AsmObject                   asm compile                  | LinkTarget
#
# CObject/dep                 dependencies                 these targets generate empty dep files
# CxxObject/dep               dependencies                 that are populated upon compile
# GenCObject/dep              dependencies
# GenCxxObject/dep            dependencies
# ObjCObject/dep              dependencies
# ObjCxxObject/dep            dependencies
# GenObjCObject/dep           dependencies
# GenObjCxxObject/dep         dependencies
# GenAsmObject/dep            dependencies
# GenNasmObject/dep           dependencies
# CxxClrObject/dep            dependencies
# GenCxxClrObject/dep         dependencies
# AsmObject/dep               dependencies

# LinkTarget/headers means gb_LinkTarget_get_headers_target etc.
# dependencies prefixed with | are build-order only dependencies


# check that objects are only linked into one link target:
# multiple linking may cause problems because different link targets may
# require different compiler flags
define gb_Object__owner
$$(if $$(OBJECTOWNER),\
  $$(call gb_Output_error,fdo#47246: $(1) is linked in by $$(OBJECTOWNER) $(2)))$(2)
endef

# For every object there is a dep file (if gb_FULLDEPS is active).
# The dep file depends on the object: the Object__command also updates the
# dep file as a side effect.
# In the dep file rule just touch it so it's newer than the object.

# Setting FORCE_COMPILE allows forcing compilation for specific sources,
# usually used to force running a tool on the sources (see compilerplugins/README).
# If set, it'll force all considered sources for rebuild. But it's possible
# to explicitly specify gbuild build targets where running of the tool will be skipped
# (where 'all' means everything, '-' prepended means to not enable, '/' appended means
# everything in the directory; there is no ordering, more specific overrides
# more general, and disabling takes precedence).
# Example: FORCE_COMPILE="all -sw/ -Library_sc"

# Detect whether forced compile should be used for the given gbuild target.
# enable if: no "-TARGET" defined AND [module is enabled OR "TARGET" defined]
# call gb_LinkTarget__force_compile,linktargetmakefilename
gb_LinkTarget__force_compile = \
 $(and $(if $(filter -$(1),$(FORCE_COMPILE)),,$(true)),\
       $(or $(gb_Module_CURRENTMODULE_FORCE_COMPILE),\
            $(filter $(1),$(FORCE_COMPILE))))

# This one only exists to force .c/.cxx "rebuilds" when running a compiler tool.
.PHONY: force_compile_target
force_compile_target:
ifneq ($(FORCE_COMPILE),)
gb_FORCE_COMPILE_TARGET := force_compile_target
endif

# A tool is run either if FORCE_COMPILE is not set (in that case it's always run,
# because the target is not up to date), or if FORCE_COMPILE is set then
# the tool is run only if the value of FORCE_COMPILE includes the target.
gb_LinkTarget__tool_compile_enabled = \
 $(if $(FORCE_COMPILE),$(T_FORCE_COMPILE),$(true))

# CObject class

gb_CObject_get_source = $(1)/$(2).c

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_CObject_get_target,%) : $(call gb_CObject_get_source,$(SRCDIR),%) $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$*.c,$(true),C  ,3)
	$(call gb_Trace_StartRange,$*.c,C  )
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_CObject__tool_command,$*,$<,$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_CFLAGS) $(T_CFLAGS_APPEND),$<,$(call gb_CObject_get_dep_target,$*),$(COMPILER_PLUGINS),$(T_CC))))
	$(call gb_Trace_EndRange,$*.c,C  )
else
$(call gb_CObject_get_target,%) : $(call gb_CObject_get_source,$(SRCDIR),%)
	$(call gb_Output_announce,$*.c,$(true),$(if $(COMPILER_TEST),C? ,C  ),3)
	$(call gb_Trace_StartRange,$*.c,$(if $(COMPILER_TEST),C? ,C  ))
	$(call gb_CObject__command_pattern,$@,$(T_CFLAGS) $(T_CFLAGS_APPEND),$<,$(call gb_CObject_get_dep_target,$*),$(COMPILER_PLUGINS),$(T_CC))
	$(call gb_Trace_EndRange,$*.c,$(if $(COMPILER_TEST),C? ,C  ))
endif

# Note: if the *Object_dep_target does not exist it will be created by
# concat-deps as PHONY
ifeq ($(gb_FULLDEPS),$(true))

$(call gb_CObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# CxxObject class

gb_CxxObject_get_source = $(1)/$(2).cxx

# Only enable PCH if the PCH_CXXFLAGS and the PCH_DEFS (from the linktarget)
# are the same as the T_CXXFLAGS and DEFS we want to use for this object. This
# should usually be the case.  The DEFS/T_CXXFLAGS would have to be manually
# overridden for one object file for them to differ.  PCH_CXXFLAGS/PCH_DEFS
# should never be overridden on an object -- they should be the same as for the
# whole linktarget. In general it should be cleaner to use a static library
# compiled with different flags and link that in rather than mixing different
# flags in one linktarget. If OBJECT_HAS_EXTRA_CXXFLAGS is set, the object
# has explicitly set additional CXXFLAGS, so in that case avoid using the PCH.
# T_PCH_EXTRA_CXXFLAGS is used when some object requires extra flags when using
# the PCH, but they are intended (gb_PrecompiledHeader_pch_with_obj).
define gb_CxxObject__set_pchflags
ifneq ($(gb_ENABLE_PCH),)
ifneq ($(strip $$(PCH_NAME)),)
ifeq ($(OBJECT_HAS_EXTRA_CXXFLAGS),)
ifeq ($$(sort $$(PCH_CXXFLAGS) $$(PCH_DEFS)),$$(sort $$(T_CXXFLAGS) $$(T_CXXFLAGS_APPEND) $$(DEFS)))
$$@ : PCHFLAGS := $$(call gb_PrecompiledHeader_get_enableflags,$$(PCH_NAME),$$(PCH_LINKTARGETMAKEFILENAME),$$(PCH_HEADER)) $$(T_PCH_EXTRA_CXXFLAGS)
else
$$(warning No precompiled header available for $$*.cxx .)
$$(info precompiled header flags : $$(sort $$(PCH_CXXFLAGS) $$(PCH_DEFS)))
$$(info .           object flags : $$(sort $$(T_CXXFLAGS) $$(T_CXXFLAGS_APPEND) $$(DEFS)))
$$(error   Incorrect precompiled header setup or internal gbuild error.)
$$@ : PCHFLAGS :=
endif
endif
endif
endif
endef

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_CxxObject_get_target,%) : $(call gb_CxxObject_get_source,$(SRCDIR),%) $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$*.cxx,$(true),CXX,3)
	$(call gb_Trace_StartRange,$*.cxx,CXX)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_CxxObject__tool_command,$*,$<,$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(eval $(gb_CxxObject__set_pchflags))))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_CXXFLAGS) $(T_CXXFLAGS_APPEND) $(if $(COMPILER_TEST),$(gb_COMPILER_TEST_FLAGS)),$<,$(call gb_CxxObject_get_dep_target,$*),$(COMPILER_PLUGINS),$(T_CXX))))
	$(call gb_Trace_EndRange,$*.cxx,CXX)
else
$(call gb_CxxObject_get_target,%) : $(call gb_CxxObject_get_source,$(SRCDIR),%)
	$(call gb_Output_announce,$*.cxx,$(true),$(if $(COMPILER_TEST),CPT,CXX),3)
	$(call gb_Trace_StartRange,$*.cxx,$(if $(COMPILER_TEST),CPT,CXX))
	$(eval $(gb_CxxObject__set_pchflags))
	$(call gb_CObject__command_pattern,$@,$(T_CXXFLAGS) $(T_CXXFLAGS_APPEND) $(if $(COMPILER_TEST),$(gb_COMPILER_TEST_FLAGS)),$<,$(call gb_CxxObject_get_dep_target,$*),$(COMPILER_PLUGINS),$(T_CXX))
	$(call gb_Trace_EndRange,$*.cxx,$(if $(COMPILER_TEST),CPT,CXX))
endif

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_CxxObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# GenCObject class

gb_GenCObject_get_source = $(WORKDIR)/$(1).c

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_GenCObject_get_target,%) : $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$*.c,$(true),C  ,3)
	$(call gb_Trace_StartRange,$*.c,C  )
	test -f $(call gb_GenCObject_get_source,$*) || (echo "Missing generated source file $(call gb_GenCObject_get_source,$*)" && false)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_CObject__tool_command,$*,$(call gb_GenCObject_get_source,$*),$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_CFLAGS) $(T_CFLAGS_APPEND),$(call gb_GenCObject_get_source,$*),$(call gb_GenCObject_get_dep_target,$*),$(COMPILER_PLUGINS),$(T_CC))))
	$(call gb_Trace_EndRange,$*.c,C  )
else
$(call gb_GenCObject_get_target,%) :
	$(call gb_Output_announce,$*.c,$(true),C  ,3)
	$(call gb_Trace_StartRange,$*.c,C  )
	test -f $(call gb_GenCObject_get_source,$*) || (echo "Missing generated source file $(call gb_GenCObject_get_source,$*)" && false)
	$(call gb_CObject__command_pattern,$@,$(T_CFLAGS) $(T_CFLAGS_APPEND),$(call gb_GenCObject_get_source,$*),$(call gb_GenCObject_get_dep_target,$*),$(COMPILER_PLUGINS),$(T_CC))
	$(call gb_Trace_EndRange,$*.c,C  )
endif

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_GenCObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_GenCObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_GenCObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# GenCxxObject class

gb_GenCxxObject_get_source = $(WORKDIR)/$(1).$(gb_LinkTarget_CXX_SUFFIX_$(call gb_LinkTarget__get_workdir_linktargetname,$(2)))

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_GenCxxObject_get_target,%) : $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$(GEN_CXX_SOURCE)),$(true),CXX,3)
	$(call gb_Trace_StartRange,$(subst $(BUILDDIR)/,,$(GEN_CXX_SOURCE)),CXX)
	test -f $(GEN_CXX_SOURCE) || (echo "Missing generated source file $(GEN_CXX_SOURCE)" && false)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_CxxObject__tool_command,$*,$(GEN_CXX_SOURCE),$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(eval $(gb_CxxObject__set_pchflags))))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_CXXFLAGS) $(T_CXXFLAGS_APPEND),$(GEN_CXX_SOURCE),$(call gb_GenCxxObject_get_dep_target,$*),$(COMPILER_PLUGINS),$(T_CXX))))
	$(call gb_Trace_EndRange,$(subst $(BUILDDIR)/,,$(GEN_CXX_SOURCE)),CXX)
else
$(call gb_GenCxxObject_get_target,%) :
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$(GEN_CXX_SOURCE)),$(true),CXX,3)
	$(call gb_Trace_StartRange,$(subst $(BUILDDIR)/,,$(GEN_CXX_SOURCE)),CXX)
	test -f $(GEN_CXX_SOURCE) || (echo "Missing generated source file $(GEN_CXX_SOURCE)" && false)
	$(eval $(gb_CxxObject__set_pchflags))
	$(call gb_CObject__command_pattern,$@,$(T_CXXFLAGS) $(T_CXXFLAGS_APPEND),$(GEN_CXX_SOURCE),$(call gb_GenCxxObject_get_dep_target,$*),$(COMPILER_PLUGINS),$(T_CXX))
	$(call gb_Trace_EndRange,$(subst $(BUILDDIR)/,,$(GEN_CXX_SOURCE)),CXX)
endif

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_GenCxxObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_GenCxxObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_GenCxxObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# GenCxxClrObject class

gb_GenCxxClrObject_get_source = $(WORKDIR)/$(1).$(gb_LinkTarget_CXX_SUFFIX_$(call gb_LinkTarget__get_workdir_linktargetname,$(2)))

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_GenCxxClrObject_get_target,%) : $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$(GEN_CXXCLR_SOURCE)),$(true),CLR,3)
	$(call gb_Trace_StartRange,$(subst $(BUILDDIR)/,,$(GEN_CXXCLR_SOURCE)),CLR)
	test -f $(GEN_CXXCLR_SOURCE) || (echo "Missing generated source file $(GEN_CXXCLR_SOURCE)" && false)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_CxxClrObject__tool_command,$*,$(GEN_CXXCLR_SOURCE),$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_CXXCLRFLAGS) $(T_CXXCLRFLAGS_APPEND),$(GEN_CXXCLR_SOURCE),$(call gb_GenCxxClrObject_get_dep_target,$*),$(COMPILER_PLUGINS),)))
	$(call gb_Trace_EndRange,$(subst $(BUILDDIR)/,,$(GEN_CXXCLR_SOURCE)),CLR)
else
$(call gb_GenCxxClrObject_get_target,%) :
	$(call gb_Output_announce,$(subst $(BUILDDIR)/,,$(GEN_CXXCLR_SOURCE)),$(true),CLR,3)
	$(call gb_Trace_StartRange,$(subst $(BUILDDIR)/,,$(GEN_CXXCLR_SOURCE)),CLR)
	test -f $(GEN_CXXCLR_SOURCE) || (echo "Missing generated source file $(GEN_CXXCLR_SOURCE)" && false)
	$(call gb_CObject__command_pattern,$@,$(T_CXXCLRFLAGS) $(T_CXXCLRFLAGS_APPEND),$(GEN_CXXCLR_SOURCE),$(call gb_GenCxxClrObject_get_dep_target,$*),$(COMPILER_PLUGINS),)
	$(call gb_Trace_EndRange,$(subst $(BUILDDIR)/,,$(GEN_CXXCLR_SOURCE)),CLR)
endif

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_GenCxxClrObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_GenCxxClrObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_GenCxxClrObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# YaccTarget class

# XXX: This is more complicated than necessary, but we cannot just use
# the generated C++ file as the main target, because we need to let the
# header depend on that to ensure the header is present before anything
# tries to use it.

gb_YaccTarget_get_source = $(1)/$(2).y

.PHONY : $(call gb_YaccTarget_get_clean_target,%)
$(call gb_YaccTarget_get_clean_target,%) :
	$(call gb_Output_announce,$*,$(false),YAC,3)
	$(call gb_Helper_abbreviate_dirs,\
	    rm -f $(call gb_YaccTarget_get_grammar_target,$*) $(call gb_YaccTarget_get_header_target,$*) $(call gb_YaccTarget_get_target,$*))

$(call gb_YaccTarget_get_target,%) : $(call gb_YaccTarget_get_source,$(SRCDIR),%)
	$(call gb_Output_announce,$*,$(true),YAC,3)
	$(call gb_Trace_StartRange,$*,YAC)
	mkdir -p $(@D) && \
	$(call gb_Helper_wsl_path,$(WSL) $(BISON) -v $(T_YACCFLAGS) \
	    --defines=$(call gb_YaccTarget_get_header_target,$*) \
	    -o $(call gb_YaccTarget_get_grammar_target,$*) $<) && touch $@
	$(call gb_Trace_EndRange,$*,YAC)

# call gb_YaccTarget_YaccTarget,yacctarget
define gb_YaccTarget_YaccTarget
$(call gb_YaccTarget_get_grammar_target,$(1)) : $(call gb_YaccTarget_get_target,$(1))
	touch $$@
$(call gb_YaccTarget_get_header_target,$(1)) : $(call gb_YaccTarget_get_target,$(1))
	touch $$@

endef

# LexTarget class

gb_LexTarget_get_source = $(1)/$(2).l

.PHONY : $(call gb_LexTarget_get_clean_target,%)
$(call gb_LexTarget_get_clean_target,%) :
	$(call gb_Output_announce,$*,$(false),LEX,3)
	$(call gb_Helper_abbreviate_dirs,\
	    rm -f $(call gb_LexTarget_get_scanner_target,$*) $(call gb_LexTarget_get_target,$*))

$(call gb_LexTarget_get_target,%) : $(call gb_LexTarget_get_source,$(SRCDIR),%)
	$(call gb_Output_announce,$*,$(true),LEX,3)
	$(call gb_Trace_StartRange,$*,LEX)
	mkdir -p $(@D) && \
	$(call gb_Helper_wsl_path,$(FLEX) $(T_LEXFLAGS) -o$(call gb_LexTarget_get_scanner_target,$*) $<) && touch $@
	$(call gb_Trace_EndRange,$*,LEX)

# gb_LexTarget_LexTarget(scanner-file)
define gb_LexTarget_LexTarget
$(call gb_LexTarget_get_scanner_target,$(1)) : $(call gb_LexTarget_get_target,$(1))
	touch $$@

endef


# ObjCxxObject class
#

gb_ObjCxxObject_get_source = $(1)/$(2).mm

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_ObjCxxObject_get_target,%) : $(call gb_ObjCxxObject_get_source,$(SRCDIR),%) $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$*.mm,$(true),OCX,3)
	$(call gb_Trace_StartRange,$*.mm,OCX)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_ObjCxxObject__tool_command,$*,$<,$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_OBJCXXFLAGS) $(T_OBJCXXFLAGS_APPEND),$<,$(call gb_ObjCxxObject_get_dep_target,$*),$(COMPILER_PLUGINS),)))
	$(call gb_Trace_EndRange,$*.mm,OCX)
else
$(call gb_ObjCxxObject_get_target,%) : $(call gb_ObjCxxObject_get_source,$(SRCDIR),%)
	$(call gb_Output_announce,$*.mm,$(true),$(if $(COMPILER_TEST),O?X,OCX),3)
	$(call gb_Trace_StartRange,$*.mm,$(if $(COMPILER_TEST),O?X,OCX))
	$(call gb_CObject__command_pattern,$@,$(T_OBJCXXFLAGS) $(T_OBJCXXFLAGS_APPEND),$<,$(call gb_ObjCxxObject_get_dep_target,$*),$(COMPILER_PLUGINS),)
	$(call gb_Trace_EndRange,$*.mm,$(if $(COMPILER_TEST),O?X,OCX))
endif

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_ObjCxxObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_ObjCxxObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_ObjCxxObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# ObjCObject class
#

gb_ObjCObject_get_source = $(1)/$(2).m

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_ObjCObject_get_target,%) : $(call gb_ObjCObject_get_source,$(SRCDIR),%) $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$*.m,$(true),OCC,3)
	$(call gb_Trace_StartRange,$*.m,OCC)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_ObjCObject__tool_command,$*,$<,$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_OBJCFLAGS) $(T_OBJCFLAGS_APPEND),$<,$(call gb_ObjCObject_get_dep_target,$*),$(COMPILER_PLUGINS),)))
	$(call gb_Trace_EndRange,$*.m,OCC)
else
$(call gb_ObjCObject_get_target,%) : $(call gb_ObjCObject_get_source,$(SRCDIR),%)
	$(call gb_Output_announce,$*.m,$(true),$(if $(COMPILER_TEST),O?C,OCC),3)
	$(call gb_Trace_StartRange,$*.m,$(if $(COMPILER_TEST),O?C,OCC))
	$(call gb_CObject__command_pattern,$@,$(T_OBJCFLAGS) $(T_OBJCFLAGS_APPEND),$<,$(call gb_ObjCObject_get_dep_target,$*),$(COMPILER_PLUGINS),)
	$(call gb_Trace_EndRange,$*.m,$(if $(COMPILER_TEST),O?C,OCC))
endif

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_ObjCObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_ObjCObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_ObjCObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# GenObjCObject class

gb_GenObjCObject_get_source = $(WORKDIR)/$(1).m

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_GenObjCObject_get_target,%) : $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$*.m,$(true),OCC,3)
	$(call gb_Trace_StartRange,$*.m,OCC)
	test -f $(call gb_GenObjCObject_get_source,$*) || (echo "Missing generated source file $(call gb_GenObjCObject_get_source,$*)" && false)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_ObjCObject__tool_command,$*,$(call gb_GenObjCObject_get_source,$*),$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_OBJCFLAGS) $(T_OBJCFLAGS_APPEND),$(call gb_GenObjCObject_get_source,$*),$(call gb_GenObjCObject_get_dep_target,$*),$(COMPILER_PLUGINS),)))
	$(call gb_Trace_EndRange,$*.m,OCC)
else
$(call gb_GenObjCObject_get_target,%) :
	$(call gb_Output_announce,$*.m,$(true),OCC,3)
	$(call gb_Trace_StartRange,$*.m,OCC)
	test -f $(call gb_GenObjCObject_get_source,$*) || (echo "Missing generated source file $(call gb_GenObjCObject_get_source,$*)" && false)
	$(call gb_CObject__command_pattern,$@,$(T_OBJCFLAGS) $(T_OBJCFLAGS_APPEND),$(call gb_GenObjCObject_get_source,$*),$(call gb_GenObjCObject_get_dep_target,$*),$(COMPILER_PLUGINS),)
	$(call gb_Trace_EndRange,$*.m,OCC)
endif

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_GenObjCObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_GenObjCObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_GenObjCObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# GenObjCxxObject class

gb_GenObjCxxObject_get_source = $(WORKDIR)/$(1).mm

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_GenObjCxxObject_get_target,%) : $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$*.mm,$(true),OCX,3)
	$(call gb_Trace_StartRange,$*.mm,OCX)
	test -f $(call gb_GenObjCxxObject_get_source,$*) || (echo "Missing generated source file $(call gb_GenObjCxxObject_get_source,$*)" && false)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_ObjCxxObject__tool_command,$*,$(call gb_GenObjCxxObject_get_source,$*),$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_OBJCXXFLAGS) $(T_OBJCXXFLAGS_APPEND),$(call gb_GenObjCxxObject_get_source,$*),$(call gb_GenObjCxxObject_get_dep_target,$*),$(COMPILER_PLUGINS),)))
	$(call gb_Trace_EndRange,$*.mm,OCX)
else
$(call gb_GenObjCxxObject_get_target,%) :
	$(call gb_Output_announce,$*.mm,$(true),OCX,3)
	$(call gb_Trace_StartRange,$*.mm,OCX)
	test -f $(call gb_GenObjCxxObject_get_source,$*) || (echo "Missing generated source file $(call gb_GenObjCxxObject_get_source,$*)" && false)
	$(call gb_CObject__command_pattern,$@,$(T_OBJCXXFLAGS) $(T_OBJCXXFLAGS_APPEND),$(call gb_GenObjCxxObject_get_source,$*),$(call gb_GenObjCxxObject_get_dep_target,$*),$(COMPILER_PLUGINS),)
	$(call gb_Trace_EndRange,$*.mm,OCX)
endif

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_GenObjCxxObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_GenObjCxxObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_GenObjCxxObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif

# GenAsmObject class

gb_GenAsmObject_get_source = $(call gb_AsmObject_get_source,$(WORKDIR),$(1))

$(call gb_GenAsmObject_get_target,%) :
	$(call gb_AsmObject__command,$@,$*,$<,$(call gb_GenAsmObject_get_dep_target,$*))

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_GenAsmObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_GenAsmObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_GenAsmObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif

# GenNasmObject class

gb_GenNasmObject_get_source = $(WORKDIR)/$(1)

$(call gb_GenNasmObject_get_target,%) :
	$(call gb_Output_announce,$*,$(true),ASM,3)
	$(call gb_Trace_StartRange,$*,ASM)
	test -f $(call gb_GenNasmObject_get_source,$*) || (echo "Missing generated source file $(call gb_GenNasmObject_get_source,$*)" && false)
	mkdir -p $(dir $@) $(dir $(call gb_GenNasmObject_get_dep_target,$*)) && cd $(SRCDIR) && \
	    $(call gb_Helper_wsl_path,$(NASM) $(T_NASMFLAGS) $(T_NASMFLAGS_APPEND) -I$(dir $(call gb_GenNasmObject_get_source,$*)) \
	    $(call gb_GenNasmObject_get_source,$*) -o $@) && \
	    echo "$@ : $(call gb_GenNasmObject_get_source,$*)" > $(call gb_GenNasmObject_get_dep_target,$*)
	$(call gb_Trace_EndRange,$*,ASM)

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_GenNasmObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_GenNasmObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_GenNasmObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# CxxClrObject class
#

gb_CxxClrObject_get_source = $(1)/$(2).cxx

ifneq ($(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),)
$(call gb_CxxClrObject_get_target,%) : $(call gb_CxxClrObject_get_source,$(SRCDIR),%) $(gb_FORCE_COMPILE_TARGET)
	$(call gb_Output_announce,$*.cxx,$(true),CLR,3)
	$(call gb_Trace_StartRange,$*.cxx,CLR)
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(call gb_CxxClrObject__tool_command,$*,$<,$(COMPILER_PLUGINS)))
	$(if $(call gb_LinkTarget__tool_compile_enabled),$(if $(COMPILER_PLUGIN_TOOL),$(call gb_CObject__command_pattern,$@,$(T_CXXCLRFLAGS) $(T_CXXCLRFLAGS_APPEND),$<,$(call gb_CxxClrObject_get_dep_target,$*),$(COMPILER_PLUGINS),)))
	$(call gb_Trace_EndRange,$*.cxx,CLR)
else
$(call gb_CxxClrObject_get_target,%) : $(call gb_CxxClrObject_get_source,$(SRCDIR),%)
	$(call gb_Output_announce,$*.cxx,$(true),$(if $(COMPILER_TEST),C?R,CLR),3)
	$(call gb_Trace_StartRange,$*.cxx,$(if $(COMPILER_TEST),C?R,CLR))
	$(call gb_CObject__command_pattern,$@,$(T_CXXCLRFLAGS) $(T_CXXCLRFLAGS_APPEND),$<,$(call gb_CxxClrObject_get_dep_target,$*),$(COMPILER_PLUGINS),)
	$(call gb_Trace_EndRange,$*.cxx,$(if $(COMPILER_TEST),C?R,CLR))
endif

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_CxxClrObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_CxxClrObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_CxxClrObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# AsmObject class

$(call gb_AsmObject_get_target,%) : $(call gb_AsmObject_get_source,$(SRCDIR),%)
	$(call gb_AsmObject__command,$@,$*,$<,$(call gb_AsmObject_get_dep_target,$*))

ifeq ($(gb_FULLDEPS),$(true))
$(dir $(call gb_AsmObject_get_dep_target,%)).dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(dir $(call gb_AsmObject_get_dep_target,%))%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

$(call gb_AsmObject_get_dep_target,%) :
	$(if $(wildcard $@),touch $@)

endif


# LinkTarget class

gb_LinkTarget_DEFAULTDEFS := $(gb_GLOBALDEFS)

.PHONY : $(WORKDIR)/Clean/LinkTarget/%
$(WORKDIR)/Clean/LinkTarget/% :
	$(call gb_Output_announce,$(LINKTARGETMAKEFILENAME),$(false),LNK,4)
	RESPONSEFILE=$(call gb_var2file,$(shell $(gb_MKTEMP)),\
		$(foreach object,$(COBJECTS),$(call gb_CObject_get_target,$(object))) \
		$(foreach object,$(COBJECTS),$(call gb_CObject_get_dep_target,$(object))) \
		$(foreach object,$(COBJECTS),$(call gb_CObject_get_dwo_target,$(object))) \
		$(foreach object,$(CXXOBJECTS),$(call gb_CxxObject_get_target,$(object))) \
		$(foreach object,$(CXXOBJECTS),$(call gb_CxxObject_get_dep_target,$(object))) \
		$(foreach object,$(CXXOBJECTS),$(call gb_CxxObject_get_dwo_target,$(object))) \
		$(foreach object,$(OBJCOBJECTS),$(call gb_ObjCObject_get_target,$(object))) \
		$(foreach object,$(OBJCOBJECTS),$(call gb_ObjCObject_get_dep_target,$(object))) \
		$(foreach object,$(OBJCOBJECTS),$(call gb_ObjCObject_get_dwo_target,$(object))) \
		$(foreach object,$(OBJCXXOBJECTS),$(call gb_ObjCxxObject_get_target,$(object))) \
		$(foreach object,$(OBJCXXOBJECTS),$(call gb_ObjCxxObject_get_dep_target,$(object))) \
		$(foreach object,$(OBJCXXOBJECTS),$(call gb_ObjCxxObject_get_dwo_target,$(object))) \
		$(foreach object,$(CXXCLROBJECTS),$(call gb_CxxClrObject_get_target,$(object))) \
		$(foreach object,$(CXXCLROBJECTS),$(call gb_CxxClrObject_get_dep_target,$(object))) \
		$(foreach object,$(CXXCLROBJECTS),$(call gb_CxxClrObject_get_dwo_target,$(object))) \
		$(foreach object,$(ASMOBJECTS),$(call gb_AsmObject_get_target,$(object))) \
		$(foreach object,$(ASMOBJECTS),$(call gb_AsmObject_get_dep_target,$(object))) \
		$(foreach object,$(ASMOBJECTS),$(call gb_AsmObject_get_dwo_target,$(object))) \
		$(foreach object,$(GENCOBJECTS),$(call gb_GenCObject_get_target,$(object))) \
		$(foreach object,$(GENCOBJECTS),$(call gb_GenCObject_get_dep_target,$(object))) \
		$(foreach object,$(GENCOBJECTS),$(call gb_GenCObject_get_dwo_target,$(object))) \
		$(foreach object,$(GENCXXOBJECTS),$(call gb_GenCxxObject_get_target,$(object))) \
		$(foreach object,$(GENCXXOBJECTS),$(call gb_GenCxxObject_get_dep_target,$(object))) \
		$(foreach object,$(GENCXXOBJECTS),$(call gb_GenCxxObject_get_dwo_target,$(object))) \
		$(foreach object,$(GENOBJCOBJECTS),$(call gb_GenObjCObject_get_target,$(object))) \
		$(foreach object,$(GENOBJCOBJECTS),$(call gb_GenObjCObject_get_dep_target,$(object))) \
		$(foreach object,$(GENOBJCOBJECTS),$(call gb_GenObjCObject_get_dwo_target,$(object))) \
		$(foreach object,$(GENOBJCXXOBJECTS),$(call gb_GenObjCxxObject_get_target,$(object))) \
		$(foreach object,$(GENOBJCXXOBJECTS),$(call gb_GenObjCxxObject_get_dep_target,$(object))) \
		$(foreach object,$(GENOBJCXXOBJECTS),$(call gb_GenObjCxxObject_get_dwo_target,$(object))) \
		$(foreach object,$(GENASMOBJECTS),$(call gb_GenAsmObject_get_target,$(object))) \
		$(foreach object,$(GENASMOBJECTS),$(call gb_GenAsmObject_get_dep_target,$(object))) \
		$(foreach object,$(GENASMOBJECTS),$(call gb_GenAsmObject_get_dwo_target,$(object))) \
		$(foreach object,$(GENNASMOBJECTS),$(call gb_GenNasmObject_get_target,$(object))) \
		$(foreach object,$(GENNASMOBJECTS),$(call gb_GenNasmObject_get_dep_target,$(object))) \
		$(foreach object,$(GENNASMOBJECTS),$(call gb_GenNasmObject_get_dwo_target,$(object))) \
		$(foreach object,$(GENCXXCLROBJECTS),$(call gb_GenCxxClrObject_get_target,$(object))) \
		$(foreach object,$(GENCXXCLROBJECTS),$(call gb_GenCxxClrObject_get_dep_target,$(object))) \
		$(foreach object,$(GENCXXCLROBJECTS),$(call gb_GenCxxClrObject_get_dwo_target,$(object))) \
		$(call gb_LinkTarget_get_target,$(LINKTARGET)) \
		$(call gb_LinkTarget_get_dep_target,$(LINKTARGET)) \
		$(if $(gb_PARTIAL_BUILD),, \
			$(call gb_LinkTarget_get_dep_libraries_target,$(LINKTARGET)) \
			$(call gb_LinkTarget_get_dep_externals_target,$(LINKTARGET)) \
			$(call gb_LinkTarget_get_dep_statics_target,$(LINKTARGET)) \
			) \
		$(call gb_LinkTarget_get_headers_target,$(LINKTARGET)) \
		$(call gb_LinkTarget_get_objects_list,$(LINKTARGET)) \
		$(call gb_LinkTarget_get_pch_timestamp,$(LINKTARGETMAKEFILENAME)) \
		$(call gb_LinkTarget_get_pch_reuse_timestamp,$(LINKTARGETMAKEFILENAME)) \
		$(ILIBTARGET) \
		$(AUXTARGETS)) && \
		cat $${RESPONSEFILE} /dev/null | $(if $(filter WNT,$(OS)),env -i PATH="$$PATH") xargs -n 200 rm -fr && \
		rm -f $${RESPONSEFILE}


# cat the deps of all objects in one file, then we need only open that one file
# call gb_LinkTarget__command_dep,dep_target,linktargetname
define gb_LinkTarget__command_dep
$(call gb_Output_announce,LNK:$(2).d,$(true),DEP,1)
	$(call gb_Trace_StartRange,LNK:$(2),DEP)
$(call gb_Helper_abbreviate_dirs,\
	mkdir -p $(dir $(1)) && \
	RESPONSEFILE=$(call gb_var2file,$(shell $(gb_MKTEMP)),\
		$(foreach object,$(COBJECTS),$(call gb_CObject_get_dep_target,$(object))) \
		$(foreach object,$(CXXOBJECTS),$(call gb_CxxObject_get_dep_target,$(object))) \
		$(foreach object,$(OBJCOBJECTS),$(call gb_ObjCObject_get_dep_target,$(object)))\
		$(foreach object,$(OBJCXXOBJECTS),$(call gb_ObjCxxObject_get_dep_target,$(object)))\
		$(foreach object,$(CXXCLROBJECTS),$(call gb_CxxClrObject_get_dep_target,$(object)))\
		$(foreach object,$(ASMOBJECTS),$(call gb_AsmObject_get_dep_target,$(object)))\
		$(foreach object,$(GENCOBJECTS),$(call gb_GenCObject_get_dep_target,$(object))) \
		$(foreach object,$(GENCXXOBJECTS),$(call gb_GenCxxObject_get_dep_target,$(object))) \
		$(foreach object,$(GENOBJCOBJECTS),$(call gb_GenObjCObject_get_dep_target,$(object))) \
		$(foreach object,$(GENOBJCXXOBJECTS),$(call gb_GenObjCxxObject_get_dep_target,$(object))) \
		$(foreach object,$(GENASMOBJECTS),$(call gb_GenAsmObject_get_dep_target,$(object))) \
		$(foreach object,$(GENNASMOBJECTS),$(call gb_GenNasmObject_get_dep_target,$(object))) \
		$(foreach object,$(GENCXXCLROBJECTS),$(call gb_GenCxxClrObject_get_dep_target,$(object))) \
		) && \
	SYSTEM_BOOST="$(SYSTEM_BOOST)" $(call gb_Executable_get_command,concat-deps) $${RESPONSEFILE} > $(1)) && \
	rm -f $${RESPONSEFILE}
	$(call gb_Trace_EndRange,LNK:$(2),DEP)

endef

# call gb_LinkTarget__command_objectlist,linktarget
define gb_LinkTarget__command_objectlist
$(file >$(1),\
	$(foreach object,$(COBJECTS),$(call gb_CObject_get_target,$(object))) \
	$(foreach object,$(CXXOBJECTS),$(call gb_CxxObject_get_target,$(object))) \
	$(foreach object,$(OBJCOBJECTS),$(call gb_ObjCObject_get_target,$(object))) \
	$(foreach object,$(OBJCXXOBJECTS),$(call gb_ObjCxxObject_get_target,$(object))) \
	$(foreach object,$(CXXCLROBJECTS),$(call gb_CxxClrObject_get_target,$(object))) \
	$(foreach object,$(ASMOBJECTS),$(call gb_AsmObject_get_target,$(object))) \
	$(foreach object,$(GENCOBJECTS),$(call gb_GenCObject_get_target,$(object))) \
	$(foreach object,$(GENCXXOBJECTS),$(call gb_GenCxxObject_get_target,$(object))) \
	$(foreach object,$(GENOBJCOBJECTS),$(call gb_GenObjCObject_get_target,$(object))) \
	$(foreach object,$(GENOBJCXXOBJECTS),$(call gb_GenObjCxxObject_get_target,$(object))) \
	$(foreach object,$(GENASMOBJECTS),$(call gb_GenAsmObject_get_target,$(object))) \
	$(foreach object,$(GENNASMOBJECTS),$(call gb_GenNasmObject_get_target,$(object))) \
	$(foreach object,$(GENCXXCLROBJECTS),$(call gb_GenCxxClrObject_get_target,$(object))) \
	$(PCHOBJS) \
	$(foreach extraobjectlist,$(EXTRAOBJECTLISTS),$(shell cat $(extraobjectlist))))

endef

$(WORKDIR)/LinkTarget/%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

# Target for the .exports of the shared library, to speed up incremental build.
# This deliberately does nothing if the file exists; the file is actually
# written in gb_LinkTarget__command_dynamiclink.
# Put this pattern rule here so it overrides the one below.
# (this is rather ugly: because of % the functions cannot be used)
$(gb_Library_DLLDIR)/%.exports :
	$(if $(wildcard $@),,mkdir -p $(dir $@) && touch $@)

# This recipe actually also builds the dep-target as a side-effect, which
# is an optimization to reduce incremental build time.
# (with exception for concat-dep executable itself which does not exist yet...)
$(WORKDIR)/LinkTarget/% : $(gb_Helper_MISCDUMMY)
	$(call gb_LinkTarget__command_impl,$@,$*)

# call gb_LinkTarget__make_installed_rule,linktarget
define gb_LinkTarget__make_installed_rule
$(call gb_LinkTarget_get_target,$(1)) : $(call gb_LinkTarget_get_headers_target,$(1))
	$$(call gb_LinkTarget__command_impl,$(call gb_LinkTarget_get_target,$(1)),$(call gb_LinkTarget__get_workdir_linktargetname,$(1)))

endef

define gb_LinkTarget__add_linked_libs
$(call gb_LinkTarget_get_target,$(1)) : LINKED_LIBS += $(2)

endef

# it's not possible to use a pattern rule for files in INSTDIR because
# it would inevitably conflict with the pattern rule for Package
# (especially since external libraries are delivered via Package)
# call gb_LinkTarget__command_impl,linktargettarget,linktargetname
define gb_LinkTarget__command_impl
	$(if $(gb_FULLDEPS),
		$(if $(ENABLE_CUSTOMTARGET_COMPONENTS),$(if $(gb_PARTIAL_BUILD),,
			$(call gb_LinkTarget__command_dep_libraries,$(call gb_LinkTarget_get_dep_libraries_target,$(2)).tmp,$(2))
			mv $(call gb_LinkTarget_get_dep_libraries_target,$(2)).tmp $(call gb_LinkTarget_get_dep_libraries_target,$(2))
			$(call gb_LinkTarget__command_dep_externals,$(call gb_LinkTarget_get_dep_externals_target,$(2)).tmp,$(2))
			mv $(call gb_LinkTarget_get_dep_externals_target,$(2)).tmp $(call gb_LinkTarget_get_dep_externals_target,$(2))
			$(call gb_LinkTarget__command_dep_statics,$(call gb_LinkTarget_get_dep_statics_target,$(2)).tmp,$(2))
			mv $(call gb_LinkTarget_get_dep_statics_target,$(2)).tmp $(call gb_LinkTarget_get_dep_statics_target,$(2))))
		$(if $(findstring concat-deps,$(2)),,
			$(call gb_LinkTarget__command_dep,$(call gb_LinkTarget_get_dep_target,$(2)).tmp,$(2))
			mv $(call gb_LinkTarget_get_dep_target,$(2)).tmp $(call gb_LinkTarget_get_dep_target,$(2))))
	$(if $(filter $(2),$(foreach lib,$(gb_MERGEDLIBS),$(call gb_Library__get_workdir_linktargetname,$(lib)))),
		$(if $(filter $(true),$(call gb_LinkTarget__is_build_lib,$(2))),
			$(call gb_LinkTarget__command,$(1),$(2)),
			mkdir -p $(dir $(1)) && echo invalid - merged lib > $(1)
			$(if $(SOVERSIONSCRIPT),&& echo invalid - merged lib > $(WORKDIR)/LinkTarget/$(2))),
		$(if $(filter-out CompilerTest,$(TARGETTYPE)),
			$(call gb_LinkTarget__command,$(1),$(2))))
	$(call gb_LinkTarget__command_objectlist,$(WORKDIR)/LinkTarget/$(2).objectlist)
endef

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,%) : $(call gb_Executable_get_runtime_dependencies,concat-deps)
	$(call gb_LinkTarget__command_dep,$@,$*)

$(dir $(call gb_LinkTarget_get_dep_target,%))/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

ifeq ($(ENABLE_CUSTOMTARGET_COMPONENTS),TRUE)
ifeq (,$(gb_PARTIAL_BUILD))

define gb_LinkTarget__static_dep_x_template

define gb_LinkTarget__command_dep_$(1)
$$(call gb_Output_announce,LNK:$$(2).d.$(1),$$(true),DEP,1)
$$(shell mkdir -p $$(dir $$(1)))
$$(file >$$(1).tmp,$$(call gb_LinkTarget__get_all_$(1),$$(2)))
$$(call gb_Helper_replace_if_different_and_touch,$$(1).tmp,$$(1))

endef

$$(call gb_LinkTarget_get_dep_$(1)_target,%) : ;
	$$(call gb_LinkTarget__command_dep_$(1),$$@,$$*)

endef # gb_LinkTarget__static_dep_x_template

$(eval $(call gb_LinkTarget__static_dep_x_template,libraries))
$(eval $(call gb_LinkTarget__static_dep_x_template,externals))
$(eval $(call gb_LinkTarget__static_dep_x_template,statics))

endif # !gb_PARTIAL_BUILD
endif # ENABLE_CUSTOMTARGET_COMPONENTS
endif # gb_FULLDEPS

# Ok, this is some dark voodoo: When declaring a linktarget with
# gb_LinkTarget_LinkTarget we set SELF in the headertarget to name of the
# target. When the rule for the headertarget is executed and SELF does not
# match the target name, we are depending on a linktarget that was never
# declared. In a full build exclusively in gbuild that should never happen.
define gb_LinkTarget__get_headers_check
ifneq ($$(SELF),$$*)
$$(eval $$(call gb_Output_error,used LinkTarget $$* not defined))
endif
$$@ : COMMAND := $$(call gb_Helper_abbreviate_dirs, touch $$@)

endef

$(WORKDIR)/Headers/%/.dir :
	$(if $(wildcard $(dir $@)),,mkdir -p $(dir $@))

# sadly because of the subdirectories can't have pattern deps on .dir here
$(WORKDIR)/Headers/% :
	$(eval $(gb_LinkTarget__get_headers_check))
	$(COMMAND)

# Explanation of some of the targets:
# - gb_LinkTarget_get_headers_target is the target that guarantees all headers
#   from the linked against libraries and the linktargets own generated headers
#   are generated.
# - gb_LinkTarget_get_target links the objects into a file in WORKDIR.
# gb_LinkTarget_get_target depends on gb_LinkTarget_get_headers_target.
# gb_LinkTarget_get_target depends additionally on the objects, which in turn
# depend build-order only on the gb_LinkTarget_get_headers_target. The build
# order-only dependency ensures all headers to be there for compiling and
# dependency generation without causing all objects to be rebuild when one
# header changes. Only the ones with an explicit dependency in their generated
# dependency file will be rebuild.
#
# gb_LinkTarget_get_target is the target that links the objects into a file in
# WORKDIR
# Explanation of some of the variables:
# - AUXTARGETS are the additionally generated files that need to be cleaned out
#   on clean.
# - PCH_CXXFLAGS and PCH_DEFS are the flags that the precompiled headers will
#   be compiled with.  They should never be overridden in a single object
#   files.
# - TARGETTYPE is the type of linktarget as some platforms need very different
#   command to link different targettypes.
# - LIBRARY_X64 is only relevant for building a x64 library on windows.
# - PE_X86 is only relevant for building a x86 binaries on Windows.
#
# Since most variables are set on the linktarget and not on the object, the
# object learns about these setting via GNU makes scoping of target variables.
# Therefore it is important that objects are only directly depended on by the
# linktarget. This for example means that you cannot build a single object
# alone, because then you would directly depend on the object.
#
# A note about flags: because the overriding the global variables with a target
# local variable of the same name is considered obscure, the target local
# variables have a T_ prefix.
#
# call gb_LinkTarget_LinkTarget,linktarget,linktargetmakefilename,layer
define gb_LinkTarget_LinkTarget
$(call gb_LinkTarget_get_clean_target,$(1)) : LINKTARGET := $(1)
$(call gb_LinkTarget_get_clean_target,$(1)) : LINKTARGETMAKEFILENAME := $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : AUXTARGETS :=
$(call gb_LinkTarget_get_headers_target,$(1)) : SELF := $(call gb_LinkTarget__get_workdir_linktargetname,$(1))
$(call gb_LinkTarget_get_headers_target,$(1)) : \
	| $(dir $(call gb_LinkTarget_get_headers_target,$(1))).dir \
	  $(dir $(call gb_LinkTarget_get_target,$(1))).dir \
	  $(dir $(WORKDIR)/LinkTarget/$(call gb_LinkTarget__get_workdir_linktargetname,$(1))).dir
$(call gb_LinkTarget_get_target,$(1)) : ILIBTARGET :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : COBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : CXXOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : YACCOBJECT :=
$(call gb_LinkTarget_get_target,$(1)) : T_YACCFLAGS := $$(gb_LinkTarget_YYACFLAGS) $(YACCFLAGS)
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : LEXOBJECT :=
$(call gb_LinkTarget_get_target,$(1)) : T_LEXFLAGS := $$(gb_LinkTarget_LEXFLAGS) $(LEXFLAGS)
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : OBJCOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : OBJCXXOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : CXXCLROBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : ASMOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : GENCOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : GENCXXOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : GENOBJCOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : GENASMOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : GENNASMOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : GENOBJCXXOBJECTS :=
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : GENCXXCLROBJECTS :=
$(call gb_LinkTarget_get_target,$(1)) : T_CFLAGS := $$(gb_LinkTarget_CFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : T_CFLAGS_APPEND :=
$(call gb_LinkTarget_get_target,$(1)) : T_CXXFLAGS := $$(gb_LinkTarget_CXXFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : T_CXXFLAGS_APPEND :=
$(call gb_LinkTarget_get_target,$(1)) : PCH_CXXFLAGS := $$(gb_LinkTarget_CXXFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : OBJECT_HAS_EXTRA_CXXFLAGS :=
$(call gb_LinkTarget_get_target,$(1)) : T_OBJCXXFLAGS := $$(gb_LinkTarget_OBJCXXFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : T_OBJCXXFLAGS_APPEND :=
$(call gb_LinkTarget_get_target,$(1)) : T_OBJCFLAGS := $$(gb_LinkTarget_OBJCFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : T_OBJCFLAGS_APPEND :=
$(call gb_LinkTarget_get_target,$(1)) : T_NASMFLAGS := $$(NAFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : T_NASMFLAGS_APPEND :=
$(call gb_LinkTarget_get_target,$(1)) : T_CXXCLRFLAGS := $$(gb_LinkTarget_CXXCLRFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : T_CXXCLRFLAGS_APPEND :=
$(call gb_LinkTarget_get_target,$(1)) : DEFS := $$(gb_LinkTarget_DEFAULTDEFS) $(CPPFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : PCH_DEFS := $$(gb_LinkTarget_DEFAULTDEFS) $(CPPFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : INCLUDE := -I$$(SRCDIR)/include $$(gb_LinkTarget_INCLUDE)
$(call gb_LinkTarget_get_target,$(1)) : T_LDFLAGS := $$(gb_LinkTarget_LDFLAGS) $(call gb_LinkTarget_get_linksearchpath_for_layer,$(3)) $(call gb_LinkTarget__get_ldflags,$(2))
$(call gb_LinkTarget_get_target,$(1)) : LINKED_LIBS :=
$(call gb_LinkTarget_get_target,$(1)) : LINKED_STATIC_LIBS :=
$(call gb_LinkTarget_get_target,$(1)) : T_LIBS :=
$(call gb_LinkTarget_get_target,$(1)) : T_STDLIBS_CXX := $(gb_STDLIBS_CXX)
$(call gb_LinkTarget_get_target,$(1)) : TARGETTYPE :=
$(call gb_LinkTarget_get_target,$(1)) : LIBRARY_X64 :=
$(call gb_LinkTarget_get_target,$(1)) : PCH_NAME :=
$(call gb_LinkTarget_get_target,$(1)) : PCH_HEADER :=
$(call gb_LinkTarget_get_target,$(1)) : PCH_LINKTARGETMAKEFILENAME :=
$(call gb_LinkTarget_get_target,$(1)) : PCHOBJS :=
$(call gb_LinkTarget_get_target,$(1)) : PCHOBJEX :=
$(call gb_LinkTarget_get_target,$(1)) : PCHOBJNOEX :=
$(call gb_LinkTarget_get_target,$(1)) : T_PCH_EXTRA_CXXFLAGS :=
$(call gb_LinkTarget_get_target,$(1)) : PE_X86 :=
$(call gb_LinkTarget_get_target,$(1)) : PDBFILE :=
$(call gb_LinkTarget_get_target,$(1)) : TARGETGUI :=
$(call gb_LinkTarget_get_target,$(1)) : EXTRAOBJECTLISTS :=
$(call gb_LinkTarget_get_target,$(1)) : NATIVERES :=
$(call gb_LinkTarget_get_target,$(1)) : VISIBILITY :=
$(call gb_LinkTarget_get_target,$(1)) : WARNINGS_NOT_ERRORS :=
$(call gb_LinkTarget_get_target,$(1)) : WARNINGS_DISABLED :=
$(call gb_LinkTarget_get_target,$(1)) : PLUGIN_WARNINGS_AS_ERRORS :=
$(call gb_LinkTarget_get_target,$(1)) : EXTERNAL_CODE :=
$(call gb_LinkTarget_get_target,$(1)) : SOVERSIONSCRIPT :=
$(call gb_LinkTarget_get_target,$(1)) : COMPILER_TEST :=
$(call gb_LinkTarget_get_target,$(1)) : T_SYMBOLS := $(if $(call gb_target_symbols_enabled,$(2)),$(true),$(false))
$(call gb_LinkTarget_get_target,$(1)) : T_FORCE_COMPILE := $(if $(call gb_LinkTarget__force_compile,$(2)),$(true),$(false))
$(call gb_LinkTarget_get_target,$(1)) : T_CC :=
$(call gb_LinkTarget_get_target,$(1)) : T_CXX :=
$(call gb_LinkTarget_get_target,$(1)) : T_USE_LD := $(USE_LD)
$(call gb_LinkTarget_get_target,$(1)) : T_LTOFLAGS := $(gb_LTOFLAGS)
$(call gb_LinkTarget_get_target,$(1)) : T_PREJS :=

ifeq ($(gb_FULLDEPS),$(true))
ifeq (depcache:,$(filter depcache,$(.FEATURES)):$(gb_PARTIAL_BUILD))
-includedepcache $(call gb_LinkTarget_get_dep_target,$(1))
else
-include $(call gb_LinkTarget_get_dep_target,$(1))
endif
$(call gb_LinkTarget_get_dep_target,$(1)) : COBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : CXXOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : OBJCOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : OBJCXXOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : CXXCLROBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : ASMOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : GENCOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : GENCXXOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : GENOBJCOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : GENOBJCXXOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : GENASMOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : GENNASMOBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : GENCXXCLROBJECTS :=
$(call gb_LinkTarget_get_dep_target,$(1)) : YACCOBJECTS :=
endif # gb_FULLDEPS

gb_LinkTarget_CXX_SUFFIX_$(call gb_LinkTarget__get_workdir_linktargetname,$(1)) := cxx

# installed linktargets need a rule to build!
$(if $(findstring $(INSTDIR),$(1)),$(call gb_LinkTarget__make_installed_rule,$(1)))

$(call gb_PrecompiledHeader_generate_timestamp_rule,$(2))

endef # gb_LinkTarget_LinkTarget

# call gb_LinkTarget_set_soversion_script,linktarget,soversionscript
define gb_LinkTarget_set_soversion_script
$(call gb_LinkTarget_get_target,$(1)) : $(2)
$(call gb_LinkTarget_get_target,$(1)) : SOVERSIONSCRIPT := $(2)

endef

# call gb_LinkTarget_add_defs,linktarget,defines
define gb_LinkTarget_add_defs
$(call gb_LinkTarget_get_target,$(1)) : DEFS += $(2)
$(call gb_LinkTarget_get_target,$(1)) : PCH_DEFS += $(2)
endef

# call gb_LinkTarget_add_cflags,linktarget,cflags
define gb_LinkTarget_add_cflags
$(call gb_LinkTarget_get_target,$(1)) : T_CFLAGS_APPEND += $(2)
endef

# call gb_LinkTarget_add_cxxflags,linktarget,cxxflags
define gb_LinkTarget_add_cxxflags
$(call gb_LinkTarget_get_target,$(1)) : T_CXXFLAGS_APPEND += $(2)
$(call gb_LinkTarget_get_target,$(1)) : PCH_CXXFLAGS += $(2)
endef

# call gb_LinkTarget_add_objcxxflags,linktarget,objcxxflags
define gb_LinkTarget_add_objcxxflags
$(call gb_LinkTarget_get_target,$(1)) : T_OBJCXXFLAGS_APPEND += $(2)
endef

# call gb_LinkTarget_add_objcflags,linktarget,objcflags
define gb_LinkTarget_add_objcflags
$(call gb_LinkTarget_get_target,$(1)) : T_OBJCFLAGS_APPEND += $(2)

endef

# call gb_LinkTarget_add_nasmflags,linktarget,nasmflags
define gb_LinkTarget_add_nasmflags
$(call gb_LinkTarget_get_target,$(1)) : T_NASMFLAGS_APPEND += $(2)
endef

# call gb_LinkTarget_add_cxxclrflags,linktarget,cxxclrflags
define gb_LinkTarget_add_cxxclrflags
$(call gb_LinkTarget_get_target,$(1)) : T_CXXCLRFLAGS_APPEND += $(2)
endef

# call gb_LinkTarget__add_include,linktarget,includes
define gb_LinkTarget__add_include
$(call gb_LinkTarget_get_target,$(1)) : INCLUDE += -I$(2)

endef

# call gb_LinkTarget__check_srcdir_paths,linktarget,includepaths
define gb_LinkTarget__check_srcdir_paths
$(if $(filter-out $(wildcard $(2)),$(2)),\
    $(call gb_Output_error,gb_LinkTarget_set_include: include paths $(filter-out $(wildcard $(2)),$(2)) do not exist) \
)
endef

# call gb_LinkTarget_set_include,linktarget,includes
define gb_LinkTarget_set_include
$(call gb_LinkTarget__check_srcdir_paths,$(1),\
    $(patsubst -I%,%,$(filter -I$(SRCDIR)/%,$(filter-out -I$(INSTDIR)/% -I$(WORKDIR)/%,$(2)))))
$(call gb_LinkTarget_get_target,$(1)) : INCLUDE := $(2)

endef

# call gb_LinkTarget_add_ldflags,linktarget,ldflags
define gb_LinkTarget_add_ldflags
$(call gb_LinkTarget_get_target,$(1)) : T_LDFLAGS += $(2)

endef

# real use in RepositoryExternal.mk
# call gb_LinkTarget_set_ldflags,linktarget,ldflags
define gb_LinkTarget_set_ldflags
$(call gb_LinkTarget_get_target,$(1)) : T_LDFLAGS := $(2)

endef

# call gb_LinkTarget_add_libs,linktarget,libs
define gb_LinkTarget_add_libs
$(call gb_LinkTarget_get_target,$(1)) : T_LIBS += $(2)
$(if $(call gb_LinkTarget__is_merged,$(1)),\
  $(call gb_Library_get_linktarget_target,merged) : T_LIBS += $(2))
ifeq ($(ENABLE_CUSTOMTARGET_COMPONENTS),TRUE)
$(if $(gb_DEBUG_STATIC),$$(info $$(call gb_LinkTarget__get_all_libraries_var,$(1)) += $(filter-out $(call gb_LinkTarget__get_all_libraries,$(1)),$(patsubst %,$(gb_LinkTarget__syslib),$(2)))))
$$(eval $$(call gb_LinkTarget__get_all_libraries_var,$(1)) += $(filter-out $(call gb_LinkTarget__get_all_libraries,$(1)),$(patsubst %,$(gb_LinkTarget__syslib),$(2))))
ifeq (,$(gb_PARTIAL_BUILD))
$(call gb_LinkTarget_get_target,$(1)) : LINKED_LIBS += $(filter-out $(call gb_LinkTarget__get_all_libraries,$(1)),$(patsubst %,$(gb_LinkTarget__syslib),$(2)))
endif
endif

endef

# remove platform specific standard libraries for linktarget $(1)
# assumption is that adding these standard libs is always useful, but in very
# exceptional cases this disable method may be used
# call gb_LinkTarget_disable_standard_system_libs,linktarget
define gb_LinkTarget_disable_standard_system_libs
$(call gb_LinkTarget_get_target,$(1)) : T_LIBS := $$(filter-out $$(gb_STDLIBS),$$(T_LIBS))
$(call gb_LinkTarget_get_target,$(1)) : T_STDLIBS_CXX :=

endef

# call gb_LinkTarget__use_api,linktarget,api
define gb_LinkTarget__use_api
$(call gb_LinkTarget_get_headers_target,$(1)) : $(call gb_UnoApiHeadersTarget_get_target,$(2))
$(call gb_LinkTarget__add_include,$(1),$(call gb_UnoApiHeadersTarget_get_dir,$(2)))

endef

# call gb_LinkTarget_use_api,linktarget,apis
define gb_LinkTarget_use_api
$(foreach api,$(2),$(call gb_LinkTarget__use_api,$(1),$(api)))

endef

# call gb_LinkTarget_use_udk_api,linktarget
define gb_LinkTarget_use_udk_api
$(call gb_LinkTarget__use_api,$(1),udkapi)
endef

# call gb_LinkTarget_use_sdk_api,linktarget
define gb_LinkTarget_use_sdk_api
$(call gb_LinkTarget__use_api,$(1),udkapi)
$(call gb_LinkTarget__use_api,$(1),offapi)
endef

# call gb_LinkTarget__use_internal_api_one,linktarget,api,apiprefix
define gb_LinkTarget__use_internal_api_one
$(call gb_LinkTarget_get_headers_target,$(1)) :| \
	$(call gb_UnoApiHeadersTarget_get_$(3)target,$(2))
$(call gb_LinkTarget__add_include,$(1),$(call gb_UnoApiHeadersTarget_get_$(3)dir,$(2)))

endef

# call gb_LinkTarget__use_internal_api,linktarget,apis,apiprefix
define gb_LinkTarget__use_internal_api
$(foreach api,$(2),$(call gb_LinkTarget__use_internal_api_one,$(1),$(api),$(3)))

endef

# call gb_LinkTarget_use_internal_api,linktarget,api
define gb_LinkTarget_use_internal_api
$(call gb_LinkTarget__use_internal_api,$(1),$(2))

endef

# call gb_LinkTarget_use_internal_bootstrap_api,linktarget,api
define gb_LinkTarget_use_internal_bootstrap_api
$(call gb_LinkTarget__use_internal_api,$(1),$(2),bootstrap_)

endef

# call gb_LinkTarget_use_internal_comprehensive_api,linktarget,api
define gb_LinkTarget_use_internal_comprehensive_api
$(call gb_LinkTarget__use_internal_api,$(1),$(2),comprehensive_)

endef

define gb_PrintDeps_info
$(info LibraryDep: $(1) links against $(2))
endef

# returns $(true), if the target class really calls a linker.
# call gb_LinkTarget_does_real_link,linktarget
gb_LinkTarget_does_real_link = $(if $(filter Executable CppunitTest $(if $(DISABLE_DYNLOADING),,Library), \
    $(call gb_LinkTarget__get_workdir_linktargetclass,$(1))),$(true))

# avoid problem when a module is built partially but other modules that define
# needed libraries is not yet built: prevent invocation of pattern rule
# for library with invalid parameters by depending on the header target
define gb_LinkTarget__lib_dummy_depend
$(call gb_Library_get_target,$(1)) :| $(call gb_Library_get_headers_target,$(1))

endef

define gb_LinkTarget__generate_all_x_accessors
gb_LinkTarget__get_all_$(1)_var = $$(call gb_LinkTarget__get_workdir_linktargetname,$$(1))<>ALL_$(2)
gb_LinkTarget__get_all_$(1) = $$($$(call gb_LinkTarget__get_all_$(1)_var,$$(1)))
gb_Library__get_all_$(1) = $$($$(call gb_LinkTarget__get_all_$(1)_var,$$(call gb_Library_get_linktarget,$$(1))))
gb_Executable__get_all_$(1) = $$($$(call gb_LinkTarget__get_all_$(1)_var,$$(call gb_Executable_get_linktarget,$$(1))))
gb_ExternalProject__get_all_$(1) = $$($$(call gb_LinkTarget__get_all_$(1)_var,$$(call gb_ExternalProject__get_workdir_linktargetname,$$(1))))
gb_CppunitTest__get_all_$(1) = $$($$(call gb_LinkTarget__get_all_$(1)_var,$$(call gb_CppunitTest__get_workdir_linktargetname,$$(1))))

endef

$(eval $(call gb_LinkTarget__generate_all_x_accessors,libraries,LIBRARIES))
gb_LinkTarget__filter_lo_libraries = $(filter-out $(gb_LinkTarget__syslib),$(1))
gb_LinkTarget__get_all_lo_libraries = $(call gb_LinkTarget__filter_lo_libraries,$(call gb_LinkTarget__get_all_libraries,$(1)))
gb_LinkTarget__filter_sys_libraries = $(filter $(gb_LinkTarget__syslib),$(1))
gb_LinkTarget__get_all_sys_libraries = $(call gb_LinkTarget__filter_sys_libraries,$(call gb_LinkTarget__get_all_libraries,$(1)))
$(eval $(call gb_LinkTarget__generate_all_x_accessors,externals,EXTERNALS))
$(eval $(call gb_LinkTarget__generate_all_x_accessors,statics,STATICS))

# call gb_LinkTarget__register_type,type,linktarget,type list
define gb_LinkTarget__register_type
ifeq ($(ENABLE_CUSTOMTARGET_COMPONENTS),TRUE)
ifeq (,$(DISABLE_DYNLOADING))
$$(error ENABLE_CUSTOMTARGET_COMPONENTS just works with DISABLE_DYNLOADING)
endif
$(foreach type,$(3),$(if $(filter $(type),$(call gb_LinkTarget__get_all_$(1),$(2))),, \
    $(if $(gb_DEBUG_STATIC),$$(info $(call gb_LinkTarget__get_all_$(1)_var,$(2)) += $(type))) \
    $$(eval $(call gb_LinkTarget__get_all_$(1)_var,$(2)) += $(type)) \
))
endif

endef

# call gb_LinkTarget__use_libraries,linktarget,requestedlibs,actuallibs,linktargetmakefilename
define gb_LinkTarget__use_libraries

# used by bin/module-deps.pl
ifneq ($(ENABLE_PRINT_DEPS),)
# exclude libraries in Library_merged
ifeq ($(filter $(1),$(foreach lib,$(gb_MERGEDLIBS),$(call gb_Library_get_linktarget,$(lib)))),)
$$(eval $$(call gb_PrintDeps_info,$(4),$(3)))
endif
endif

$(call gb_LinkTarget_get_target,$(1)) : LINKED_LIBS += $(3)

ifeq (,$(DISABLE_DYNLOADING))
# depend on the exports of the library, not on the library itself
# for faster incremental builds when the ABI is unchanged.
# export files are created from the library, so this also ensures the library exists.
$(foreach lib,$(call gb_LinkTarget__filter_lo_libraries,$(3)),$(if $(filter $(lib),$(gb_Library_KNOWNLIBS)), \
    $$(eval $(call gb_LinkTarget_get_target,$(1)) : $(call gb_Library_get_exports_target,$(lib))) \
))

else # DISABLE_DYNLOADING
# depend on the now-static libraries themself, but only if the target actually links to it
ifneq (,$(call gb_LinkTarget_does_real_link,$(1)))
$(call gb_LinkTarget_get_target,$(1)) : T_LIBS += $(call gb_LinkTarget__filter_sys_libraries,$(3))
$(if $(filter-out Library,gb_LinkTarget__get_workdir_linktargetclass,$(1)), \
    $(foreach lib,$(call gb_LinkTarget__filter_lo_libraries,$(3)),$(if $(filter $(lib),$(gb_Library_KNOWNLIBS)), \
        $$(eval $(call gb_LinkTarget_get_target,$(1)) : $(call gb_Library_get_linktarget_target,$(lib))) \
    )))
endif
endif # DISABLE_DYNLOADING

$(call gb_LinkTarget__register_type,libraries,$(1),$(3))

$(foreach lib,$(call gb_LinkTarget__filter_lo_libraries,$(2)),$(if $(filter $(lib),$(gb_Library_KNOWNLIBS)), \
    $(eval $(call gb_LinkTarget_get_headers_target,$(1)) : $(call gb_Library_get_headers_target,$(lib))) \
    $(call gb_LinkTarget__lib_dummy_depend,$(lib)) \
))

endef # gb_LinkTarget__use_libraries

# libraries which are merged but need to be built for gb_BUILD_HELPER_TOOLS
gb_BUILD_HELPER_LIBS := $(foreach lib, \
    basegfx \
	comphelper \
	cppu \
	cppuhelper \
	i18nlangtag \
	reg \
	sal \
	salhelper \
	sax \
	store \
	tl \
	ucbhelper \
	unoidl \
	xmlreader \
    , $(call gb_Library__get_workdir_linktargetname,$(lib)))

# tools libmerged depends on, so they link against gb_BUILD_HELPER_LIBS
gb_BUILD_HELPER_TOOLS := $(foreach exe,\
	cppumaker \
	svidl \
	unoidl-check \
	unoidl-write \
	, $(call gb_Executable__get_workdir_linktargetname,$(exe)))

# call gb_LinkTarget__is_build_lib,linktargetname
define gb_LinkTarget__is_build_lib
$(if $(filter $(call gb_LinkTarget__get_workdir_linktargetname,$(1)),$(call gb_BUILD_HELPER_LIBS)),$(true))
endef

# call gb_LinkTarget__is_build_tool,linktargetname
define gb_LinkTarget__is_build_tool
$(if $(filter $(call gb_LinkTarget__get_workdir_linktargetname,$(1)),$(call gb_BUILD_HELPER_TOOLS)),$(true))
endef

define gb_LinkTarget__is_merged
$(filter $(1),$(foreach lib,$(gb_MERGEDLIBS),$(call gb_Library_get_linktarget,$(lib))))
endef

# call gb_LinkTarget_use_libraries,linktarget,libs
define gb_LinkTarget_use_libraries
ifneq (,$$(filter-out $(gb_Library_KNOWNLIBS) $(gb_LinkTarget__syslib),$(2)))
$$(eval $$(call gb_Output_info,currently known libraries are: $(sort $(gb_Library_KNOWNLIBS)),ALL))
$$(eval $$(call gb_Output_error,Cannot link against library/libraries '$$(filter-out $(gb_Library_KNOWNLIBS) $(gb_LinkTarget__syslib),$(2))'. Libraries must be registered in Repository.mk or RepositoryExternal.mk))
endif
ifneq (,$$(filter $(2),$(gb_Library_KNOWNPLUGINS)))
ifneq (,$$(filter $(1),$$(foreach plugin,$(gb_Library_KNOWNPLUGINS),$(call gb_Library__get_workdir_linktargetname,$(plugin)))))
$$(eval $$(call gb_Output_error,Cannot link against plugin library/libraries '$$(filter $(2),$(gb_Library_KNOWNPLUGINS))'. Only plugins are allowed to do that.))
endif
endif

ifeq ($(call gb_LinkTarget__is_build_tool,$(1))$(call gb_LinkTarget__is_build_lib,$(1)),$(true))
$(call gb_LinkTarget__use_libraries,$(1),$(2),$(2),$(4))
else
# $$(3) = Always just depend on non-merged libs. If any dependency is merged, but you aren't, also depend on "merged".
$(call gb_LinkTarget__use_libraries,$(1),$(2),$(strip \
        $(filter-out $(gb_MERGEDLIBS),$(2)) \
        $(if $(filter $(gb_MERGEDLIBS),$(2)),$(if $(call gb_LinkTarget__is_merged,$(1)),,merged)) \
    ),$(4))
endif

endef

# avoid problem when a module is built partially but other modules that define
# needed static libraries is not yet built: prevent invocation of pattern rule
# for static library with invalid parameters by depending on the header target
define gb_LinkTarget__static_lib_dummy_depend
$(call gb_StaticLibrary_get_target,$(1)) :| \
	$(call gb_StaticLibrary_get_headers_target,$(1))

endef

# for a StaticLibrary, dependent libraries are not actually linked in
# call gb_LinkTarget_use_static_libraries,linktarget,staticlibs
define gb_LinkTarget_use_static_libraries
$(call gb_LinkTarget_get_target,$(1)) : LINKED_STATIC_LIBS += $$(if $$(filter-out StaticLibrary,$$(TARGETTYPE)),$(2))
$(if $(call gb_LinkTarget__is_merged,$(1)),\
	$(call gb_Library_get_linktarget_target,merged) : \
		LINKED_STATIC_LIBS += $$(if $$(filter-out StaticLibrary,$$(TARGETTYPE)),$(2)))

# depend on the static libraries, but only if the target actually links to it
ifneq (,$(call gb_LinkTarget_does_real_link,$(1)))
# make has a size limit for the prerequisites string, which will be exceeded for some larger static links,
# like soffice.bin, but there seems to be no limit for makefile lines...
$(foreach lib,$(2), \
    $$(eval $(call gb_LinkTarget_get_target,$(1)): $(call gb_StaticLibrary_get_linktarget_target,$(lib))))
endif

$(call gb_LinkTarget__register_type,statics,$(1),$(2))

$(call gb_LinkTarget_get_headers_target,$(1)) : \
	$(foreach lib,$(2),$(call gb_StaticLibrary_get_headers_target,$(lib)))
$(foreach lib,$(2),$(call gb_LinkTarget__static_lib_dummy_depend,$(lib)))

endef # gb_LinkTarget_use_static_libraries

# call gb_LinkTarget_add_cobject,linktarget,sourcefile,cflags,linktargetmakefilename
define gb_LinkTarget_add_cobject
$(if $(wildcard $(call gb_CObject_get_source,$(SRCDIR),$(2))),,$(eval $(call gb_Output_error,No such source file $(call gb_CObject_get_source,$(SRCDIR),$(2)))))
$(call gb_LinkTarget_get_target,$(1)) : COBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : COBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_CObject_get_target,$(2))
$(call gb_CObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_CObject_get_target,$(2)) : T_CFLAGS += $(call gb_LinkTarget__get_cflags,$(4)) $(3)
$(call gb_CObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : COBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_CObject_get_dep_target,$(2))
$(call gb_CObject_get_dep_target,$(2)) :| $(dir $(call gb_CObject_get_dep_target,$(2))).dir
$(call gb_CObject_get_target,$(2)) :| $(dir $(call gb_CObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_cxxobject_internal,linktarget,sourcefile,cxxflags,linktargetmakefilename,exceptionflags
# The purpose of the exceptionflags extra argument is to differentiate between usage that just needs
# exception flags and usage that adds other flags. Using a PCH requires the same cxxflags as the ones used
# to create the PCH, so non-empty cxxflags here mean the object cannot use the PCH, and the add_exception_cxxobject
# variant passes the necessary flags by setting the extra argument.
define gb_LinkTarget_add_cxxobject_internal
$(if $(wildcard $(call gb_CxxObject_get_source,$(SRCDIR),$(2))),,$(eval $(call gb_Output_error,No such source file $(call gb_CxxObject_get_source,$(SRCDIR),$(2)))))
$(call gb_LinkTarget_get_target,$(1)) : CXXOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : CXXOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_CxxObject_get_target,$(2))
$(call gb_CxxObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_CxxObject_get_target,$(2)) : T_CXXFLAGS += $(call gb_LinkTarget__get_cxxflags,$(4)) $(3) $(5)
$(call gb_CxxObject_get_target,$(2)) : OBJECT_HAS_EXTRA_CXXFLAGS := $(if $(strip $(3)),1)
$(call gb_CxxObject_get_target,$(2)) : \
	OBJECTOWNER := $(if $(6),,$(call gb_Object__owner,$(2),$(1)))
ifneq ($(gb_ENABLE_PCH),)
ifeq ($(6),)
$(call gb_CxxObject_get_target,$(2)) : $(call gb_LinkTarget_get_pch_timestamp,$(4))
endif
endif

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : CXXOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_CxxObject_get_dep_target,$(2))
$(call gb_CxxObject_get_dep_target,$(2)) :| $(dir $(call gb_CxxObject_get_dep_target,$(2))).dir
$(call gb_CxxObject_get_target,$(2)) :| $(dir $(call gb_CxxObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_cxxobject,linktarget,sourcefile,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_cxxobject
$(call gb_LinkTarget_add_cxxobject_internal,$(1),$(2),$(3),$(4))
endef

# call gb_LinkTarget_add_objcobject,linktarget,sourcefile,objcflags,linktargetmakefilename
define gb_LinkTarget_add_objcobject
$(if $(wildcard $(call gb_ObjCObject_get_source,$(SRCDIR),$(2))),,$(eval $(call gb_Output_error,No such source file $(call gb_ObjCObject_get_source,$(SRCDIR),$(2)))))
$(call gb_LinkTarget_get_target,$(1)) : OBJCOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : OBJCOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_ObjCObject_get_target,$(2))
$(call gb_ObjCObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_ObjCObject_get_target,$(2)) : T_OBJCFLAGS += $(call gb_LinkTarget__get_objcflags,$(4)) $(3)
$(call gb_ObjCObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : OBJCOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_ObjCObject_get_dep_target,$(2))
$(call gb_ObjCObject_get_dep_target,$(2)) :| $(dir $(call gb_ObjCObject_get_dep_target,$(2))).dir
$(call gb_ObjCObject_get_target,$(2)) :| $(dir $(call gb_ObjCObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_objcxxobject,linktarget,sourcefile,objcxxflags,linktargetmakefilename
define gb_LinkTarget_add_objcxxobject
$(if $(wildcard $(call gb_ObjCxxObject_get_source,$(SRCDIR),$(2))),,$(eval $(call gb_Output_error,No such source file $(call gb_ObjCxxObject_get_source,$(SRCDIR),$(2)))))
$(call gb_LinkTarget_get_target,$(1)) : OBJCXXOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : OBJCXXOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_ObjCxxObject_get_target,$(2))
$(call gb_ObjCxxObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_ObjCxxObject_get_target,$(2)) : T_OBJCXXFLAGS += $(call gb_LinkTarget__get_objcxxflags,$(4)) $(3)
$(call gb_ObjCxxObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : OBJCXXOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_ObjCxxObject_get_dep_target,$(2))
$(call gb_ObjCxxObject_get_dep_target,$(2)) :| $(dir $(call gb_ObjCxxObject_get_dep_target,$(2))).dir
$(call gb_ObjCxxObject_get_target,$(2)) :| $(dir $(call gb_ObjCxxObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_cxxclrobject,linktarget,sourcefile,cxxclrflags,linktargetmakefilename
define gb_LinkTarget_add_cxxclrobject
$(if $(wildcard $(call gb_CxxClrObject_get_source,$(SRCDIR),$(2))),,$(eval $(call gb_Output_error,No such source file $(call gb_CxxClrObject_get_source,$(SRCDIR),$(2)))))
$(call gb_LinkTarget_get_target,$(1)) : CXXCLROBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : CXXCLROBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_CxxClrObject_get_target,$(2))
$(call gb_CxxClrObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_CxxClrObject_get_target,$(2)) : T_CXXCLRFLAGS += $(call gb_LinkTarget__get_cxxclrflags,$(4)) $(3)
$(call gb_CxxClrObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : CXXCLROBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_CxxClrObject_get_dep_target,$(2))
$(call gb_CxxClrObject_get_dep_target,$(2)) :| $(dir $(call gb_CxxClrObject_get_dep_target,$(2))).dir
$(call gb_CxxClrObject_get_target,$(2)) :| $(dir $(call gb_CxxClrObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_asmobject,linktarget,sourcefile
define gb_LinkTarget_add_asmobject
$(if $(wildcard $(call gb_AsmObject_get_source,$(SRCDIR),$(2))),,$(eval $(call gb_Output_error,No such source file $(call gb_AsmObject_get_source,$(SRCDIR),$(2)))))
$(call gb_LinkTarget_get_target,$(1)) : ASMOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : ASMOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_AsmObject_get_target,$(2))
$(call gb_AsmObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_AsmObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : ASMOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_AsmObject_get_dep_target,$(2))
$(call gb_AsmObject_get_dep_target,$(2)) :| $(dir $(call gb_AsmObject_get_dep_target,$(2))).dir
$(call gb_AsmObject_get_target,$(2)) :| $(dir $(call gb_AsmObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_generated_c_object,linktarget,sourcefile,cflags,linktargetmakefilename
define gb_LinkTarget_add_generated_c_object
$(call gb_LinkTarget_get_target,$(1)) : GENCOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : GENCOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_GenCObject_get_target,$(2))
$(call gb_GenCObject_get_target,$(2)) : $(call gb_GenCObject_get_source,$(2))
# Often gb_GenCObject_get_source does not have its own rule and is only a byproduct.
# That's why we need this order-only dependency on gb_Helper_MISCDUMMY
$(call gb_GenCObject_get_source,$(2)) : | $(gb_Helper_MISCDUMMY)
$(call gb_GenCObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_GenCObject_get_target,$(2)) : WARNINGS_NOT_ERRORS := $(true)
$(call gb_GenCObject_get_target,$(2)) : T_CFLAGS += $(call gb_LinkTarget__get_cflags,$(4)) $(3)
$(call gb_GenCObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : GENCOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_GenCObject_get_dep_target,$(2))
$(call gb_GenCObject_get_dep_target,$(2)) :| $(dir $(call gb_GenCObject_get_dep_target,$(2))).dir
$(call gb_GenCObject_get_target,$(2)) :| $(dir $(call gb_GenCObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_generated_cxx_object_internal,linktarget,sourcefile,cxxflags,linktargetmakefilename,exceptionflags
define gb_LinkTarget_add_generated_cxx_object_internal
$(call gb_LinkTarget_get_target,$(1)) : GENCXXOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : GENCXXOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_GenCxxObject_get_target,$(2))
$(call gb_GenCxxObject_get_target,$(2)) : $(call gb_GenCxxObject_get_source,$(2),$(1))
# Often gb_GenCxxObject_get_source does not have its own rule and is only a byproduct.
# That's why we need this order-only dependency on gb_Helper_MISCDUMMY
$(call gb_GenCxxObject_get_source,$(2),$(1)) : | $(gb_Helper_MISCDUMMY)
$(call gb_GenCxxObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_GenCxxObject_get_target,$(2)) : WARNINGS_NOT_ERRORS := $(true)
$(call gb_GenCxxObject_get_target,$(2)) : T_CXXFLAGS += $(call gb_LinkTarget__get_cxxflags,$(4)) $(3) $(5)
$(call gb_GenCxxObject_get_target,$(2)) : OBJECT_HAS_EXTRA_CXXFLAGS := $(if $(strip $(3)),1)
$(call gb_GenCxxObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))
$(call gb_GenCxxObject_get_target,$(2)) : GEN_CXX_SOURCE := $(call gb_GenCxxObject_get_source,$(2),$(1))
ifneq ($(gb_ENABLE_PCH),)
$(call gb_GenCxxObject_get_target,$(2)) : $(call gb_LinkTarget_get_pch_timestamp,$(4))
endif

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : GENCXXOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_GenCxxObject_get_dep_target,$(2))
$(call gb_GenCxxObject_get_dep_target,$(2)) :| $(dir $(call gb_GenCxxObject_get_dep_target,$(2))).dir
$(call gb_GenCxxObject_get_target,$(2)) :| $(dir $(call gb_GenCxxObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_generated_cxx_object,linktarget,sourcefile,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_generated_cxx_object
$(call gb_LinkTarget_add_generated_cxx_object_internal,$(1),$(2),$(3),$(4))
endef

# call gb_LinkTarget_add_generated_objc_object,linktarget,sourcefile,cflags,linktargetmakefilename
define gb_LinkTarget_add_generated_objc_object
$(call gb_LinkTarget_get_target,$(1)) : GENOBJCOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : GENOBJCOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_GenObjCObject_get_target,$(2))
$(call gb_GenObjCObject_get_target,$(2)) : $(call gb_GenObjCObject_get_source,$(2))
# Often gb_GenObjCObject_get_source does not have its own rule and is only a byproduct.
# That's why we need this order-only dependency on gb_Helper_MISCDUMMY
$(call gb_GenObjCObject_get_source,$(2)) : | $(gb_Helper_MISCDUMMY)
$(call gb_GenObjCObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_GenObjCObject_get_target,$(2)) : WARNINGS_NOT_ERRORS := $(true)
$(call gb_GenObjCObject_get_target,$(2)) : T_OBJCFLAGS += $(call gb_LinkTarget__get_objcflags,$(4)) $(3)
$(call gb_GenObjCObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : GENOBJCOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_GenObjCObject_get_dep_target,$(2))
$(call gb_GenObjCObject_get_dep_target,$(2)) :| $(dir $(call gb_GenObjCObject_get_dep_target,$(2))).dir
$(call gb_GenObjCObject_get_target,$(2)) :| $(dir $(call gb_GenObjCObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_generated_asm_object,linktarget,sourcefile,asmflags,linktargetmakefilename
define gb_LinkTarget_add_generated_asm_object
$(call gb_LinkTarget_get_target,$(1)) : GENASMOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : GENASMOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_GenAsmObject_get_target,$(2))
$(call gb_GenAsmObject_get_target,$(2)) : $(call gb_GenAsmObject_get_source,$(2))
# Often gb_GenAsmObject_get_source does not have its own rule and is only a byproduct.
# That's why we need this order-only dependency on gb_Helper_MISCDUMMY
$(call gb_GenAsmObject_get_source,$(2)) : | $(gb_Helper_MISCDUMMY)
$(call gb_GenAsmObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_GenAsmObject_get_target,$(2)) : T_ASMFLAGS += $(call gb_LinkTarget__get_asmflags,$(4)) $(3)
$(call gb_GenAsmObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : GENASMOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_GenAsmObject_get_dep_target,$(2))
$(call gb_GenAsmObject_get_dep_target,$(2)) :| $(dir $(call gb_GenAsmObject_get_dep_target,$(2))).dir
$(call gb_GenAsmObject_get_target,$(2)) :| $(dir $(call gb_GenAsmObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_generated_nasm_object,linktarget,sourcefile,nasmflags,linktargetmakefilename
define gb_LinkTarget_add_generated_nasm_object
$(call gb_LinkTarget_get_target,$(1)) : GENNASMOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : GENNASMOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_GenNasmObject_get_target,$(2))
$(call gb_GenNasmObject_get_target,$(2)) : $(call gb_GenNasmObject_get_source,$(2))
# Often gb_GenNasmObject_get_source does not have its own rule and is only a byproduct.
# That's why we need this order-only dependency on gb_Helper_MISCDUMMY
$(call gb_GenNasmObject_get_source,$(2)) : | $(gb_Helper_MISCDUMMY)
$(call gb_GenNasmObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_GenNasmObject_get_target,$(2)) : T_NASMFLAGS += $(call gb_LinkTarget__get_nasmflags,$(4)) $(3)
$(call gb_GenNasmObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : GENNASMOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_GenNasmObject_get_dep_target,$(2))
$(call gb_GenNasmObject_get_dep_target,$(2)) :| $(dir $(call gb_GenNasmObject_get_dep_target,$(2))).dir
$(call gb_GenNasmObject_get_target,$(2)) :| $(dir $(call gb_GenNasmObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_generated_objcxx_object,linktarget,sourcefile,cflags,linktargetmakefilename
define gb_LinkTarget_add_generated_objcxx_object
$(call gb_LinkTarget_get_target,$(1)) : GENOBJCXXOBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : GENOBJCXXOBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_GenObjCxxObject_get_target,$(2))
$(call gb_GenObjCxxObject_get_target,$(2)) : $(call gb_GenObjCxxObject_get_source,$(2))
# Often gb_GenObjCxxObject_get_source does not have its own rule and is only a byproduct.
# That's why we need this order-only dependency on gb_Helper_MISCDUMMY
$(call gb_GenObjCxxObject_get_source,$(2)) : | $(gb_Helper_MISCDUMMY)
$(call gb_GenObjCxxObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_GenObjCxxObject_get_target,$(2)) : WARNINGS_NOT_ERRORS := $(true)
$(call gb_GenObjCxxObject_get_target,$(2)) : T_OBJCXXFLAGS += $(call gb_LinkTarget__get_objcxxflags,$(4)) $(3)
$(call gb_GenObjCxxObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : GENOBJCXXOBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_GenObjCxxObject_get_dep_target,$(2))
$(call gb_GenObjCxxObject_get_dep_target,$(2)) :| $(dir $(call gb_GenObjCxxObject_get_dep_target,$(2))).dir
$(call gb_GenObjCxxObject_get_target,$(2)) :| $(dir $(call gb_GenObjCxxObject_get_dep_target,$(2))).dir
endif

endef

# call gb_LinkTarget_add_generated_cxxclrobject,linktarget,sourcefile,cxxclrflags,linktargetmakefilename
define gb_LinkTarget_add_generated_cxxclrobject
$(call gb_LinkTarget_get_target,$(1)) : GENCXXCLROBJECTS += $(2)
$(call gb_LinkTarget_get_clean_target,$(1)) : GENCXXCLROBJECTS += $(2)

$(call gb_LinkTarget_get_target,$(1)) : $(call gb_GenCxxClrObject_get_target,$(2))
$(call gb_GenCxxClrObject_get_target,$(2)) : $(call gb_GenCxxClrObject_get_source,$(2),$(1))
$(call gb_GenCxxClrObject_get_target,$(2)) : | $(call gb_LinkTarget_get_headers_target,$(1))
$(call gb_GenCxxClrObject_get_target,$(2)) : T_CXXCLRFLAGS += $(call gb_LinkTarget__get_cxxclrflags,$(4)) $(3)
$(call gb_GenCxxClrObject_get_target,$(2)) : \
	OBJECTOWNER := $(call gb_Object__owner,$(2),$(1))
$(call gb_GenCxxClrObject_get_target,$(2)) : GEN_CXXCLR_SOURCE := $(call gb_GenCxxClrObject_get_source,$(2),$(1))

ifeq ($(gb_FULLDEPS),$(true))
$(call gb_LinkTarget_get_dep_target,$(1)) : GENCXXCLROBJECTS += $(2)
$(call gb_LinkTarget_get_dep_target,$(1)) : $(call gb_GenCxxClrObject_get_dep_target,$(2))
$(call gb_GenCxxClrObject_get_dep_target,$(2)) :| $(dir $(call gb_GenCxxClrObject_get_dep_target,$(2))).dir
$(call gb_GenCxxClrObject_get_target,$(2)) :| $(dir $(call gb_GenCxxClrObject_get_dep_target,$(2))).dir
endif

endef

# Add a bison grammar to the build.
# call gb_LinkTarget_add_grammar,linktarget,yaccfile,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_grammar
$(call gb_YaccTarget_YaccTarget,$(2))
$(call gb_LinkTarget_add_generated_exception_object,$(1),YaccTarget/$(2),$(3) $(if $(filter GCC,$(COM)),-Wno-unused-macros),$(4))
$(call gb_GenCxxObject_get_target,YaccTarget/$(2)): PLUGIN_WARNINGS_AS_ERRORS := $(true)
$(call gb_LinkTarget_get_clean_target,$(1)) : $(call gb_YaccTarget_get_clean_target,$(2))
$(call gb_LinkTarget_get_headers_target,$(1)) : $(call gb_YaccTarget_get_header_target,$(2))
$(call gb_LinkTarget__add_include,$(1),$(dir $(call gb_YaccTarget_get_header_target,$(2))))

endef

# Add bison grammars to the build.
# call gb_LinkTarget_add_grammars,linktarget,yaccfiles,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_grammars
$(foreach grammar,$(2),$(call gb_LinkTarget_add_grammar,$(1),$(grammar),$(3),$(4)))
endef

# Add a flex scanner to the build.
# call gb_LinkTarget_add_scanner,linktarget,lexfile,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_scanner
$(call gb_LexTarget_LexTarget,$(2))
$(call gb_LinkTarget_add_generated_exception_object,$(1),LexTarget/$(2),$(3) $(if $(filter GCC,$(COM)),-Wno-unused-macros),$(4))
$(call gb_LinkTarget_get_clean_target,$(1)) : $(call gb_LexTarget_get_clean_target,$(2))

endef

# Add flex scanners to the build.
# call gb_LinkTarget_add_scanners,linktarget,lexfiles,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_scanners
$(foreach scanner,$(2),$(call gb_LinkTarget_add_scanner,$(1),$(scanner),$(3),$(4)))

endef

# call gb_LinkTarget_add_exception_object,linktarget,sourcefile,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_exception_object
$(call gb_LinkTarget_add_cxxobject_internal,$(1),$(2),$(3),$(4),$(gb_LinkTarget_EXCEPTIONFLAGS))
endef

# call gb_LinkTarget__use_linktarget_objects,linktarget,linktargets
define gb_LinkTarget__use_linktarget_objects
$(call gb_LinkTarget_get_target,$(1)) : $(foreach linktarget,$(2),$(call gb_LinkTarget_get_target,$(linktarget)))
ifneq ($(OS),iOS)
$(call gb_LinkTarget_get_target,$(1)) : EXTRAOBJECTLISTS += $(foreach linktarget,$(2),$(call gb_LinkTarget_get_objects_list,$(linktarget)))
endif

endef

# call gb_LinkTarget_use_library_objects,linktarget,libs
define gb_LinkTarget_use_library_objects
ifneq (,$$(filter-out $(gb_Library_KNOWNLIBS),$(2)))
$$(eval $$(call gb_Output_info,currently known libraries are: $(sort $(gb_Library_KNOWNLIBS)),ALL))
$$(eval $$(call gb_Output_error,Cannot import objects library/libraries $$(filter-out $(gb_Library_KNOWNLIBS),$(2)). Libraries must be registered in Repository.mk or RepositoryExternal.mk))
endif
$(call gb_LinkTarget__use_linktarget_objects,$(1),$(foreach lib,$(2),$(call gb_Library_get_linktarget,$(lib))))
$(call gb_LinkTarget_get_headers_target,$(1)) : \
	$(foreach lib,$(2),$(call gb_Library_get_headers_target,$(lib)))

endef

# call gb_LinkTarget_use_executable_objects,linktarget,exes
define gb_LinkTarget_use_executable_objects
$(call gb_LinkTarget__use_linktarget_objects,$(1),$(foreach exe,$(2),$(call gb_Executable_get_linktarget,$(exe))))

endef

# call gb_LinkTarget_add_cobjects,linktarget,sourcefiles,cflags,linktargetmakefilename
define gb_LinkTarget_add_cobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_cobject,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_cxxobjects,linktarget,sourcefiles,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_cxxobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_cxxobject,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_objcobjects,linktarget,sourcefiles,objcflags,linktargetmakefilename
define gb_LinkTarget_add_objcobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_objcobject,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_objcxxobjects,linktarget,sourcefiles,objcxxflags,linktargetmakefilename
define gb_LinkTarget_add_objcxxobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_objcxxobject,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_cxxclrobjects,linktarget,sourcefiles,cxxclrflags,linktargetmakefilename
define gb_LinkTarget_add_cxxclrobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_cxxclrobject,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_asmobjects,linktarget,sourcefiles,asmflags,linktargetmakefilename
define gb_LinkTarget_add_asmobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_asmobject,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_exception_objects,linktarget,sourcefiles,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_exception_objects
$(foreach obj,$(2),$(call gb_LinkTarget_add_exception_object,$(1),$(obj),$(3),$(4)))
endef

#only useful for building x64 libraries on windows
# call gb_LinkTarget_add_x64_generated_exception_objects,linktarget,sourcefiles,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_x64_generated_exception_objects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_exception_object,$(1),$(obj),$(3),$(4)))
$(foreach obj,$(2),$(eval $(call gb_GenCxxObject_get_target,$(obj)) : CXXOBJECT_X64 := YES))
endef

# call gb_LinkTarget_add_generated_cobjects,linktarget,sourcefiles,cflags,linktargetmakefilename
define gb_LinkTarget_add_generated_cobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_c_object,$(1),$(obj),$(3),$(4)))
endef

#only useful for building x64 libraries on windows
# call gb_LinkTarget_add_x64_generated_cobjects,linktarget,sourcefiles,cflags,linktargetmakefilename
define gb_LinkTarget_add_x64_generated_cobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_c_object,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_generated_exception_object,linktarget,sourcefile,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_generated_exception_object
$(call gb_LinkTarget_add_generated_cxx_object_internal,$(1),$(2),$(3),$(4),$(gb_LinkTarget_EXCEPTIONFLAGS))
endef

# call gb_LinkTarget_add_generated_exception_objects,linktarget,sourcefile,cxxflags,linktargetmakefilename
define gb_LinkTarget_add_generated_exception_objects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_exception_object,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_generated_objcobjects,linktarget,sourcefiles,cflags,linktargetmakefilename
define gb_LinkTarget_add_generated_objcobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_objc_object,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_generated_objcxxobjects,linktarget,sourcefiles,cflags,linktargetmakefilename
define gb_LinkTarget_add_generated_objcxxobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_objcxx_object,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_generated_asmobjects,linktarget,sourcefiles,asmflags,linktargetmakefilename
define gb_LinkTarget_add_generated_asmobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_asm_object,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_generated_nasmobjects,linktarget,sourcefiles,cflags,linktargetmakefilename
define gb_LinkTarget_add_generated_nasmobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_nasm_object,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_add_generated_cxxclrobjects,linktarget,sourcefiles,cxxclrflags,linktargetmakefilename
define gb_LinkTarget_add_generated_cxxclrobjects
$(foreach obj,$(2),$(call gb_LinkTarget_add_generated_cxxclrobject,$(1),$(obj),$(3),$(4)))
endef

# call gb_LinkTarget_set_targettype,linktarget,targettype
define gb_LinkTarget_set_targettype
$(call gb_LinkTarget_get_target,$(1)) : TARGETTYPE := $(2)

endef

# call gb_LinkTarget_set_x64,linktarget,boolean
define gb_LinkTarget_set_x64
$(call gb_LinkTarget_get_target,$(1)) : LIBRARY_X64 := $(2)

endef

# call gb_LinkTarget_set_x86,linktarget,boolean
define gb_LinkTarget_set_x86
$(call gb_LinkTarget_get_target,$(1)) : PE_X86 := $(2)

endef

# call gb_LinkTarget_set_ilibtarget,linktarget,ilibfilename
define gb_LinkTarget_set_ilibtarget
ifeq (,$(DISABLE_DYNLOADING))
$(call gb_LinkTarget_get_clean_target,$(1)) \
$(call gb_LinkTarget_get_target,$(1)) : ILIBTARGET := $(2)
endif

endef

# Add a file that is built by the LinkTarget command and define
# a dummy touch rule for it so it can be tracked via dependencies.
# The assumption is that the file is created by linking; in case it does not
# exist there is some problem.  This can be caused on WNT by re-naming DLL
# files (which are aux-targets) but not the import .lib files (which
# are the LinkTargets) and doing an incremental build.
# call gb_LinkTarget_add_auxtarget,linktarget,auxtarget
define gb_LinkTarget_add_auxtarget
$(2) : $(call gb_LinkTarget_get_target,$(1))
	if test -e $$@; then \
		touch -r $$< $$@; \
	else \
		rm -f $$<; \
		echo "ERROR: aux-target $$@ missing, library deleted, please try running make again"; \
		false; \
	fi

$(call gb_LinkTarget_get_clean_target,$(1)) : AUXTARGETS += $(2)

endef

# call gb_LinkTarget_add_auxtargets,linktarget,auxtargets
define gb_LinkTarget_add_auxtargets
$(foreach aux,$(2),$(call gb_LinkTarget_add_auxtarget,$(1),$(aux)))

endef

# call gb_LinkTarget__use_custom_headers,linktarget,customtarget
define gb_LinkTarget__use_custom_headers
$(call gb_LinkTarget_get_headers_target,$(1)) :| \
	$(call gb_CustomTarget_get_target,$(2))
$(call gb_LinkTarget__add_include,$(1),$(gb_CustomTarget_workdir)/$(2))

endef

# call gb_LinkTarget_use_custom_headers,linktarget,customtargets
define gb_LinkTarget_use_custom_headers
$(foreach customtarget,$(2),$(call gb_LinkTarget__use_custom_headers,$(1),$(customtarget)))

endef

# add SDI (svidl) headers
# call gb_LinkTarget_add_sdi_headers,linktarget,sditargets
define gb_LinkTarget_add_sdi_headers
$(call gb_LinkTarget_get_headers_target,$(1)) : $(foreach sdi,$(2),$(call gb_SdiTarget_get_target,$(sdi)))
$(call gb_LinkTarget_get_clean_target,$(1)) : $(foreach sdi,$(2),$(call gb_SdiTarget_get_clean_target,$(sdi)))

endef

# call gb_LinkTarget__set_precompiled_header_variables,linktarget,pchcxxfile,pchtarget,linktargetmakefilename
define gb_LinkTarget__set_precompiled_header_variables
$(call gb_LinkTarget_get_target,$(1)) : PCH_NAME := $(3)
$(call gb_LinkTarget_get_target,$(1)) : PCH_HEADER := $(patsubst %.cxx,%.hxx,$(2))
$(call gb_LinkTarget_get_target,$(1)) : PCH_LINKTARGETMAKEFILENAME := $(4)

$(call gb_LinkTarget_get_target,$(1)) : PCH_DEFS := $$(DEFS)
$(call gb_LinkTarget_get_target,$(1)) : PCH_CXXFLAGS := $$(T_CXXFLAGS) $(call gb_LinkTarget__get_cxxflags,$(4)) $(gb_LinkTarget_EXCEPTIONFLAGS)

$(call gb_LinkTarget_get_target,$(1)) : DEFS += -DPCH_LEVEL=$(gb_ENABLE_PCH)
$(call gb_LinkTarget_get_target,$(1)) : PCH_DEFS += -DPCH_LEVEL=$(gb_ENABLE_PCH)

endef

# call gb_LinkTarget__set_precompiled_header_impl,linktarget,pchcxxfile,pchtarget,linktargetmakefilename
define gb_LinkTarget__set_precompiled_header_impl
$(call gb_LinkTarget_get_clean_target,$(1)) : $(call gb_PrecompiledHeader_get_clean_target,$(3))
$(call gb_PrecompiledHeader_get_target,$(3),$(4)) : $(call gb_CxxObject_get_source,$(SRCDIR),$(2))
$(call gb_PrecompiledHeader_get_target,$(3),$(4)) : $(patsubst %.cxx,%.hxx,$(call gb_CxxObject_get_source,$(SRCDIR),$(2)))
$(call gb_PrecompiledHeader_get_target,$(3),$(4)) : $(call gb_PrecompiledHeader_get_flags_file,$(3),$(4))

$(call gb_PrecompiledHeader_get_target,$(3),$(4)) : $(call gb_LinkTarget_get_headers_target,$(1))

$(call gb_PrecompiledHeader_get_target,$(3),$(4)) : VISIBILITY :=

$(call gb_LinkTarget_get_pch_timestamp,$(4)) : $(call gb_PrecompiledHeader_get_target,$(3),$(4))

$(call gb_LinkTarget__set_precompiled_header_variables,$(1),$(2),$(3),$(4))

ifeq ($(gb_FULLDEPS),$(true))
-include $(call gb_PrecompiledHeader_get_dep_target,$(3),$(4))
endif

endef

# call gb_LinkTarget__add_precompiled_header_object,linktarget,pchcxxfile,pchtarget,linktargetmakefilename
define gb_LinkTarget__add_precompiled_header_object
# Clang-style
ifneq ($(BUILDING_PCH_WITH_OBJ),)
$(call gb_LinkTarget_add_exception_object,$(1),$(2),,$(4))
$(call gb_CxxObject_get_target,$(2)) : T_PCH_EXTRA_CXXFLAGS += $(gb_PrecompiledHeader_pch_with_obj)
endif
# MSVC-style
$(call gb_LinkTarget_get_target,$(1)) : PCHOBJEX = $(call gb_PrecompiledHeader_get_objectfile, $(call gb_PrecompiledHeader_get_target,$(3),$(4)))
$(call gb_LinkTarget_get_target,$(1)) : PCHOBJS = $$(PCHOBJEX)

endef

# 'compiler' set comes only from gb_LinkTarget_set_clang_precompiled_header
# call gb_LinkTarget_set_precompiled_header,linktarget,pchcxxfile,,linktargetmakefilename,compiler
define gb_LinkTarget_set_precompiled_header
ifneq ($(gb_ENABLE_PCH),)
$(call gb_LinkTarget__set_precompiled_header_impl,$(1),$(2),$(notdir $(2)),$(4))
$(call gb_PrecompiledHeader_generate_rules,$(notdir $(2)),$(1),$(4),$(2),$(5))
endif
ifneq ($(gb_ENABLE_PCH)$(BLOCK_PCH),)
$(call gb_LinkTarget__add_precompiled_header_object,$(1),$(2),$(notdir $(2)),$(4))
endif

endef

# It seems complicated to forward the clang setting to the PCH rules, so use an extra
# function to set it manually. This variant should be used if gb_LinkTarget_use_clang is used.
# call gb_LinkTarget_set_clang_precompiled_header,linktarget,pchcxxfile,,linktargetmakefilename
define gb_LinkTarget_set_clang_precompiled_header
$(call gb_LinkTarget_set_precompiled_header,$(1),$(2),$(3),$(4),$(LO_CLANG_CXX))
endef

# call gb_LinkTarget__reuse_precompiled_header_impl,linktarget,pchcxxfile,pchtarget,linktargetmakefilename
# Use the PCH as if it was LinkTarget's own, but do nothing with the PCH itself, just depend on it.
define gb_LinkTarget__reuse_precompiled_header_impl
$(call gb_LinkTarget__set_precompiled_header_variables,$(1),$(2),$(3),$(4))

$(call gb_LinkTarget_get_pch_timestamp,$(4)) : $(call gb_LinkTarget_get_pch_reuse_timestamp,$(4))

# We need to depend on a special for_reuse target that depends on the linktarget that owns the PCH.
# Depending directly on the PCH could cause that PCH to be built with this linktarget's flags.
$(call gb_LinkTarget_get_pch_reuse_timestamp,$(4)) : $(call gb_PrecompiledHeader_get_for_reuse_target,$(3),$(4))
	$(call gb_PrecompiledHeader_check_flags,$(4),$(2),\
		$(call gb_PrecompiledHeader_get_target,$(3),$(4)),\
		$(call gb_PrecompiledHeader_get_flags_file,$(3),$(4)),\
		$(gb_PrecompiledHeader_cxxflags_includes))
	$$(call gb_PrecompiledHeader__copy_reuse_files,$(1),$(3),$(4))
	mkdir -p $$(dir $$@) && touch $$@

endef

# call gb_LinkTarget__add_reuse_precompiled_header_object,linktarget,pchcxxfile,pchtarget,linktargetmakefilename
define gb_LinkTarget__add_reuse_precompiled_header_object
# Clang-style
ifneq ($(BUILDING_PCH_WITH_OBJ),)
# We need to link in also the PCH's object file. Again, rely on a special for_reuse target for dependencies.
$(if $(wildcard $(call gb_CxxObject_get_source,$(SRCDIR),$(2))),,$(eval $(call gb_Output_error,No such source file $(call gb_CxxObject_get_source,$(SRCDIR),$(2)))))
$(call gb_LinkTarget_get_target,$(1)) : CXXOBJECTS += $(2)
endif
# MSVC-style
$(call gb_LinkTarget_get_target,$(1)) : PCHOBJEX = $(call gb_PrecompiledHeader_get_objectfile, $(call gb_PrecompiledHeader_get_target,$(3),$(4)))
$(call gb_LinkTarget_get_target,$(1)) : PCHOBJS = $$(PCHOBJEX)

endef

# call gb_LinkTarget__reuse_precompiled_header_workarounds,linktarget,pchcxxfile,pchtarget,linktargetmakefilename
define gb_LinkTarget__reuse_precompiled_header_workarounds
ifeq ($(COM_IS_CLANG),TRUE)
$(call gb_LinkTarget_add_defs,$(1),$(gb_CXXFLAGS_include)$(SRCDIR)/pch/inc/clangfix.hxx)
$(call gb_LinkTarget_add_defs,$(1),$(gb_CXXFLAGS_no_pch_warnings))
endif
$(if $(filter precompiled_system,$(3)), $(call gb_LinkTarget_add_defs,$(1),-DBOOST_ALL_NO_LIB))
endef

# call gb_LinkTarget_reuse_precompiled_header,linktarget,pchcxxfile,,linktargetmakefilename
define gb_LinkTarget_reuse_precompiled_header
ifeq ($(gb_DISABLE_PCH_REUSE),$(false))
ifneq ($(gb_ENABLE_PCH),)
$(call gb_LinkTarget__reuse_precompiled_header_impl,$(1),$(2),$(notdir $(2)),$(4))
$(call gb_LinkTarget__reuse_precompiled_header_workarounds,$(1),$(2),$(notdir $(2)),$(4))
endif
ifneq ($(gb_ENABLE_PCH)$(BLOCK_PCH),)
$(call gb_LinkTarget__add_reuse_precompiled_header_object,$(1),$(2),$(notdir $(2)),$(4))
endif
endif

endef

# call gb_LinkTarget_use_common_precompiled_header,linktarget,,,linktargetmakefilename
define gb_LinkTarget_use_common_precompiled_header
$(call gb_LinkTarget_reuse_precompiled_header,$(1),pch/inc/pch/precompiled_system,,$(4))

endef

# use a header package, possibly from another module
# call gb_LinkTarget_use_package,linktarget,package
define gb_LinkTarget_use_package
$(call gb_LinkTarget_get_headers_target,$(1)) :| \
	$(call gb_Package_get_target,$(strip $(2)))
$(call gb_Package_get_target,$(strip $(2))): RDEPENDS += $(call gb_LinkTarget__get_workdir_linktargetname,$(1))

endef

# call gb_LinkTarget_use_packages,linktarget,packages
define gb_LinkTarget_use_packages
$(foreach package,$(2),$(call gb_LinkTarget_use_package,$(1),$(package)))
endef

# use a GeneratedPackage, possibly from another module
# call gb_LinkTarget_use_generated_package,linktarget,package
define gb_LinkTarget_use_generated_package
$(call gb_LinkTarget_get_headers_target,$(1)) :| \
	$(call gb_GeneratedPackage_get_target,$(strip $(2)))

endef

# Use sources from unpacked tarball of an external project
# call gb_LinkTarget_use_unpacked,linktarget,unpackedtarget
define gb_LinkTarget_use_unpacked
$(call gb_LinkTarget_get_headers_target,$(1)) :| $(call gb_UnpackedTarball_get_final_target,$(2))

endef

# Use artifacts from ExternalProject (i. e. configure) of an external project
# example in expat: StaticLibrary depends on ExternalProject outcome
# call gb_LinkTarget_use_external_project,linktarget,externalproject,full-dep
define gb_LinkTarget_use_external_project
$(call gb_LinkTarget_get_target,$(1)) :| $(call gb_ExternalProject_get_target,$(2))
$(call gb_LinkTarget_get_headers_target,$(1)) :| \
    $(if $(3),$(call gb_ExternalProject_get_target,$(2)),$(call gb_UnpackedTarball_get_final_target,$(2)))

endef

# this forwards to functions that must be defined in RepositoryExternal.mk.
# Automatically forward for libmerged library too when linktarget is merged.
#
# call gb_LinkTarget_use_external,linktarget,external
define gb_LinkTarget_use_external
$(if $(filter undefined,$(origin gb_LinkTarget__use_$(2))),\
    $(error gb_LinkTarget_use_external: unknown external: $(2)) \
, \
    $(if $(gb_PARTIAL_BUILD),,$(if $(call gb_LinkTarget__is_merged,$(1)), \
        $(call gb_LinkTarget__use_$(2),$(call gb_Library_get_linktarget,merged)))) \
    $(call gb_LinkTarget__use_$(2),$(1)) \
)

$(call gb_LinkTarget__register_type,externals,$(1),$(2))

endef

# $(call gb_LinkTarget_use_externals,library,externals)
gb_LinkTarget_use_externals = \
 $(foreach external,$(2),$(call gb_LinkTarget_use_external,$(1),$(external)))

# call gb_LinkTarget_set_visibility_default,linktarget
define gb_LinkTarget_set_visibility_default
$(call gb_LinkTarget_get_target,$(1)) : VISIBILITY := default
ifneq ($(gb_ENABLE_PCH),)
ifneq ($(strip $$(PCH_NAME)),)
$(call gb_PrecompiledHeader_get_target,$$(PCH_NAME),$$(PCH_LINKTARGETMAKEFILENAME)) : VISIBILITY := default
endif
endif

endef

# call gb_LinkTarget_set_warnings_not_errors,linktarget
define gb_LinkTarget_set_warnings_not_errors
$(call gb_LinkTarget_get_target,$(1)) : WARNINGS_NOT_ERRORS := $(true)

endef

# call gb_LinkTarget_set_warnings_disabled,linktarget
define gb_LinkTarget_set_warnings_disabled
$(call gb_LinkTarget_get_target,$(1)) : WARNINGS_DISABLED := $(true)

endef

# call gb_LinkTarget_set_external_code,linktarget
define gb_LinkTarget_set_external_code
$(call gb_LinkTarget_get_target,$(1)) : EXTERNAL_CODE := $(true)

endef

# Set suffix of C++ files, if different from 'cxx'
#
# This is useful for external libraries.
#
# call gb_LinkTarget_set_generated_cxx_suffix,linktarget,used-suffix
define gb_LinkTarget_set_generated_cxx_suffix
gb_LinkTarget_CXX_SUFFIX_$(call gb_LinkTarget__get_workdir_linktargetname,$(1)) := $(2)

endef

# C/C++ files will be build with Clang (if possible) instead of the default compiler.
# call gb_LinkTarget_use_clang,linktarget,,linktargetmakefilename
define gb_LinkTarget_use_clang
$(call gb_LinkTarget_get_target,$(1)) : T_CC := $(LO_CLANG_CC)
$(call gb_LinkTarget_get_target,$(1)) : T_CXX := $(LO_CLANG_CXX)
$(call gb_LinkTarget_get_target,$(1)) : T_USE_CLANG := $(true)
$(call gb_LinkTarget_get_target,$(1)) : T_USE_LD := $(or $(CLANG_USE_LD),$(USE_LD))
$(call gb_LinkTarget_get_target,$(1)) : T_LTOFLAGS := $(if $(LO_CLANG_CXX),$(gb_CLANG_LTOFLAGS),$(gb_LTOFLAGS))
endef

# call gb_LinkTarget_use_vclmain,linktarget
define gb_LinkTarget_use_vclmain
$(call gb_LinkTarget_use_static_libraries,$(1),vclmain)

endef # gb_LinkTarget_use_vclmain

# Used by URE libraries that need to keep binary compatibility.
# Reset some flags that make sense for our internal libraries but might
# break public ABI.
# (clang-cl's -Zc:dllexportInlines- would not only be a problem for the URE libraries themselves but
# also for any libraries they depend on.  While that does not appear to be a problem for -Zc:inline
# for neither MSVC nor clang-cl, it should not really hurt to also switch that off not only for the
# URE libraries themselves but also for their dependencies.)
# call gb_LinkTarget_set_is_ure_library_or_dependency,linktarget,,linktargetmakefilename
define gb_LinkTarget_set_is_ure_library_or_dependency
$(call gb_LinkTarget_add_cxxflags,$(1),$(gb_CXXFLAGS_ZCINLINE_OFF))
ifeq ($(HAVE_DLLEXPORTINLINES),TRUE)
$(call gb_LinkTarget_add_cxxflags,$(1),-Zc:dllexportInlines)
endif

endef

gb_LinkTarget__get_plugins_var = $(call gb_LinkTarget__get_workdir_linktargetname,$(1))<>PLUGINS
gb_LinkTarget__get_plugins = $($(call gb_LinkTarget__get_plugins_var,$(1)))
gb_Library__get_plugins = $($(call gb_LinkTarget__get_plugins_var,$(call gb_Library_get_linktarget,$(1))))

define gb_LinkTarget__add_plugin
$(call gb_LinkTarget__get_plugins_var,$(1)) += $(2)

endef

# Instead of setting nodep use gb_LinkTarget__set_plugin_for_nodep
#
# call gb_LinkTarget__set_plugin_for,linktarget,loader,nodep
define gb_LinkTarget__set_plugin_for
ifeq (,$(filter $(1),$(foreach plugin,$(gb_Library_KNOWNPLUGINS),$(call gb_Library_get_linktarget,$(plugin)))))
$$(eval $$(call gb_Output_error,Unknown plugin(s) '$(filter $(1),$(foreach plugin,$(gb_Library_KNOWNPLUGINS),$(call gb_Library_get_linktarget,$(plugin))))'. Plugins must be registered in Repository.mk or RepositoryExternal.mk))
endif
ifneq (,$(filter $(1),$(foreach lib,$(gb_MERGEDLIBS),$(call gb_Library_get_linktarget,$(lib)))))
$$(eval $$(call gb_Output_error,Plugins can't be in mergelibs))
endif
ifeq ($(call gb_LinkTarget__is_build_tool,$(1)),$(true))
$$(eval $$(call gb_Output_error,Plugin support for build tools not implemented))
endif

$(if $(filter $(2),$(gb_Library_KNOWNLOADERS)),,gb_Library_KNOWNLOADERS += $(2))
$(if $(3),,$(call gb_LinkTarget_use_libraries,$(1),$(2),,$(4)))

endef

# call gb_LinkTarget__set_plugin_for_nodep,linktarget,loader
gb_LinkTarget__set_plugin_for_nodep = $(call gb_LinkTarget__set_plugin_for,$(1),$(2),$(true))

# call gb_LinkTarget_add_prejs,linktarget,js_file
define gb_LinkTarget_add_prejs
ifeq (EMSCRIPTEN,$(OS))
$(call gb_LinkTarget_get_target,$(1)) : T_PREJS += $(2)
$(call gb_LinkTarget_get_target,$(1)) : $(2)
endif

endef

# vim: set noet sw=4:
