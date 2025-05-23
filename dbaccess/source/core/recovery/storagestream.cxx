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

#include "storagestream.hxx"

#include <com/sun/star/embed/ElementModes.hpp>

#include <comphelper/diagnose_ex.hxx>

namespace dbaccess
{

    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::uno::UNO_SET_THROW;
    using ::com::sun::star::embed::XStorage;
    using ::com::sun::star::io::XStream;

    namespace ElementModes = ::com::sun::star::embed::ElementModes;

    // StorageOutputStream
    StorageOutputStream::StorageOutputStream(   const Reference< XStorage >& i_rParentStorage,
                                                const OUString& i_rStreamName
                                             )
    {
        ENSURE_OR_THROW( i_rParentStorage.is(), "illegal stream" );

        const Reference< XStream > xStream(
            i_rParentStorage->openStreamElement( i_rStreamName, ElementModes::READWRITE ), UNO_SET_THROW );
        m_xOutputStream.set( xStream->getOutputStream(), UNO_SET_THROW );
    }

    StorageOutputStream::~StorageOutputStream()
    {
    }

} // namespace dbaccess

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
