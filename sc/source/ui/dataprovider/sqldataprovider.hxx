/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <dataprovider.hxx>

namespace sc
{
class SQLFetchThread;

class SQLDataProvider : public DataProvider
{
private:
    ScDocument* mpDocument;
    rtl::Reference<SQLFetchThread> mxSQLFetchThread;

    ScDocumentUniquePtr mpDoc;

public:
    SQLDataProvider(ScDocument* pDoc, sc::ExternalDataSource& rDataSource);
    virtual ~SQLDataProvider() override;

    virtual void Import() override;

    virtual const OUString& GetURL() const override;

    void ImportFinished();
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
