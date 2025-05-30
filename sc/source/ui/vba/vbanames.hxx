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

#include <ooo/vba/excel/XNames.hpp>
#include <vbahelper/vbacollectionimpl.hxx>

namespace com::sun::star::sheet { class XNamedRanges; }

class ScDocument;

typedef CollTestImplHelper< ov::excel::XNames > ScVbaNames_BASE;

class ScVbaNames final : public ScVbaNames_BASE
{
    css::uno::Reference< css::frame::XModel > mxModel;
    css::uno::Reference< css::sheet::XNamedRanges > mxNames;

    const css::uno::Reference< css::frame::XModel >&  getModel() const { return mxModel; }

public:
    ScVbaNames( const css::uno::Reference< ov::XHelperInterface >& xParent,  const css::uno::Reference< css::uno::XComponentContext >& xContext, const css::uno::Reference< css::sheet::XNamedRanges >& xNames , css::uno::Reference< css::frame::XModel > xModel );

    ScDocument& getScDocument();

    virtual ~ScVbaNames() override;

    // XEnumerationAccess
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL createEnumeration() override;

    // Methods
    virtual css::uno::Any SAL_CALL Add( const css::uno::Any& aName ,
                    const css::uno::Any& aRefersTo,
                    const css::uno::Any& aVisible,
                    const css::uno::Any& aMacroType,
                    const css::uno::Any& aShoutcutKey,
                    const css::uno::Any& aCategory,
                    const css::uno::Any& aNameLocal,
                    const css::uno::Any& aRefersToLocal,
                    const css::uno::Any& aCategoryLocal,
                    const css::uno::Any& aRefersToR1C1,
                    const css::uno::Any& aRefersToR1C1Local ) override;

    virtual css::uno::Any createCollectionObject( const css::uno::Any& aSource ) override;

    // ScVbaNames_BASE
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;

};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
