/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/weld.hxx>
#include <calcconfig.hxx>

class ScCalcOptionsDialog : public weld::GenericDialogController
{
public:
    ScCalcOptionsDialog(weld::Window* pParent, const ScCalcConfig& rConfig, bool bWriteConfig);
    virtual ~ScCalcOptionsDialog() override;

    DECL_LINK(AsZeroModifiedHdl, weld::Toggleable&, void);
    DECL_LINK(ConversionModifiedHdl, weld::ComboBox&, void);
    DECL_LINK(SyntaxModifiedHdl, weld::ComboBox&, void);
    DECL_LINK(CurrentDocOnlyHdl, weld::Toggleable&, void);

    const ScCalcConfig& GetConfig() const { return maConfig; }
    bool GetWriteCalcConfig() const { return mbWriteConfig; }

private:
    void CoupleEmptyAsZeroToStringConversion();

private:
    ScCalcConfig maConfig;
    bool mbSelectedEmptyStringAsZero;
    bool mbWriteConfig;

    std::unique_ptr<weld::CheckButton> mxEmptyAsZero;
    std::unique_ptr<weld::ComboBox> mxConversion;
    std::unique_ptr<weld::CheckButton> mxCurrentDocOnly;
    std::unique_ptr<weld::ComboBox> mxSyntax;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
