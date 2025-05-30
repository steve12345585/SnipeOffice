#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CustomTarget_CustomTarget,vcl/workben))

fuzzer_PYTHONCOMMAND := $(call gb_ExternalExecutable_get_command,python)

fuzzer_Native_cxx=$(gb_CustomTarget_workdir)/vcl/workben/native-draw.cxx

$(fuzzer_Native_cxx): $(SRCDIR)/solenv/bin/native-code.py | $(gb_CustomTarget_workdir)/vcl/workben/.dir
	$(call gb_Helper_abbreviate_dirs, $(fuzzer_PYTHONCOMMAND) $(SRCDIR)/solenv/bin/native-code.py -g core -g draw) > $@

# vim: set noet sw=4 ts=4:
