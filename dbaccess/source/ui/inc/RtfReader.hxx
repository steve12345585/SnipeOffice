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

#include <vector>
#include <svtools/parrtf.hxx>
#include "DExport.hxx"

class SvStream;

namespace dbaui
{
    class ORTFReader final : public SvRTFParser , public ODatabaseExport
    {
        std::vector<Color>    m_vecColor;

        bool                    CreateTable(int nToken);
        virtual void            NextToken( int nToken ) override; // base class
        virtual TypeSelectionPageFactory
                                getTypeSelectionPageFactory() override;

        virtual ~ORTFReader() override;

    public:
        ORTFReader( SvStream& rIn,
                    const SharedConnection& _rxConnection,
                    const css::uno::Reference< css::util::XNumberFormatter >& _rxNumberF,
                    const css::uno::Reference< css::uno::XComponentContext >& _rxContext);
        // required for automatic type recognition
        ORTFReader( SvStream& rIn,
                    sal_Int32 nRows,
                    TPositions&& _rColumnPositions,
                    const css::uno::Reference< css::util::XNumberFormatter >& _rxNumberF,
                    const css::uno::Reference< css::uno::XComponentContext >& _rxContext,
                    const TColumnVector* rList,
                    const OTypeInfoMap* _pInfoMap,
                    bool _bAutoIncrementEnabled);

        virtual SvParserState   CallParser() override;// base class
    };
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
