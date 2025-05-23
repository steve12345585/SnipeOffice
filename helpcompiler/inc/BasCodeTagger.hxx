/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <deque>
#include <memory>
#include <vector>
#include <libxml/parser.h>
#include <comphelper/syntaxhighlight.hxx>

class LibXmlTreeWalker;

//!Tagger class.
class BasicCodeTagger
{
  private:
    xmlDocPtr             m_pDocument;
    std::vector<xmlNodePtr> m_BasicCodeContainerTags;
    std::unique_ptr<LibXmlTreeWalker>  m_pXmlTreeWalker;
    SyntaxHighlighter     m_Highlighter;
    bool m_bTaggingCompleted;
    void tagParagraph( xmlNodePtr paragraph );
    static xmlChar* getTypeString( TokenType tokenType );
    void getBasicCodeContainerNodes();
    void tagBasCodeParagraphs();

  public:
    enum TaggerException { NULL_DOCUMENT, EMPTY_DOCUMENT };
    BasicCodeTagger( xmlDocPtr rootDoc );
    ~BasicCodeTagger();
    void tagBasicCodes();
};

//================LibXmlTreeWalker===========================================================

class LibXmlTreeWalker
{
  private:
    xmlNodePtr            m_pCurrentNode;
    std::deque<xmlNodePtr> m_Queue; //!Queue for breath-first search

  public:
    LibXmlTreeWalker( xmlDocPtr doc );
    void nextNode();
    xmlNodePtr currentNode() { return m_pCurrentNode;}
    bool end() const;
    void ignoreCurrNodesChildren();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
