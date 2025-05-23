/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_TEST_CONTAINER_XNAMED_HXX
#define INCLUDED_TEST_CONTAINER_XNAMED_HXX

#include <rtl/ustring.hxx>

#include <com/sun/star/uno/XInterface.hpp>

#include <com/sun/star/uno/Reference.hxx>

#include <test/testdllapi.hxx>
#include <utility>

namespace apitest
{
class OOO_DLLPUBLIC_TEST XNamed
{
public:
    XNamed(OUString aTestName)
        : m_aTestName(std::move(aTestName))
    {
    }

    void testGetName();
    void testSetName();
    // special case, here name is equal to links URL
    void testSetNameByScSheetLinkObj();
    void testSetNameThrowsException();

    virtual css::uno::Reference<css::uno::XInterface> init() = 0;

protected:
    ~XNamed() {}

private:
    OUString m_aTestName;
};
}

#endif // INCLUDED_TEST_CONTAINER_XNAMED_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
