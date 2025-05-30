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

#ifndef INCLUDED_DRAWINGLAYER_ATTRIBUTE_SDRSHADOWATTRIBUTE_HXX
#define INCLUDED_DRAWINGLAYER_ATTRIBUTE_SDRSHADOWATTRIBUTE_HXX

#include <drawinglayer/drawinglayerdllapi.h>
#include <o3tl/cow_wrapper.hxx>


// predefines

namespace basegfx {
    class BColor;
    class B2DVector;
}

namespace model {
    enum class RectangleAlignment;
}

namespace drawinglayer::attribute {
    class ImpSdrShadowAttribute;
}


namespace drawinglayer::attribute
    {
        class DRAWINGLAYER_DLLPUBLIC SdrShadowAttribute
        {
        public:
            typedef o3tl::cow_wrapper< ImpSdrShadowAttribute > ImplType;

        private:
            ImplType mpSdrShadowAttribute;

        public:
            /// constructors/assignmentoperator/destructor
            SdrShadowAttribute(
                const basegfx::B2DVector& rOffset,
                const basegfx::B2DVector& rSize,
                double fTransparence,
                sal_Int32 nBlur,
                model::RectangleAlignment eAlignment,
                const basegfx::BColor& rColor);
            SdrShadowAttribute();
            SdrShadowAttribute(const SdrShadowAttribute&);
            SdrShadowAttribute(SdrShadowAttribute&&);
            SdrShadowAttribute& operator=(const SdrShadowAttribute&);
            SdrShadowAttribute& operator=(SdrShadowAttribute&&);
            ~SdrShadowAttribute();

            // checks if the incarnation is default constructed
            bool isDefault() const;

            // compare operator
            bool operator==(const SdrShadowAttribute& rCandidate) const;

            // data access
            const basegfx::B2DVector& getOffset() const;
            const basegfx::B2DVector& getSize() const;
            double getTransparence() const;
            sal_Int32 getBlur() const;
            model::RectangleAlignment getAlignment() const;
            const basegfx::BColor& getColor() const;
        };

} // end of namespace drawinglayer::attribute


#endif //INCLUDED_DRAWINGLAYER_ATTRIBUTE_SDRSHADOWATTRIBUTE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
