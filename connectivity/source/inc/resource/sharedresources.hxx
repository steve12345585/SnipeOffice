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

#pragma once

#include <rtl/ustring.hxx>
#include <vector>
#include <connectivity/dbtoolsdllapi.hxx>
#include <unotools/resmgr.hxx>

namespace connectivity
{


    typedef sal_uInt16  ResourceId;

    /** helper class for accessing resources shared by different libraries
        in the connectivity module
    */
    class OOO_DLLPUBLIC_DBTOOLS SharedResources
    {
    public:
        SharedResources();
        ~SharedResources();

        /** loads a string from the shared resource file
            @param  pResId
                the resource ID of the string
            @return
                the string from the resource file
        */
        OUString
            getResourceString(
                TranslateId pResId
            ) const;

        /** loads a string from the shared resource file, and replaces
            a given ASCII pattern with a given string

            @param  pResId
                the resource ID of the string to load
            @param  _pAsciiPatternToReplace
                the ASCII string which is to search in the string. Must not be <NULL/>.
            @param  _rStringToSubstitute
                the String which should substitute the ASCII pattern.

            @return
                the string from the resource file, with applied string substitution
        */
        OUString
            getResourceStringWithSubstitution(
                TranslateId pResId,
                const char* _pAsciiPatternToReplace,
                const OUString& _rStringToSubstitute
            ) const;

        /** loads a string from the shared resource file, and replaces
            a given ASCII pattern with a given string

            @param  pResId
                the resource ID of the string to load
            @param  _pAsciiPatternToReplace1
                the ASCII string (1) which is to search in the string. Must not be <NULL/>.
            @param  _rStringToSubstitute1
                the String which should substitute the ASCII pattern (1)
            @param  _pAsciiPatternToReplace2
                the ASCII string (2) which is to search in the string. Must not be <NULL/>.
            @param  _rStringToSubstitute2
                the String which should substitute the ASCII pattern (2)

            @return
                the string from the resource file, with applied string substitution
        */
        OUString
            getResourceStringWithSubstitution(
                TranslateId pResId,
                const char* _pAsciiPatternToReplace1,
                const OUString& _rStringToSubstitute1,
                const char* _pAsciiPatternToReplace2,
                const OUString& _rStringToSubstitute2
            ) const;

        /** loads a string from the shared resource file, and replaces
            a given ASCII pattern with a given string

            @param  pResId
                the resource ID of the string to load
            @param  _pAsciiPatternToReplace1
                the ASCII string (1) which is to search in the string. Must not be <NULL/>.
            @param  _rStringToSubstitute1
                the String which should substitute the ASCII pattern (1)
            @param  _pAsciiPatternToReplace2
                the ASCII string (2) which is to search in the string. Must not be <NULL/>.
            @param  _rStringToSubstitute2
                the String which should substitute the ASCII pattern (2)
            @param  _pAsciiPatternToReplace3
                the ASCII string (3) which is to search in the string. Must not be <NULL/>.
            @param  _rStringToSubstitute3
                the String which should substitute the ASCII pattern (3)

            @return
                the string from the resource file, with applied string substitution
        */
        OUString
            getResourceStringWithSubstitution(
                TranslateId pResId,
                const char* _pAsciiPatternToReplace1,
                const OUString& _rStringToSubstitute1,
                const char* _pAsciiPatternToReplace2,
                const OUString& _rStringToSubstitute2,
                const char* _pAsciiPatternToReplace3,
                const OUString& _rStringToSubstitute3
            ) const;

        /** loads a string from the shared resource file, and replaces a given ASCII pattern with a given string

            @param  pResId
                the resource ID of the string to load
            @param  _aStringToSubstitutes
                A list of substitutions.

            @return
                the string from the resource file, with applied string substitution
        */
        OUString getResourceStringWithSubstitution( TranslateId pResId,
                    const std::vector< std::pair<const char* , OUString > >& _rStringToSubstitutes) const;
    };


} // namespace connectivity


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
