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

#include <sal/config.h>

#include <vcl/commandinfoprovider.hxx>
#include <vcl/event.hxx>
#include <vcl/weld.hxx>
#include <vcl/svapp.hxx>

#include <algorithm>
#include <cstddef>

#include <helpids.h>
#include <strings.hrc>

#include <cfg.hxx>
#include <SvxNotebookbarConfigPage.hxx>
#include <SvxConfigPageHelper.hxx>
#include <dialmgr.hxx>
#include <libxml/parser.h>
#include <osl/file.hxx>
#include <CustomNotebookbarGenerator.hxx>
#include <sfx2/notebookbar/SfxNotebookBar.hxx>
#include <unotools/configmgr.hxx>
#include <comphelper/processfactory.hxx>
#include <o3tl/string_view.hxx>
#include <com/sun/star/frame/theUICommandDescription.hpp>

namespace uno = css::uno;
namespace frame = css::frame;
namespace container = css::container;
namespace beans = css::beans;

static bool isCategoryAvailable(std::u16string_view sClassId, std::u16string_view sUIItemId,
                                std::u16string_view sActiveCategory, bool& isCategory)
{
    if (sUIItemId == sActiveCategory)
        return true;
    else if ((sClassId == u"GtkMenu" || sClassId == u"GtkGrid") && sUIItemId != sActiveCategory)
    {
        isCategory = false;
        return false;
    }
    return false;
}

static OUString charToString(const char* cString)
{
    return OUString(cString, strlen(cString), RTL_TEXTENCODING_UTF8);
}

static OUString getFileName(std::u16string_view aFileName)
{
    if (aFileName == u"notebookbar.ui")
        return CuiResId(RID_CUISTR_TABBED);
    else if (aFileName == u"notebookbar_compact.ui")
        return CuiResId(RID_CUISTR_TABBED_COMPACT);
    else if (aFileName == u"notebookbar_groupedbar_full.ui")
        return CuiResId(RID_CUISTR_GROUPEDBAR);
    else if (aFileName == u"notebookbar_groupedbar_compact.ui")
        return CuiResId(RID_CUISTR_GROUPEDBAR_COMPACT);
    else
        return u"None"_ustr;
}

static OUString getModuleId(std::u16string_view sModuleName)
{
    if (sModuleName == u"Writer")
        return u"com.sun.star.text.TextDocument"_ustr;
    else if (sModuleName == u"Draw")
        return u"com.sun.star.drawing.DrawingDocument"_ustr;
    else if (sModuleName == u"Impress")
        return u"com.sun.star.presentation.PresentationDocument"_ustr;
    else if (sModuleName == u"Calc")
        return u"com.sun.star.sheet.SpreadsheetDocument"_ustr;
    else
        return u"None"_ustr;
}

SvxNotebookbarConfigPage::SvxNotebookbarConfigPage(weld::Container* pPage,
                                                   weld::DialogController* pController,
                                                   const SfxItemSet& rSet)
    : SvxConfigPage(pPage, pController, rSet)
{
    m_xCommandCategoryListBox->set_visible(false);
    m_xDescriptionFieldLb->set_visible(false);
    m_xSearchEdit->set_visible(false);
    m_xDescriptionField->set_visible(false);
    m_xMoveUpButton->set_visible(false);
    m_xMoveDownButton->set_visible(false);
    m_xCommandButtons->set_visible(false);
    m_xLeftFunctionLabel->set_visible(false);
    m_xSearchLabel->set_visible(false);
    m_xCategoryLabel->set_visible(false);
    m_xCustomizeBox->set_visible(false);
    m_xCustomizeLabel->set_visible(false);

    weld::TreeView& rCommandCategoryBox = m_xFunctions->get_widget();
    rCommandCategoryBox.hide();

    m_xContentsListBox.reset(
        new SvxNotebookbarEntriesListBox(m_xBuilder->weld_tree_view(u"toolcontents"_ustr), this));
    m_xDropTargetHelper.reset(
        new SvxConfigPageFunctionDropTarget(*this, m_xContentsListBox->get_widget()));
    weld::TreeView& rTreeView = m_xContentsListBox->get_widget();
    Size aSize(m_xFunctions->get_size_request());
    rTreeView.set_size_request(aSize.Width(), aSize.Height());

    rTreeView.set_hexpand(true);
    rTreeView.set_vexpand(true);
    rTreeView.set_help_id(HID_SVX_CONFIG_NOTEBOOKBAR_CONTENTS);
    rTreeView.show();
}

SvxNotebookbarConfigPage::~SvxNotebookbarConfigPage() {}

void SvxNotebookbarConfigPage::DeleteSelectedTopLevel() {}

void SvxNotebookbarConfigPage::DeleteSelectedContent() {}

void SvxNotebookbarConfigPage::Init()
{
    m_xTopLevelListBox->clear();
    m_xContentsListBox->clear();
    m_xSaveInListBox->clear();
    OUString sNotebookbarInterface = getFileName(m_sFileName);

    OUString sScopeName
        = utl::ConfigManager::getProductName() + " " + m_sAppName + " -  " + sNotebookbarInterface;
    OUString sSaveInListBoxID = notebookbarTabScope;

    m_xSaveInListBox->append(sSaveInListBoxID, sScopeName);
    m_xSaveInListBox->set_active_id(sSaveInListBoxID);

    m_xTopLevelListBox->append(u"NotebookBar"_ustr, CuiResId(RID_CUISTR_ALL_COMMANDS));
    m_xTopLevelListBox->set_active_id(u"NotebookBar"_ustr);
    SelectElement();
}

SaveInData* SvxNotebookbarConfigPage::CreateSaveInData(
    const css::uno::Reference<css::ui::XUIConfigurationManager>& xCfgMgr,
    const css::uno::Reference<css::ui::XUIConfigurationManager>& xParentCfgMgr,
    const OUString& aModuleId, bool bDocConfig)
{
    return static_cast<SaveInData*>(
        new ToolbarSaveInData(xCfgMgr, xParentCfgMgr, aModuleId, bDocConfig));
}

void SvxNotebookbarConfigPage::UpdateButtonStates() {}

short SvxNotebookbarConfigPage::QueryReset()
{
    OUString msg = CuiResId(RID_CUISTR_CONFIRM_TOOLBAR_RESET);

    OUString saveInName = m_xSaveInListBox->get_active_text();

    OUString label = SvxConfigPageHelper::replaceSaveInName(msg, saveInName);

    std::unique_ptr<weld::MessageDialog> xQueryBox(Application::CreateMessageDialog(
        GetFrameWeld(), VclMessageType::Question, VclButtonsType::YesNo, label));
    int nValue = xQueryBox->run();
    if (nValue == RET_YES)
    {
        osl::File::remove(CustomNotebookbarGenerator::getCustomizedUIPath());

        OUString sNotebookbarInterface = getFileName(m_sFileName);
        Sequence<OUString> sSequenceEntries;
        CustomNotebookbarGenerator::setCustomizedUIItem(sSequenceEntries, sNotebookbarInterface);
        OUString sUIPath = "modules/s" + m_sAppName.toAsciiLowerCase() + "/ui/";
        sfx2::SfxNotebookBar::ReloadNotebookBar(sUIPath);
    }
    return nValue;
}

void SvxConfigPage::InsertEntryIntoNotebookbarTabUI(std::u16string_view sClassId,
                                                    const OUString& sUIItemId,
                                                    const OUString& sUIItemCommand,
                                                    weld::TreeView& rTreeView,
                                                    const weld::TreeIter& rIter)
{
    css::uno::Reference<css::container::XNameAccess> m_xCommandToLabelMap;
    const uno::Reference<uno::XComponentContext>& xContext
        = ::comphelper::getProcessComponentContext();
    uno::Reference<container::XNameAccess> xNameAccess(
        css::frame::theUICommandDescription::get(xContext));

    uno::Sequence<beans::PropertyValue> aPropSeq;

    xNameAccess->getByName(getModuleId(m_sAppName)) >>= m_xCommandToLabelMap;

    try
    {
        uno::Any aModuleVal = m_xCommandToLabelMap->getByName(sUIItemCommand);

        aModuleVal >>= aPropSeq;
    }
    catch (container::NoSuchElementException&)
    {
    }

    OUString aLabel;
    for (auto const& prop : aPropSeq)
        if (prop.Name == "Name")
            prop.Value >>= aLabel;

    OUString aName = SvxConfigPageHelper::stripHotKey(aLabel);

    if (sClassId == u"GtkSeparatorMenuItem" || sClassId == u"GtkSeparator")
    {
        rTreeView.set_text(rIter, u"--------------------------------------------"_ustr, 0);
    }
    else
    {
        if (aName.isEmpty())
            aName = sUIItemId;
        auto xImage = GetSaveInData()->GetImage(sUIItemCommand);
        if (xImage.is())
            rTreeView.set_image(rIter, xImage, -1);
        rTreeView.set_text(rIter, aName, 0);
        rTreeView.set_id(rIter, sUIItemId);
    }
}

void SvxNotebookbarConfigPage::getNodeValue(xmlNode* pNodePtr, NotebookbarEntries& aNodeEntries)
{
    pNodePtr = pNodePtr->xmlChildrenNode;
    while (pNodePtr)
    {
        if (!(xmlStrcmp(pNodePtr->name, reinterpret_cast<const xmlChar*>("property"))))
        {
            xmlChar* UriValue = xmlGetProp(pNodePtr, reinterpret_cast<const xmlChar*>("name"));
            if (!(xmlStrcmp(UriValue, reinterpret_cast<const xmlChar*>("visible"))))
            {
                xmlChar* aValue = xmlNodeGetContent(pNodePtr);
                const char* cVisibleValue = reinterpret_cast<const char*>(aValue);
                aNodeEntries.sVisibleValue = charToString(cVisibleValue);
                xmlFree(aValue);
            }
            if (!(xmlStrcmp(UriValue, reinterpret_cast<const xmlChar*>("action_name"))))
            {
                xmlChar* aValue = xmlNodeGetContent(pNodePtr);
                const char* cActionName = reinterpret_cast<const char*>(aValue);
                aNodeEntries.sActionName = charToString(cActionName);
                xmlFree(aValue);
            }
            xmlFree(UriValue);
        }
        pNodePtr = pNodePtr->next;
    }
}

void SvxNotebookbarConfigPage::searchNodeandAttribute(std::vector<NotebookbarEntries>& aEntries,
                                                      std::vector<CategoriesEntries>& aCategoryList,
                                                      OUString& sActiveCategory,
                                                      CategoriesEntries& aCurItemEntry,
                                                      xmlNode* pNodePtr, bool isCategory)
{
    pNodePtr = pNodePtr->xmlChildrenNode;
    while (pNodePtr)
    {
        if (pNodePtr->type == XML_ELEMENT_NODE)
        {
            const char* cNodeName = reinterpret_cast<const char*>(pNodePtr->name);
            if (strcmp(cNodeName, "object") == 0)
            {
                OUString sSecondVal;

                xmlChar* UriValue = xmlGetProp(pNodePtr, reinterpret_cast<const xmlChar*>("id"));
                const char* cUIItemID = reinterpret_cast<const char*>(UriValue);
                OUString sUIItemId = charToString(cUIItemID);
                xmlFree(UriValue);

                UriValue = xmlGetProp(pNodePtr, reinterpret_cast<const xmlChar*>("class"));
                const char* cClassId = reinterpret_cast<const char*>(UriValue);
                OUString sClassId = charToString(cClassId);
                xmlFree(UriValue);

                CategoriesEntries aCategoryEntry;
                if (sClassId == "sfxlo-PriorityHBox")
                {
                    aCategoryEntry.sDisplayName = sUIItemId;
                    aCategoryEntry.sUIItemId = sUIItemId;
                    aCategoryEntry.sClassType = sClassId;
                    aCategoryList.push_back(aCategoryEntry);

                    aCurItemEntry = std::move(aCategoryEntry);
                }
                else if (sClassId == "sfxlo-PriorityMergedHBox")
                {
                    aCategoryEntry.sDisplayName = aCurItemEntry.sDisplayName + " | " + sUIItemId;
                    aCategoryEntry.sUIItemId = sUIItemId;
                    aCategoryEntry.sClassType = sClassId;

                    if (aCurItemEntry.sClassType == sClassId)
                    {
                        sal_Int32 rPos = 0;
                        aCategoryEntry.sDisplayName
                            = OUString::Concat(
                                  o3tl::getToken(aCurItemEntry.sDisplayName, rPos, ' ', rPos))
                              + " | " + sUIItemId;
                    }
                    aCategoryList.push_back(aCategoryEntry);
                    aCurItemEntry = std::move(aCategoryEntry);
                }
                else if (sClassId == "svtlo-ManagedMenuButton")
                {
                    sal_Int32 rPos = 1;
                    sSecondVal = sUIItemId.getToken(rPos, ':', rPos);
                    if (!sSecondVal.isEmpty())
                    {
                        aCategoryEntry.sDisplayName
                            = aCurItemEntry.sDisplayName + " | " + sSecondVal;
                        aCategoryEntry.sUIItemId = sSecondVal;
                        aCategoryList.push_back(aCategoryEntry);
                    }
                }

                NotebookbarEntries nodeEntries;
                if (isCategoryAvailable(sClassId, sUIItemId, sActiveCategory, isCategory)
                    || isCategory)
                {
                    isCategory = true;
                    if (sClassId == "GtkMenuItem" || sClassId == "GtkToolButton"
                        || sClassId == "GtkMenuToolButton"
                        || (sClassId == "svtlo-ManagedMenuButton" && sSecondVal.isEmpty()))
                    {
                        nodeEntries.sClassId = sClassId;
                        nodeEntries.sUIItemId = sUIItemId;
                        nodeEntries.sDisplayName = sUIItemId;

                        getNodeValue(pNodePtr, nodeEntries);
                        aEntries.push_back(nodeEntries);
                    }
                    else if (sClassId == "GtkSeparatorMenuItem" || sClassId == "GtkSeparator")
                    {
                        nodeEntries.sClassId = sClassId;
                        nodeEntries.sUIItemId = sUIItemId;
                        nodeEntries.sDisplayName = "Null";
                        nodeEntries.sVisibleValue = "Null";
                        nodeEntries.sActionName = "Null";
                        aEntries.push_back(nodeEntries);
                    }
                    else if (sClassId == "sfxlo-PriorityHBox"
                             || sClassId == "sfxlo-PriorityMergedHBox"
                             || sClassId == "svtlo-ManagedMenuButton")
                    {
                        nodeEntries.sClassId = sClassId;
                        nodeEntries.sUIItemId = sUIItemId;
                        nodeEntries.sDisplayName
                            = aCategoryList[aCategoryList.size() - 1].sDisplayName;
                        nodeEntries.sVisibleValue = "Null";
                        nodeEntries.sActionName = "Null";
                        aEntries.push_back(nodeEntries);
                    }
                }
            }
            searchNodeandAttribute(aEntries, aCategoryList, sActiveCategory, aCurItemEntry,
                                   pNodePtr, isCategory);
        }
        pNodePtr = pNodePtr->next;
    }
}

void SvxNotebookbarConfigPage::FillFunctionsList(xmlNodePtr pRootNodePtr,
                                                 std::vector<NotebookbarEntries>& aEntries,
                                                 std::vector<CategoriesEntries>& aCategoryList,
                                                 OUString& sActiveCategory)
{
    CategoriesEntries aCurItemEntry;
    searchNodeandAttribute(aEntries, aCategoryList, sActiveCategory, aCurItemEntry, pRootNodePtr,
                           false);
}

void SvxNotebookbarConfigPage::SelectElement()
{
    OString sUIFileUIPath = CustomNotebookbarGenerator::getSystemPath(
        CustomNotebookbarGenerator::getCustomizedUIPath());
    xmlDocPtr pDoc = xmlParseFile(sUIFileUIPath.getStr());
    if (!pDoc)
    {
        sUIFileUIPath = CustomNotebookbarGenerator::getSystemPath(
            CustomNotebookbarGenerator::getOriginalUIPath());
        pDoc = xmlParseFile(sUIFileUIPath.getStr());
    }

    if (!pDoc)
        return;
    xmlNodePtr pNodePtr = xmlDocGetRootElement(pDoc);

    std::vector<NotebookbarEntries> aEntries;
    std::vector<CategoriesEntries> aCategoryList;
    OUString sActiveCategory = m_xTopLevelListBox->get_active_id();
    FillFunctionsList(pNodePtr, aEntries, aCategoryList, sActiveCategory);

    if (m_xTopLevelListBox->get_count() == 1)
    {
        for (const auto& rCategory : aCategoryList)
            m_xTopLevelListBox->append(rCategory.sUIItemId, rCategory.sDisplayName);
    }
    tools::ULong nStart = 0;
    if (aEntries[nStart].sClassId == "sfxlo-PriorityHBox"
        || aEntries[nStart].sClassId == "sfxlo-PriorityMergedHBox")
        nStart = 1;

    std::vector<NotebookbarEntries> aTempEntries;
    for (std::size_t nIdx = nStart; nIdx < aEntries.size(); nIdx++)
    {
        if (aEntries[nIdx].sClassId == "svtlo-ManagedMenuButton")
        {
            aTempEntries.push_back(aEntries[nIdx]);
            sal_Int32 rPos = 1;
            sActiveCategory = aEntries[nIdx].sUIItemId.getToken(rPos, ':', rPos);
            FillFunctionsList(pNodePtr, aTempEntries, aCategoryList, sActiveCategory);
        }
        else
            aTempEntries.push_back(aEntries[nIdx]);
    }

    aEntries = std::move(aTempEntries);

    static_cast<SvxNotebookbarEntriesListBox*>(m_xContentsListBox.get())->GetTooltipMap().clear();
    weld::TreeView& rTreeView = m_xContentsListBox->get_widget();
    rTreeView.bulk_insert_for_each(
        aEntries.size(), [this, &rTreeView, &aEntries](weld::TreeIter& rIter, int nIdx) {
            if (aEntries[nIdx].sActionName != "Null")
            {
                if (aEntries[nIdx].sVisibleValue == "True")
                {
                    rTreeView.set_toggle(rIter, TRISTATE_TRUE);
                }
                else
                {
                    rTreeView.set_toggle(rIter, TRISTATE_FALSE);
                }
            }
            InsertEntryIntoNotebookbarTabUI(aEntries[nIdx].sClassId, aEntries[nIdx].sDisplayName,
                                            aEntries[nIdx].sActionName, rTreeView, rIter);
            if (aEntries[nIdx].sClassId != u"GtkSeparatorMenuItem"
                && aEntries[nIdx].sClassId != u"GtkSeparator")
            {
                static_cast<SvxNotebookbarEntriesListBox*>(m_xContentsListBox.get())
                    ->GetTooltipMap()[aEntries[nIdx].sDisplayName]
                    = aEntries[nIdx].sActionName;
            }
        });

    aEntries.clear();

    xmlFreeDoc(pDoc);
}

SvxNotebookbarEntriesListBox::SvxNotebookbarEntriesListBox(std::unique_ptr<weld::TreeView> xParent,
                                                           SvxConfigPage* pPg)
    : SvxMenuEntriesListBox(std::move(xParent), pPg)
{
    m_xControl->connect_toggled(LINK(this, SvxNotebookbarEntriesListBox, CheckButtonHdl));
    m_xControl->connect_key_press(Link<const KeyEvent&, bool>());
    m_xControl->connect_key_press(LINK(this, SvxNotebookbarEntriesListBox, KeyInputHdl));
    // remove the inherited connect_query_tooltip then add the new one
    m_xControl->connect_query_tooltip(Link<const weld::TreeIter&, OUString>());
    m_xControl->connect_query_tooltip(LINK(this, SvxNotebookbarEntriesListBox, QueryTooltip));
}

SvxNotebookbarEntriesListBox::~SvxNotebookbarEntriesListBox() {}

static void EditRegistryFile(std::u16string_view sUIItemId, const OUString& sSetEntry,
                             const OUString& sNotebookbarInterface)
{
    int nFlag = 0;
    Sequence<OUString> aOldEntries
        = CustomNotebookbarGenerator::getCustomizedUIItem(sNotebookbarInterface);
    Sequence<OUString> aNewEntries(aOldEntries.getLength() + 1);
    auto pNewEntries = aNewEntries.getArray();
    for (int nIdx = 0; nIdx < aOldEntries.getLength(); nIdx++)
    {
        sal_Int32 rPos = 0;
        std::u16string_view sFirstValue = o3tl::getToken(aOldEntries[nIdx], rPos, ',', rPos);
        if (sFirstValue == sUIItemId)
        {
            aOldEntries.getArray()[nIdx] = sSetEntry;
            nFlag = 1;
            break;
        }
        pNewEntries[nIdx] = aOldEntries[nIdx];
    }

    if (nFlag == 0)
    {
        pNewEntries[aOldEntries.getLength()] = sSetEntry;
        CustomNotebookbarGenerator::setCustomizedUIItem(aNewEntries, sNotebookbarInterface);
    }
    else
    {
        CustomNotebookbarGenerator::setCustomizedUIItem(aOldEntries, sNotebookbarInterface);
    }
}

void SvxNotebookbarEntriesListBox::ChangedVisibility(int nRow)
{
    OUString sUIItemId = m_xControl->get_selected_id();
    OUString sNotebookbarInterface = getFileName(m_pPage->GetFileName());

    OUString sVisible;
    if (m_xControl->get_toggle(nRow) == TRISTATE_TRUE)
        sVisible = "True";
    else
        sVisible = "False";
    OUString sSetEntries = sUIItemId + ",visible," + sVisible;
    Sequence<OUString> sSeqOfEntries{ sSetEntries };
    EditRegistryFile(sUIItemId, sSetEntries, sNotebookbarInterface);
    CustomNotebookbarGenerator::modifyCustomizedUIFile(sSeqOfEntries);
    OUString sUIPath = "modules/s" + m_pPage->GetAppName().toAsciiLowerCase() + "/ui/";
    sfx2::SfxNotebookBar::ReloadNotebookBar(sUIPath);
}

IMPL_LINK(SvxNotebookbarEntriesListBox, CheckButtonHdl, const weld::TreeView::iter_col&, rRowCol,
          void)
{
    ChangedVisibility(m_xControl->get_iter_index_in_parent(rRowCol.first));
}

IMPL_LINK(SvxNotebookbarEntriesListBox, KeyInputHdl, const KeyEvent&, rKeyEvent, bool)
{
    if (rKeyEvent.GetKeyCode() == KEY_SPACE)
    {
        int nRow = m_xControl->get_selected_index();
        m_xControl->set_toggle(nRow, m_xControl->get_toggle(nRow) == TRISTATE_TRUE ? TRISTATE_FALSE
                                                                                   : TRISTATE_TRUE);
        ChangedVisibility(nRow);
        return true;
    }
    return SvxMenuEntriesListBox::KeyInputHdl(rKeyEvent);
}

IMPL_LINK(SvxNotebookbarEntriesListBox, QueryTooltip, const weld::TreeIter&, rIter, OUString)
{
    const OUString& rsCommand = m_aTooltipMap[m_xControl->get_id(rIter)];
    if (rsCommand.isEmpty())
        return OUString();
    OUString aModuleName(vcl::CommandInfoProvider::GetModuleIdentifier(m_pPage->GetFrame()));
    auto aProperties = vcl::CommandInfoProvider::GetCommandProperties(rsCommand, aModuleName);
    OUString sTooltipLabel = vcl::CommandInfoProvider::GetTooltipForCommand(rsCommand, aProperties,
                                                                            m_pPage->GetFrame());
    return CuiResId(RID_CUISTR_COMMANDLABEL) + ": "
           + m_xControl->get_text(rIter).replaceFirst("~", "") + "\n"
           + CuiResId(RID_CUISTR_COMMANDNAME) + ": " + rsCommand + "\n"
           + CuiResId(RID_CUISTR_COMMANDTIP) + ": " + sTooltipLabel.replaceFirst("~", "");
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
