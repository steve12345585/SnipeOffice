/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <memory>
#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include "config_clang.h"
#include "plugin.hxx"

/**
  Find calls to "delete x" where x is a field on an object.
  Should rather be using std::unique_ptr
*/

namespace {

class AutoMem:
    public loplugin::FilteringPlugin<AutoMem>
{
public:
    explicit AutoMem(loplugin::InstantiationData const & data): FilteringPlugin(data), mbInsideDestructor(false) {}

    virtual void run() override
    {
        TraverseDecl(compiler.getASTContext().getTranslationUnitDecl());
    }

    bool TraverseCXXDestructorDecl(CXXDestructorDecl* );
    bool VisitCXXDeleteExpr(const CXXDeleteExpr* );
private:
    bool mbInsideDestructor;
};

bool AutoMem::TraverseCXXDestructorDecl(CXXDestructorDecl* expr)
{
    mbInsideDestructor = true;
    bool ret = RecursiveASTVisitor::TraverseCXXDestructorDecl(expr);
    mbInsideDestructor = false;
    return ret;
}

bool AutoMem::VisitCXXDeleteExpr(const CXXDeleteExpr* expr)
{
    if (ignoreLocation( expr ))
        return true;
    StringRef aFileName = getFilenameOfLocation(compiler.getSourceManager().getSpellingLoc(expr->getBeginLoc()));
    if (loplugin::hasPathnamePrefix(aFileName, SRCDIR "/include/salhelper/")
        || loplugin::hasPathnamePrefix(aFileName, SRCDIR "/include/osl/")
        || loplugin::hasPathnamePrefix(aFileName, SRCDIR "/salhelper/")
        || loplugin::hasPathnamePrefix(aFileName, SRCDIR "/store/")
        || loplugin::hasPathnamePrefix(aFileName, SRCDIR "/sal/"))
        return true;

    if (mbInsideDestructor)
        return true;

    const ImplicitCastExpr* pCastExpr = dyn_cast<ImplicitCastExpr>(expr->getArgument());
    if (!pCastExpr)
        return true;
    const MemberExpr* pMemberExpr = dyn_cast<MemberExpr>(pCastExpr->getSubExpr());
    if (!pMemberExpr)
        return true;
    // ignore union games
    const FieldDecl* pFieldDecl = dyn_cast<FieldDecl>(pMemberExpr->getMemberDecl());
    if (!pFieldDecl)
        return true;
    TagDecl const * td = dyn_cast<TagDecl>(pFieldDecl->getDeclContext());
    if (td->isUnion())
        return true;

    report(
        DiagnosticsEngine::Warning,
        "calling delete on object field, rather use std::unique_ptr or std::scoped_ptr",
        expr->getBeginLoc())
        << expr->getSourceRange();
    return true;
}

loplugin::Plugin::Registration< AutoMem > X("automem", false);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
