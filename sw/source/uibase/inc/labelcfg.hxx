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
#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_LABELCFG_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_LABELCFG_HXX

#include <unotools/configitem.hxx>
#include <swdllapi.h>
#include "labrec.hxx"

#include <map>
#include <vector>

struct SwLabelMeasure
{
    OUString m_aMeasure;     // string contains the label dimensions
    bool     m_bPredefined;  // used to distinguish predefined from user-defined labels
};

class SW_DLLPUBLIC SwLabelConfig final : public utl::ConfigItem
{
private:
    std::vector<OUString> m_aManufacturers;
    std::map< OUString, std::map<OUString, SwLabelMeasure> > m_aLabels;

    virtual void ImplCommit() override;

public:
    SwLabelConfig();
    virtual ~SwLabelConfig() override;

    virtual void Notify( const css::uno::Sequence< OUString >& aPropertyNames ) override;

    void    FillLabels(const OUString& rManufacturer, SwLabRecs& rLabArr);
    const std::vector<OUString>& GetManufacturers() const {return m_aManufacturers;}

    bool    HasLabel(const OUString& rManufacturer, const OUString& rType);
    bool        IsPredefinedLabel(const OUString& rManufacturer, const OUString& rType)
                  { return m_aLabels[rManufacturer][rType].m_bPredefined; };
    void        SaveLabel(const OUString& rManufacturer, const OUString& rType,
                            const SwLabRec& rRec);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
