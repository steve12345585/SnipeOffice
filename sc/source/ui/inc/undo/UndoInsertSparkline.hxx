/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#pragma once

#include <undobase.hxx>
#include <address.hxx>
#include <memory>

namespace sc
{
class SparklineGroup;
struct SparklineData;

/** Undo action for inserting a Sparkline */
class UndoInsertSparkline : public ScSimpleUndo
{
private:
    std::vector<sc::SparklineData> maSparklineDataVector;
    std::shared_ptr<sc::SparklineGroup> mpSparklineGroup;

public:
    UndoInsertSparkline(ScDocShell& rDocShell, std::vector<SparklineData> pSparklineDataVector,
                        std::shared_ptr<sc::SparklineGroup> pSparklineGroup);

    virtual ~UndoInsertSparkline() override;

    void Undo() override;
    void Redo() override;
    bool CanRepeat(SfxRepeatTarget& rTarget) const override;
    void Repeat(SfxRepeatTarget& rTarget) override;
    OUString GetComment() const override;
};

} // namespace sc

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
