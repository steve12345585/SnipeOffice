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

#include <vcl/dllapi.h>
#include <utility>
#include <vcl/font.hxx>
#include <o3tl/typed_flags_set.hxx>


enum class InputContextFlags
{
    NONE         = 0x0000,
    Text         = 0x0001,
    ExtText      = 0x0002
};
namespace o3tl
{
    template<> struct typed_flags<InputContextFlags> : is_typed_flags<InputContextFlags, 0x0003> {};
}


class VCL_DLLPUBLIC InputContext
{
private:
    vcl::Font          maFont;
    InputContextFlags  mnOptions;

public:
                    InputContext() { mnOptions = InputContextFlags::NONE; }
                    InputContext( const InputContext& rInputContext ) :
                        maFont( rInputContext.maFont )
                    { mnOptions = rInputContext.mnOptions; }
                    InputContext( vcl::Font aFont, InputContextFlags nOptions = InputContextFlags::NONE ) :
                        maFont(std::move( aFont ))
                    { mnOptions = nOptions; }

    const vcl::Font& GetFont() const { return maFont; }

    void              SetOptions( InputContextFlags nOptions ) { mnOptions = nOptions; }
    InputContextFlags GetOptions() const { return mnOptions; }

    InputContext&   operator=( const InputContext& rInputContext );
    bool            operator==( const InputContext& rInputContext ) const;
    bool            operator!=( const InputContext& rInputContext ) const
                        { return !(InputContext::operator==( rInputContext )); }
};

inline InputContext& InputContext::operator=( const InputContext& rInputContext )
{
    maFont      = rInputContext.maFont;
    mnOptions   = rInputContext.mnOptions;
    return *this;
}

inline bool InputContext::operator==( const InputContext& rInputContext ) const
{
    return ((mnOptions  == rInputContext.mnOptions) &&
            (maFont     == rInputContext.maFont));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
