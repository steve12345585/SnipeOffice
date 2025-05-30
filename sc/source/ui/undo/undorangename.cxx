/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <undorangename.hxx>
#include <globstr.hrc>
#include <scresid.hxx>

#include <sfx2/app.hxx>

#include <memory>
#include <utility>

ScUndoAllRangeNames::ScUndoAllRangeNames(
    ScDocShell* pDocSh,
    const std::map<OUString, ScRangeName*>& rOldNames,
    const std::map<OUString, ScRangeName>& rNewNames)
        : ScSimpleUndo(pDocSh)
{
    for (const auto& [rName, pRangeName] : rOldNames)
    {
        m_OldNames.insert(std::make_pair(rName, *pRangeName));
    }

    for (auto const& it : rNewNames)
    {
        m_NewNames.insert(std::make_pair(it.first, it.second));
    }
}

ScUndoAllRangeNames::~ScUndoAllRangeNames()
{
}

void ScUndoAllRangeNames::Undo()
{
    DoChange(m_OldNames);
}

void ScUndoAllRangeNames::Redo()
{
    DoChange(m_NewNames);
}

void ScUndoAllRangeNames::Repeat(SfxRepeatTarget& /*rTarget*/)
{
}

bool ScUndoAllRangeNames::CanRepeat(SfxRepeatTarget& /*rTarget*/) const
{
    return false;
}

OUString ScUndoAllRangeNames::GetComment() const
{
    return ScResId(STR_UNDO_RANGENAMES);
}

void ScUndoAllRangeNames::DoChange(const std::map<OUString, ScRangeName>& rNames)
{
    ScDocument& rDoc = pDocShell->GetDocument();

    rDoc.PreprocessAllRangeNamesUpdate(rNames);
    rDoc.SetAllRangeNames(rNames);
    rDoc.CompileHybridFormula();

    SfxGetpApp()->Broadcast(SfxHint(SfxHintId::ScAreasChanged));
}

ScUndoAddRangeData::ScUndoAddRangeData(ScDocShell* pDocSh, const ScRangeData* pRangeData, SCTAB nTab) :
    ScSimpleUndo(pDocSh),
    mpRangeData(new ScRangeData(*pRangeData)),
    mnTab(nTab)
{

}

ScUndoAddRangeData::~ScUndoAddRangeData()
{
}

void ScUndoAddRangeData::Undo()
{
    ScDocument& rDoc = pDocShell->GetDocument();
    ScRangeName* pRangeName = nullptr;
    if (mnTab == -1)
    {
        pRangeName = rDoc.GetRangeName();
    }
    else
    {
        pRangeName = rDoc.GetRangeName( mnTab );
    }
    pRangeName->erase(*mpRangeData);
    SfxGetpApp()->Broadcast( SfxHint( SfxHintId::ScAreasChanged ) );

}

void ScUndoAddRangeData::Redo()
{
    ScDocument& rDoc = pDocShell->GetDocument();
    ScRangeName* pRangeName = nullptr;
    if (mnTab == -1)
    {
        pRangeName = rDoc.GetRangeName();
    }
    else
    {
        pRangeName = rDoc.GetRangeName( mnTab );
    }
    pRangeName->insert(new ScRangeData(*mpRangeData));
    SfxGetpApp()->Broadcast( SfxHint( SfxHintId::ScAreasChanged ) );
}

void ScUndoAddRangeData::Repeat(SfxRepeatTarget& /*rTarget*/)
{
}

bool ScUndoAddRangeData::CanRepeat(SfxRepeatTarget& /*rTarget*/) const
{
    return false;
}

OUString ScUndoAddRangeData::GetComment() const
{
    return ScResId(STR_UNDO_RANGENAMES);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
