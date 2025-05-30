#!/bin/sh
# -*- Mode: sh; tab-width: 4; indent-tabs-mode: t -*-
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
cat << EOF

Error: a unit test failed, please do one of:

make $1Test_$2 CPPUNITTRACE="gdb --args"
    # for interactive debugging on Linux
make $1Test_$2 VALGRIND=memcheck
    # for memory checking
make $1Test_$2 DEBUGCPPUNIT=TRUE
    # for exception catching

You can limit the execution to just one particular test by:

EOF

case $1 in
    Python)
    cat << EOF
make PYTHON_TEST_NAME="testXYZ" ...above mentioned params...

EOF
    ;;
    *)
    cat << EOF
make CPPUNIT_TEST_NAME="testXYZ" ...above mentioned params...

EOF
    ;;
esac

exit 1

# vim: set et sw=4:
