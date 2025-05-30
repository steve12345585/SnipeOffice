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

package pre2par::files;

use pre2par::exiter;

############################################
# File Operations
############################################

sub read_file
{
    my ($localfile) = @_;

    my @localfile = ();

    open( IN, "<$localfile" ) || pre2par::exiter::exit_program("ERROR: Cannot open file: $localfile", "read_file");
    while ( <IN> ) { push(@localfile, $_); }
    close( IN );

    return \@localfile;
}

###########################################
# Saving files
###########################################

sub save_file
{
    my ($savefile, $savecontent) = @_;
    if (-f $savefile) { unlink $savefile };
    if (-f $savefile) { pre2par::exiter::exit_program("ERROR: Cannot delete existing file: $savefile", "save_file"); };
    open( OUT, ">$savefile" );
    print OUT @{$savecontent};
    close( OUT);
    if (! -f $savefile) { pre2par::exiter::exit_program("ERROR: Cannot write file: $savefile", "save_file"); }
}

1;
