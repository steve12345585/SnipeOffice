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

#include <SqlNameEdit.hxx>

namespace dbaui
{
    static bool isCharOk(sal_Unicode _cChar,bool _bFirstChar, std::u16string_view _sAllowedChars)
    {
        return  (
                 (_cChar >= 'A' && _cChar <= 'Z') ||
                 _cChar == '_' ||
                 _sAllowedChars.find(_cChar) != std::u16string_view::npos ||
                 (!_bFirstChar && (_cChar >= '0' && _cChar <= '9')) ||
                 (_cChar >= 'a' && _cChar <= 'z')
                );
    }
    bool OSQLNameChecker::checkString(std::u16string_view _sToCheck,
                                        OUString& _rsCorrected)
    {
        bool bCorrected = false;
        if ( m_bCheck )
        {
            sal_Int32 nMatch = 0;
            for (size_t i = nMatch; i < _sToCheck.size(); ++i)
            {
                if ( !isCharOk( _sToCheck[i], i == 0, m_sAllowedChars ) )
                {
                    _rsCorrected += _sToCheck.substr(nMatch, i - nMatch);
                    bCorrected = true;
                    nMatch = i + 1;
                }
            }
            _rsCorrected += _sToCheck.substr( nMatch );
        }
        return bCorrected;
    }

    namespace
    {
        void checkName(OSQLNameChecker& rChecker, weld::Entry& rEntry)
        {
            OUString sCorrected;
            if (rChecker.checkString(rEntry.get_text(), sCorrected))
            {
                int nStartPos, nEndPos;
                rEntry.get_selection_bounds(nStartPos, nEndPos);
                int nMin = std::min(nStartPos, nEndPos);
                rEntry.set_text(sCorrected);
                rEntry.select_region(nMin, nMin);

                rEntry.save_value();
            }
        }
    }

    IMPL_LINK(OSQLNameEditControl, ModifyHdl, weld::Entry&, rEntry, void)
    {
        checkName(*this, rEntry);
        m_ChainChangedHdl.Call(rEntry);
    }

    IMPL_LINK(OSQLNameEntry, ModifyHdl, weld::Entry&, rEntry, void)
    {
        checkName(*this, rEntry);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
