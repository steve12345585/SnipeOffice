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

#ifndef INCLUDED_UNOTOOLS_MEDIADESCRIPTOR_HXX
#define INCLUDED_UNOTOOLS_MEDIADESCRIPTOR_HXX

#include <sal/config.h>

#include <vector>

#include <comphelper/docpasswordrequest.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <rtl/ustring.hxx>
#include <unotools/unotoolsdllapi.h>

namespace com::sun::star::io {
    class XInputStream;
}
namespace comphelper { class IDocPasswordVerifier; }

namespace utl {

/** @short  can be used to work with a css::document::MediaDescriptor
            struct.

    @descr  It wraps an unordered_map around the Sequence< css::beans::PropertyValue >, which
            represent the MediaDescriptor item.
            Further this helper defines often used functions (as e.g. open of the required streams,
            consistent checks etcpp.) and it defines all usable property names.

    @attention  This class isn't threadsafe and must be guarded from outside!
 */
class UNOTOOLS_DLLPUBLIC MediaDescriptor : public comphelper::SequenceAsHashMap
{
    public:

        /** @short  these methods can be used to get the different property names
                    as static const OUString values.

            @descr  Because definition and declaration of static const class members
                    does not work as expected under windows (under unix it works as well)
                    these way must be used :-(
          */
        static constexpr OUString PROP_ABORTED = u"Aborted"_ustr;
        static constexpr OUString PROP_ASTEMPLATE = u"AsTemplate"_ustr;
        static constexpr OUString PROP_COMPONENTDATA = u"ComponentData"_ustr;
        static constexpr OUString PROP_DOCUMENTSERVICE = u"DocumentService"_ustr;
        static constexpr OUString PROP_ENCRYPTIONDATA = u"EncryptionData"_ustr;
        static constexpr OUString PROP_FILENAME = u"FileName"_ustr;
        static constexpr OUString PROP_FILTERNAME = u"FilterName"_ustr;
        static constexpr OUString PROP_FILTERPROVIDER = u"FilterProvider"_ustr;
        static constexpr OUString PROP_FILTEROPTIONS = u"FilterOptions"_ustr;
        static constexpr OUString PROP_FRAME = u"Frame"_ustr;
        static constexpr OUString PROP_FRAMENAME = u"FrameName"_ustr;
        static constexpr OUString PROP_HIDDEN = u"Hidden"_ustr;
        static constexpr OUString PROP_INPUTSTREAM = u"InputStream"_ustr;
        static constexpr OUString PROP_INTERACTIONHANDLER = u"InteractionHandler"_ustr;
        static constexpr OUString PROP_AUTHENTICATIONHANDLER = u"AuthenticationHandler"_ustr;
        static constexpr OUString PROP_JUMPMARK = u"JumpMark"_ustr;
        static constexpr OUString PROP_MACROEXECUTIONMODE = u"MacroExecutionMode"_ustr;
        static constexpr OUString PROP_MEDIATYPE = u"MediaType"_ustr;
        static constexpr OUString PROP_MINIMIZED = u"Minimized"_ustr;
        static constexpr OUString PROP_NOAUTOSAVE = u"NoAutoSave"_ustr;
        static constexpr OUString PROP_OPENNEWVIEW = u"OpenNewView"_ustr;
        static constexpr OUString PROP_OUTPUTSTREAM = u"OutputStream"_ustr;
        static constexpr OUString PROP_PASSWORD = u"Password"_ustr;
        static constexpr OUString PROP_POSTDATA = u"PostData"_ustr;
        static constexpr OUString PROP_PREVIEW = u"Preview"_ustr;
        static constexpr OUString PROP_READONLY = u"ReadOnly"_ustr;
        static constexpr OUString PROP_REFERRER = u"Referer"_ustr;
        static constexpr OUString PROP_REPLACEABLE = u"Replaceable"_ustr;
        static constexpr OUString PROP_SALVAGEDFILE = u"SalvagedFile"_ustr;
        static constexpr OUString PROP_SILENT = u"Silent"_ustr;
        static constexpr OUString PROP_STATUSINDICATOR = u"StatusIndicator"_ustr;
        static constexpr OUString PROP_STREAM = u"Stream"_ustr;
        static constexpr OUString PROP_STREAMFOROUTPUT = u"StreamForOutput"_ustr;
        static constexpr OUString PROP_TEMPLATENAME = u"TemplateName"_ustr;
        static constexpr OUString PROP_TITLE = u"Title"_ustr;
        static constexpr OUString PROP_TYPENAME = u"TypeName"_ustr;
        static constexpr OUString PROP_UCBCONTENT = u"UCBContent"_ustr;
        static constexpr OUString PROP_UPDATEDOCMODE = u"UpdateDocMode"_ustr;
        static constexpr OUString PROP_URL = u"URL"_ustr;
        static constexpr OUString PROP_VERSION = u"Version"_ustr;
        static constexpr OUString PROP_DOCUMENTTITLE = u"DocumentTitle"_ustr;
        static constexpr OUString PROP_MODEL = u"Model"_ustr;
        static constexpr OUString PROP_VIEWONLY = u"ViewOnly"_ustr;
        static constexpr OUString PROP_DOCUMENTBASEURL = u"DocumentBaseURL"_ustr;
        static constexpr OUString PROP_SUGGESTEDSAVEASNAME = u"SuggestedSaveAsName"_ustr;
        static constexpr OUString PROP_EXPORTDIRECTORY = u"ExportDirectory"_ustr;
        static constexpr OUString PROP_AUTOSAVEEVENT = u"AutoSaveEvent"_ustr;

    // interface
    public:

        /** @short  these ctors do nothing - excepting that they forward
                    the given parameters to the base class ctors.

            @descr  The ctors must be overwritten to resolve conflicts with
                    the default ctors of the compiler :-(.
         */
        MediaDescriptor();
        MediaDescriptor(const css::uno::Sequence< css::beans::PropertyValue >& lSource);

        /** @short  it checks if the descriptor already has a valid
                    InputStream item and creates a new one, if not.

            @descr  This method uses the current items of this MediaDescriptor,
                    to open the stream (as e.g. URL, ReadOnly, PostData etcpp.).
                    It creates a seekable stream and put it into the descriptor.

                    A might existing InteractionHandler will be used automatically,
                    to solve problems!

                    In case of local file the system file locking is used.

            @return TRUE, if the stream was already part of the descriptor or could
                    be created as new item. FALSE otherwise.
         */
        bool addInputStream();

        /** @short  it checks if the descriptor already has a valid
                    InputStream item and creates a new one, if not.

            @descr  This method uses the current items of this MediaDescriptor,
                    to open the stream (as e.g. URL, ReadOnly, PostData etcpp.).
                    It creates a seekable stream and put it into the descriptor.

                    A might existing InteractionHandler will be used automatically,
                    to solve problems!

                    In case of local file the system file locking is used based on
                    configuration settings.

            @return TRUE, if the stream was already part of the descriptor or could
                    be created as new item. FALSE otherwise.
         */
        bool addInputStreamOwnLock();

        /** @short  it checks if the descriptor describes a readonly stream.

            @descr  The descriptor itself isn't changed doing so.
                    It's only checked if the stream seems to be based
                    of a real readonly file.

            @Attention
                    We don't check the property "ReadOnly" here. Because
                    this property can be set from outside and overwrites
                    the readonly state of  the stream.
                    If e.g. the stream could be opened read/write ...
                    but "ReadOnly" property is set to TRUE, this means:
                    show a readonly UI on top of this read/write stream.

            @return TRUE, if the stream must be interpreted as readonly ...
                    FALSE otherwise.
         */
        bool isStreamReadOnly() const;

        /** Returns a value from the sequence contained in the property
            'ComponentData' of this media descriptor.

            @descr  The property 'ComponentData' should be empty, or should
                contain a value of type sequence<com.sun.star.beans.NamedValue>
                or sequence<com.sun.star.beans.PropertyValue>.

            @return  The value with the specified name, if existing in the
                sequence of the 'ComponentData' property, otherwise an empty
                Any.
         */
        css::uno::Any getComponentDataEntry(
            const OUString& rName ) const;

        /** Inserts a value into the sequence contained in the property
            'ComponentData' of the media descriptor.

            @descr  The property 'ComponentData' should be empty, or should
                contain a value of type sequence<com.sun.star.beans.NamedValue>
                or sequence<com.sun.star.beans.PropertyValue>. The passed value
                will be inserted into the sequence, or, if already existing,
                will be overwritten.

            @param rName  The name of the value to be inserted into the
                sequence of the 'ComponentData' property.

            @param rValue  The value to be inserted into the sequence of the
                'ComponentData' property.
         */
        void setComponentDataEntry(
            const OUString& rName,
            const css::uno::Any& rValue );

        /** Removes a value from the sequence contained in the property
            'ComponentData' of the media descriptor.

            @descr  The property 'ComponentData' should be empty, or should
                contain a value of type sequence<com.sun.star.beans.NamedValue>
                or sequence<com.sun.star.beans.PropertyValue>. The value with
                the passed name will be removed from the sequence, if existing.

            @param rName  The name of the value to be removed from the sequence
                of the 'ComponentData' property.
         */
        void clearComponentDataEntry(
            const OUString& rName );

        /** This helper function tries to find a password for the document
            described by this media descriptor.

            First, the list of default passwords will be tried if provided. This
            is needed by import filters for external file formats that have to
            check a predefined password in some cases without asking the user
            for a password. Every password is checked using the passed password
            verifier.

            If not successful, this media descriptor is asked for a password,
            that has been set e.g. by an API call to load a document. If
            existing, the password is checked using the passed password
            verifier.

            If still not successful, the interaction handler contained in this
            media descriptor is used to request a password from the user. This
            will be repeated until the passed password verifier validates the
            entered password, or if the user chooses to cancel password input.

            If a valid password (that is not contained in the passed list of
            default passwords) was found, it will be inserted into the
            "Password" property of this descriptor.

            @param rVerifier
            The password verifier used to check every processed password.

            @param eRequestType
            The password request type that will be passed to the
            DocPasswordRequest object created internally. See
            docpasswordrequest.hxx for more details.

            @param pDefaultPasswords
            If not null, contains default passwords that will be tried before a
            password will be requested from the media descriptor or the user.

            @return
            If not empty, contains the password that has been validated by the
            passed password verifier. If empty, no valid password has been
            found, or the user has chosen to cancel password input.
        */
        css::uno::Sequence< css::beans::NamedValue > requestAndVerifyDocPassword(
            comphelper::IDocPasswordVerifier& rVerifier,
            comphelper::DocPasswordRequestType eRequestType,
            const ::std::vector< OUString >* pDefaultPasswords );

    // helper
    private:

        /** @short  tries to open a stream by using the given PostData stream.

            @descr  The stream is used directly ...

                    The MediaDescriptor itself is changed inside this method.
                    Means: the stream is added internal and not returned by a value.

            @param  _rxPostData
                    the PostData stream.

            @return TRUE if the stream could be added successfully.
                    Note: If FALSE is returned, the error was already handled inside!

            @throw  [css::uno::RuntimeException]
                    if the MediaDescriptor seems to be invalid!

            @throw  [css::lang::IllegalArgumentException]
                    if the given PostData stream is <NULL/>.
         */
        SAL_DLLPRIVATE bool impl_openStreamWithPostData(
            const css::uno::Reference< css::io::XInputStream >& _rxPostData
            );

        /** @short  tries to open a stream by using the given URL.

            @descr  First it tries to open the content in r/w mode (if its
                    allowed to do so). Only in case it's not allowed or it failed
                    the stream will be tried to open in readonly mode.

                    The MediaDescriptor itself is changed inside this method.
                    Means: the stream is added internal and not returned by a value.

            @param  sURL
                    the URL for open.

            @param  bLockFile
                    specifies whether the file should be locked

            @return TRUE if the stream could be added successfully.
                    Note: If FALSE is returned, the error was already handled inside!

            @throw  [css::uno::RuntimeException]
                    if the MediaDescriptor seems to be invalid!
         */
        SAL_DLLPRIVATE bool impl_openStreamWithURL(
            const OUString& sURL,
            bool bLockFile
            );

        /** @short  it checks if the descriptor already has a valid
                    InputStream item and creates a new one, if not.

            @descr  This method uses the current items of this MediaDescriptor,
                    to open the stream (as e.g. URL, ReadOnly, PostData etcpp.).
                    It creates a seekable stream and put it into the descriptor.

                    A might existing InteractionHandler will be used automatically,
                    to solve problems!

            @param  bLockFile
                    specifies whether the file should be locked

            @return TRUE, if the stream was already part of the descriptor or could
                    be created as new item. FALSE otherwise.
         */
        SAL_DLLPRIVATE bool impl_addInputStream( bool bLockFile );
};

}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
