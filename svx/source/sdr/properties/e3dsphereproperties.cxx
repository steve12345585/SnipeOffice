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

#include <sdr/properties/e3dsphereproperties.hxx>
#include <svx/sphere3d.hxx>


namespace sdr::properties
{
        E3dSphereProperties::E3dSphereProperties(SdrObject& rObj)
        :   E3dCompoundProperties(rObj)
        {
        }

        E3dSphereProperties::E3dSphereProperties(const E3dSphereProperties& rProps, SdrObject& rObj)
        :   E3dCompoundProperties(rProps, rObj)
        {
        }

        E3dSphereProperties::~E3dSphereProperties()
        {
        }

        std::unique_ptr<BaseProperties> E3dSphereProperties::Clone(SdrObject& rObj) const
        {
            return std::unique_ptr<BaseProperties>(new E3dSphereProperties(*this, rObj));
        }

        void E3dSphereProperties::PostItemChange(const sal_uInt16 nWhich)
        {
            // call parent
            E3dCompoundProperties::PostItemChange(nWhich);

            // handle value change
            E3dSphereObj& rObj = static_cast<E3dSphereObj&>(GetSdrObject());

            switch(nWhich)
            {
                case SDRATTR_3DOBJ_HORZ_SEGS:
                {
                    rObj.ActionChanged();
                    break;
                }
                case SDRATTR_3DOBJ_VERT_SEGS:
                {
                    rObj.ActionChanged();
                    break;
                }
            }
        }
} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
