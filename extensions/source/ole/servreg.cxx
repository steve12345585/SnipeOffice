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

#include <osl/time.h>
#include "ole2uno.hxx"
#include "servprov.hxx"
#include <rtl/ustring.hxx>
#include <cppuhelper/factory.hxx>

using namespace cppu;

Reference<XInterface> ConverterProvider_CreateInstance2(   const Reference<XMultiServiceFactory> & xSMgr)
{
    Reference<XInterface> xService = *new OleConverter( xSMgr);
    return xService;
}

Reference<XInterface> ConverterProvider_CreateInstanceVar1(    const Reference<XMultiServiceFactory> & xSMgr)
{
    Reference<XInterface> xService = *new OleConverter( xSMgr, UNO_OBJECT_WRAPPER_REMOTE_OPT, IUNKNOWN_WRAPPER_IMPL);
    return xService;
}

Reference<XInterface> OleClient_CreateInstance( const Reference<XMultiServiceFactory> & xSMgr)
{
    Reference<XInterface> xService = *new OleClient( xSMgr);
    return xService;
}

Reference<XInterface> OleServer_CreateInstance( const Reference<XMultiServiceFactory> & xSMgr)
{
    Reference<XInterface > xService = *new OleServer(xSMgr);
    return xService;
}

extern "C" SAL_DLLPUBLIC_EXPORT void * oleautobridge_component_getFactory(
    const char * pImplName, void * pServiceManager, void * /*pRegistryKey*/ )
{
    void * pRet = nullptr;

    OUString aImplName( OUString::createFromAscii( pImplName ) );
    Reference< XSingleServiceFactory > xFactory;
    Sequence<OUString> seqServiceNames;
    if (pServiceManager && aImplName == "com.sun.star.comp.ole.OleConverter2")
    {
        xFactory=  createSingleFactory( static_cast< XMultiServiceFactory*>(pServiceManager),
                                         aImplName,
                                         ConverterProvider_CreateInstance2, seqServiceNames );
    }
    else if (pServiceManager && aImplName == "com.sun.star.comp.ole.OleConverterVar1")
    {
        xFactory= createSingleFactory( static_cast<XMultiServiceFactory*>(pServiceManager),
                                       aImplName,
                                       ConverterProvider_CreateInstanceVar1, seqServiceNames );
    }
    else if(pServiceManager && aImplName == "com.sun.star.comp.ole.OleClient")
    {
        xFactory= createSingleFactory( static_cast< XMultiServiceFactory*>(pServiceManager),
                                       aImplName,
                                       OleClient_CreateInstance, seqServiceNames );
    }
    else if(pServiceManager && aImplName == "com.sun.star.comp.ole.OleServer")
    {
        xFactory= createOneInstanceFactory( static_cast< XMultiServiceFactory*>(pServiceManager),
                                            aImplName,
                                            OleServer_CreateInstance, seqServiceNames );
    }

    if (xFactory.is())
    {
        xFactory->acquire();
        pRet = xFactory.get();
    }

    return pRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
