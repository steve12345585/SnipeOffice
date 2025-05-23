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
#ifndef INCLUDED_SW_INC_INIT_HXX
#define INCLUDED_SW_INC_INIT_HXX

#include <sal/config.h>

#include <vector>

#include <osl/module.h>
#include <osl/module.hxx>

class SvGlobalName;
class SwViewShell;
class ItemInfoPackage;

void InitCore();   // bastyp/init.cxx
void FinitCore();
ItemInfoPackage& getItemInfoPackageSwAttributes();

namespace sw {

// basflt/fltini.cxx
class Filters
{
private:
    Filters(Filters const&) = delete;
    Filters& operator=(Filters const&) = delete;

public:
    Filters();

    ~Filters();
#ifndef DISABLE_DYNLOADING
    static oslGenericFunction GetMswordLibSymbol( const char *pSymbol );
#endif
};

}

// layout/newfrm.cxx
void FrameInit();
void FrameFinit();
void SetShell( SwViewShell *pSh );

// text/txtfrm.cxx
void TextInit_();
void TextFinit();

// We collect the GlobalNames of the servers at runtime, who don't want to be notified
// about printer changes. Thereby saving loading a lot of objects (luckily all foreign
// objects are mapped to one ID).
// Initialisation and deinitialisation can be found in init.cxx
extern std::vector<SvGlobalName> *pGlobalOLEExcludeList;

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
