/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sdbcx/VUser.hxx>
#include <com/sun/star/sdbc/XConnection.hpp>

namespace connectivity::mysqlc
{
/**
* This implements com.sun.star.sdbcx.Container.
*/
class User : public ::connectivity::sdbcx::OUser
{
    css::uno::Reference<css::sdbc::XConnection> m_xConnection;

public:
    /**
    * Create a "new" descriptor, which isn't yet in the database.
    */
    User(css::uno::Reference<css::sdbc::XConnection> xConnection);
    /**
    * For a user that already exists in the db.
    */
    User(css::uno::Reference<css::sdbc::XConnection> xConnection, const OUString& rName);

    // XAuthorizable
    virtual void SAL_CALL changePassword(const OUString&, const OUString& newPassword) override;
    virtual sal_Int32 SAL_CALL getPrivileges(const OUString&, sal_Int32) override;
    // return the privileges and additional the grant rights
    /// @throws css::sdbc::SQLException
    /// @throws css::uno::RuntimeException
    void findPrivilegesAndGrantPrivileges(const OUString& objName, sal_Int32 objType,
                                          sal_Int32& nRights, sal_Int32& nRightsWithGrant);

    virtual sal_Int32 SAL_CALL getGrantablePrivileges(const OUString&, sal_Int32) override;

    // IRefreshableGroups::
    virtual void refreshGroups() override;
};

class OUserExtend;
typedef ::comphelper::OPropertyArrayUsageHelper<OUserExtend> OUserExtend_PROP;

class OUserExtend : public User, public OUserExtend_PROP
{
    OUString m_Password;

protected:
    // OPropertyArrayUsageHelper
    virtual ::cppu::IPropertyArrayHelper* createArrayHelper() const override;
    // OPropertySetHelper
    virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper() override;

public:
    OUserExtend(const css::uno::Reference<css::sdbc::XConnection>& _xConnection,
                const OUString& rName);

    virtual void construct() override;
};

} // namespace connectivity::mysqlc

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
