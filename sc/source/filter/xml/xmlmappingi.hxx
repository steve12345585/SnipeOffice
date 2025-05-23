/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "importcontext.hxx"

namespace sax_fastparser { class FastAttributeList; }

class ScXMLMappingsContext : public ScXMLImportContext
{
public:

    ScXMLMappingsContext( ScXMLImport& rImport );

    virtual ~ScXMLMappingsContext() override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
                        sal_Int32 nElement,
                        const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;
};

class ScXMLMappingContext : public ScXMLImportContext
{

public:

    ScXMLMappingContext( ScXMLImport& rImport,
                        const rtl::Reference<sax_fastparser::FastAttributeList>& rAttrList );

    virtual ~ScXMLMappingContext() override;

    virtual css::uno::Reference<css::xml::sax::XFastContextHandler> SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference<css::xml::sax::XFastAttributeList>& xAttrList) override;
};


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
