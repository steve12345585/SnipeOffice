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

#ifndef INCLUDED_SVX_DBAOBJECTEX_HXX
#define INCLUDED_SVX_DBAOBJECTEX_HXX

#include <config_options.h>
#include <vcl/transfer.hxx>
#include <svx/dataaccessdescriptor.hxx>
#include <svx/svxdllapi.h>

namespace com::sun::star::ucb { class XContent; }

namespace svx
{
    //= OComponentTransferable
    class SAL_WARN_UNUSED UNLESS_MERGELIBS_MORE(SVX_DLLPUBLIC) OComponentTransferable final : public TransferDataContainer
    {
    public:
        OComponentTransferable();

        void Update(
            const OUString&  rDatasourceOrLocation,
            const css::uno::Reference< css::ucb::XContent>& xContent
        );

        /** checks whether or not a component descriptor can be extracted from the data flavor vector given
            @param _rFlavors
                available flavors
        */
        static bool canExtractComponentDescriptor(const DataFlavorExVector& _rFlavors, bool _bForm );

        /** extracts a component descriptor from the transferable given
        */
        static ODataAccessDescriptor
                        extractComponentDescriptor(const TransferableDataHelper& _rData);

    private:
        // TransferableHelper overridables
        virtual void        AddSupportedFormats() override;
        virtual bool GetData( const css::datatransfer::DataFlavor& rFlavor, const OUString& rDestDoc ) override;

        static SotClipboardFormatId getDescriptorFormatId(bool _bExtractForm);

        ODataAccessDescriptor   m_aDescriptor;
    };
}

#endif // INCLUDED_SVX_DBAOBJECTEX_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
