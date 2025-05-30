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

#include <salwtype.hxx>
#include <vcl/dllapi.h>
#include <salusereventlist.hxx>

class SalFrame;
class VCL_DLLPUBLIC SalGenericDisplay : public SalUserEventList
{
protected:
    SalFrame* m_pCapture;

    virtual void ProcessEvent( SalUserEvent aEvent ) override;

public:
                 SalGenericDisplay();
    virtual      ~SalGenericDisplay() override;

    void registerFrame( SalFrame* pFrame );
    virtual void deregisterFrame( SalFrame* pFrame );
    void emitDisplayChanged();

    void SendInternalEvent( SalFrame* pFrame, void* pData, SalEvent nEvent = SalEvent::UserEvent );
    void CancelInternalEvent( SalFrame* pFrame, void* pData, SalEvent nEvent );
    bool DispatchInternalEvent( bool bHandleAllCurrentEvent = false );

    bool     MouseCaptured( const SalFrame *pFrameData ) const
                        { return m_pCapture == pFrameData; }
    SalFrame*    GetCaptureFrame() const
                        { return m_pCapture; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
