# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_CustomTarget_CustomTarget,odk/html))

$(eval $(call gb_CustomTarget_register_targets,odk/html,\
	docs/install.html \
	docs/tools.html \
	examples/DevelopersGuide/examples.html \
	examples/examples.html \
	index.html \
))

$(gb_CustomTarget_workdir)/odk/html/%.html : $(SRCDIR)/odk/%.html
	$(call gb_Output_announce,$*.html,$(true),SED,1)
	$(call gb_Trace_StartRange,$*.html,SED)
	sed -e 's|%PRODUCTNAME%|$(PRODUCTNAME)|g' \
	    -e 's|%LCPRODUCTNAME%|'"$$(printf %s '$(PRODUCTNAME)' | tr A-Z a-z)"'|g' \
	    -e 's|%PRODUCT_RELEASE%|$(PRODUCTVERSION)|g' \
	    -e 's|%DOXYGEN_PREFIX0%|$(if $(DOXYGEN),.,https://api.libreoffice.org)|g' \
	    -e 's|%DOXYGEN_PREFIX1%|$(if $(DOXYGEN),..,https://api.libreoffice.org)|g' \
	    -e 's|%DOXYGEN_PREFIX2%|$(if $(DOXYGEN),../..,https://api.libreoffice.org)|g' \
	    -e 's|%JAVADOC_PREFIX0%|$(if $(ENABLE_JAVA),.,https://api.libreoffice.org)|g' \
	    -e 's|%JAVADOC_PREFIX1%|$(if $(ENABLE_JAVA),..,https://api.libreoffice.org)|g' \
	    < $< > $@
	$(call gb_Trace_EndRange,$*.html,SED)

# vim: set noet sw=4 ts=4:
