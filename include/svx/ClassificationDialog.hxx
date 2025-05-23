/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#ifndef INCLUDED_SVX_CLASSIFICATIONDIALOG_HXX
#define INCLUDED_SVX_CLASSIFICATIONDIALOG_HXX

#include <sal/config.h>
#include <vcl/weld.hxx>
#include <svx/svxdllapi.h>
#include <sfx2/classificationhelper.hxx>
#include <svx/ClassificationField.hxx>

namespace svx
{
class ClassificationEditView;
}
namespace weld
{
class CustomWeld;
}

namespace svx
{
class SVX_DLLPUBLIC ClassificationDialog final : public weld::GenericDialogController
{
private:
    SfxClassificationHelper maHelper;
    SfxClassificationHelper maInternationalHelper;

    const bool m_bPerParagraph;
    const std::function<void()> m_aParagraphSignHandler;

    ImplSVEvent* m_nAsyncExpandEvent;
    sal_Int32 m_nCurrentSelectedCategory;

    std::vector<std::vector<ClassificationResult>> m_aRecentlyUsedValuesCollection;
    std::vector<ClassificationResult> m_aInitialValues;

    std::unique_ptr<weld::Button> m_xOkButton;
    std::unique_ptr<weld::Button> m_xSignButton;
    std::unique_ptr<weld::ToggleButton> m_xToolBox;
    std::unique_ptr<weld::ComboBox> m_xRecentlyUsedListBox;
    std::unique_ptr<weld::ComboBox> m_xClassificationListBox;
    std::unique_ptr<weld::ComboBox> m_xInternationalClassificationListBox;
    std::unique_ptr<weld::Label> m_xMarkingLabel;
    std::unique_ptr<weld::TreeView> m_xMarkingListBox;
    std::unique_ptr<weld::TreeView> m_xIntellectualPropertyPartListBox;
    std::unique_ptr<weld::TreeView> m_xIntellectualPropertyPartNumberListBox;
    std::unique_ptr<weld::Button> m_xIntellectualPropertyPartAddButton;
    std::unique_ptr<weld::Entry> m_xIntellectualPropertyPartEdit;
    std::unique_ptr<weld::Expander> m_xIntellectualPropertyExpander;
    std::unique_ptr<ClassificationEditView> m_xEditWindow;
    std::unique_ptr<weld::CustomWeld> m_xEditWindowWeld;

    DECL_DLLPRIVATE_LINK(ButtonClicked, weld::Button&, void);
    DECL_DLLPRIVATE_LINK(OkHdl, weld::Button&, void);
    DECL_DLLPRIVATE_LINK(SelectToolboxHdl, weld::Toggleable&, void);
    DECL_DLLPRIVATE_LINK(SelectClassificationHdl, weld::ComboBox&, void);
    DECL_DLLPRIVATE_LINK(SelectMarkingHdl, weld::TreeView&, bool);
    DECL_DLLPRIVATE_LINK(SelectIPPartNumbersHdl, weld::TreeView&, bool);
    DECL_DLLPRIVATE_LINK(SelectRecentlyUsedHdl, weld::ComboBox&, void);
    DECL_DLLPRIVATE_LINK(SelectIPPartHdl, weld::TreeView&, bool);
    DECL_DLLPRIVATE_LINK(EditWindowModifiedHdl, LinkParamNone*, void);
    DECL_DLLPRIVATE_STATIC_LINK(ClassificationDialog, ExpandedHdl, weld::Expander&, void);
    DECL_DLLPRIVATE_STATIC_LINK(ClassificationDialog, KeyInput, const KeyEvent&, bool);
    DECL_DLLPRIVATE_LINK(OnAsyncExpandHdl, void*, void);

    void insertField(ClassificationType eType, OUString const& rString, OUString const& rFullString,
                     OUString const& rIdentifier = OUString());
    void insertCategoryField(sal_Int32 nID);

    void readIn(std::vector<ClassificationResult> const& rInput);
    void readRecentlyUsed();
    void writeRecentlyUsed();
    void toggleWidgetsDependingOnCategory();

public:
    ClassificationDialog(weld::Window* pParent,
                         const css::uno::Reference<css::document::XDocumentProperties>& rDocProps,
                         bool bPerParagraph, std::function<void()> aParagraphSignHandler = []() {});
    ~ClassificationDialog() override;

    std::vector<ClassificationResult> getResult();
    void setupValues(std::vector<ClassificationResult>&& rInput);
};

} // end svx namespace

#endif // INCLUDED_SVX_CLASSIFICATIONDIALOG_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
