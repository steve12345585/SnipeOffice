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

#include <sal/config.h>

#include <string_view>
#include <vector>

#include <codemaker/unotype.hxx>
#include <rtl/ref.hxx>
#include <rtl/string.hxx>
#include <rtl/textenc.h>
#include <rtl/ustring.hxx>
#include <salhelper/simplereferenceobject.hxx>

namespace unoidl {
    class Entity;
    class Manager;
    class MapCursor;
    class Provider;
}

class TypeManager final : public salhelper::SimpleReferenceObject {
public:
    TypeManager();

    rtl::Reference<unoidl::Manager> const & getManager() const { return manager_; }

    void loadProvider(OUString const & uri, bool primary);

    std::vector<rtl::Reference<unoidl::Provider>> const & getPrimaryProviders() const
    { return primaryProviders_; }

    bool foundAtPrimaryProvider(OUString const & name) const;

    codemaker::UnoType::Sort getSort(
        OUString const & name, rtl::Reference< unoidl::Entity > * entity = nullptr,
        rtl::Reference< unoidl::MapCursor > * cursor = nullptr) const;

    codemaker::UnoType::Sort decompose(
        std::u16string_view name, bool resolveTypedefs, OUString * nucleus,
        sal_Int32 * rank, std::vector< OUString > * arguments,
        rtl::Reference< unoidl::Entity > * entity) const;

private:
    virtual ~TypeManager() override;

    rtl::Reference< unoidl::Manager > manager_;
    std::vector< rtl::Reference< unoidl::Provider > > primaryProviders_;
};


inline OString u2b(std::u16string_view s) {
    return OUStringToOString(s, RTL_TEXTENCODING_UTF8);
}

inline OUString b2u(std::string_view s) {
    return OStringToOUString(s, RTL_TEXTENCODING_UTF8);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
