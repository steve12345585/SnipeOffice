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

#ifndef INCLUDED_VBAHELPER_VBACOLLECTIONIMPL_HXX
#define INCLUDED_VBAHELPER_VBACOLLECTIONIMPL_HXX

#include <exception>
#include <utility>
#include <vector>

#include <com/sun/star/container/NoSuchElementException.hpp>
#include <com/sun/star/container/XEnumeration.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/lang/WrappedTargetException.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/Type.hxx>
#include <com/sun/star/uno/TypeClass.hpp>
#include <cppu/unotype.hxx>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/weakref.hxx>
#include <ooo/vba/XCollection.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <vbahelper/vbadllapi.h>
#include <vbahelper/vbahelper.hxx>
#include <vbahelper/vbahelperinterface.hxx>

namespace com::sun::star {
    namespace container { class XEnumerationAccess; }
    namespace uno { class XComponentContext; }
}

namespace ooo::vba {
    class XHelperInterface;
}

typedef ::cppu::WeakImplHelper< css::container::XEnumeration > EnumerationHelper_BASE;


/** A wrapper that holds a com.sun.star.container.XIndexAccess and provides a
    com.sun.star.container.XEnumeration.

    Can be used to provide an enumeration from an index container that contains
    completely constructed/initialized VBA implementation objects. CANNOT be
    used to provide an enumeration from an index container with other objects
    (e.g. UNO objects) where construction of the VBA objects is needed first.
 */
class VBAHELPER_DLLPUBLIC SimpleIndexAccessToEnumeration final : public EnumerationHelper_BASE
{
public:
    /// @throws css::uno::RuntimeException
    explicit SimpleIndexAccessToEnumeration(
            css::uno::Reference< css::container::XIndexAccess > xIndexAccess ) :
        mxIndexAccess(std::move( xIndexAccess )), mnIndex( 0 ) {}

    virtual sal_Bool SAL_CALL hasMoreElements() override
    {
        return mnIndex < mxIndexAccess->getCount();
    }

    virtual css::uno::Any SAL_CALL nextElement() override
    {
        if( !hasMoreElements() )
            throw css::container::NoSuchElementException();
        return mxIndexAccess->getByIndex( mnIndex++ );
    }

private:
    css::uno::Reference< css::container::XIndexAccess > mxIndexAccess;
    sal_Int32 mnIndex;
};


/** A wrapper that holds a com.sun.star.container.XEnumeration or a
    com.sun.star.container.XIndexAccess and provides an enumeration of VBA objects.

    The method createCollectionObject() needs to be implemented by the derived
    class. This class can be used to convert an enumeration or an index container
    containing UNO objects to an enumeration providing the related VBA objects.
 */
class VBAHELPER_DLLPUBLIC SimpleEnumerationBase : public EnumerationHelper_BASE
{
public:
    /// @throws css::uno::RuntimeException
    explicit SimpleEnumerationBase(
            const css::uno::Reference< css::container::XIndexAccess >& rxIndexAccess ) :
        mxEnumeration( new SimpleIndexAccessToEnumeration( rxIndexAccess ) ) {}

    virtual sal_Bool SAL_CALL hasMoreElements() override
    {
        return mxEnumeration->hasMoreElements();
    }

    virtual css::uno::Any SAL_CALL nextElement() override
    {
        return createCollectionObject( mxEnumeration->nextElement() );
    }

    /** Derived classes implement creation of a VBA implementation object from
        the passed container element. */
    virtual css::uno::Any createCollectionObject( const css::uno::Any& rSource ) = 0;

private:
    css::uno::Reference< css::container::XEnumeration > mxEnumeration;
};


// deprecated, use SimpleEnumerationBase instead!
class VBAHELPER_DLLPUBLIC EnumerationHelperImpl : public EnumerationHelper_BASE
{
protected:
    css::uno::WeakReference< ov::XHelperInterface > m_xParent;
    css::uno::Reference< css::uno::XComponentContext > m_xContext;
    css::uno::Reference< css::container::XEnumeration > m_xEnumeration;
public:
    /// @throws css::uno::RuntimeException
    EnumerationHelperImpl( const css::uno::Reference< ov::XHelperInterface >& xParent, css::uno::Reference< css::uno::XComponentContext >  xContext, css::uno::Reference< css::container::XEnumeration > xEnumeration ) : m_xParent( xParent ), m_xContext(std::move( xContext )),  m_xEnumeration(std::move( xEnumeration )) { }
    virtual sal_Bool SAL_CALL hasMoreElements(  ) override { return m_xEnumeration->hasMoreElements(); }
};

// a wrapper class for a providing a XIndexAccess, XNameAccess, XEnumerationAccess impl based on providing a vector of interfaces
// only requirement is the object needs to implement XName


template< typename OneIfc >
class XNamedObjectCollectionHelper final : public ::cppu::WeakImplHelper< css::container::XNameAccess,
                                                                    css::container::XIndexAccess,
                                                                    css::container::XEnumerationAccess >
{
public:
typedef std::vector< css::uno::Reference< OneIfc > >  XNamedVec;
private:

    class XNamedEnumerationHelper final : public EnumerationHelper_BASE
    {
        XNamedVec mXNamedVec;
        typename XNamedVec::iterator mIt;
    public:
            XNamedEnumerationHelper( XNamedVec sMap ) : mXNamedVec(std::move( sMap )), mIt( mXNamedVec.begin() ) {}

            virtual sal_Bool SAL_CALL hasMoreElements(  ) override
            {
            return ( mIt != mXNamedVec.end() );
            }

            virtual css::uno::Any SAL_CALL nextElement(  ) override
            {
                if ( hasMoreElements() )
                    return css::uno::Any( *mIt++ );
                throw css::container::NoSuchElementException();
            }
    };

    XNamedVec mXNamedVec;
    typename XNamedVec::iterator cachePos;
public:
    XNamedObjectCollectionHelper( XNamedVec sMap ) : mXNamedVec(std::move( sMap )), cachePos(mXNamedVec.begin()) {}
    // XElementAccess
    virtual css::uno::Type SAL_CALL getElementType(  ) override { return cppu::UnoType< OneIfc >::get(); }
    virtual sal_Bool SAL_CALL hasElements(  ) override { return ( mXNamedVec.size() > 0 ); }
    // XNameAccess
    virtual css::uno::Any SAL_CALL getByName( const OUString& aName ) override
    {
        if ( !hasByName(aName) )
            throw css::container::NoSuchElementException();
        return css::uno::Any( *cachePos );
    }
    virtual css::uno::Sequence< OUString > SAL_CALL getElementNames(  ) override
    {
        css::uno::Sequence< OUString > sNames( mXNamedVec.size() );
        OUString* pString = sNames.getArray();
        typename XNamedVec::iterator it = mXNamedVec.begin();
        typename XNamedVec::iterator it_end = mXNamedVec.end();

        for ( ; it != it_end; ++it, ++pString )
        {
            css::uno::Reference< css::container::XNamed > xName( *it, css::uno::UNO_QUERY_THROW );
            *pString = xName->getName();
        }
        return sNames;
    }
    virtual sal_Bool SAL_CALL hasByName( const OUString& aName ) override
    {
        cachePos = mXNamedVec.begin();
        typename XNamedVec::iterator it_end = mXNamedVec.end();
        for ( ; cachePos != it_end; ++cachePos )
        {
            css::uno::Reference< css::container::XNamed > xName( *cachePos, css::uno::UNO_QUERY_THROW );
            if ( aName == xName->getName() )
                break;
        }
        return ( cachePos != it_end );
    }

    // XElementAccess
    virtual ::sal_Int32 SAL_CALL getCount(  ) override { return mXNamedVec.size(); }
    virtual css::uno::Any SAL_CALL getByIndex( ::sal_Int32 Index ) override
    {
        if ( Index < 0 || Index >= getCount() )
            throw css::lang::IndexOutOfBoundsException();

        return css::uno::Any( mXNamedVec[ Index ] );

    }
    // XEnumerationAccess
    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL createEnumeration(  ) override
    {
        return new XNamedEnumerationHelper( mXNamedVec );
    }
};

// including a HelperInterface implementation
template< typename... Ifc >
class SAL_DLLPUBLIC_RTTI ScVbaCollectionBase : public InheritedHelperInterfaceImpl< Ifc... >
{
typedef InheritedHelperInterfaceImpl< Ifc... > BaseColBase;
protected:
    css::uno::Reference< css::container::XIndexAccess > m_xIndexAccess;
    css::uno::Reference< css::container::XNameAccess > m_xNameAccess;
    bool mbIgnoreCase;

    /// @throws css::uno::RuntimeException
    virtual css::uno::Any getItemByStringIndex( const OUString& sIndex )
    {
        if ( !m_xNameAccess.is() )
            throw css::uno::RuntimeException(u"ScVbaCollectionBase string index access not supported by this object"_ustr );

        if( mbIgnoreCase )
        {
            const css::uno::Sequence< OUString > sElementNames = m_xNameAccess->getElementNames();
            for( const OUString& rName : sElementNames )
            {
                if( rName.equalsIgnoreAsciiCase( sIndex ) )
                {
                    return createCollectionObject( m_xNameAccess->getByName( rName ) );
                }
            }
        }
        return createCollectionObject( m_xNameAccess->getByName( sIndex ) );
    }

    /// @throws css::uno::RuntimeException
    /// @throws css::lang::IndexOutOfBoundsException
    virtual css::uno::Any getItemByIntIndex( const sal_Int32 nIndex )
    {
        if ( !m_xIndexAccess.is() )
            throw css::uno::RuntimeException(u"ScVbaCollectionBase numeric index access not supported by this object"_ustr );
        if ( nIndex <= 0 )
        {
            throw css::lang::IndexOutOfBoundsException(
                u"index is 0 or negative"_ustr );
        }
        // need to adjust for vba index ( for which first element is 1 )
        return createCollectionObject( m_xIndexAccess->getByIndex( nIndex - 1 ) );
    }

    void UpdateCollectionIndex( const css::uno::Reference< css::container::XIndexAccess >& xIndexAccess )
    {
        m_xNameAccess.set(xIndexAccess, css::uno::UNO_QUERY_THROW);
        m_xIndexAccess = xIndexAccess;
    }

public:
    ScVbaCollectionBase( const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext, css::uno::Reference< css::container::XIndexAccess > xIndexAccess, bool bIgnoreCase = false ) : BaseColBase( xParent, xContext ), m_xIndexAccess(std::move( xIndexAccess )), mbIgnoreCase( bIgnoreCase ) { m_xNameAccess.set(m_xIndexAccess, css::uno::UNO_QUERY); }

    //XCollection
    virtual ::sal_Int32 SAL_CALL getCount() override
    {
        return m_xIndexAccess->getCount();
    }

    virtual css::uno::Any SAL_CALL Item(const css::uno::Any& Index1, const css::uno::Any& /*not processed in this base class*/) override
    {
        OUString aStringSheet;
        if (Index1.getValueTypeClass() == css::uno::TypeClass_DOUBLE)
        {
            // This is needed for ContentControls, where the unique integer ID
            // can be passed as float to simulate a "by name" lookup.
            double fIndex = 0;
            Index1 >>= fIndex;
            aStringSheet = OUString::number(fIndex);
        }
        else if (Index1.getValueTypeClass() != css::uno::TypeClass_STRING)
        {
            sal_Int32 nIndex = 0;
            if ( !( Index1 >>= nIndex ) )
            {
                throw  css::lang::IndexOutOfBoundsException( u"Couldn't convert index to Int32"_ustr );
            }

            return  getItemByIntIndex( nIndex );
        }
        else
            Index1 >>= aStringSheet;

        return getItemByStringIndex( aStringSheet );
    }

    // XDefaultMethod
    OUString SAL_CALL getDefaultMethodName(  ) override
    {
        return u"Item"_ustr;
    }
    // XEnumerationAccess
    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL createEnumeration() override = 0;

    // XElementAccess
    virtual css::uno::Type SAL_CALL getElementType() override = 0;
    // XElementAccess
    virtual sal_Bool SAL_CALL hasElements() override
    {
        return ( m_xIndexAccess->getCount() > 0 );
    }
    virtual css::uno::Any createCollectionObject( const css::uno::Any& aSource ) = 0;

};

typedef ScVbaCollectionBase< ::cppu::WeakImplHelper<ov::XCollection> > CollImplBase;
// compatible with the old collections ( pre XHelperInterface base class ) ( some internal objects still use this )
class VBAHELPER_DLLPUBLIC ScVbaCollectionBaseImpl : public CollImplBase
{
public:
    /// @throws css::uno::RuntimeException
    ScVbaCollectionBaseImpl( const css::uno::Reference< ov::XHelperInterface > & xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext, const css::uno::Reference< css::container::XIndexAccess >& xIndexAccess ) : CollImplBase( xParent, xContext, xIndexAccess){}

};

template < typename... Ifc > // where Ifc must implement XCollectionTest
class SAL_DLLPUBLIC_RTTI CollTestImplHelper :  public ScVbaCollectionBase< ::cppu::WeakImplHelper< Ifc... > >
{
typedef ScVbaCollectionBase< ::cppu::WeakImplHelper< Ifc... >  > ImplBase;

public:
    /// @throws css::uno::RuntimeException
    CollTestImplHelper( const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext,  const css::uno::Reference< css::container::XIndexAccess >& xIndexAccess, bool bIgnoreCase = false ) : ImplBase( xParent, xContext, xIndexAccess, bIgnoreCase ) {}
};


#endif //SC_VBA_COLLECTION_IMPL_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
