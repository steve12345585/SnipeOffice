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

#include "comment.hxx"

#include <com/sun/star/xml/sax/XExtendedDocumentHandler.hpp>

using namespace css::uno;
using namespace css::xml::dom;
using namespace css::xml::sax;

namespace DOM
{
    CComment::CComment(CDocument const& rDocument, ::osl::Mutex const& rMutex,
            xmlNodePtr const pNode)
        : CComment_Base(rDocument, rMutex, NodeType_COMMENT_NODE, pNode)
    {
    }

    void CComment::saxify(
            const Reference< XDocumentHandler >& i_xHandler) {
        if (!i_xHandler.is()) throw RuntimeException();
        Reference< XExtendedDocumentHandler > xExtended(i_xHandler, UNO_QUERY);
        if (xExtended.is()) {
            xExtended->comment(getData());
        }
    }

    OUString SAL_CALL CComment::getNodeName()
    {
        return u"#comment"_ustr;
    }

    OUString SAL_CALL CComment::getNodeValue()
    {
        return CCharacterData::getData();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
