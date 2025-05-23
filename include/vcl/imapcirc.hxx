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
#ifndef INCLUDED_VCL_IMAPCIRC_HXX
#define INCLUDED_VCL_IMAPCIRC_HXX

#include <vcl/dllapi.h>
#include <tools/gen.hxx>
#include <vcl/imapobj.hxx>

class Fraction;

class UNLESS_MERGELIBS(VCL_DLLPUBLIC) IMapCircleObject final : public IMapObject
{
    Point               aCenter;
    sal_Int32           nRadius;

    void                ImpConstruct( const Point& rCenter, sal_Int32 nRad, bool bPixel );

    // binary import/export
    virtual void        WriteIMapObject( SvStream& rOStm ) const override;
    virtual void        ReadIMapObject(  SvStream& rIStm ) override;

public:

                        IMapCircleObject() : nRadius(0) {}
                        IMapCircleObject( const Point& rCenter, sal_Int32 nRad,
                                          const OUString& rURL,
                                          const OUString& rAltText,
                                          const OUString& rDesc,
                                          const OUString& rTarget,
                                          const OUString& rName,
                                          bool bActive = true,
                                          bool bPixelCoords = true );

    virtual IMapObjectType GetType() const override;
    virtual bool        IsHit( const Point& rPoint ) const override;

    Point               GetCenter( bool bPixelCoords = true ) const;
    sal_Int32           GetRadius( bool bPixelCoords = true ) const;

    void                Scale( const Fraction& rFractX, const Fraction& rFracY );

    using IMapObject::IsEqual;
    bool                IsEqual( const IMapCircleObject& rEqObj ) const;

    // import/export
    void                WriteCERN( SvStream& rOStm ) const;
    void                WriteNCSA( SvStream& rOStm ) const;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
