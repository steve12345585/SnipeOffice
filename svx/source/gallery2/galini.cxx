/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 * The world's quickest and lamest .desktop / .ini file parser.
 * Ideally the .thm file would move to a .desktop file in
 * future.
 */

#include <sal/config.h>
#include <sal/log.hxx>

#include <unotools/ucbstreamhelper.hxx>
#include <galleryfilestorageentry.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <o3tl/string_view.hxx>
#include <memory>

OUString GalleryFileStorageEntry::ReadStrFromIni(std::u16string_view aKeyName ) const
{
    std::unique_ptr<SvStream> pStrm(::utl::UcbStreamHelper::CreateStream(
        GetStrURL().GetMainURL( INetURLObject::DecodeMechanism::NONE ),
                                StreamMode::READ ));

    const LanguageTag &rLangTag = Application::GetSettings().GetUILanguageTag();

    ::std::vector< OUString > aFallbacks = rLangTag.getFallbackStrings( true);

    OUString aResult;
    sal_Int32 nRank = 42;

    if( pStrm )
    {
        OString aLine;
        while( pStrm->ReadLine( aLine ) )
        {
            OUString aKey;
            OUString aLocale;
            OUString aValue;
            sal_Int32 n;

            // comments
            if( aLine.startsWith( "#" ) )
                continue;

            // a[en_US] = Bob
            if( ( n = aLine.indexOf( '=' ) ) >= 1)
            {
                aKey = OStringToOUString(
                    o3tl::trim(aLine.subView( 0, n )), RTL_TEXTENCODING_ASCII_US );
                aValue = OStringToOUString(
                    o3tl::trim(aLine.subView( n + 1 )), RTL_TEXTENCODING_UTF8 );

                if( ( n = aKey.indexOf( '[' ) ) >= 1 )
                {
                    aLocale = o3tl::trim(aKey.subView( n + 1 ));
                    aKey = o3tl::trim(aKey.subView( 0, n ));
                    if( (n = aLocale.indexOf( ']' ) ) >= 1 )
                        aLocale = o3tl::trim(aLocale.subView( 0, n ));
                }
            }
            SAL_INFO("svx", "ini file has '" << aKey << "' [ '" << aLocale << "' ] = '" << aValue << "'");

            // grisly language matching, is this not available somewhere else?
            if( aKey == aKeyName )
            {
                /* FIXME-BCP47: what is this supposed to do? */
                n = 0;
                OUString aLang = aLocale.replace('_','-');
                for( const auto& rFallback : aFallbacks )
                {
                    SAL_INFO( "svx", "compare '" << aLang << "' with '" << rFallback << "' rank " << nRank << " vs. " << n );
                    if( rFallback == aLang && n < nRank ) {
                        nRank = n; // try to get the most accurate match
                        aResult = aValue;
                    }
                    ++n;
                }
            }
        }
    }

    SAL_INFO( "svx", "readStrFromIni returns '" << aResult << "'");
    return aResult;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
