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

#ifndef INCLUDED_XMLOFF_NAMEDBOOLPROPERTYHDL_HXX
#define INCLUDED_XMLOFF_NAMEDBOOLPROPERTYHDL_HXX

#include <config_options.h>
#include <utility>
#include <xmloff/xmlprhdl.hxx>
#include <xmloff/xmltoken.hxx>

/**
    PropertyHandler for a named xml bool type:
*/
class UNLESS_MERGELIBS_MORE(XMLOFF_DLLPUBLIC) XMLNamedBoolPropertyHdl final : public XMLPropertyHandler
{
private:
    const OUString maTrueStr;
    const OUString maFalseStr;

public:
    XMLNamedBoolPropertyHdl( OUString sTrueStr, OUString sFalseStr ) : maTrueStr(std::move( sTrueStr )), maFalseStr(std::move( sFalseStr )) {}

    XMLNamedBoolPropertyHdl(
            ::xmloff::token::XMLTokenEnum eTrue,
            ::xmloff::token::XMLTokenEnum eFalse
            ) :
        maTrueStr( ::xmloff::token::GetXMLToken( eTrue ) ),
        maFalseStr( ::xmloff::token::GetXMLToken( eFalse ) )
    {}

    virtual ~XMLNamedBoolPropertyHdl() override;

    virtual bool importXML( const OUString& rStrImpValue, css::uno::Any& rValue, const SvXMLUnitConverter& rUnitConverter ) const override;
    virtual bool exportXML( OUString& rStrExpValue, const css::uno::Any& rValue, const SvXMLUnitConverter& rUnitConverter ) const override;
};

#endif // INCLUDED_XMLOFF_NAMEDBOOLPROPERTYHDL_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
