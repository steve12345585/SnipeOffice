/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <cassert>
#include <string>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include "plugin.hxx"
#include "check.hxx"
#include "clang/AST/CXXInheritance.h"
#include "clang/AST/StmtVisitor.h"

/**
    Look for nested if statements with relatively small conditions, where they can be collapsed into
    one if statement.
*/
namespace
{
class CollapseIf : public loplugin::FilteringPlugin<CollapseIf>
{
public:
    explicit CollapseIf(loplugin::InstantiationData const& data)
        : FilteringPlugin(data)
    {
    }

    virtual void run() override { TraverseDecl(compiler.getASTContext().getTranslationUnitDecl()); }

    bool VisitIfStmt(IfStmt const*);

private:
    int getNoCharsInSourceCodeOfExpr(IfStmt const*);
};

bool CollapseIf::VisitIfStmt(IfStmt const* ifStmt)
{
    if (ignoreLocation(ifStmt))
        return true;
    if (ifStmt->getElse())
        return true;

    IfStmt const* secondIfStmt = nullptr;
    if (auto compoundStmt = dyn_cast<CompoundStmt>(ifStmt->getThen()))
    {
        if (compoundStmt->size() != 1)
            return true;
        secondIfStmt = dyn_cast<IfStmt>(*compoundStmt->body_begin());
        if (!secondIfStmt)
            return true;
        if (secondIfStmt->getElse())
            return true;
    }
    else
    {
        secondIfStmt = dyn_cast<IfStmt>(ifStmt->getThen());
        if (!secondIfStmt)
            return true;
    }

    int noChars1 = getNoCharsInSourceCodeOfExpr(ifStmt);
    int noChars2 = getNoCharsInSourceCodeOfExpr(secondIfStmt);
    if (noChars1 + noChars2 > 40)
        return true;

    // Sometimes there is a comment between the first and second if, so
    // merging them would make the comment more awkward to write.
    if (containsComment(ifStmt->getSourceRange()))
        return true;

    report(DiagnosticsEngine::Warning, "nested if should be collapsed into one statement %0 %1",
           ifStmt->getBeginLoc())
        << noChars1 << noChars2 << ifStmt->getSourceRange();
    return true;
}

int CollapseIf::getNoCharsInSourceCodeOfExpr(IfStmt const* ifStmt)
{
    // Measure the space between the "if" the beginning of the "then" block because
    // measuring the size of the condition expression is unreliable, because clang
    // does not report the location of the last token accurately.
    SourceManager& SM = compiler.getSourceManager();
    SourceLocation startLoc = ifStmt->getBeginLoc();
    SourceLocation endLoc = ifStmt->getThen()->getBeginLoc();
    char const* p1 = SM.getCharacterData(startLoc);
    char const* p2 = SM.getCharacterData(endLoc);

    int count = 0;
    for (auto p = p1; p < p2; ++p)
        if (*p != ' ')
            ++count;

    return count;
}

/** Off by default because some places are a judgement call if it should be collapsed or not. */
loplugin::Plugin::Registration<CollapseIf> X("collapseif", false);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
