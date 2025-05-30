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

#include "stdafx.h"
#include <UAccCOM.h>
#include "AccImage.h"

#include <vcl/svapp.hxx>
#include <o3tl/char16_t2wchar_t.hxx>
#include <systools/win32/oleauto.hxx>

#include <com/sun/star/accessibility/XAccessible.hpp>
#include <com/sun/star/accessibility/XAccessibleContext.hpp>

using namespace css::accessibility;
using namespace css::uno;

/**
   * Get description.
   * @param description Variant to get description.
   * @return Result.
*/
COM_DECLSPEC_NOTHROW STDMETHODIMP CAccImage::get_description(BSTR* description)
{
    SolarMutexGuard g;

    try
    {
        if (description == nullptr)
            return E_INVALIDARG;
        if (!m_xImage.is())
            return E_FAIL;

        OUString ouStr = m_xImage->getAccessibleImageDescription();
        SysFreeString(*description);
        *description = sal::systools::BStr::newBSTR(ouStr);

        return S_OK;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

COM_DECLSPEC_NOTHROW STDMETHODIMP CAccImage::get_imagePosition(
    /* [in] */ enum IA2CoordinateType,
    /* [out] */ long __RPC_FAR*,
    /* [retval][out] */ long __RPC_FAR*)
{
    return E_NOTIMPL;
}

COM_DECLSPEC_NOTHROW STDMETHODIMP CAccImage::get_imageSize(
    /* [out] */ long __RPC_FAR*,
    /* [retval][out] */ long __RPC_FAR*)
{
    return E_NOTIMPL;
}

/**
   * Put UNO interface.
   * @param pXInterface UNO interface.
   * @return Result.
*/
COM_DECLSPEC_NOTHROW STDMETHODIMP CAccImage::put_XInterface(hyper pXInterface)
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
        Reference<XAccessibleImage> xImage(pRContext, UNO_QUERY);
        m_xImage = xImage;
        return S_OK;
    }
    catch (...)
    {
        return E_FAIL;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
