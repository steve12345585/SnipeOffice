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

#pragma once

#include <formula/errorcodes.hxx>

namespace sc {

/** Return fNumerator/fDenominator if fDenominator!=0 else #DIV/0! error coded
    into double.
 */
inline double div( const double& fNumerator, const double& fDenominator )
{
    return (fDenominator != 0.0) ? (fNumerator / fDenominator) :
        CreateDoubleError( FormulaError::DivisionByZero);
}

/** Return fNumerator/fDenominator if fDenominator!=0 else +-Infinity if
    fNumerator!=0 or NaN if fNumerator==0.

    This allows to build/run with -fsanitize=float-divide-by-zero and have a
    defined behavior for the otherwise undefined division by zero case ("If the
    second operand of / or % is zero the behavior is undefined."
    ([expr.mul]/4)).

    The Calc interpreter gracefully handles Infinity or NaN double values
    encountered as interim or final results, using this function we can ensure
    defined behavior where desired.

    Use where the double coded error creating div() is not wanted.
 */
inline double divide( const double& fNumerator, const double& fDenominator )
{
    if (fDenominator == 0.0)
    {
        if (std::isfinite( fNumerator) && fNumerator != 0.0)
            return std::signbit(fNumerator) ? -std::numeric_limits<double>::infinity()
                                            :  std::numeric_limits<double>::infinity();
        return std::numeric_limits<double>::quiet_NaN();
    }
    return fNumerator / fDenominator;
}

/** Return pow(fVal1,fVal2) with error handling.

    If an error was detected, a coded double error of
    FormulaError::IllegalFPOperation is returned.
 */
double power( const double& fVal1, const double& fVal2 );

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
