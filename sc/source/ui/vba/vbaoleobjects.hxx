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

#include <ooo/vba/excel/XOLEObjects.hpp>

#include <vbahelper/vbacollectionimpl.hxx>

typedef CollTestImplHelper<ov::excel::XOLEObjects> OLEObjectsImpl_BASE;

class ScVbaOLEObjects : public OLEObjectsImpl_BASE
{
protected:
    virtual css::uno::Any getItemByStringIndex(const OUString& sIndex) override;
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;

public:
    ScVbaOLEObjects(const css::uno::Reference<ov::XHelperInterface>& xParent,
                    const css::uno::Reference<css::uno::XComponentContext>& xContext,
                    const css::uno::Reference<css::container::XIndexAccess>& xIndexAccess);

    // XEnumerationAccess
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual css::uno::Reference<css::container::XEnumeration> SAL_CALL createEnumeration() override;

    // ScVbaCollectionBaseImpl
    virtual css::uno::Any createCollectionObject(const css::uno::Any& aSource) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
