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

#include <utility>
#include <vbahelper/vbadocumentbase.hxx>

#include <com/sun/star/beans/PropertyVetoException.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/WrappedTargetRuntimeException.hpp>
#include <com/sun/star/util/XModifiable.hpp>
#include <com/sun/star/util/XProtectable.hpp>
#include <com/sun/star/util/XCloseable.hpp>
#include <com/sun/star/util/URLTransformer.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XStorable.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XTitle.hpp>
#include <ooo/vba/XApplicationBase.hpp>

#include <comphelper/automationinvokedzone.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <tools/urlobj.hxx>
#include <osl/file.hxx>
#include <sal/log.hxx>

using namespace ::com::sun::star;
using namespace ::ooo::vba;

VbaDocumentBase::VbaDocumentBase( const uno::Reference< ov::XHelperInterface >& xParent,
                                  const uno::Reference< uno::XComponentContext >& xContext )
: VbaDocumentBase_BASE( xParent, xContext )
{
}

VbaDocumentBase::VbaDocumentBase( uno::Sequence< uno::Any> const & args,
                                  uno::Reference< uno::XComponentContext> const & xContext )
: VbaDocumentBase_BASE( getXSomethingFromArgs< XHelperInterface >( args, 0 ), xContext )
{
}

OUString
VbaDocumentBase::getName()
{
    OUString sName = VbaDocumentBase::getNameFromModel( getModel() );
    SAL_INFO("vbahelper", "VbaDocumentBase::getName: '" << sName << "'");

    return sName;
}

OUString VbaDocumentBase::getNameFromModel( const uno::Reference< frame::XModel >& xModel )
{
    OUString sName = xModel.is() ? xModel->getURL() : OUString();
    if ( !sName.isEmpty() )
    {

        INetURLObject aURL( xModel->getURL() );
        ::osl::File::getSystemPathFromFileURL( aURL.GetLastName(), sName );
    }
    else
    {
        uno::Reference< frame::XTitle > xTitle( xModel, uno::UNO_QUERY_THROW );
        sName = xTitle->getTitle();
        sName = sName.trim();
    }
    return sName;
}
OUString
VbaDocumentBase::getPath()
{
    INetURLObject aURL( getModel()->getURL() );
    OUString sURL = aURL.GetMainURL( INetURLObject::DecodeMechanism::ToIUri );
    OUString sPath;
    if( !sURL.isEmpty() )
    {
       sURL = sURL.copy( 0, sURL.getLength() - aURL.GetLastName().getLength() - 1 );
       ::osl::File::getSystemPathFromFileURL( sURL, sPath );
    }
    SAL_INFO("vbahelper", "VbaDocumentBase::getPath: '" << sPath << "'");

    return sPath;
}

OUString
VbaDocumentBase::getFullName()
{
    // In the Automation case, follow the specs.
    if (comphelper::Automation::AutomationInvokedZone::isActive())
    {
        // We know that Automation is relevant only on Windows, so hardcode "\\".
        OUString sPath = getPath() + "\\" + getName();
        SAL_INFO("vbahelper", "VbaDocumentBase::getFullName: '" << sPath << "'");
        return sPath;
    }

    OUString sPath = getName();
    //::osl::File::getSystemPathFromFileURL( getModel()->getURL(), sPath );
    SAL_INFO("vbahelper", "VbaDocumentBase::getFullName: '" << sPath << "'");
    return sPath;
}

void
VbaDocumentBase::Close( const uno::Any &rSaveArg, const uno::Any &rFileArg,
                      const uno::Any &rRouteArg )
{
    bool bSaveChanges = false;
    OUString aFileName;
    bool bRouteWorkbook = true;

    rSaveArg >>= bSaveChanges;
    bool bFileName =  ( rFileArg >>= aFileName );
    rRouteArg >>= bRouteWorkbook;
    uno::Reference< frame::XStorable > xStorable( getModel(), uno::UNO_QUERY_THROW );
    uno::Reference< util::XModifiable > xModifiable( getModel(), uno::UNO_QUERY_THROW );

    if( bSaveChanges )
    {
        if( xStorable->isReadonly() )
        {
            throw uno::RuntimeException(u"Unable to save to a read only file "_ustr );
        }
        if( bFileName )
            xStorable->storeAsURL( aFileName, uno::Sequence< beans::PropertyValue >(0) );
        else
            xStorable->store();
    }
    else
        xModifiable->setModified( false );

    // first try to close the document using UI dispatch functionality
    try
    {
        uno::Reference< frame::XController > xController( getModel()->getCurrentController() );
        if (xController)
        {
            uno::Reference< frame::XDispatchProvider > xDispatchProvider( xController->getFrame(), uno::UNO_QUERY );
            uno::Reference< lang::XMultiComponentFactory > xServiceManager( mxContext->getServiceManager() );
            if (xDispatchProvider && xServiceManager)
            {
                uno::Reference< util::XURLTransformer > xURLTransformer( util::URLTransformer::create(mxContext) );

                util::URL aURL;
                aURL.Complete = ".uno:CloseDoc";
                xURLTransformer->parseStrict( aURL );

                uno::Reference< css::frame::XDispatch > xDispatch(
                        xDispatchProvider->queryDispatch( aURL, u"_self"_ustr , 0 ) );
                if (xDispatch)
                {
                    xDispatch->dispatch( aURL, uno::Sequence< beans::PropertyValue >() );
                    return;
                }
            }
        }
    }
    catch(const uno::Exception&)
    {
    }

    // if it is not possible to use UI dispatch, try to close the model directly
    bool bCloseable = false;
    uno::Reference< frame::XModel > xModel = getModel();
    try
    {
        uno::Reference< util::XCloseable > xCloseable( xModel, uno::UNO_QUERY );

        // use close(boolean DeliverOwnership)
        // The boolean parameter DeliverOwnership tells objects vetoing the close
        // process that they may assume ownership if they object the closure by
        // throwing a CloseVetoException. Here we give up ownership. To be on the
        // safe side, catch possible veto exception anyway.
        if ( xCloseable.is() )
        {
            bCloseable = true;
            xCloseable->close(true);
        }
    }
    catch (const uno::Exception &)
    {
        // vetoed
    }
    if (bCloseable)
        return;

    try {
        // If close is not supported by this model - try to dispose it.
        // But if the model disagree with a reset request for the modify state
        // we shouldn't do so. Otherwise some strange things can happen.
        if (xModel)
            xModel->dispose();
    }
    catch(const uno::Exception&)
    {
    }
}

void
VbaDocumentBase::Protect( const uno::Any &aPassword )
{
    OUString rPassword;
    uno::Reference< util::XProtectable > xProt( getModel(), uno::UNO_QUERY_THROW );
    SAL_INFO("vbahelper", "Workbook::Protect stub");
    if(  aPassword >>= rPassword )
        xProt->protect( rPassword );
    else
        xProt->protect( OUString() );
}

void
VbaDocumentBase::Unprotect( const uno::Any &aPassword )
{
    OUString rPassword;
    uno::Reference< util::XProtectable > xProt( getModel(), uno::UNO_QUERY_THROW );
    if( !xProt->isProtected() )
        throw uno::RuntimeException(u"File is already unprotected"_ustr );
    if( aPassword >>= rPassword )
        xProt->unprotect( rPassword );
    else
        xProt->unprotect( OUString() );
}

void
VbaDocumentBase::setSaved( sal_Bool bSave )
{
    uno::Reference< util::XModifiable > xModifiable( getModel(), uno::UNO_QUERY_THROW );
    try
    {
        xModifiable->setModified( !bSave );
    }
    catch (const lang::DisposedException&)
    {
        // impossibility to set the modified state on disposed document should not trigger an error
    }
    catch (const beans::PropertyVetoException&)
    {
        uno::Any aCaught( ::cppu::getCaughtException() );
        throw lang::WrappedTargetRuntimeException(
                u"Can't change modified state of model!"_ustr,
                uno::Reference< uno::XInterface >(),
                aCaught );
    }
}

sal_Bool
VbaDocumentBase::getSaved()
{
    uno::Reference< util::XModifiable > xModifiable( getModel(), uno::UNO_QUERY_THROW );
    return !xModifiable->isModified();
}

void
VbaDocumentBase::Save()
{
    uno::Reference< frame::XModel > xModel = getModel();
    dispatchRequests(xModel,u".uno:Save"_ustr);
}

void
VbaDocumentBase::Activate()
{
    uno::Reference< frame::XFrame > xFrame( getModel()->getCurrentController()->getFrame(), uno::UNO_SET_THROW );
    xFrame->activate();
}

uno::Any SAL_CALL
VbaDocumentBase::getVBProject()
{
    if( !mxVBProject.is() ) try
    {
        uno::Reference< XApplicationBase > xApp( Application(), uno::UNO_QUERY );
        if (xApp)
        {
            uno::Reference< XInterface > xVBE( xApp->getVBE(), uno::UNO_QUERY );
            if (xVBE)
            {
                uno::Sequence< uno::Any > aArgs{ uno::Any(xVBE), // the VBE
                                                 uno::Any(getModel()) }; // document model for script container access
                uno::Reference< lang::XMultiComponentFactory > xServiceManager( mxContext->getServiceManager() );
                if (xServiceManager)
                    mxVBProject = xServiceManager->createInstanceWithArgumentsAndContext(
                        u"ooo.vba.vbide.VBProject"_ustr, aArgs, mxContext );
            }
        }
    }
    catch(const uno::Exception&)
    {
    }
    return uno::Any( mxVBProject );
}

OUString
VbaDocumentBase::getServiceImplName()
{
    return u"VbaDocumentBase"_ustr;
}

uno::Sequence< OUString >
VbaDocumentBase::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.VbaDocumentBase"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
