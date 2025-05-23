/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <svx/svxdllapi.h>
#include <svx/annotation/Annotation.hxx>
#include <svx/annotation/IAnnotationPopup.hxx>

namespace sdr::annotation
{
/** Contains the annotation data specific for a SdrObject (which represents an annotation) */
class ObjectAnnotationData
{
public:
    ~ObjectAnnotationData() { closePopup(); }

    /// The annotation
    rtl::Reference<sdr::annotation::Annotation> mxAnnotation;

    /// Object handling the pop-up window
    std::unique_ptr<sdr::annotation::IAnnotationPopup> mpAnnotationPopup;

    /// Open popup for the annotation
    void openPopup()
    {
        if (mpAnnotationPopup)
            mpAnnotationPopup->openPopup();
    }

    /// Close popup for the annotation
    void closePopup()
    {
        if (mpAnnotationPopup)
            mpAnnotationPopup->closePopup();
    }
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
