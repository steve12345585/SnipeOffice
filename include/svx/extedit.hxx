/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SVX_EXTEDIT_HXX
#define INCLUDED_SVX_EXTEDIT_HXX

#include <svx/svxdllapi.h>
#include <svx/svdograf.hxx>
#include <svl/lstner.hxx>
#include <rtl/ustring.hxx>
#include <rtl/ref.hxx>
#include <memory>

class Graphic;
class GraphicObject;
class FileChangedChecker;

class SAL_WARN_UNUSED SVXCORE_DLLPUBLIC ExternalToolEdit
{
protected:
    OUString m_aFileName;

    ::std::unique_ptr<FileChangedChecker> m_pChecker;

public:

    ExternalToolEdit();
    virtual ~ExternalToolEdit();

    virtual void Update( Graphic& aGraphic ) = 0;
    void Edit(GraphicObject const*const pGraphic);

    void StartListeningEvent();

    static void HandleCloseEvent( ExternalToolEdit* pData );
};

class FmFormView;

class SAL_WARN_UNUSED SVXCORE_DLLPUBLIC SdrExternalToolEdit final
:   public ExternalToolEdit
    ,public SfxListener
{
private:
    FmFormView* m_pView;
    rtl::Reference<SdrGrafObj>  m_pObj;

    SAL_DLLPRIVATE virtual void Update(Graphic&) override;
    SAL_DLLPRIVATE virtual void Notify(SfxBroadcaster&, const SfxHint&) override;

public:
    SdrExternalToolEdit(
        FmFormView* pView,
        SdrGrafObj* pObj);
};

#endif
