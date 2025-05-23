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

#include <o3tl/any.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/string_view.hxx>
#include <o3tl/underlyingenumvalue.hxx>
#include <osl/diagnose.h>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <typelib/typedescription.hxx>
#include <uno/data.h>

#ifdef _WIN32
#include <cmath>
#else
#include <math.h>
#endif
#include <float.h>

#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/script/CannotConvertException.hpp>
#include <com/sun/star/script/XTypeConverter.hpp>
#include <com/sun/star/script/FailReason.hpp>

namespace com::sun::star::uno { class XComponentContext; }

using namespace css::uno;
using namespace css::lang;
using namespace css::script;
using namespace cppu;

namespace stoc_tcv
{

static double round( double aVal )
{
    bool bPos   = (aVal >= 0.0);
    aVal            = ::fabs( aVal );
    double aUpper   = ::ceil( aVal );

    aVal            = ((aUpper-aVal) <= 0.5) ? aUpper : (aUpper - 1.0);
    return (bPos ? aVal : -aVal);
}


static bool getNumericValue( double & rfVal, std::u16string_view rStr )
{
    double fRet = o3tl::toDouble(rStr);
    if (fRet == 0.0)
    {
        size_t nLen = rStr.size();
        if (!nLen || (nLen == 1 && rStr[0] == '0')) // common case
        {
            rfVal = 0.0;
            return true;
        }

        std::u16string_view trim( o3tl::trim(rStr) );

        // try hex
        size_t nX = trim.find( 'x' );
        if (nX == std::u16string_view::npos)
            nX = trim.find( 'X' );

        if (nX > 0 && nX != std::u16string_view::npos && trim[nX-1] == '0') // 0x
        {
            bool bNeg = false;
            switch (nX)
            {
            case 2: // (+|-)0x...
                if (trim[0] == '-')
                    bNeg = true;
                else if (trim[0] != '+')
                    return false;
                break;
            case 1: // 0x...
                break;
            default:
                return false;
            }

            OUString aHexRest( trim.substr( nX+1 ) );
            sal_uInt64 nRet = aHexRest.toUInt64( 16 );

            if (nRet == 0)
            {
                for ( sal_Int32 nPos = aHexRest.getLength(); nPos--; )
                {
                    if (aHexRest[nPos] != '0')
                        return false;
                }
            }

            rfVal = (bNeg ? -static_cast<double>(nRet) : static_cast<double>(nRet));
            return true;
        }

        nLen = trim.size();
        size_t nPos = 0;

        // skip +/-
        if (nLen && (trim[0] == '-' || trim[0] == '+'))
            ++nPos;

        while (nPos < nLen) // skip leading zeros
        {
            if (trim[nPos] != '0')
            {
                if (trim[nPos] != '.')
                    return false;
                ++nPos;
                while (nPos < nLen) // skip trailing zeros
                {
                    if (trim[nPos] != '0')
                        return false;
                    ++nPos;
                }
                break;
            }
            ++nPos;
        }
    }
    rfVal = fRet;
    return true;
}


static bool getHyperValue( sal_Int64 & rnVal, std::u16string_view rStr )
{
    size_t nLen = rStr.size();
    if (!nLen || (nLen == 1 && rStr[0] == '0')) // common case
    {
        rnVal = 0;
        return true;
    }

    std::u16string_view trim( o3tl::trim(rStr) );

    // try hex
    size_t nX = trim.find( 'x' );
    if (nX == std::u16string_view::npos)
        nX = trim.find( 'X' );

    if (nX != std::u16string_view::npos)
    {
        if (nX > 0 && trim[nX-1] == '0') // 0x
        {
            bool bNeg = false;
            switch (nX)
            {
            case 2: // (+|-)0x...
                if (trim[0] == '-')
                    bNeg = true;
                else if (trim[0] != '+')
                    return false;
                break;
            case 1: // 0x...
                break;
            default:
                return false;
            }

            OUString aHexRest( trim.substr( nX+1 ) );
            sal_uInt64 nRet = aHexRest.toUInt64( 16 );

            if (nRet == 0)
            {
                for ( sal_Int32 nPos = aHexRest.getLength(); nPos--; )
                {
                    if (aHexRest[nPos] != '0')
                        return false;
                }
            }

            rnVal = (bNeg ? -static_cast<sal_Int64>(nRet) : nRet);
            return true;
        }
        return false;
    }

    double fVal;
    if (getNumericValue( fVal, rStr ) &&
        fVal >= double(SAL_MIN_INT64) &&
        fVal <= double(SAL_MAX_UINT64))
    {
        rnVal = static_cast<sal_Int64>(round( fVal ));
        return true;
    }
    return false;
}

namespace {

class TypeConverter_Impl : public WeakImplHelper< XTypeConverter, XServiceInfo >
{
    // ...misc helpers...
    /// @throws CannotConvertException
    static sal_Int64 toHyper(
        const Any& rAny, sal_Int64 min, sal_uInt64 max = SAL_MAX_UINT64 );
    /// @throws CannotConvertException
    static double toDouble( const Any& rAny, double min = -DBL_MAX, double max = DBL_MAX );

public:
    TypeConverter_Impl();

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual  Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XTypeConverter
    virtual Any SAL_CALL convertTo( const Any& aFrom, const Type& DestinationType ) override;
    virtual Any SAL_CALL convertToSimpleType( const Any& aFrom, TypeClass aDestinationType ) override;
};

}

TypeConverter_Impl::TypeConverter_Impl() {}

// XServiceInfo
OUString TypeConverter_Impl::getImplementationName()
{
    return u"com.sun.star.comp.stoc.TypeConverter"_ustr;
}

// XServiceInfo
sal_Bool TypeConverter_Impl::supportsService(const OUString& ServiceName)
{
    return cppu::supportsService(this, ServiceName);
}

// XServiceInfo
Sequence< OUString > TypeConverter_Impl::getSupportedServiceNames()
{
    return { u"com.sun.star.script.Converter"_ustr };
}


sal_Int64 TypeConverter_Impl::toHyper( const Any& rAny, sal_Int64 min, sal_uInt64 max )
{
    sal_Int64 nRet;
    TypeClass aDestinationClass = rAny.getValueTypeClass();

    switch (aDestinationClass)
    {
    // ENUM
    case TypeClass_ENUM:
        nRet = *static_cast<sal_Int32 const *>(rAny.getValue());
        break;
    // BOOL
    case TypeClass_BOOLEAN:
        nRet = *o3tl::forceAccess<bool>(rAny) ? 1 : 0;
        break;
    // CHAR, BYTE
    case TypeClass_CHAR:
        nRet = *o3tl::forceAccess<sal_Unicode>(rAny);
        break;
    case TypeClass_BYTE:
        nRet = *o3tl::forceAccess<sal_Int8>(rAny);
        break;
    // SHORT
    case TypeClass_SHORT:
        nRet = *o3tl::forceAccess<sal_Int16>(rAny);
        break;
    // UNSIGNED SHORT
    case TypeClass_UNSIGNED_SHORT:
        nRet = *o3tl::forceAccess<sal_uInt16>(rAny);
        break;
    // LONG
    case TypeClass_LONG:
        nRet = *o3tl::forceAccess<sal_Int32>(rAny);
        break;
    // UNSIGNED LONG
    case TypeClass_UNSIGNED_LONG:
        nRet = *o3tl::forceAccess<sal_uInt32>(rAny);
        break;
    // HYPER
    case TypeClass_HYPER:
        nRet = *o3tl::forceAccess<sal_Int64>(rAny);
        break;
    // UNSIGNED HYPER
    case TypeClass_UNSIGNED_HYPER:
    {
        auto const n = *o3tl::forceAccess<sal_uInt64>(rAny);
        if ((min < 0 || n >= o3tl::make_unsigned(min)) && // lower bound
            n <= max)                            // upper bound
        {
            return static_cast<sal_Int64>(n);
        }
        throw CannotConvertException(
            u"UNSIGNED HYPER out of range!"_ustr,
            Reference<XInterface>(), aDestinationClass, FailReason::OUT_OF_RANGE, 0 );
    }

    // FLOAT, DOUBLE
    case TypeClass_FLOAT:
    {
        double fVal = round( *o3tl::forceAccess<float>(rAny) );
        if (fVal >= min && fVal <= max)
        {
            nRet = (fVal >= 0.0 ? static_cast<sal_Int64>(static_cast<sal_uInt64>(fVal)) : static_cast<sal_Int64>(fVal));
            return nRet;
        }
        throw CannotConvertException(
            u"FLOAT out of range!"_ustr,
            Reference<XInterface>(), aDestinationClass, FailReason::OUT_OF_RANGE, 0 );
    }
    case TypeClass_DOUBLE:
    {
        double fVal = round( *o3tl::forceAccess<double>(rAny) );
        if (fVal >= min && fVal <= max)
        {
            nRet = (fVal >= 0.0 ? static_cast<sal_Int64>(static_cast<sal_uInt64>(fVal)) : static_cast<sal_Int64>(fVal));
            return nRet;
        }
        throw CannotConvertException(
            u"DOUBLE out of range!"_ustr,
            Reference<XInterface>(), aDestinationClass, FailReason::OUT_OF_RANGE, 0 );
    }

    // STRING
    case TypeClass_STRING:
    {
        sal_Int64 nVal = SAL_CONST_INT64(0);
        if (! getHyperValue( nVal, *o3tl::forceAccess<OUString>(rAny) ))
        {
            throw CannotConvertException(
                u"invalid STRING value!"_ustr,
                Reference<XInterface>(), aDestinationClass, FailReason::IS_NOT_NUMBER, 0 );
        }
        nRet = nVal;
        if (nVal >= min && (nVal < 0 || o3tl::make_unsigned(nVal) <= max))
            return nRet;
        throw CannotConvertException(
            u"STRING value out of range!"_ustr,
            Reference<XInterface>(), aDestinationClass, FailReason::OUT_OF_RANGE, 0 );
    }

    default:
        throw CannotConvertException(
            "Type " + OUString::number(o3tl::to_underlying(aDestinationClass)) + " is not supported!",
            Reference<XInterface>(), aDestinationClass, FailReason::TYPE_NOT_SUPPORTED, 0 );
    }

    if (nRet >= min && (nRet < 0 || o3tl::make_unsigned(nRet) <= max))
        return nRet;
    throw CannotConvertException(
        u"VALUE is out of range!"_ustr,
        Reference<XInterface>(), aDestinationClass, FailReason::OUT_OF_RANGE, 0 );
}


double TypeConverter_Impl::toDouble( const Any& rAny, double min, double max )
{
    double fRet;
    TypeClass aDestinationClass = rAny.getValueTypeClass();

    switch (aDestinationClass)
    {
    // ENUM
    case TypeClass_ENUM:
        fRet = *static_cast<sal_Int32 const *>(rAny.getValue());
        break;
    // BOOL
    case TypeClass_BOOLEAN:
        fRet = *o3tl::forceAccess<bool>(rAny) ? 1.0 : 0.0;
        break;
    // CHAR, BYTE
    case TypeClass_CHAR:
        fRet = *o3tl::forceAccess<sal_Unicode>(rAny);
        break;
    case TypeClass_BYTE:
        fRet = *o3tl::forceAccess<sal_Int8>(rAny);
        break;
    // SHORT
    case TypeClass_SHORT:
        fRet = *o3tl::forceAccess<sal_Int16>(rAny);
        break;
    // UNSIGNED SHORT
    case TypeClass_UNSIGNED_SHORT:
        fRet = *o3tl::forceAccess<sal_uInt16>(rAny);
        break;
    // LONG
    case TypeClass_LONG:
        fRet = *o3tl::forceAccess<sal_Int32>(rAny);
        break;
    // UNSIGNED LONG
    case TypeClass_UNSIGNED_LONG:
        fRet = *o3tl::forceAccess<sal_uInt32>(rAny);
        break;
    // HYPER
    case TypeClass_HYPER:
        fRet = static_cast<double>(*o3tl::forceAccess<sal_Int64>(rAny));
        break;
    // UNSIGNED HYPER
    case TypeClass_UNSIGNED_HYPER:
        fRet = static_cast<double>(*o3tl::forceAccess<sal_uInt64>(rAny));
        break;
    // FLOAT, DOUBLE
    case TypeClass_FLOAT:
        fRet = *o3tl::forceAccess<float>(rAny);
        break;
    case TypeClass_DOUBLE:
        fRet = *o3tl::forceAccess<double>(rAny);
        break;

    // STRING
    case TypeClass_STRING:
    {
        if (! getNumericValue( fRet, *o3tl::forceAccess<OUString>(rAny) ))
        {
            throw CannotConvertException(
                u"invalid STRING value!"_ustr,
                Reference<XInterface>(), aDestinationClass, FailReason::IS_NOT_NUMBER, 0 );
        }
        break;
    }

    default:
        throw CannotConvertException(
            "Type " + OUString::number(o3tl::to_underlying(aDestinationClass)) + " is not supported!",
            Reference< XInterface >(), aDestinationClass, FailReason::TYPE_NOT_SUPPORTED, 0 );
    }

    if (fRet >= min && fRet <= max)
        return fRet;
    throw CannotConvertException(
        u"VALUE is out of range!"_ustr,
        Reference< XInterface >(), aDestinationClass, FailReason::OUT_OF_RANGE, 0 );
}


Any SAL_CALL TypeConverter_Impl::convertTo( const Any& rVal, const Type& aDestType )
{
    const Type& aSourceType = rVal.getValueType();
    if (aSourceType == aDestType)
        return rVal;

    TypeClass aSourceClass = aSourceType.getTypeClass();
    TypeClass aDestinationClass = aDestType.getTypeClass();

    Any aRet;

    // convert to...
    switch (aDestinationClass)
    {
    // --- to VOID ------------------------------------------------------------------------------
    case TypeClass_VOID:
        return Any();
    // --- to ANY -------------------------------------------------------------------------------
    case TypeClass_ANY:
        return rVal;

    // --- to STRUCT, EXCEPTION ----------------------------------------------------------
    case TypeClass_STRUCT:
    case TypeClass_EXCEPTION:
    {
        // same types or destination type is derived source type?
        TypeDescription aSourceTD( aSourceType );
        TypeDescription aDestTD( aDestType );
        if (!typelib_typedescription_isAssignableFrom( aDestTD.get(), aSourceTD.get() ))
        {
            throw CannotConvertException(
                u"value is not of same or derived type!"_ustr,
                Reference< XInterface >(), aDestinationClass,
                FailReason::SOURCE_IS_NO_DERIVED_TYPE, 0 );
        }
        aRet.setValue( rVal.getValue(), aDestTD.get() ); // evtl. .uP.cAsT.
        break;
    }
    // --- to INTERFACE -------------------------------------------------------------------------
    case TypeClass_INTERFACE:
    {
        if (! rVal.hasValue())
        {
            // void -> interface (null)
            void * null_ref = nullptr;
            // coverity[var_deref_model : FALSE] - null_ref will not be derefed in this case
            aRet.setValue( &null_ref, aDestType );
            break;
        }

        auto ifc = o3tl::tryAccess<css::uno::Reference<css::uno::XInterface>>(
            rVal);
        if (!ifc || !ifc->is())
        {
            throw CannotConvertException(
                u"value is not interface"_ustr,
                Reference< XInterface >(), aDestinationClass, FailReason::NO_SUCH_INTERFACE, 0 );
        }
        aRet = (*ifc)->queryInterface(aDestType );
        if (! aRet.hasValue())
        {
            throw CannotConvertException(
                "value does not implement " + aDestType.getTypeName(),
                Reference< XInterface >(), aDestinationClass, FailReason::NO_SUCH_INTERFACE, 0 );
        }
        break;
    }
    // --- to SEQUENCE --------------------------------------------------------------------------
    case TypeClass_SEQUENCE:
    {
        if (aSourceClass==TypeClass_SEQUENCE)
        {
            if( aSourceType == aDestType )
                return rVal;

            TypeDescription aSourceTD( aSourceType );
            TypeDescription aDestTD( aDestType );
            // For a sequence type notation "[]...", SequenceTypeDescription in
            // cppuhelper/source/typemanager.cxx resolves the "..." component type notation part
            // only lazily, so it could happen here that bad user input (e.g., "[]" or "[]foo" from
            // a Basic script CreateUnoValue call) leads to a bad but as-of-yet undetected
            // aDestType, so check it here; this is less likely an issue for the non-sequence type
            // classes, whose notation is not resolved lazily based on their syntax:
            if (!aDestTD.is()) {
                throw css::lang::IllegalArgumentException(
                    "Bad XTypeConverter::convertTo destination " + aDestType.getTypeName(),
                    getXWeak(), 1);
            }
            typelib_TypeDescription * pSourceElementTD = nullptr;
            TYPELIB_DANGER_GET(
                &pSourceElementTD,
                reinterpret_cast<typelib_IndirectTypeDescription *>(aSourceTD.get())->pType );
            typelib_TypeDescription * pDestElementTD = nullptr;
            TYPELIB_DANGER_GET(
                &pDestElementTD,
                reinterpret_cast<typelib_IndirectTypeDescription *>(aDestTD.get())->pType );

            sal_uInt32 nPos = (*static_cast<const uno_Sequence * const *>(rVal.getValue()))->nElements;
            uno_Sequence * pRet = nullptr;
            uno_sequence_construct(
                &pRet, aDestTD.get(), nullptr, nPos,
                reinterpret_cast< uno_AcquireFunc >(cpp_acquire) );
            aRet.setValue( &pRet, aDestTD.get() );
            uno_destructData(
                &pRet, aDestTD.get(),
                reinterpret_cast< uno_ReleaseFunc >(cpp_release) );
                // decr ref count

            char * pDestElements = (*static_cast<uno_Sequence * const *>(aRet.getValue()))->elements;
            const char * pSourceElements =
                (*static_cast<const uno_Sequence * const *>(rVal.getValue()))->elements;

            while (nPos--)
            {
                char * pDestPos = pDestElements + (nPos * pDestElementTD->nSize);
                const char * pSourcePos = pSourceElements + (nPos * pSourceElementTD->nSize);

                Any aElement(
                    convertTo( Any( pSourcePos, pSourceElementTD ), pDestElementTD->pWeakRef ) );

                if (!uno_assignData(
                        pDestPos, pDestElementTD,
                        (pDestElementTD->eTypeClass == typelib_TypeClass_ANY
                         ? &aElement
                         : const_cast< void * >( aElement.getValue() )),
                        pDestElementTD,
                        reinterpret_cast< uno_QueryInterfaceFunc >(
                            cpp_queryInterface),
                        reinterpret_cast< uno_AcquireFunc >(cpp_acquire),
                        reinterpret_cast< uno_ReleaseFunc >(cpp_release) ))
                {
                    OSL_ASSERT( false );
                }
            }
            TYPELIB_DANGER_RELEASE( pDestElementTD );
            TYPELIB_DANGER_RELEASE( pSourceElementTD );
        }
        break;
    }
    // --- to ENUM ------------------------------------------------------------------------------
    case TypeClass_ENUM:
    {
        TypeDescription aEnumTD( aDestType );
        aEnumTD.makeComplete();
        sal_Int32 nPos = -1;

        if (aSourceClass==TypeClass_STRING)
        {
            for ( nPos = reinterpret_cast<typelib_EnumTypeDescription *>(aEnumTD.get())->nEnumValues; nPos--; )
            {
                if (o3tl::forceAccess<OUString>(rVal)->equalsIgnoreAsciiCase(
                        OUString::unacquired(&reinterpret_cast<typelib_EnumTypeDescription *>(aEnumTD.get())->ppEnumNames[nPos]) ))
                    break;
            }
        }
        else if (aSourceClass!=TypeClass_ENUM && // exclude some unwanted types for toHyper()
                 aSourceClass!=TypeClass_BOOLEAN &&
                 aSourceClass!=TypeClass_CHAR)
        {
            sal_Int32 nEnumValue = static_cast<sal_Int32>(toHyper( rVal, -sal_Int64(0x80000000), 0x7fffffff ));
            for ( nPos = reinterpret_cast<typelib_EnumTypeDescription *>(aEnumTD.get())->nEnumValues; nPos--; )
            {
                if (nEnumValue == reinterpret_cast<typelib_EnumTypeDescription *>(aEnumTD.get())->pEnumValues[nPos])
                    break;
            }
        }

        if (nPos < 0)
        {
            throw CannotConvertException(
                u"value cannot be converted to demanded ENUM!"_ustr,
                Reference< XInterface >(), aDestinationClass, FailReason::IS_NOT_ENUM, 0 );
        }

        aRet.setValue(
            &reinterpret_cast<typelib_EnumTypeDescription *>(aEnumTD.get())->pEnumValues[nPos],
            aEnumTD.get() );

        break;
    }

    default:
        // else simple type conversion possible?
        try
        {
            aRet = convertToSimpleType( rVal, aDestinationClass );
        }
        catch (IllegalArgumentException &)
        {
            // ...FailReason::INVALID is thrown
        }
    }

    if (aRet.hasValue())
        return aRet;

    throw CannotConvertException(
        u"conversion not possible!"_ustr,
        Reference< XInterface >(), aDestinationClass, FailReason::INVALID, 0 );
}


Any TypeConverter_Impl::convertToSimpleType( const Any& rVal, TypeClass aDestinationClass )
{
    switch (aDestinationClass)
    {
        // only simple Conversion of _simple_ types
    case TypeClass_VOID:
    case TypeClass_BOOLEAN:
    case TypeClass_BYTE:
    case TypeClass_SHORT:
    case TypeClass_UNSIGNED_SHORT:
    case TypeClass_LONG:
    case TypeClass_UNSIGNED_LONG:
    case TypeClass_HYPER:
    case TypeClass_UNSIGNED_HYPER:
    case TypeClass_FLOAT:
    case TypeClass_DOUBLE:
    case TypeClass_CHAR:
    case TypeClass_STRING:
    case TypeClass_ANY:
        break;

    default:
        throw IllegalArgumentException(
            u"destination type is not simple!"_ustr,
            Reference< XInterface >(), sal_Int16(1) );
    }

    const Type& aSourceType = rVal.getValueType();
    TypeClass aSourceClass = aSourceType.getTypeClass();
    if (aDestinationClass == aSourceClass)
        return rVal;

    Any aRet;

    // Convert to...
    switch (aDestinationClass)
    {
    // --- to VOID ------------------------------------------------------------------------------
    case TypeClass_VOID:
        return Any();

    // --- to ANY -------------------------------------------------------------------------------
    case TypeClass_ANY:
        return rVal;

    // --- to BOOL ------------------------------------------------------------------------------
    case TypeClass_BOOLEAN:
        switch (aSourceClass)
        {
        default:
            aRet <<= (toDouble( rVal ) != 0.0);
            break;
        case TypeClass_ENUM:  // exclude enums
            break;

        case TypeClass_STRING:
        {
            const OUString & aStr = *o3tl::forceAccess<OUString>(rVal);
            if ( aStr == "0" || aStr.equalsIgnoreAsciiCase( "false" ))
            {
                aRet <<= false;
            }
            else if ( aStr == "1" || aStr.equalsIgnoreAsciiCase( "true" ))
            {
                aRet <<= true;
            }
            else
            {
                throw CannotConvertException(
                    "STRING has no boolean value, " + aStr,
                    Reference< XInterface >(), aDestinationClass, FailReason::IS_NOT_BOOL, 0 );
            }
        }
        }
        break;

    // --- to CHAR, BYTE ------------------------------------------------------------------------
    case TypeClass_CHAR:
    {
        if (aSourceClass==TypeClass_STRING)
        {
            auto const s = o3tl::forceAccess<OUString>(rVal);
            if (s->getLength() == 1)      // single char
                aRet <<= (*s)[0];
        }
        else if (aSourceClass!=TypeClass_ENUM &&        // exclude enums, chars
                 aSourceClass!=TypeClass_CHAR)
        {
            aRet <<= sal_Unicode(toHyper( rVal, 0, 0xffff ));    // range
        }
        break;
    }
    case TypeClass_BYTE:
        aRet <<= static_cast<sal_Int8>( toHyper( rVal, -sal_Int64(0x80), 0x7f ) );
        break;

    // --- to SHORT, UNSIGNED SHORT -------------------------------------------------------------
    case TypeClass_SHORT:
        aRet <<= static_cast<sal_Int16>( toHyper( rVal, -sal_Int64(0x8000), 0x7fff ) );
        break;
    case TypeClass_UNSIGNED_SHORT:
        aRet <<= static_cast<sal_uInt16>( toHyper( rVal, 0, 0xffff ) );
        break;

    // --- to LONG, UNSIGNED LONG ---------------------------------------------------------------
    case TypeClass_LONG:
        aRet <<= static_cast<sal_Int32>( toHyper( rVal, -sal_Int64(0x80000000), 0x7fffffff ) );
        break;
    case TypeClass_UNSIGNED_LONG:
        aRet <<= static_cast<sal_uInt32>( toHyper( rVal, 0, 0xffffffff ) );
        break;

    // --- to HYPER, UNSIGNED HYPER--------------------------------------------
    case TypeClass_HYPER:
        aRet <<= toHyper( rVal, SAL_MIN_INT64, SAL_MAX_INT64 );
        break;
    case TypeClass_UNSIGNED_HYPER:
        aRet <<= static_cast<sal_uInt64>( toHyper( rVal, 0 ) );
        break;

    // --- to FLOAT, DOUBLE ---------------------------------------------------------------------
    case TypeClass_FLOAT:
        aRet <<= static_cast<float>( toDouble( rVal, -FLT_MAX, FLT_MAX ) );
        break;
    case TypeClass_DOUBLE:
        aRet <<= toDouble( rVal, -DBL_MAX, DBL_MAX );
        break;

    // --- to STRING ----------------------------------------------------------------------------
    case TypeClass_STRING:
        switch (aSourceClass)
        {
        case TypeClass_ENUM:
        {
            TypeDescription aEnumTD( aSourceType );
            aEnumTD.makeComplete();
            sal_Int32 nPos;
            sal_Int32 nEnumValue = *static_cast<sal_Int32 const *>(rVal.getValue());
            for ( nPos = reinterpret_cast<typelib_EnumTypeDescription *>(aEnumTD.get())->nEnumValues; nPos--; )
            {
                if (nEnumValue == reinterpret_cast<typelib_EnumTypeDescription *>(aEnumTD.get())->pEnumValues[nPos])
                    break;
            }
            if (nPos < 0)
            {
                throw CannotConvertException(
                    u"value is not ENUM!"_ustr,
                    Reference< XInterface >(), aDestinationClass, FailReason::IS_NOT_ENUM, 0 );
            }

            aRet <<= OUString::unacquired(
                &reinterpret_cast<typelib_EnumTypeDescription *>(aEnumTD.get())->ppEnumNames[nPos]);

            break;
        }

        case TypeClass_BOOLEAN:
            aRet <<= *o3tl::forceAccess<bool>(rVal) ?
                u"true"_ustr :
                u"false"_ustr;
            break;
        case TypeClass_CHAR:
            aRet <<= OUString(*o3tl::forceAccess<sal_Unicode>(rVal));
            break;

        case TypeClass_BYTE:
            aRet <<= OUString::number( *o3tl::forceAccess<sal_Int8>(rVal) );
            break;
        case TypeClass_SHORT:
            aRet <<= OUString::number( *o3tl::forceAccess<sal_Int16>(rVal) );
            break;
        case TypeClass_UNSIGNED_SHORT:
            aRet <<= OUString::number( *o3tl::forceAccess<sal_uInt16>(rVal) );
            break;
        case TypeClass_LONG:
            aRet <<= OUString::number( *o3tl::forceAccess<sal_Int32>(rVal) );
            break;
        case TypeClass_UNSIGNED_LONG:
            aRet <<= OUString::number( *o3tl::forceAccess<sal_uInt32>(rVal) );
            break;
        case TypeClass_HYPER:
            aRet <<= OUString::number( *o3tl::forceAccess<sal_Int64>(rVal) );
            break;
//      case TypeClass_UNSIGNED_HYPER:
//             aRet <<= OUString::valueOf( (sal_Int64)*(sal_uInt64 const *)rVal.getValue() );
//          break;
            // handle unsigned hyper like double

        default:
            aRet <<= OUString::number( toDouble( rVal ) );
        }
        break;

    default:
        OSL_ASSERT(false);
        break;
    }

    if (aRet.hasValue())
        return aRet;

    throw CannotConvertException(
        u"conversion not possible!"_ustr,
        Reference< XInterface >(), aDestinationClass, FailReason::INVALID, 0 );
}

}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_stoc_TypeConverter_get_implementation(css::uno::XComponentContext*,
        css::uno::Sequence<css::uno::Any> const &)
{
    return ::cppu::acquire(new stoc_tcv::TypeConverter_Impl());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
