/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/drawing/xgluepointssupplier.hxx>

#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/drawing/XGluePointsSupplier.hpp>

#include <com/sun/star/uno/Reference.hxx>

#include <cppunit/TestAssert.h>

using namespace css;

namespace apitest
{
void XGluePointsSupplier::testGetGluePoints()
{
    uno::Reference<drawing::XGluePointsSupplier> xGPS(init(), uno::UNO_QUERY_THROW);

    uno::Reference<container::XIndexContainer> xIC = xGPS->getGluePoints();
    CPPUNIT_ASSERT(xIC->getCount());
}
} // namespace apitest

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
