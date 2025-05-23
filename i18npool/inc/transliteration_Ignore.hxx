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

#include "transliteration_commonclass.hxx"
#include <unicode/translit.h>
#include <memory>

namespace i18nutil { class oneToOneMapping; }

typedef sal_Unicode (*TransFunc)(const sal_Unicode);

namespace i18npool {

class transliteration_Ignore : public transliteration_commonclass
{
public:
        virtual OUString
        foldingImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, css::uno::Sequence< sal_Int32 >* pOffset ) override;

        // This method is shared.
        sal_Bool SAL_CALL
        equals( const OUString& str1, sal_Int32 pos1, sal_Int32 nCount1, sal_Int32& nMatch1,
            const OUString& str2, sal_Int32 pos2, sal_Int32 nCount2, sal_Int32& nMatch2 ) override;

        // This method is implemented in sub class if needed. Otherwise, the method implemented in this class will be used.
        css::uno::Sequence< OUString > SAL_CALL
        transliterateRange( const OUString& str1, const OUString& str2 ) override;


        // Methods which are shared.
        sal_Int16 SAL_CALL getType(  ) override;

        OUString
        transliterateImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, css::uno::Sequence< sal_Int32 >* pOffset ) override;

        virtual sal_Unicode SAL_CALL
        transliterateChar2Char( sal_Unicode inChar) override;

        /// @throws css::uno::RuntimeException
        static css::uno::Sequence< OUString >
        transliterateRange( const OUString& str1, const OUString& str2, XTransliteration& t1, XTransliteration& t2 );

        struct Mapping {
            sal_Unicode previousChar;
            sal_Unicode currentChar;
            sal_Unicode replaceChar;
            bool two2one;
        };

protected:
        TransFunc func;
        i18nutil::oneToOneMapping *table;
        const Mapping *map;
};

#define TRANSLITERATION_IGNORE( name ) \
class ignore##name final : public transliteration_Ignore {\
public:\
        ignore##name ();\
};

TRANSLITERATION_IGNORE(BaFa_ja_JP)
TRANSLITERATION_IGNORE(HyuByu_ja_JP)
TRANSLITERATION_IGNORE(SeZe_ja_JP)
TRANSLITERATION_IGNORE(TiJi_ja_JP)
TRANSLITERATION_IGNORE(MiddleDot_ja_JP)
TRANSLITERATION_IGNORE(MinusSign_ja_JP)
TRANSLITERATION_IGNORE(Separator_ja_JP)
TRANSLITERATION_IGNORE(Space_ja_JP)
TRANSLITERATION_IGNORE(TraditionalKana_ja_JP)
TRANSLITERATION_IGNORE(TraditionalKanji_ja_JP)
TRANSLITERATION_IGNORE(ZiZu_ja_JP)
TRANSLITERATION_IGNORE(Kashida_CTL)

class ignoreDiacritics_CTL final : public transliteration_Ignore
{
    std::unique_ptr<icu::Transliterator> m_transliterator;

public:
    ignoreDiacritics_CTL();

    OUString
    foldingImpl(const OUString& rInStr, sal_Int32 nStartPos, sal_Int32 nCount, css::uno::Sequence<sal_Int32>* pOffset) override;

    sal_Unicode SAL_CALL
    transliterateChar2Char(sal_Unicode nInChar) override;
};

#undef TRANSLITERATION_IGNORE

#define TRANSLITERATION_IGNORE( name ) \
class ignore##name final : public transliteration_Ignore {\
public:\
        ignore##name () {\
            func = nullptr;\
            table = nullptr;\
            map = nullptr;\
            transliterationName = "ignore"#name;\
            implementationName = "com.sun.star.i18n.Transliteration.ignore"#name;\
        };\
        OUString foldingImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, \
                css::uno::Sequence< sal_Int32 >* pOffset) override; \
};

TRANSLITERATION_IGNORE(KiKuFollowedBySa_ja_JP)
TRANSLITERATION_IGNORE(IandEfollowedByYa_ja_JP)
TRANSLITERATION_IGNORE(IterationMark_ja_JP)
TRANSLITERATION_IGNORE(ProlongedSoundMark_ja_JP)

#undef TRANSLITERATION_IGNORE

#define TRANSLITERATION_IGNORE( name, implname ) \
class ignore##name final : public transliteration_Ignore {\
public:\
        ignore##name () {\
            func = nullptr;\
            table = nullptr;\
            map = nullptr;\
            transliterationName = "ignore"#name;\
            implementationName = "com.sun.star.i18n.Transliteration." implname;\
        };\
        OUString foldingImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, \
                css::uno::Sequence< sal_Int32 >* pOffset) override; \
        using transliteration_Ignore::transliterateRange;\
        css::uno::Sequence< OUString > SAL_CALL transliterateRange( const OUString& str1, \
                const OUString& str2 ) override; \
        sal_Unicode SAL_CALL \
        transliterateChar2Char( sal_Unicode inChar) \
             override;\
};

TRANSLITERATION_IGNORE(Kana, "IGNORE_KANA")
TRANSLITERATION_IGNORE(Width, "IGNORE_WIDTH")
TRANSLITERATION_IGNORE(Size_ja_JP, "ignoreSize_ja_JP")

#undef TRANSLITERATION_IGNORE

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
