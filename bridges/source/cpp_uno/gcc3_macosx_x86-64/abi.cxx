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


// This is an implementation of the x86-64 ABI as described in 'System V
// Application Binary Interface, AMD64 Architecture Processor Supplement'
// (http://www.x86-64.org/documentation/abi-0.95.pdf)
//
// The code in this file is a modification of src/x86/ffi64.c from libffi
// (http://sources.redhat.com/libffi/) which is under the following license:

/* -----------------------------------------------------------------------
   ffi.c - Copyright (c) 2002  Bo Thorsen <bo@suse.de>

   x86-64 Foreign Function Interface

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   ``Software''), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED ``AS IS'', WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL CYGNUS SOLUTIONS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.
   ----------------------------------------------------------------------- */

#include <sal/config.h>

#include "abi.hxx"

#include <o3tl/unreachable.hxx>
#include <sal/log.hxx>

using namespace x86_64;

namespace {

/* Register class used for passing given 64bit part of the argument.
   These represent classes as documented by the PS ABI, with the exception
   of SSESF, SSEDF classes, that are basically SSE class, just gcc will
   use SF or DFmode move instead of DImode to avoid reformatting penalties.

   Similarly we play games with INTEGERSI_CLASS to use cheaper SImode moves
   whenever possible (upper half does contain padding).
 */
enum x86_64_reg_class
{
    X86_64_NO_CLASS,
    X86_64_INTEGER_CLASS,
    X86_64_INTEGERSI_CLASS,
    X86_64_SSE_CLASS,
    X86_64_SSESF_CLASS,
    X86_64_MEMORY_CLASS
};

}

constexpr auto MAX_CLASSES = 4;

/* x86-64 register passing implementation.  See x86-64 ABI for details.  Goal
   of this code is to classify each 8bytes of incoming argument by the register
   class and assign registers accordingly.  */

/* Return the union class of CLASS1 and CLASS2.
   See the x86-64 PS ABI for details.  */

static enum x86_64_reg_class
merge_classes (enum x86_64_reg_class class1, enum x86_64_reg_class class2)
    noexcept
{
    /* Rule #1: If both classes are equal, this is the resulting class.  */
    if (class1 == class2)
        return class1;

    /* Rule #2: If one of the classes is NO_CLASS, the resulting class is
       the other class.  */
    if (class1 == X86_64_NO_CLASS)
        return class2;
    if (class2 == X86_64_NO_CLASS)
        return class1;

    /* Rule #3: If one of the classes is MEMORY, the result is MEMORY.  */
    if (class1 == X86_64_MEMORY_CLASS || class2 == X86_64_MEMORY_CLASS)
        return X86_64_MEMORY_CLASS;

    /* Rule #4: If one of the classes is INTEGER, the result is INTEGER.  */
    if ((class1 == X86_64_INTEGERSI_CLASS && class2 == X86_64_SSESF_CLASS)
            || (class2 == X86_64_INTEGERSI_CLASS && class1 == X86_64_SSESF_CLASS))
        return X86_64_INTEGERSI_CLASS;
    if (class1 == X86_64_INTEGER_CLASS || class1 == X86_64_INTEGERSI_CLASS
            || class2 == X86_64_INTEGER_CLASS || class2 == X86_64_INTEGERSI_CLASS)
        return X86_64_INTEGER_CLASS;

    /* Rule #6: Otherwise class SSE is used.  */
    return X86_64_SSE_CLASS;
}

/* Classify a parameter/return type.
   CLASSES will be filled by the register class used to pass each word
   of the operand.  The number of words is returned.  In case the operand
   should be passed in memory, 0 is returned.

   See the x86-64 PS ABI for details.
*/
static int
classify_argument( typelib_TypeDescriptionReference *pTypeRef, enum x86_64_reg_class classes[], int byteOffset ) noexcept
{
    switch ( pTypeRef->eTypeClass )
    {
        case typelib_TypeClass_CHAR:
        case typelib_TypeClass_BOOLEAN:
        case typelib_TypeClass_BYTE:
        case typelib_TypeClass_SHORT:
        case typelib_TypeClass_UNSIGNED_SHORT:
        case typelib_TypeClass_LONG:
        case typelib_TypeClass_UNSIGNED_LONG:
        case typelib_TypeClass_HYPER:
        case typelib_TypeClass_UNSIGNED_HYPER:
        case typelib_TypeClass_ENUM:
            if ( ( byteOffset % 8 + pTypeRef->pType->nSize ) <= 4 )
                classes[0] = X86_64_INTEGERSI_CLASS;
            else
                classes[0] = X86_64_INTEGER_CLASS;
            return 1;
        case typelib_TypeClass_FLOAT:
            if ( ( byteOffset % 8 ) == 0 )
                classes[0] = X86_64_SSESF_CLASS;
            else
                classes[0] = X86_64_SSE_CLASS;
            return 1;
        case typelib_TypeClass_DOUBLE:
            classes[0] = X86_64_SSE_CLASS;
            return 1;
        case typelib_TypeClass_STRING:
        case typelib_TypeClass_TYPE:
        case typelib_TypeClass_ANY:
        case typelib_TypeClass_SEQUENCE:
        case typelib_TypeClass_INTERFACE:
            return 0;
        case typelib_TypeClass_STRUCT:
            {
                typelib_TypeDescription * pTypeDescr = nullptr;
                TYPELIB_DANGER_GET( &pTypeDescr, pTypeRef );

                const int UNITS_PER_WORD = 8;
                int words = ( pTypeDescr->nSize + UNITS_PER_WORD - 1 ) / UNITS_PER_WORD;
                enum x86_64_reg_class subclasses[MAX_CLASSES];

                /* If the struct is larger than 16 bytes, pass it on the stack.  */
                if ( pTypeDescr->nSize > 16 )
                {
                    TYPELIB_DANGER_RELEASE( pTypeDescr );
                    return 0;
                }

                for ( int i = 0; i < words; i++ )
                    classes[i] = X86_64_NO_CLASS;

                const typelib_CompoundTypeDescription *pStruct = reinterpret_cast<const typelib_CompoundTypeDescription*>( pTypeDescr );

                /* Merge the fields of structure.  */
                for ( sal_Int32 nMember = 0; nMember < pStruct->nMembers; ++nMember )
                {
                    typelib_TypeDescriptionReference *pTypeInStruct = pStruct->ppTypeRefs[ nMember ];
                    int offset = byteOffset + pStruct->pMemberOffsets[ nMember ];

                    int num = classify_argument( pTypeInStruct, subclasses, offset );

                    if ( num == 0 )
                    {
                        TYPELIB_DANGER_RELEASE( pTypeDescr );
                        return 0;
                    }

                    for ( int i = 0; i < num; i++ )
                    {
                        int pos = offset / 8;
                        classes[i + pos] = merge_classes( subclasses[i], classes[i + pos] );
                        if (classes[i + pos] == X86_64_MEMORY_CLASS) {
                            TYPELIB_DANGER_RELEASE( pTypeDescr );
                            return 0;
                        }
                    }
                }

                TYPELIB_DANGER_RELEASE( pTypeDescr );

                return words;
            }

        default:
            O3TL_UNREACHABLE;
    }
}

/* Examine the argument and return set number of register required in each
   class.  Return 0 iff parameter should be passed in memory.  */
bool x86_64::examine_argument( typelib_TypeDescriptionReference *pTypeRef, int &nUsedGPR, int &nUsedSSE ) noexcept
{
    enum x86_64_reg_class classes[MAX_CLASSES];
    int n;

    n = classify_argument( pTypeRef, classes, 0 );

    if ( n == 0 )
        return false;

    nUsedGPR = 0;
    nUsedSSE = 0;
    for ( n--; n >= 0; n-- )
        switch ( classes[n] )
        {
            case X86_64_INTEGER_CLASS:
            case X86_64_INTEGERSI_CLASS:
                nUsedGPR++;
                break;
            case X86_64_SSE_CLASS:
            case X86_64_SSESF_CLASS:
                nUsedSSE++;
                break;
            default:
                O3TL_UNREACHABLE;
        }
    return true;
}

bool x86_64::return_in_hidden_param( typelib_TypeDescriptionReference *pTypeRef ) noexcept
{
    if (pTypeRef->eTypeClass == typelib_TypeClass_VOID) {
        return false;
    }
    x86_64_reg_class classes[MAX_CLASSES];
    return classify_argument(pTypeRef, classes, 0) == 0;
}

x86_64::ReturnKind x86_64::getReturnKind(typelib_TypeDescriptionReference * type) noexcept {
    x86_64_reg_class classes[MAX_CLASSES];
    auto const n = classify_argument(type, classes, 0);
    if (n == 0) {
        return ReturnKind::Memory;
    }
    if (n == 2 && (classes[0] == X86_64_SSE_CLASS || classes[0] == X86_64_SSESF_CLASS)
        && (classes[1] == X86_64_INTEGER_CLASS || classes[1] == X86_64_INTEGERSI_CLASS))
    {
        return ReturnKind::RegistersFpInt;
    }
    if (n == 2 && (classes[0] == X86_64_INTEGER_CLASS || classes[0] == X86_64_INTEGERSI_CLASS)
        && (classes[1] == X86_64_SSE_CLASS || classes[1] == X86_64_SSESF_CLASS))
    {
        return ReturnKind::RegistersIntFp;
    }
    return ReturnKind::RegistersGeneral;
}

void x86_64::fill_struct( typelib_TypeDescriptionReference *pTypeRef, const sal_uInt64 *pGPR, const double *pSSE, void *pStruct ) noexcept
{
    enum x86_64_reg_class classes[MAX_CLASSES];
    int n;

    n = classify_argument( pTypeRef, classes, 0 );

    sal_uInt64 *pStructAlign = static_cast<sal_uInt64 *>( pStruct );
    for ( int i = 0; i != n; ++i )
        switch ( classes[i] )
        {
            case X86_64_INTEGER_CLASS:
            case X86_64_INTEGERSI_CLASS:
                *pStructAlign++ = *pGPR++;
                break;
            case X86_64_SSE_CLASS:
            case X86_64_SSESF_CLASS:
                *pStructAlign++ = *reinterpret_cast<const sal_uInt64 *>( pSSE++ );
                break;
            default:
                O3TL_UNREACHABLE;
        }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
