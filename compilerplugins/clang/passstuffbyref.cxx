/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <string>
#include <set>
#include <iostream>

#include "check.hxx"
#include "plugin.hxx"

// Find places where various things are passed by value.
// It's not very efficient, because we generally end up copying it twice - once into the parameter and
// again into the destination.
// They should rather be passed by reference.
//
// Generally recommending lambda capture by-ref rather than by-copy is even more
// problematic than with function parameters, as a lambda instance can easily
// outlive a referenced variable. So once lambdas start to get used in more
// sophisticated ways than passing them into standard algorithms, this plugin's
// advice, at least for explicit captures, will need to be revisited.

namespace {

class PassStuffByRef:
    public loplugin::FilteringPlugin<PassStuffByRef>
{
public:
    explicit PassStuffByRef(loplugin::InstantiationData const & data): FilteringPlugin(data), mbInsideFunctionDecl(false), mbFoundReturnValueDisqualifier(false) {}

    virtual void run() override { TraverseDecl(compiler.getASTContext().getTranslationUnitDecl()); }

    // When warning about function params of primitive type that could be passed
    // by value instead of by reference, make sure not to warn if the parameter
    // is ever bound to a reference; on the one hand, this needs scaffolding in
    // all Traverse*Decl functions (indirectly) derived from FunctionDecl; and
    // on the other hand, use a hack of ignoring just the DeclRefExprs nested in
    // LValueToRValue ImplicitCastExprs when determining whether a param is
    // bound to a reference:
    bool TraverseFunctionDecl(FunctionDecl * decl);
    bool TraverseCXXMethodDecl(CXXMethodDecl * decl);
    bool TraverseCXXConstructorDecl(CXXConstructorDecl * decl);
    bool TraverseCXXDestructorDecl(CXXDestructorDecl * decl);
    bool TraverseCXXConversionDecl(CXXConversionDecl * decl);
    bool VisitFunctionDecl(const FunctionDecl * decl);
    bool TraverseImplicitCastExpr(ImplicitCastExpr * expr);
    bool VisitDeclRefExpr(const DeclRefExpr * expr);

    bool VisitReturnStmt(const ReturnStmt * );
    bool VisitVarDecl(const VarDecl * );

private:
    template<typename T> bool traverseAnyFunctionDecl(
        T * decl, bool (RecursiveASTVisitor::* fn)(T *));
    void checkParams(const FunctionDecl * functionDecl);
    void checkReturnValue(const FunctionDecl * functionDecl, const CXXMethodDecl * methodDecl);
    bool isPrimitiveConstRef(QualType type);
    bool isReturnExprDisqualified(const Expr* expr);

    bool mbInsideFunctionDecl;
    bool mbFoundReturnValueDisqualifier;

    struct FDecl {
        std::set<ParmVarDecl const *> parms;
        bool check = false;
    };
    std::vector<FDecl> functionDecls_;
};

bool PassStuffByRef::TraverseFunctionDecl(FunctionDecl * decl) {
    return traverseAnyFunctionDecl(
        decl, &RecursiveASTVisitor::TraverseFunctionDecl);
}

bool PassStuffByRef::TraverseCXXMethodDecl(CXXMethodDecl * decl) {
    return traverseAnyFunctionDecl(
        decl, &RecursiveASTVisitor::TraverseCXXMethodDecl);
}

bool PassStuffByRef::TraverseCXXConstructorDecl(CXXConstructorDecl * decl) {
    return traverseAnyFunctionDecl(
        decl, &RecursiveASTVisitor::TraverseCXXConstructorDecl);
}

bool PassStuffByRef::TraverseCXXDestructorDecl(CXXDestructorDecl * decl) {
    return traverseAnyFunctionDecl(
        decl, &RecursiveASTVisitor::TraverseCXXDestructorDecl);
}

bool PassStuffByRef::TraverseCXXConversionDecl(CXXConversionDecl * decl) {
    return traverseAnyFunctionDecl(
        decl, &RecursiveASTVisitor::TraverseCXXConversionDecl);
}

template<typename T> bool PassStuffByRef::traverseAnyFunctionDecl(
    T * decl, bool (RecursiveASTVisitor::* fn)(T *))
{
    if (ignoreLocation(decl)) {
        return true;
    }
    if (decl->doesThisDeclarationHaveABody()) {
        functionDecls_.emplace_back();
    }
    bool ret = (this->*fn)(decl);
    if (decl->doesThisDeclarationHaveABody()) {
        assert(!functionDecls_.empty());
        if (functionDecls_.back().check) {
            for (auto d: functionDecls_.back().parms) {
                report(
                    DiagnosticsEngine::Warning,
                    ("passing primitive type param %0 by const &, rather pass"
                     " by value"),
                    d->getLocation())
                    << d->getType() << d->getSourceRange();
                auto can = decl->getCanonicalDecl();
                if (can->getLocation() != decl->getLocation()) {
                    report(
                        DiagnosticsEngine::Note, "function is declared here:",
                        can->getLocation())
                        << can->getSourceRange();
                }
            }
        }
        functionDecls_.pop_back();
    }
    return ret;
}

bool PassStuffByRef::VisitFunctionDecl(const FunctionDecl * functionDecl) {
    if (ignoreLocation(functionDecl)) {
        return true;
    }
    if (functionDecl->isDeleted()
        || functionDecl->isFunctionTemplateSpecialization())
    {
        return true;
    }
    // only consider base declarations, not overridden ones, or we warn on methods that
    // are overriding stuff from external libraries
    const CXXMethodDecl * methodDecl = dyn_cast<CXXMethodDecl>(functionDecl);
    if (methodDecl && methodDecl->size_overridden_methods() > 0) {
            return true;
    }

    checkParams(functionDecl);
    checkReturnValue(functionDecl, methodDecl);
    return true;
}

bool PassStuffByRef::TraverseImplicitCastExpr(ImplicitCastExpr * expr) {
    if (ignoreLocation(expr)) {
        return true;
    }
    return
        (expr->getCastKind() == CK_LValueToRValue
         && (dyn_cast<DeclRefExpr>(expr->getSubExpr()->IgnoreParenImpCasts())
             != nullptr))
        || RecursiveASTVisitor::TraverseImplicitCastExpr(expr);
}

bool PassStuffByRef::VisitDeclRefExpr(const DeclRefExpr * expr) {
    if (ignoreLocation(expr)) {
        return true;
    }
    auto d = dyn_cast<ParmVarDecl>(expr->getDecl());
    if (d == nullptr) {
        return true;
    }
    for (auto & fd: functionDecls_) {
        if (fd.parms.erase(d) == 1) {
            break;
        }
    }
    return true;
}

void PassStuffByRef::checkParams(const FunctionDecl * functionDecl) {
    // Only warn on the definition of the function:
    if (!functionDecl->doesThisDeclarationHaveABody()) {
        return;
    }
    // ignore stuff that forms part of the stable URE interface
    if (isInUnoIncludeFile(functionDecl)) {
        return;
    }
    // these functions are passed as parameters to another function
    auto dc = loplugin::DeclCheck(functionDecl);
    if (dc.MemberFunction()
        .Class("ShapeAttributeLayer").Namespace("internal")
        .Namespace("slideshow").GlobalNamespace())
    {
        return;
    }
    // not sure why, but changing these causes problems
    if (dc.Function("lcl_createColorMapFromShapeProps")
        || dc.Function("getAPIAnglesFrom3DProperties").Class("Scene3DHelper")
        || dc.Function("FillSeriesSimple").Class("ScTable")
        || dc.Function("FillAutoSimple").Class("ScTable"))
        return;

    unsigned n = functionDecl->getNumParams();
    assert(!functionDecls_.empty());
    functionDecls_.back().check = true;
    for (unsigned i = 0; i != n; ++i) {
        const ParmVarDecl * pvDecl = functionDecl->getParamDecl(i);
        auto const t = pvDecl->getType();
        if (isPrimitiveConstRef(t)) {
            functionDecls_.back().parms.insert(pvDecl);
        }
    }
}

static bool startswith(const std::string& rStr, const char* pSubStr) {
    return rStr.compare(0, strlen(pSubStr), pSubStr) == 0;
}

void PassStuffByRef::checkReturnValue(const FunctionDecl * functionDecl, const CXXMethodDecl * methodDecl) {
    if (methodDecl && (methodDecl->isVirtual() || methodDecl->hasAttr<OverrideAttr>())) {
        return;
    }
    if( !functionDecl->doesThisDeclarationHaveABody()
        || functionDecl->isLateTemplateParsed())
    {
        return;
    }

    const QualType type = functionDecl->getReturnType().getDesugaredType(compiler.getASTContext());
    if (type->isReferenceType() || type->isIntegralOrEnumerationType() || type->isPointerType()
        || type->isTemplateTypeParmType() || type->isDependentType() || type->isBuiltinType()
        || type->isScalarType())
    {
        return;
    }

    // not sure if it's possible to modify these
    if (isa<CXXConversionDecl>(functionDecl))
        return;

    // ignore stuff that forms part of the stable URE interface
    if (isInUnoIncludeFile(functionDecl)) {
        return;
    }

    loplugin::DeclCheck dc(functionDecl);
    // function is passed as parameter to another function
    if (dc.Function("ImplColMonoFnc").Class("GDIMetaFile").GlobalNamespace()
        || (dc.Function("darkColor").Class("SvxBorderLine").Namespace("editeng")
            .GlobalNamespace())
        || (dc.MemberFunction().Class("Binding").Namespace("xforms")
            .GlobalNamespace())
        || (dc.MemberFunction().Class("Model").Namespace("xforms")
            .GlobalNamespace())
        || (dc.MemberFunction().Class("Submission").Namespace("xforms")
            .GlobalNamespace())
        || (dc.Function("TopLeft").Class("SwRect").GlobalNamespace())
        || (dc.Function("ConvDicList_CreateInstance").GlobalNamespace())
        || (dc.Function("Create").Class("OUnoAutoPilot").Namespace("dbp").GlobalNamespace())
        || (dc.Function("Size_").Class("SwRect").GlobalNamespace()))
    {
        return;
    }
    // not sure how to exclude this yet, returns copy of one of it's params
    if (dc.Function("sameDistColor").GlobalNamespace()
        || dc.Function("sameColor").GlobalNamespace()
        || (dc.Operator(OO_Call).Struct("StringIdentity").AnonymousNamespace()
            .Namespace("pcr").GlobalNamespace())
        || (dc.Function("accumulate").Namespace("internal")
            .Namespace("slideshow").GlobalNamespace())
        || (dc.Function("lerp").Namespace("internal").Namespace("slideshow")
            .GlobalNamespace()))
        return;
    // depends on a define
    if (dc.Function("GetSharedFileURL").Class("SfxObjectShell")
        .GlobalNamespace()) {
        return;
    }
    // hides a constructor
    if (dc.Function("createNonOwningCopy").Class("SortedAutoCompleteStrings").Namespace("editeng")
        .GlobalNamespace()) {
        return;
    }
    // template function
    if (dc.Function("convertItems").Class("ValueParser").Namespace("configmgr").GlobalNamespace()
        || dc.Function("parseListValue").AnonymousNamespace().Namespace("configmgr").GlobalNamespace()
        || dc.Function("parseSingleValue").AnonymousNamespace().Namespace("configmgr").GlobalNamespace()
        || dc.Function("Create").Class("HandlerComponentBase").Namespace("pcr").GlobalNamespace()
        || dc.Function("toAny").Struct("Convert").Namespace("detail").Namespace("comphelper").GlobalNamespace()
        || dc.Function("makeAny").Namespace("utl").GlobalNamespace()
        || dc.Operator(OO_Call).Struct("makeAny") // in chart2
        || dc.Function("toString").Struct("assertion_traits"))
        return;
    // these are sometimes dummy classes
    if (dc.MemberFunction().Class("DdeService")
        || dc.MemberFunction().Class("DdeConnection")
        || dc.MemberFunction().Class("DdeTopic")
        || dc.MemberFunction().Class("CrashReporter"))
        return;
    // function has its address taken and is used as a function pointer value
    if (dc.Function("xforms_bool"))
        return;
    if (startswith(type.getAsString(), "struct o3tl::strong_int"))
        return;
    // false positive
    if (dc.Function("FindFieldSep").Namespace("mark").Namespace("sw"))
        return;
    // false positive
    if (dc.Function("GetTime").Class("SwDateTimeField"))
        return;
    // false positive
    if (dc.Function("Create").Class("StyleFamilyEntry"))
        return;
    if (dc.Function("getManualMax").Class("SparklineAttributes"))
        return;
    if (dc.Function("getManualMin").Class("SparklineAttributes"))
        return;
    if (dc.Function("ReadEmbeddedData").Class("XclImpHyperlink"))
        return;

    auto tc = loplugin::TypeCheck(functionDecl->getReturnType());

    // these functions are passed by function-pointer
    if (functionDecl->getIdentifier() && functionDecl->getName() == "GetRanges"
        && tc.Struct("WhichRangesContainer").GlobalNamespace())
        return;

    // extremely simple classes, might as well pass by value
    if (tc.Class("Color")
        || tc.Struct("TranslateId")
        || tc.Class("span").StdNamespace()
        || tc.Class("strong_ordering").StdNamespace()
        || tc.Struct("TokenId"))
        return;

    // functionDecl->dump();

    mbInsideFunctionDecl = true;
    mbFoundReturnValueDisqualifier = false;
    TraverseStmt(functionDecl->getBody());
    mbInsideFunctionDecl = false;

    if (mbFoundReturnValueDisqualifier)
        return;

    report( DiagnosticsEngine::Warning,
            "rather return %0 by const& than by value, to avoid unnecessary copying",
            functionDecl->getSourceRange().getBegin())
        << type.getAsString();

    // display the location of the class member declaration so I don't have to search for it by hand
    auto canonicalDecl = functionDecl->getCanonicalDecl();
    if (functionDecl != canonicalDecl)
    {
        report( DiagnosticsEngine::Note,
                "decl here",
                canonicalDecl->getSourceRange().getBegin())
            << canonicalDecl->getSourceRange();
    }

    //functionDecl->dump();
}

bool PassStuffByRef::VisitReturnStmt(const ReturnStmt * returnStmt)
{
    if (!mbInsideFunctionDecl)
        return true;
    if (returnStmt->child_begin() == returnStmt->child_end())
        return true;
    auto expr = dyn_cast<Expr>(*returnStmt->child_begin());
    if (!expr)
        return true;
    expr = expr->IgnoreParenImpCasts();

    if (isReturnExprDisqualified(expr))
        mbFoundReturnValueDisqualifier = true;

    return true;
}

/**
 * Does a return expression disqualify this method from doing return by const & ?
 */
bool PassStuffByRef::isReturnExprDisqualified(const Expr* expr)
{
    while (true)
    {
        expr = expr->IgnoreParens();
//        expr->dump();
        if (auto implicitCast = dyn_cast<ImplicitCastExpr>(expr)) {
            expr = implicitCast->getSubExpr();
            continue;
        }
        if (auto exprWithCleanups = dyn_cast<ExprWithCleanups>(expr)) {
            expr = exprWithCleanups->getSubExpr();
            continue;
        }
        if (auto constructExpr = dyn_cast<CXXConstructExpr>(expr))
        {
            if (constructExpr->getNumArgs()==1
                && constructExpr->getConstructor()->isCopyOrMoveConstructor())
            {
                expr = constructExpr->getArg(0);
                continue;
            }
            else
                return true;
        }
        if (isa<CXXFunctionalCastExpr>(expr)) {
            return true;
        }
        if (isa<MaterializeTemporaryExpr>(expr)) {
            return true;
        }
        if (auto bindExpr = dyn_cast<CXXBindTemporaryExpr>(expr)) {
            // treat "return OUString();" as fine, because that is easy to convert
            // to use EMPTY_OUSTRING.
            if (loplugin::TypeCheck(expr->getType()).Class("OUString"))
                if (auto tempExpr = dyn_cast<CXXTemporaryObjectExpr>(bindExpr->getSubExpr()))
                    if (tempExpr->getNumArgs() == 0)
                        return false;
            return true;
        }
        if (isa<InitListExpr>(expr)) {
            return true;
        }
        expr = expr->IgnoreParenCasts();
        if (auto childExpr = dyn_cast<ArraySubscriptExpr>(expr)) {
            expr = childExpr->getLHS();
            continue;
        }
        if (auto memberExpr = dyn_cast<MemberExpr>(expr)) {
            expr = memberExpr->getBase();
            continue;
        }
        if (auto declRef = dyn_cast<DeclRefExpr>(expr)) {
            // a param might be a temporary
            if (isa<ParmVarDecl>(declRef->getDecl()))
                return true;
            const VarDecl* varDecl = dyn_cast<VarDecl>(declRef->getDecl());
            if (varDecl) {
                if (varDecl->getStorageDuration() == SD_Thread
                    || varDecl->getStorageDuration() == SD_Static ) {
                    return false;
                }
                return true;
            }
        }
        if (auto condOper = dyn_cast<ConditionalOperator>(expr)) {
            return isReturnExprDisqualified(condOper->getTrueExpr())
                || isReturnExprDisqualified(condOper->getFalseExpr());
        }
        if (auto unaryOp = dyn_cast<UnaryOperator>(expr)) {
            expr = unaryOp->getSubExpr();
            continue;
        }
        if (auto operatorCallExpr = dyn_cast<CXXOperatorCallExpr>(expr)) {
            // TODO could improve this, but sometimes it means we're returning a copy of a temporary.
            // Same logic as CXXOperatorCallExpr::isAssignmentOp(), which our supported clang
            // doesn't have yet.
            auto Opc = operatorCallExpr->getOperator();
            if (Opc == OO_Equal || Opc == OO_StarEqual ||
                Opc == OO_SlashEqual || Opc == OO_PercentEqual ||
                Opc == OO_PlusEqual || Opc == OO_MinusEqual ||
                Opc == OO_LessLessEqual || Opc == OO_GreaterGreaterEqual ||
                Opc == OO_AmpEqual || Opc == OO_CaretEqual ||
                Opc == OO_PipeEqual)
                return true;
            if (Opc == OO_Subscript)
            {
                if (isReturnExprDisqualified(operatorCallExpr->getArg(0)))
                    return true;
                // otherwise fall through to the checking below
            }
            if (Opc == OO_Arrow)
                if (isReturnExprDisqualified(operatorCallExpr->getArg(0)))
                    return true;
        }
        if (auto memberCallExpr = dyn_cast<CXXMemberCallExpr>(expr)) {
            if (isReturnExprDisqualified(memberCallExpr->getImplicitObjectArgument()))
                return true;
            // otherwise fall through to the checking in CallExpr
        }
        if (auto callExpr = dyn_cast<CallExpr>(expr)) {
            FunctionDecl const * calleeFunctionDecl = callExpr->getDirectCallee();
            if (!calleeFunctionDecl)
                return true;
            // TODO anything takes a non-integral param is suspect because it might return the param by ref.
            // we could tighten this to only reject functions that have a param of the same type
            // as the return type. Or we could check for such functions and disallow them.
            // Or we could force such functions to be annotated somehow.
            for (unsigned i = 0; i != calleeFunctionDecl->getNumParams(); ++i) {
                if (!calleeFunctionDecl->getParamDecl(i)->getType()->isIntegralOrEnumerationType())
                    return true;
            }
            auto tc = loplugin::TypeCheck(calleeFunctionDecl->getReturnType());
            if (!tc.LvalueReference() && !tc.Pointer())
                return true;
        }
        return false;
    }
}

bool PassStuffByRef::VisitVarDecl(const VarDecl * varDecl)
{
    if (!mbInsideFunctionDecl || mbFoundReturnValueDisqualifier)
        return true;
    // things guarded by locking are probably best left alone
    loplugin::TypeCheck dc(varDecl->getType());
    if (dc.Class("Guard").Namespace("osl").GlobalNamespace() ||
        dc.Class("ClearableGuard").Namespace("osl").GlobalNamespace() ||
        dc.Class("ResettableGuard").Namespace("osl").GlobalNamespace() ||
        dc.Class("SolarMutexGuard").GlobalNamespace() ||
        dc.Class("SfxModelGuard").GlobalNamespace() ||
        dc.Class("ReadWriteGuard").Namespace("utl").GlobalNamespace() ||
        dc.Class("LifeTimeGuard").Namespace("apphelper").GlobalNamespace() || // in chart2
        dc.Class("unique_lock").StdNamespace() ||
        dc.Class("lock_guard").StdNamespace() ||
        dc.Class("scoped_lock").StdNamespace())
        mbFoundReturnValueDisqualifier = true;
    return true;
}

bool PassStuffByRef::isPrimitiveConstRef(QualType type) {
    if (type->isIncompleteType()) {
        return false;
    }
    const clang::ReferenceType* referenceType = type->getAs<ReferenceType>();
    if (referenceType == nullptr) {
        return false;
    }
    QualType pointeeQT = referenceType->getPointeeType();
    if (!pointeeQT.isConstQualified()) {
        return false;
    }
    if (!pointeeQT->isFundamentalType()) {
        return false;
    }
    // ignore double for now, some of our code seems to believe it is cheaper to pass by ref
    const BuiltinType* builtinType = pointeeQT->getAs<BuiltinType>();
    return builtinType->getKind() != BuiltinType::Kind::Double;
}


loplugin::Plugin::Registration< PassStuffByRef > X("passstuffbyref", false);

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
