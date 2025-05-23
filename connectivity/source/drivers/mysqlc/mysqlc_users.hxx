/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <connectivity/sdbcx/VCollection.hxx>
#include <com/sun/star/sdbc/XDatabaseMetaData.hpp>

namespace connectivity::mysqlc
{
class Users : public ::connectivity::sdbcx::OCollection
{
    css::uno::Reference<css::sdbc::XDatabaseMetaData> m_xMetaData;

protected:
    // OCollection
    virtual void impl_refresh() override;
    virtual ::css::uno::Reference<css::beans::XPropertySet>
    createObject(const OUString& rName) override;
    virtual css::uno::Reference<css::beans::XPropertySet> createDescriptor() override;
    virtual ::css::uno::Reference<css::beans::XPropertySet>
    appendObject(const OUString& rName,
                 const css::uno::Reference<css::beans::XPropertySet>& rDescriptor) override;

public:
    Users(const css::uno::Reference<css::sdbc::XDatabaseMetaData>& rMetaData,
          ::cppu::OWeakObject& rParent, ::osl::Mutex& rMutex,
          ::std::vector<OUString> const& rNames);

    // TODO: we should also implement XDataDescriptorFactory, XRefreshable,
    // XAppend,  etc., but all are optional.

    // XDrop
    virtual void dropObject(sal_Int32 nPosition, const OUString& rName) override;
};
} // namespace connectivity::mysqlc

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
