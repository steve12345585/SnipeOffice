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

#ifndef INCLUDED_SDEXT_SOURCE_PDFIMPORT_TREE_WRITERTREEVISITING_HXX
#define INCLUDED_SDEXT_SOURCE_PDFIMPORT_TREE_WRITERTREEVISITING_HXX

#include <treevisiting.hxx>

#include <pdfihelper.hxx>

#include <com/sun/star/i18n/XBreakIterator.hpp>
#include <com/sun/star/i18n/XCharacterClassification.hpp>

namespace pdfi
{
    struct DrawElement;

    class WriterXmlOptimizer : public ElementTreeVisitor
    {
    private:
        PDFIProcessor& m_rProcessor;
        css::uno::Reference<css::i18n::XBreakIterator> mxBreakIter;
        void optimizeTextElements(Element& rParent);
        void checkHeaderAndFooter( PageElement& rElem );

    public:
        const css::uno::Reference<css::i18n::XBreakIterator>& GetBreakIterator();
        explicit WriterXmlOptimizer(PDFIProcessor& rProcessor) :
            m_rProcessor(rProcessor)
        {}

        virtual void visit( HyperlinkElement&, const std::list< std::unique_ptr<Element> >::const_iterator& ) override;
        virtual void visit( TextElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( ParagraphElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( FrameElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( PolyPolyElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( ImageElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( PageElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( DocumentElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
    };

    class WriterXmlFinalizer : public ElementTreeVisitor
    {
    private:
        StyleContainer& m_rStyleContainer;
        PDFIProcessor&  m_rProcessor;

        static void setFirstOnPage( ParagraphElement&    rElem,
                             StyleContainer&      rStyles,
                             const OUString& rMasterPageName );

    public:
        explicit WriterXmlFinalizer(StyleContainer& rStyleContainer,
                                    PDFIProcessor&  rProcessor) :
            m_rStyleContainer(rStyleContainer),
            m_rProcessor(rProcessor)
        {}

        virtual void visit( HyperlinkElement&, const std::list< std::unique_ptr<Element> >::const_iterator& ) override;
        virtual void visit( TextElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( ParagraphElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( FrameElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( PolyPolyElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( ImageElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( PageElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( DocumentElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
    };

    class WriterXmlEmitter : public ElementTreeVisitor
    {
    private:
        css::uno::Reference< css::i18n::XCharacterClassification > mxCharClass;
        EmitContext& m_rEmitContext ;
        static void fillFrameProps( DrawElement&       rElem,
                             PropertyMap&       rProps,
                             const EmitContext& rEmitContext );

    public:
        const css::uno::Reference<css::i18n::XCharacterClassification >& GetCharacterClassification();
        explicit WriterXmlEmitter(EmitContext& rEmitContext) :
            m_rEmitContext(rEmitContext)
        {}

        virtual void visit( HyperlinkElement&, const std::list< std::unique_ptr<Element> >::const_iterator& ) override;
        virtual void visit( TextElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( ParagraphElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( FrameElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( PolyPolyElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( ImageElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( PageElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
        virtual void visit( DocumentElement&, const std::list< std::unique_ptr<Element> >::const_iterator&  ) override;
    };
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
