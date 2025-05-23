# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CustomTarget_CustomTarget,solenv/gbuildtesttools))

$(call gb_CustomTarget_get_target,solenv/gbuildtesttools) :
	echo -n "{ \"MAKE\": \"$(if $(filter WNT,$(OS)),$(shell cygpath -u $(MAKE)),$(MAKE))\"" > $@
	echo -n ", \"BASH\": \"$(if $(filter WNT,$(OS)),$(shell cygpath -m `command -v bash`),bash)\"" >> $@
	echo -n ", \"GBUILDTOJSON\": \"$(call gb_Executable_get_target,gbuildtojson)\" }" >> $@

# vim: set noet sw=4 ts=4:
