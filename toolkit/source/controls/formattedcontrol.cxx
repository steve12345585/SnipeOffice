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

#include <controls/formattedcontrol.hxx>
#include <helper/property.hxx>

#include <com/sun/star/awt/XVclWindowPeer.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/util/NumberFormatter.hpp>
#include <com/sun/star/util/NumberFormatsSupplier.hpp>

#include <comphelper/diagnose_ex.hxx>
#include <comphelper/processfactory.hxx>
#include <osl/diagnose.h>

#include <helper/unopropertyarrayhelper.hxx>
#include <mutex>

namespace toolkit
{


    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::awt;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::util;


    namespace
    {

        std::mutex& getDefaultFormatsMutex()
        {
            static std::mutex s_aDefaultFormatsMutex;
            return s_aDefaultFormatsMutex;
        }

        Reference< XNumberFormatsSupplier > s_xDefaultFormats;
        bool s_bTriedCreation = false;
        oslInterlockedCount  s_refCount(0);

        const Reference< XNumberFormatsSupplier >& lcl_getDefaultFormats_throw()
        {
            std::scoped_lock aGuard( getDefaultFormatsMutex() );

            if ( !s_xDefaultFormats.is() && !s_bTriedCreation )
            {
                s_bTriedCreation = true;
                s_xDefaultFormats = NumberFormatsSupplier::createWithDefaultLocale( ::comphelper::getProcessComponentContext() );
            }
            if ( !s_xDefaultFormats.is() )
                throw RuntimeException();

            return s_xDefaultFormats;
        }

        void    lcl_registerDefaultFormatsClient()
        {
            osl_atomic_increment( &s_refCount );
        }

        void    lcl_revokeDefaultFormatsClient()
        {
            Reference< XNumberFormatsSupplier > xReleasePotentialLastReference;
            {
                std::scoped_lock aGuard( getDefaultFormatsMutex() );
                if ( 0 != osl_atomic_decrement( &s_refCount ) )
                    return;

                xReleasePotentialLastReference = std::move(s_xDefaultFormats);
                s_bTriedCreation = false;
            }
            xReleasePotentialLastReference.clear();
        }
    }


    // = UnoControlFormattedFieldModel


    UnoControlFormattedFieldModel::UnoControlFormattedFieldModel( const Reference< XComponentContext >& rxContext )
        :UnoControlModel( rxContext )
        ,m_bRevokedAsClient( false )
        ,m_bSettingValueAndText( false )
    {
        ImplRegisterProperty( BASEPROPERTY_ALIGN );
        ImplRegisterProperty( BASEPROPERTY_BACKGROUNDCOLOR );
        ImplRegisterProperty( BASEPROPERTY_BORDER );
        ImplRegisterProperty( BASEPROPERTY_BORDERCOLOR );
        ImplRegisterProperty( BASEPROPERTY_DEFAULTCONTROL );
        ImplRegisterProperty( BASEPROPERTY_EFFECTIVE_DEFAULT );
        ImplRegisterProperty( BASEPROPERTY_EFFECTIVE_VALUE );
        ImplRegisterProperty( BASEPROPERTY_EFFECTIVE_MAX );
        ImplRegisterProperty( BASEPROPERTY_EFFECTIVE_MIN );
        ImplRegisterProperty( BASEPROPERTY_ENABLED );
        ImplRegisterProperty( BASEPROPERTY_ENABLEVISIBLE );
        ImplRegisterProperty( BASEPROPERTY_FONTDESCRIPTOR );
        ImplRegisterProperty( BASEPROPERTY_FORMATKEY );
        ImplRegisterProperty( BASEPROPERTY_FORMATSSUPPLIER );
        ImplRegisterProperty( BASEPROPERTY_HELPTEXT );
        ImplRegisterProperty( BASEPROPERTY_HELPURL );
        ImplRegisterProperty( BASEPROPERTY_MAXTEXTLEN );
        ImplRegisterProperty( BASEPROPERTY_PRINTABLE );
        ImplRegisterProperty( BASEPROPERTY_REPEAT );
        ImplRegisterProperty( BASEPROPERTY_REPEAT_DELAY );
        ImplRegisterProperty( BASEPROPERTY_READONLY );
        ImplRegisterProperty( BASEPROPERTY_SPIN );
        ImplRegisterProperty( BASEPROPERTY_STRICTFORMAT );
        ImplRegisterProperty( BASEPROPERTY_TABSTOP );
        ImplRegisterProperty( BASEPROPERTY_TEXT );
        ImplRegisterProperty( BASEPROPERTY_TEXTCOLOR );
        ImplRegisterProperty( BASEPROPERTY_HIDEINACTIVESELECTION );
        ImplRegisterProperty( BASEPROPERTY_ENFORCE_FORMAT );
        ImplRegisterProperty( BASEPROPERTY_VERTICALALIGN );
        ImplRegisterProperty( BASEPROPERTY_WRITING_MODE );
        ImplRegisterProperty( BASEPROPERTY_CONTEXT_WRITING_MODE );
        ImplRegisterProperty( BASEPROPERTY_MOUSE_WHEEL_BEHAVIOUR );
        ImplRegisterProperty( BASEPROPERTY_HIGHLIGHT_COLOR );
        ImplRegisterProperty( BASEPROPERTY_HIGHLIGHT_TEXT_COLOR );

        Any aTreatAsNumber;
        aTreatAsNumber <<= true;
        ImplRegisterProperty( BASEPROPERTY_TREATASNUMBER, aTreatAsNumber );

        lcl_registerDefaultFormatsClient();
    }


    UnoControlFormattedFieldModel::~UnoControlFormattedFieldModel()
    {
    }


    OUString UnoControlFormattedFieldModel::getServiceName()
    {
        return u"stardiv.vcl.controlmodel.FormattedField"_ustr;
    }


    void UnoControlFormattedFieldModel::setFastPropertyValue_NoBroadcast( std::unique_lock<std::mutex>& rGuard, sal_Int32 nHandle, const Any& rValue )
    {
        UnoControlModel::setFastPropertyValue_NoBroadcast( rGuard, nHandle, rValue );

        switch ( nHandle )
        {
        case BASEPROPERTY_EFFECTIVE_VALUE:
            if ( !m_bSettingValueAndText )
                impl_updateTextFromValue_nothrow(rGuard);
            break;
        case BASEPROPERTY_FORMATSSUPPLIER:
            impl_updateCachedFormatter_nothrow(rGuard);
            impl_updateTextFromValue_nothrow(rGuard);
            break;
        case BASEPROPERTY_FORMATKEY:
            impl_updateCachedFormatKey_nothrow(rGuard);
            impl_updateTextFromValue_nothrow(rGuard);
            break;
        }
    }


    void UnoControlFormattedFieldModel::impl_updateTextFromValue_nothrow( std::unique_lock<std::mutex>& rGuard)
    {
        if ( !m_xCachedFormatter.is() )
            impl_updateCachedFormatter_nothrow(rGuard);
        if ( !m_xCachedFormatter.is() )
            return;

        try
        {
            Any aEffectiveValue;
            getFastPropertyValue( rGuard, aEffectiveValue, BASEPROPERTY_EFFECTIVE_VALUE );

            OUString sStringValue;
            if ( !( aEffectiveValue >>= sStringValue ) )
            {
                double nDoubleValue(0);
                if ( aEffectiveValue >>= nDoubleValue )
                {
                    sal_Int32 nFormatKey( 0 );
                    if ( m_aCachedFormat.hasValue() )
                        m_aCachedFormat >>= nFormatKey;
                    sStringValue = m_xCachedFormatter->convertNumberToString( nFormatKey, nDoubleValue );
                }
            }

            setFastPropertyValueImpl( rGuard, BASEPROPERTY_TEXT, Any( sStringValue ) );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        }
    }


    void UnoControlFormattedFieldModel::impl_updateCachedFormatter_nothrow(std::unique_lock<std::mutex>& rGuard)
    {
        Any aFormatsSupplier;
        getFastPropertyValue( rGuard, aFormatsSupplier, BASEPROPERTY_FORMATSSUPPLIER );
        try
        {
            Reference< XNumberFormatsSupplier > xSupplier( aFormatsSupplier, UNO_QUERY );
            if ( !xSupplier.is() )
                xSupplier = lcl_getDefaultFormats_throw();

            if ( !m_xCachedFormatter.is() )
            {
                m_xCachedFormatter.set(
                    NumberFormatter::create(::comphelper::getProcessComponentContext()),
                    UNO_QUERY_THROW
                );
            }
            m_xCachedFormatter->attachNumberFormatsSupplier( xSupplier );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        }
    }

    void UnoControlFormattedFieldModel::impl_updateCachedFormatKey_nothrow(std::unique_lock<std::mutex>& rGuard)
    {
        Any aFormatKey;
        getFastPropertyValue( rGuard, aFormatKey, BASEPROPERTY_FORMATKEY );
        m_aCachedFormat = std::move(aFormatKey);
    }

    void UnoControlFormattedFieldModel::dispose(  )
    {
        UnoControlModel::dispose();

        std::unique_lock aGuard( m_aMutex );
        if ( !m_bRevokedAsClient )
        {
            lcl_revokeDefaultFormatsClient();
            m_bRevokedAsClient = true;
        }
    }


    void UnoControlFormattedFieldModel::ImplNormalizePropertySequence( const sal_Int32 _nCount, sal_Int32* _pHandles,
        Any* _pValues, sal_Int32* _pValidHandles ) const
    {
        ImplEnsureHandleOrder( _nCount, _pHandles, _pValues, BASEPROPERTY_EFFECTIVE_VALUE, BASEPROPERTY_TEXT );

        UnoControlModel::ImplNormalizePropertySequence( _nCount, _pHandles, _pValues, _pValidHandles );
    }


    namespace
    {
        class ResetFlagOnExit
        {
        private:
            bool&   m_rFlag;

        public:
            explicit ResetFlagOnExit( bool& _rFlag )
                :m_rFlag( _rFlag )
            {
            }
            ~ResetFlagOnExit()
            {
                m_rFlag = false;
            }
        };
    }


    void SAL_CALL UnoControlFormattedFieldModel::setPropertyValues( const Sequence< OUString >& _rPropertyNames, const Sequence< Any >& _rValues )
    {
        bool bSettingValue = false;
        bool bSettingText = false;
        for ( auto const & propertyName : _rPropertyNames )
        {
            if ( BASEPROPERTY_EFFECTIVE_VALUE == GetPropertyId( propertyName ) )
                bSettingValue = true;

            if ( BASEPROPERTY_TEXT == GetPropertyId( propertyName ) )
                bSettingText = true;
        }

        m_bSettingValueAndText = ( bSettingValue && bSettingText );
        ResetFlagOnExit aResetFlag( m_bSettingValueAndText );
        UnoControlModel::setPropertyValues( _rPropertyNames, _rValues );
    }


    bool UnoControlFormattedFieldModel::convertFastPropertyValue(
                std::unique_lock<std::mutex>& rGuard,
                Any& rConvertedValue, Any& rOldValue, sal_Int32 nPropId,
                const Any& rValue )
    {
        if ( BASEPROPERTY_EFFECTIVE_DEFAULT == nPropId && rValue.hasValue() )
        {
            double dVal = 0;
            bool bStreamed = (rValue >>= dVal);
            if ( bStreamed )
            {
                rConvertedValue <<= dVal;
            }
            else
            {
                sal_Int32  nVal = 0;
                bStreamed = (rValue >>= nVal);
                if ( bStreamed )
                {
                    rConvertedValue <<= static_cast<double>(nVal);
                }
                else
                {
                    OUString sVal;
                    bStreamed = (rValue >>= sVal);
                    if ( bStreamed )
                    {
                        rConvertedValue <<= sVal;
                    }
                }
            }

            if ( bStreamed )
            {
                getFastPropertyValue( rGuard, rOldValue, nPropId );
                return !CompareProperties( rConvertedValue, rOldValue );
            }

            throw IllegalArgumentException(
                ("Unable to convert the given value for the property "
                 + GetPropertyName(static_cast<sal_uInt16>(nPropId))
                 + " (double, integer, or string expected)."),
                static_cast< XPropertySet* >(this),
                1);
        }

        return UnoControlModel::convertFastPropertyValue( rGuard, rConvertedValue, rOldValue, nPropId, rValue );
    }


    Any UnoControlFormattedFieldModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
    {
        Any aReturn;
        switch (nPropId)
        {
            case BASEPROPERTY_DEFAULTCONTROL: aReturn <<= u"stardiv.vcl.control.FormattedField"_ustr; break;

            case BASEPROPERTY_TREATASNUMBER: aReturn <<= true; break;

            case BASEPROPERTY_EFFECTIVE_DEFAULT:
            case BASEPROPERTY_EFFECTIVE_VALUE:
            case BASEPROPERTY_EFFECTIVE_MAX:
            case BASEPROPERTY_EFFECTIVE_MIN:
            case BASEPROPERTY_FORMATKEY:
            case BASEPROPERTY_FORMATSSUPPLIER:
                // (void)
                break;

            default : aReturn = UnoControlModel::ImplGetDefaultValue( nPropId ); break;
        }

        return aReturn;
    }


    ::cppu::IPropertyArrayHelper& UnoControlFormattedFieldModel::getInfoHelper()
    {
        static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
        return aHelper;
    }

    // beans::XMultiPropertySet

    Reference< XPropertySetInfo > UnoControlFormattedFieldModel::getPropertySetInfo(  )
    {
        static Reference< XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
        return xInfo;
    }

    OUString UnoControlFormattedFieldModel::getImplementationName()
    {
        return u"stardiv.Toolkit.UnoControlFormattedFieldModel"_ustr;
    }

    css::uno::Sequence<OUString>
    UnoControlFormattedFieldModel::getSupportedServiceNames()
    {
        auto s(UnoControlModel::getSupportedServiceNames());
        s.realloc(s.getLength() + 2);
        auto ps = s.getArray();
        ps[s.getLength() - 2] = "com.sun.star.awt.UnoControlFormattedFieldModel";
        ps[s.getLength() - 1] = "stardiv.vcl.controlmodel.FormattedField";
        return s;
    }

    // = UnoFormattedFieldControl


    UnoFormattedFieldControl::UnoFormattedFieldControl()
    {
    }


    OUString UnoFormattedFieldControl::GetComponentServiceName() const
    {
        return u"FormattedField"_ustr;
    }


    void UnoFormattedFieldControl::textChanged(const TextEvent& e)
    {
        Reference< XVclWindowPeer >  xPeer(getPeer(), UNO_QUERY);
        OSL_ENSURE(xPeer.is(), "UnoFormattedFieldControl::textChanged : what kind of peer do I have ?");

        Sequence< OUString > aNames{ GetPropertyName( BASEPROPERTY_EFFECTIVE_VALUE ),
                                     GetPropertyName( BASEPROPERTY_TEXT ) };

        Sequence< Any > aValues{ xPeer->getProperty( aNames[0] ),
                                 xPeer->getProperty( aNames[1] ) };

        ImplSetPropertyValues( aNames, aValues, false );

        if ( GetTextListeners().getLength() )
            GetTextListeners().textChanged( e );
    }

    OUString UnoFormattedFieldControl::getImplementationName()
    {
        return u"stardiv.Toolkit.UnoFormattedFieldControl"_ustr;
    }

    css::uno::Sequence<OUString>
    UnoFormattedFieldControl::getSupportedServiceNames()
    {
        auto s(UnoEditControl::getSupportedServiceNames());
        s.realloc(s.getLength() + 2);
        auto ps = s.getArray();
        ps[s.getLength() - 2] = "com.sun.star.awt.UnoControlFormattedField";
        ps[s.getLength() - 1] = "stardiv.vcl.control.FormattedField";
        return s;
    }
}   // namespace toolkit


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlFormattedFieldModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new toolkit::UnoControlFormattedFieldModel(context));
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoFormattedFieldControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new toolkit::UnoFormattedFieldControl());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
