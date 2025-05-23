/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <map>

#include <rtl/ref.hxx>
#include <unoidl/unoidl.hxx>

namespace unoidl::detail
{
class SourceTreeProvider : public Provider
{
public:
    // throws FileFormatException, NoSuchFileException:
    SourceTreeProvider(Manager& manager, OUString const& uri);

    // throws FileFormatException:
    virtual rtl::Reference<MapCursor> createRootCursor() const override;

    // throws FileFormatException:
    virtual rtl::Reference<Entity> findEntity(OUString const& name) const override;

private:
    virtual ~SourceTreeProvider() noexcept override;

    Manager& manager_;
    OUString uri_;
    mutable std::map<OUString, rtl::Reference<Entity>> cache_; //TODO: at manager
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
