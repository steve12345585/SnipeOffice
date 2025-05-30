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

#include "ucpext_provider.hxx"
#include "ucpext_content.hxx"

#include <com/sun/star/ucb/IllegalIdentifierException.hpp>
#include <cppuhelper/weak.hxx>
#include <ucbhelper/contentidentifier.hxx>
#include <osl/mutex.hxx>
#include <rtl/ustrbuf.hxx>


namespace ucb::ucp::ext
{


    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::uno::XInterface;
    using ::com::sun::star::uno::Sequence;
    using ::com::sun::star::ucb::XContentIdentifier;
    using ::com::sun::star::ucb::IllegalIdentifierException;
    using ::com::sun::star::ucb::XContent;
    using ::com::sun::star::uno::XComponentContext;


    //= ContentProvider


    ContentProvider::ContentProvider( const Reference< XComponentContext >& rxContext )
        :ContentProvider_Base( rxContext )
    {
    }


    ContentProvider::~ContentProvider()
    {
    }


    OUString SAL_CALL ContentProvider::getImplementationName()
    {
        return u"org.openoffice.comp.ucp.ext.ContentProvider"_ustr;
    }


    Sequence< OUString > SAL_CALL ContentProvider::getSupportedServiceNames(  )
    {
        return { u"com.sun.star.ucb.ContentProvider"_ustr, u"com.sun.star.ucb.ExtensionContentProvider"_ustr };
    }


    OUString ContentProvider::getRootURL()
    {
        return u"vnd.sun.star.extension://"_ustr;
    }


    OUString ContentProvider::getArtificialNodeContentType()
    {
        return u"application/vnd.sun.star.extension-content"_ustr;
    }


    namespace
    {
        void lcl_ensureAndTransfer( std::u16string_view& io_rIdentifierFragment, OUStringBuffer& o_rNormalization, const sal_Unicode i_nLeadingChar )
        {
            if ( ( io_rIdentifierFragment.empty() ) || ( io_rIdentifierFragment[0] != i_nLeadingChar ) )
                throw IllegalIdentifierException();
            io_rIdentifierFragment = io_rIdentifierFragment.substr( 1 );
            o_rNormalization.append( i_nLeadingChar );
        }
    }


    Reference< XContent > SAL_CALL ContentProvider::queryContent( const Reference< XContentIdentifier  >& i_rIdentifier )
    {
        // Check URL scheme...
        static constexpr OUString sScheme( u"vnd.sun.star.extension"_ustr );
        if ( !i_rIdentifier->getContentProviderScheme().equalsIgnoreAsciiCase( sScheme ) )
            throw IllegalIdentifierException();

        // normalize the identifier
        const OUString sIdentifier( i_rIdentifier->getContentIdentifier() );

        // the scheme needs to be lower-case
        OUStringBuffer aComposer( sIdentifier.copy( 0, sScheme.getLength() ).toAsciiLowerCase() );

        // one : is required after the scheme
        std::u16string_view sRemaining( sIdentifier.subView( sScheme.getLength() ) );
        lcl_ensureAndTransfer( sRemaining, aComposer, ':' );

        // and at least one /
        lcl_ensureAndTransfer( sRemaining, aComposer, '/' );

        // the normalized form requires one additional /, but we also accept identifiers which don't have it
        if ( sRemaining.empty() )
        {
            // the root content is a special case, it requires /
            aComposer.append( "//" );
        }
        else
        {
            if ( sRemaining[0] != '/' )
            {
                aComposer.append( OUString::Concat("/") + sRemaining );
            }
            else
            {
                lcl_ensureAndTransfer( sRemaining, aComposer, '/' );
                // by now, we moved "vnd.sun.star.extension://" from the URL to aComposer
                if ( sRemaining.empty() )
                {
                    // again, it's the root content, but one / is missing
                    aComposer.append( '/' );
                }
                else
                {
                    aComposer.append( sRemaining );
                }
            }
        }
        const Reference< XContentIdentifier > xNormalizedIdentifier( new ::ucbhelper::ContentIdentifier( aComposer.makeStringAndClear() ) );

        ::osl::MutexGuard aGuard( m_aMutex );

        // check if a content with given id already exists...
        Reference< XContent > xContent( queryExistingContent( xNormalizedIdentifier ) );
        if ( xContent.is() )
            return xContent;

        // create a new content
        xContent = new Content( m_xContext, this, xNormalizedIdentifier );
        if ( !xContent->getIdentifier().is() )
            throw IllegalIdentifierException();

        registerNewContent( xContent );
        return xContent;
    }


}   // namespace ucb::ucp::ext


extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
ucb_ext_ContentProvider_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const&)
{
    return cppu::acquire(new ucb::ucp::ext::ContentProvider(context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
