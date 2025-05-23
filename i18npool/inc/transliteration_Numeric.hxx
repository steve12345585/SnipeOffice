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

namespace i18npool {

class transliteration_Numeric : public transliteration_commonclass {
public:
        virtual OUString
        transliterateImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, css::uno::Sequence< sal_Int32 >* pOffset ) override;

        virtual sal_Unicode SAL_CALL
        transliterateChar2Char( sal_Unicode inChar) override;

        // Methods which are shared.
        virtual sal_Int16 SAL_CALL getType(  ) override;

        virtual OUString
        foldingImpl( const OUString& inStr, sal_Int32 startPos, sal_Int32 nCount, css::uno::Sequence< sal_Int32 >* pOffset ) override;

        virtual sal_Bool SAL_CALL
        equals( const OUString& str1, sal_Int32 pos1, sal_Int32 nCount1, sal_Int32& nMatch1, const OUString& str2, sal_Int32 pos2, sal_Int32 nCount2, sal_Int32& nMatch2 ) override;

        virtual css::uno::Sequence< OUString > SAL_CALL
        transliterateRange( const OUString& str1, const OUString& str2 ) override;
protected:
        sal_Int16 nNativeNumberMode;
        sal_Int16 tableSize;
        const sal_Unicode* table;
        bool recycleSymbol;
private:
        /// @throws css::uno::RuntimeException
        OUString
        transliterateBullet( std::u16string_view inStr, sal_Int32 startPos, sal_Int32 nCount,
                             css::uno::Sequence< sal_Int32 >* pOffset ) const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
