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

#include <ado/AView.hxx>
#include <com/sun/star/lang/DisposedException.hpp>
#include <ado/adoimp.hxx>
#include <ado/Awrapado.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/types.hxx>
#include <systools/win32/oleauto.hxx>

#include <TConnection.hxx>


using namespace comphelper;
using namespace connectivity::ado;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::sdbc;

//  IMPLEMENT_SERVICE_INFO(OAdoView,"com.sun.star.sdbcx.AView","com.sun.star.sdbcx.View");

OAdoView::OAdoView(bool _bCase,ADOView* _pView) : OView_ADO(_bCase,nullptr)
,m_aView(_pView)
{
}

void OAdoView::getFastPropertyValue(Any& rValue,sal_Int32 nHandle) const
{
    if(m_aView.IsValid())
    {
        switch(nHandle)
        {
            case PROPERTY_ID_NAME:
                rValue <<= m_aView.get_Name();
                break;
            case PROPERTY_ID_CATALOGNAME:
                break;
            case PROPERTY_ID_SCHEMANAME:
                //  rValue <<= m_aView.get_Type();
                break;
            case PROPERTY_ID_COMMAND:
                {
                    OLEVariant aVar;
                    m_aView.get_Command(aVar);
                    if(!aVar.isNull() && !aVar.isEmpty())
                    {
                        ADOCommand* pCom = static_cast<ADOCommand*>(aVar.getIDispatch());
                        sal::systools::BStr aBSTR;
                        pCom->get_CommandText(&aBSTR);
                        rValue <<= OUString(aBSTR);
                    }
                }
                break;
        }
    }
    else
        OView_ADO::getFastPropertyValue(rValue,nHandle);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
