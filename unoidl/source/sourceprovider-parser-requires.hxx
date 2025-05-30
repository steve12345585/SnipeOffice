/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <utility>
#include <vector>

#include <rtl/ustring.hxx>
#include <sal/types.h>

#define YYLTYPE int

typedef void * yyscan_t;

namespace unoidl::detail {

struct SourceProviderEntity;

enum SourceProviderAccessDecls { ACCESS_DECL_GET = 0x1, ACCESS_DECL_SET = 0x2 };

enum SourceProviderFlags {
    FLAG_ATTRIBUTE = 0x001, FLAG_BOUND = 0x002, FLAG_CONSTRAINED = 0x004,
    FLAG_MAYBEAMBIGUOUS = 0x008, FLAG_MAYBEDEFAULT = 0x010,
    FLAG_MAYBEVOID = 0x020, FLAG_OPTIONAL = 0x040, FLAG_PROPERTY = 0x080,
    FLAG_READONLY = 0x100, FLAG_REMOVABLE = 0x200, FLAG_TRANSIENT = 0x400
};

struct SourceProviderExpr {
    static SourceProviderExpr Bool(bool v) {
        SourceProviderExpr e;
        e.type = TYPE_BOOL;
        e.bval = v;
        return e;
    }

    static SourceProviderExpr Int(sal_Int64 v) {
        SourceProviderExpr e;
        e.type = TYPE_INT;
        e.ival = v;
        return e;
    }

    static SourceProviderExpr Uint(sal_uInt64 v) {
        SourceProviderExpr e;
        e.type = TYPE_UINT;
        e.uval = v;
        return e;
    }

    static SourceProviderExpr Float(double v) {
        SourceProviderExpr e;
        e.type = TYPE_FLOAT;
        e.fval = v;
        return e;
    }

    enum Type { TYPE_BOOL, TYPE_INT, TYPE_UINT, TYPE_FLOAT };

    Type type;
    union {
        bool bval;
        sal_Int64 ival;
        sal_uInt64 uval;
        double fval;
    };
};

struct SourceProviderType {
    enum Type {
        TYPE_VOID, TYPE_BOOLEAN, TYPE_BYTE, TYPE_SHORT, TYPE_UNSIGNED_SHORT,
        TYPE_LONG, TYPE_UNSIGNED_LONG, TYPE_HYPER, TYPE_UNSIGNED_HYPER,
        TYPE_FLOAT, TYPE_DOUBLE, TYPE_CHAR, TYPE_STRING, TYPE_TYPE, TYPE_ANY,
        TYPE_SEQUENCE, TYPE_ENUM, TYPE_PLAIN_STRUCT, TYPE_EXCEPTION,
        TYPE_INTERFACE, TYPE_INSTANTIATED_POLYMORPHIC_STRUCT, TYPE_PARAMETER
    };

    SourceProviderType():
        type(), entity() // avoid false warnings about uninitialized members
    {}

    explicit SourceProviderType(Type theType):
        type(theType),
        entity() // avoid false warnings about uninitialized member
    { assert(theType <= TYPE_ANY); }

    explicit SourceProviderType(SourceProviderType const * componentType):
        type(TYPE_SEQUENCE),
        entity() // avoid false warnings about uninitialized member
    { assert(componentType != nullptr); subtypes.push_back(*componentType); }

    SourceProviderType(
        Type theType, OUString theName,
        SourceProviderEntity const * theEntity):
        type(theType), name(std::move(theName)), entity(theEntity)
    {
        assert(theType >= TYPE_ENUM && theType <= TYPE_INTERFACE);
        assert(theEntity != nullptr);
    }

    SourceProviderType(
        OUString polymorphicStructTypeTemplateName,
        SourceProviderEntity const * theEntity,
        std::vector<SourceProviderType>&& typeArguments):
        type(TYPE_INSTANTIATED_POLYMORPHIC_STRUCT),
        name(std::move(polymorphicStructTypeTemplateName)), entity(theEntity),
        subtypes(std::move(typeArguments))
    { assert(theEntity != nullptr); }

    explicit SourceProviderType(OUString identifier):
        type(TYPE_PARAMETER), name(std::move(identifier)),
        entity() // avoid false warnings about uninitialized member
    {}

    OUString getName() const;

    bool equals(SourceProviderType const & other) const;

    Type type;
    OUString name; // TYPE_ENUM ... TYPE_PARAMETER
    SourceProviderEntity const * entity;
        // TYPE_ENUM ... TYPE_INSTANTIATED_POLYMORPHIC_STRUCT
    std::vector<SourceProviderType> subtypes;
        // TYPE_SEQUENCE, TYPE_INSTANTIATED_POLYMORPHIC_STRUCT
    OUString typedefName;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
