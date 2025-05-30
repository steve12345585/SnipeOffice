/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#ifndef INCLUDED_SVL_MEMBERID_H
#define INCLUDED_SVL_MEMBERID_H

#define MID_X       1
#define MID_Y       2
#define MID_RECT_LEFT    3
#define MID_RECT_TOP     4
#define MID_WIDTH   5
#define MID_HEIGHT  6
#define MID_RECT_RIGHT   7

// SvxSizeItem
#define MID_SIZE_SIZE       0
#define MID_SIZE_WIDTH      1
#define MID_SIZE_HEIGHT     2

// SvxSearchItem
// XXX When changing the MID count here, also increment the corresponding
// SvxSearchItem SFX_DECL_TYPE(n) value in include/sfx2/msg.hxx to match, and
// add a member to struct SvxSearch in sfx2/sdi/sfxitems.sdi so that dependent
// slot items get generated.
#define MID_SEARCH_STYLEFAMILY          1
#define MID_SEARCH_CELLTYPE             2
#define MID_SEARCH_ROWDIRECTION         3
#define MID_SEARCH_ALLTABLES            4
#define MID_SEARCH_SEARCHFILTERED       5
#define MID_SEARCH_BACKWARD             6
#define MID_SEARCH_PATTERN              7
#define MID_SEARCH_CONTENT              8
#define MID_SEARCH_ASIANOPTIONS         9
#define MID_SEARCH_ALGORITHMTYPE        10
#define MID_SEARCH_FLAGS                11
#define MID_SEARCH_SEARCHSTRING         12
#define MID_SEARCH_REPLACESTRING        13
#define MID_SEARCH_LOCALE               14
#define MID_SEARCH_CHANGEDCHARS         15
#define MID_SEARCH_DELETEDCHARS         16
#define MID_SEARCH_INSERTEDCHARS        17
#define MID_SEARCH_TRANSLITERATEFLAGS   18
#define MID_SEARCH_COMMAND              19
#define MID_SEARCH_STARTPOINTX          20
#define MID_SEARCH_STARTPOINTY          21
#define MID_SEARCH_SEARCHFORMATTED      22
#define MID_SEARCH_ALGORITHMTYPE2       23

// SfxDocumentInfoItem
#define MID_DOCINFO_DESCRIPTION              0x13
#define MID_DOCINFO_KEYWORDS                 0x17
#define MID_DOCINFO_SUBJECT                  0x1b
#define MID_DOCINFO_TITLE                    0x1d
#define MID_DOCINFO_AUTOLOADENABLED          0x2d
#define MID_DOCINFO_AUTOLOADURL              0x2e
#define MID_DOCINFO_AUTOLOADSECS             0x2f
#define MID_DOCINFO_DEFAULTTARGET            0x30
#define MID_DOCINFO_USEUSERDATA              0x31
#define MID_DOCINFO_DELETEUSERDATA           0x32
#define MID_DOCINFO_USETHUMBNAILSAVE         0x33
#define MID_DOCINFO_CONTRIBUTOR              0x34
#define MID_DOCINFO_COVERAGE                 0x35
#define MID_DOCINFO_IDENTIFIER               0x38
#define MID_DOCINFO_PUBLISHER                0x3a
#define MID_DOCINFO_RELATION                 0x3b
#define MID_DOCINFO_RIGHTS                   0x3c
#define MID_DOCINFO_SOURCE                   0x3d
#define MID_DOCINFO_TYPE                     0x3e

// only for FastPropertySet
#define MID_TYPE                             0x3f
#define MID_VALUE                            0x40
#define MID_VALUESET                         0x41

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
