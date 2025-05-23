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
#ifndef INCLUDED_FILTER_MSFILTER_MSOLEEXP_HXX
#define INCLUDED_FILTER_MSFILTER_MSOLEEXP_HXX

#include <com/sun/star/uno/Reference.hxx>
#include <filter/msfilter/msfilterdllapi.h>
#include <sal/types.h>

namespace com::sun::star {
    namespace embed { class XEmbeddedObject; }
}

namespace svt {
    class EmbeddedObjectRef;
}

class SotStorage;

// for the CreateSdrOLEFromStorage we need the information, how we handle
// convert able OLE-Objects - this is stored in
#define OLE_STARMATH_2_MATHTYPE             0x0001
#define OLE_STARWRITER_2_WINWORD            0x0002
#define OLE_STARCALC_2_EXCEL                0x0004
#define OLE_STARIMPRESS_2_POWERPOINT        0x0008

class MSFILTER_DLLPUBLIC SvxMSExportOLEObjects
{
    sal_uInt32 nConvertFlags;
public:
    SvxMSExportOLEObjects( sal_uInt32 nCnvrtFlgs ) : nConvertFlags(nCnvrtFlgs) {}

    void ExportOLEObject( svt::EmbeddedObjectRef const & rObj, SotStorage& rDestStg );
    void ExportOLEObject( const css::uno::Reference < css::embed::XEmbeddedObject>& rObj, SotStorage& rDestStg );
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
