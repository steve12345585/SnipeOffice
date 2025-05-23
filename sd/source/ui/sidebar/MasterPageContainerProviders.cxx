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

#include "MasterPageContainerProviders.hxx"

#include <DrawDocShell.hxx>
#include <drawdoc.hxx>
#include <sdpage.hxx>
#include <PreviewRenderer.hxx>
#include <svl/eitem.hxx>
#include <sfx2/app.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/thumbnailview.hxx>
#include <utility>
#include <vcl/image.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <sal/log.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace sd::sidebar {

//===== PagePreviewProvider ===================================================

PagePreviewProvider::PagePreviewProvider()
{
}

Image PagePreviewProvider::operator () (
    int nWidth,
    SdPage* pPage,
    ::sd::PreviewRenderer& rRenderer)
{
    Image aPreview;

    if (pPage != nullptr)
    {
        // Use the given renderer to create a preview of the given page
        // object.
        aPreview = rRenderer.RenderPage(
            pPage,
            nWidth);
    }

    return aPreview;
}

int PagePreviewProvider::GetCostIndex()
{
    return 5;
}

bool PagePreviewProvider::NeedsPageObject()
{
    return true;
}

//===== TemplatePreviewProvider ===============================================

TemplatePreviewProvider::TemplatePreviewProvider (OUString sURL)
    : msURL(std::move(sURL))
{
}

Image TemplatePreviewProvider::operator() (
    int,
    SdPage*,
    ::sd::PreviewRenderer&)
{
    return Image(ThumbnailView::readThumbnail(msURL));
}

int TemplatePreviewProvider::GetCostIndex()
{
    return 10;
}

bool TemplatePreviewProvider::NeedsPageObject()
{
    return false;
}

//===== TemplatePageObjectProvider =============================================

TemplatePageObjectProvider::TemplatePageObjectProvider (OUString sURL)
    : msURL(std::move(sURL))
{
}

SdPage* TemplatePageObjectProvider::operator() (SdDrawDocument*)
{
    SdPage* pPage = nullptr;

    mxDocumentShell = nullptr;
    try
    {
        // Load the template document and return its first page.
        ::sd::DrawDocShell* pDocumentShell = LoadDocument (msURL);
        if (pDocumentShell != nullptr)
        {
            SdDrawDocument* pDocument = pDocumentShell->GetDoc();
            if (pDocument != nullptr)
            {
                pPage = pDocument->GetMasterSdPage(0, PageKind::Standard);
                // In order to make the newly loaded master page deletable
                // when copied into documents it is marked as no "precious".
                // When it is modified then it is marked as "precious".
                if (pPage != nullptr)
                    pPage->SetPrecious(false);
            }
        }
    }
    catch (const uno::RuntimeException&)
    {
        DBG_UNHANDLED_EXCEPTION("sd");
        pPage = nullptr;
    }

    return pPage;
}

::sd::DrawDocShell* TemplatePageObjectProvider::LoadDocument (const OUString& sFileName)
{
    SfxApplication* pSfxApp = SfxGetpApp();
    std::unique_ptr<SfxItemSet> pSet(new SfxAllItemSet (pSfxApp->GetPool()));
    pSet->Put (SfxBoolItem (SID_TEMPLATE, true));
    pSet->Put (SfxBoolItem (SID_PREVIEW, true));
    if (pSfxApp->LoadTemplate (mxDocumentShell, sFileName, std::move(pSet)))
    {
        mxDocumentShell = nullptr;
    }
    SfxObjectShell* pShell = mxDocumentShell;
    return dynamic_cast< ::sd::DrawDocShell *>( pShell );
}

int TemplatePageObjectProvider::GetCostIndex()
{
    return 20;
}

//===== DefaultPageObjectProvider ==============================================

DefaultPageObjectProvider::DefaultPageObjectProvider()
{
}

SdPage* DefaultPageObjectProvider::operator () (SdDrawDocument* pContainerDocument)
{
    SdPage* pLocalMasterPage = nullptr;
    if (pContainerDocument != nullptr)
    {
        SdPage* pLocalSlide = pContainerDocument->GetSdPage(0, PageKind::Standard);
        if (pLocalSlide!=nullptr && pLocalSlide->TRG_HasMasterPage())
            pLocalMasterPage = dynamic_cast<SdPage*>(&pLocalSlide->TRG_GetMasterPage());
    }

    if (pLocalMasterPage == nullptr)
    {
        SAL_WARN( "sd", "can not create master page for slide");
    }

    return pLocalMasterPage;
}

int DefaultPageObjectProvider::GetCostIndex()
{
    return 15;
}

//===== ExistingPageProvider ==================================================

ExistingPageProvider::ExistingPageProvider (SdPage* pPage)
    : mpPage(pPage)
{
}

SdPage* ExistingPageProvider::operator() (SdDrawDocument*)
{
    return mpPage;
}

int ExistingPageProvider::GetCostIndex()
{
    return 0;
}

} // end of namespace sd::sidebar

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
