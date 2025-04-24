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

#include <sal/config.h>

#include <string_view>

#include <bastype2.hxx>
#include <vcl/weld.hxx>
#include <com/sun/star/task/XInteractionHandler.hpp>

#include <basctl/basctldllpublic.hxx>

class SvxPasswordDialog;

namespace basctl
{

enum class ObjectMode
{
    Library = 1,
    Module  = 2,
    Dialog  = 3,
};

class NewObjectDialog : public weld::GenericDialogController
{
private:
    std::unique_ptr<weld::Entry> m_xEdit;
    std::unique_ptr<weld::Button> m_xOKButton;
    bool m_bCheckName;

    DECL_LINK(OkButtonHandler, weld::Button&, void);
public:
    NewObjectDialog(weld::Window* pParent, ObjectMode, bool bCheckName = false);
    OUString GetObjectName() const { return m_xEdit->get_text(); }
    void SetObjectName(const OUString& rName)
    {
        m_xEdit->set_text(rName);
        m_xEdit->select_region(0, -1);
    }
};

class GotoLineDialog : public weld::GenericDialogController
{
    std::unique_ptr<weld::SpinButton> m_xSpinButton;
    std::unique_ptr<weld::Label> m_xLineCount;
    std::unique_ptr<weld::Button> m_xOKButton;
    sal_uInt32 m_nCurLine;
    sal_uInt32 m_nLineCount;

    DECL_LINK(OkButtonHandler, weld::Button&, void);
public:
    explicit GotoLineDialog(weld::Window* pParent, sal_uInt32 nCurLine, sal_uInt32 nLineCount);
    virtual ~GotoLineDialog() override;
    sal_Int32 GetLineNumber() const;
};

class ExportDialog : public weld::GenericDialogController
{
private:
    bool m_bExportAsPackage;

    std::unique_ptr<weld::RadioButton> m_xExportAsPackageButton;
    std::unique_ptr<weld::Button> m_xOKButton;

    DECL_LINK(OkButtonHandler, weld::Button&, void);

public:
    explicit ExportDialog(weld::Window * pParent);
    virtual ~ExportDialog() override;

    bool isExportAsPackage () const { return m_bExportAsPackage; }
};

class LibDialog : public weld::GenericDialogController
{
private:
    std::unique_ptr<weld::Frame> m_xStorageFrame;
    std::unique_ptr<weld::TreeView> m_xLibBox;
    std::unique_ptr<weld::CheckButton> m_xReferenceBox;
    std::unique_ptr<weld::CheckButton> m_xReplaceBox;

public:
    explicit LibDialog(weld::Window* pParent);
    virtual ~LibDialog() override;

    void            SetStorageName( std::u16string_view rName );

    weld::TreeView& GetLibBox()                 { return *m_xLibBox; }
    bool            IsReference() const         { return m_xReferenceBox->get_active(); }
    bool            IsReplace() const           { return m_xReplaceBox->get_active(); }

    void            EnableReference (bool b)    { m_xReferenceBox->set_sensitive(b); }
};

class OrganizeDialog;

class OrganizePage
{
protected:
    OrganizeDialog* m_pDialog;
    std::unique_ptr<weld::Builder> m_xBuilder;
    std::unique_ptr<weld::Container> m_xContainer;

    OrganizePage(weld::Container* pParent, const OUString& rUIFile, const OUString &rName, OrganizeDialog* pDialog);
    virtual ~OrganizePage();

public:
    virtual void        ActivatePage() = 0;
};

class SbTreeListBoxDropTarget;

class ObjectPage final : public OrganizePage
{
    std::unique_ptr<SbTreeListBox> m_xBasicBox;
    std::unique_ptr<weld::Button> m_xEditButton;
    std::unique_ptr<weld::Button> m_xNewModButton;
    std::unique_ptr<weld::Button> m_xNewDlgButton;
    std::unique_ptr<weld::Button> m_xDelButton;
    std::unique_ptr<SbTreeListBoxDropTarget> m_xDropTarget;

    DECL_LINK( BasicBoxHighlightHdl, weld::TreeView&, void );
    DECL_LINK( ButtonHdl, weld::Button&, void );
    DECL_LINK( EditingEntryHdl, const weld::TreeIter&, bool );
    typedef std::pair<const weld::TreeIter&, OUString> IterString;
    DECL_LINK( EditedEntryHdl, const IterString&, bool );

    void                CheckButtons();
    bool                GetSelection( ScriptDocument& rDocument, OUString& rLibName );
    void                DeleteCurrent();
    void                NewModule();
    void                NewDialog();
    void                EndTabDialog();

public:
    ObjectPage(weld::Container* pParent, const OUString& rName, BrowseMode nMode, OrganizeDialog* pDialog);
    virtual ~ObjectPage() override;

    void                SetCurrentEntry(const EntryDescriptor& rDesc) { m_xBasicBox->SetCurrentEntry(rDesc); }

    virtual void        ActivatePage() override;
};

class LibPage final : public OrganizePage
{
    std::unique_ptr<weld::ComboBox> m_xBasicsBox;
    std::unique_ptr<weld::TreeView> m_xLibBox;
    std::unique_ptr<weld::Button> m_xEditButton;
    std::unique_ptr<weld::Button> m_xPasswordButton;
    std::unique_ptr<weld::Button> m_xNewLibButton;
    std::unique_ptr<weld::Button> m_xInsertLibButton;
    std::unique_ptr<weld::Button> m_xExportButton;
    std::unique_ptr<weld::Button> m_xDelButton;

    ScriptDocument      m_aCurDocument;
    LibraryLocation     m_eCurLocation;

    DECL_LINK( TreeListHighlightHdl, weld::TreeView&, void );
    DECL_LINK( BasicSelectHdl, weld::ComboBox&, void );
    DECL_LINK( ButtonHdl, weld::Button&, void );
    DECL_LINK( CheckPasswordHdl, SvxPasswordDialog *, bool );
    DECL_LINK( EditingEntryHdl, const weld::TreeIter&, bool );
    typedef std::pair<const weld::TreeIter&, OUString> IterString;
    DECL_LINK( EditedEntryHdl, const IterString&, bool );

    void                CheckButtons();
    void                DeleteCurrent();
    void                NewLib();
    void                InsertLib();
    void                EndTabDialog();
    void                FillListBox();
    void                InsertListBoxEntry( const ScriptDocument& rDocument, LibraryLocation eLocation );
    void                SetCurLib();
    void                ImpInsertLibEntry( const OUString& rLibName, int nPos );

public:
    explicit LibPage(weld::Container* pParent, OrganizeDialog* pDialog);
    virtual             ~LibPage() override;
    virtual void        ActivatePage() override;
};

void implExportLib(const ScriptDocument& rScriptDocument, const OUString& aLibName,
                   const OUString& aTargetURL,
                   const css::uno::Reference<css::task::XInteractionHandler>& Handler);
void ExportAsPackage(const ScriptDocument& rScriptDocument, const OUString& aLibName,
                     weld::Dialog* pDialog);
void ExportAsBasic(const ScriptDocument& rScriptDocument, const OUString& aLibName,
                   weld::Dialog* pDialog);

class OrganizeDialog : public weld::GenericDialogController
{
private:
    std::unique_ptr<weld::Notebook> m_xTabCtrl;
    std::unique_ptr<ObjectPage> m_xModulePage;
    std::unique_ptr<ObjectPage> m_xDialogPage;
    std::unique_ptr<LibPage> m_xLibPage;

    DECL_LINK(ActivatePageHdl, const OUString&, void);

    void SetCurrentEntry(const css::uno::Reference<css::frame::XFrame>& xDocFrame);

public:
    OrganizeDialog(weld::Window* pParent, const css::uno::Reference<css::frame::XFrame>& xDocFrame, sal_Int16 tabId);
    virtual ~OrganizeDialog() override;
};

// Helper functions
SbModule* createModImpl(weld::Window* pWin, const ScriptDocument& rDocument,
                        SbTreeListBox& rBasicBox, const OUString& rLibName, const OUString& aModName, bool bMain);
void createLibImpl(weld::Window* pWin, const ScriptDocument& rDocument,
                   weld::TreeView* pLibBox, SbTreeListBox* pBasicBox);

} // namespace basctl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
