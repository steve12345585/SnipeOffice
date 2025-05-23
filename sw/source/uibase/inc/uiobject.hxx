/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef SW_SOURCE_UIBASE_INC_UIOBJECT_HXX
#define SW_SOURCE_UIBASE_INC_UIOBJECT_HXX

#include <memory>
#include <vcl/uitest/uiobject.hxx>

#include "edtwin.hxx"
#include "PageBreakWin.hxx"

#include <AnnotationWin.hxx>

class SwEditWinUIObject final : public WindowUIObject
{
public:

    SwEditWinUIObject(const VclPtr<SwEditWin>& xEditWin);

    virtual StringMap get_state() override;

    virtual void execute(const OUString& rAction,
            const StringMap& rParameters) override;

    static std::unique_ptr<UIObject> create(vcl::Window* pWindow);

    virtual OUString get_name() const override;

private:

    VclPtr<SwEditWin> mxEditWin;

};

// This class handles the Comments as a UIObject to be used in UITest Framework
class CommentUIObject final : public WindowUIObject
{
    VclPtr<sw::annotation::SwAnnotationWin> mxCommentUIObject;

public:

    CommentUIObject(const VclPtr<sw::annotation::SwAnnotationWin>& xCommentUIObject);

    virtual StringMap get_state() override;

    virtual void execute(const OUString& rAction,
            const StringMap& rParameters) override;

    static std::unique_ptr<UIObject> create(vcl::Window* pWindow);

private:

    OUString get_name() const override;

};

class PageBreakUIObject final : public WindowUIObject
{
public:

    PageBreakUIObject(const VclPtr<SwBreakDashedLine>& xEditWin);

    virtual void execute(const OUString& rAction,
            const StringMap& rParameters) override;

    static std::unique_ptr<UIObject> create(vcl::Window* pWindow);

private:

    virtual OUString get_name() const override;

    VclPtr<SwBreakDashedLine> mxPageBreakUIObject;

};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
