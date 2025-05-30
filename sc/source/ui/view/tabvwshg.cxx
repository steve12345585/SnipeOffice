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

#include <config_features.h>

#include <tools/urlobj.hxx>
#include <svx/svdobjkind.hxx>
#include <svx/svdouno.hxx>
#include <sfx2/docfile.hxx>
#include <osl/diagnose.h>

#include <com/sun/star/form/FormButtonType.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/awt/XControlModel.hpp>

#include <tabvwsh.hxx>
#include <document.hxx>
#include <drawview.hxx>
#include <globstr.hrc>
#include <gridwin.hxx>
#include <avmedia/mediawindow.hxx>

using namespace com::sun::star;

void ScTabViewShell::InsertURLButton( const OUString& rName, const OUString& rURL,
                                        const OUString& rTarget,
                                        const Point* pInsPos )
{
    // protected sheet ?

    ScViewData& rViewData = GetViewData();
    ScDocument& rDoc = rViewData.GetDocument();
    SCTAB nTab = rViewData.GetTabNo();
    if ( rDoc.IsTabProtected(nTab) )
    {
        ErrorMessage(STR_PROTECTIONERR);
        return;
    }

    MakeDrawLayer();

    ScTabView*  pView   = rViewData.GetView();
    ScDrawView* pDrView = pView->GetScDrawView();
    SdrModel& rModel = pDrView->GetModel();

    rtl::Reference<SdrObject> pObj = SdrObjFactory::MakeNewObject(
        rModel,
        SdrInventor::FmForm,
        SdrObjKind::FormButton);

    SdrUnoObj* pUnoCtrl = dynamic_cast<SdrUnoObj*>( pObj.get() );
    OSL_ENSURE( pUnoCtrl, "no SdrUnoObj");
    if( !pUnoCtrl )
        return;

    uno::Reference<awt::XControlModel> xControlModel = pUnoCtrl->GetUnoControlModel();
    OSL_ENSURE( xControlModel.is(), "UNO control without model" );
    if( !xControlModel.is() )
        return;

    uno::Reference< beans::XPropertySet > xPropSet( xControlModel, uno::UNO_QUERY );

    xPropSet->setPropertyValue(u"Label"_ustr, uno::Any(rName) );

    OUString aTmp = INetURLObject::GetAbsURL( rDoc.GetDocumentShell()->GetMedium()->GetBaseURL(), rURL );
    xPropSet->setPropertyValue(u"TargetURL"_ustr, uno::Any(aTmp) );

    if( !rTarget.isEmpty() )
    {
        xPropSet->setPropertyValue(u"TargetFrame"_ustr, uno::Any(rTarget) );
    }

    xPropSet->setPropertyValue(u"ButtonType"_ustr, uno::Any(form::FormButtonType_URL) );

#if HAVE_FEATURE_AVMEDIA
    if ( ::avmedia::MediaWindow::isMediaURL( rURL, u""_ustr/*TODO?*/ ) )
    {
        xPropSet->setPropertyValue(u"DispatchURLInternal"_ustr, uno::Any(true) );
    }
#endif

    Point aPos;
    if (pInsPos)
        aPos = *pInsPos;
    else
        aPos = GetInsertPos();

    // Size as in 3.1:
    Size aSize = GetActiveWin()->PixelToLogic(Size(140, 20));

    if ( rDoc.IsNegativePage(nTab) )
        aPos.AdjustX( -(aSize.Width()) );

    pObj->SetLogicRect(tools::Rectangle(aPos, aSize));

    // for the old VC-Button the position/size had to be set explicitly once more
    // that seems not to be needed with UnoControls

    // do not mark when Ole
    pDrView->InsertObjectSafe( pObj.get(), *pDrView->GetSdrPageView() );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
