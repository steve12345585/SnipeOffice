/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <framework/fwkdllapi.h>
#include <functional>

#include <com/sun/star/uno/Reference.hxx>

namespace com::sun::star::ui { class XContextChangeEventListener; }
namespace com::sun::star::uno { class XInterface; }
namespace com::sun::star::uno { class XComponentContext; }

namespace framework {

// this is pretty horrible, don't use it!
FWK_DLLPUBLIC css::uno::Reference<css::ui::XContextChangeEventListener>
GetFirstListenerWith(
    css::uno::Reference<css::uno::XComponentContext> const & xComponentContext,
    css::uno::Reference<css::uno::XInterface> const& xEventFocus,
    std::function<bool (css::uno::Reference<css::ui::XContextChangeEventListener> const&)> const& rPredicate);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
