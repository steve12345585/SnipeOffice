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
#ifndef INCLUDED_SW_SOURCE_CORE_ACCESS_TEXTMARKUPHELPER_HXX
#define INCLUDED_SW_SOURCE_CORE_ACCESS_TEXTMARKUPHELPER_HXX

#include <sal/types.h>
#include <com/sun/star/uno/Sequence.h>

namespace com::sun::star::accessibility {
    struct TextSegment;
}

class SwAccessiblePortionData;
class SwTextFrame;
class SwWrongList; // #i108125#

namespace sw {
    class WrongListIteratorCounter;
}

class SwTextMarkupHelper
{
    public:
        SwTextMarkupHelper( const SwAccessiblePortionData& rPortionData,
                            const SwTextFrame& rTextFrame);
        SwTextMarkupHelper( const SwAccessiblePortionData& rPortionData,
                            const SwWrongList& rTextMarkupList ); // #i108125#

        /// @throws css::lang::IllegalArgumentException
        /// @throws css::uno::RuntimeException
        sal_Int32 getTextMarkupCount( const sal_Int32 nTextMarkupType );

        /// @throws css::lang::IndexOutOfBoundsException
        /// @throws css::lang::IllegalArgumentException
        /// @throws css::uno::RuntimeException
        css::accessibility::TextSegment getTextMarkup(
                                            const sal_Int32 nTextMarkupIndex,
                                            const sal_Int32 nTextMarkupType );

        /// @throws css::lang::IndexOutOfBoundsException
        /// @throws css::lang::IllegalArgumentException
        /// @throws css::uno::RuntimeException
        css::uno::Sequence< css::accessibility::TextSegment >
                getTextMarkupAtIndex( const sal_Int32 nCharIndex,
                                      const sal_Int32 nTextMarkupType );

    private:
        SwTextMarkupHelper( const SwTextMarkupHelper& ) = delete;
        SwTextMarkupHelper& operator=( const SwTextMarkupHelper& ) = delete;

        std::unique_ptr<sw::WrongListIteratorCounter> getIterator(sal_Int32 nTextMarkupType);

        const SwAccessiblePortionData& mrPortionData;

        SwTextFrame const* m_pTextFrame;
        const SwWrongList* mpTextMarkupList;
};
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
