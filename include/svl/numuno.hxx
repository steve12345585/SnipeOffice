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
#ifndef INCLUDED_SVL_NUMUNO_HXX
#define INCLUDED_SVL_NUMUNO_HXX

#include <svl/svldllapi.h>
#include <com/sun/star/util/XNumberFormatsSupplier.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <comphelper/servicehelper.hxx>
#include <cppuhelper/implbase2.hxx>
#include <memory>

class SvNumberFormatter;
class SvNumFmtSuppl_Impl;

namespace comphelper
{
    class SharedMutex;
}


/**
 * Factory for XNumberFormats objects.
 * Implements XAggregation because it is aggregated to ScModelObj
 */
class SVL_DLLPUBLIC SvNumberFormatsSupplierObj : public cppu::WeakAggImplHelper2<
                                    css::util::XNumberFormatsSupplier,
                                    css::lang::XUnoTunnel>
{
private:
    std::unique_ptr<SvNumFmtSuppl_Impl> pImpl;

public:
                                SvNumberFormatsSupplierObj();
                                SvNumberFormatsSupplierObj(SvNumberFormatter* pForm);
    virtual                     ~SvNumberFormatsSupplierObj() override;

    void                        SetNumberFormatter(SvNumberFormatter* pNew);
    SvNumberFormatter*          GetNumberFormatter() const;

                                // XNumberFormatsSupplier
    virtual css::uno::Reference< css::beans::XPropertySet > SAL_CALL
                                getNumberFormatSettings() override;
    virtual css::uno::Reference< css::util::XNumberFormats > SAL_CALL
                                getNumberFormats() override;

                                // XUnoTunnel
    UNO3_GETIMPLEMENTATION_DECL(SvNumberFormatsSupplierObj)

    ::comphelper::SharedMutex&  getSharedMutex() const;
};

#endif // INCLUDED_SVL_NUMUNO_HXX


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
