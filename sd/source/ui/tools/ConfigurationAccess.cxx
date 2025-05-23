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

#include <tools/ConfigurationAccess.hxx>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertysequence.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <sal/log.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace sd::tools {

ConfigurationAccess::ConfigurationAccess (
    const OUString& rsRootName,
    const WriteMode eMode)
{
    Reference<lang::XMultiServiceFactory> xProvider =
        configuration::theDefaultProvider::get( ::comphelper::getProcessComponentContext() );
    Initialize(xProvider, rsRootName, eMode);
}

void ConfigurationAccess::Initialize (
    const Reference<lang::XMultiServiceFactory>& rxProvider,
    const OUString& rsRootName,
    const WriteMode eMode)
{
    try
    {
        Sequence<Any> aCreationArguments(comphelper::InitAnyPropertySequence(
        {
            {"nodepath", Any(rsRootName)},
            {"depth", Any(sal_Int32(-1))}
        }));

        OUString sAccessService;
        if (eMode == READ_ONLY)
            sAccessService = "com.sun.star.configuration.ConfigurationAccess";
        else
            sAccessService = "com.sun.star.configuration.ConfigurationUpdateAccess";

        mxRoot = rxProvider->createInstanceWithArguments(
            sAccessService,
            aCreationArguments);
    }
    catch (Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("sd.tools");
    }
}

Any ConfigurationAccess::GetConfigurationNode (
    const OUString& sPathToNode)
{
    return GetConfigurationNode(
        Reference<container::XHierarchicalNameAccess>(mxRoot, UNO_QUERY),
        sPathToNode);
}

Any ConfigurationAccess::GetConfigurationNode (
    const css::uno::Reference<css::container::XHierarchicalNameAccess>& rxNode,
    const OUString& sPathToNode)
{
    if (sPathToNode.isEmpty())
        return Any(rxNode);

    try
    {
        if (rxNode.is())
        {
            return rxNode->getByHierarchicalName(sPathToNode);
        }
    }
    catch (const Exception&)
    {
        TOOLS_WARN_EXCEPTION("sd", "caught exception while getting configuration node" << sPathToNode);
    }

    return Any();
}

void ConfigurationAccess::CommitChanges()
{
    Reference<util::XChangesBatch> xConfiguration (mxRoot, UNO_QUERY);
    if (xConfiguration.is())
        xConfiguration->commitChanges();
}

} // end of namespace sd::tools

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
