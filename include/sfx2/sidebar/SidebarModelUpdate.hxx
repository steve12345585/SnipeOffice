/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <config_options.h>
#include <sfx2/dllapi.h>

namespace com::sun::star::frame
{
class XModel;
}
namespace com::sun::star::uno
{
template <typename> class Reference;
}

namespace sfx2::sidebar
{
class UNLESS_MERGELIBS(SFX2_DLLPUBLIC) SAL_LOPLUGIN_ANNOTATE("crosscast") SidebarModelUpdate
{
public:
    virtual ~SidebarModelUpdate();
    virtual void updateModel(css::uno::Reference<css::frame::XModel> xModel) = 0;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
