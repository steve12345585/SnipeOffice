/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <tools/urlobj.hxx>
#include <utility>

class Place
{
private:
    OUString msName;
    INetURLObject maUrl;

    bool mbEditable;

public:

    Place( OUString sName, std::u16string_view sUrl, bool bEditable = false ) :
        msName(std::move( sName )),
        maUrl( sUrl ),
        mbEditable( bEditable ) {};

    void SetName(const OUString& aName ) { msName = aName; }
    void SetUrl(std::u16string_view aUrl ) { maUrl.SetURL( aUrl ); }

    OUString& GetName( ) { return msName; }
    OUString GetUrl( ) const { return maUrl.GetMainURL( INetURLObject::DecodeMechanism::NONE ); }
    INetURLObject& GetUrlObject( ) { return maUrl; }
    bool  IsLocal( ) const { return maUrl.GetProtocol() == INetProtocol::File; }
    bool  IsEditable( ) const { return mbEditable; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
