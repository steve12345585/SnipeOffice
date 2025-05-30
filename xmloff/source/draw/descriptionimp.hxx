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

#pragma once

#include <xmloff/xmlictxt.hxx>
#include <com/sun/star/drawing/XShape.hpp>

// office:events inside a shape

class SdXMLDescriptionContext final : public SvXMLImportContext
{
private:
    css::uno::Reference< css::drawing::XShape > mxShape;
    OUString msText;
    sal_Int32 mnElement;
public:

    SdXMLDescriptionContext( SvXMLImport& rImport, sal_Int32 mnElement,
        const css::uno::Reference< css::drawing::XShape >& rxShape );
    virtual ~SdXMLDescriptionContext() override;

    virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

    // This method is called for all characters that are contained in the
    // current element. The default is to ignore them.
    virtual void SAL_CALL characters( const OUString& rChars ) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
