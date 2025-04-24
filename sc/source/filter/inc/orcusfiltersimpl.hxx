/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <orcusfilters.hxx>

#include <orcus/xml_namespace.hpp>

class ScOrcusFiltersImpl : public ScOrcusFilters
{
public:
    virtual ImportResult importByName(ScDocument& rDoc, SfxMedium& rMedium,
                                      const OUString& rFilterName) const override;

    virtual bool importODS_Styles(ScDocument& rDoc, OUString& aFileName) const override;

    virtual std::unique_ptr<ScOrcusXMLContext>
    createXMLContext(ScDocument& rDoc, const OUString& rPath) const override;
};

class ScOrcusXMLContextImpl : public ScOrcusXMLContext
{
    ScDocument& mrDoc;
    OUString maPath;

    orcus::xmlns_repository maNsRepo; /// XML namespace repository for this context.

public:
    ScOrcusXMLContextImpl(ScDocument& rDoc, OUString aPath);
    virtual ~ScOrcusXMLContextImpl() override;

    virtual void loadXMLStructure(weld::TreeView& rTreeCtrl, ScOrcusXMLTreeParam& rParam) override;

    virtual void importXML(const ScOrcusImportXMLParam& rParam) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
