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


#include "basdoc.hxx"
#include <basidesh.hxx>
#include <iderdll.hxx>
#include <com/sun/star/io/IOException.hpp>
#include <comphelper/sequence.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <sfx2/objsh.hxx>
#include <vcl/svapp.hxx>

#include "unomodel.hxx"


namespace {

// Implements XEnumeration to hold a single selected portion of text
// This will actually only hold a single string value
class SelectionEnumeration : public ::cppu::WeakImplHelper<css::container::XEnumeration>
{
private:
    OUString m_sText;
    bool m_bHasElements;

public:
    explicit SelectionEnumeration(const OUString& sSelectedText)
        : m_sText(sSelectedText)
        , m_bHasElements(true) {}

    virtual sal_Bool SAL_CALL hasMoreElements() override
    {
        return m_bHasElements;
    }

    virtual css::uno::Any SAL_CALL nextElement() override
    {
        if (m_bHasElements)
        {
            m_bHasElements = false;
            return css::uno::Any(m_sText);
        }

        throw css::container::NoSuchElementException();
    }
};

} // End of unnamed namespace

namespace basctl
{

using namespace ::com::sun::star;

SIDEModel::SIDEModel( SfxObjectShell *pObjSh )
    : cppu::ImplInheritanceHelper<SfxBaseModel, css::lang::XServiceInfo>(pObjSh)
{
}

SIDEModel::~SIDEModel()
{
}

OUString SIDEModel::getImplementationName()
{
    return u"com.sun.star.comp.basic.BasicIDE"_ustr;
}

sal_Bool SIDEModel::supportsService(const OUString& rServiceName)
{
    return cppu::supportsService(this, rServiceName);
}

uno::Sequence< OUString > SIDEModel::getSupportedServiceNames()
{
    return { u"com.sun.star.script.BasicIDE"_ustr };
}

//  XStorable
void SAL_CALL SIDEModel::store()
{
    notImplemented();
}

void SAL_CALL SIDEModel::storeAsURL( const OUString&, const uno::Sequence< beans::PropertyValue >& )
{
    notImplemented();
}

void SAL_CALL SIDEModel::storeToURL( const OUString&,
        const uno::Sequence< beans::PropertyValue >& )
{
    notImplemented();
}

void  SIDEModel::notImplemented()
{
    throw io::IOException(u"Can't store IDE model"_ustr );
}

// XModel
css::uno::Reference< css::uno::XInterface > SAL_CALL SIDEModel::getCurrentSelection()
{
    SolarMutexGuard aGuard;
    Shell* pShell = GetShell();
    if (!pShell)
        return nullptr;
    OUString sText = GetShell()->GetSelectionText(false);
    return uno::Reference<container::XEnumeration>(new SelectionEnumeration(sText));
}

} // namespace basctl

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_basic_BasicID_get_implementation(
    css::uno::XComponentContext* , css::uno::Sequence<css::uno::Any> const&)
{
    SolarMutexGuard aGuard;
    basctl::EnsureIde();
    rtl::Reference<SfxObjectShell> pShell = new basctl::DocShell();
    auto pModel = pShell->GetModel();
    pModel->acquire();
    return pModel.get();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
