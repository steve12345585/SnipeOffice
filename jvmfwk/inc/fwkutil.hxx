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
#ifndef INCLUDED_JVMFWK_SOURCE_FWKUTIL_HXX
#define INCLUDED_JVMFWK_SOURCE_FWKUTIL_HXX

#include <config_folders.h>

#include <sal/config.h>
#include <rtl/bootstrap.hxx>
#include <rtl/byteseq.hxx>
#include <osl/mutex.hxx>

namespace osl { class Mutex; }

namespace jfw
{

/** Returns the file URL of the directory where the framework library
    (this library) resides.
*/
OUString getLibraryLocation();

/** provides a bootstrap class which already knows the values from the
    jvmfkwrc file.
*/
const rtl::Bootstrap * Bootstrap();

osl::Mutex& FwkMutex();

rtl::ByteSequence encodeBase16(const rtl::ByteSequence& rawData);
rtl::ByteSequence decodeBase16(const rtl::ByteSequence& data);

OUString getDirFromFile(std::u16string_view usFilePath);

enum FileStatus
{
    FILE_OK,
    FILE_DOES_NOT_EXIST,
    FILE_INVALID
};

/** checks if the URL is a file.

    If it is a link to a file than
    it is resolved. Assuming that the argument
    represents a relative URL then FILE_INVALID
    is returned.


    @return
    one of the values of FileStatus.

    @exception
    Errors occurred during determining if the file exists
 */
FileStatus checkFileURL(const OUString & path);

}
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
