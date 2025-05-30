/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/sheet/xsheetannotationssupplier.hxx>

#include <com/sun/star/sheet/XSheetAnnotations.hpp>
#include <com/sun/star/sheet/XSheetAnnotationsSupplier.hpp>
#include <com/sun/star/uno/Reference.hxx>

using namespace css;
using namespace css::uno;

namespace apitest
{
void XSheetAnnotationsSupplier::testGetAnnotations()
{
    uno::Reference<sheet::XSheetAnnotationsSupplier> xSupplier(init(), UNO_QUERY_THROW);

    uno::Reference<sheet::XSheetAnnotations> xAnnotations(xSupplier->getAnnotations(),
                                                          UNO_SET_THROW);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
