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

#include <cppuhelper/implbase.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/inspection/XStringRepresentation.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/script/CannotConvertException.hpp>
#include <com/sun/star/script/XTypeConverter.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/reflection/XConstantsTypeDescription.hpp>
#include <com/sun/star/util/DateTime.hpp>
#include <com/sun/star/util/Date.hpp>
#include <com/sun/star/util/Time.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <connectivity/dbconversion.hxx>
#include <osl/diagnose.h>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include <utility>
#include <yesno.hrc>
#include <comphelper/types.hxx>
#include <o3tl/string_view.hxx>
#include "modulepcr.hxx"

#include <algorithm>

namespace pcr{

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;

namespace {

class StringRepresentation:
    public ::cppu::WeakImplHelper<
        lang::XServiceInfo,
        inspection::XStringRepresentation,
        lang::XInitialization>
{
public:
    explicit StringRepresentation(uno::Reference< uno::XComponentContext > context);
    StringRepresentation (const StringRepresentation&) = delete;
    StringRepresentation& operator=(const StringRepresentation&) = delete;

    // lang::XServiceInfo:
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString & ServiceName) override;
    virtual uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // inspection::XStringRepresentation:
    virtual OUString SAL_CALL convertToControlValue(const uno::Any & PropertyValue) override;
    virtual uno::Any SAL_CALL convertToPropertyValue(const OUString & ControlValue, const uno::Type & ControlValueType) override;

    // lang::XInitialization:
    virtual void SAL_CALL initialize(const uno::Sequence< uno::Any > & aArguments) override;

private:
    virtual ~StringRepresentation() override {}

    /** converts a generic value into a string representation

        If you want to convert values whose string representation does not depend
        on a concrete property, use this version

        @return <TRUE/>
            if and only if the value could be converted
    */
    static bool     convertGenericValueToString(
                        const uno::Any&   _rValue,
                                OUString&              _rStringRep
                    );

    /** converts string representation into generic value

        If you want to convert values whose string representation does not depend
        on a concrete property, use this version

        @return <TRUE/>
            if and only if the value could be converted
    */
    static bool    convertStringToGenericValue(
                        const OUString&              _rStringRep,
                                uno::Any&   _rValue,
                        const uno::Type& _rTargetType
                    );

    /** uses the simple convert method from the type converter
    *
    * \param _rValue the value to be converted
    * \return the converted string.
    */
    OUString convertSimpleToString( const uno::Any& _rValue );

    /** converts a string into his constant value if it exists, otherwise the type converter is used.
    * \param _rValue the value to be converted
    * \param _ePropertyType the type of the property to be converted into
    * \return the converted value
    */
    uno::Any convertStringToSimple( const OUString& _rValue,const uno::TypeClass& _ePropertyType );

    uno::Reference< uno::XComponentContext >                                m_xContext;
    uno::Reference< script::XTypeConverter >                                m_xTypeConverter;
    uno::Reference< reflection::XConstantsTypeDescription >                 m_xTypeDescription;
    uno::Sequence< OUString >                                        m_aValues;
    uno::Sequence< uno::Reference< reflection::XConstantTypeDescription> >  m_aConstants;

};

}

StringRepresentation::StringRepresentation(uno::Reference< uno::XComponentContext > context) :
    m_xContext(std::move(context))
{}

// com.sun.star.uno.XServiceInfo:
OUString  SAL_CALL StringRepresentation::getImplementationName()
{
    return u"StringRepresentation"_ustr;
}

sal_Bool SAL_CALL StringRepresentation::supportsService(OUString const & serviceName)
{
    return cppu::supportsService(this, serviceName);
}

uno::Sequence< OUString >  SAL_CALL StringRepresentation::getSupportedServiceNames()
{
    return { u"com.sun.star.inspection.StringRepresentation"_ustr };
}

// inspection::XStringRepresentation:
OUString SAL_CALL StringRepresentation::convertToControlValue(const uno::Any & PropertyValue)
{
    OUString sReturn;
    if ( !convertGenericValueToString( PropertyValue, sReturn ) )
    {
        sReturn = convertSimpleToString( PropertyValue );
#ifdef DBG_UTIL
        if ( sReturn.isEmpty() && PropertyValue.hasValue() )
        {
            SAL_WARN( "extensions.propctrlr", "StringRepresentation::convertPropertyValueToStringRepresentation: cannot convert values of type '"
                        << PropertyValue.getValueTypeName()
                        << "'!" );
        }
#endif
    }

    return sReturn;
}

uno::Any SAL_CALL StringRepresentation::convertToPropertyValue(const OUString & ControlValue, const uno::Type & ControlValueType)
{
    uno::Any aReturn;

    uno::TypeClass ePropertyType = ControlValueType.getTypeClass();
    switch ( ePropertyType )
    {
    case uno::TypeClass_FLOAT:
    case uno::TypeClass_DOUBLE:
    case uno::TypeClass_BYTE:
    case uno::TypeClass_SHORT:
    case uno::TypeClass_LONG:
    case uno::TypeClass_HYPER:
    case uno::TypeClass_UNSIGNED_SHORT:
    case uno::TypeClass_UNSIGNED_LONG:
    case uno::TypeClass_UNSIGNED_HYPER:
        try
        {
            aReturn = convertStringToSimple(ControlValue, ePropertyType);
        }
        catch( const script::CannotConvertException& ) { }
        catch( const lang::IllegalArgumentException& ) { }
        break;

    default:
    #if OSL_DEBUG_LEVEL > 0
        bool bCanConvert =
    #endif
        convertStringToGenericValue( ControlValue, aReturn, ControlValueType );

    #if OSL_DEBUG_LEVEL > 0
        // could not convert ...
        if ( !bCanConvert && !ControlValue.isEmpty() )
        {
            SAL_WARN( "extensions.propctrlr", "StringRepresentation::convertStringRepresentationToPropertyValue: cannot convert into values of type '"
            << ControlValueType.getTypeName() << "'!" );
        }
    #endif
    }

    return aReturn;
}

namespace {

// This comparison functor assumes an underlying set of constants with pairwise
// unequal values that are all of UNO SHORT or LONG type:
struct CompareConstants {
    bool operator ()(
        css::uno::Reference< css::reflection::XConstantTypeDescription > const &
            c1,
        css::uno::Reference< css::reflection::XConstantTypeDescription > const &
            c2) const
    {
        return c1->getConstantValue().get<sal_Int32>()
            < c2->getConstantValue().get<sal_Int32>();
    }
};

}

// lang::XInitialization:
void SAL_CALL StringRepresentation::initialize(const uno::Sequence< uno::Any > & aArguments)
{
    sal_Int32 nLength = aArguments.getLength();
    if ( !nLength )
        return;

    const uno::Any* pIter = aArguments.getConstArray();
    m_xTypeConverter.set(*pIter++,uno::UNO_QUERY);
    if ( nLength != 3 )
        return;

    OUString sConstantName;
    *pIter++ >>= sConstantName;
    *pIter >>= m_aValues;

    if ( !m_xContext.is() )
        return;

    uno::Reference< container::XHierarchicalNameAccess > xTypeDescProv(
        m_xContext->getValueByName(u"/singletons/com.sun.star.reflection.theTypeDescriptionManager"_ustr),
        uno::UNO_QUERY_THROW );

    m_xTypeDescription.set( xTypeDescProv->getByHierarchicalName( sConstantName ), uno::UNO_QUERY_THROW );
    uno::Sequence<
        uno::Reference< reflection::XConstantTypeDescription > >
        cs(m_xTypeDescription->getConstants());
    auto [begin, end] = asNonConstRange(cs);
    std::sort(begin, end, CompareConstants());
    m_aConstants = std::move(cs);
}

OUString StringRepresentation::convertSimpleToString( const uno::Any& _rValue )
{
    OUString sReturn;
    if ( m_xTypeConverter.is() && _rValue.hasValue() )
    {
        try
        {
            if ( m_aConstants.hasElements() )
            {
                sal_Int16 nConstantValue = 0;
                if ( _rValue >>= nConstantValue )
                {
                    const uno::Reference< reflection::XConstantTypeDescription>* pIter = m_aConstants.getConstArray();
                    const uno::Reference< reflection::XConstantTypeDescription>* pEnd  = pIter + m_aConstants.getLength();
                    for(sal_Int32 i = 0;pIter != pEnd;++pIter,++i)
                    {
                        if ( (*pIter)->getConstantValue() == _rValue )
                        {
                            OSL_ENSURE(i < m_aValues.getLength() ,"StringRepresentation::convertSimpleToString: Index is not in range of m_aValues");
                            sReturn = m_aValues[i];
                            break;
                        }
                    }
                }
            }

            if ( sReturn.isEmpty() )
                m_xTypeConverter->convertToSimpleType( _rValue, uno::TypeClass_STRING ) >>= sReturn;
        }
        catch( const script::CannotConvertException& ) { }
        catch( const lang::IllegalArgumentException& ) { }
    }
    return sReturn;
}


namespace
{
    struct ConvertIntegerFromAndToString
    {
        OUString operator()( sal_Int32 _rIntValue ) const
        {
            return OUString::number( _rIntValue );
        }
        sal_Int32 operator()( std::u16string_view _rStringValue ) const
        {
            return o3tl::toInt32(_rStringValue);
        }
    };

    struct StringIdentity
    {
        OUString operator()( const OUString& _rValue ) const
        {
            return _rValue;
        }
    };

    template < class ElementType, class Transformer >
    OUString composeSequenceElements( const Sequence< ElementType >& _rElements, const Transformer& _rTransformer )
    {
        OUStringBuffer sCompose;

        // loop through the elements and concatenate the string representations of the integers
        // (separated by a line break)
        for (const auto& rElement : _rElements)
        {
            sCompose.append(OUString(_rTransformer(rElement)) + "\n");
        }
        sCompose.stripEnd('\n');

        return sCompose.makeStringAndClear();
    }

    template < class ElementType, class Transformer >
    void splitComposedStringToSequence( std::u16string_view _rComposed, Sequence< ElementType >& _out_SplitUp, const Transformer& _rTransformer )
    {
        _out_SplitUp.realloc( 0 );
        if ( _rComposed.empty() )
            return;
        sal_Int32 tokenPos = 0;
        do
        {
            _out_SplitUp.realloc( _out_SplitUp.getLength() + 1 );
            _out_SplitUp.getArray()[ _out_SplitUp.getLength() - 1 ] = static_cast<ElementType>(_rTransformer( OUString(o3tl::getToken(_rComposed, 0, '\n', tokenPos )) ));
        }
        while ( tokenPos != -1 );
    }
}


bool StringRepresentation::convertGenericValueToString( const uno::Any& _rValue, OUString& _rStringRep )
{
    bool bCanConvert = true;

    switch ( _rValue.getValueTypeClass() )
    {
    case uno::TypeClass_STRING:
        _rValue >>= _rStringRep;
        break;

    case uno::TypeClass_BOOLEAN:
    {
        bool bValue = false;
        _rValue >>= bValue;
        _rStringRep = bValue ? PcrRes(RID_RSC_ENUM_YESNO[1])
                             : PcrRes(RID_RSC_ENUM_YESNO[0]);
    }
    break;

    // some sequence types
    case uno::TypeClass_SEQUENCE:
    {
        Sequence< OUString > aStringValues;
        Sequence< sal_Int8 > aInt8Values;
        Sequence< sal_uInt16 > aUInt16Values;
        Sequence< sal_Int16 > aInt16Values;
        Sequence< sal_uInt32 > aUInt32Values;
        Sequence< sal_Int32 > aInt32Values;

        // string sequences
        if ( _rValue >>= aStringValues )
        {
            _rStringRep = composeSequenceElements( aStringValues, StringIdentity() );
        }
        // byte sequences
        else if ( _rValue >>= aInt8Values )
        {
            _rStringRep = composeSequenceElements( aInt8Values, ConvertIntegerFromAndToString() );
        }
        // uInt16 sequences
        else if ( _rValue >>= aUInt16Values )
        {
            _rStringRep = composeSequenceElements( aUInt16Values, ConvertIntegerFromAndToString() );
        }
        // Int16 sequences
        else if ( _rValue >>= aInt16Values )
        {
            _rStringRep = composeSequenceElements( aInt16Values, ConvertIntegerFromAndToString() );
        }
        // uInt32 sequences
        else if ( _rValue >>= aUInt32Values )
        {
            _rStringRep = composeSequenceElements( aUInt32Values, ConvertIntegerFromAndToString() );
        }
        // Int32 sequences
        else if ( _rValue >>= aInt32Values )
        {
            _rStringRep = composeSequenceElements( aInt32Values, ConvertIntegerFromAndToString() );
        }
        else
            bCanConvert = false;
    }
    break;
    case uno::TypeClass_CONSTANT:
        break;

    // some structs
    case uno::TypeClass_STRUCT:
        OSL_FAIL( "StringRepresentation::convertGenericValueToString(STRUCT): this is dead code - isn't it?" );
        if ( _rValue.getValueType().equals( cppu::UnoType< util::Date >::get() ))
        {
            // weird enough, the string representation of dates, as used
            // by the control displaying dates, and thus as passed through the layers,
            // is YYYYMMDD.
            util::Date aUnoDate;
            _rValue >>= aUnoDate;
            _rStringRep = ::dbtools::DBTypeConversion::toDateString(aUnoDate);
        }
        else if ( _rValue.getValueType().equals( cppu::UnoType< util::Time >::get() ))
        {
            // similar for time (HHMMSSHH)
            util::Time aUnoTime;
            _rValue >>= aUnoTime;
            _rStringRep = ::dbtools::DBTypeConversion::toTimeString(aUnoTime);
        }
        else if ( _rValue.getValueType().equals( cppu::UnoType< util::DateTime >::get() ))
        {
            util::DateTime aUnoDateTime;
            _rValue >>= aUnoDateTime;
            _rStringRep = ::dbtools::DBTypeConversion::toDateTimeString(aUnoDateTime);
        }
        else
            bCanConvert = false;
        break;

    default:
        bCanConvert = false;
        break;
    }

    return bCanConvert;
}

uno::Any StringRepresentation::convertStringToSimple( const OUString& _rValue,const uno::TypeClass& _ePropertyType )
{
    uno::Any aReturn;
    if ( m_xTypeConverter.is() && !_rValue.isEmpty() )
    {
        try
        {
            if ( m_aConstants.hasElements() && m_aValues.hasElements() )
            {
                const OUString* pIter = m_aValues.getConstArray();
                const OUString* pEnd   = pIter + m_aValues.getLength();
                for(sal_Int32 i = 0;pIter != pEnd;++pIter,++i)
                {
                    if ( *pIter == _rValue )
                    {
                        OSL_ENSURE(i < m_aConstants.getLength() ,"StringRepresentation::convertSimpleToString: Index is not in range of m_aValues");
                        aReturn = m_aConstants[i]->getConstantValue();
                        break;
                    }
                }
            }

            if ( !aReturn.hasValue() )
                aReturn = m_xTypeConverter->convertToSimpleType( Any( _rValue ), _ePropertyType );
        }
        catch( const script::CannotConvertException& ) { }
        catch( const lang::IllegalArgumentException& ) { }
    }
    return aReturn;
}

bool StringRepresentation::convertStringToGenericValue( const OUString& _rStringRep, uno::Any& _rValue, const uno::Type& _rTargetType )
{
    bool bCanConvert = true;

    switch ( _rTargetType.getTypeClass() )
    {
    case uno::TypeClass_STRING:
        _rValue <<= _rStringRep;
        break;

    case uno::TypeClass_BOOLEAN:
    {
        _rValue <<= PcrRes(RID_RSC_ENUM_YESNO[0]) != _rStringRep;
    }
    break;

    case uno::TypeClass_SEQUENCE:
    {
        uno::Type aElementType = ::comphelper::getSequenceElementType( _rTargetType );

        switch ( aElementType.getTypeClass() )
        {
            case uno::TypeClass_STRING:
            {
                Sequence< OUString > aElements;
                splitComposedStringToSequence( _rStringRep, aElements, StringIdentity() );
                _rValue <<= aElements;
            }
            break;
            case uno::TypeClass_SHORT:
            {
                Sequence< sal_Int16 > aElements;
                splitComposedStringToSequence( _rStringRep, aElements, ConvertIntegerFromAndToString() );
                _rValue <<= aElements;
            }
            break;
            case uno::TypeClass_UNSIGNED_SHORT:
            {
                Sequence< sal_uInt16 > aElements;
                splitComposedStringToSequence( _rStringRep, aElements, ConvertIntegerFromAndToString() );
                _rValue <<= aElements;
            }
            break;
            case uno::TypeClass_LONG:
            {
                Sequence< sal_Int32 > aElements;
                splitComposedStringToSequence( _rStringRep, aElements, ConvertIntegerFromAndToString() );
                _rValue <<= aElements;
            }
            break;
            case uno::TypeClass_UNSIGNED_LONG:
            {
                Sequence< sal_uInt32 > aElements;
                splitComposedStringToSequence( _rStringRep, aElements, ConvertIntegerFromAndToString() );
                _rValue <<= aElements;
            }
            break;
            case uno::TypeClass_BYTE:
            {
                Sequence< sal_Int8 > aElements;
                splitComposedStringToSequence( _rStringRep, aElements, ConvertIntegerFromAndToString() );
                _rValue <<= aElements;
            }
            break;
            default:
                bCanConvert = false;
                break;
        }
    }
    break;

    case uno::TypeClass_STRUCT:
        OSL_FAIL( "StringRepresentation::convertStringToGenericValue(STRUCT): this is dead code - isn't it?" );
        if ( _rTargetType.equals( cppu::UnoType< util::Date >::get() ))
        {
            // weird enough, the string representation of dates, as used
            // by the control displaying dates, and thus as passed through the layers,
            // is YYYYMMDD.

            _rValue <<= ::dbtools::DBTypeConversion::toDate(_rStringRep);
        }
        else if ( _rTargetType.equals( cppu::UnoType< util::Time >::get() ))
        {
            // similar for time (HHMMSSHH)
            _rValue <<= ::dbtools::DBTypeConversion::toTime(_rStringRep);
        }
        else if ( _rTargetType.equals( cppu::UnoType< util::DateTime >::get() ))
        {
            _rValue <<= ::dbtools::DBTypeConversion::toDateTime(_rStringRep);
        }
        else
            bCanConvert = false;
        break;

    default:
        bCanConvert = false;
        break;
    }

    return bCanConvert;
}


} // pcr


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
extensions_propctrlr_StringRepresentation_get_implementation(
    css::uno::XComponentContext* context , css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new pcr::StringRepresentation(context));
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
