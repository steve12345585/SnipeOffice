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

#include <svl/macitem.hxx>
#include <rtl/strbuf.hxx>

class Point;
class SvStream;

enum class IMapObjectType
{
    Rectangle = 1,
    Circle    = 2,
    Polygon   = 3
};

#define IMAP_OBJ_VERSION    (sal_uInt16(0x0005))
#define IMAGE_MAP_VERSION   (sal_uInt16(0x0001))

#define IMAPMAGIC           "SDIMAP"

#define IMAP_MIRROR_HORZ    0x00000001L
#define IMAP_MIRROR_VERT    0x00000002L

enum class IMapFormat
{
    Binary  = 1,
    CERN    = 2,
    NCSA    = 3,
    Detect  = 15,
};

#define IMAP_ERR_OK         0x00000000L
#define IMAP_ERR_FORMAT     0x00000001L

class IMapObject
{
    friend class        ImageMap;

    OUString            aURL;
    OUString            aAltText;
    OUString            aDesc;
    OUString            aTarget;
    OUString            aName;
    SvxMacroTableDtor   aEventList;
    bool                bActive;

protected:
    sal_uInt16              nReadVersion;

    // binary import/export
    virtual void        WriteIMapObject( SvStream& rOStm ) const = 0;
    virtual void        ReadIMapObject(  SvStream& rIStm ) = 0;

    // helper methods
    static void AppendCERNCoords(OStringBuffer& rBuf, const Point& rPoint100);
    void AppendCERNURL(OStringBuffer& rBuf) const;
    static void AppendNCSACoords(OStringBuffer& rBuf, const Point& rPoint100);
    void AppendNCSAURL(OStringBuffer&rBuf) const;

public:

                        IMapObject();
                        IMapObject( OUString aURL,
                                    OUString aAltText,
                                    OUString aDesc,
                                    OUString aTarget,
                                    OUString aName,
                                    bool bActive );
    virtual             ~IMapObject() {};

    IMapObject(IMapObject const &) = default;
    IMapObject(IMapObject &&) = default;
    IMapObject & operator =(IMapObject const &) = default;
    IMapObject & operator =(IMapObject &&) = default;

    virtual IMapObjectType GetType() const = 0;
    virtual bool        IsHit( const Point& rPoint ) const = 0;

    void                Write ( SvStream& rOStm ) const;
    void                Read( SvStream& rIStm );

    const OUString&     GetURL() const { return aURL; }
    void                SetURL( const OUString& rURL ) { aURL = rURL; }

    const OUString&     GetAltText() const { return aAltText; }
    void                SetAltText( const OUString& rAltText) { aAltText = rAltText; }

    const OUString&     GetDesc() const { return aDesc; }
    void                SetDesc( const OUString& rDesc ) { aDesc = rDesc; }

    const OUString&     GetTarget() const { return aTarget; }
    void                SetTarget( const OUString& rTarget ) { aTarget = rTarget; }

    const OUString&     GetName() const { return aName; }
    void                SetName( const OUString& rName ) { aName = rName; }

    bool                IsActive() const { return bActive; }
    void                SetActive( bool bSetActive ) { bActive = bSetActive; }

    bool                IsEqual( const IMapObject& rEqObj ) const;

    // IMap-Events
    const SvxMacroTableDtor& GetMacroTable() const { return aEventList;}
    void SetMacroTable( const SvxMacroTableDtor& rTbl ) { aEventList = rTbl; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
