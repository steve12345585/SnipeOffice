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


#include "opropertybag.hxx"

#include <com/sun/star/beans/IllegalTypeException.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/Property.hpp>

#include <comphelper/namedvaluecollection.hxx>
#include <cppuhelper/supportsservice.hxx>

#include <cppuhelper/exc_hlp.hxx>

#include <algorithm>

namespace com::sun::star::uno { class XComponentContext; }

using namespace ::com::sun::star;

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
com_sun_star_comp_comphelper_OPropertyBag (
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new comphelper::OPropertyBag());
}

namespace comphelper
{


    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::lang;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::util;
    using namespace ::com::sun::star::container;

    OPropertyBag::OPropertyBag()
        :OPropertyBag_PBase( GetBroadcastHelper(), this )
        ,::cppu::IEventNotificationHook()
        ,m_bAutoAddProperties( false )
        ,m_NotifyListeners(m_aMutex)
        ,m_isModified(false)

    {
    }


    OPropertyBag::~OPropertyBag()
    {
    }


    IMPLEMENT_FORWARD_XINTERFACE2( OPropertyBag, OPropertyBag_Base, OPropertyBag_PBase )
    IMPLEMENT_FORWARD_XTYPEPROVIDER2( OPropertyBag, OPropertyBag_Base, OPropertyBag_PBase )

    void SAL_CALL OPropertyBag::initialize( const Sequence< Any >& _rArguments )
    {
        Sequence< Type > aTypes;
        bool AllowEmptyPropertyName(false);
        bool AutomaticAddition(false);

        if (_rArguments.getLength() == 3
           && (_rArguments[0] >>= aTypes)
           && (_rArguments[1] >>= AllowEmptyPropertyName)
           && (_rArguments[2] >>= AutomaticAddition))
        {
            m_aAllowedTypes.insert(std::cbegin(aTypes), std::cend(aTypes));
            m_bAutoAddProperties = AutomaticAddition;

        } else {
            ::comphelper::NamedValueCollection aArguments( _rArguments );

            if ( aArguments.get_ensureType( u"AllowedTypes"_ustr, aTypes ) )
                m_aAllowedTypes.insert(std::cbegin(aTypes), std::cend(aTypes));

            aArguments.get_ensureType( u"AutomaticAddition"_ustr, m_bAutoAddProperties );
            aArguments.get_ensureType( u"AllowEmptyPropertyName"_ustr,
                AllowEmptyPropertyName );
        }
        if (AllowEmptyPropertyName) {
            m_aDynamicProperties.setAllowEmptyPropertyName(
                AllowEmptyPropertyName);
        }
    }

    OUString SAL_CALL OPropertyBag::getImplementationName()
    {
        return u"com.sun.star.comp.comphelper.OPropertyBag"_ustr;
    }

    sal_Bool SAL_CALL OPropertyBag::supportsService( const OUString& rServiceName )
    {
        return cppu::supportsService(this, rServiceName);
    }

    Sequence< OUString > SAL_CALL OPropertyBag::getSupportedServiceNames(  )
    {
         return { u"com.sun.star.beans.PropertyBag"_ustr };
    }

    void OPropertyBag::fireEvents(
            sal_Int32 * /*pnHandles*/,
            sal_Int32 nCount,
            sal_Bool bVetoable,
            bool bIgnoreRuntimeExceptionsWhileFiring)
    {
        if (nCount && !bVetoable) {
            setModifiedImpl(true, bIgnoreRuntimeExceptionsWhileFiring);
        }
    }

    void OPropertyBag::setModifiedImpl(bool bModified,
            bool bIgnoreRuntimeExceptionsWhileFiring)
    {
        { // do not lock mutex while notifying (#i93514#) to prevent deadlock
            ::osl::MutexGuard aGuard( m_aMutex );
            m_isModified = bModified;
        }
        if (!bModified)
            return;

        try {
            Reference<XInterface> xThis(*this);
            EventObject event(xThis);
            m_NotifyListeners.notifyEach(
                &XModifyListener::modified, event);
        } catch (RuntimeException &) {
            if (!bIgnoreRuntimeExceptionsWhileFiring) {
                throw;
            }
        } catch (Exception &) {
            // ignore
        }
    }


    sal_Bool SAL_CALL OPropertyBag::isModified()
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        return m_isModified;
    }

    void SAL_CALL OPropertyBag::setModified( sal_Bool bModified )
    {
        setModifiedImpl(bModified, false);
    }

    void SAL_CALL OPropertyBag::addModifyListener(
        const Reference< XModifyListener > & xListener)
    {
        m_NotifyListeners.addInterface(xListener);
    }

    void SAL_CALL OPropertyBag::removeModifyListener(
        const Reference< XModifyListener > & xListener)
    {
        m_NotifyListeners.removeInterface(xListener);
    }


    Reference< XPropertySetInfo > SAL_CALL OPropertyBag::getPropertySetInfo(  )
    {
        return createPropertySetInfo( getInfoHelper() );
    }


    sal_Bool SAL_CALL OPropertyBag::has( const Any& /*aElement*/ )
    {
        // XSet is only a workaround for addProperty not being able to add default-void properties.
        // So, everything of XSet except insert is implemented empty
        return false;
    }


    void SAL_CALL OPropertyBag::insert( const Any& _element )
    {
        // This is a workaround for addProperty not being able to add default-void properties.
        // If we ever have a smarter XPropertyContainer::addProperty interface, we can remove this, ehm, well, hack.
        Property aProperty;
        if ( !( _element >>= aProperty ) )
            throw IllegalArgumentException( u"element is not Property"_ustr, *this, 1 );

        {
            osl::MutexGuard g(m_aMutex);

            // check whether the type is allowed, everything else will be checked
            // by m_aDynamicProperties
            if (!m_aAllowedTypes.empty()
                && m_aAllowedTypes.find(aProperty.Type) == m_aAllowedTypes.end())
                throw IllegalArgumentException(u"not in list of allowed types"_ustr, *this, 1);

            m_aDynamicProperties.addVoidProperty(aProperty.Name, aProperty.Type, findFreeHandle(),
                                                 aProperty.Attributes);

            // our property info is dirty
            m_pArrayHelper.reset();
        }
        setModified(true);
    }


    void SAL_CALL OPropertyBag::remove( const Any& /*aElement*/ )
    {
        // XSet is only a workaround for addProperty not being able to add default-void properties.
        // So, everything of XSet except insert is implemented empty
        throw NoSuchElementException( OUString(), *this );
    }


    Reference< XEnumeration > SAL_CALL OPropertyBag::createEnumeration(  )
    {
        // XSet is only a workaround for addProperty not being able to add default-void properties.
        // So, everything of XSet except insert is implemented empty
        return nullptr;
    }


    Type SAL_CALL OPropertyBag::getElementType(  )
    {
        // XSet is only a workaround for addProperty not being able to add default-void properties.
        // So, everything of XSet except insert is implemented empty
        return Type();
    }


    sal_Bool SAL_CALL OPropertyBag::hasElements(  )
    {
        // XSet is only a workaround for addProperty not being able to add default-void properties.
        // So, everything of XSet except insert is implemented empty
        return false;
    }


    void SAL_CALL OPropertyBag::getFastPropertyValue( Any& _rValue, sal_Int32 _nHandle ) const
    {
        m_aDynamicProperties.getFastPropertyValue( _nHandle, _rValue );
    }

    sal_Bool SAL_CALL OPropertyBag::convertFastPropertyValue( Any& _rConvertedValue, Any& _rOldValue, sal_Int32 _nHandle, const Any& _rValue )
    {
        return m_aDynamicProperties.convertFastPropertyValue( _nHandle, _rValue, _rConvertedValue, _rOldValue );
    }

    void SAL_CALL OPropertyBag::setFastPropertyValue_NoBroadcast( sal_Int32 nHandle, const Any& rValue )
    {
        m_aDynamicProperties.setFastPropertyValue( nHandle, rValue );
    }


    ::cppu::IPropertyArrayHelper& SAL_CALL OPropertyBag::getInfoHelper()
    {
        if (!m_pArrayHelper)
        {
            Sequence< Property > aProperties;
            m_aDynamicProperties.describeProperties( aProperties );
            m_pArrayHelper.reset( new ::cppu::OPropertyArrayHelper( aProperties ) );
        }
        return *m_pArrayHelper;

    }


    sal_Int32 OPropertyBag::findFreeHandle() const
    {
        const sal_Int32 nPrime = 1009;
        const sal_Int32 nSeed = 11;

        sal_Int32 nCheck = nSeed;
        while ( m_aDynamicProperties.hasPropertyByHandle( nCheck ) && ( nCheck != 1 ) )
        {
            nCheck = ( nCheck * nSeed ) % nPrime;
        }

        if ( nCheck == 1 )
        {   // uh ... we already have 1008 handles used up
            // -> simply count upwards
            while ( m_aDynamicProperties.hasPropertyByHandle( nCheck ) )
                ++nCheck;
        }

        return nCheck;
    }


    void SAL_CALL OPropertyBag::addProperty( const OUString& _rName, ::sal_Int16 _nAttributes, const Any& _rInitialValue )
    {
        {
            osl::MutexGuard g(m_aMutex);

            // check whether the type is allowed, everything else will be checked
            // by m_aDynamicProperties
            const Type& aPropertyType = _rInitialValue.getValueType();
            if (_rInitialValue.hasValue() && !m_aAllowedTypes.empty()
                && m_aAllowedTypes.find(aPropertyType) == m_aAllowedTypes.end())
                throw IllegalTypeException(OUString(), *this);

            m_aDynamicProperties.addProperty(_rName, findFreeHandle(), _nAttributes,
                                             _rInitialValue);

            // our property info is dirty
            m_pArrayHelper.reset();
        }
        setModified(true);
    }


    void SAL_CALL OPropertyBag::removeProperty( const OUString& _rName )
    {
        {
            osl::MutexGuard g(m_aMutex);

            m_aDynamicProperties.removeProperty(_rName);

            // our property info is dirty
            m_pArrayHelper.reset();
        }
        setModified(true);
    }


    namespace
    {
        struct ComparePropertyValueByName
        {
            bool operator()( const PropertyValue& _rLHS, const PropertyValue& _rRHS )
            {
                return _rLHS.Name < _rRHS.Name;
            }
        };

        template< typename CLASS >
        struct TransformPropertyToName
        {
            const OUString& operator()( const CLASS& _rProp )
            {
                return _rProp.Name;
            }
        };

        struct ExtractPropertyValue
        {
            const Any& operator()( const PropertyValue& _rProp )
            {
                return _rProp.Value;
            }
        };
    }


    Sequence< PropertyValue > SAL_CALL OPropertyBag::getPropertyValues(  )
    {
        ::osl::MutexGuard aGuard( m_aMutex );

        // all registered properties
        Sequence< Property > aProperties;
        m_aDynamicProperties.describeProperties( aProperties );

        // their names
        Sequence< OUString > aNames( aProperties.getLength() );
        std::transform(
            std::cbegin(aProperties),
            std::cend(aProperties),
            aNames.getArray(),
            TransformPropertyToName< Property >()
        );

        // their values
        Sequence< Any > aValues;
        try
        {
            aValues = OPropertyBag_PBase::getPropertyValues( aNames );
            if ( aValues.getLength() != aNames.getLength() )
                throw RuntimeException(u"property name and value counts out of sync"_ustr);
        }
        catch( const RuntimeException& )
        {
            throw;
        }
        catch( const Exception& )
        {
            // ignore
        }

        // merge names and values, and retrieve the state/handle
        ::cppu::IPropertyArrayHelper& rPropInfo = getInfoHelper();

        Sequence< PropertyValue > aPropertyValues( aNames.getLength() );
        PropertyValue* pPropertyValue = aPropertyValues.getArray();

        for (sal_Int32 i = 0; i < aNames.getLength(); ++i)
        {
            pPropertyValue[i].Name = aNames[i];
            pPropertyValue[i].Handle = rPropInfo.getHandleByName(aNames[i]);
            pPropertyValue[i].Value = aValues[i];
            pPropertyValue[i].State = getPropertyStateByHandle(pPropertyValue[i].Handle);
        }

        return aPropertyValues;
    }


    void OPropertyBag::impl_setPropertyValues_throw( const Sequence< PropertyValue >& _rProps )
    {
        // sort (the XMultiPropertySet interface requires this)
        Sequence< PropertyValue > aProperties( _rProps );
        auto [begin, end] = asNonConstRange(aProperties);
        std::sort(
            begin,
            end,
            ComparePropertyValueByName()
        );

        // a sequence of names
        Sequence< OUString > aNames( aProperties.getLength() );
        std::transform(
            std::cbegin(aProperties),
            std::cend(aProperties),
            aNames.getArray(),
            TransformPropertyToName< PropertyValue >()
        );

        try
        {
            // check for unknown properties
            // we cannot simply rely on the XMultiPropertySet::setPropertyValues
            // implementation of our base class, since it does not throw
            // an UnknownPropertyException. More precise, XMultiPropertySet::setPropertyValues
            // does not allow to throw this exception, while XPropertyAccess::setPropertyValues
            // requires it
            sal_Int32 nCount = aNames.getLength();

            Sequence< sal_Int32 > aHandles( nCount );
            sal_Int32* pHandles = aHandles.getArray();
            for (sal_Int32 i = 0; i < nCount; ++i)
            {
                ::cppu::IPropertyArrayHelper& rPropInfo = getInfoHelper();
                pHandles[i] = rPropInfo.getHandleByName(aNames[i]);
                if (pHandles[i] != -1)
                    continue;

                // there's a property requested which we do not know
                if ( m_bAutoAddProperties )
                {
                    // add the property
                    sal_Int16 const nAttributes = PropertyAttribute::BOUND | PropertyAttribute::REMOVABLE | PropertyAttribute::MAYBEDEFAULT;
                    addProperty(aNames[i], nAttributes, aProperties[i].Value);
                    continue;
                }

                // no way out
                throw UnknownPropertyException(aNames[i], *this);
            }

            // a sequence of values
            Sequence< Any > aValues( aProperties.getLength() );
            std::transform(
                std::cbegin(aProperties),
                std::cend(aProperties),
                aValues.getArray(),
                ExtractPropertyValue()
            );

            setFastPropertyValues(nCount, pHandles, aValues.getConstArray(), nCount);
        }
        catch( const PropertyVetoException& )       { throw; }
        catch( const IllegalArgumentException& )    { throw; }
        catch( const WrappedTargetException& )      { throw; }
        catch( const RuntimeException& )            { throw; }
        catch( const UnknownPropertyException& )    { throw; }
        catch( const Exception& )
        {
            throw WrappedTargetException( OUString(), *this, ::cppu::getCaughtException() );
        }
    }


    void SAL_CALL OPropertyBag::setPropertyValues( const Sequence< PropertyValue >& _rProps )
    {
        ::osl::MutexGuard aGuard( m_aMutex );
        impl_setPropertyValues_throw( _rProps );
    }


    PropertyState OPropertyBag::getPropertyStateByHandle( sal_Int32 _nHandle )
    {
        // for properties which do not support the MAYBEDEFAULT attribute, don't rely on the base class, but
        // assume they're always in DIRECT state.
        // (Note that this probably would belong into the base class. However, this would mean we would need
        // to check all existent usages of the base class, where MAYBEDEFAULT is *not* set, but
        // a default is nonetheless supplied/used. This is hard to accomplish reliably, in the
        // current phase. #i78593#

        ::cppu::IPropertyArrayHelper& rPropInfo = getInfoHelper();
        sal_Int16 nAttributes(0);
        OSL_VERIFY( rPropInfo.fillPropertyMembersByHandle( nullptr, &nAttributes, _nHandle ) );
        if ( ( nAttributes & PropertyAttribute::MAYBEDEFAULT ) == 0 )
            return PropertyState_DIRECT_VALUE;

        return OPropertyBag_PBase::getPropertyStateByHandle( _nHandle );
    }


    Any OPropertyBag::getPropertyDefaultByHandle( sal_Int32 _nHandle ) const
    {
        Any aDefault;
        m_aDynamicProperties.getPropertyDefaultByHandle( _nHandle, aDefault );
        return aDefault;
    }


}   // namespace comphelper


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
