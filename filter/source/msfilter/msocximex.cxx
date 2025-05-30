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

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/XDrawPage.hpp>
#include <com/sun/star/drawing/XShapes.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>
#include <com/sun/star/form/XFormsSupplier.hpp>
#include <com/sun/star/form/XForm.hpp>
#include <osl/diagnose.h>
#include <filter/msfilter/msocximex.hxx>
#include <utility>

using namespace ::com::sun::star;

constexpr OUString sWW8_form( u"WW-Standard"_ustr );

SvxMSConvertOCXControls::SvxMSConvertOCXControls( uno::Reference< frame::XModel > xModel) : mxModel(std::move(xModel))
{
}

SvxMSConvertOCXControls::~SvxMSConvertOCXControls()
{
}

void SvxMSConvertOCXControls::GetDrawPage()
{
    if( !xDrawPage.is() && mxModel.is() )
    {
        uno::Reference< drawing::XDrawPageSupplier > xTxtDoc(mxModel,
            uno::UNO_QUERY);
        OSL_ENSURE(xTxtDoc.is(),"no XDrawPageSupplier from XModel");
        xDrawPage = xTxtDoc->getDrawPage();
        OSL_ENSURE( xDrawPage.is(), "no XDrawPage" );
    }
}


const uno::Reference< lang::XMultiServiceFactory >&
    SvxMSConvertOCXControls::GetServiceFactory()
{
    if( !xServiceFactory.is() && mxModel.is() )
    {
        xServiceFactory = uno::Reference< lang::XMultiServiceFactory >
            (mxModel, uno::UNO_QUERY);
        OSL_ENSURE( xServiceFactory.is(),
                "no XMultiServiceFactory from doc Model" );
    }

    return xServiceFactory;
}

const uno::Reference< drawing::XShapes >& SvxMSConvertOCXControls::GetShapes()
{
    if( !xShapes.is() )
    {
        GetDrawPage();
        if( xDrawPage.is() )
        {
            xShapes = xDrawPage;
        }
    }
    return xShapes;
}

const uno::Reference< container::XIndexContainer >&
    SvxMSConvertOCXControls::GetFormComps()
{
    if( !xFormComps.is() )
    {
        GetDrawPage();
        if( xDrawPage.is() )
        {
            uno::Reference< form::XFormsSupplier > xFormsSupplier( xDrawPage,
                uno::UNO_QUERY );
            OSL_ENSURE( xFormsSupplier.is(),
                    "UNO_QUERY failed for XFormsSupplier from XDrawPage" );

            uno::Reference< container::XNameContainer >  xNameCont =
                xFormsSupplier->getForms();

            // The form gets a new name like "WW-Standard[n]" and will
            // created new in any case.
            OUString sName( sWW8_form );
            sal_uInt16 n = 0;

            while( xNameCont->hasByName( sName ) )
            {
                sName = sWW8_form + OUString::number( ++n );
            }

            const uno::Reference< lang::XMultiServiceFactory > &rServiceFactory
                = GetServiceFactory();
            if( !rServiceFactory.is() )
                return xFormComps;

            uno::Reference< uno::XInterface >  xCreate =
                rServiceFactory->createInstance(
                    u"com.sun.star.form.component.Form"_ustr );
            if( xCreate.is() )
            {
                uno::Reference< beans::XPropertySet > xFormPropSet( xCreate,
                    uno::UNO_QUERY );

                uno::Any aTmp(&sName,cppu::UnoType<OUString>::get());
                xFormPropSet->setPropertyValue( u"Name"_ustr, aTmp );

                uno::Reference< form::XForm > xForm( xCreate, uno::UNO_QUERY );
                OSL_ENSURE(xForm.is(), "no Form?");

                uno::Reference< container::XIndexContainer > xForms( xNameCont,
                    uno::UNO_QUERY );
                OSL_ENSURE( xForms.is(), "XForms not available" );

                aTmp <<= xForm;
                xForms->insertByIndex( xForms->getCount(), aTmp );

                xFormComps = uno::Reference< container::XIndexContainer >
                    (xCreate, uno::UNO_QUERY);
            }
        }
    }

    return xFormComps;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
