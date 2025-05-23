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

#include "basecontainer.hxx"
#include <com/sun/star/document/XTypeDetection.hpp>
#include <com/sun/star/frame/XTerminateListener.hpp>
#include <unotools/mediadescriptor.hxx>
#include <cppuhelper/compbase.hxx>
#include <cppuhelper/implbase.hxx>
#include <comphelper/compbase.hxx>

namespace filter::config {

class TerminateDetection;

/** @short      implements the service <type scope="com.sun.star.document">TypeDetection</type>.
 */
class TypeDetection : public ::cppu::ImplInheritanceHelper< BaseContainer                 ,
                                                             css::document::XTypeDetection >
{

// native interface

    css::uno::Reference< css::uno::XComponentContext > m_xContext;
    rtl::Reference<TerminateDetection> m_xTerminateListener;
    bool m_bCancel;

public:


    // ctor/dtor

    /** @short  standard ctor to connect this interface wrapper to
                the global filter cache instance ...

        @param  rxContext
                reference to the uno service manager, which created this service instance.
     */
    explicit TypeDetection(const css::uno::Reference< css::uno::XComponentContext >& rxContext);

    void cancel()
    {
        m_bCancel = true;
    }


    /** @short  standard dtor.
     */
    virtual ~TypeDetection() override;


// private helper

private:

    static bool impl_getPreselectionForType(
        std::unique_lock<std::mutex>& rGuard,
        const OUString& sPreSelType, const css::util::URL& aParsedURL, FlatDetection& rFlatTypes, bool bDocService);

    static void impl_getPreselectionForDocumentService(
        std::unique_lock<std::mutex>& rGuard,
        const OUString& sPreSelDocumentService, const css::util::URL& aParsedURL, FlatDetection& rFlatTypes);

    static OUString impl_getTypeFromFilter(std::unique_lock<std::mutex>& rGuard, const OUString& rFilterName);

    /**
     * Get all format types that we handle.
     */
    static void impl_getAllFormatTypes(
        std::unique_lock<std::mutex>& rGuard,
        const css::util::URL& aParsedURL, utl::MediaDescriptor const & rDescriptor,
        FlatDetection& rFlatTypes);


    /** @short      make a combined flat/deep type detection

        @descr      It steps over all flat detected types (given by the parameter lFlatTypes),
                    try it and search for most suitable one.
                    The specified MediaDescriptor will be patched, so it contain
                    the right values every time. Using of any deep detection service
                    can be enabled/disabled. And last but not least: If the results
                    won't be really clear (because a flat detected type has no deep
                    detection service), a "suggested" type name will be returned as "rLastChance".
                    It can be used after e.g. all well known deep detection services
                    was used without getting any result. Then this "last-chance-type"
                    should be returned. Of course using of it can fail too ... but it's a try :-)

        @param      rDescriptor
                    provides any easy-to-use stl interface to the MediaDescriptor.
                    Note : Its content will be adapted to returned result of this method.
                    Means: The type/filter entries of it will be actualized or removed from it.

        @param      lFlatTypes
                    a list of all flat detected types, which should be checked here.
                    No other types are allowed here!

        @param      rLastChance
                    the internal name of a "suggested type" ... (see before)
                    Note: it will be reset to an empty string every time. So
                    a set value of "rLastChance" can be detected outside very easy.

        @param      bAllowDeep
                    enable/disable using of a might existing deep detection service.

        @return     The internal name of a detected type.
                    An empty value if detection failed. ... but see rLastChance
                    for additional returns!
     */
    OUString impl_detectTypeFlatAndDeep(      utl::MediaDescriptor& rDescriptor   ,
                                               const FlatDetection&                 lFlatTypes    ,
                                                     bool                       bAllowDeep    ,
                                                     OUString&               rLastChance   );


    /** @short      seek a might existing stream to position 0.

        @descr      This is an optional action to be more robust
                    in case any detect service doesn't make this seek...
                    Normally it's part of any called detect service or filter...
                    but sometimes it's not done there.

        @param      rDescriptor
                    a stl representation of the MediaDescriptor as in/out parameter.
     */
    static void impl_seekStreamToZero(utl::MediaDescriptor const & rDescriptor);


    /** @short      make deep type detection for a specified
                    detect service (threadsafe!).

        @descr      It creates the right uno service, prepare the
                    needed MediaDescriptor, call the right interfaces,
                    and return the results.

        @attention  The results (means type and corresponding filter)
                    are already part of the in/out parameter pDescriptor.
                    (in case they were valid).

        @param      sDetectService
                    uno service name of the detect service.

        @param      rDescriptor
                    a stl representation of the MediaDescriptor as in/out parameter.
     */
    OUString impl_askDetectService(const OUString&               sDetectService,
                                                utl::MediaDescriptor& rDescriptor   );


    /** @short      try to find an interaction handler and
                    ask him to select a possible filter for
                    this unknown format.

        @descr      If the user select a filter, it will be used as return value
                    without further checking against the given file content!

        @param      rDescriptor
                    a stl representation of the MediaDescriptor as in/out parameter.

        @return     [string]
                    a valid type name or an empty string if user canceled interaction.
     */
    static OUString impl_askUserForTypeAndFilterIfAllowed(utl::MediaDescriptor& rDescriptor);


    /** @short      check if an input stream is already part of the
                    given MediaDescriptor and creates a new one if necessary.

        @attention  This method does further something special!
                    <ul>
                        <li>
                            If the given URL seem to be a streamable content, but creation of the stream
                            failed (might by an IOException), this method throws an exception.
                            (May be an existing interaction handler must be called here too ...)
                            The whole detection must be interrupted then and the interface method queryTypeByDescriptor()
                            must return an empty type name value.

                            That prevent us against multiple handling of the same error more than ones
                            (e.g. if we ask all detect services as fallback ...).
                        </li>
                        <li>
                            In case the stream already exists inside the descriptor this method does nothing.
                        </li>
                        <li>
                            In case the stream does not exists but can be created successfully, the stream will
                            be added to the descriptor.
                        </li>
                    </ul>

        @param      rDescriptor
                    provides any easy-to-use stl interface to the MediaDescriptor.
                    Note : Its content will be adapted to returned result of this method.
                    Means: The stream will be added to it.

        @throw      Any suitable exception if stream should be opened but operation was not successful.
                    Note: If an interactionHandler is part of the given descriptor too, it was already used.
                    Means: let the exception pass through the top most interface method!
     */
    void impl_openStream(utl::MediaDescriptor& rDescriptor);


    /** @short      validate the specified type and its relationships
                    and set all needed information related to this type
                    in the specified descriptor.

        @descr      Related information are: - corresponding filter
                                             - media type
                                             - ...

        @param      rDescriptor
                    provides access to the outside MediaDescriptor.

        @param      sType
                    the name of the type, which should be set on the descriptor.
                    Can be empty to remove any related value from the descriptor!

        @return     TRUE the specified type and its registrations was valid(!) and
                    could be set on the descriptor.
     */
    static bool impl_validateAndSetTypeOnDescriptor( utl::MediaDescriptor& rDescriptor,
                                                     const OUString&               sType      );


    /** @short      validate the specified filter and its relationships
                    and set all needed information related to this filter
                    in the specified descriptor.

        @descr      Related information are: - corresponding type
                                             - ...

        @param      rDescriptor
                    provides access to the outside MediaDescriptor.

        @param      sFilter
                    the name of the filter, which should be set on the descriptor.
                    Can be empty to remove any related value from the descriptor!

        @return     TRUE the specified type and its registrations was valid(!) and
                    could be set on the descriptor.
     */
    static bool impl_validateAndSetFilterOnDescriptor(
                                                utl::MediaDescriptor& rDescriptor,
                                                   const OUString&               sFilter    );


    /** @short      remove anything related to a TYPE/FILTER entry from the
                    specified MediaDescriptor.

        @descr      This method works together with impl_validateAndSetTypeOnDescriptor()/
                    impl_validateAndSetFilterOnDescriptor(). All information, which can be
                    set by these two operations must be "removable" by this method.

        @param      rDescriptor
                    reference to the MediaDescriptor (represented by an easy-to-use
                    stl interface!), which should be patched.
     */
    static void impl_removeTypeFilterFromDescriptor(utl::MediaDescriptor& rDescriptor);


    /** @short      search the best suitable filter for the given type
                    and add it into the media descriptor.

        @descr      Normally this is a type detection only ...
                    but for some special features we must overwrite our detection
                    because a file must be loaded into a special (means preselected)
                    application.

                    E.g. CSV/TXT format are sometimes ugly to handle .-)

                    Note: If the descriptor already include a filter
                    (may be selected by a FilterSelect interaction or preselected
                    by the user itself) ... we don't change that here !

        @param      rDescriptor
                    reference to the MediaDescriptor (represented by an easy-to-use
                    stl interface!), which should be patched.

        @param      sType
                    the internal type name, where we search a filter for.
                    Used as IN/OUT parameter so we can overrule the detection result for
                    types too !

        @note       #i60158#
                    sometimes our text ascii and our csv filter can't work together.
                    Then we overwrite our detection hardly.
                    sType param is used as out parameter then too ... and
                    rDescriptor will be changed by selecting another filter.
                    (see code)
     */
    void impl_checkResultsAndAddBestFilter(utl::MediaDescriptor& rDescriptor,
                                           OUString&               sType      );


// uno interface

public:


    // XTypeDetection

    virtual OUString SAL_CALL queryTypeByURL(const OUString& sURL) override;

    virtual OUString SAL_CALL queryTypeByDescriptor(css::uno::Sequence< css::beans::PropertyValue >& lDescriptor,
                                                           sal_Bool                                         bAllowDeep ) override;

};

class TerminateDetection : public comphelper::WeakComponentImplHelper<css::frame::XTerminateListener>
{
private:
    TypeDetection* m_pTypeDetection;

public:

    using comphelper::WeakComponentImplHelperBase::disposing;
    virtual void SAL_CALL disposing(const css::lang::EventObject&) override
    {
    }

    // XTerminateListener
    virtual void SAL_CALL queryTermination(const css::lang::EventObject&) override
    {
        m_pTypeDetection->cancel();
    }

    virtual void SAL_CALL notifyTermination(const css::lang::EventObject&) override
    {
    }

    TerminateDetection(TypeDetection* pTypeDetection)
        : m_pTypeDetection(pTypeDetection)
    {
    }
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
