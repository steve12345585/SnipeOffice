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

#ifndef INCLUDED_SVX_INC_SDR_CONTACT_VIEWOBJECTCONTACTOFE3DSCENE_HXX
#define INCLUDED_SVX_INC_SDR_CONTACT_VIEWOBJECTCONTACTOFE3DSCENE_HXX

#include <svx/sdr/contact/viewobjectcontactofsdrobj.hxx>


namespace sdr::contact
    {
        class ViewObjectContactOfE3dScene final : public ViewObjectContactOfSdrObj
        {
            // 2d primitive creator
            virtual void createPrimitive2DSequence(const DisplayInfo& rDisplayInfo, drawinglayer::primitive2d::Primitive2DDecompositionVisitor& rVisitor) const override;

        public:
            // basic constructor.
            ViewObjectContactOfE3dScene(ObjectContact& rObjectContact, ViewContact& rViewContact);
            virtual ~ViewObjectContactOfE3dScene() override;

            // process this primitive: Eventually also recursively travel an existing hierarchy,
            // e.g. for group objects, scenes or pages. This method will test geometrical visibility.
            virtual void getPrimitive2DSequenceHierarchy(DisplayInfo& rDisplayInfo, drawinglayer::primitive2d::Primitive2DDecompositionVisitor& rVisitor ) const override;
        };
} // end of namespace sdr::contact


#endif // INCLUDED_SVX_INC_SDR_CONTACT_VIEWOBJECTCONTACTOFE3DSCENE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
