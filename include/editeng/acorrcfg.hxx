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
#ifndef INCLUDED_EDITENG_ACORRCFG_HXX
#define INCLUDED_EDITENG_ACORRCFG_HXX

#include <editeng/editengdllapi.h>
#include <unotools/configitem.hxx>
#include <memory>

class SvxAutoCorrect;
class SvxAutoCorrCfg;

class SvxBaseAutoCorrCfg final : public utl::ConfigItem
{
private:
    SvxAutoCorrCfg& rParent;
    static css::uno::Sequence<OUString>    GetPropertyNames();

    virtual void            ImplCommit() override;

public:
    SvxBaseAutoCorrCfg(SvxAutoCorrCfg& rParent);
    virtual ~SvxBaseAutoCorrCfg() override;

    void                    Load(bool bInit);
    virtual void            Notify( const css::uno::Sequence<OUString>& aPropertyNames) override;
    using ConfigItem::SetModified;
};

class SvxSwAutoCorrCfg final : public utl::ConfigItem
{
private:
    SvxAutoCorrCfg& rParent;
    static css::uno::Sequence<OUString>    GetPropertyNames();

    virtual void            ImplCommit() override;

public:
    SvxSwAutoCorrCfg(SvxAutoCorrCfg& rParent);
    virtual ~SvxSwAutoCorrCfg() override;

    void                    Load(bool bInit);
    virtual void            Notify( const css::uno::Sequence<OUString>& aPropertyNames) override;
    using ConfigItem::SetModified;
};
/*--------------------------------------------------------------------
    Description:   Configuration for Auto Correction
 --------------------------------------------------------------------*/
class EDITENG_DLLPUBLIC SvxAutoCorrCfg final
{
    friend class SvxBaseAutoCorrCfg;
    friend class SvxSwAutoCorrCfg;

    std::unique_ptr<SvxAutoCorrect> pAutoCorrect;

    SvxBaseAutoCorrCfg      aBaseConfig;
    SvxSwAutoCorrCfg        aSwConfig;

    // Flags for Autotext:
    bool    bFileRel;
    bool    bNetRel;
    // Help tip for Autotext as you type
    bool    bAutoTextTip;
    bool    bAutoTextPreview;
    bool    bAutoFmtByInput;
    bool    bSearchInAllCategories;

public:
    void        SetModified()
                {
                    aBaseConfig.SetModified();
                    aSwConfig.SetModified();
                }
    void        Commit()
                {
                    aBaseConfig.Commit();
                    aSwConfig.Commit();
                }

          SvxAutoCorrect* GetAutoCorrect()          { return pAutoCorrect.get(); }
    const SvxAutoCorrect* GetAutoCorrect() const    { return pAutoCorrect.get(); }
    // the pointer is transferred to the possession of the ConfigItems!
    void SetAutoCorrect( SvxAutoCorrect* );

    bool IsAutoFormatByInput() const       { return bAutoFmtByInput; }
    void SetAutoFormatByInput( bool bSet ) { bAutoFmtByInput = bSet;aSwConfig.SetModified();}

    bool IsSaveRelFile() const          { return bFileRel; }
    void SetSaveRelFile( bool bSet )    { bFileRel = bSet; aSwConfig.SetModified(); }

    bool IsSaveRelNet() const           { return bNetRel; }
    void SetSaveRelNet( bool bSet )     { bNetRel = bSet; aSwConfig.SetModified();}

    bool IsAutoTextTip() const          { return bAutoTextTip; }
    void SetAutoTextTip(bool bSet )     { bAutoTextTip = bSet;aSwConfig.SetModified();}

    bool IsSearchInAllCategories() const        { return bSearchInAllCategories;}

    SvxAutoCorrCfg();
    ~SvxAutoCorrCfg();
    static SvxAutoCorrCfg& Get();
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
