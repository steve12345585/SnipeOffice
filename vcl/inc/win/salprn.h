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

#include <salprn.hxx>


// WNT3
#define SAL_DRIVERDATA_SYSSIGN              ((sal_uIntPtr)0x574E5433)

#pragma pack( 1 )

struct SalDriverData
{
    sal_uIntPtr                 mnSysSignature;
    sal_uInt16                  mnDriverOffset;
    BYTE                    maDriverData[1];
};

#pragma pack()


class WinSalGraphics;

class WinSalInfoPrinter final : public SalInfoPrinter
{
public:
    OUString               maDriverName;           // printer driver name
    OUString               maDeviceName;           // printer device name
    OUString               maPortName;             // printer port name

private:
    HDC m_hDC;                    ///< printer hdc
    WinSalGraphics* m_pGraphics;  ///< current Printer graphics
    bool m_bGraphics;             ///< is Graphics used

public:
    WinSalInfoPrinter();
    virtual ~WinSalInfoPrinter() override;

    void setHDC(HDC);

    virtual SalGraphics*            AcquireGraphics() override;
    virtual void                    ReleaseGraphics( SalGraphics* pGraphics ) override;
    virtual bool                    Setup( weld::Window* pFrame, ImplJobSetup* pSetupData ) override;
    virtual bool                    SetPrinterData( ImplJobSetup* pSetupData ) override;
    virtual bool                    SetData( JobSetFlags nFlags, ImplJobSetup* pSetupData ) override;
    virtual void                    GetPageInfo( const ImplJobSetup* pSetupData,
                                                 tools::Long& rOutWidth, tools::Long& rOutHeight,
                                                 Point& rPageOffset,
                                                 Size& rPaperSize ) override;
    virtual sal_uInt32              GetCapabilities( const ImplJobSetup* pSetupData, PrinterCapType nType ) override;
    virtual sal_uInt16              GetPaperBinCount( const ImplJobSetup* pSetupData ) override;
    virtual OUString                GetPaperBinName( const ImplJobSetup* pSetupData, sal_uInt16 nPaperBin ) override;
    virtual sal_uInt16              GetPaperBinBySourceIndex(const ImplJobSetup* pSetupData,
                                                             sal_uInt16 nPaperSource) override;
    virtual sal_uInt16              GetSourceIndexByPaperBin(const ImplJobSetup* pSetupData,
                                                             sal_uInt16 nPaperBin) override;

    virtual void                    InitPaperFormats( const ImplJobSetup* pSetupData ) override;
    virtual int                     GetLandscapeAngle( const ImplJobSetup* pSetupData ) override;
};


class WinSalPrinter : public SalPrinter
{
public:
    std::unique_ptr<WinSalGraphics> mxGraphics;    // current Printer graphics
    WinSalInfoPrinter*      mpInfoPrinter;          // pointer to the compatible InfoPrinter
    WinSalPrinter*          mpNextPrinter;          // next printing printer
    HDC                     mhDC;                   // printer hdc
    SalPrinterError         mnError;                // error code
    sal_uInt32              mnCopies;               // copies
    bool                    mbCollate;              // collated copies
    bool                    mbAbort;                // Job Aborted

    bool                    mbValid;

protected:
    void                            DoEndDoc(HDC hDC);

public:
    WinSalPrinter();
    virtual ~WinSalPrinter() override;

    using SalPrinter::StartJob;
    virtual bool                    StartJob( const OUString* pFileName,
                                              const OUString& rJobName,
                                              const OUString& rAppName,
                                              sal_uInt32 nCopies,
                                              bool bCollate,
                                              bool bDirect,
                                              ImplJobSetup* pSetupData ) override;
    virtual bool                    EndJob() override;
    virtual SalGraphics*            StartPage( ImplJobSetup* pSetupData, bool bNewJobData ) override;
    virtual void                    EndPage() override;
    virtual SalPrinterError         GetErrorCode() override;

    void markInvalid();
    bool isValid() const { return mbValid && mhDC; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
