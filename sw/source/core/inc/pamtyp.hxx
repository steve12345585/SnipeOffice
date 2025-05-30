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

#ifndef INCLUDED_SW_SOURCE_CORE_INC_PAMTYP_HXX
#define INCLUDED_SW_SOURCE_CORE_INC_PAMTYP_HXX

#include <unotools/textsearch.hxx>

#include <memory>
#include <optional>

class SwpHints;
struct SwPosition;
class SwPaM;
class SwTextAttr;
class SwFormat;
class SfxPoolItem;
class SwRootFrame;
class SwNode;
class SwNodeIndex;
class SwContentNode;
class SwContentIndex;
class SvxSearchItem;
enum class SwCursorSkipMode;

namespace i18nutil {
    struct SearchOptions2;
}

// function prototypes for the move/find methods of SwPaM

void GoStartDoc( SwPosition*);
void GoEndDoc( SwPosition*);
void GoStartSection( SwPosition*);
void GoEndSection( SwPosition*);
void GoStartOfSection( SwPosition& );
void GoEndOfSection( SwPosition& );
const SwTextAttr* GetFrwrdTextHint( const SwpHints&, size_t&, sal_Int32 );
const SwTextAttr* GetBkwrdTextHint( const SwpHints&, size_t&, sal_Int32 );

bool GoNext(SwNode& rNd, SwContentIndex& rIdx, SwCursorSkipMode nMode );
bool GoPrevious(SwNode& rNd, SwContentIndex& rIdx, SwCursorSkipMode nMode );
SwContentNode* GoNextPos( SwPosition * pIdx, bool );
SwContentNode* GoPreviousPos( SwPosition * pIdx, bool );

// type definitions of functions
typedef bool (*GoNd)( SwNode&, SwContentIndex&, SwCursorSkipMode );
typedef SwContentNode* (*GoPos)( SwPosition*, bool );
typedef void (*GoDoc)( SwPosition* );
typedef void (*GoSection)( SwPosition* );
typedef bool (SwPosition::*CmpOp)( const SwPosition& ) const;
typedef const SwTextAttr* (*GetHint)( const SwpHints&, size_t&, sal_Int32 );
typedef bool (utl::TextSearch::*SearchText)( const OUString&, sal_Int32*,
                    sal_Int32*, css::util::SearchResult* );
typedef void (*MvSection)( SwPosition& );

struct SwMoveFnCollection
{
    GoNd      fnNd;
    GoPos     fnPos;
    GoDoc     fnDoc;
    GoSection fnSections;
    CmpOp     fnCmpOp;
    GetHint   fnGetHint;
    SearchText fnSearch;
    MvSection fnSection;
};

// function prototype for searching
SwContentNode* GetNode(SwPaM&, bool&, SwMoveFnCollection const &,
        bool bInReadOnly = false, SwRootFrame const* pLayout = nullptr);

namespace sw {

    void MakeRegion(SwMoveFnCollection const & fnMove,
            const SwPaM & rOrigRg, std::optional<SwPaM>& rDestinaton);

    /// Search.
    bool FindTextImpl(SwPaM & rSearchPam,
                const i18nutil::SearchOptions2& rSearchOpt,
                bool bSearchInNotes,
                utl::TextSearch& rSText,
                SwMoveFnCollection const & fnMove,
                const SwPaM & rRegion, bool bInReadOnly,
                SwRootFrame const* pLayout,
                std::unique_ptr<SvxSearchItem>& xSearchItem);
    bool FindFormatImpl(SwPaM & rSearchPam,
                const SwFormat& rFormat,
                SwMoveFnCollection const & fnMove,
                const SwPaM & rRegion, bool bInReadOnly,
                SwRootFrame const* pLayout);
    bool FindAttrImpl(SwPaM & rSearchPam,
                const SfxPoolItem& rAttr,
                SwMoveFnCollection const & fnMove,
                const SwPaM & rPam, bool bInReadOnly,
                SwRootFrame const* pLayout);

} // namespace sw

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
