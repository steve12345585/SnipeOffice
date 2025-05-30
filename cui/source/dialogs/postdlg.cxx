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

#include <tools/date.hxx>
#include <tools/lineend.hxx>
#include <tools/time.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <svl/itempool.hxx>
#include <svl/itemset.hxx>
#include <unotools/useroptions.hxx>
#include <unotools/localedatawrapper.hxx>
#include <svx/svxids.hrc>

#include <svx/postattr.hxx>
#include <postdlg.hxx>

// class SvxPostItDialog -------------------------------------------------

SvxPostItDialog::SvxPostItDialog(weld::Widget* pParent, const SfxItemSet& rCoreSet,
    bool bPrevNext)
    : SfxDialogController(pParent, u"cui/ui/comment.ui"_ustr, u"CommentDialog"_ustr)
    , m_rSet(rCoreSet)
    , m_xLastEditFT(m_xBuilder->weld_label(u"lastedit"_ustr))
    , m_xAltTitle(m_xBuilder->weld_label(u"alttitle"_ustr))
    , m_xEditED(m_xBuilder->weld_text_view(u"edit"_ustr))
    , m_xInsertAuthor(m_xBuilder->weld_widget(u"insertauthor"_ustr))
    , m_xAuthorBtn(m_xBuilder->weld_button(u"author"_ustr))
    , m_xOKBtn(m_xBuilder->weld_button(u"ok"_ustr))
    , m_xPrevBtn(m_xBuilder->weld_button(u"previous"_ustr))
    , m_xNextBtn(m_xBuilder->weld_button(u"next"_ustr))
{
    m_xPrevBtn->connect_clicked( LINK( this, SvxPostItDialog, PrevHdl ) );
    m_xNextBtn->connect_clicked( LINK( this, SvxPostItDialog, NextHdl ) );
    m_xAuthorBtn->connect_clicked( LINK( this, SvxPostItDialog, Stamp ) );
    m_xOKBtn->connect_clicked( LINK( this, SvxPostItDialog, OKHdl ) );

    bool bNew = true;

    m_xPrevBtn->set_visible(bPrevNext);
    m_xNextBtn->set_visible(bPrevNext);

    OUString aAuthorStr, aDateStr;

    if (m_rSet.GetItemState( SID_ATTR_POSTIT_AUTHOR ) >= SfxItemState::DEFAULT)
    {
        bNew = false;
        const SvxPostItAuthorItem& rAuthor = m_rSet.Get(SID_ATTR_POSTIT_AUTHOR);
        aAuthorStr = rAuthor.GetValue();
    }
    else
        aAuthorStr = SvtUserOptions().GetID();

    if (m_rSet.GetItemState( SID_ATTR_POSTIT_DATE ) >= SfxItemState::DEFAULT)
    {
        const SvxPostItDateItem& rDate = m_rSet.Get( SID_ATTR_POSTIT_DATE );
        aDateStr = rDate.GetValue();
    }
    else
    {
        const LocaleDataWrapper& rLocaleWrapper( Application::GetSettings().GetLocaleDataWrapper() );
        aDateStr = rLocaleWrapper.getDate( Date( Date::SYSTEM ) );
    }

    OUString aTextStr;
    if (m_rSet.GetItemState( SID_ATTR_POSTIT_TEXT ) >= SfxItemState::DEFAULT)
    {
        const SvxPostItTextItem& rText = m_rSet.Get( SID_ATTR_POSTIT_TEXT );
        aTextStr = rText.GetValue();
    }

    ShowLastAuthor(aAuthorStr, aDateStr);

    //lock to an initial size before replacing contents
    m_xEditED->set_size_request(m_xEditED->get_approximate_digit_width() * 32,
                                m_xEditED->get_height_rows(10));
    m_xEditED->set_text(convertLineEnd(aTextStr, GetSystemLineEnd()));

    if (!bNew)
        m_xDialog->set_title(m_xAltTitle->get_label());
}


SvxPostItDialog::~SvxPostItDialog()
{
}

void SvxPostItDialog::ShowLastAuthor(std::u16string_view rAuthor, std::u16string_view rDate)
{
    OUString sTxt = OUString::Concat(rAuthor) + ", " + rDate;
    m_xLastEditFT->set_label( sTxt );
}

const WhichRangesContainer & SvxPostItDialog::GetRanges()
{
    static const auto gRanges = WhichRangesContainer(svl::Items<SID_ATTR_POSTIT_AUTHOR, SID_ATTR_POSTIT_TEXT>);
    return gRanges;
}

void SvxPostItDialog::EnableTravel(bool bNext, bool bPrev)
{
    m_xPrevBtn->set_sensitive(bPrev);
    m_xNextBtn->set_sensitive(bNext);
}

IMPL_LINK_NOARG(SvxPostItDialog, PrevHdl, weld::Button&, void)
{
    m_aPrevHdlLink.Call( *this );
}

IMPL_LINK_NOARG(SvxPostItDialog, NextHdl, weld::Button&, void)
{
    m_aNextHdlLink.Call( *this );
}

IMPL_LINK_NOARG(SvxPostItDialog, Stamp, weld::Button&, void)
{
    Date aDate( Date::SYSTEM );
    tools::Time aTime( tools::Time::SYSTEM );
    OUString aTmp( SvtUserOptions().GetID() );
    const LocaleDataWrapper& rLocaleWrapper( Application::GetSettings().GetLocaleDataWrapper() );
    OUString aStr( m_xEditED->get_text() + "\n---- " );

    if ( !aTmp.isEmpty() )
    {
     aStr += aTmp + ", ";
    }
    aStr += rLocaleWrapper.getDate(aDate) + ", " + rLocaleWrapper.getTime(aTime, false) + " ----\n";
    aStr = convertLineEnd(aStr, GetSystemLineEnd());

    m_xEditED->set_text(aStr);
    sal_Int32 nLen = aStr.getLength();
    m_xEditED->grab_focus();
    m_xEditED->select_region(nLen, nLen);
}

IMPL_LINK_NOARG(SvxPostItDialog, OKHdl, weld::Button&, void)
{
    const LocaleDataWrapper& rLocaleWrapper( Application::GetSettings().GetLocaleDataWrapper() );
    m_xOutSet.reset(new SfxItemSet(m_rSet));
    m_xOutSet->Put( SvxPostItAuthorItem(SvtUserOptions().GetID(), SID_ATTR_POSTIT_AUTHOR ) );
    m_xOutSet->Put( SvxPostItDateItem(rLocaleWrapper.getDate( Date( Date::SYSTEM ) ), SID_ATTR_POSTIT_DATE ) );
    m_xOutSet->Put( SvxPostItTextItem(m_xEditED->get_text(), SID_ATTR_POSTIT_TEXT ) );
    m_xDialog->response(RET_OK);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
