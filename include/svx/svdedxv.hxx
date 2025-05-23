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

#ifndef INCLUDED_SVX_SVDEDXV_HXX
#define INCLUDED_SVX_SVDEDXV_HXX

#include <rtl/ref.hxx>
#include <svl/languageoptions.hxx>
#include <svx/svxdllapi.h>
#include <svx/svdglev.hxx>
#include <svx/selectioncontroller.hxx>
#include <editeng/editview.hxx>
#include <unotools/weakref.hxx>
#include <memory>

class SdrOutliner;
class OutlinerView;
class EditStatus;
class EditFieldInfo;
struct PasteOrDropInfos;
class SdrUndoManager;
class TextChainCursorManager;

namespace com::sun::star::uno {
    class Any;
}

namespace sdr {
    class SelectionController;
}

enum class SdrEndTextEditKind
{
    Unchanged,      // textobject unchanged
    Changed,        // textobject changed
    Deleted,        // textobject implicitly deleted
    ShouldBeDeleted // for writer: textobject should be deleted
};

// - general edit for objectspecific properties
// - textedit for all drawobjects, inherited from SdrTextObj
// - macromode


class SVXCORE_DLLPUBLIC SdrObjEditView : public SdrGlueEditView, public EditViewCallbacks
{
    friend class SdrPageView;
    friend class ImpSdrEditPara;

    // Now derived from EditViewCallbacks and overriding these callbacks to
    // allow own EditText visualization
    virtual void EditViewInvalidate(const tools::Rectangle& rRect) override;
    virtual void EditViewSelectionChange() override;
    virtual OutputDevice& EditViewOutputDevice() const override;
    virtual Point EditViewPointerPosPixel() const override;
    virtual css::uno::Reference<css::datatransfer::clipboard::XClipboard> GetClipboard() const override;
    virtual css::uno::Reference<css::datatransfer::dnd::XDropTarget> GetDropTarget() override;
    virtual void EditViewInputContext(const InputContext& rInputContext) override;
    virtual void EditViewCursorRect(const tools::Rectangle& rRect, int nExtTextInputWidth) override;

    // The OverlayObjects used for visualizing active TextEdit (currently
    // using TextEditOverlayObject, but not limited to it)
    sdr::overlay::OverlayObjectList maTEOverlayGroup;
    Timer                           maTextEditUpdateTimer;

    // IASS: allow reaction to active TextEdit changes
    DECL_DLLPRIVATE_LINK(ImpModifyHdl, LinkParamNone*, void);

    // IASS: timer-based reaction on TextEdit changes
    DECL_DLLPRIVATE_LINK(TextEditUpdate, Timer*, void);

protected:
    // TextEdit
    unotools::WeakReference<SdrTextObj> mxWeakTextEditObj; // current object in TextEdit
    SdrPageView* mpTextEditPV;
    std::unique_ptr<SdrOutliner> mpTextEditOutliner; // outliner for the TextEdit
    OutlinerView* mpTextEditOutlinerView; // current view of the outliners
    VclPtr<vcl::Window> mpTextEditWin; // matching window to pTextEditOutlinerView

    vcl::Cursor*                m_pTextEditCursorBuffer; // to restore the cursor in each window
    SdrObject*                  m_pMacroObj;
    SdrPageView*                m_pMacroPV;
    VclPtr<vcl::Window>         m_pMacroWin;

    tools::Rectangle            m_aTextEditArea;
    tools::Rectangle            m_aMinTextEditArea;
    Link<EditFieldInfo*,void>   m_aOldCalcFieldValueLink; // for call the old handler
    Point                       m_aMacroDownPos;

    sal_uInt16                  m_nMacroTol;

    bool mbTextEditDontDelete : 1;  // do not delete outliner and view of SdrEndTextEdit (f. spellchecking)
    bool mbTextEditOnlyOneView : 1; // a single OutlinerView (f. spellchecking)
    bool mbTextEditNewObj : 1;      // current edited object was just recreated
    bool mbQuickTextEditMode : 1;   // persistent(->CrtV). Default=TRUE
    bool mbMacroDown : 1;
    bool mbInteractiveSlideShow : 1; // IASS

    rtl::Reference< sdr::SelectionController > mxSelectionController;
    rtl::Reference< sdr::SelectionController > mxLastSelectionController;

    // check/set if we are in IASS and need to refresh evtl.
    void setInteractiveSlideShow(bool bNew) { mbInteractiveSlideShow = bNew; }
    bool isInteractiveSlideShow() const { return mbInteractiveSlideShow; }

private:
    EditUndoManager* mpOldTextEditUndoManager;
    std::unique_ptr<SdrUndoManager> mpLocalTextEditUndoManager;

protected:
    // Create a local UndoManager that is used for text editing.
    virtual std::unique_ptr<SdrUndoManager> createLocalTextUndoManager();

    void ImpMoveCursorAfterChainingEvent(TextChainCursorManager *pCursorManager);
    std::unique_ptr<TextChainCursorManager> ImpHandleMotionThroughBoxesKeyInput(const KeyEvent& rKEvt, bool *bOutHandled);

    OutlinerView* ImpFindOutlinerView(vcl::Window const * pWin) const;

    // Create a new OutlinerView at the heap and initialize all required parameters.
    // pTextEditObj, pTextEditPV and pTextEditOutliner have to be initialized
    OutlinerView* ImpMakeOutlinerView(vcl::Window* pWin, OutlinerView* pGivenView, SfxViewShell* pViewShell = nullptr) const;
    void ImpPaintOutlinerView(OutlinerView& rOutlView, const tools::Rectangle& rRect, OutputDevice& rTargetDevice) const;
    void ImpInvalidateOutlinerView(OutlinerView const & rOutlView) const;

    // Chaining
    void ImpChainingEventHdl();
    DECL_DLLPRIVATE_LINK(ImpAfterCutOrPasteChainingEventHdl, LinkParamNone*, void);


    // Check if the whole text is selected.
    // Still returns sal_True if there is no text present.
    bool ImpIsTextEditAllSelected() const;
    void ImpMakeTextCursorAreaVisible();

    // handler for AutoGrowing text with active Outliner
    DECL_DLLPRIVATE_LINK(ImpOutlinerStatusEventHdl, EditStatus&, void);
    DECL_DLLPRIVATE_LINK(ImpOutlinerCalcFieldValueHdl, EditFieldInfo*, void);

    // link for EndTextEditHdl
    DECL_DLLPRIVATE_LINK(EndTextEditHdl, SdrUndoManager*, void);

    void ImpMacroUp(const Point& rUpPos);
    void ImpMacroDown(const Point& rDownPos);

    DECL_LINK( BeginPasteOrDropHdl, PasteOrDropInfos*, void );
    DECL_LINK( EndPasteOrDropHdl, PasteOrDropInfos*, void );

protected:
    // #i71538# make constructors of SdrView sub-components protected to avoid incomplete incarnations which may get casted to SdrView
    SdrObjEditView(
        SdrModel& rSdrModel,
        OutputDevice* pOut);

    virtual ~SdrObjEditView() override;

public:

    // used to call the old ImpPaintOutlinerView. Will be replaced when the
    // outliner will be displayed on the overlay in edit mode.
    void TextEditDrawing(SdrPaintWindow& rPaintWindow);

    // Actionhandling for macromode
    virtual bool IsAction() const override;
    virtual void MovAction(const Point& rPnt) override;
    virtual void EndAction() override;
    virtual void BrkAction() override;
    virtual void BckAction() override;
    virtual void TakeActionRect(tools::Rectangle& rRect) const override;

    SdrPageView* ShowSdrPage(SdrPage* pPage) override;
    void HideSdrPage() override;

    virtual void Notify(SfxBroadcaster& rBC, const SfxHint& rHint) override;
    virtual void ModelHasChanged() override;

    const std::unique_ptr<SdrUndoManager>& getViewLocalUndoManager() const
    {
        return mpLocalTextEditUndoManager;
    }

    // TextEdit over an outliner

    // QuickTextEditMode = edit the text straight after selection. Default=TRUE. Persistent.
    void SetQuickTextEditMode(bool bOn)
    {
        mbQuickTextEditMode = bOn;
    }
    bool IsQuickTextEditMode() const
    {
        return mbQuickTextEditMode;
    }

    // Start the TextEditMode. If pWin==NULL, use the first window, which is logged at the View.
    // The cursor of the currently edited window is stored with SdrBeginTextEdit()
    // and restored with SdrEndTextEdit().
    // The app has to ensure, that the BegEdit of the window logged cursor is still valid,
    // when SdrEndTextEdit is called.
    // With the parameter pEditOutliner, the app has the possibility to specify his own outliner,
    // which is used for editing. After the SdrBeginTextEdit call, the outliner belongs to
    // SdrObjEditView, and is also later destroyed by this via delete (if bDontDeleteOutliner=sal_False).
    // Afterwards the SdrObjEditView sets the modflag (EditEngine/Outliner) at this instance and also the
    // StatusEventHdl.
    // Similarly a specific OutlinerView can be specified.

    virtual bool SdrBeginTextEdit(SdrObject* pObj, SdrPageView* pPV = nullptr, vcl::Window* pWin = nullptr, bool bIsNewObj = false,
        SdrOutliner* pGivenOutliner = nullptr, OutlinerView* pGivenOutlinerView = nullptr,
        bool bDontDeleteOutliner = false, bool bOnlyOneView = false, bool bGrabFocus = true);
    // bDontDeleteReally is a special parameter for writer
    // If this flag is set, then a maybe empty textobject is not deleted.
    // Instead you get a return code SdrEndTextEditKind::ShouldBeDeleted
    // (in place of SDRENDTEXTEDIT_BEDELETED), which says, the obj should be
    // deleted.
    virtual SdrEndTextEditKind SdrEndTextEdit(bool bDontDeleteReally = false);
    virtual bool IsTextEdit() const final override;

    // This method returns sal_True, if the point rHit is inside the
    // objectspace or the OutlinerView.
    bool IsTextEditHit(const Point& rHit) const;

    // This method returns sal_True, if the point rHit is inside the
    // handle-thick frame, which surrounds the OutlinerView at TextFrames.
    bool IsTextEditFrameHit(const Point& rHit) const;

    // At active selection, between MouseButtonDown and
    // MouseButtonUp, this method always returns TRUE.
    bool IsTextEditInSelectionMode() const;

    // If sb needs the object out of the TextEdit:
    SdrTextObj* GetTextEditObject() const { return mxWeakTextEditObj.get().get(); }

    // info about TextEditPageView. Default is 0L.
    SdrPageView* GetTextEditPageView() const;

    // Current window of the outliners.
    void SetTextEditWin(vcl::Window* pWin);

    // Now at this outliner, events can be send, attributes can be set,
    // call Cut/Copy/Paste, call Undo/Redo, and so on...
    virtual const SdrOutliner* GetTextEditOutliner() const
    {
        return mpTextEditOutliner.get();
    }
    virtual SdrOutliner* GetTextEditOutliner()
    {
        return mpTextEditOutliner.get();
    }
    virtual const OutlinerView* GetTextEditOutlinerView() const
    {
        return mpTextEditOutlinerView;
    }
    virtual OutlinerView* GetTextEditOutlinerView()
    {
        return mpTextEditOutlinerView;
    }

    virtual bool KeyInput(const KeyEvent& rKEvt, vcl::Window* pWin) override;
    virtual bool MouseButtonDown(const MouseEvent& rMEvt, OutputDevice* pWin) override;
    virtual bool MouseButtonUp(const MouseEvent& rMEvt, OutputDevice* pWin) override;
    virtual bool MouseMove(const MouseEvent& rMEvt, OutputDevice* pWin) override;
    virtual bool Command(const CommandEvent& rCEvt, vcl::Window* pWin) override;

    // #97766# make virtual to change implementation e.g. for SdOutlineView
    virtual SvtScriptType GetScriptType() const;

    /* new interface src537 */
    void GetAttributes(SfxItemSet& rTargetSet, bool bOnlyHardAttr) const;

    bool SetAttributes(const SfxItemSet& rSet, bool bReplaceAll);
    SfxStyleSheet* GetStyleSheet() const; // SfxStyleSheet* GetStyleSheet(bool& rOk) const;
    void SetStyleSheet(SfxStyleSheet* pStyleSheet, bool bDontRemoveHardAttr);

    // Intern: at mounting new OutlinerView...
    virtual void AddDeviceToPaintView(OutputDevice& rNewDev, vcl::Window* pWindow) override;
    virtual void DeleteDeviceFromPaintView(OutputDevice& rOldWin) override;

    sal_uInt16 GetSelectionLevel() const;


    // Object MacroMode (e.g. rect as button or sth. like that):

    void BegMacroObj(const Point& rPnt, short nTol, SdrObject* pObj, SdrPageView* pPV, vcl::Window* pWin);
    void BegMacroObj(const Point& rPnt, SdrObject* pObj, SdrPageView* pPV, vcl::Window* pWin) { BegMacroObj(rPnt,-2,pObj,pPV,pWin); }
    void MovMacroObj(const Point& rPnt);
    void BrkMacroObj();
    bool EndMacroObj();
    bool IsMacroObj() const { return m_pMacroObj!=nullptr; }

    /** fills the given any with a XTextCursor for the current text selection.
        Leaves the any untouched if there currently is no text selected */
    void getTextSelection( css::uno::Any& rSelection );

    virtual void MarkListHasChanged() override;

    const rtl::Reference< sdr::SelectionController >& getSelectionController() const { return mxSelectionController; }

    /** returns true if the shape identified by its inventor and identifier supports format paint brush operation */
    static bool SupportsFormatPaintbrush( SdrInventor nObjectInventor, SdrObjKind nObjectIdentifier );

    /** fills a format paint brush set from the current selection and returns the numbering depth */
    sal_Int32 TakeFormatPaintBrush( std::shared_ptr< SfxItemSet >& rFormatSet  );

    /** applies a format paint brush set from the current selection.
        if bNoCharacterFormats is true, no character attributes are changed.
        if bNoParagraphFormats is true, no paragraph attributes are changed.
    */
    void ApplyFormatPaintBrush( SfxItemSet& rFormatSet, sal_Int16 nDepth, bool bNoCharacterFormats, bool bNoParagraphFormats );

    /** helper function for selections with multiple SdrText for one SdrTextObj (f.e. tables ) */
    static void ApplyFormatPaintBrushToText( SfxItemSet const & rFormatSet, SdrTextObj& rTextObj, SdrText* pText, sal_Int16 nDepth, bool bNoCharacterFormats, bool bNoParagraphFormats );

    void DisposeUndoManager();

protected:
    virtual void OnBeginPasteOrDrop( PasteOrDropInfos* pInfo );
    virtual void OnEndPasteOrDrop( PasteOrDropInfos* pInfo );

};

#endif // INCLUDED_SVX_SVDEDXV_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
