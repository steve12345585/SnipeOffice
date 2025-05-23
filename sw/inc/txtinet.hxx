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
#ifndef INCLUDED_SW_INC_TXTINET_HXX
#define INCLUDED_SW_INC_TXTINET_HXX

#include "txatbase.hxx"
#include "calbck.hxx"

class SwTextNode;
class SwCharFormat;

/// SwTextAttr subclass that tracks the location of the wrapped SwFormatINetFormat.
class SW_DLLPUBLIC SwTextINetFormat final: public SwTextAttrNesting, public SwClient
{
    private:
        SwTextNode* m_pTextNode;
        bool m_bVisited         : 1; // visited link?
        bool m_bVisitedValid    : 1; // is m_bVisited valid?
        virtual void SwClientNotify(const SwModify&, const SfxHint&) override;

    public:
        SwTextINetFormat(
            const SfxPoolItemHolder& rAttr,
            sal_Int32 nStart,
            sal_Int32 nEnd );
        virtual ~SwTextINetFormat() override;

        SAL_DLLPRIVATE void InitINetFormat(SwTextNode & rNode);

        // get and set TextNode pointer
        const SwTextNode* GetpTextNode() const { return m_pTextNode; }
        inline const SwTextNode& GetTextNode() const;
        inline SwTextNode& GetTextNode();
        void ChgTextNode( SwTextNode* pNew ) { m_pTextNode = pNew; }

              SwCharFormat* GetCharFormat();
        const SwCharFormat* GetCharFormat() const
                { return const_cast<SwTextINetFormat*>(this)->GetCharFormat(); }

        bool IsVisited() const { return m_bVisited; }
        void SetVisited( bool bNew ) { m_bVisited = bNew; }

        bool IsVisitedValid() const { return m_bVisitedValid; }
        void SetVisitedValid( bool bNew ) { m_bVisitedValid = bNew; }

        bool IsProtect() const;
};

inline const SwTextNode& SwTextINetFormat::GetTextNode() const
{
    assert( m_pTextNode );
    return *m_pTextNode;
}

inline SwTextNode& SwTextINetFormat::GetTextNode()
{
    assert( m_pTextNode );
    return *m_pTextNode;
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
