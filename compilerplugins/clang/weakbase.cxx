/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LO_CLANG_SHARED_PLUGINS

#include <string>
#include <iostream>
#include <map>
#include <set>

#include "plugin.hxx"
#include "check.hxx"
#include "clang/AST/CXXInheritance.h"

/**
 * Check for multiple copies of WeakBase in base classes
 */
namespace
{
class WeakBase : public loplugin::FilteringPlugin<WeakBase>
{
public:
    explicit WeakBase(loplugin::InstantiationData const& data)
        : FilteringPlugin(data)
    {
    }

    bool preRun() override { return compiler.getLangOpts().CPlusPlus; }

    void run() override
    {
        if (preRun())
        {
            TraverseDecl(compiler.getASTContext().getTranslationUnitDecl());
        }
    }

    bool VisitCXXRecordDecl(CXXRecordDecl const*);
};

bool WeakBase::VisitCXXRecordDecl(CXXRecordDecl const* recordDecl)
{
    if (ignoreLocation(recordDecl))
    {
        return true;
    }
    //    StringRef aFileName = getFilenameOfLocation(
    //        compiler.getSourceManager().getSpellingLoc(fieldDecl->getBeginLoc()));

    //    if (loplugin::hasPathnamePrefix(aFileName, SRCDIR "/chart2/source/"))
    //        return true;
    //    if (loplugin::isSamePathname(aFileName, SRCDIR "/include/sfx2/recentdocsview.hxx"))
    //        return true;
    //    if (loplugin::isSamePathname(aFileName, SRCDIR "/include/sfx2/templatelocalview.hxx"))
    //        return true;
    //    if (loplugin::isSamePathname(aFileName, SRCDIR "/store/source/stortree.hxx")
    //        || loplugin::isSamePathname(aFileName, SRCDIR "/store/source/stordata.hxx"))
    //        return true;
    //    if (loplugin::isSamePathname(aFileName, SRCDIR "/sw/source/uibase/inc/dbtree.hxx"))
    //        return true;

    recordDecl = recordDecl->getCanonicalDecl();
    if (!recordDecl->hasDefinition())
        return true;

    int noWeakBases = 0;
    int noWeakObjects = 0;
    bool foundVirtualWeakBase = false;
    bool foundVirtualOWeakObject = false;
    std::string basePaths1;
    std::string basePaths2;
    auto BaseMatchesCallback = [&](const CXXBaseSpecifier* cxxBaseSpecifier, CXXBasePath& Paths) {
        if (!cxxBaseSpecifier->getType().getTypePtr())
            return false;
        const CXXRecordDecl* baseCXXRecordDecl = cxxBaseSpecifier->getType()->getAsCXXRecordDecl();
        if (!baseCXXRecordDecl)
            return false;
        if (baseCXXRecordDecl->isInvalidDecl())
            return false;
        bool isWeakBase(loplugin::DeclCheck(baseCXXRecordDecl)
                            .Struct("WeakBase")
                            .Namespace("tools")
                            .GlobalNamespace());
        bool isOWeakObject(loplugin::DeclCheck(baseCXXRecordDecl)
                               .Class("OWeakObject")
                               .Namespace("cppu")
                               .GlobalNamespace());
        if (isWeakBase)
        {
            if (cxxBaseSpecifier->isVirtual())
                foundVirtualWeakBase = true;
            else
                ++noWeakBases;
        }
        else if (isOWeakObject)
        {
            if (cxxBaseSpecifier->isVirtual())
                foundVirtualOWeakObject = true;
            else
                ++noWeakObjects;
        }
        else
            return false;
        std::string sPath;
        for (CXXBasePathElement const& pathElement : Paths)
        {
            if (!sPath.empty())
            {
                sPath += "->";
            }
            if (pathElement.Class->hasDefinition())
                sPath += pathElement.Class->getNameAsString();
            else
                sPath += "???";
        }
        sPath += "->";
        sPath += baseCXXRecordDecl->getNameAsString();
        if (isWeakBase)
        {
            if (!basePaths1.empty())
                basePaths1 += ", ";
            basePaths1 += sPath;
        }
        else
        {
            if (!basePaths2.empty())
                basePaths2 += ", ";
            basePaths2 += sPath;
        }
        return false;
    };

    CXXBasePaths aPaths;
    recordDecl->lookupInBases(BaseMatchesCallback, aPaths);

    if (foundVirtualWeakBase && noWeakBases > 0)
        report(DiagnosticsEngine::Warning,
               "found one virtual base and one or more normal bases of tools::WeakBase, through "
               "inheritance paths %0",
               recordDecl->getBeginLoc())
            << basePaths1;
    else if (!foundVirtualWeakBase && noWeakBases > 1)
        report(DiagnosticsEngine::Warning,
               "found multiple copies of tools::WeakBase, through inheritance paths %0",
               recordDecl->getBeginLoc())
            << basePaths1;

    if (foundVirtualOWeakObject && noWeakObjects > 0)
        report(DiagnosticsEngine::Warning,
               "found one virtual base and one or more normal bases of cppu::OWeakObject, through "
               "inheritance paths %0",
               recordDecl->getBeginLoc())
            << basePaths2;
    else if (!foundVirtualOWeakObject && noWeakObjects > 1)
        report(DiagnosticsEngine::Warning,
               "found multiple copies of cppu::OWeakObject, through inheritance paths %0",
               recordDecl->getBeginLoc())
            << basePaths2;

    return true;
}

loplugin::Plugin::Registration<WeakBase> weakbase("weakbase");

} // namespace

#endif // LO_CLANG_SHARED_PLUGINS

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
