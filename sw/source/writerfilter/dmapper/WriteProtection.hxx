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

#include "LoggedResources.hxx"
#include <com/sun/star/beans/PropertyValue.hpp>

namespace writerfilter::dmapper
{
class WriteProtection : public LoggedProperties
{
private:
    /** Provider type
         *
         * Possible values:
         *  "rsaAES"  - NS_ooxml::LN_Value_doc_ST_CryptProv_rsaAES
         *  "rsaFull" - NS_ooxml::LN_Value_doc_ST_CryptProv_rsaFull
         */
    sal_Int32 m_nCryptProviderType;
    OUString m_sCryptAlgorithmClass;
    OUString m_sCryptAlgorithmType;
    sal_Int32 m_CryptSpinCount;
    OUString m_sAlgorithmName;
    OUString m_sHash;
    OUString m_sSalt;
    bool m_bRecommended;

    virtual void lcl_attribute(Id Name, const Value& val) override;
    virtual void lcl_sprm(Sprm& sprm) override;

public:
    WriteProtection();
    virtual ~WriteProtection() override;

    css::uno::Sequence<css::beans::PropertyValue> toSequence() const;

    bool getRecommended() const { return m_bRecommended; }
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
