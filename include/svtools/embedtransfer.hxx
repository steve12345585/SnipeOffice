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

#include <svtools/svtdllapi.h>
#include <vcl/transfer.hxx>
#include <vcl/graph.hxx>
#include <optional>

namespace com :: sun :: star :: embed { class XEmbeddedObject; }

class SVT_DLLPUBLIC SvEmbedTransferHelper final : public TransferableHelper
{
private:

    css::uno::Reference< css::embed::XEmbeddedObject > m_xObj;
    std::optional<Graphic> m_oGraphic;
    sal_Int64 m_nAspect;

    OUString maParentShellID;

    virtual void        AddSupportedFormats() override;
    virtual bool        GetData( const css::datatransfer::DataFlavor& rFlavor, const OUString& rDestDoc ) override;
    virtual void        ObjectReleased() override;

public:
    // object, replacement image, and the aspect
    SvEmbedTransferHelper( const css::uno::Reference< css::embed::XEmbeddedObject >& xObj,
                           const Graphic* pGraphic,
                            sal_Int64 nAspect );
    virtual ~SvEmbedTransferHelper() override;

    void SetParentShellID( const OUString& rShellID );

    static void         FillTransferableObjectDescriptor( TransferableObjectDescriptor& rDesc,
                            const css::uno::Reference< css::embed::XEmbeddedObject >& xObj,
                            const Graphic* pGraphic,
                            sal_Int64 nAspect );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
