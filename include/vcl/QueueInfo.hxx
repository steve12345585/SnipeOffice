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

#ifndef INCLUDED_VCL_QUEUEINFO_HXX
#define INCLUDED_VCL_QUEUEINFO_HXX

#include <rtl/ustring.hxx>

#include <vcl/dllapi.h>
#include <vcl/prntypes.hxx>

class VCL_DLLPUBLIC QueueInfo
{
    friend class Printer;

private:
    OUString maPrinterName;
    OUString maDriver;
    OUString maLocation;
    OUString maComment;
    PrintQueueFlags mnStatus;
    sal_uInt32 mnJobs;

public:
    QueueInfo();

    const OUString& GetPrinterName() const;
    const OUString& GetDriver() const;
    const OUString& GetLocation() const;
    const OUString& GetComment() const;
    PrintQueueFlags GetStatus() const;
    sal_uInt32 GetJobs() const;
};

#endif // INCLUDED_VCL_QUEUEINFO_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
