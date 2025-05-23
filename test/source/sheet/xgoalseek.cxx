/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <test/sheet/xgoalseek.hxx>
#include <com/sun/star/sheet/XGoalSeek.hpp>
#include <com/sun/star/sheet/GoalResult.hpp>
#include <com/sun/star/table/CellAddress.hpp>

#include <cppunit/TestAssert.h>

using namespace css;
using namespace css::uno;

namespace apitest
{
void XGoalSeek::testSeekGoal()
{
    uno::Reference<sheet::XGoalSeek> xGoalSeek(init(), UNO_QUERY_THROW);

    table::CellAddress aFormulaAddr(0, 3, 5);
    table::CellAddress aVariableAddr(0, 3, 4);
    sheet::GoalResult aResult = xGoalSeek->seekGoal(aFormulaAddr, aVariableAddr, u"4"_ustr);

    double nDivergence = 0.01;
    CPPUNIT_ASSERT(aResult.Divergence < nDivergence);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(16, aResult.Result, nDivergence);
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
