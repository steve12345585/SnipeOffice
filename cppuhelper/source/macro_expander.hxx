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

#include <com/sun/star/uno/Reference.hxx>

namespace com::sun::star::lang { class XSingleComponentFactory; }

namespace cppuhelper {

namespace detail {

/**
 * Helper function to expand macros based on the unorc/uno.ini.
 *
 * @internal
 *
 * @param text
 * Some text.
 *
 * @return
 * The expanded text.
 *
 * @exception com::sun::star::lang::IllegalArgumentException
 * If uriReference is a vnd.sun.star.expand URL reference that contains unknown
 * macros.
 */
OUString expandMacros(OUString const & text);

css::uno::Reference< css::lang::XSingleComponentFactory >
create_bootstrap_macro_expander_factory();

}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
