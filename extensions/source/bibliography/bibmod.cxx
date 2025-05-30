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

#include <unotools/resmgr.hxx>

#include "bibmod.hxx"
#include "bibresid.hxx"
#include "datman.hxx"
#include "bibconfig.hxx"
#include <ucbhelper/content.hxx>

static BibModul*  pBibModul=nullptr;
static sal_uInt32 nBibModulCount=0;

using namespace ::com::sun::star;

HdlBibModul OpenBibModul()
{
    if(pBibModul==nullptr)
    {
        pBibModul=new BibModul();
    }
    nBibModulCount++;
    return &pBibModul;
}

void CloseBibModul(HdlBibModul ppBibModul)
{
    nBibModulCount--;
    if(nBibModulCount==0 && ppBibModul!=nullptr)
    {
        delete pBibModul;
        pBibModul=nullptr;
    }
}

OUString BibResId(TranslateId aId)
{
    return Translate::get(aId, pBibModul->GetResLocale());
}

BibConfig* BibModul::pBibConfig = nullptr;

BibModul::BibModul()
    : m_aResLocale(Translate::Create("pcr"))
{
}

BibModul::~BibModul()
{
    if (pBibConfig && pBibConfig->IsModified())
        pBibConfig->Commit();
    delete pBibConfig;
    pBibConfig = nullptr;
}

rtl::Reference<BibDataManager>  BibModul::createDataManager()
{
    return new BibDataManager();
}

BibConfig*  BibModul::GetConfig()
{
    if(! pBibConfig)
        pBibConfig = new BibConfig;
    return pBibConfig;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
