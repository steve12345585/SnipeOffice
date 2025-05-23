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

#include "types.hxx"
#include <com/sun/star/uno/TypeClass.hpp>

namespace com::sun::star::uno { class Any; }

struct ScInterpreterContext;
class ScDocument;
class ScMatrix;
class ScRange;

class ScRangeToSequence
{
public:
    static bool FillLongArray( css::uno::Any& rAny,
                                ScDocument& rDoc, const ScRange& rRange );
    static bool FillLongArray( css::uno::Any& rAny,
                                const ScMatrix* pMatrix );
    static bool FillDoubleArray( css::uno::Any& rAny,
                                ScDocument& rDoc, const ScRange& rRange );
    static bool FillDoubleArray( css::uno::Any& rAny,
                                const ScMatrix* pMatrix );
    static bool FillStringArray( css::uno::Any& rAny,
                                ScDocument& rDoc, const ScRange& rRange );
    static bool FillStringArray( css::uno::Any& rAny,
                                const ScMatrix* pMatrix, const ScInterpreterContext& rContext );
    static bool FillMixedArray( css::uno::Any& rAny,
                                ScDocument& rDoc, const ScRange& rRange,
                                bool bAllowNV = false );

    /** @param bDataTypes
            Additionally to the differentiation between string and double allow
            differentiation between other types such as boolean. Needed for
            implementation of XFormulaParser. If <FALSE/>, boolean values are
            treated as ordinary double values 1 (true) and 0 (false).
     */
    static bool FillMixedArray( css::uno::Any& rAny,
                                const ScMatrix* pMatrix, bool bDataTypes = false );
};

class ScApiTypeConversion
{
public:

    /** Convert a uno::Any to double if possible, including integer types.
        @param o_fVal
            Out: the double value on successful conversion.
        @param o_eClass
            Out: the uno::TypeClass of rAny.
        @returns <TRUE/> if successfully converted.
     */
    static  bool        ConvertAnyToDouble(
                            double & o_fVal,
                            css::uno::TypeClass & o_eClass,
                            const css::uno::Any & rAny );

};

class ScSequenceToMatrix
{
public:

    /** Convert a sequence of mixed elements to ScMatrix.

        Precondition: rAny.getValueType().equals( cppu::UnoType< uno::Sequence< uno::Sequence< uno::Any > > >::get() )

        @returns a new'd ScMatrix as ScMatrixRef, NULL if rAny couldn't be read
        as type Sequence<Sequence<Any>>
     */
    static  ScMatrixRef CreateMixedMatrix( const css::uno::Any & rAny );

};

class ScByteSequenceToString
{
public:
    //  rAny must contain either OUString or Sequence<sal_Int8> (thread encoding assumed),
    //  may or may not contain 0-bytes at the end
    static bool GetString( OUString& rString, const css::uno::Any& rAny );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
