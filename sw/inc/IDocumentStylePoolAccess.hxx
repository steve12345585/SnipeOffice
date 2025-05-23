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

#include <sal/types.h>

class SwTextFormatColl;
class SwCharFormat;
class SwFormat;
class SwFrameFormat;
class SwNumRule;
class SwPageDesc;

/** Access to the style pool
 */
class IDocumentStylePoolAccess
{
public:
    /** Return "Auto-Collection with ID.
        Create, if it does not yet exist.
        If string pointer is defined request only description
        of attributes, do not create style sheet!
    */
    virtual SwTextFormatColl* GetTextCollFromPool(sal_uInt16 nId, bool bRegardLanguage = true) = 0;

    /** Return required automatic format base class.
    */
    virtual SwFormat* GetFormatFromPool(sal_uInt16 nId) = 0;

    /** Return required automatic format.
     */
    virtual SwFrameFormat* GetFrameFormatFromPool(sal_uInt16 nId) = 0;

    virtual SwCharFormat* GetCharFormatFromPool(sal_uInt16 nId) = 0;

    /** Return required automatic page style.
     */
    virtual SwPageDesc* GetPageDescFromPool(sal_uInt16 nId, bool bRegardLanguage = true) = 0;

    virtual SwNumRule* GetNumRuleFromPool(sal_uInt16 nId) = 0;

    /** Check whether this "auto-collection" is used in document.
     */
    virtual bool IsPoolTextCollUsed(sal_uInt16 nId) const = 0;
    virtual bool IsPoolFormatUsed(sal_uInt16 nId) const = 0;
    virtual bool IsPoolPageDescUsed(sal_uInt16 nId) const = 0;

protected:
    virtual ~IDocumentStylePoolAccess(){};
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
