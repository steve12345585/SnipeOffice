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
#include "vbachart.hxx"
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/document/XEmbeddedObjectSupplier.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/script/BasicErrorException.hpp>
#include <com/sun/star/view/XSelectionSupplier.hpp>
#include <basic/sberrors.hxx>
#include <utility>
#include "vbachartobject.hxx"
#include "vbachartobjects.hxx"

using namespace ::com::sun::star;
using namespace ::ooo::vba;

constexpr OUStringLiteral PERSIST_NAME(u"PersistName");

ScVbaChartObject::ScVbaChartObject( const css::uno::Reference< ov::XHelperInterface >& _xParent, const css::uno::Reference< css::uno::XComponentContext >& _xContext, css::uno::Reference< css::table::XTableChart >  _xTableChart, css::uno::Reference< css::drawing::XDrawPageSupplier >  _xDrawPageSupplier ) : ChartObjectImpl_BASE( _xParent, _xContext ), xTableChart(std::move( _xTableChart )), xDrawPageSupplier(std::move( _xDrawPageSupplier ))
{
        xDrawPage = xDrawPageSupplier->getDrawPage();
        xEmbeddedObjectSupplier.set( xTableChart, uno::UNO_QUERY_THROW );
        xNamed.set( xTableChart, uno::UNO_QUERY_THROW );
        sPersistName = getPersistName();
        xShape = setShape();
        setName(sPersistName);
        oShapeHelper.emplace(xShape);
}

OUString const & ScVbaChartObject::getPersistName()
{
    if ( sPersistName.isEmpty() )
        sPersistName = xNamed->getName();
    return sPersistName;
}

uno::Reference< drawing::XShape >
ScVbaChartObject::setShape()
{
    try
    {
        sal_Int32 nItems = xDrawPage->getCount();
        for (int i = 0; i < nItems; i++)
        {
            xShape.set( xDrawPage->getByIndex(i), uno::UNO_QUERY_THROW );
            if (xShape->getShapeType() == "com.sun.star.drawing.OLE2Shape")
            {
                uno::Reference< beans::XPropertySet > xShapePropertySet(xShape, uno::UNO_QUERY_THROW );
                OUString sName;
                xShapePropertySet->getPropertyValue(PERSIST_NAME ) >>=sName;
                if ( sName == sPersistName )
                {
                    xNamedShape.set( xShape, uno::UNO_QUERY_THROW );
                    return xShape;
                }
            }
        }
    }
    catch (uno::Exception& )
    {
        throw script::BasicErrorException( OUString(), uno::Reference< uno::XInterface >(), sal_uInt32(ERRCODE_BASIC_METHOD_FAILED), OUString() );
    }
    return nullptr;
}

void SAL_CALL
ScVbaChartObject::setName( const OUString& sName )
{
    xNamedShape->setName(sName);
}

OUString SAL_CALL
ScVbaChartObject::getName()
{
    return xNamedShape->getName();
}

void SAL_CALL
ScVbaChartObject::Delete()
{
    // parent of this object is sheet
    uno::Reference< excel::XWorksheet > xParent( getParent(), uno::UNO_QUERY_THROW );
    uno::Reference< excel::XChartObjects > xColl( xParent->ChartObjects( uno::Any() ), uno::UNO_QUERY_THROW );
    ScVbaChartObjects* pChartObjectsImpl = static_cast< ScVbaChartObjects* >( xColl.get() );
    if (!pChartObjectsImpl)
        throw script::BasicErrorException( OUString(), uno::Reference< uno::XInterface >(), sal_uInt32(ERRCODE_BASIC_METHOD_FAILED), u"Parent is not ChartObjects"_ustr );

    pChartObjectsImpl->removeByName( getPersistName() );

}

void
ScVbaChartObject::Activate()
{
    try
    {
        // #TODO #FIXME should be ThisWorkbook or equivalent, or in
        // fact probably the chart object should be created with
        // the XModel owner
        //uno::Reference< view::XSelectionSupplier > xSelectionSupplier( getXModel().getCurrentController());
        uno::Reference< view::XSelectionSupplier > xSelectionSupplier( getCurrentExcelDoc(mxContext)->getCurrentController(), uno::UNO_QUERY_THROW );
        xSelectionSupplier->select(uno::Any(xShape));
    }
    catch (uno::Exception& )
    {
        throw script::BasicErrorException( OUString(), uno::Reference< uno::XInterface >(), sal_uInt32(ERRCODE_BASIC_METHOD_FAILED), u"ChartObject Activate internal error"_ustr );
    }
}

uno::Reference< excel::XChart > SAL_CALL
ScVbaChartObject::getChart()
{
    return new ScVbaChart( this, mxContext, xEmbeddedObjectSupplier->getEmbeddedObject(), xTableChart );
}

OUString
ScVbaChartObject::getServiceImplName()
{
    return u"ScVbaChartObject"_ustr;
}

uno::Sequence< OUString >
ScVbaChartObject::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.excel.ChartObject"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
