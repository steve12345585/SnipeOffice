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

#include <sfx2/htmlmode.hxx>

#include <sfx2/sfxresid.hxx>

#include <sfx2/sfxsids.hrc>
#include <sfx2/objsh.hxx>
#include <sfx2/viewsh.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/passwd.hxx>

#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <svl/eitem.hxx>
#include <svl/poolitem.hxx>
#include <svl/intitem.hxx>
#include <svl/PasswordHelper.hxx>
#include <comphelper/docpasswordhelper.hxx>

#include <sfx2/strings.hrc>

#include "securitypage.hxx"

using namespace ::com::sun::star;

namespace
{
    enum RedliningMode  { RL_NONE, RL_WRITER, RL_CALC };

    bool QueryState( TypedWhichId<SfxBoolItem> _nSlot, bool& _rValue )
    {
        bool bRet = false;
        SfxViewShell* pViewSh = SfxViewShell::Current();
        if (pViewSh)
        {
            SfxPoolItemHolder aResult;
            const SfxItemState nState(pViewSh->GetDispatcher()->QueryState(_nSlot, aResult));
            bRet = SfxItemState::DEFAULT <= nState;
            if (bRet)
                _rValue = static_cast<const SfxBoolItem*>(aResult.getItem())->GetValue();
        }
        return bRet;
    }


    bool QueryRecordChangesProtectionState( RedliningMode _eMode, bool& _rValue )
    {
        bool bRet = false;
        if (_eMode != RL_NONE)
        {
            TypedWhichId<SfxBoolItem> nSlot = _eMode == RL_WRITER ? FN_REDLINE_PROTECT : SID_CHG_PROTECT;
            bRet = QueryState( nSlot, _rValue );
        }
        return bRet;
    }


    bool QueryRecordChangesState( RedliningMode _eMode, bool& _rValue )
    {
        bool bRet = false;
        if (_eMode != RL_NONE)
        {
            TypedWhichId<SfxBoolItem> nSlot = _eMode == RL_WRITER ? FN_REDLINE_ON : FID_CHG_RECORD;
            bRet = QueryState( nSlot, _rValue );
        }
        return bRet;
    }
}


static bool lcl_GetPassword(
    weld::Window *pParent,
    bool bProtect,
    /*out*/OUString &rPassword )
{
    bool bRes = false;
    SfxPasswordDialog aPasswdDlg(pParent);
    aPasswdDlg.SetMinLen(1);
    if (bProtect)
        aPasswdDlg.ShowExtras( SfxShowExtras::CONFIRM );
    if (RET_OK == aPasswdDlg.run() && !aPasswdDlg.GetPassword().isEmpty())
    {
        rPassword = aPasswdDlg.GetPassword();
        bRes = true;
    }
    return bRes;
}


static bool lcl_IsPasswordCorrect(weld::Window *pParent, std::u16string_view rPassword)
{
    SfxObjectShell* pCurDocShell = SfxObjectShell::Current();
    if (!pCurDocShell)
        return false;

    bool bRes = false;
    uno::Sequence< sal_Int8 >   aPasswordHash;
    pCurDocShell->GetProtectionHash( aPasswordHash );

    // check if supplied password was correct
    if (aPasswordHash.getLength() == 1 && aPasswordHash[0] == 1)
    {
        // dummy RedlinePassword from OOXML import: get real password info
        // from the grab-bag to verify the password
        const css::uno::Sequence< css::beans::PropertyValue > aDocumentProtection =
                                                pCurDocShell->GetDocumentProtectionFromGrabBag();
        bRes =
            // password is ok, if there is no DocumentProtection in the GrabBag,
            // i.e. the dummy RedlinePassword imported from an OpenDocument file
            !aDocumentProtection.hasElements() ||
            // verify password with the password info imported from OOXML
            ::comphelper::DocPasswordHelper::IsModifyPasswordCorrect( rPassword,
                    ::comphelper::DocPasswordHelper::ConvertPasswordInfo ( aDocumentProtection ) );
    }
    else
    {
        uno::Sequence< sal_Int8 > aNewPasswd( aPasswordHash );
        SvPasswordHelper::GetHashPassword( aNewPasswd, rPassword );
        bRes = SvPasswordHelper::CompareHashPassword( aPasswordHash, rPassword );
    }

    if ( !bRes )
    {
        std::unique_ptr<weld::MessageDialog> xInfoBox(Application::CreateMessageDialog(pParent,
                                                      VclMessageType::Info, VclButtonsType::Ok,
                                                      SfxResId(RID_SVXSTR_INCORRECT_PASSWORD)));
        xInfoBox->run();
    }

    return bRes;
}

struct SfxSecurityPage_Impl
{
    SfxSecurityPage &   m_rMyTabPage;

    RedliningMode       m_eRedlingMode;             // for record changes

    bool                m_bOrigPasswordIsConfirmed;
    bool                m_bNewPasswordIsValid;
    OUString            m_aNewPassword;

    OUString            m_aEndRedliningWarning;
    bool                m_bEndRedliningWarningDone;

    std::unique_ptr<weld::CheckButton> m_xOpenReadonlyCB;
    std::unique_ptr<weld::CheckButton> m_xRecordChangesCB;         // for record changes
    std::unique_ptr<weld::Button> m_xProtectPB;               // for record changes
    std::unique_ptr<weld::Button> m_xUnProtectPB;             // for record changes

    DECL_LINK(RecordChangesCBToggleHdl, weld::Toggleable&, void);
    DECL_LINK(ChangeProtectionPBHdl, weld::Button&, void);

    SfxSecurityPage_Impl( SfxSecurityPage &rDlg );

    bool    FillItemSet_Impl();
    void    Reset_Impl();
};

SfxSecurityPage_Impl::SfxSecurityPage_Impl(SfxSecurityPage &rTabPage)
    : m_rMyTabPage(rTabPage)
    , m_eRedlingMode(RL_NONE)
    , m_bOrigPasswordIsConfirmed(false)
    , m_bNewPasswordIsValid(false)
    , m_aEndRedliningWarning(SfxResId(RID_SVXSTR_END_REDLINING_WARNING))
    , m_bEndRedliningWarningDone(false)
    , m_xOpenReadonlyCB(rTabPage.GetBuilder().weld_check_button(u"readonly"_ustr))
    , m_xRecordChangesCB(rTabPage.GetBuilder().weld_check_button(u"recordchanges"_ustr))
    , m_xProtectPB(rTabPage.GetBuilder().weld_button(u"protect"_ustr))
    , m_xUnProtectPB(rTabPage.GetBuilder().weld_button(u"unprotect"_ustr))
{
    m_xProtectPB->show();
    m_xUnProtectPB->hide();

    m_xRecordChangesCB->connect_toggled(LINK(this, SfxSecurityPage_Impl, RecordChangesCBToggleHdl));
    m_xProtectPB->connect_clicked(LINK(this, SfxSecurityPage_Impl, ChangeProtectionPBHdl));
    m_xUnProtectPB->connect_clicked(LINK(this, SfxSecurityPage_Impl, ChangeProtectionPBHdl));
}

bool SfxSecurityPage_Impl::FillItemSet_Impl()
{
    bool bModified = false;

    SfxObjectShell* pCurDocShell = SfxObjectShell::Current();
    if (pCurDocShell && !pCurDocShell->IsReadOnly())
    {
        if (m_eRedlingMode != RL_NONE )
        {
            const bool bDoRecordChanges = m_xRecordChangesCB->get_active();
            const bool bDoChangeProtection  = m_xUnProtectPB->get_visible();

            // sanity checks
            DBG_ASSERT( bDoRecordChanges || !bDoChangeProtection, "no change recording should imply no change protection" );
            DBG_ASSERT( bDoChangeProtection || !bDoRecordChanges, "no change protection should imply no change recording" );
            DBG_ASSERT( !bDoChangeProtection || !m_aNewPassword.isEmpty(), "change protection should imply password length is > 0" );
            DBG_ASSERT( bDoChangeProtection || m_aNewPassword.isEmpty(), "no change protection should imply password length is 0" );

            // change recording
            if (bDoRecordChanges != pCurDocShell->IsChangeRecording())
            {
                pCurDocShell->SetChangeRecording( bDoRecordChanges );
                bModified = true;
            }

            // change record protection
            if (m_bNewPasswordIsValid &&
                bDoChangeProtection != pCurDocShell->HasChangeRecordProtection())
            {
                DBG_ASSERT( !bDoChangeProtection || bDoRecordChanges,
                        "change protection requires record changes to be active!" );
                pCurDocShell->SetProtectionPassword( m_aNewPassword );
                bModified = true;
            }
        }

        // open read-only?
        const bool bDoOpenReadonly = m_xOpenReadonlyCB->get_active();
        if (bDoOpenReadonly != pCurDocShell->IsSecurityOptOpenReadOnly())
        {
            pCurDocShell->SetSecurityOptOpenReadOnly( bDoOpenReadonly );
            bModified = true;
        }
    }

    return bModified;
}


void SfxSecurityPage_Impl::Reset_Impl()
{
    SfxObjectShell* pCurDocShell = SfxObjectShell::Current();

    if (!pCurDocShell)
    {
        // no doc -> hide document settings
        m_xOpenReadonlyCB->set_sensitive(false);
        m_xRecordChangesCB->set_sensitive(false);
        m_xProtectPB->show();
        m_xProtectPB->set_sensitive(false);
        m_xUnProtectPB->hide();
        m_xUnProtectPB->set_sensitive(false);
    }
    else
    {
        bool bIsHTMLDoc = false;
        bool bProtect = true, bUnProtect = false;
        SfxViewShell* pViewSh = SfxViewShell::Current();
        if (pViewSh)
        {
            SfxPoolItemHolder aResult;

            if (SfxItemState::DEFAULT <= pViewSh->GetDispatcher()->QueryState(SID_HTML_MODE, aResult))
            {
                const SfxUInt16Item* pItem(static_cast<const SfxUInt16Item*>(aResult.getItem()));
                const sal_uInt16 nMode(pItem->GetValue());
                bIsHTMLDoc = ( ( nMode & HTMLMODE_ON ) != 0 );
            }
        }

        bool bIsReadonly = pCurDocShell->IsReadOnly();
        if (!bIsHTMLDoc)
        {
            m_xOpenReadonlyCB->set_active(pCurDocShell->IsSecurityOptOpenReadOnly());
            m_xOpenReadonlyCB->set_sensitive(!bIsReadonly);
        }
        else
            m_xOpenReadonlyCB->set_sensitive(false);

        bool bRecordChanges;
        if (QueryRecordChangesState( RL_WRITER, bRecordChanges ) && !bIsHTMLDoc)
            m_eRedlingMode = RL_WRITER;
        else if (QueryRecordChangesState( RL_CALC, bRecordChanges ))
            m_eRedlingMode = RL_CALC;
        else
            m_eRedlingMode = RL_NONE;

        if (m_eRedlingMode != RL_NONE)
        {
            bool bProtection(false);
            QueryRecordChangesProtectionState( m_eRedlingMode, bProtection );

            m_xProtectPB->set_sensitive(!bIsReadonly);
            m_xUnProtectPB->set_sensitive(!bIsReadonly);
            // set the right text
            if (bProtection)
            {
                bProtect = false;
                bUnProtect = true;
            }

            m_xRecordChangesCB->set_active(bRecordChanges);
            m_xRecordChangesCB->set_sensitive(/*!bProtection && */!bIsReadonly);

            m_bOrigPasswordIsConfirmed = true;   // default case if no password is set
            uno::Sequence< sal_Int8 > aPasswordHash;
            // check if password is available
            if (pCurDocShell->GetProtectionHash( aPasswordHash ) &&
                aPasswordHash.hasElements())
                m_bOrigPasswordIsConfirmed = false;  // password found, needs to be confirmed later on
        }
        else
        {
            // A Calc document that is shared will have 'm_eRedlingMode == RL_NONE'
            // In shared documents change recording and protection must be disabled,
            // similar to documents that do not support change recording at all.
            m_xRecordChangesCB->set_active(false);
            m_xRecordChangesCB->set_sensitive(false);
            m_xProtectPB->set_sensitive(false);
            m_xUnProtectPB->set_sensitive(false);
        }

        m_xProtectPB->set_visible(bProtect);
        m_xUnProtectPB->set_visible(bUnProtect);
    }
}

IMPL_LINK_NOARG(SfxSecurityPage_Impl, RecordChangesCBToggleHdl, weld::Toggleable&, void)
{
    // when change recording gets disabled protection must be disabled as well
    if (m_xRecordChangesCB->get_active())    // the new check state is already present, thus the '!'
        return;

    bool bAlreadyDone = false;
    if (!m_bEndRedliningWarningDone)
    {
        std::unique_ptr<weld::MessageDialog> xWarn(Application::CreateMessageDialog(m_rMyTabPage.GetFrameWeld(),
                                                   VclMessageType::Warning, VclButtonsType::YesNo,
                                                   m_aEndRedliningWarning));
        xWarn->set_default_response(RET_NO);
        if (xWarn->run() != RET_YES)
            bAlreadyDone = true;
        else
            m_bEndRedliningWarningDone = true;
    }

    const bool bNeedPassword = !m_bOrigPasswordIsConfirmed
            && m_xUnProtectPB->get_visible(); // tdf#128230 Require password if the Unprotect button is visible
    if (!bAlreadyDone && bNeedPassword)
    {
        OUString aPasswordText;

        // dialog canceled or no password provided
        if (!lcl_GetPassword( m_rMyTabPage.GetFrameWeld(), false, aPasswordText ))
            bAlreadyDone = true;

        // ask for password and if dialog is canceled or no password provided return
        if (lcl_IsPasswordCorrect(m_rMyTabPage.GetFrameWeld(), aPasswordText))
            m_bOrigPasswordIsConfirmed = true;
        else
            bAlreadyDone = true;
    }

    if (bAlreadyDone)
        m_xRecordChangesCB->set_active(true);     // restore original state
    else
    {
        // remember required values to change protection and change recording in
        // FillItemSet_Impl later on if password was correct.
        m_bNewPasswordIsValid = true;
        m_aNewPassword.clear();
        m_xProtectPB->show();
        m_xUnProtectPB->hide();
    }
}

IMPL_LINK_NOARG(SfxSecurityPage_Impl, ChangeProtectionPBHdl, weld::Button&, void)
{
    if (m_eRedlingMode == RL_NONE)
        return;

    // the push button text is always the opposite of the current state. Thus:
    const bool bCurrentProtection = m_xUnProtectPB->get_visible();

    // ask user for password (if still necessary)
    OUString aPasswordText;
    bool bNewProtection = !bCurrentProtection;
    const bool bNeedPassword = bNewProtection || !m_bOrigPasswordIsConfirmed;
    if (bNeedPassword)
    {
        // ask for password and if dialog is canceled or no password provided return
        if (!lcl_GetPassword(m_rMyTabPage.GetFrameWeld(), bNewProtection, aPasswordText))
            return;

        // provided password still needs to be checked?
        if (!bNewProtection && !m_bOrigPasswordIsConfirmed)
        {
            if (lcl_IsPasswordCorrect(m_rMyTabPage.GetFrameWeld(), aPasswordText))
                m_bOrigPasswordIsConfirmed = true;
            else
                return;
        }
    }
    DBG_ASSERT( m_bOrigPasswordIsConfirmed, "ooops... this should not have happened!" );

    // remember required values to change protection and change recording in
    // FillItemSet_Impl later on if password was correct.
    m_bNewPasswordIsValid = true;
    m_aNewPassword = bNewProtection? aPasswordText : OUString();

    m_xRecordChangesCB->set_active(bNewProtection);

    m_xUnProtectPB->set_visible(bNewProtection);
    m_xProtectPB->set_visible(!bNewProtection);
}

std::unique_ptr<SfxTabPage> SfxSecurityPage::Create(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet * rItemSet)
{
    return std::make_unique<SfxSecurityPage>(pPage, pController, *rItemSet);
}

SfxSecurityPage::SfxSecurityPage(weld::Container* pPage, weld::DialogController* pController, const SfxItemSet& rItemSet)
    : SfxTabPage(pPage, pController, u"sfx/ui/securityinfopage.ui"_ustr, u"SecurityInfoPage"_ustr, &rItemSet)
{
    m_pImpl.reset(new SfxSecurityPage_Impl( *this ));
}

bool SfxSecurityPage::FillItemSet( SfxItemSet * /*rItemSet*/ )
{
    bool bModified = false;
    DBG_ASSERT(m_pImpl, "implementation pointer is 0. Still in c-tor?");
    if (m_pImpl != nullptr)
        bModified = m_pImpl->FillItemSet_Impl();
    return bModified;
}

void SfxSecurityPage::Reset( const SfxItemSet * /*rItemSet*/ )
{
    DBG_ASSERT(m_pImpl, "implementation pointer is 0. Still in c-tor?");
    if (m_pImpl != nullptr)
        m_pImpl->Reset_Impl();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
