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

#include <editeng/forbiddencharacterstable.hxx>

#include <unotools/localedatawrapper.hxx>
#include <utility>

SvxForbiddenCharactersTable::SvxForbiddenCharactersTable(
    css::uno::Reference<css::uno::XComponentContext> xContext)
    : m_xContext(std::move(xContext))
{
}

std::shared_ptr<SvxForbiddenCharactersTable>
SvxForbiddenCharactersTable::makeForbiddenCharactersTable(
    const css::uno::Reference<css::uno::XComponentContext>& rxContext)
{
    return std::shared_ptr<SvxForbiddenCharactersTable>(new SvxForbiddenCharactersTable(rxContext));
}

const css::i18n::ForbiddenCharacters*
SvxForbiddenCharactersTable::GetForbiddenCharacters(LanguageType nLanguage, bool bGetDefault)
{
    css::i18n::ForbiddenCharacters* pForbiddenCharacters = nullptr;
    Map::iterator it = maMap.find(nLanguage);
    if (it != maMap.end())
        pForbiddenCharacters = &(it->second);
    else if (bGetDefault && m_xContext.is())
    {
        const LocaleDataWrapper* pWrapper = LocaleDataWrapper::get(LanguageTag(nLanguage));
        maMap[nLanguage] = pWrapper->getForbiddenCharacters();
        pForbiddenCharacters = &maMap[nLanguage];
    }
    return pForbiddenCharacters;
}

void SvxForbiddenCharactersTable::SetForbiddenCharacters(
    LanguageType nLanguage, const css::i18n::ForbiddenCharacters& rForbiddenChars)
{
    maMap[nLanguage] = rForbiddenChars;
}

void SvxForbiddenCharactersTable::ClearForbiddenCharacters(LanguageType nLanguage)
{
    maMap.erase(nLanguage);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
