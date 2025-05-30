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
#include "vbacolumn.hxx"
#include <utility>
#include <vbahelper/vbahelper.hxx>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <com/sun/star/view/XSelectionSupplier.hpp>
#include "vbatablehelper.hxx"
#include "wordvbahelper.hxx"
#include <unotxdoc.hxx>

using namespace ::ooo::vba;
using namespace ::ooo::vba::word;
using namespace ::com::sun::star;

SwVbaColumn::SwVbaColumn( const uno::Reference< ooo::vba::XHelperInterface >& rParent, const uno::Reference< uno::XComponentContext >& rContext, uno::Reference< text::XTextTable >  xTextTable, sal_Int32 nIndex ) :
    SwVbaColumn_BASE( rParent, rContext ), mxTextTable(std::move( xTextTable )), mnIndex( nIndex )
{
}

SwVbaColumn::~SwVbaColumn()
{
}

sal_Int32 SAL_CALL
SwVbaColumn::getWidth( )
{
    SwVbaTableHelper aTableHelper( mxTextTable );
    return aTableHelper.GetColWidth( mnIndex );
}

void SAL_CALL
SwVbaColumn::setWidth( sal_Int32 _width )
{

    SwVbaTableHelper aTableHelper( mxTextTable );
    aTableHelper.SetColWidth( _width, mnIndex );
}

void SAL_CALL
SwVbaColumn::Select( )
{
    SelectColumn( getCurrentWordDoc(mxContext), mxTextTable, mnIndex, mnIndex );
}

void SwVbaColumn::SelectColumn( const rtl::Reference< SwXTextDocument >& xModel, const uno::Reference< text::XTextTable >& xTextTable, sal_Int32 nStartColumn, sal_Int32 nEndColumn )
{
    OUString sStartCol = SwVbaTableHelper::getColumnStr( nStartColumn );
    OUString aRangeName = sStartCol + OUString::number( 1 );
    OUString sEndCol = SwVbaTableHelper::getColumnStr( nEndColumn );
    sal_Int32 nRowCount = xTextTable->getRows()->getCount();
    aRangeName += ":" + sEndCol + OUString::number(nRowCount);

    uno::Reference< table::XCellRange > xCellRange( xTextTable, uno::UNO_QUERY_THROW );
    uno::Reference< table::XCellRange > xSelRange = xCellRange->getCellRangeByName( aRangeName );

    uno::Reference< view::XSelectionSupplier > xSelection( xModel->getCurrentController(), uno::UNO_QUERY_THROW );
    xSelection->select( uno::Any( xSelRange ) );
}

OUString
SwVbaColumn::getServiceImplName()
{
    return u"SwVbaColumn"_ustr;
}

uno::Sequence< OUString >
SwVbaColumn::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.word.Column"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
