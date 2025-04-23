/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <namedefdlg.hxx>

#include <formula/errorcodes.hxx>
#include <sfx2/app.hxx>
#include <unotools/charclass.hxx>

#include <compiler.hxx>
#include <document.hxx>
#include <globstr.hrc>
#include <scresid.hxx>
#include <globalnames.hxx>
#include <rangenam.hxx>
#include <reffact.hxx>
#include <undorangename.hxx>
#include <tabvwsh.hxx>
#include <tokenarray.hxx>

ScNameDefDlg::ScNameDefDlg( SfxBindings* pB, SfxChildWindow* pCW, weld::Window* pParent,
        const ScViewData& rViewData, std::map<OUString, ScRangeName*>&& aRangeMap,
        const ScAddress& aCursorPos, const bool bUndo )
    : ScAnyRefDlgController( pB, pCW, pParent, u"modules/scalc/ui/definename.ui"_ustr, u"DefineNameDialog"_ustr)
    , mbUndo( bUndo )
    , mrDoc(rViewData.GetDocument())
    , mpDocShell ( rViewData.GetDocShell() )
    , maCursorPos( aCursorPos )
    , maGlobalNameStr  ( ScResId(STR_GLOBAL_SCOPE) )
    , maErrInvalidNameStr( ScResId(STR_ERR_NAME_INVALID))
    , maErrInvalidNameCellRefStr( ScResId(STR_ERR_NAME_INVALID_CELL_REF))
    , maErrInvalidSheetReference(ScResId(STR_INVALID_TABREF_PRINT_AREA))
    , maErrNameInUse   ( ScResId(STR_ERR_NAME_EXISTS))
    , maRangeMap( std::move(aRangeMap) )
    , m_xEdName(m_xBuilder->weld_entry(u"edit"_ustr))
    , m_xEdRange(new formula::RefEdit(m_xBuilder->weld_entry(u"range"_ustr)))
    , m_xRbRange(new formula::RefButton(m_xBuilder->weld_button(u"refbutton"_ustr)))
    , m_xLbScope(m_xBuilder->weld_combo_box(u"scope"_ustr))
    , m_xBtnRowHeader(m_xBuilder->weld_check_button(u"rowheader"_ustr))
    , m_xBtnColHeader(m_xBuilder->weld_check_button(u"colheader"_ustr))
    , m_xBtnPrintArea(m_xBuilder->weld_check_button(u"printarea"_ustr))
    , m_xBtnCriteria(m_xBuilder->weld_check_button(u"filter"_ustr))
    , m_xBtnAdd(m_xBuilder->weld_button(u"add"_ustr))
    , m_xBtnCancel(m_xBuilder->weld_button(u"cancel"_ustr))
    , m_xFtInfo(m_xBuilder->weld_label(u"label"_ustr))
    , m_xExpander(m_xBuilder->weld_expander(u"more"_ustr))
    , m_xFtRange(m_xBuilder->weld_label(u"label3"_ustr))
{
    m_xEdRange->SetReferences(this, m_xFtRange.get());
    m_xRbRange->SetReferences(this, m_xEdRange.get());
    maStrInfoDefault = m_xFtInfo->get_label();

    // Initialize scope list.
    m_xLbScope->append_text(maGlobalNameStr);
    m_xLbScope->set_active(0);
    SCTAB n = mrDoc.GetTableCount();
    for (SCTAB i = 0; i < n; ++i)
    {
        OUString aTabName;
        mrDoc.GetName(i, aTabName);
        m_xLbScope->append_text(aTabName);
    }

    m_xBtnCancel->connect_clicked( LINK( this, ScNameDefDlg, CancelBtnHdl));
    m_xBtnAdd->connect_clicked( LINK( this, ScNameDefDlg, AddBtnHdl ));
    m_xEdName->connect_changed( LINK( this, ScNameDefDlg, NameModifyHdl ));
    m_xEdRange->SetGetFocusHdl( LINK( this, ScNameDefDlg, AssignGetFocusHdl ) );
    m_xEdRange->SetModifyHdl( LINK( this, ScNameDefDlg, RefEdModifyHdl ) );
    m_xBtnPrintArea->connect_toggled(LINK(this, ScNameDefDlg, EdModifyCheckBoxHdl));

    m_xBtnAdd->set_sensitive(false); // empty name is invalid

    ScRange aRange;

    rViewData.GetSimpleArea( aRange );
    OUString aAreaStr(aRange.Format(mrDoc, ScRefFlags::RANGE_ABS_3D,
            ScAddress::Details(mrDoc.GetAddressConvention(), 0, 0)));

    m_xEdRange->SetText( aAreaStr );

    m_xEdName->grab_focus();
    m_xEdName->select_region(0, -1);
}

ScNameDefDlg::~ScNameDefDlg()
{
}

void ScNameDefDlg::CancelPushed()
{
    if (mbUndo)
        response(RET_CANCEL);
    else
    {
        if (ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell())
            pViewSh->SwitchBetweenRefDialogs(this);
    }
}

bool ScNameDefDlg::IsFormulaValid()
{
    const OUString aRangeOrFormulaExp = m_xEdRange->GetText();
    // tdf#140394 - check if formula is a valid print range
    if (m_xBtnPrintArea->get_active())
    {
        const ScRefFlags nValidAddr  = ScRefFlags::VALID | ScRefFlags::ROW_VALID | ScRefFlags::COL_VALID;
        const ScRefFlags nValidRange = nValidAddr | ScRefFlags::ROW2_VALID | ScRefFlags::COL2_VALID;
        const formula::FormulaGrammar::AddressConvention eConv = mrDoc.GetAddressConvention();
        const sal_Unicode sep = ScCompiler::GetNativeSymbolChar(ocSep);

        ScAddress aAddr;
        ScRange aRange;
        for (sal_Int32 nIdx = 0; nIdx >= 0;)
        {
            const OUString aOne = aRangeOrFormulaExp.getToken(0, sep, nIdx);
            ScRefFlags nResult = aRange.Parse(aOne, mrDoc, eConv);
            if ((nResult & nValidRange) != nValidRange)
            {
                ScRefFlags nAddrResult = aAddr.Parse(aOne, mrDoc, eConv);
                if ((nAddrResult & nValidAddr) != nValidAddr)
                    return false;
            }
        }
    }
    else
    {
        ScCompiler aComp(mrDoc, maCursorPos, mrDoc.GetGrammar());
        std::unique_ptr<ScTokenArray> pCode = aComp.CompileString(m_xEdRange->GetText());
        if (pCode->GetCodeError() != FormulaError::NONE)
            return false;
    }

    return true;
}

bool ScNameDefDlg::IsNameValid()
{
    OUString aScope = m_xLbScope->get_active_text();
    OUString aName = m_xEdName->get_text();

    bool bIsNameValid = true;
    OUString aHelpText = maStrInfoDefault;

    ScRangeName* pRangeName = nullptr;
    if(aScope == maGlobalNameStr)
    {
        const auto iter = maRangeMap.find(STR_GLOBAL_RANGE_NAME);
        assert(iter != maRangeMap.end());
        pRangeName = iter->second;
    }
    else
    {
        const auto iter = maRangeMap.find(aScope);
        assert(iter != maRangeMap.end());
        pRangeName = iter->second;
    }

    ScRangeData::IsNameValidType eType;
    if ( aName.isEmpty() )
    {
        bIsNameValid = false;
    }
    else if ((eType = ScRangeData::IsNameValid(aName, mrDoc))
             != ScRangeData::IsNameValidType::NAME_VALID)
    {
        if (eType == ScRangeData::IsNameValidType::NAME_INVALID_BAD_STRING)
        {
            aHelpText = maErrInvalidNameStr;
        }
        else if (eType == ScRangeData::IsNameValidType::NAME_INVALID_CELL_REF)
        {
            aHelpText = maErrInvalidNameCellRefStr;
        }
        bIsNameValid = false;
    }
    else if (pRangeName->findByUpperName(ScGlobal::getCharClass().uppercase(aName)))
    {
        aHelpText = maErrNameInUse;
        bIsNameValid = false;
    }

    if (!IsFormulaValid())
    {
        bIsNameValid = false;
        if (m_xBtnPrintArea->get_active())
            aHelpText = maErrInvalidSheetReference;
        //TODO: info message for a non valid formula (print range not checked)
    }

    m_xEdName->set_tooltip_text(aHelpText);
    m_xEdName->set_message_type(bIsNameValid || aName.isEmpty() ? weld::EntryMessageType::Normal
                                                                : weld::EntryMessageType::Error);
    m_xBtnAdd->set_sensitive(bIsNameValid);
    return bIsNameValid;
}

void ScNameDefDlg::AddPushed()
{
    OUString aScope = m_xLbScope->get_active_text();
    OUString aName = m_xEdName->get_text();
    OUString aExpression = m_xEdRange->GetText();

    if (aName.isEmpty())
    {
        return;
    }
    if (aScope.isEmpty())
    {
        return;
    }

    ScRangeName* pRangeName = nullptr;
    if(aScope == maGlobalNameStr)
    {
        const auto iter = maRangeMap.find(STR_GLOBAL_RANGE_NAME);
        assert(iter != maRangeMap.end());
        pRangeName = iter->second;
    }
    else
    {
        const auto iter = maRangeMap.find(aScope);
        assert(iter != maRangeMap.end());
        pRangeName = iter->second;
    }
    if (!pRangeName)
        return;

    if (!IsNameValid()) //should not happen, but make sure we don't break anything
        return;
    else
    {
        ScRangeData::Type nType = ScRangeData::Type::Name;

        ScRangeData* pNewEntry = new ScRangeData( mrDoc,
                aName,
                aExpression,
                maCursorPos,
                nType );

        if ( m_xBtnRowHeader->get_active() ) nType |= ScRangeData::Type::RowHeader;
        if ( m_xBtnColHeader->get_active() ) nType |= ScRangeData::Type::ColHeader;
        if ( m_xBtnPrintArea->get_active() ) nType |= ScRangeData::Type::PrintArea;
        if ( m_xBtnCriteria->get_active()  ) nType |= ScRangeData::Type::Criteria;

        pNewEntry->AddType(nType);

        // aExpression valid?
        if ( FormulaError::NONE == pNewEntry->GetErrCode() )
        {
            if ( !pRangeName->insert( pNewEntry, false /*bReuseFreeIndex*/ ) )
                pNewEntry = nullptr;

            if (mbUndo)
            {
                // this means we called directly through the menu

                SCTAB nTab;
                // if no table with that name is found, assume global range name
                if (!mrDoc.GetTable(aScope, nTab))
                    nTab = -1;

                assert( pNewEntry);     // undo of no insertion smells fishy
                if (pNewEntry)
                    mpDocShell->GetUndoManager()->AddUndoAction(
                            std::make_unique<ScUndoAddRangeData>( mpDocShell, pNewEntry, nTab) );

                // set table stream invalid, otherwise RangeName won't be saved if no other
                // call invalidates the stream
                if (nTab != -1)
                    mrDoc.SetStreamValid(nTab, false);
                SfxGetpApp()->Broadcast( SfxHint( SfxHintId::ScAreasChanged ) );
                mpDocShell->SetDocumentModified();
                Close();
            }
            else
            {
                maName = aName;
                maScope = aScope;
                if (ScTabViewShell* pViewSh = ScTabViewShell::GetActiveViewShell())
                    pViewSh->SwitchBetweenRefDialogs(this);
            }
        }
        else
        {
            delete pNewEntry;
            m_xEdRange->GrabFocus();
            m_xEdRange->SelectAll();
        }
    }
}

void ScNameDefDlg::GetNewData(OUString& rName, OUString& rScope)
{
    rName = maName;
    rScope = maScope;
}

bool ScNameDefDlg::IsRefInputMode() const
{
    return m_xEdRange->GetWidget()->get_sensitive();
}

void ScNameDefDlg::RefInputDone( bool bForced)
{
    ScAnyRefDlgController::RefInputDone(bForced);
    IsNameValid();
}

void ScNameDefDlg::SetReference( const ScRange& rRef, ScDocument& rDocP )
{
    if (m_xEdRange->GetWidget()->get_sensitive())
    {
        if ( rRef.aStart != rRef.aEnd )
            RefInputStart(m_xEdRange.get());
        OUString aRefStr(rRef.Format(rDocP, ScRefFlags::RANGE_ABS_3D,
                ScAddress::Details(rDocP.GetAddressConvention(), 0, 0)));
        m_xEdRange->SetRefString( aRefStr );
    }
}

void ScNameDefDlg::Close()
{
    DoClose( ScNameDefDlgWrapper::GetChildWindowId() );
}

void ScNameDefDlg::SetActive()
{
    m_xEdRange->GrabFocus();
    RefInputDone();
}

IMPL_LINK_NOARG(ScNameDefDlg, CancelBtnHdl, weld::Button&, void)
{
    CancelPushed();
}

IMPL_LINK_NOARG(ScNameDefDlg, AddBtnHdl, weld::Button&, void)
{
    AddPushed();
};

IMPL_LINK_NOARG(ScNameDefDlg, NameModifyHdl, weld::Entry&, void)
{
    IsNameValid();
}

IMPL_LINK_NOARG(ScNameDefDlg, AssignGetFocusHdl, formula::RefEdit&, void)
{
    IsNameValid();
}

IMPL_LINK_NOARG(ScNameDefDlg, EdModifyCheckBoxHdl, weld::Toggleable&, void)
{
    IsNameValid();
}

IMPL_LINK_NOARG(ScNameDefDlg, RefEdModifyHdl, formula::RefEdit&, void)
{
    IsNameValid();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
