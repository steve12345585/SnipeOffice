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

#include <utility>

#include "gio_datasupplier.hxx"
#include "gio_resultset.hxx"

using namespace com::sun::star::ucb;
using namespace com::sun::star::uno;

using namespace gio;

DynamicResultSet::DynamicResultSet(
    const Reference< XComponentContext >& rxContext,
    rtl::Reference< Content > xContent,
    const OpenCommandArgument2& rCommand,
    const Reference< XCommandEnvironment >& rxEnv )
    : ResultSetImplHelper( rxContext, rCommand ),
      m_xContent(std::move( xContent )),
      m_xEnv( rxEnv )
{
}

void DynamicResultSet::initStatic()
{
    m_xResultSet1 = new ::ucbhelper::ResultSet(
        m_xContext, m_aCommand.Properties,
        new DataSupplier( m_xContent, m_aCommand.Mode ), m_xEnv );
}

void DynamicResultSet::initDynamic()
{
    initStatic();
    m_xResultSet2 = m_xResultSet1;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
