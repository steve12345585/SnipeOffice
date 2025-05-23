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

#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/text/XParagraphAppend.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <o3tl/any.hxx>
#include <XMLTextHeaderFooterContext.hxx>
#include <xmloff/xmlimp.hxx>


using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::xml::sax;
using namespace ::com::sun::star::text;
using namespace ::com::sun::star::beans;


XMLTextHeaderFooterContext::XMLTextHeaderFooterContext( SvXMLImport& rImport,
                        const Reference < XPropertySet > & rPageStylePropSet,
                       bool bFooter, bool bLft, bool bFrst ) :
    SvXMLImportContext( rImport ),
    xPropSet( rPageStylePropSet ),
    sOn( bFooter ? u"FooterIsOn"_ustr : u"HeaderIsOn"_ustr ),
    sShareContent( bFooter ? u"FooterIsShared"_ustr : u"HeaderIsShared"_ustr ),
    sText( bFooter ? u"FooterText"_ustr : u"HeaderText"_ustr ),
    sTextFirst(bFooter ? u"FooterTextFirst"_ustr : u"HeaderTextFirst"_ustr),
    sTextLeft( bFooter ?  u"FooterTextLeft"_ustr : u"HeaderTextLeft"_ustr ),
    bInsertContent( true ),
    bLeft( bLft ),
    bFirst( bFrst )
{
    // NOTE: if this ever handles XML_DISPLAY attr then beware of fdo#72850 !
    if( !(bLeft || bFirst) )
        return;

    Any aAny = xPropSet->getPropertyValue( sOn );
    bool bOn = *o3tl::doAccess<bool>(aAny);

    if( bOn )
    {
        if (bLeft)
        {
            aAny = xPropSet->getPropertyValue( sShareContent );
            bool bShared = bool();
            if (!(aAny >>= bShared))
                assert(false); // should return a value!
            if( bShared )
            {
                // Don't share headers any longer
                xPropSet->setPropertyValue( sShareContent, Any(false) );
            }
        }
        if (bFirst)
        {
            static constexpr OUString sShareContentFirst( u"FirstIsShared"_ustr );
            aAny = xPropSet->getPropertyValue( sShareContentFirst );
            bool bSharedFirst = bool();
            if (!(aAny >>= bSharedFirst))
                assert(false); // should return a value!
            if( bSharedFirst )
            {
                // Don't share first/right headers any longer
                xPropSet->setPropertyValue( sShareContentFirst, Any(false) );
            }
        }
    }
    else
    {
        // If headers or footers are switched off, no content must be
        // inserted.
        bInsertContent = false;
    }
}

XMLTextHeaderFooterContext::~XMLTextHeaderFooterContext()
{
}

css::uno::Reference< css::xml::sax::XFastContextHandler > XMLTextHeaderFooterContext::createFastChildContext(
    sal_Int32 nElement,
    const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList )
{
    SvXMLImportContext *pContext = nullptr;
    if( bInsertContent )
    {
        if( !xOldTextCursor.is() )
        {
            bool bRemoveContent = true;
            Any aAny;
            if( bLeft || bFirst )
            {
                // Headers and footers are switched on already,
                // and they aren't shared.
                if (bLeft)
                    aAny = xPropSet->getPropertyValue( sTextLeft );
                else
                    aAny = xPropSet->getPropertyValue( sTextFirst );
            }
            else
            {
                aAny = xPropSet->getPropertyValue( sOn );
                bool bOn = *o3tl::doAccess<bool>(aAny);

                if( !bOn )
                {
                    // Switch header on
                    xPropSet->setPropertyValue( sOn, Any(true) );

                    // The content has not to be removed, because the header
                    // or footer is empty already.
                    bRemoveContent = false;
                }

                // If a header or footer is not shared, share it now.
                aAny = xPropSet->getPropertyValue( sShareContent );
                bool bShared = *o3tl::doAccess<bool>(aAny);
                if( !bShared )
                {
                    xPropSet->setPropertyValue( sShareContent, Any(true) );
                }

                aAny = xPropSet->getPropertyValue( sText );
            }

            Reference < XText > xText;
            aAny >>= xText;

            if( bRemoveContent )
            {
                xText->setString(OUString());
                // fdo#82165 shapes anchored at the beginning or end survive
                // setString("") - kill them the hard way: SwDoc::DelFullPara()
                uno::Reference<text::XParagraphAppend> const xAppend(
                        xText, uno::UNO_QUERY_THROW);
                uno::Reference<lang::XComponent> const xPara(
                    xAppend->finishParagraph(
                        uno::Sequence<beans::PropertyValue>()),
                    uno::UNO_QUERY_THROW);
                xPara->dispose();
            }

            rtl::Reference < XMLTextImportHelper > xTxtImport =
                GetImport().GetTextImport();

            xOldTextCursor = xTxtImport->GetCursor();
            xTxtImport->SetCursor( xText->createTextCursor() );
        }

        pContext =
            GetImport().GetTextImport()->CreateTextChildContext(
                GetImport(), nElement, xAttrList,
                XMLTextType::HeaderFooter );
    }

    return pContext;
}

void XMLTextHeaderFooterContext::endFastElement(sal_Int32 )
{
    if( xOldTextCursor.is() )
    {
        GetImport().GetTextImport()->DeleteParagraph();
        GetImport().GetTextImport()->SetCursor( xOldTextCursor );
    }
    else if( !bLeft )
    {
        // If no content has been inserted into the header or footer,
        // switch it off.
        xPropSet->setPropertyValue( sOn, Any(false) );
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
