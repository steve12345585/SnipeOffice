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


package pre2par::pathanalyzer;

use pre2par::globals;

###########################################
# Path analyzer
###########################################

sub get_path_from_fullqualifiedname
{
    my ($longfilenameref) = @_;

    if ( $$longfilenameref =~ /\Q$pre2par::globals::separator\E/ )  # Is there a separator in the path? Otherwise the path is empty.
    {
        if ( $$longfilenameref =~ /^\s*(\S.*\S\Q$pre2par::globals::separator\E)(\S.+?\S)/ )
        {
            $$longfilenameref = $1;
        }
    }
    else
    {
        $$longfilenameref = ""; # there is no path
    }
}

sub make_absolute_filename_to_relative_filename
{
    my ($longfilenameref) = @_;

    if ( $pre2par::globals::isunix )
    {
        if ( $$longfilenameref =~ /^.*\/(?=\S)([^\/]+)(?<=\S)/ )
        {
            $$longfilenameref = $1;
        }
    }

    if ( $pre2par::globals::iswin )
    {
        # Either '/' or '\'.
        if ( $$longfilenameref =~ /^.*[\/\\](?=\S)([^\/\\]+)(?<=\S)/ )
        {
            $$longfilenameref = $1;
        }
    }
}

1;
