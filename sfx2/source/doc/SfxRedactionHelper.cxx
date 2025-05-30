/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <SfxRedactionHelper.hxx>
#include <autoredactdialog.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/XDrawPagesSupplier.hpp>
#include <com/sun/star/drawing/LineStyle.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/frame/XLayoutManager.hpp>

// For page margin related methods
#include <com/sun/star/style/XStyle.hpp>
#include <com/sun/star/style/XStyleFamiliesSupplier.hpp>
#include <com/sun/star/text/XPageCursor.hpp>
#include <com/sun/star/text/XTextViewCursorSupplier.hpp>
#include <com/sun/star/sheet/XSpreadsheetView.hpp>

// Search util
#include <i18nutil/searchopt.hxx>
#include <com/sun/star/util/SearchAlgorithms.hpp>
#include <com/sun/star/util/SearchAlgorithms2.hpp>
#include <com/sun/star/util/SearchFlags.hpp>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <unotools/textsearch.hxx>

#include <sfx2/request.hxx>
#include <sfx2/sfxsids.hrc>
#include <sfx2/viewfrm.hxx>

#include <svl/eitem.hxx>
#include <svl/stritem.hxx>

#include <svtools/DocumentToGraphicRenderer.hxx>

#include <tools/gen.hxx>
#include <tools/stream.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <vcl/gdimtf.hxx>
#include <vcl/graph.hxx>
#include <sal/log.hxx>

#include <vcl/wmf.hxx>
#include <vcl/metaact.hxx>
#include <vcl/outdev.hxx>
#include <vcl/vcllayout.hxx>
#include <o3tl/string_view.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;

bool SfxRedactionHelper::isRedactMode(const SfxRequest& rReq)
{
    const SfxItemSet* pArgs = rReq.GetArgs();
    if (pArgs)
    {
        const SfxBoolItem* pIsRedactMode = rReq.GetArg<SfxBoolItem>(SID_IS_REDACT_MODE);
        if (pIsRedactMode && pIsRedactMode->GetValue())
            return true;
    }

    return false;
}

OUString SfxRedactionHelper::getStringParam(const SfxRequest& rReq, sal_uInt16 nParamId)
{
    OUString sStringParam;

    const SfxItemSet* pArgs = rReq.GetArgs();
    if (!pArgs)
        return sStringParam;

    const SfxStringItem* pStringArg = rReq.GetArg<SfxStringItem>(nParamId);
    if (!pStringArg)
        return sStringParam;

    sStringParam = pStringArg->GetValue();
    return sStringParam;
}

namespace
{
/*
 * Roundtrip the gdimetafile to and from WMF
 * to get rid of the position and size irregularities
 * We better check the conversion method to see what it
 * actually does to correct these issues, and do it ourselves.
 * */
void fixMetaFile(GDIMetaFile& tmpMtf)
{
    SvMemoryStream aDestStrm(65535, 65535);
    ConvertGDIMetaFileToWMF(tmpMtf, aDestStrm, nullptr, false);
    aDestStrm.Seek(0);

    tmpMtf.Clear();

    ReadWindowMetafile(aDestStrm, tmpMtf);
}

/*
 * Sets page margins for a Draw page. Negative values are considered erroneous
 * */
void setPageMargins(const uno::Reference<beans::XPropertySet>& xPageProperySet,
                    const PageMargins& aPageMargins)
{
    if (aPageMargins.nTop < 0 || aPageMargins.nBottom < 0 || aPageMargins.nLeft < 0
        || aPageMargins.nRight < 0)
        return;

    xPageProperySet->setPropertyValue(u"BorderTop"_ustr, css::uno::Any(aPageMargins.nTop));
    xPageProperySet->setPropertyValue(u"BorderBottom"_ustr, css::uno::Any(aPageMargins.nBottom));
    xPageProperySet->setPropertyValue(u"BorderLeft"_ustr, css::uno::Any(aPageMargins.nLeft));
    xPageProperySet->setPropertyValue(u"BorderRight"_ustr, css::uno::Any(aPageMargins.nRight));
}

// #i10613# Extracted from ImplCheckRect::ImplCreate
tools::Rectangle ImplCalcActionBounds(const MetaAction& rAct, const OutputDevice& rOut,
                                      sal_Int32 nStrStartPos, sal_Int32 nStrEndPos)
{
    tools::Rectangle aActionBounds;

    switch (rAct.GetType())
    {
        case MetaActionType::TEXTARRAY:
        {
            const MetaTextArrayAction& rTextAct = static_cast<const MetaTextArrayAction&>(rAct);
            const OUString aString(rTextAct.GetText().copy(rTextAct.GetIndex(), rTextAct.GetLen()));

            if (!aString.isEmpty())
            {
                // #105987# ImplLayout takes everything in logical coordinates
                std::unique_ptr<SalLayout> pSalLayout1 = rOut.ImplLayout(
                    aString, 0, nStrStartPos, rTextAct.GetPoint(), 0, rTextAct.GetDXArray());
                std::unique_ptr<SalLayout> pSalLayout2 = rOut.ImplLayout(
                    aString, 0, nStrEndPos, rTextAct.GetPoint(), 0, rTextAct.GetDXArray());
                if (pSalLayout2)
                {
                    tools::Rectangle aBoundRect2(rOut.ImplGetTextBoundRect(*pSalLayout2));
                    aActionBounds = rOut.PixelToLogic(aBoundRect2);
                }
                if (pSalLayout1 && nStrStartPos > 0)
                {
                    tools::Rectangle aBoundRect1(rOut.ImplGetTextBoundRect(*pSalLayout1));
                    aActionBounds.SetLeft(rOut.PixelToLogic(aBoundRect1).Right());
                }
            }
        }
        break;

        default:
            break;
    }

    if (!aActionBounds.IsEmpty())
    {
        // fdo#40421 limit current action's output to clipped area
        if (rOut.IsClipRegion())
            return rOut.GetClipRegion().GetBoundRect().Intersection(aActionBounds);
        else
            return aActionBounds;
    }
    else
        return aActionBounds;
}

} // End of anon namespace

void SfxRedactionHelper::getPageMetaFilesFromDoc(std::vector<GDIMetaFile>& aMetaFiles,
                                                 std::vector<::Size>& aPageSizes, sal_Int32 nPages,
                                                 DocumentToGraphicRenderer& aRenderer)
{
    for (sal_Int32 nPage = 1; nPage <= nPages; ++nPage)
    {
        ::Size aDocumentSizePixel = aRenderer.getDocumentSizeInPixels(nPage);
        ::Point aLogicPos;
        ::Point aCalcPageLogicPos;
        ::Size aCalcPageContentSize;
        ::Size aLogic = aRenderer.getDocumentSizeIn100mm(nPage, &aLogicPos, &aCalcPageLogicPos,
                                                         &aCalcPageContentSize);

        aPageSizes.push_back(aLogic);

        Graphic aGraphic = aRenderer.renderToGraphic(nPage, aDocumentSizePixel, aDocumentSizePixel,
                                                     COL_TRANSPARENT, true);
        auto& rGDIMetaFile = const_cast<GDIMetaFile&>(aGraphic.GetGDIMetaFile());

        // Set preferred map unit and size on the metafile, so the Shape size
        // will be correct in MM.
        MapMode aMapMode;
        aMapMode.SetMapUnit(MapUnit::Map100thMM);

        rGDIMetaFile.SetPrefMapMode(aMapMode);
        rGDIMetaFile.SetPrefSize(aLogic);

        fixMetaFile(rGDIMetaFile);

        aMetaFiles.push_back(rGDIMetaFile);
    }
}

void SfxRedactionHelper::addPagesToDraw(
    const uno::Reference<XComponent>& xComponent, sal_Int32 nPages,
    const std::vector<GDIMetaFile>& aMetaFiles, const std::vector<::Size>& aPageSizes,
    const PageMargins& aPageMargins,
    const std::vector<std::pair<RedactionTarget, OUString>>& r_aTableTargets, bool bIsAutoRedact)
{
    // Access the draw pages
    uno::Reference<drawing::XDrawPagesSupplier> xDrawPagesSupplier(xComponent, uno::UNO_QUERY);
    uno::Reference<drawing::XDrawPages> xDrawPages = xDrawPagesSupplier->getDrawPages();

    uno::Reference<css::lang::XMultiServiceFactory> xFactory(xComponent, uno::UNO_QUERY);

    for (sal_Int32 nPage = 0; nPage < nPages; ++nPage)
    {
        const GDIMetaFile& rGDIMetaFile = aMetaFiles[nPage];
        Graphic aGraphic(rGDIMetaFile);

        sal_Int32 nPageHeight(aPageSizes[nPage].Height());
        sal_Int32 nPageWidth(aPageSizes[nPage].Width());

        uno::Reference<graphic::XGraphic> xGraph = aGraphic.GetXGraphic();
        uno::Reference<drawing::XDrawPage> xPage = xDrawPages->insertNewByIndex(nPage);

        // Set page size & margins
        uno::Reference<beans::XPropertySet> xPageProperySet(xPage, uno::UNO_QUERY);
        xPageProperySet->setPropertyValue(u"Height"_ustr, css::uno::Any(nPageHeight));
        xPageProperySet->setPropertyValue(u"Width"_ustr, css::uno::Any(nPageWidth));

        setPageMargins(xPageProperySet, aPageMargins);

        // Create and insert the shape
        uno::Reference<drawing::XShape> xShape(
            xFactory->createInstance(u"com.sun.star.drawing.GraphicObjectShape"_ustr),
            uno::UNO_QUERY);
        uno::Reference<beans::XPropertySet> xShapeProperySet(xShape, uno::UNO_QUERY);
        xShapeProperySet->setPropertyValue(u"Graphic"_ustr, uno::Any(xGraph));
        xShapeProperySet->setPropertyValue(u"MoveProtect"_ustr, uno::Any(true));
        xShapeProperySet->setPropertyValue(u"SizeProtect"_ustr, uno::Any(true));

        // Set size
        xShape->setSize(
            awt::Size(rGDIMetaFile.GetPrefSize().Width(), rGDIMetaFile.GetPrefSize().Height()));

        xPage->add(xShape);

        if (bIsAutoRedact && !r_aTableTargets.empty())
        {
            for (const auto& targetPair : r_aTableTargets)
            {
                autoRedactPage(targetPair.first, rGDIMetaFile, xPage, xComponent);
            }
        }
    }

    // Remove the extra page at the beginning
    uno::Reference<drawing::XDrawPage> xPage(xDrawPages->getByIndex(0), uno::UNO_QUERY_THROW);
    xDrawPages->remove(xPage);
}

void SfxRedactionHelper::showRedactionToolbar(const SfxViewFrame* pViewFrame)
{
    if (!pViewFrame)
        return;

    Reference<frame::XFrame> xFrame = pViewFrame->GetFrame().GetFrameInterface();
    Reference<css::beans::XPropertySet> xPropSet(xFrame, UNO_QUERY);
    Reference<css::frame::XLayoutManager> xLayoutManager;

    if (!xPropSet.is())
        return;

    try
    {
        Any aValue = xPropSet->getPropertyValue(u"LayoutManager"_ustr);
        aValue >>= xLayoutManager;
        xLayoutManager->createElement(u"private:resource/toolbar/redactionbar"_ustr);
        xLayoutManager->showElement(u"private:resource/toolbar/redactionbar"_ustr);
    }
    catch (const css::uno::RuntimeException&)
    {
        throw;
    }
    catch (css::uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION("sfx.doc", "Exception while trying to show the Redaction Toolbar!");
    }
}

PageMargins
SfxRedactionHelper::getPageMarginsForWriter(const css::uno::Reference<css::frame::XModel>& xModel)
{
    PageMargins aPageMargins = { -1, -1, -1, -1 };

    Reference<text::XTextViewCursorSupplier> xTextViewCursorSupplier(xModel->getCurrentController(),
                                                                     UNO_QUERY);
    if (!xTextViewCursorSupplier.is())
    {
        SAL_WARN("sfx.doc", "Ref to xTextViewCursorSupplier is null in setPageMargins().");
        return aPageMargins;
    }

    Reference<text::XPageCursor> xCursor(xTextViewCursorSupplier->getViewCursor(), UNO_QUERY);

    uno::Reference<beans::XPropertySet> xPageProperySet(xCursor, UNO_QUERY);
    OUString sPageStyleName;
    Any aValue = xPageProperySet->getPropertyValue(u"PageStyleName"_ustr);
    aValue >>= sPageStyleName;

    Reference<css::style::XStyleFamiliesSupplier> xStyleFamiliesSupplier(xModel, UNO_QUERY);
    if (!xStyleFamiliesSupplier.is())
    {
        SAL_WARN("sfx.doc", "Ref to xStyleFamiliesSupplier is null in setPageMargins().");
        return aPageMargins;
    }
    uno::Reference<container::XNameAccess> xStyleFamilies
        = xStyleFamiliesSupplier->getStyleFamilies();

    if (!xStyleFamilies.is())
        return aPageMargins;

    uno::Reference<container::XNameAccess> xPageStyles(
        xStyleFamilies->getByName(u"PageStyles"_ustr), UNO_QUERY);

    if (!xPageStyles.is())
        return aPageMargins;

    uno::Reference<css::style::XStyle> xPageStyle(xPageStyles->getByName(sPageStyleName),
                                                  UNO_QUERY);

    if (!xPageStyle.is())
        return aPageMargins;

    uno::Reference<beans::XPropertySet> xPageProperties(xPageStyle, uno::UNO_QUERY);

    if (!xPageProperties.is())
        return aPageMargins;

    xPageProperties->getPropertyValue(u"LeftMargin"_ustr) >>= aPageMargins.nLeft;
    xPageProperties->getPropertyValue(u"RightMargin"_ustr) >>= aPageMargins.nRight;
    xPageProperties->getPropertyValue(u"TopMargin"_ustr) >>= aPageMargins.nTop;
    xPageProperties->getPropertyValue(u"BottomMargin"_ustr) >>= aPageMargins.nBottom;

    return aPageMargins;
}

PageMargins
SfxRedactionHelper::getPageMarginsForCalc(const css::uno::Reference<css::frame::XModel>& xModel)
{
    PageMargins aPageMargins = { -1, -1, -1, -1 };
    OUString sPageStyleName(u"Default"_ustr);

    css::uno::Reference<css::sheet::XSpreadsheetView> xSpreadsheetView(
        xModel->getCurrentController(), UNO_QUERY);

    if (!xSpreadsheetView.is())
    {
        SAL_WARN("sfx.doc", "Ref to xSpreadsheetView is null in getPageMarginsForCalc().");
        return aPageMargins;
    }

    uno::Reference<beans::XPropertySet> xSheetProperties(xSpreadsheetView->getActiveSheet(),
                                                         UNO_QUERY);

    xSheetProperties->getPropertyValue(u"PageStyle"_ustr) >>= sPageStyleName;

    Reference<css::style::XStyleFamiliesSupplier> xStyleFamiliesSupplier(xModel, UNO_QUERY);
    if (!xStyleFamiliesSupplier.is())
    {
        SAL_WARN("sfx.doc", "Ref to xStyleFamiliesSupplier is null in getPageMarginsForCalc().");
        return aPageMargins;
    }
    uno::Reference<container::XNameAccess> xStyleFamilies
        = xStyleFamiliesSupplier->getStyleFamilies();

    if (!xStyleFamilies.is())
        return aPageMargins;

    uno::Reference<container::XNameAccess> xPageStyles(
        xStyleFamilies->getByName(u"PageStyles"_ustr), UNO_QUERY);

    if (!xPageStyles.is())
        return aPageMargins;

    uno::Reference<css::style::XStyle> xPageStyle(xPageStyles->getByName(sPageStyleName),
                                                  UNO_QUERY);

    if (!xPageStyle.is())
        return aPageMargins;

    uno::Reference<beans::XPropertySet> xPageProperties(xPageStyle, uno::UNO_QUERY);

    if (!xPageProperties.is())
        return aPageMargins;

    xPageProperties->getPropertyValue(u"LeftMargin"_ustr) >>= aPageMargins.nLeft;
    xPageProperties->getPropertyValue(u"RightMargin"_ustr) >>= aPageMargins.nRight;
    xPageProperties->getPropertyValue(u"TopMargin"_ustr) >>= aPageMargins.nTop;
    xPageProperties->getPropertyValue(u"BottomMargin"_ustr) >>= aPageMargins.nBottom;

    return aPageMargins;
}

void SfxRedactionHelper::searchInMetaFile(const RedactionTarget& rRedactionTarget,
                                          const GDIMetaFile& rMtf,
                                          std::vector<::tools::Rectangle>& aRedactionRectangles,
                                          const uno::Reference<XComponent>& xComponent)
{
    // Initialize search
    i18nutil::SearchOptions2 aSearchOptions;
    fillSearchOptions(aSearchOptions, rRedactionTarget);

    utl::TextSearch textSearch(aSearchOptions);

    OutputDevice* pOutputDevice
        = SfxObjectShell::GetShellFromComponent(xComponent)->GetDocumentRefDev();
    pOutputDevice->Push(::vcl::PushFlags::FONT);

    MetaAction* pCurrAct;

    for (pCurrAct = const_cast<GDIMetaFile&>(rMtf).FirstAction(); pCurrAct;
         pCurrAct = const_cast<GDIMetaFile&>(rMtf).NextAction())
    {
        // Watch for TEXTARRAY actions.
        // They contain the text of paragraphs.
        if (pCurrAct->GetType() == MetaActionType::TEXTARRAY)
        {
            MetaTextArrayAction* pMetaTextArrayAction = static_cast<MetaTextArrayAction*>(pCurrAct);

            // Search operation takes place here
            OUString sText = pMetaTextArrayAction->GetText();
            sal_Int32 nStart = 0;
            sal_Int32 nEnd = sText.getLength();

            bool bFound = textSearch.SearchForward(sText, &nStart, &nEnd);

            // If found the string, add the corresponding rectangle to the collection
            while (bFound)
            {
                tools::Rectangle aNewRect(
                    ImplCalcActionBounds(*pMetaTextArrayAction, *pOutputDevice, nStart, nEnd));

                if (!aNewRect.IsEmpty())
                {
                    // Then increase 10% of the new value to make it look better.
                    auto const adj(aNewRect.GetHeight() / 20);
                    aNewRect.AdjustTop(-adj);
                    aNewRect.AdjustBottom(adj);
                    aNewRect.AdjustRight(adj); // also add a bit on the right
                    aRedactionRectangles.push_back(aNewRect);
                }

                // Search for the next occurrence
                nStart = nEnd;
                nEnd = sText.getLength();
                bFound = textSearch.SearchForward(sText, &nStart, &nEnd);
            }
        }
        else if (pCurrAct->GetType() == MetaActionType::FONT)
        {
            const MetaFontAction* pFontAct = static_cast<const MetaFontAction*>(pCurrAct);
            vcl::Font const font{ pFontAct->GetFont() };
            pOutputDevice->SetFont(font);
        }
    }

    pOutputDevice->Pop();
}

void SfxRedactionHelper::addRedactionRectToPage(
    const uno::Reference<XComponent>& xComponent, const uno::Reference<drawing::XDrawPage>& xPage,
    const std::vector<::tools::Rectangle>& aNewRectangles)
{
    if (!xComponent.is() || !xPage.is())
        return;

    if (aNewRectangles.empty())
        return;

    uno::Reference<css::lang::XMultiServiceFactory> xFactory(xComponent, uno::UNO_QUERY);

    for (auto const& aNewRectangle : aNewRectangles)
    {
        uno::Reference<drawing::XShape> xRectShape(
            xFactory->createInstance(u"com.sun.star.drawing.RectangleShape"_ustr), uno::UNO_QUERY);
        uno::Reference<beans::XPropertySet> xRectShapeProperySet(xRectShape, uno::UNO_QUERY);

        xRectShapeProperySet->setPropertyValue(u"Name"_ustr,
                                               uno::Any(u"RectangleRedactionShape"_ustr));
        xRectShapeProperySet->setPropertyValue(u"FillTransparence"_ustr,
                                               css::uno::Any(static_cast<sal_Int16>(50)));
        xRectShapeProperySet->setPropertyValue(u"FillColor"_ustr, css::uno::Any(COL_GRAY7));
        xRectShapeProperySet->setPropertyValue(
            u"LineStyle"_ustr, css::uno::Any(css::drawing::LineStyle::LineStyle_NONE));

        xRectShape->setSize(awt::Size(aNewRectangle.GetWidth(), aNewRectangle.GetHeight()));
        xRectShape->setPosition(awt::Point(aNewRectangle.Left(), aNewRectangle.Top()));

        xPage->add(xRectShape);
    }
}

void SfxRedactionHelper::autoRedactPage(const RedactionTarget& rRedactionTarget,
                                        const GDIMetaFile& rGDIMetaFile,
                                        const uno::Reference<drawing::XDrawPage>& xPage,
                                        const uno::Reference<XComponent>& xComponent)
{
    if (rRedactionTarget.sContent.isEmpty())
        return;

    // Search for the redaction strings, and get the rectangle coordinates
    std::vector<::tools::Rectangle> aRedactionRectangles;
    searchInMetaFile(rRedactionTarget, rGDIMetaFile, aRedactionRectangles, xComponent);

    // Add the redaction rectangles to the page
    addRedactionRectToPage(xComponent, xPage, aRedactionRectangles);
}

namespace
{
const LanguageTag& GetAppLanguageTag() { return Application::GetSettings().GetLanguageTag(); }
}

void SfxRedactionHelper::fillSearchOptions(i18nutil::SearchOptions2& rSearchOpt,
                                           const RedactionTarget& rTarget)
{
    if (rTarget.sType == RedactionTargetType::REDACTION_TARGET_REGEX
        || rTarget.sType == RedactionTargetType::REDACTION_TARGET_PREDEFINED)
    {
        rSearchOpt.AlgorithmType2 = util::SearchAlgorithms2::REGEXP;
    }
    else
    {
        rSearchOpt.AlgorithmType2 = util::SearchAlgorithms2::ABSOLUTE;
    }

    rSearchOpt.Locale = GetAppLanguageTag().getLocale();
    if (rTarget.sType == RedactionTargetType::REDACTION_TARGET_PREDEFINED)
    {
        auto nPredefIndex = o3tl::toUInt32(o3tl::getToken(rTarget.sContent, 0, ';'));
        rSearchOpt.searchString = m_aPredefinedTargets[nPredefIndex];
    }
    else
        rSearchOpt.searchString = rTarget.sContent;

    rSearchOpt.replaceString.clear();

    if (!rTarget.bCaseSensitive && rTarget.sType != RedactionTargetType::REDACTION_TARGET_REGEX
        && rTarget.sType != RedactionTargetType::REDACTION_TARGET_PREDEFINED)
        rSearchOpt.transliterateFlags |= TransliterationFlags::IGNORE_CASE;
    if (rTarget.bWholeWords)
        rSearchOpt.searchFlag |= util::SearchFlags::NORM_WORD_ONLY;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
