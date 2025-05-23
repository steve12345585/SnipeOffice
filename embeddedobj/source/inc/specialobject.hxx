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

#include <com/sun/star/awt/Size.hpp>

#include "commonembobj.hxx"

class OSpecialEmbeddedObject : public OCommonEmbeddedObject
{
private:
    css::awt::Size         maSize;
public:
    OSpecialEmbeddedObject(
        const css::uno::Reference< css::uno::XComponentContext >& rxContext,
        const css::uno::Sequence< css::beans::NamedValue >& aObjectProps );

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type& rType ) override ;

    // XVisualObject
    virtual css::embed::VisualRepresentation SAL_CALL getPreferredVisualRepresentation( ::sal_Int64 nAspect ) override;

    virtual void SAL_CALL setVisualAreaSize( sal_Int64 nAspect, const css::awt::Size& aSize ) override;

    virtual css::awt::Size SAL_CALL getVisualAreaSize( sal_Int64 nAspect ) override;

    virtual sal_Int32 SAL_CALL getMapUnit( sal_Int64 nAspect ) override;

    virtual void SAL_CALL changeState( sal_Int32 nNewState ) override;

    virtual void SAL_CALL doVerb( sal_Int32 nVerbID ) override;

// XCommonEmbedPersist

    virtual void SAL_CALL reload(
                const css::uno::Sequence< css::beans::PropertyValue >& lArguments,
                const css::uno::Sequence< css::beans::PropertyValue >& lObjArgs ) override;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() override;
    sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
