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

/**
 * AccComponent.cpp : Implementation of CUAccCOMApp and DLL registration.
 */
#include "stdafx.h"
#include <UAccCOM.h>
#include "AccComponent.h"

#include <com/sun/star/accessibility/XAccessible.hpp>
#include <com/sun/star/accessibility/XAccessibleContext.hpp>
#include <vcl/svapp.hxx>

using namespace com::sun::star::accessibility;
using namespace com::sun::star::uno;

CAccComponent::CAccComponent() {}

CAccComponent::~CAccComponent() {}

/**
 * Returns the location of the upper left corner of the object's bounding
 * box relative to the parent.
 *
 * @param    Location    the upper left corner of the object's bounding box.
 */
COM_DECLSPEC_NOTHROW STDMETHODIMP CAccComponent::get_locationInParent(long* x, long* y)
{
    SolarMutexGuard g;

    try
    {
        if (x == nullptr || y == nullptr)
            return E_INVALIDARG;

        if (!m_xComponent.is())
            return E_FAIL;

        const css::awt::Point& pt = m_xComponent->getLocation();
        *x = pt.X;
        *y = pt.Y;
        return S_OK;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

/**
 * Returns the foreground color of this object.
 *
 * @param    Color    the color of foreground.
 */
COM_DECLSPEC_NOTHROW STDMETHODIMP CAccComponent::get_foreground(IA2Color* foreground)
{
    SolarMutexGuard g;

    try
    {
        if (foreground == nullptr)
            return E_INVALIDARG;

        if (!m_xComponent.is())
        {
            return E_FAIL;
        }
        *foreground = static_cast<long>(m_xComponent->getForeground());

        return S_OK;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

/**
 * Returns the background color of this object.
 *
 * @param    Color    the color of background.
 */
COM_DECLSPEC_NOTHROW STDMETHODIMP CAccComponent::get_background(IA2Color* background)
{
    SolarMutexGuard g;

    try
    {
        if (background == nullptr)
            return E_INVALIDARG;

        if (!m_xComponent.is())
        {
            return E_FAIL;
        }
        *background = static_cast<long>(m_xComponent->getBackground());

        return S_OK;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

/**
 * Override of IUNOXWrapper.
 *
 * @param    pXInterface    the pointer of UNO interface.
 */
COM_DECLSPEC_NOTHROW STDMETHODIMP CAccComponent::put_XInterface(hyper pXInterface)
{
    // internal IUNOXWrapper - no mutex meeded

    try
    {
        CUNOXWrapper::put_XInterface(pXInterface);

        if (pUNOInterface == nullptr)
            return E_FAIL;

        Reference<XAccessibleContext> pRContext = pUNOInterface->getAccessibleContext();
        if (!pRContext.is())
        {
            return E_FAIL;
        }
        Reference<XAccessibleComponent> pRXI(pRContext, UNO_QUERY);
        m_xComponent = pRXI;

        return S_OK;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
