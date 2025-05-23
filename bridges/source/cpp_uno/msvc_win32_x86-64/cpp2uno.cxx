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


#include <sal/config.h>

#include <cassert>
#include <limits>
#include <typeinfo>

#include <malloc.h>

#include <com/sun/star/uno/genfunc.hxx>
#include <sal/log.hxx>
#include <uno/data.h>
#include <typelib/typedescription.hxx>

#include <bridge.hxx>
#include <cppinterfaceproxy.hxx>
#include <types.hxx>
#include <vtablefactory.hxx>

#include "call.hxx"
#include <msvc/cpp2uno.hxx>
#include <msvc/amd64.hxx>

extern "C" IMAGE_DOS_HEADER const __ImageBase;

using namespace ::com::sun::star;

extern "C" typelib_TypeClass cpp_vtable_call(sal_Int64 nOffsetAndIndex, void ** pCallStack)
{
    sal_Int32 nFunctionIndex = (nOffsetAndIndex & 0xFFFFFFFF);
    sal_Int32 nVtableOffset = ((nOffsetAndIndex >> 32) & 0xFFFFFFFF);
    return cpp_mediate(pCallStack, nFunctionIndex, nVtableOffset, nullptr);
}

int const codeSnippetSize = 48;

namespace {

typedef enum { REGPARAM_INT, REGPARAM_FLT } RegParamKind;

}

extern "C" char privateSnippetExecutor;

// This function generates the code that acts as a proxy for the UNO function to be called.
// The generated code does the following:
// - Spills register parameters on stack
// - Loads functionIndex and vtableOffset into scratch registers
// - Jumps to privateSnippetExecutor

static unsigned char * codeSnippet(
    unsigned char * code,
    RegParamKind param_kind[4],
    sal_Int32 nFunctionIndex,
    sal_Int32 nVtableOffset )
{
    sal_uInt64 nOffsetAndIndex = ( static_cast<sal_uInt64>(nVtableOffset) << 32 ) | static_cast<sal_uInt64>(nFunctionIndex);
    unsigned char *p = code;

    // Spill parameters
    if (param_kind[0] == REGPARAM_INT)
    {
        // mov qword ptr 8[rsp], rcx
        *p++ = 0x48; *p++ = 0x89; *p++ = 0x4C; *p++ = 0x24; *p++ = 0x08;
    }
    else
    {
        // movsd qword ptr 8[rsp], xmm0
        *p++ = 0xF2; *p++ = 0x0F; *p++ = 0x11; *p++ = 0x44; *p++ = 0x24; *p++ = 0x08;
    }
    if ( param_kind[1] == REGPARAM_INT )
    {
        // mov qword ptr 16[rsp], rdx
        *p++ = 0x48; *p++ = 0x89; *p++ = 0x54; *p++ = 0x24; *p++ = 0x10;
    }
    else
    {
        // movsd qword ptr 16[rsp], xmm1
        *p++ = 0xF2; *p++ = 0x0F; *p++ = 0x11; *p++ = 0x4C; *p++ = 0x24; *p++ = 0x10;
    }
    if ( param_kind[2] == REGPARAM_INT )
    {
        // mov qword ptr 24[rsp], r8
        *p++ = 0x4C; *p++ = 0x89; *p++ = 0x44; *p++ = 0x24; *p++ = 0x18;
    }
    else
    {
        // movsd qword ptr 24[rsp], xmm2
        *p++ = 0xF2; *p++ = 0x0F; *p++ = 0x11; *p++ = 0x54; *p++ = 0x24; *p++ = 0x18;
    }
    if ( param_kind[3] == REGPARAM_INT )
    {
        // mov qword ptr 32[rsp], r9
        *p++ = 0x4C;*p++ = 0x89; *p++ = 0x4C; *p++ = 0x24; *p++ = 0x20;
    }
    else
    {
        // movsd qword ptr 32[rsp], xmm3
        *p++ = 0xF2; *p++ = 0x0F; *p++ = 0x11; *p++ = 0x5C; *p++ = 0x24; *p++ = 0x20;
    }

    // mov rcx, nOffsetAndIndex
    *p++ = 0x48; *p++ = 0xB9;
    *reinterpret_cast<sal_uInt64 *>(p) = nOffsetAndIndex; p += 8;

    // mov r11, privateSnippetExecutor
    *p++ = 0x49; *p++ = 0xBB;
    *reinterpret_cast<void **>(p) = &privateSnippetExecutor; p += 8;

    // jmp r11
    *p++ = 0x41; *p++ = 0xFF; *p++ = 0xE3;

    assert(p < code + codeSnippetSize);

    return code + codeSnippetSize;
}

struct bridges::cpp_uno::shared::VtableFactory::Slot { void * fn; };

bridges::cpp_uno::shared::VtableFactory::Slot *
bridges::cpp_uno::shared::VtableFactory::mapBlockToVtable(
    void * block )
{
    return static_cast< Slot * >(block) + 1;
}

std::size_t bridges::cpp_uno::shared::VtableFactory::getBlockSize(
    sal_Int32 slotCount)
{
    return (slotCount + 1) * sizeof (Slot) + slotCount * codeSnippetSize;
}

static sal_uInt32 imageRelative(void const * p) {
    assert(
        reinterpret_cast<sal_uIntPtr>(p) >= reinterpret_cast<sal_uIntPtr>(&__ImageBase)
        && reinterpret_cast<sal_uIntPtr>(p) - reinterpret_cast<sal_uIntPtr>(&__ImageBase)
            <= std::numeric_limits<sal_uInt32>::max());
    return reinterpret_cast<sal_uIntPtr>(p) - reinterpret_cast<sal_uIntPtr>(&__ImageBase);
}

namespace {

// Some dummy type whose RTTI is used in the synthesized proxy vtables to make uses of dynamic_cast
// on such proxy objects not crash:
struct ProxyRtti {};

// The following vtable RTTI data is based on how the code at
// <https://github.com/llvm/llvm-project/blob/main/clang/lib/CodeGen/MicrosoftCXXABI.cpp> computes
// such data, and on how <https://devblogs.microsoft.com/oldnewthing/20041025-00/?p=37483>
// "Accessing the current module’s HINSTANCE from a static library" obtians __ImageBase:

struct RttiClassHierarchyDescriptor;

#pragma warning (push)
#pragma warning (disable: 4324) // "structure was padded due to alignment specifier"

struct alignas(16) RttiBaseClassDescriptor {
    sal_uInt32 n0 = imageRelative(&typeid(ProxyRtti));
    sal_uInt32 n1 = 0;
    sal_uInt32 n2 = 0;
    sal_uInt32 n3 = 0xFFFFFFFF;
    sal_uInt32 n4 = 0;
    sal_uInt32 n5 = 0x40;
    sal_uInt32 n6;
    RttiBaseClassDescriptor(RttiClassHierarchyDescriptor const * chd): n6(imageRelative(chd)) {}
};

struct alignas(4) RttiBaseClassArray {
    sal_uInt32 n0;
    sal_uInt32 n1 = 0;
    RttiBaseClassArray(RttiBaseClassDescriptor const * bcd): n0(imageRelative(bcd)) {}
};

struct alignas(8) RttiClassHierarchyDescriptor {
    sal_uInt32 n0 = 0;
    sal_uInt32 n1 = 0;
    sal_uInt32 n2 = 1;
    sal_uInt32 n3;
    RttiClassHierarchyDescriptor(RttiBaseClassArray const * bca): n3(imageRelative(bca)) {}
};

struct alignas(16) RttiCompleteObjectLocator {
    sal_uInt32 n0 = 1;
    sal_uInt32 n1 = 0;
    sal_uInt32 n2 = 0;
    sal_uInt32 n3 = imageRelative(&typeid(ProxyRtti));
    sal_uInt32 n4;
    sal_uInt32 n5 = imageRelative(this);
    RttiCompleteObjectLocator(RttiClassHierarchyDescriptor const * chd): n4(imageRelative(chd)) {}
};

struct Rtti {
    RttiBaseClassDescriptor bcd;
    RttiBaseClassArray bca;
    RttiClassHierarchyDescriptor chd;
    RttiCompleteObjectLocator col;
    Rtti(): bcd(&chd), bca(&bcd), chd(&bca), col(&chd) {}
};

#pragma warning (pop)

}

bridges::cpp_uno::shared::VtableFactory::Slot *
bridges::cpp_uno::shared::VtableFactory::initializeBlock(
    void * block,
    sal_Int32 slotCount,
    sal_Int32, typelib_InterfaceTypeDescription *)
{
    static Rtti rtti;

    Slot * slots = mapBlockToVtable(block);
    slots[-1].fn = &rtti.col;
    return slots + slotCount;
}

unsigned char * bridges::cpp_uno::shared::VtableFactory::addLocalFunctions(
    Slot ** slots,
    unsigned char * code,
    typelib_InterfaceTypeDescription const * type,
    sal_Int32 nFunctionOffset,
    sal_Int32 functionCount,
    sal_Int32 nVtableOffset )
{
    (*slots) -= functionCount;
    Slot * s = *slots;

    for (int member = 0; member < type->nMembers; ++member) {
        typelib_TypeDescription * pTD = nullptr;

        TYPELIB_DANGER_GET( &pTD, type->ppMembers[ member ] );
        assert(pTD);

        RegParamKind param_kind[4];
        int nr = 0;

        for (int i = 0; i < 4; ++i)
            param_kind[i] = REGPARAM_INT;

        // 'this'
        ++nr;

        if ( pTD->eTypeClass == typelib_TypeClass_INTERFACE_ATTRIBUTE )
        {
            typelib_InterfaceAttributeTypeDescription * pIfaceAttrTD =
                reinterpret_cast<typelib_InterfaceAttributeTypeDescription *>( pTD );

            // Getter

            (s++)->fn = code;
            code = codeSnippet( code, param_kind, nFunctionOffset++, nVtableOffset );
            if ( ! pIfaceAttrTD->bReadOnly )
            {
                typelib_TypeDescription * pAttrTD = nullptr;
                TYPELIB_DANGER_GET( &pAttrTD, pIfaceAttrTD->pAttributeTypeRef );
                assert(pAttrTD);

                // Setter
                if ( pAttrTD->eTypeClass == typelib_TypeClass_FLOAT ||
                     pAttrTD->eTypeClass == typelib_TypeClass_DOUBLE )
                    param_kind[nr++] = REGPARAM_FLT;

                TYPELIB_DANGER_RELEASE( pAttrTD );

                (s++)->fn = code;
                code = codeSnippet( code, param_kind, nFunctionOffset++, nVtableOffset );
            }
        }
        else if ( pTD->eTypeClass == typelib_TypeClass_INTERFACE_METHOD )
        {
            typelib_InterfaceMethodTypeDescription * pMethodTD =
                reinterpret_cast<typelib_InterfaceMethodTypeDescription *>( pTD );

            typelib_TypeDescription * pReturnTD = nullptr;
            TYPELIB_DANGER_GET( &pReturnTD, pMethodTD->pReturnTypeRef );
            assert(pReturnTD);

            if ( !bridges::cpp_uno::shared::isSimpleType( pReturnTD ) )
            {
                // Return value
                ++nr;
            }

            for (int param = 0; nr < 4 && param < pMethodTD->nParams; ++param, ++nr)
            {
                typelib_TypeDescription * pParamTD = nullptr;

                TYPELIB_DANGER_GET( &pParamTD, pMethodTD->pParams[param].pTypeRef );
                assert(pParamTD);

                if ( pParamTD->eTypeClass == typelib_TypeClass_FLOAT ||
                     pParamTD->eTypeClass == typelib_TypeClass_DOUBLE )
                    param_kind[nr] = REGPARAM_FLT;

                TYPELIB_DANGER_RELEASE( pParamTD );
            }
            (s++)->fn = code;
            code = codeSnippet( code, param_kind, nFunctionOffset++, nVtableOffset );

            TYPELIB_DANGER_RELEASE( pReturnTD );
        }
        else
            assert(false);

        TYPELIB_DANGER_RELEASE( pTD );
    }
    return code;
}

void bridges::cpp_uno::shared::VtableFactory::flushCode(
    unsigned char const *,
    unsigned char const *)
{
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
