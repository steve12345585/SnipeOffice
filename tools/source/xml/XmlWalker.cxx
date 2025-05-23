/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <tools/stream.hxx>
#include <tools/XmlWalker.hxx>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <vector>

namespace tools
{
struct XmlWalkerImpl
{
    XmlWalkerImpl()
        : mpDocPtr(nullptr)
        , mpRoot(nullptr)
        , mpCurrent(nullptr)
    {
    }

    xmlDocPtr mpDocPtr;
    xmlNodePtr mpRoot;
    xmlNodePtr mpCurrent;

    std::vector<xmlNodePtr> mpStack;
};

XmlWalker::XmlWalker()
    : mpImpl(std::make_unique<XmlWalkerImpl>())
{
}

XmlWalker::~XmlWalker()
{
    if (mpImpl)
        xmlFreeDoc(mpImpl->mpDocPtr);
}

bool XmlWalker::open(SvStream* pStream)
{
    std::size_t nSize = pStream->remainingSize();
    std::vector<sal_uInt8> aBuffer(nSize + 1);
    pStream->ReadBytes(aBuffer.data(), nSize);
    aBuffer[nSize] = 0;
    mpImpl->mpDocPtr = xmlParseDoc(reinterpret_cast<xmlChar*>(aBuffer.data()));
    if (mpImpl->mpDocPtr == nullptr)
        return false;
    mpImpl->mpRoot = xmlDocGetRootElement(mpImpl->mpDocPtr);
    mpImpl->mpCurrent = mpImpl->mpRoot;
    mpImpl->mpStack.push_back(mpImpl->mpCurrent);
    return true;
}

std::string_view XmlWalker::name()
{
    return reinterpret_cast<const char*>(mpImpl->mpCurrent->name);
}

std::string_view XmlWalker::namespaceHref()
{
    return reinterpret_cast<const char*>(mpImpl->mpCurrent->ns->href);
}

std::string_view XmlWalker::namespacePrefix()
{
    return reinterpret_cast<const char*>(mpImpl->mpCurrent->ns->prefix);
}

OString XmlWalker::content()
{
    OString aContent;
    if (mpImpl->mpCurrent->xmlChildrenNode != nullptr)
    {
        xmlChar* pContent
            = xmlNodeListGetString(mpImpl->mpDocPtr, mpImpl->mpCurrent->xmlChildrenNode, 1);
        aContent = OString(reinterpret_cast<const char*>(pContent));
        xmlFree(pContent);
    }
    return aContent;
}

void XmlWalker::children()
{
    mpImpl->mpStack.push_back(mpImpl->mpCurrent);
    mpImpl->mpCurrent = mpImpl->mpCurrent->xmlChildrenNode;
}

void XmlWalker::parent()
{
    mpImpl->mpCurrent = mpImpl->mpStack.back();
    mpImpl->mpStack.pop_back();
}

OString XmlWalker::attribute(const OString& sName) const
{
    xmlChar* xmlAttribute
        = xmlGetProp(mpImpl->mpCurrent, reinterpret_cast<const xmlChar*>(sName.getStr()));
    OString aAttributeContent(
        xmlAttribute == nullptr ? "" : reinterpret_cast<const char*>(xmlAttribute));
    xmlFree(xmlAttribute);

    return aAttributeContent;
}

void XmlWalker::next() { mpImpl->mpCurrent = mpImpl->mpCurrent->next; }

bool XmlWalker::isValid() const { return mpImpl->mpCurrent != nullptr; }

} // end tools namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
