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

#ifndef INCLUDED_OOX_CORE_RECORDPARSER_HXX
#define INCLUDED_OOX_CORE_RECORDPARSER_HXX

#include <unordered_map>
#include <memory>

#include <oox/helper/binaryinputstream.hxx>
#include <oox/core/fragmenthandler.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>

namespace oox {
namespace core {

namespace prv {
    class ContextStack;
    class Locator;
}


struct RecordInputSource
{
    BinaryInputStreamRef mxInStream;
    OUString     maSystemId;
};


class RecordParser final
{
public:
                        RecordParser();
                        ~RecordParser();

    void                setFragmentHandler( const ::rtl::Reference< FragmentHandler >& rxHandler );

    /// @throws css::xml::sax::SAXException
    /// @throws css::io::IOException
    /// @throws css::uno::RuntimeException
    void                parseStream( const RecordInputSource& rInputSource );

    const RecordInputSource& getInputSource() const { return maSource; }

private:
    /** Returns a RecordInfo struct that contains the passed record identifier
        as context start identifier. */
    const RecordInfo*   getStartRecordInfo( sal_Int32 nRecId ) const;
    /** Returns a RecordInfo struct that contains the passed record identifier
        as context end identifier. */
    const RecordInfo*   getEndRecordInfo( sal_Int32 nRecId ) const;

private:
    typedef ::std::unordered_map< sal_Int32, RecordInfo > RecordInfoMap;

    RecordInputSource   maSource;
    ::rtl::Reference< FragmentHandler > mxHandler;
    ::rtl::Reference< prv::Locator > mxLocator;
    ::std::unique_ptr< prv::ContextStack > mxStack;
    RecordInfoMap       maStartMap;
    RecordInfoMap       maEndMap;
};


} // namespace core
} // namespace oox

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
