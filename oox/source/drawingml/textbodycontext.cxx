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

#include <drawingml/textbodycontext.hxx>
#include <drawingml/textbodypropertiescontext.hxx>
#include <drawingml/textparagraph.hxx>
#include <drawingml/textparagraphpropertiescontext.hxx>
#include <drawingml/textcharacterpropertiescontext.hxx>
#include <drawingml/textliststylecontext.hxx>
#include <drawingml/textfield.hxx>
#include <drawingml/textfieldcontext.hxx>
#include <oox/drawingml/shape.hxx>
#include <oox/token/namespaces.hxx>
#include <oox/helper/attributelist.hxx>
#include <sax/fastattribs.hxx>
#include "hyperlinkcontext.hxx"

#include <oox/mathml/imexport.hxx>

#include <sal/log.hxx>
#include <utility>

using namespace ::oox::core;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::text;
using namespace ::com::sun::star::xml::sax;

namespace oox::drawingml {

namespace {

// CT_TextParagraph
class TextParagraphContext : public ContextHandler2
{
public:
    TextParagraphContext( ContextHandler2Helper const & rParent, TextParagraph& rPara );

    virtual ContextHandlerRef onCreateContext( sal_Int32 aElementToken, const AttributeList& rAttribs ) override;

protected:
    TextParagraph& mrParagraph;
};

}

TextParagraphContext::TextParagraphContext( ContextHandler2Helper const & rParent, TextParagraph& rPara )
: ContextHandler2( rParent )
, mrParagraph( rPara )
{
    mbEnableTrimSpace = false;
}

ContextHandlerRef TextParagraphContext::onCreateContext( sal_Int32 aElementToken, const AttributeList& rAttribs )
{
    // EG_TextRun
    switch( aElementToken )
    {
        case A_TOKEN( r ):      // "CT_RegularTextRun" Regular Text Run.
        case W_TOKEN( r ):
        {
            TextRunPtr pRun = std::make_shared<TextRun>();
            mrParagraph.addRun( pRun );
            return new RegularTextRunContext( *this, std::move(pRun) );
        }
        case A_TOKEN( br ): // "CT_TextLineBreak" Soft return line break (vertical tab).
        {
            TextRunPtr pRun = std::make_shared<TextRun>();
            pRun->setLineBreak();
            mrParagraph.addRun( pRun );
            return new RegularTextRunContext( *this, std::move(pRun) );
        }
        case A_TOKEN( fld ):    // "CT_TextField" Text Field.
        {
            auto pField = std::make_shared<TextField>();
            mrParagraph.addRun( pField );
            return new TextFieldContext( *this, rAttribs, *pField );
        }
        case A_TOKEN( pPr ):
        case W_TOKEN( pPr ):
            mrParagraph.setHasProperties();
            return new TextParagraphPropertiesContext( *this, rAttribs, mrParagraph.getProperties() );
        case A_TOKEN( endParaRPr ):
            return new TextCharacterPropertiesContext( *this, rAttribs, mrParagraph.getEndProperties() );
        case W_TOKEN( sdt ):
        case W_TOKEN( sdtContent ):
            return this;
        case W_TOKEN( del ):
        break;
        case W_TOKEN( ins ):
            return this;
        case OOX_TOKEN(a14, m):
            return CreateLazyMathBufferingContext(*this, mrParagraph);
        case W_TOKEN( hyperlink ):
        {
            TextRunPtr pRun = std::make_shared<TextRun>();
            mrParagraph.addRun(pRun);
            // parse hyperlink attributes: use HyperLinkContext for that
            rtl::Reference<HyperLinkContext> pContext(new HyperLinkContext(
                *this, rAttribs, pRun->getTextCharacterProperties().maHyperlinkPropertyMap));
            // but create text run context because HyperLinkContext can't process internal w:r, w:t, etc
            return new RegularTextRunContext(*this, std::move(pRun));
        }
        default:
            SAL_WARN("oox", "TextParagraphContext::onCreateContext: unhandled element: " << getBaseToken(aElementToken));
    }

    return nullptr;
}

RegularTextRunContext::RegularTextRunContext( ContextHandler2Helper const & rParent, TextRunPtr pRunPtr )
: ContextHandler2( rParent )
, mpRunPtr(std::move( pRunPtr ))
, mbIsInText( false )
{
}

void RegularTextRunContext::onEndElement( )
{
    switch( getCurrentElement() )
    {
        case A_TOKEN( t ):
        case W_TOKEN( t ):
        {
            mbIsInText = false;
            break;
        }
        case A_TOKEN( r ):
        break;
    }
}

void RegularTextRunContext::onCharacters( const OUString& aChars )
{
    if( mbIsInText )
    {
        mpRunPtr->getText() += aChars;
    }
}

ContextHandlerRef RegularTextRunContext::onCreateContext( sal_Int32 aElementToken, const AttributeList& rAttribs)
{
    switch( aElementToken )
    {
        case A_TOKEN( rPr ):    // "CT_TextCharPropertyBag" The text char properties of this text run.
        case W_TOKEN( rPr ):
            return new TextCharacterPropertiesContext( *this, rAttribs, mpRunPtr->getTextCharacterProperties() );
        case A_TOKEN( t ):      // "xsd:string" minOccurs="1" The actual text string.
        case W_TOKEN( t ):
            mbIsInText = true;
        break;
        case W_TOKEN( drawing ):
        break;
        default:
            SAL_WARN("oox", "RegularTextRunContext::onCreateContext: unhandled element: " << getBaseToken(aElementToken));
        break;
    }

    return this;
}

TextBodyContext::TextBodyContext( ContextHandler2Helper const & rParent, TextBody& rTextBody )
: ContextHandler2( rParent )
, mrTextBody( rTextBody )
{
}

TextBodyContext::TextBodyContext(ContextHandler2Helper const& rParent, const ShapePtr& pShapePtr)
    : TextBodyContext(rParent, *pShapePtr->getTextBody())
{
    mpShapePtr = pShapePtr;
}

ContextHandlerRef TextBodyContext::onCreateContext( sal_Int32 aElementToken, const AttributeList& rAttribs )
{
    switch( aElementToken )
    {
        case A_TOKEN( bodyPr ):     // CT_TextBodyPropertyBag
            if (sax_fastparser::castToFastAttributeList(rAttribs.getFastAttributeList()).getFastAttributeTokens().size() > 0)
                mrTextBody.setHasNoninheritedBodyProperties();
            if ( mpShapePtr )
                return new TextBodyPropertiesContext( *this, rAttribs, mpShapePtr );
            else
                return new TextBodyPropertiesContext( *this, rAttribs, mrTextBody.getTextProperties() );
        case A_TOKEN( lstStyle ):   // CT_TextListStyle
            return new TextListStyleContext( *this, mrTextBody.getTextListStyle() );
        case A_TOKEN( p ):          // CT_TextParagraph
        case W_TOKEN( p ):
            return new TextParagraphContext( *this, mrTextBody.addParagraph() );
        case W_TOKEN( sdt ):
        case W_TOKEN( sdtContent ):
            return this;
        case W_TOKEN( sdtPr ):
        case W_TOKEN( sdtEndPr ):
        break;
        case W_TOKEN( tbl ):
        break;
        default:
            SAL_WARN("oox", "TextBodyContext::onCreateContext: unhandled element: " << getBaseToken(aElementToken));
    }

    return nullptr;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
