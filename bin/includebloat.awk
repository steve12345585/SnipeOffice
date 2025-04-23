#!/usr/bin/gawk -f
# -*- tab-width: 4; indent-tabs-mode: t -*-
#
# This file is Part of the SnipeOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

# Generate a list of files included by the C++ compiler during the build
# sorted by the total bytes an included file contributed to preprocessor input.
# usage: first do a full build with "make check", then run this from $BUILDDIR

# NOTE: by default gbuild does not generate dependencies for system headers
#    (in particular the C++ standard library), so those will NOT be counted

BEGIN {
    cmd = "find workdir/Dep/CxxObject/ -name *.d | xargs cat"
    while ((cmd | getline) > 0) {
        if ($0 ~ /^ .*\\$/) {
            gsub(/^ /, "");
            gsub(/ *\\$/, "");
            includes[$1]++
            if ($2) {
                # GCC emits 2 per line if short enough!
                includes[$2]++
            }
        }
    }
    exit
}

END {
    for (inc in includes) {
        cmd = "wc -c " inc
        if ((cmd | getline) < 0)
            print "ERROR on: " cmd
        sizes[inc] = $1 # $0 is wc -c output, $1 is size
        totals[inc] = $1 * includes[inc]
        totalsize += totals[inc]
        close(cmd)
    }
    PROCINFO["sorted_in"] = "@val_num_desc"
    printf "Sum total bytes included (excluding system headers): %'d\n", totalsize
    print "Total bytes\tSize\t   Occurrences\tFilename"
    OFS="\t"
    for (inc in totals) {
        printf "%'13d\t%'7d\t%'8d\t%s\n", totals[inc], sizes[inc], includes[inc], inc
    }
}

# vim: set noet sw=4 ts=4:
