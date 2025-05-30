/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/types.h>
#include <string_view>

namespace vcl
{
/// Allows creating, updating and writing PDF objects in a container.
class SAL_NO_VTABLE SAL_DLLPUBLIC_RTTI PDFObjectContainer
{
public:
    /* adds an entry to m_aObjects and returns its index+1,
     * sets the offset to ~0
     */
    virtual sal_Int32 createObject() = 0;
    /* sets the offset of object n to the current position of output file+1
     */
    virtual bool updateObject(sal_Int32 n) = 0;

    // Write pBuffer to the end of the output.
    virtual bool writeBufferBytes(const void* pBuffer, sal_uInt64 nBytes) = 0;

    // Utility method, reduces verbosity at call sites
    bool writeBuffer(std::string_view aBuffer)
    {
        return writeBufferBytes(aBuffer.data(), aBuffer.size());
    }

    virtual void checkAndEnableStreamEncryption(sal_Int32 nObject) = 0;

    virtual void disableStreamEncryption() = 0;

protected:
    ~PDFObjectContainer() noexcept = default;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
