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


#include <memory>
#include <string_view>

#include <dp_misc.h>
#include <dp_backend.h>
#include <dp_ucb.h>
#include <dp_interact.h>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <osl/file.hxx>
#include <ucbhelper/content.hxx>
#include <svl/inettype.hxx>
#include "dp_executablebackenddb.hxx"
#include <cppuhelper/supportsservice.hxx>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::ucb;
using namespace dp_misc;

namespace dp_registry::backend::executable {
namespace {

class BackendImpl : public ::dp_registry::backend::PackageRegistryBackend
{
    class ExecutablePackageImpl : public ::dp_registry::backend::Package
    {
        BackendImpl * getMyBackend() const;

        // Package
        virtual beans::Optional< beans::Ambiguous<sal_Bool> > isRegistered_(
            ::osl::ResettableMutexGuard & guard,
            ::rtl::Reference<dp_misc::AbortChannel> const & abortChannel,
            Reference<XCommandEnvironment> const & xCmdEnv ) override;
        virtual void processPackage_(
            ::osl::ResettableMutexGuard & guard,
            bool registerPackage,
            bool startup,
            ::rtl::Reference<dp_misc::AbortChannel> const & abortChannel,
            Reference<XCommandEnvironment> const & xCmdEnv ) override;

        bool getFileAttributes(sal_uInt64& out_Attributes);
        bool isUrlTargetInExtension() const;

    public:
        ExecutablePackageImpl(
            ::rtl::Reference<PackageRegistryBackend> const & myBackend,
            OUString const & url, OUString const & name,
            Reference<deployment::XPackageTypeInfo> const & xPackageType,
            bool bRemoved, OUString const & identifier)
            : Package( myBackend, url, name, name /* display-name */,
                       xPackageType, bRemoved, identifier)
            {}
    };
    friend class ExecutablePackageImpl;

    // PackageRegistryBackend
    virtual Reference<deployment::XPackage> bindPackage_(
        OUString const & url, OUString const & mediaType, bool bRemoved,
        OUString const & identifier, Reference<XCommandEnvironment> const & xCmdEnv ) override;

    void addDataToDb(OUString const & url);
    bool hasActiveEntry(std::u16string_view url);
    void revokeEntryFromDb(std::u16string_view url);

    Reference<deployment::XPackageTypeInfo> m_xExecutableTypeInfo;
    std::unique_ptr<ExecutableBackendDb> m_backendDb;
public:
    BackendImpl( Sequence<Any> const & args,
                 Reference<XComponentContext> const & xComponentContext );

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XPackageRegistry
    virtual Sequence< Reference<deployment::XPackageTypeInfo> > SAL_CALL
    getSupportedPackageTypes() override;
    virtual void SAL_CALL packageRemoved(OUString const & url, OUString const & mediaType) override;

};


BackendImpl::BackendImpl(
    Sequence<Any> const & args,
    Reference<XComponentContext> const & xComponentContext )
    : PackageRegistryBackend( args, xComponentContext ),
      m_xExecutableTypeInfo(new Package::TypeInfo(
                                u"application/vnd.sun.star.executable"_ustr,
                                u""_ustr, u"Executable"_ustr ) )
{
    if (!transientMode())
    {
        OUString dbFile = makeURL(getCachePath(), u"backenddb.xml"_ustr);
        m_backendDb.reset(
            new ExecutableBackendDb(getComponentContext(), dbFile));
    }
}

// XServiceInfo
OUString BackendImpl::getImplementationName()
{
    return u"com.sun.star.comp.deployment.executable.PackageRegistryBackend"_ustr;
}

sal_Bool BackendImpl::supportsService( const OUString& ServiceName )
{
    return cppu::supportsService(this, ServiceName);
}

css::uno::Sequence< OUString > BackendImpl::getSupportedServiceNames()
{
    return { BACKEND_SERVICE_NAME };
}

void BackendImpl::addDataToDb(OUString const & url)
{
    if (m_backendDb)
        m_backendDb->addEntry(url);
}

void BackendImpl::revokeEntryFromDb(std::u16string_view url)
{
    if (m_backendDb)
        m_backendDb->revokeEntry(url);
}

bool BackendImpl::hasActiveEntry(std::u16string_view url)
{
    if (m_backendDb)
        return m_backendDb->hasActiveEntry(url);
    return false;
}


// XPackageRegistry
Sequence< Reference<deployment::XPackageTypeInfo> >
BackendImpl::getSupportedPackageTypes()
{
    return Sequence<Reference<deployment::XPackageTypeInfo> >(
        & m_xExecutableTypeInfo, 1);
}

void BackendImpl::packageRemoved(OUString const & url, OUString const & /*mediaType*/)
{
    if (m_backendDb)
        m_backendDb->removeEntry(url);
}

// PackageRegistryBackend
Reference<deployment::XPackage> BackendImpl::bindPackage_(
    OUString const & url, OUString const & mediaType, bool bRemoved,
    OUString const & identifier, Reference<XCommandEnvironment> const & xCmdEnv )
{
    if (mediaType.isEmpty())
    {
        throw lang::IllegalArgumentException(
            StrCannotDetectMediaType() + url,
            static_cast<OWeakObject *>(this), static_cast<sal_Int16>(-1) );
    }

    OUString type, subType;
    INetContentTypeParameterList params;
    if (INetContentTypes::parse( mediaType, type, subType, &params ))
    {
        if (type.equalsIgnoreAsciiCase("application"))
        {
            OUString name;
            if (!bRemoved)
            {
                ::ucbhelper::Content ucbContent(
                    url, xCmdEnv, getComponentContext() );
                name = StrTitle::getTitle( ucbContent );
            }
            if (subType.equalsIgnoreAsciiCase("vnd.sun.star.executable"))
            {
                return new BackendImpl::ExecutablePackageImpl(
                    this, url, name,  m_xExecutableTypeInfo, bRemoved,
                    identifier);
            }
        }
    }
    return Reference<deployment::XPackage>();
}


// Package
BackendImpl * BackendImpl::ExecutablePackageImpl::getMyBackend() const
{
    BackendImpl * pBackend = static_cast<BackendImpl *>(m_myBackend.get());
    if (nullptr == pBackend)
    {
        //May throw a DisposedException
        check();
        //We should never get here...
        throw RuntimeException( u"Failed to get the BackendImpl"_ustr,
            static_cast<OWeakObject*>(const_cast<ExecutablePackageImpl *>(this)));
    }
    return pBackend;
}

beans::Optional< beans::Ambiguous<sal_Bool> >
BackendImpl::ExecutablePackageImpl::isRegistered_(
    ::osl::ResettableMutexGuard &,
    ::rtl::Reference<dp_misc::AbortChannel> const &,
    Reference<XCommandEnvironment> const & )
{
    bool registered = getMyBackend()->hasActiveEntry(getURL());
    return beans::Optional< beans::Ambiguous<sal_Bool> >(
            true /* IsPresent */,
                beans::Ambiguous<sal_Bool>(
                    registered, false /* IsAmbiguous */ ) );
}

void BackendImpl::ExecutablePackageImpl::processPackage_(
    ::osl::ResettableMutexGuard &,
    bool doRegisterPackage,
    bool /*startup*/,
    ::rtl::Reference<dp_misc::AbortChannel> const & abortChannel,
    Reference<XCommandEnvironment> const & /*xCmdEnv*/ )
{
    checkAborted(abortChannel);
    if (doRegisterPackage)
    {
        if (!isUrlTargetInExtension())
        {
            OSL_ASSERT(false);
            return;
        }
        sal_uInt64 attributes = 0;
        //Setting the executable attribute does not affect executables on Windows
        if (getFileAttributes(attributes))
        {
            if(getMyBackend()->m_context == "user")
                attributes |= osl_File_Attribute_OwnExe;
            else if (getMyBackend()->m_context == "shared")
                attributes |= (osl_File_Attribute_OwnExe | osl_File_Attribute_GrpExe
                               | osl_File_Attribute_OthExe);
            else if (getMyBackend()->m_context != "bundled")
                //Bundled extensions are required to be in the properly
                //installed. That is an executable must have the right flags
                OSL_ASSERT(false);

            //This won't have effect on Windows
            osl::File::setAttributes(
                    dp_misc::expandUnoRcUrl(m_url), attributes);
        }
        getMyBackend()->addDataToDb(getURL());
    }
    else
    {
        getMyBackend()->revokeEntryFromDb(getURL());
    }
}

//We currently cannot check if this XPackage represents a content of a particular extension
//But we can check if we are within $UNO_USER_PACKAGES_CACHE etc.
//Done for security reasons. For example an extension manifest could contain a path to
//an executable outside the extension.
bool BackendImpl::ExecutablePackageImpl::isUrlTargetInExtension() const
{
    bool bSuccess = false;
    OUString sExtensionDir;
    if(getMyBackend()->m_context == "user")
        sExtensionDir = dp_misc::expandUnoRcTerm(u"$UNO_USER_PACKAGES_CACHE"_ustr);
    else if (getMyBackend()->m_context == "shared")
        sExtensionDir = dp_misc::expandUnoRcTerm(u"$UNO_SHARED_PACKAGES_CACHE"_ustr);
    else if (getMyBackend()->m_context == "bundled")
        sExtensionDir = dp_misc::expandUnoRcTerm(u"$BUNDLED_EXTENSIONS"_ustr);
    else
        OSL_ASSERT(false);
    //remove file ellipses
    if (osl::File::E_None == osl::File::getAbsoluteFileURL(OUString(), sExtensionDir, sExtensionDir))
    {
        OUString sFile;
        if (osl::File::E_None == osl::File::getAbsoluteFileURL(
            OUString(), dp_misc::expandUnoRcUrl(m_url), sFile))
        {
            if (sFile.match(sExtensionDir))
                bSuccess = true;
        }
    }
    return bSuccess;
}

bool BackendImpl::ExecutablePackageImpl::getFileAttributes(sal_uInt64& out_Attributes)
{
    bool bSuccess = false;
    const OUString url(dp_misc::expandUnoRcUrl(m_url));
    osl::DirectoryItem item;
    if (osl::FileBase::E_None == osl::DirectoryItem::get(url, item))
    {
        osl::FileStatus aStatus(osl_FileStatus_Mask_Attributes);
        if( osl::FileBase::E_None == item.getFileStatus(aStatus))
        {
            out_Attributes = aStatus.getAttributes();
            bSuccess = true;
        }
    }
    return bSuccess;
}


} // anon namespace


} // namespace dp_registry::backend::executable

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_deployment_executable_PackageRegistryBackend_get_implementation(
    css::uno::XComponentContext* context, css::uno::Sequence<css::uno::Any> const& args)
{
    return cppu::acquire(new dp_registry::backend::executable::BackendImpl(args, context));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
