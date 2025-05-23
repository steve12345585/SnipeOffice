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

#ifndef INCLUDED_SVX_INC_SDR_CONTACT_VIEWOBJECTCONTACTOFSDRMEDIAOBJ_HXX
#define INCLUDED_SVX_INC_SDR_CONTACT_VIEWOBJECTCONTACTOFSDRMEDIAOBJ_HXX

#include <svx/sdr/contact/viewobjectcontactofsdrobj.hxx>
#include <svx/sdr/contact/viewobjectcontact.hxx>
#include <config_features.h>
#include <tools/gen.hxx>
#include <memory>

namespace avmedia { class MediaItem; }
namespace vcl { class Window; }

namespace sdr::contact
    {
        class SdrMediaWindow;

        class ViewObjectContactOfSdrMediaObj final : public ViewObjectContactOfSdrObj
        {
        public:

            ViewObjectContactOfSdrMediaObj( ObjectContact& rObjectContact,
                                            ViewContact& rViewContact,
                                            const ::avmedia::MediaItem& rMediaItem );
            virtual ~ViewObjectContactOfSdrMediaObj() override;

        public:

            vcl::Window* getWindow() const;

            Size    getPreferredSize() const;

            void    updateMediaItem( ::avmedia::MediaItem& rItem ) const;
            void    executeMediaItem( const ::avmedia::MediaItem& rItem );

            virtual void ActionChanged() override;

        private:
            void updateMediaWindow(bool bShow) const;

#if HAVE_FEATURE_AVMEDIA
            std::unique_ptr<sdr::contact::SdrMediaWindow> mpMediaWindow;
#endif
        };

} // end of namespace sdr::contact


#endif // INCLUDED_SVX_INC_SDR_CONTACT_VIEWOBJECTCONTACTOFSDRMEDIAOBJ_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
