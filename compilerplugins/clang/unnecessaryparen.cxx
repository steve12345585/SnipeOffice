/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef LO_CLANG_SHARED_PLUGINS

#include <cassert>
#include <string>
#include <iostream>
#include <fstream>
#include <set>
#include <unordered_set>

#include <clang/AST/CXXInheritance.h>

#include "config_clang.h"

#include "plugin.hxx"

/**
look for unnecessary parentheses
*/

namespace {

// Like clang::Stmt::IgnoreImplicit (lib/AST/Stmt.cpp), but also ignoring CXXConstructExpr and
// looking through implicit UserDefinedConversion's member function call:
Expr const * ignoreAllImplicit(Expr const * expr) {
    while (true)
    {
        auto oldExpr = expr;
        if (auto const e = dyn_cast<ExprWithCleanups>(expr)) {
            expr = e->getSubExpr();
        }
        else if (auto const e = dyn_cast<CXXConstructExpr>(expr)) {
            if (e->getNumArgs() == 1) {
                expr = e->getArg(0);
            }
        }
        else if (auto const e = dyn_cast<MaterializeTemporaryExpr>(expr)) {
            expr = e->getSubExpr();
        }
        else if (auto const e = dyn_cast<CXXBindTemporaryExpr>(expr)) {
            expr = e->getSubExpr();
        }
        else if (auto const e = dyn_cast<ImplicitCastExpr>(expr)) {
            expr = e->getSubExpr();
            if (e->getCastKind() == CK_UserDefinedConversion) {
                auto const ce = cast<CXXMemberCallExpr>(expr);
                assert(ce->getNumArgs() == 0);
                expr = ce->getImplicitObjectArgument();
            }
        }
        else if (auto const e = dyn_cast<ConstantExpr>(expr)) {
            expr = e->getSubExpr();
        }
        if (expr == oldExpr)
            return expr;
    }
    return expr;
}

bool isParenWorthyOpcode(BinaryOperatorKind op) {
    return !(BinaryOperator::isMultiplicativeOp(op) || BinaryOperator::isAdditiveOp(op)
             || BinaryOperator::isPtrMemOp(op));
}

class UnnecessaryParen:
    public loplugin::FilteringRewritePlugin<UnnecessaryParen>
{
public:
    explicit UnnecessaryParen(loplugin::InstantiationData const & data):
        FilteringRewritePlugin(data) {}

    virtual bool preRun() override
    {
        StringRef fn(handler.getMainFileName());
        // fixing this, makes the source in the .y files look horrible
        if (loplugin::isSamePathname(fn, WORKDIR "/YaccTarget/unoidl/source/sourceprovider-parser.cxx"))
            return false;
        return true;
    }
    virtual void run() override
    {
        if( preRun())
            TraverseDecl(compiler.getASTContext().getTranslationUnitDecl());
    }

    bool VisitParenExpr(const ParenExpr *);
    bool VisitIfStmt(const IfStmt *);
    bool VisitDoStmt(const DoStmt *);
    bool VisitWhileStmt(const WhileStmt *);
    bool VisitForStmt(ForStmt const * stmt);
    bool VisitSwitchStmt(const SwitchStmt *);
    bool VisitCaseStmt(const CaseStmt *);
    bool VisitReturnStmt(const ReturnStmt* );
    bool VisitCallExpr(const CallExpr *);
    bool VisitVarDecl(const VarDecl *);
    bool VisitCXXOperatorCallExpr(const CXXOperatorCallExpr *);
    bool VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr const *);
    bool VisitConditionalOperator(ConditionalOperator const * expr);
    bool VisitBinaryConditionalOperator(BinaryConditionalOperator const * expr);
    bool VisitMemberExpr(const MemberExpr *f);
    bool VisitCXXDeleteExpr(const CXXDeleteExpr *);

    bool VisitImplicitCastExpr(ImplicitCastExpr const * expr) {
        if (ignoreLocation(expr)) {
            return true;
        }
        if (expr->getCastKind() != CK_UserDefinedConversion) {
            return true;
        }
        // Filter out a MemberExpr (resp. a ParenExpr sub-expr, if any, as would be found by
        // VisitMemberExpr) that is part of a CXXMemberCallExpr which in turn is part of an
        // ImplicitCastExpr, so that VisitMemberExpr doesn't erroneously pick it up (and note that
        // CXXMemberCallExpr's getImplicitObjectArgument() skips past the underlying MemberExpr):
        if (auto const e1 = dyn_cast<CXXMemberCallExpr>(expr->getSubExpr())) {
            if (auto const e2 = dyn_cast<ParenExpr>(
                    e1->getImplicitObjectArgument()->IgnoreImpCasts()))
            {
                handled_.insert(e2);
            }
        }
        return true;
    }

private:
    void VisitSomeStmt(Stmt const * stmt, const Expr* cond, StringRef stmtName);

    void handleUnreachableCodeConditionParens(Expr const * expr);

    // Hack for libxml2's BAD_CAST object-like macro (expanding to "(xmlChar *)"), which is
    // typically used as if it were a function-like macro, e.g., as "BAD_CAST(pName)" in
    // SwNode::dumpAsXml (sw/source/core/docnode/node.cxx):
    bool isPrecededBy_BAD_CAST(Expr const * expr);

    bool badCombination(SourceLocation loc, int prevOffset, int nextOffset);

    bool removeParens(ParenExpr const * expr);

    // Returns 0 if not a string literal at all:
    unsigned getStringLiteralTokenCount(Expr const * expr, Expr const * parenExpr) {
        if (auto const e = dyn_cast<clang::StringLiteral>(expr)) {
            if (parenExpr == nullptr || !isPrecededBy_BAD_CAST(parenExpr)) {
                return e->getNumConcatenated();
            }
        } else if (auto const e = dyn_cast<UserDefinedLiteral>(expr)) {
            clang::StringLiteral const * lit = nullptr;
            switch (e->getLiteralOperatorKind()) {
            case UserDefinedLiteral::LOK_Template:
                {
                    auto const decl = e->getDirectCallee();
                    assert(decl != nullptr);
                    auto const args = decl->getTemplateSpecializationArgs();
                    assert(args != nullptr);
                    if (args->size() == 1 && (*args)[0].getKind() == TemplateArgument::Declaration)
                    {
                        if (auto const d
                            = dyn_cast<TemplateParamObjectDecl>((*args)[0].getAsDecl()))
                        {
                            if (d->getValue().isStruct() || d->getValue().isUnion()) {
                                //TODO: There appears to be no way currently to get at the original
                                // clang::StringLiteral expression from which this struct/union
                                // non-type template argument was constructed, so no way to tell
                                // whether it was written as a single literal (=> in which case we
                                // should warn about unnecessary parentheses) or as a concatenation
                                // of multiple literals (=> in which case we should not warn).  So
                                // be conservative and not warn at all (by pretending to have more
                                // than one token):
                                return 2;
                            }
                        }
                    }
                    break;
                }
            case UserDefinedLiteral::LOK_String:
                assert(e->getNumArgs() == 2);
                lit = dyn_cast<clang::StringLiteral>(e->getArg(0)->IgnoreImplicit());
                break;
            default:
                break;
            }
            if (lit != nullptr) {
                return lit->getNumConcatenated();
            }
        }
        return 0;
    }

    std::unordered_set<ParenExpr const *> handled_;
};

bool UnnecessaryParen::VisitUnaryExprOrTypeTraitExpr(UnaryExprOrTypeTraitExpr const * expr)
{
    if (expr->getKind() == UETT_SizeOf && !expr->isArgumentType()) {
        if (auto const e = dyn_cast<ParenExpr>(ignoreAllImplicit(expr->getArgumentExpr()))) {
            handled_.insert(e);
        }
    }
    return true;
}

bool UnnecessaryParen::VisitConditionalOperator(ConditionalOperator const * expr) {
    handleUnreachableCodeConditionParens(expr->getCond());
    return true;
}

bool UnnecessaryParen::VisitBinaryConditionalOperator(BinaryConditionalOperator const * expr) {
    handleUnreachableCodeConditionParens(expr->getCond());
    return true;
}

bool UnnecessaryParen::VisitParenExpr(const ParenExpr* parenExpr)
{
    if (ignoreLocation(parenExpr))
        return true;
    if (parenExpr->getBeginLoc().isMacroID())
        return true;
    if (handled_.find(parenExpr) != handled_.end())
        return true;

    auto subExpr = ignoreAllImplicit(parenExpr->getSubExpr());

    if (auto subParenExpr = dyn_cast<ParenExpr>(subExpr))
    {
        if (subParenExpr->getBeginLoc().isMacroID())
            return true;
        report(
            DiagnosticsEngine::Warning, "parentheses around parentheses",
            parenExpr->getBeginLoc())
            << parenExpr->getSourceRange();
        handled_.insert(subParenExpr);
    }

    // Somewhat redundantly add parenExpr to handled_, so that issues within InitListExpr don't get
    // reported twice (without having to change TraverseInitListExpr to only either traverse the
    // syntactic or semantic form, as other plugins do):

    if (isa<DeclRefExpr>(subExpr)) {
        if (!isPrecededBy_BAD_CAST(parenExpr)) {
            report(
                DiagnosticsEngine::Warning, "unnecessary parentheses around identifier",
                parenExpr->getBeginLoc())
                << parenExpr->getSourceRange();
            handled_.insert(parenExpr);
        }
    } else if (isa<IntegerLiteral>(subExpr) || isa<CharacterLiteral>(subExpr)
               || isa<FloatingLiteral>(subExpr) || isa<ImaginaryLiteral>(subExpr)
               || isa<CXXBoolLiteralExpr>(subExpr) || isa<CXXNullPtrLiteralExpr>(subExpr)
               || isa<ObjCBoolLiteralExpr>(subExpr))
    {
        auto const loc = subExpr->getBeginLoc();
        if (loc.isMacroID() && compiler.getSourceManager().isAtStartOfImmediateMacroExpansion(loc))
        {
            // just in case the macro could also expand to something that /would/ require
            // parentheses here
            return true;
        }
        report(
            DiagnosticsEngine::Warning, "unnecessary parentheses around literal",
            parenExpr->getBeginLoc())
            << parenExpr->getSourceRange();
        handled_.insert(parenExpr);
    } else if (isa<clang::StringLiteral>(subExpr) || isa<UserDefinedLiteral>(subExpr)) {
        if (getStringLiteralTokenCount(subExpr, parenExpr) == 1) {
            report(
                DiagnosticsEngine::Warning,
                "unnecessary parentheses around single-token string literal",
                parenExpr->getBeginLoc())
                << parenExpr->getSourceRange();
            handled_.insert(parenExpr);
        }
    } else if (auto const e = dyn_cast<UnaryOperator>(subExpr)) {
        auto const op = e->getOpcode();
        if (op == UO_Plus || op == UO_Minus) {
            auto const e2 = e->getSubExpr();
            if (isa<IntegerLiteral>(e2) || isa<FloatingLiteral>(e2) || isa<ImaginaryLiteral>(e2)) {
                report(
                    DiagnosticsEngine::Warning,
                    "unnecessary parentheses around signed numeric literal",
                    parenExpr->getBeginLoc())
                    << parenExpr->getSourceRange();
                handled_.insert(parenExpr);
            }
        }
    } else if (isa<CXXNamedCastExpr>(subExpr)) {
        if (!removeParens(parenExpr)) {
            report(
                DiagnosticsEngine::Warning, "unnecessary parentheses around cast",
                parenExpr->getBeginLoc())
                << parenExpr->getSourceRange();
        }
        handled_.insert(parenExpr);
    } else if (auto memberExpr = dyn_cast<MemberExpr>(subExpr)) {
        if (isa<CXXThisExpr>(ignoreAllImplicit(memberExpr->getBase()))) {
            report(
                DiagnosticsEngine::Warning, "unnecessary parentheses around member expr",
                parenExpr->getBeginLoc())
                << parenExpr->getSourceRange();
            handled_.insert(parenExpr);
        }
    }

    return true;
}

bool UnnecessaryParen::VisitIfStmt(const IfStmt* ifStmt)
{
    if (auto const cond = ifStmt->getCond()) {
        handleUnreachableCodeConditionParens(cond);
        VisitSomeStmt(ifStmt, cond, "if");
    }
    return true;
}

bool UnnecessaryParen::VisitDoStmt(const DoStmt* doStmt)
{
    VisitSomeStmt(doStmt, doStmt->getCond(), "do");
    return true;
}

bool UnnecessaryParen::VisitWhileStmt(const WhileStmt* whileStmt)
{
    handleUnreachableCodeConditionParens(whileStmt->getCond());
    VisitSomeStmt(whileStmt, whileStmt->getCond(), "while");
    return true;
}

bool UnnecessaryParen::VisitForStmt(ForStmt const * stmt) {
    if (auto const cond = stmt->getCond()) {
        handleUnreachableCodeConditionParens(cond);
    }
    return true;
}

bool UnnecessaryParen::VisitSwitchStmt(const SwitchStmt* switchStmt)
{
    VisitSomeStmt(switchStmt, switchStmt->getCond(), "switch");
    return true;
}

bool UnnecessaryParen::VisitCaseStmt(const CaseStmt* caseStmt)
{
    VisitSomeStmt(caseStmt, caseStmt->getLHS(), "case");
    return true;
}

bool UnnecessaryParen::VisitReturnStmt(const ReturnStmt* returnStmt)
{
    if (ignoreLocation(returnStmt))
        return true;

    if (!returnStmt->getRetValue())
        return true;
    auto parenExpr = dyn_cast<ParenExpr>(ignoreAllImplicit(returnStmt->getRetValue()));
    if (!parenExpr)
        return true;
    if (parenExpr->getBeginLoc().isMacroID())
        return true;
    // assignments need extra parentheses or they generate a compiler warning
    auto binaryOp = dyn_cast<BinaryOperator>(parenExpr->getSubExpr());
    if (binaryOp && binaryOp->getOpcode() == BO_Assign)
        return true;

    // only non-operator-calls for now
    auto subExpr = ignoreAllImplicit(parenExpr->getSubExpr());
    if (isa<CallExpr>(subExpr) && !isa<CXXOperatorCallExpr>(subExpr)
        && !isa<UserDefinedLiteral>(subExpr))
    {
        report(
            DiagnosticsEngine::Warning, "parentheses immediately inside return statement",
            parenExpr->getBeginLoc())
            << parenExpr->getSourceRange();
        handled_.insert(parenExpr);
    }
    return true;
}

void UnnecessaryParen::VisitSomeStmt(const Stmt * stmt, const Expr* cond, StringRef stmtName)
{
    if (ignoreLocation(stmt))
        return;

    auto parenExpr = dyn_cast<ParenExpr>(ignoreAllImplicit(cond));
    if (parenExpr) {
        if (handled_.find(parenExpr) != handled_.end()) {
            return;
        }
        if (parenExpr->getBeginLoc().isMacroID())
            return;
        // assignments need extra parentheses or they generate a compiler warning
        auto binaryOp = dyn_cast<BinaryOperator>(parenExpr->getSubExpr());
        if (binaryOp && binaryOp->getOpcode() == BO_Assign)
            return;
        if (auto const opCall = dyn_cast<CXXOperatorCallExpr>(parenExpr->getSubExpr())) {
            if (opCall->getOperator() == OO_Equal) {
                return;
            }
        }
        report(
            DiagnosticsEngine::Warning, "parentheses immediately inside %0 statement",
            parenExpr->getBeginLoc())
            << stmtName
            << parenExpr->getSourceRange();
        handled_.insert(parenExpr);
    }
}

bool UnnecessaryParen::VisitCallExpr(const CallExpr* callExpr)
{
    if (ignoreLocation(callExpr))
        return true;
    if (callExpr->getNumArgs() == 0 || isa<CXXOperatorCallExpr>(callExpr))
        return true;

    // if we are calling a >1 arg method, are we using the defaults?
    if (callExpr->getNumArgs() > 1)
    {
        if (!isa<CXXDefaultArgExpr>(callExpr->getArg(1)))
            return true;
    }

    auto parenExpr = dyn_cast<ParenExpr>(ignoreAllImplicit(callExpr->getArg(0)));
    if (!parenExpr)
        return true;
    if (parenExpr->getBeginLoc().isMacroID())
        return true;
    // assignments need extra parentheses or they generate a compiler warning
    auto binaryOp = dyn_cast<BinaryOperator>(parenExpr->getSubExpr());
    if (binaryOp && binaryOp->getOpcode() == BO_Assign)
        return true;
    if (getStringLiteralTokenCount(parenExpr->getSubExpr()->IgnoreImplicit(), nullptr) > 1) {
        return true;
    }
    report(
        DiagnosticsEngine::Warning, "parentheses immediately inside single-arg call",
        parenExpr->getBeginLoc())
        << parenExpr->getSourceRange();
    handled_.insert(parenExpr);
    return true;
}

bool UnnecessaryParen::VisitCXXDeleteExpr(const CXXDeleteExpr* deleteExpr)
{
    if (ignoreLocation(deleteExpr))
        return true;

    auto parenExpr = dyn_cast<ParenExpr>(ignoreAllImplicit(deleteExpr->getArgument()));
    if (!parenExpr)
        return true;
    if (parenExpr->getBeginLoc().isMacroID())
        return true;
    // assignments need extra parentheses or they generate a compiler warning
    auto binaryOp = dyn_cast<BinaryOperator>(parenExpr->getSubExpr());
    if (binaryOp && binaryOp->getOpcode() == BO_Assign)
        return true;
    report(
        DiagnosticsEngine::Warning, "parentheses immediately inside delete expr",
        parenExpr->getBeginLoc())
        << parenExpr->getSourceRange();
    handled_.insert(parenExpr);
    return true;
}

bool UnnecessaryParen::VisitCXXOperatorCallExpr(const CXXOperatorCallExpr* callExpr)
{
    if (ignoreLocation(callExpr))
        return true;
    if (callExpr->getNumArgs() != 2)
        return true;

    // Same logic as CXXOperatorCallExpr::isAssignmentOp(), which our supported clang
    // doesn't have yet.
    auto Opc = callExpr->getOperator();
    if (Opc != OO_Equal && Opc != OO_StarEqual &&
        Opc != OO_SlashEqual && Opc != OO_PercentEqual &&
        Opc != OO_PlusEqual && Opc != OO_MinusEqual &&
        Opc != OO_LessLessEqual && Opc != OO_GreaterGreaterEqual &&
        Opc != OO_AmpEqual && Opc != OO_CaretEqual &&
        Opc != OO_PipeEqual)
        return true;
    auto parenExpr = dyn_cast<ParenExpr>(ignoreAllImplicit(callExpr->getArg(1)));
    if (!parenExpr)
        return true;
    if (parenExpr->getBeginLoc().isMacroID())
        return true;
    // Sometimes parentheses make the RHS of an assignment easier to read by
    // visually disambiguating the = from a call to ==
    auto sub = parenExpr->getSubExpr();
    if (auto const e = dyn_cast<CXXRewrittenBinaryOperator>(sub)) {
        if (isParenWorthyOpcode(e->getDecomposedForm().Opcode)) {
            return true;
        }
    }
    if (auto subBinOp = dyn_cast<BinaryOperator>(sub))
    {
        if (isParenWorthyOpcode(subBinOp->getOpcode()))
            return true;
    }
    if (auto subOperatorCall = dyn_cast<CXXOperatorCallExpr>(sub))
    {
        auto op = subOperatorCall->getOperator();
        if (!((op >= OO_Plus && op <= OO_Exclaim) || (op >= OO_ArrowStar && op <= OO_Subscript)))
            return true;
    }
    if (isa<ConditionalOperator>(sub))
        return true;

    report(
        DiagnosticsEngine::Warning, "parentheses immediately inside assignment",
        parenExpr->getBeginLoc())
        << parenExpr->getSourceRange();
    handled_.insert(parenExpr);
    return true;
}

bool UnnecessaryParen::VisitVarDecl(const VarDecl* varDecl)
{
    if (ignoreLocation(varDecl))
        return true;
    if (!varDecl->getInit())
        return true;

    auto parenExpr = dyn_cast<ParenExpr>(ignoreAllImplicit(varDecl->getInit()));
    if (!parenExpr)
        return true;
    if (parenExpr->getBeginLoc().isMacroID())
        return true;

    // Sometimes parentheses make the RHS of an assignment easier to read by
    // visually disambiguating the = from a call to ==
    auto sub = parenExpr->getSubExpr();
    if (auto const e = dyn_cast<CXXRewrittenBinaryOperator>(sub)) {
        sub = e->getDecomposedForm().InnerBinOp;
    }
    if (auto subBinOp = dyn_cast<BinaryOperator>(sub))
    {
        if (!(subBinOp->isMultiplicativeOp() || subBinOp->isAdditiveOp() || subBinOp->isPtrMemOp()))
            return true;
    }
    if (auto subOperatorCall = dyn_cast<CXXOperatorCallExpr>(sub))
    {
        auto op = subOperatorCall->getOperator();
        if (!((op >= OO_Plus && op <= OO_Exclaim) || (op >= OO_ArrowStar && op <= OO_Subscript)))
            return true;
    }
    if (isa<ConditionalOperator>(sub))
        return true;

    // these two are for "parentheses were disambiguated as a function declaration [-Werror,-Wvexing-parse]"
    auto const sub2 = sub->IgnoreImplicit();
    if (isa<CXXTemporaryObjectExpr>(sub2)
        || isa<CXXFunctionalCastExpr>(sub2))
        return true;

    report(
        DiagnosticsEngine::Warning, "parentheses immediately inside vardecl statement",
        parenExpr->getBeginLoc())
        << parenExpr->getSourceRange();
    handled_.insert(parenExpr);
    return true;
}

bool UnnecessaryParen::VisitMemberExpr(const MemberExpr* memberExpr)
{
    if (ignoreLocation(memberExpr))
        return true;

    auto parenExpr = dyn_cast<ParenExpr>(ignoreAllImplicit(memberExpr->getBase()));
    if (!parenExpr)
        return true;
    if (handled_.find(parenExpr) != handled_.end())
        return true;
    if (parenExpr->getBeginLoc().isMacroID())
        return true;

    auto sub = parenExpr->getSubExpr();
    if (isa<CallExpr>(sub)) {
        if (isa<CXXOperatorCallExpr>(sub))
           return true;
    } else if (isa<CXXConstructExpr>(sub)) {
        // warn
    } else if (isa<MemberExpr>(sub)) {
        // warn
    } else if (isa<DeclRefExpr>(sub)) {
        // warn
    } else
        return true;

    report(
        DiagnosticsEngine::Warning, "unnecessary parentheses around member expr",
        parenExpr->getBeginLoc())
        << parenExpr->getSourceRange();
    handled_.insert(parenExpr);
    return true;
}

// Conservatively assume any parenthesised integer or Boolean (incl. Objective-C ones) literal in
// certain condition expressions (i.e., those for which handleUnreachableCodeConditionParens is
// called) to be parenthesised to silence Clang -Wunreachable-code, if that is either the whole
// condition expression or appears as a certain sub-expression (looking at what isConfigurationValue
// in Clang's lib/Analysis/ReachableCode.cpp looks for, descending into certain unary and binary
// operators):
void UnnecessaryParen::handleUnreachableCodeConditionParens(Expr const * expr) {
    auto const e = ignoreAllImplicit(expr);
    if (auto const e1 = dyn_cast<ParenExpr>(e)) {
        auto const sub = e1->getSubExpr();
        if (isa<IntegerLiteral>(sub) || isa<CXXBoolLiteralExpr>(sub)
            || isa<ObjCBoolLiteralExpr>(sub))
        {
            handled_.insert(e1);
        }
    } else if (auto const e1 = dyn_cast<UnaryOperator>(e)) {
        if (e1->getOpcode() == UO_LNot) {
            handleUnreachableCodeConditionParens(e1->getSubExpr());
        }
    } else if (auto const e1 = dyn_cast<BinaryOperator>(e)) {
        if (e1->isLogicalOp() || e1->isComparisonOp()) {
            handleUnreachableCodeConditionParens(e1->getLHS());
            handleUnreachableCodeConditionParens(e1->getRHS());
        }
    }
}

bool UnnecessaryParen::isPrecededBy_BAD_CAST(Expr const * expr) {
    if (expr->getBeginLoc().isMacroID()) {
        return false;
    }
    SourceManager& SM = compiler.getSourceManager();
    const char *p1 = SM.getCharacterData( expr->getBeginLoc().getLocWithOffset(-10) );
    const char *p2 = SM.getCharacterData( expr->getBeginLoc() );
    return std::string(p1, p2 - p1).find("BAD_CAST") != std::string::npos;
}

namespace {

bool badCombinationChar(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_'
        || c == '+' || c == '-' || c == '\'' || c == '"';
}

}

bool UnnecessaryParen::badCombination(SourceLocation loc, int prevOffset, int nextOffset) {
    //TODO: check for start/end of file; take backslash-newline line concatenation into account
    auto const c1
        = compiler.getSourceManager().getCharacterData(loc.getLocWithOffset(prevOffset))[0];
    auto const c2
        = compiler.getSourceManager().getCharacterData(loc.getLocWithOffset(nextOffset))[0];
    // An approximation of avoiding whatever combinations that would cause two adjacent tokens to be
    // lexed differently, using, for now, letters (TODO: non-ASCII ones) and digits and '_'; '+' and
    // '-' (to avoid ++, etc.); '\'' and '"' (to avoid u'x' or "foo"bar, etc.):
    return badCombinationChar(c1) && badCombinationChar(c2);
}

bool UnnecessaryParen::removeParens(ParenExpr const * expr) {
    if (rewriter == nullptr) {
        return false;
    }
    auto const firstBegin = expr->getBeginLoc();
    auto secondBegin = expr->getEndLoc();
    if (firstBegin.isMacroID() || secondBegin.isMacroID()) {
        return false;
    }
    unsigned firstLen = Lexer::MeasureTokenLength(
        firstBegin, compiler.getSourceManager(), compiler.getLangOpts());
    for (auto l = firstBegin.getLocWithOffset(std::max<unsigned>(firstLen, 1));;
         l = l.getLocWithOffset(1))
    {
        unsigned n = Lexer::MeasureTokenLength(
            l, compiler.getSourceManager(), compiler.getLangOpts());
        if (n != 0) {
            break;
        }
        ++firstLen;
    }
    unsigned secondLen = Lexer::MeasureTokenLength(
        secondBegin, compiler.getSourceManager(), compiler.getLangOpts());
    for (;;) {
        auto l = secondBegin.getLocWithOffset(-1);
        auto const c = compiler.getSourceManager().getCharacterData(l)[0];
        if (c == '\n') {
            if (compiler.getSourceManager().getCharacterData(l.getLocWithOffset(-1))[0] == '\\') {
                break;
            }
        } else if (!(c == ' ' || c == '\t' || c == '\v' || c == '\f')) {
            break;
        }
        secondBegin = l;
        ++secondLen;
    }
    if (!replaceText(firstBegin, firstLen, badCombination(firstBegin, -1, firstLen) ? " " : "")) {
        if (isDebugMode()) {
            report(
                DiagnosticsEngine::Fatal,
                "TODO: cannot rewrite opening parenthesis, needs investigation",
                firstBegin);
            report(
                DiagnosticsEngine::Note, "when removing these parentheses", expr->getExprLoc())
                << expr->getSourceRange();
        }
        return false;
    }
    if (!replaceText(secondBegin, secondLen, badCombination(secondBegin, -1, secondLen) ? " " : ""))
    {
        //TODO: roll back first change
        if (isDebugMode()) {
            report(
                DiagnosticsEngine::Fatal,
                "TODO: cannot rewrite closing parenthesis, needs investigation",
                secondBegin);
            report(
                DiagnosticsEngine::Note, "when removing these parentheses", expr->getExprLoc())
                << expr->getSourceRange();
        }
        return false;
    }
    return true;
}

loplugin::Plugin::Registration< UnnecessaryParen > unnecessaryparen("unnecessaryparen", true);

}

#endif // LO_CLANG_SHARED_PLUGINS

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
