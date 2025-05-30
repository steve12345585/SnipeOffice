/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <com/sun/star/uno/XComponentContext.hpp>

#include <ImportFilter.hxx>

#include <DocumentHandlerForOdt.hxx>

/* This component will be instantiated for both import or export. Whether it calls
 * setSourceDocument or setTargetDocument determines which Impl function the filter
 * member calls */
class AbiWordImportFilter : public writerperfect::ImportFilter<OdtGenerator>
{
public:
    explicit AbiWordImportFilter(const css::uno::Reference<css::uno::XComponentContext>& rxContext)
        : writerperfect::ImportFilter<OdtGenerator>(rxContext)
    {
    }

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getSupportedServiceNames() override;

private:
    virtual bool doDetectFormat(librevenge::RVNGInputStream& rInput, OUString& rTypeName) override;
    virtual bool doImportDocument(weld::Window* pParent, librevenge::RVNGInputStream& rInput,
                                  OdtGenerator& rGenerator, utl::MediaDescriptor&) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
