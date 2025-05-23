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

#include "check.hxx"
#include "plugin.hxx"

namespace {

class BadStatics
    : public loplugin::FilteringPlugin<BadStatics>
{

public:
    explicit BadStatics(loplugin::InstantiationData const& rData):
        FilteringPlugin(rData) {}

    bool preRun() override {
        return compiler.getLangOpts().CPlusPlus; // no non-trivial dtors in C
    }

    void run() override {
        if (preRun()) {
            TraverseDecl(compiler.getASTContext().getTranslationUnitDecl());
        }
    }

    static std::pair<bool, std::vector<FieldDecl const*>> isBadStaticType(
            QualType const& rpType, std::vector<FieldDecl const*> & chain,
            std::vector<QualType> const& rParents)
    {
        QualType pt;
        if (rpType->isAnyPointerType()) {
            pt = rpType->getPointeeType();
        } else if (auto at = rpType->getAsArrayTypeUnsafe()) {
            pt = at->getElementType();
        } else if (auto rt = rpType->getAs<ReferenceType>()) {
            pt = rt->getPointeeType();
        }
        if (!pt.isNull()) {
            QualType const pPointee(pt.getUnqualifiedType().getCanonicalType());
            auto const iter(std::find(rParents.begin(), rParents.end(), pPointee));
            if (iter == rParents.end())
            {
                std::vector<QualType> copy(rParents);
                copy.push_back(rpType.getUnqualifiedType().getCanonicalType());
                return isBadStaticType(pt, chain, copy);
            } else {
                return std::make_pair(false, std::vector<FieldDecl const*>());
            }
        }
        RecordType const*const pRecordType(rpType->getAs<RecordType>());
        if (!pRecordType) {
            return std::make_pair(false, std::vector<FieldDecl const*>());
        }
        auto const type = loplugin::TypeCheck(rpType);
        if (   type.Class("Image").GlobalNamespace()
            || type.Class("Bitmap").GlobalNamespace()
            || type.Class("BitmapEx").GlobalNamespace()
            || type.Class("VclPtr").GlobalNamespace()
           )
        {
            return std::make_pair(true, chain);
        }
        if (type.Class("array").StdNamespace()
            || type.Class("deque").StdNamespace()
            || type.Class("forward_list").StdNamespace()
            || type.Class("initializer_list").StdNamespace()
            || type.Class("list").StdNamespace()
            || type.Class("multiset").StdNamespace()
            || type.Class("set").StdNamespace()
            || type.Class("unordered_multiset").StdNamespace()
            || type.Class("unordered_set").StdNamespace()
            || type.Class("vector").StdNamespace())
        {
            std::vector<QualType> copy(rParents);
            copy.push_back(rpType.getUnqualifiedType().getCanonicalType());
            auto ctsd = dyn_cast<ClassTemplateSpecializationDecl>(
                pRecordType->getDecl());
            assert(ctsd != nullptr);
            auto const & args = ctsd->getTemplateArgs();
            assert(args.size() >= 1);
            return isBadStaticType(args.get(0).getAsType(), chain, copy);
        }
        if (type.Class("map").StdNamespace()
            || type.Class("multimap").StdNamespace()
            || type.Class("unordered_map").StdNamespace()
            || type.Class("unordered_multimap").StdNamespace())
        {
            std::vector<QualType> copy(rParents);
            copy.push_back(rpType.getUnqualifiedType().getCanonicalType());
            auto ctsd = dyn_cast<ClassTemplateSpecializationDecl>(
                pRecordType->getDecl());
            assert(ctsd != nullptr);
            auto const & args = ctsd->getTemplateArgs();
            assert(args.size() >= 2);
            auto ret = isBadStaticType(args.get(0).getAsType(), chain, copy);
            if (ret.first) {
                return ret;
            }
            return isBadStaticType(args.get(1).getAsType(), chain, copy);
        }
        RecordDecl const*const pDefinition(pRecordType->getDecl()->getDefinition());
        if (!pDefinition) { // maybe no definition if it's a pointer/reference
            return std::make_pair(false, std::vector<FieldDecl const*>());
        }
        if (   type.Class("DeleteOnDeinit").Namespace("tools").GlobalNamespace()
            || type.Class("weak_ptr").StdNamespace() // not owning
            || type.Class("ImplWallpaper").GlobalNamespace() // very odd static instance here
            || type.Class("Application").GlobalNamespace() // numerous odd subclasses in vclmain::createApplication()
            || type.Class("DemoMtfApp").AnonymousNamespace().GlobalNamespace() // one of these Application with own VclPtr
           )
        {
            return std::make_pair(false, std::vector<FieldDecl const*>());
        }
        std::vector<QualType> copy(rParents);
        copy.push_back(rpType.getUnqualifiedType().getCanonicalType());
        CXXRecordDecl const*const pDecl(dyn_cast<CXXRecordDecl>(pDefinition));
        assert(pDecl);
        for (auto it = pDecl->field_begin(); it != pDecl->field_end(); ++it) {
            chain.push_back(*it);
            auto const ret(isBadStaticType((*it)->getType(), chain, copy));
            chain.pop_back();
            if (ret.first) {
                return ret;
            }
        }
        for (auto it = pDecl->bases_begin(); it != pDecl->bases_end(); ++it) {
            auto const ret(isBadStaticType((*it).getType(), chain, copy));
            if (ret.first) {
                return ret;
            }
        }
        for (auto it = pDecl->vbases_begin(); it != pDecl->vbases_end(); ++it) {
            auto const ret(isBadStaticType((*it).getType(), chain, copy));
            if (ret.first) {
                return ret;
            }
        }
        return std::make_pair(false, std::vector<FieldDecl const*>());
    }

    bool VisitVarDecl(VarDecl const*const pVarDecl)
    {
        if (ignoreLocation(pVarDecl)) {
            return true;
        }

        if (pVarDecl->hasGlobalStorage()
            && pVarDecl->isThisDeclarationADefinition())
        {
            auto const name(pVarDecl->getName());
            if (   name == "s_pPreviousView" // not an owning pointer
                || name == "s_pDefCollapsed" // SvImpLBox::~SvImpLBox()
                || name == "s_pDefExpanded"  // SvImpLBox::~SvImpLBox()
                || name == "g_pDDSource" // SvTreeListBox::dispose()
                || name == "g_pDDTarget" // SvTreeListBox::dispose()
                || name == "g_pSfxApplication" // SfxApplication::~SfxApplication()
                || name == "s_SidebarResourceManagerInstance" // ResourceManager::disposeDecks()
                || name == "s_pGallery" // this is not entirely clear but apparently the GalleryThemeCacheEntry are deleted by GalleryBrowser2::SelectTheme() or GalleryBrowser2::dispose()
                || name == "s_ExtMgr" // TheExtensionManager::disposing()
                || name == "s_pDocLockedInsertingLinks" // not owning
                || name == "s_pVout" // FrameFinit()
                || name == "s_pPaintQueue" // SwPaintQueue::Remove()
                || name == "gProp" // only owned (VclPtr) member cleared again
                || name == "g_OszCtrl" // SwCrsrOszControl::Exit()
                || name == "g_pSpellIter" // SwEditShell::SpellEnd()
                || name == "g_pConvIter" // SwEditShell::SpellEnd()
                || name == "g_pHyphIter" // SwEditShell::HyphEnd()
                || name == "xFieldEditEngine" // ScGlobal::Clear()
                || name == "xDrawClipDocShellRef" // ScGlobal::Clear()
                || name == "s_ImageTree"
                    // ImageTree::get(), ImageTree::shutDown()
                || name == "s_pMouseFrame"
                    // vcl/osx/salframeview.mm, mouseEntered/Exited, not owning
                || name == "pCurrentMenuBar"
                    // vcl/osx/salmenu.cxx, AquaSalMenu::set/unsetMainMenu, not
                    // owning
                || name == "s_pCaptureFrame" // vcl/osx/salframe.cxx, not owning
                || name == "pBlink"
                    // sw/source/core/text/blink.cxx, _TextFinit()
                || name == "s_pIconCache"
                    // sd/source/ui/tools/IconCache.cxx, leaked
                || name == "maInstanceMap"
                    // sd/source/ui/framework/tools/FrameworkHelper.cxx, would
                    // leak ViewShellBase* keys if that map is not empty at exit
                || name == "theAddInAsyncTbl"
                    // sc/source/core/tool/adiasync.cxx, would leak
                    // ScAddInAsync* keys if that set is not empty at exit
                || name == "g_aWindowList"
                    //vcl/unx/gtk3/a11y/atkutil.cxx, asserted empty at exit
                || name == "gFontPreviewVirDevs"
                    //svtools/source/control/ctrlbox.cxx, empty at exit
                || name == "gStylePreviewCache" // svx/source/tbxctrls/StylesPreviewWindow.cxx
                || name == "aLogger" // FormulaLogger& FormulaLogger::get() in sc/source/core/tool/formulalogger.cxx
                || name == "s_aUncommittedRegistrations" // sw/source/uibase/dbui/dbmgr.cxx
                || (loplugin::DeclCheck(pVarDecl).Var("aAllListeners")
                    .Class("ScAddInListener").GlobalNamespace()) // not owning
                || (loplugin::DeclCheck(pVarDecl).Var("maThreadSpecific")
                    .Class("ScDocument").GlobalNamespace()) // not owning
                || name == "s_aLOKWindowsMap" // LOK only, guarded by assert, and LOK never tries to perform a VCL cleanup
                    // vcl/inc/jsdialog/jsdialogbuilder.hxx
                || name == "m_aWidgetRegister" // LOK only, similar case as above
                    //
                || name == "gNotebookBarManager" // LOK only case, when notebookbar is closed - VclPtr instance is removed
                || name == "gStaticManager" // vcl/source/graphic/Manager.cxx - stores non-owning pointers
                || name == "aThreadedInterpreterPool"    // ScInterpreterContext(Pool), not owning
                || name == "aNonThreadedInterpreterPool" // ScInterpreterContext(Pool), not owning
                || name == "lcl_parserContext" // getParserContext(), the chain from this to a VclPtr is not owning
                || name == "aReaderWriter" // /home/noel/libo/sw/source/filter/basflt/fltini.cxx, non-owning
                || name == "aTwain"
                   // Windows-only extensions/source/scanner/scanwin.cxx, problematic
                   // Twain::mpThread -> ShimListenerThread::mxTopWindow released via Twain::Reset
                   // clearing mpThread
                || name == "g_newReadOnlyDocs"
                   // sfx2/source/doc/docfile.cxx, warning about map's key
                || name == "g_existingReadOnlyDocs"
                   // sfx2/source/doc/docfile.cxx, warning about map's key
                || name == "gaFramesArr_Impl"
                   // sfx2/source/view/frame.cxx, vector of pointer, so not a problem, nothing is going to happen on shutdown
                || name == "g_pOLELRU_Cache" || name == "s_aTableColumnsMap"
                   // TODO
                || name == "SINGLETON"
                   // TheAquaA11yFocusTracker in vcl/osx/a11yfocustracker.cxx,
                   // AquaA11yFocusTracker::m_aDocumentWindowList elements symmetrically added and
                   // removed in AquaA11yFocusTracker::window_got_focus and
                   // AquaA11yFocusTracker::WindowEventHandler (TODO: is that guaranteed?)
                || (loplugin::DeclCheck(pVarDecl).Var("maEditViewHistory")
                        .Class("LOKEditViewHistory").GlobalNamespace())
                   // sfx2/lokhelper.hxx, only handling pointers, not owning
               ) // these variables appear unproblematic
            {
                return true;
            }
            // these two are fairly harmless because they're both empty objects
            if (   name == "s_xEmptyController" // svx/source/fmcomp/gridcell.cxx
                || name == "xCell"              // svx/source/table/svdotable.cxx
               )
            {
                return true;
            }
            // ignore pointers, nothing happens to them on shutdown
            QualType const pCanonical(pVarDecl->getType().getUnqualifiedType().getCanonicalType());
            if (pCanonical->isPointerType()) {
                return true;
            }
            std::vector<FieldDecl const*> pad;
            auto const ret(isBadStaticType(pVarDecl->getType(), pad,
                            std::vector<QualType>()));
            if (ret.first) {
                report(DiagnosticsEngine::Warning,
                        "bad static variable causes crash on shutdown",
                        pVarDecl->getLocation())
                    << pVarDecl->getSourceRange();
                if (!isUnitTestMode())
                {
                    for (auto i: ret.second) {
                        report(DiagnosticsEngine::Note,
                                "... due to this member of %0",
                                i->getLocation())
                            << i->getParent() << i->getSourceRange();
                    }
                }
            }
        }

        return true;
    }

};

loplugin::Plugin::Registration<BadStatics> badstatics("badstatics");

} // namespace

#endif // LO_CLANG_SHARED_PLUGINS

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
