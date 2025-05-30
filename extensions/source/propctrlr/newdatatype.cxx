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

#include "newdatatype.hxx"

namespace pcr
{


    //= NewDataTypeDialog


    NewDataTypeDialog::NewDataTypeDialog(weld::Window* pParent, std::u16string_view _rNameBase, const std::vector< OUString >& _rProhibitedNames)
        : GenericDialogController(pParent, u"modules/spropctrlr/ui/datatypedialog.ui"_ustr, u"DataTypeDialog"_ustr)
        , m_aProhibitedNames( _rProhibitedNames.begin(), _rProhibitedNames.end() )
        , m_xName(m_xBuilder->weld_entry(u"entry"_ustr))
        , m_xOK(m_xBuilder->weld_button(u"ok"_ustr))
    {
        m_xName->connect_changed(LINK(this, NewDataTypeDialog, OnNameModified));

        // find an initial name
        // for this, first remove trailing digits
        sal_Int32 nStripUntil = _rNameBase.size();
        while ( nStripUntil > 0 )
        {
            sal_Unicode nChar = _rNameBase[ --nStripUntil ];
            if ( ( nChar < '0' ) || ( nChar > '9' ) )
            {
                if ( nChar == ' ' )
                    --nStripUntil;  // strip the space, too
                break;
            }
        }

        OUString sNameBase = OUString::Concat(_rNameBase.substr( 0, nStripUntil ? nStripUntil + 1 : 0 )) + " ";
        OUString sInitialName;
        sal_Int32 nPostfixNumber = 1;
        do
        {
            sInitialName = sNameBase + OUString::number(nPostfixNumber++);
        }
        while ( m_aProhibitedNames.find( sInitialName ) != m_aProhibitedNames.end() );

        m_xName->set_text(sInitialName);
        OnNameModified(*m_xName);
    }

    NewDataTypeDialog::~NewDataTypeDialog()
    {
    }

    IMPL_LINK_NOARG(NewDataTypeDialog, OnNameModified, weld::Entry&, void)
    {
        OUString sCurrentName = GetName();
        bool bNameIsOK = ( !sCurrentName.isEmpty() )
                      && ( m_aProhibitedNames.find( sCurrentName ) == m_aProhibitedNames.end() );

        m_xOK->set_sensitive(bNameIsOK);
    }
} // namespace pcr


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
