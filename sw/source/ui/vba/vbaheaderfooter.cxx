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
#include "vbaheaderfooter.hxx"
#include <ooo/vba/word/WdHeaderFooterIndex.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>
#include "vbarange.hxx"
#include <utility>
#include <vbahelper/vbashapes.hxx>
#include <unotxdoc.hxx>
#include <unodraw.hxx>

using namespace ::ooo::vba;
using namespace ::com::sun::star;

SwVbaHeaderFooter::SwVbaHeaderFooter( const uno::Reference< ooo::vba::XHelperInterface >& rParent,
                                      const uno::Reference< uno::XComponentContext >& rContext,
                                      rtl::Reference< SwXTextDocument >  xModel,
                                      uno::Reference< beans::XPropertySet > xProps,
                                      bool isHeader, sal_Int32 index )
: SwVbaHeaderFooter_BASE( rParent, rContext ),
  mxModel(std::move( xModel )),
  mxPageStyleProps(std::move( xProps )),
  mbHeader( isHeader ),
  mnIndex( index )
{
}

sal_Bool SAL_CALL SwVbaHeaderFooter::getIsHeader()
{
    return mbHeader;
}

sal_Bool SAL_CALL SwVbaHeaderFooter::getLinkToPrevious()
{
    // seems always false
    return false;
}

void SAL_CALL SwVbaHeaderFooter::setLinkToPrevious( sal_Bool /*_linktoprevious*/ )
{
    // not support in Writer
}

uno::Reference< word::XRange > SAL_CALL SwVbaHeaderFooter::getRange()
{
    OUString sPropsNameText;
    if( mbHeader )
    {
        sPropsNameText = "HeaderText";
    }
    else
    {
        sPropsNameText = "FooterText";
    }
    if( mnIndex == word::WdHeaderFooterIndex::wdHeaderFooterEvenPages )
    {
        sPropsNameText += "Left";
    }

    uno::Reference< text::XText > xText( mxPageStyleProps->getPropertyValue( sPropsNameText ), uno::UNO_QUERY_THROW );
    return uno::Reference< word::XRange >( new SwVbaRange( this, mxContext, mxModel, xText->getStart(), xText->getEnd(), xText ) );
}

uno::Any SAL_CALL
SwVbaHeaderFooter::Shapes( const uno::Any& index )
{
    // #FIXME: only get the shapes in the current header/footer
    //uno::Reference< drawing::XShapes > xShapes( xDrawPageSupplier->getDrawPage(), uno::UNO_QUERY_THROW );
    rtl::Reference< SwFmDrawPage > xIndexAccess( mxModel->getSwDrawPage() );
    uno::Reference< XCollection > xCol( new ScVbaShapes( this, mxContext, xIndexAccess, static_cast<SfxBaseModel*>(mxModel.get()) ) );
    if ( index.hasValue() )
        return xCol->Item( index, uno::Any() );
    return uno::Any( xCol );
}

OUString
SwVbaHeaderFooter::getServiceImplName()
{
    return u"SwVbaHeaderFooter"_ustr;
}

uno::Sequence< OUString >
SwVbaHeaderFooter::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.word.Pane"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
