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

#include <vcl/quickselectionengine.hxx>
#include <vcl/event.hxx>
#include <vcl/timer.hxx>
#include <vcl/i18nhelp.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <sal/log.hxx>

#include <optional>

namespace vcl
{

    struct QuickSelectionEngine_Data
    {
        ISearchableStringList&              rEntryList;
        OUString                            sCurrentSearchString;
        ::std::optional< sal_Unicode >    aSingleSearchChar;
        Timer                               aSearchTimeout;

        explicit QuickSelectionEngine_Data( ISearchableStringList& _entryList )
            :rEntryList( _entryList )
            ,aSearchTimeout( "vcl::QuickSelectionEngine_Data aSearchTimeout" )
        {
            aSearchTimeout.SetTimeout( 2500 );
            aSearchTimeout.SetInvokeHandler( LINK( this, QuickSelectionEngine_Data, SearchStringTimeout ) );
        }

        ~QuickSelectionEngine_Data()
        {
            aSearchTimeout.Stop();
        }

        DECL_LINK( SearchStringTimeout, Timer*, void );
    };

    namespace
    {
        void lcl_reset( QuickSelectionEngine_Data& _data )
        {
            _data.sCurrentSearchString.clear();
            _data.aSingleSearchChar.reset();
            _data.aSearchTimeout.Stop();
        }
    }

    IMPL_LINK_NOARG( QuickSelectionEngine_Data, SearchStringTimeout, Timer*, void )
    {
        lcl_reset( *this );
    }

    static StringEntryIdentifier findMatchingEntry( const OUString& _searchString, QuickSelectionEngine_Data const & _engineData )
    {
        const vcl::I18nHelper& rI18nHelper = Application::GetSettings().GetLocaleI18nHelper();
        // TODO: do we really need the Window's settings here? The original code used it ...

        OUString sEntryText;
        // get the "current + 1" entry
        StringEntryIdentifier pSearchEntry = _engineData.rEntryList.CurrentEntry( sEntryText );
        if ( pSearchEntry )
            pSearchEntry = _engineData.rEntryList.NextEntry( pSearchEntry, sEntryText );
        // loop 'til we find another matching entry
        StringEntryIdentifier pStartedWith = pSearchEntry;
        while ( pSearchEntry )
        {
            if ( rI18nHelper.MatchString( _searchString, sEntryText ) )
                break;

            pSearchEntry = _engineData.rEntryList.NextEntry( pSearchEntry, sEntryText );
            if ( pSearchEntry == pStartedWith )
                pSearchEntry = nullptr;
        }

        return pSearchEntry;
    }

    QuickSelectionEngine::QuickSelectionEngine( ISearchableStringList& _entryList )
        :m_pData( new QuickSelectionEngine_Data( _entryList ) )
    {
    }

    QuickSelectionEngine::~QuickSelectionEngine()
    {
    }

    bool QuickSelectionEngine::HandleKeyEvent( const KeyEvent& _keyEvent )
    {
        sal_Unicode c = _keyEvent.GetCharCode();

        if ( ( c >= 32 ) && ( c != 127 ) && !_keyEvent.GetKeyCode().IsMod2() )
        {
            m_pData->sCurrentSearchString += OUStringChar(c);
            SAL_INFO( "vcl", "QuickSelectionEngine::HandleKeyEvent: searching for " << m_pData->sCurrentSearchString );

            if ( m_pData->sCurrentSearchString.getLength() == 1 )
            {   // first character in the search -> remember
                m_pData->aSingleSearchChar = c;
            }
            else if ( m_pData->sCurrentSearchString.getLength() > 1 )
            {
                if ( !!m_pData->aSingleSearchChar && ( *m_pData->aSingleSearchChar != c ) )
                    // we already have a "single char", but the current one is different -> reset
                    m_pData->aSingleSearchChar.reset();
            }

            OUString aSearchTemp( m_pData->sCurrentSearchString );

            StringEntryIdentifier pMatchingEntry = findMatchingEntry( aSearchTemp, *m_pData );
            SAL_INFO( "vcl", "QuickSelectionEngine::HandleKeyEvent: found " << pMatchingEntry );
            if ( !pMatchingEntry && (aSearchTemp.getLength() > 1) && !!m_pData->aSingleSearchChar )
            {
                // if there's only one letter in the search string, use a different search mode
                aSearchTemp = OUString(*m_pData->aSingleSearchChar);
                pMatchingEntry = findMatchingEntry( aSearchTemp, *m_pData );
            }

            if ( pMatchingEntry )
            {
                m_pData->rEntryList.SelectEntry( pMatchingEntry );
                m_pData->aSearchTimeout.Start();
            }
            else
            {
                lcl_reset( *m_pData );
            }

            return true;
        }
        return false;
    }

    void QuickSelectionEngine::Reset()
    {
        lcl_reset( *m_pData );
    }

} // namespace vcl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
