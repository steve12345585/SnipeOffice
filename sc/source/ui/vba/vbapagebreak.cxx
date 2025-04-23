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
#include "vbapagebreak.hxx"
#include "vbarange.hxx"
#include <basic/sberrors.hxx>
#include <ooo/vba/excel/XlPageBreak.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/table/XCellRange.hpp>
#include <utility>

using namespace ::com::sun::star;
using namespace ::ooo::vba;

template< typename... Ifc >
ScVbaPageBreak< Ifc... >::ScVbaPageBreak( const uno::Reference< XHelperInterface >& xParent,
                    const uno::Reference< uno::XComponentContext >& xContext,
                    uno::Reference< beans::XPropertySet > xProps,
                    sheet::TablePageBreakData aTablePageBreakData):
            ScVbaPageBreak_BASE( xParent, xContext ),
            mxRowColPropertySet(std::move( xProps )),
            maTablePageBreakData( aTablePageBreakData )
{
}

template< typename... Ifc >
sal_Int32 ScVbaPageBreak< Ifc... >::getType()
{
    uno::Any aValue = mxRowColPropertySet->getPropertyValue(u"IsStartOfNewPage"_ustr);
    bool hasPageBreak = false;
    aValue >>= hasPageBreak;

    if( !hasPageBreak )
        return excel::XlPageBreak::xlPageBreakNone;

    if( maTablePageBreakData.ManualBreak )
        return excel::XlPageBreak::xlPageBreakManual;

    return excel::XlPageBreak::xlPageBreakAutomatic;
}

template< typename... Ifc >
void ScVbaPageBreak< Ifc... >::setType(sal_Int32 type)
{
    if( (type != excel::XlPageBreak::xlPageBreakNone) &&
        (type != excel::XlPageBreak::xlPageBreakManual) &&
        (type != excel::XlPageBreak::xlPageBreakAutomatic) )
    {
        DebugHelper::runtimeexception(ERRCODE_BASIC_BAD_PARAMETER);
    }

    if( type == excel::XlPageBreak::xlPageBreakNone )
    {
        mxRowColPropertySet->setPropertyValue(u"IsStartOfNewPage"_ustr, uno::Any(false));
        return;
    }

    mxRowColPropertySet->setPropertyValue(u"IsStartOfNewPage"_ustr, uno::Any(true));
    if( type == excel::XlPageBreak::xlPageBreakManual )
        maTablePageBreakData.ManualBreak = true;
    else
        maTablePageBreakData.ManualBreak = false;
}

template< typename... Ifc >
void ScVbaPageBreak< Ifc... >::Delete()
{
    mxRowColPropertySet->setPropertyValue(u"IsStartOfNewPage"_ustr, uno::Any(false));
}

template< typename... Ifc >
uno::Reference< excel::XRange> ScVbaPageBreak< Ifc... >::Location()
{
    uno::Reference< table::XCellRange > xRange( mxRowColPropertySet, uno::UNO_QUERY_THROW );
    return new ScVbaRange( ScVbaPageBreak_BASE::getParent(), ScVbaPageBreak_BASE::mxContext, xRange);
}

template class ScVbaPageBreak< excel::XHPageBreak >;

/* class ScVbaHPageBreak */
OUString
ScVbaHPageBreak::getServiceImplName()
{
    return u"ScVbaHPageBreak"_ustr;
}

uno::Sequence< OUString >
ScVbaHPageBreak::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.excel.HPageBreak"_ustr
    };
    return aServiceNames;
}

template class ScVbaPageBreak< excel::XVPageBreak >;

/* class ScVbaVPageBreak */
ScVbaVPageBreak::ScVbaVPageBreak( const css::uno::Reference< ov::XHelperInterface >& xParent,
                                  const css::uno::Reference< css::uno::XComponentContext >& xContext,
                                  const css::uno::Reference< css::beans::XPropertySet >& xProps,
                                  css::sheet::TablePageBreakData aTablePageBreakData )
:   ScVbaVPageBreak_BASE( xParent, xContext, xProps, aTablePageBreakData )
{
}

ScVbaVPageBreak::~ScVbaVPageBreak()
{
}

OUString
ScVbaVPageBreak::getServiceImplName()
{
    return u"ScVbaVPageBreak"_ustr;
}

uno::Sequence< OUString >
ScVbaVPageBreak::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.excel.VPageBreak"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
