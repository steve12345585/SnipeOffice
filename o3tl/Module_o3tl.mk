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

$(eval $(call gb_Module_Module,o3tl))

$(eval $(call gb_Module_add_targets,o3tl,\
))

$(eval $(call gb_Module_add_check_targets,o3tl,\
	CppunitTest_o3tl_tests \
	$(if $(COM_IS_CLANG),$(if $(COMPILER_EXTERNAL_TOOL)$(COMPILER_PLUGIN_TOOL),, \
	    CompilerTest_o3tl_temporary \
	    CompilerTest_o3tl_unsafe_downcast)) \
))

# vim: set noet sw=4:
