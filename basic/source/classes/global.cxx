/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <comphelper/processfactory.hxx>
#include <i18nlangtag/lang.h>
#include <i18nutil/transliteration.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>

#include <global.hxx>

namespace
{
    class lclTransliterationWrapper
    {
    private:
        utl::TransliterationWrapper m_aTransliteration;
    public:
        lclTransliterationWrapper()
            : m_aTransliteration(
                comphelper::getProcessComponentContext(),
                TransliterationFlags::IGNORE_CASE )
        {
            const LanguageType eOfficeLanguage = Application::GetSettings().GetLanguageTag().getLanguageType();
            m_aTransliteration.loadModuleIfNeeded( eOfficeLanguage );
        }
        utl::TransliterationWrapper& getTransliteration() { return m_aTransliteration; }
    };

}

utl::TransliterationWrapper& SbGlobal::GetTransliteration()
{
    static lclTransliterationWrapper theTransliterationWrapper;
    return theTransliterationWrapper.getTransliteration();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
