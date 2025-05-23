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

#include <sdr/contact/viewobjectcontactofgraphic.hxx>
#include <sdr/contact/viewcontactofgraphic.hxx>
#include <svx/sdr/contact/objectcontact.hxx>

namespace sdr::contact
{
        void ViewObjectContactOfGraphic::createPrimitive2DSequence(const DisplayInfo& rDisplayInfo, drawinglayer::primitive2d::Primitive2DDecompositionVisitor& rVisitor) const
        {
            // #i103255# suppress when graphic needs draft visualisation and output
            // is for PDF export/Printer
            const ViewContactOfGraphic& rVCOfGraphic = static_cast< const ViewContactOfGraphic& >(GetViewContact());

            if(rVCOfGraphic.visualisationUsesDraft())
            {
                const ObjectContact& rObjectContact = GetObjectContact();

                if(rObjectContact.isOutputToPDFFile() || rObjectContact.isOutputToPrinter())
                {
                    return;
                }
            }

            // get return value by calling parent
            ViewObjectContactOfSdrObj::createPrimitive2DSequence(rDisplayInfo, rVisitor);
        }

        ViewObjectContactOfGraphic::ViewObjectContactOfGraphic(ObjectContact& rObjectContact, ViewContact& rViewContact)
        :   ViewObjectContactOfSdrObj(rObjectContact, rViewContact)
        {
        }

        ViewObjectContactOfGraphic::~ViewObjectContactOfGraphic()
        {
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
