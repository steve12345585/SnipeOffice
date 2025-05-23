/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef LO_CLANG_SHARED_PLUGINS

#include <memory>
#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include <set>

#include "check.hxx"
#include "plugin.hxx"

/**
  Check for calls to operator== on a std::container< std::unique_ptr >, which is not useful,
  because std::container will compare the pointers so it is never true
*/

namespace {

class PtrVector:
    public loplugin::FilteringPlugin<PtrVector>
{
public:
    explicit PtrVector(loplugin::InstantiationData const & data): FilteringPlugin(data)
    {}

    virtual void run() override
    {
        if (preRun())
            TraverseDecl(compiler.getASTContext().getTranslationUnitDecl());
    }

    bool shouldVisitTemplateInstantiations () const { return true; }

    bool VisitCXXOperatorCallExpr(const CXXOperatorCallExpr* );
};

bool PtrVector::VisitCXXOperatorCallExpr(const CXXOperatorCallExpr* expr)
{
    if (ignoreLocation(expr)) {
        return true;
    }
    if (expr->getOperator() != clang::OverloadedOperatorKind::OO_EqualEqual
        && expr->getOperator() != clang::OverloadedOperatorKind::OO_ExclaimEqual)
    {
        return true;
    }
    if (isa<CXXNullPtrLiteralExpr>(expr->getArg(1))) {
        return true;
    }
    const Expr* argExpr = expr->getArg(0);
    std::string s = argExpr->getType().getDesugaredType(compiler.getASTContext()).getAsString();
    if (s.find("iterator") != std::string::npos
        || (loplugin::TypeCheck(argExpr->getType()).Class("__wrap_iter").Namespace("__1")
            .StdNamespace()))
    {
        return true;
    }
    if (s.find("array") == std::string::npos && s.find("deque") == std::string::npos
        && s.find("list") == std::string::npos && s.find("vector") == std::string::npos
        && s.find("set") == std::string::npos && s.find("map") == std::string::npos
        && s.find("stack") == std::string::npos && s.find("queue") == std::string::npos)
    {
        return true;
    }
    if (s.find("unique_ptr") != std::string::npos) {
         expr->getArg(1)->dump();
        report(
            DiagnosticsEngine::Warning,
            "do not call operator== on a std container containing a unique_ptr " + s,
            expr->getExprLoc());
    }
    return true;
}


loplugin::Plugin::Registration< PtrVector > ptrvector("ptrvector");

} // namespace

#endif // LO_CLANG_SHARED_PLUGINS

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
