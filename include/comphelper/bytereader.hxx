/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <config_options.h>
#include <sal/types.h>
#include <comphelper/comphelperdllapi.h>

namespace comphelper
{
/**
 * Interface that we can cast to, to bypass the inefficiency of using Sequence<sal_Int8>
 * when reading via XInputStream.
 */
class UNLESS_MERGELIBS(COMPHELPER_DLLPUBLIC) SAL_LOPLUGIN_ANNOTATE("crosscast") ByteReader
{
public:
    virtual ~ByteReader();
    virtual sal_Int32 readSomeBytes(sal_Int8* aData, sal_Int32 nBytesToRead) = 0;
};

/**
 * Interface that we can cast to, to bypass the inefficiency of using Sequence<sal_Int8>
 * when writing via XOutputStream.
 */
class UNLESS_MERGELIBS(COMPHELPER_DLLPUBLIC) SAL_LOPLUGIN_ANNOTATE("crosscast") ByteWriter
{
public:
    virtual ~ByteWriter();
    virtual void writeBytes(const sal_Int8* aData, sal_Int32 nBytesToWrite) = 0;
};

} // namespace utl

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
