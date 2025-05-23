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

class mgcLinearSystemD
{
public:
    static std::unique_ptr<std::unique_ptr<double[]>[]> NewMatrix(int N);
    static std::unique_ptr<double[]> NewVector(int N);

    static bool Solve(int N, std::unique_ptr<std::unique_ptr<double[]>[]> const& A, double* b);
    // Input:
    //     A[N][N] coefficient matrix, entries are A[row][col]
    //     b[N] vector, entries are b[row]
    // Output:
    //     return value is TRUE if successful, FALSE if pivoting failed
    //     A[N][N] is inverse matrix
    //     b[N] is solution x to Ax = b
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
