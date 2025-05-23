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
#ifndef INCLUDED_VCL_INETIMG_HXX
#define INCLUDED_VCL_INETIMG_HXX

#include <rtl/ustring.hxx>
#include <tools/gen.hxx>
#include <sot/formats.hxx>
#include <utility>


class INetImage
{
    OUString        aImageURL;
    OUString        aTargetURL;
    OUString        aTargetFrame;
    Size            aSizePixel;

public:
                    INetImage(
                        OUString _aImageURL,
                        OUString _aTargetURL,
                        OUString _aTargetFrame )
                    :   aImageURL(std::move( _aImageURL )),
                        aTargetURL(std::move( _aTargetURL )),
                        aTargetFrame(std::move( _aTargetFrame ))
                    {}
                    INetImage()
                    {}

    const OUString& GetImageURL() const { return aImageURL; }
    const OUString& GetTargetURL() const { return aTargetURL; }
    const OUString& GetTargetFrame() const { return aTargetFrame; }

    // import/export
    void Write( SvStream& rOStm, SotClipboardFormatId nFormat ) const;
    bool Read( SvStream& rIStm, SotClipboardFormatId nFormat );
};

#endif // INCLUDED_VCL_INETIMG_HXX


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
