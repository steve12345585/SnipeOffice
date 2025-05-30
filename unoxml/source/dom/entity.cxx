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

#include "entity.hxx"

#include <osl/diagnose.h>

#include <string.h>
#include <libxml/entities.h>

using namespace css::uno;
using namespace css::xml::dom;

namespace DOM
{

    CEntity::CEntity(CDocument const& rDocument, ::osl::Mutex const& rMutex,
            xmlEntityPtr const pEntity)
        : CEntity_Base(rDocument, rMutex,
            NodeType_ENTITY_NODE, reinterpret_cast<xmlNodePtr>(pEntity))
        , m_aEntityPtr(pEntity)
    {
    }

    bool CEntity::IsChildTypeAllowed(NodeType const nodeType, NodeType const*const)
    {
        switch (nodeType) {
            case NodeType_ELEMENT_NODE:
            case NodeType_PROCESSING_INSTRUCTION_NODE:
            case NodeType_COMMENT_NODE:
            case NodeType_TEXT_NODE:
            case NodeType_CDATA_SECTION_NODE:
            case NodeType_ENTITY_REFERENCE_NODE:
                return true;
            default:
                return false;
        }
    }

    /**
    For unparsed entities, the name of the notation for the entity.
    */
    OUString SAL_CALL CEntity::getNotationName()
    {
        OSL_ENSURE(false,
                "CEntity::getNotationName: not implemented (#i113683#)");
        return OUString();
    }

    /**
    The public identifier associated with the entity, if specified.
    */
    OUString SAL_CALL CEntity::getPublicId()
    {
        ::osl::MutexGuard const g(m_rMutex);

        OUString aID;
        if(m_aEntityPtr != nullptr)
        {
            aID = OUString(reinterpret_cast<char const *>(m_aEntityPtr->ExternalID), strlen(reinterpret_cast<char const *>(m_aEntityPtr->ExternalID)), RTL_TEXTENCODING_UTF8);
        }
        return aID;
    }

    /**
    The system identifier associated with the entity, if specified.
    */
    OUString SAL_CALL CEntity::getSystemId()
    {
        ::osl::MutexGuard const g(m_rMutex);

        OUString aID;
        if(m_aEntityPtr != nullptr)
        {
            aID = OUString(reinterpret_cast<char const *>(m_aEntityPtr->SystemID), strlen(reinterpret_cast<char const *>(m_aEntityPtr->SystemID)), RTL_TEXTENCODING_UTF8);
        }
        return aID;
    }
    OUString SAL_CALL CEntity::getNodeName()
    {
        ::osl::MutexGuard const g(m_rMutex);

        OUString aName;
        if (m_aNodePtr != nullptr)
        {
            const xmlChar* pName = m_aNodePtr->name;
            aName = OUString(reinterpret_cast<char const *>(pName), strlen(reinterpret_cast<char const *>(pName)), RTL_TEXTENCODING_UTF8);
        }
        return aName;
    }
    OUString SAL_CALL CEntity::getNodeValue()
    {
        return OUString();
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
