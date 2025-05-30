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

PRJ := ..$/..$/..$/..$/..$/..
PRJNAME := ucb
PACKAGE := com$/sun$/star$/comp$/ucb
TARGET := test_com_sun_star_comp_ucb

.INCLUDE : settings.mk

JAVAFILES = $(subst,$(CLASSDIR)$/$(PACKAGE)$/, $(subst,.class,.java $(JAVACLASSFILES)))

CLASSDIR !:= $(CLASSDIR)$/test

JARFILES = OOoRunner.jar \
           ridl.jar \
           unoil.jar \
           juh.jar \
           jurt.jar

JAVACLASSFILES = $(CLASSDIR)$/$(PACKAGE)$/GlobalTransfer_Test.class

.INCLUDE: target.mk

$(JAVAFILES): $(MISC)$/$(TARGET).createdclassdir

$(MISC)$/$(TARGET).createdclassdir:
    - $(MKDIR) $(CLASSDIR)
    $(TOUCH) $@
