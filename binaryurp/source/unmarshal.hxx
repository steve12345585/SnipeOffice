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

#include <vector>

#include <com/sun/star/uno/Sequence.hxx>
#include <rtl/ref.hxx>
#include <sal/types.h>
#include <typelib/typedescription.hxx>

namespace binaryurp {
    class BinaryAny;
    class Bridge;
    struct ReaderState;
}
namespace com::sun::star::uno { class TypeDescription; }

namespace binaryurp {

class Unmarshal {
public:
    Unmarshal(
        rtl::Reference< Bridge > bridge, ReaderState & state,
        css::uno::Sequence< sal_Int8 > const & buffer);

    ~Unmarshal();

    sal_uInt8 read8();

    sal_uInt16 read16();

    sal_uInt32 read32();

    css::uno::TypeDescription readType();

    OUString readOid();

    rtl::ByteSequence readTid();

    BinaryAny readValue(css::uno::TypeDescription const & type);

    void done() const;

private:
    Unmarshal(const Unmarshal&) = delete;
    Unmarshal& operator=(const Unmarshal&) = delete;

    void check(sal_Int32 size) const;

    sal_uInt32 readCompressed();

    sal_uInt16 readCacheIndex();

    sal_uInt64 read64();

    OUString readString();

    BinaryAny readSequence(css::uno::TypeDescription const & type);

    void readMemberValues(
        css::uno::TypeDescription const & type,
        std::vector< BinaryAny > * values);

    rtl::Reference< Bridge > bridge_;
    ReaderState & state_;
    css::uno::Sequence< sal_Int8 > buffer_;
    sal_uInt8 const * data_;
    sal_uInt8 const * end_;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
