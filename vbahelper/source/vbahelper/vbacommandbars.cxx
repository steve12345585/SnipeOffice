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

#include <com/sun/star/container/XNameAccess.hpp>
#include <ooo/vba/office/MsoBarType.hpp>
#include <cppuhelper/implbase.hxx>
#include <utility>

#include "vbacommandbars.hxx"
#include "vbacommandbar.hxx"

using namespace com::sun::star;
using namespace ooo::vba;

namespace {

class CommandBarEnumeration : public ::cppu::WeakImplHelper< container::XEnumeration >
{
    uno::Reference< XHelperInterface > m_xParent;
    uno::Reference< uno::XComponentContext > m_xContext;
    VbaCommandBarHelperRef m_pCBarHelper;
    uno::Sequence< OUString > m_sNames;
    sal_Int32 m_nCurrentPosition;
public:
    /// @throws uno::RuntimeException
    CommandBarEnumeration( uno::Reference< XHelperInterface > xParent, uno::Reference< uno::XComponentContext > xContext, VbaCommandBarHelperRef  pHelper) : m_xParent(std::move( xParent )), m_xContext(std::move( xContext )), m_pCBarHelper(std::move( pHelper )) , m_nCurrentPosition( 0 )
    {
        uno::Reference< container::XNameAccess > xNameAccess = m_pCBarHelper->getPersistentWindowState();
        m_sNames = xNameAccess->getElementNames();
    }
    virtual sal_Bool SAL_CALL hasMoreElements() override
    {
        return m_nCurrentPosition < m_sNames.getLength();
    }
    virtual uno::Any SAL_CALL nextElement() override
    {
        // FIXME: should be add menubar
        if( !hasMoreElements() )
            throw container::NoSuchElementException();

        OUString sResourceUrl( m_sNames[ m_nCurrentPosition++ ] );
        if( sResourceUrl.indexOf( "private:resource/toolbar/" ) != -1 )
        {
            uno::Reference< container::XIndexAccess > xCBarSetting = m_pCBarHelper->getSettings( sResourceUrl );
            uno::Reference< XCommandBar > xCommandBar( new ScVbaCommandBar( m_xParent, m_xContext, m_pCBarHelper, xCBarSetting, sResourceUrl, false ) );
            // Strange, shouldn't the Enumeration support match/share the
            // iteration code? ( e.g. ScVbaCommandBars::Item(...) )
            // and we at least should return here ( something ) it seems
            return uno::Any( xCommandBar );
        }
        else
            return nextElement();
    }
};

}

ScVbaCommandBars::ScVbaCommandBars( const uno::Reference< XHelperInterface >& xParent, const uno::Reference< uno::XComponentContext >& xContext, const uno::Reference< container::XIndexAccess >& xIndexAccess, const uno::Reference< frame::XModel >& xModel ) : CommandBars_BASE( xParent, xContext, xIndexAccess )
{
    m_pCBarHelper = std::make_shared<VbaCommandBarHelper>( mxContext, xModel );
    m_xNameAccess = m_pCBarHelper->getPersistentWindowState();
}

ScVbaCommandBars::~ScVbaCommandBars()
{
}

// XEnumerationAccess
uno::Type SAL_CALL
ScVbaCommandBars::getElementType()
{
    return cppu::UnoType<XCommandBar>::get();
}

uno::Reference< container::XEnumeration >
ScVbaCommandBars::createEnumeration()
{
    return uno::Reference< container::XEnumeration >( new CommandBarEnumeration( this, mxContext, m_pCBarHelper ) );
}

uno::Any
ScVbaCommandBars::createCollectionObject( const uno::Any& aSource )
{
    // aSource should be a name at this time, because of the class is API wrapper.
    OUString sResourceUrl;
    uno::Reference< container::XIndexAccess > xBarSettings;
    OUString sBarName;
    bool bMenu = false;
    uno::Any aRet;

    if( aSource >>= sBarName )
    {
        // some built-in command bars
        if( m_pCBarHelper->getModuleId() == "com.sun.star.sheet.SpreadsheetDocument" )
        {
            if( sBarName.equalsIgnoreAsciiCase( "Worksheet Menu Bar" ) )
            {
                // spreadsheet menu bar
                sResourceUrl = ITEM_MENUBAR_URL;
                bMenu = true;
            }
            else if( sBarName.equalsIgnoreAsciiCase( "Cell" ) )
            {
                // EVIL HACK (tm): spreadsheet cell context menu as dummy object without functionality
                aRet <<= uno::Reference< XCommandBar >( new VbaDummyCommandBar( this, mxContext, sBarName ) );
            }
        }
        else if( m_pCBarHelper->getModuleId() == "com.sun.star.text.TextDocument" )
        {
            if( sBarName.equalsIgnoreAsciiCase( "Menu Bar" ) )
            {
                // text processor menu bar
                sResourceUrl = ITEM_MENUBAR_URL;
                bMenu = true;
            }
        }

        // nothing found - try to resolve from name
        if( !aRet.hasValue() && sResourceUrl.isEmpty() )
        {
            sResourceUrl = m_pCBarHelper->findToolbarByName( m_xNameAccess, sBarName );
            bMenu = false;
        }
    }

    if( !sResourceUrl.isEmpty() )
    {
        xBarSettings = m_pCBarHelper->getSettings( sResourceUrl );
        aRet <<= uno::Reference< XCommandBar >( new ScVbaCommandBar( this, mxContext, m_pCBarHelper, xBarSettings, sResourceUrl, bMenu ) );
    }

    if( !aRet.hasValue() )
        throw uno::RuntimeException( u"Toolbar do not exist"_ustr );

    return aRet;
}

// XCommandBars
uno::Reference< XCommandBar > SAL_CALL
ScVbaCommandBars::Add( const css::uno::Any& Name, const css::uno::Any& /*Position*/, const css::uno::Any& /*MenuBar*/, const css::uno::Any& /*Temporary*/ )
{
    // FIXME: only support to add Toolbar
    // Position - MsoBar MenuBar - sal_Bool
    // Currently only the Name is supported.
    OUString sName;
    if( Name.hasValue() )
        Name >>= sName;

    OUString sResourceUrl;
    if( !sName.isEmpty() )
    {
        sResourceUrl = m_pCBarHelper->findToolbarByName( m_xNameAccess, sName );
        if( !sResourceUrl.isEmpty() )
            throw uno::RuntimeException( u"Toolbar exists"_ustr );
    }
    else
    {
        sName = "Custom1";
    }

    sResourceUrl = VbaCommandBarHelper::generateCustomURL();
    uno::Reference< container::XIndexAccess > xBarSettings( m_pCBarHelper->getSettings( sResourceUrl ), uno::UNO_SET_THROW );
    uno::Reference< XCommandBar > xCBar( new ScVbaCommandBar( this, mxContext, m_pCBarHelper, xBarSettings, sResourceUrl, false ) );
    xCBar->setName( sName );
    return xCBar;
}
sal_Int32 SAL_CALL
ScVbaCommandBars::getCount()
{
    // Filter out all toolbars from the window collection
    sal_Int32 nCount = 1; // there is a Menubar in OOo
    const uno::Sequence< ::OUString > allNames = m_xNameAccess->getElementNames();
    nCount += std::count_if(allNames.begin(), allNames.end(),
        [](const OUString& rName) { return rName.indexOf( "private:resource/toolbar/" ) != -1; });
    return nCount;
}

// ScVbaCollectionBaseImpl
uno::Any SAL_CALL
ScVbaCommandBars::Item( const uno::Any& aIndex, const uno::Any& /*aIndex2*/ )
{
    if( aIndex.getValueTypeClass() == uno::TypeClass_STRING )
    {
        return createCollectionObject( aIndex );
    }

    // hardcode if "aIndex = 1" that would return "main menu".
    sal_Int32 nIndex = 0;
    aIndex >>= nIndex;
    if( nIndex == 1 )
    {
        uno::Any aSource;
        if( m_pCBarHelper->getModuleId() == "com.sun.star.sheet.SpreadsheetDocument" )
            aSource <<= u"Worksheet Menu Bar"_ustr;
        else if( m_pCBarHelper->getModuleId() == "com.sun.star.text.TextDocument" )
            aSource <<= u"Menu Bar"_ustr;
        if( aSource.hasValue() )
            return createCollectionObject( aSource );
    }
    return uno::Any();
}

// XHelperInterface
OUString
ScVbaCommandBars::getServiceImplName()
{
    return u"ScVbaCommandBars"_ustr;
}

uno::Sequence<OUString>
ScVbaCommandBars::getServiceNames()
{
    static uno::Sequence< OUString > const aServiceNames
    {
        u"ooo.vba.CommandBars"_ustr
    };
    return aServiceNames;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
