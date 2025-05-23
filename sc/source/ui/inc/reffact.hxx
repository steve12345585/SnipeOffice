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

#pragma once

#include <sfx2/childwin.hxx>
#include "ChildWindowWrapper.hxx"

#include <sc.hrc>

#define DECL_WRAPPER_WITHID(Class) \
    class Class : public SfxChildWindow                                         \
    {                                                                           \
    public:                                                                     \
        Class( vcl::Window*, sal_uInt16, SfxBindings*, SfxChildWinInfo* ); \
        SFX_DECL_CHILDWINDOW_WITHID(Class);                                     \
    };

DECL_WRAPPER_WITHID(ScNameDlgWrapper)
DECL_WRAPPER_WITHID(ScNameDefDlgWrapper)
DECL_WRAPPER_WITHID(ScSolverDlgWrapper)
DECL_WRAPPER_WITHID(ScOptSolverDlgWrapper)
DECL_WRAPPER_WITHID(ScXMLSourceDlgWrapper)
DECL_WRAPPER_WITHID(ScPivotLayoutWrapper)
DECL_WRAPPER_WITHID(ScTabOpDlgWrapper)
DECL_WRAPPER_WITHID(ScFilterDlgWrapper)
DECL_WRAPPER_WITHID(ScSpecialFilterDlgWrapper)
DECL_WRAPPER_WITHID(ScDbNameDlgWrapper)
DECL_WRAPPER_WITHID(ScConsolidateDlgWrapper)
DECL_WRAPPER_WITHID(ScPrintAreasDlgWrapper)
DECL_WRAPPER_WITHID(ScColRowNameRangesDlgWrapper)
DECL_WRAPPER_WITHID(ScFormulaDlgWrapper)
DECL_WRAPPER_WITHID(ScHighlightChgDlgWrapper)
DECL_WRAPPER_WITHID(ScCondFormatDlgWrapper)

class ScDescriptiveStatisticsDialogWrapper :
    public ChildControllerWrapper<SID_DESCRIPTIVE_STATISTICS_DIALOG>
{
private:
    ScDescriptiveStatisticsDialogWrapper() = delete;
};

class ScSamplingDialogWrapper :
    public ChildControllerWrapper<SID_SAMPLING_DIALOG>
{
private:
    ScSamplingDialogWrapper() = delete;
};

class ScRandomNumberGeneratorDialogWrapper :
    public ChildControllerWrapper<SID_RANDOM_NUMBER_GENERATOR_DIALOG>
{
private:
    ScRandomNumberGeneratorDialogWrapper() = delete;
};

class ScAnalysisOfVarianceDialogWrapper :
    public ChildControllerWrapper<SID_ANALYSIS_OF_VARIANCE_DIALOG>
{
private:
    ScAnalysisOfVarianceDialogWrapper() = delete;
};

class ScCorrelationDialogWrapper :
    public ChildControllerWrapper<SID_CORRELATION_DIALOG>
{
private:
    ScCorrelationDialogWrapper() = delete;
};

class ScCovarianceDialogWrapper :
    public ChildControllerWrapper<SID_COVARIANCE_DIALOG>
{
private:
    ScCovarianceDialogWrapper() = delete;
};

class ScExponentialSmoothingDialogWrapper :
    public ChildControllerWrapper<SID_EXPONENTIAL_SMOOTHING_DIALOG>
{
private:
    ScExponentialSmoothingDialogWrapper() = delete;
};

class ScMovingAverageDialogWrapper :
    public ChildControllerWrapper<SID_MOVING_AVERAGE_DIALOG>
{
private:
    ScMovingAverageDialogWrapper() = delete;
};

class ScRegressionDialogWrapper :
    public ChildControllerWrapper<SID_REGRESSION_DIALOG>
{
private:
    ScRegressionDialogWrapper() = delete;
};

class ScTTestDialogWrapper :
    public ChildControllerWrapper<SID_TTEST_DIALOG>
{
private:
    ScTTestDialogWrapper() = delete;
};

class ScFTestDialogWrapper :
    public ChildControllerWrapper<SID_FTEST_DIALOG>
{
private:
    ScFTestDialogWrapper() = delete;
};

class ScZTestDialogWrapper :
    public ChildControllerWrapper<SID_ZTEST_DIALOG>
{
private:
    ScZTestDialogWrapper() = delete;
};

class ScChiSquareTestDialogWrapper :
    public ChildControllerWrapper<SID_CHI_SQUARE_TEST_DIALOG>
{
private:
    ScChiSquareTestDialogWrapper() = delete;
};

class ScFourierAnalysisDialogWrapper :
    public ChildControllerWrapper<SID_FOURIER_ANALYSIS_DIALOG>
{
private:
    ScFourierAnalysisDialogWrapper() = delete;
};

namespace sc
{
/** Wrapper for the sparkline properties dialog */
class SparklineDialogWrapper :
    public ChildControllerWrapper<SID_SPARKLINE_DIALOG>
{
private:
    SparklineDialogWrapper() = delete;
};

/** Wrapper for the sparkline data range dialog */
class SparklineDataRangeDialogWrapper :
    public ChildControllerWrapper<SID_SPARKLINE_DATA_RANGE_DIALOG>
{
private:
    SparklineDataRangeDialogWrapper() = delete;
};

/** Wrapper for the easy conditional format dialog */
class ConditionalFormatEasyDialogWrapper :
    public ChildControllerWrapper<SID_EASY_CONDITIONAL_FORMAT_DIALOG>
{
private:
    ConditionalFormatEasyDialogWrapper() = delete;
};
}

class ScAcceptChgDlgWrapper : public SfxChildWindow
{
public:
    ScAcceptChgDlgWrapper( vcl::Window*,
                           sal_uInt16,
                           SfxBindings*,
                           SfxChildWinInfo* );

    SFX_DECL_CHILDWINDOW_WITHID(Class);

    void ReInitDlg();
};

class ScSimpleRefDlgWrapper: public SfxChildWindow
{
public:
    ScSimpleRefDlgWrapper(vcl::Window*,
                          sal_uInt16,
                          SfxBindings*,
                          SfxChildWinInfo*);

    SFX_DECL_CHILDWINDOW_WITHID(Class);

    void            SetRefString(const OUString& rStr);
    void            SetCloseHdl( const Link<const OUString*,void>& rLink );
    void            SetUnoLinks( const Link<const OUString&,void>& rDone, const Link<const OUString&,void>& rAbort,
                                    const Link<const OUString&,void>& rChange );
    void            SetFlags( bool bCloseOnButtonUp, bool bSingleCell, bool bMultiSelection );
    static void     SetAutoReOpen(bool bFlag);

    void            StartRefInput();
};

class ScValidityRefChildWin : public SfxChildWindow
{
    bool    m_bVisibleLock:1;
    bool    m_bFreeWindowLock:1;
public:
    ScValidityRefChildWin( vcl::Window*, sal_uInt16, const SfxBindings*, SfxChildWinInfo* );
    SFX_DECL_CHILDWINDOW_WITHID(ScValidityRefChildWin);
    virtual ~ScValidityRefChildWin() override;
    bool    LockVisible( bool bLock ){ bool bVis = m_bVisibleLock; m_bVisibleLock = bLock; return bVis; }
    bool    LockFreeWindow( bool bLock ){ bool bFreeWindow = m_bFreeWindowLock; m_bFreeWindowLock = bLock; return bFreeWindow; }
    void                Hide() override { if( !m_bVisibleLock) SfxChildWindow::Hide(); }
    void                Show( ShowFlags nFlags ) override { if( !m_bVisibleLock ) SfxChildWindow::Show( nFlags ); }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
