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

#include <com/sun/star/awt/XTextArea.hpp>
#include <com/sun/star/awt/XVclWindowPeer.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/awt/VisualEffect.hpp>
#include <com/sun/star/awt/LineEndFormat.hpp>
#include <com/sun/star/graphic/GraphicProvider.hpp>
#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <com/sun/star/util/Date.hpp>
#include <com/sun/star/awt/ImageScaleMode.hpp>

#include <o3tl/safeint.hxx>
#include <controls/formattedcontrol.hxx>
#include <toolkit/controls/unocontrols.hxx>
#include <tools/urlobj.hxx>
#include <helper/property.hxx>
#include <toolkit/helper/macros.hxx>
#include <unotools/securityoptions.hxx>

// for introspection
#include <awt/vclxwindows.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>

#include <algorithm>

#include <helper/imagealign.hxx>
#include <helper/unopropertyarrayhelper.hxx>
#include <utility>

using namespace css;
using namespace css::awt;
using namespace css::lang;
using namespace css::uno;
using ::com::sun::star::graphic::XGraphic;
using ::com::sun::star::uno::Reference;
using namespace ::toolkit;

uno::Reference< graphic::XGraphic >
ImageHelper::getGraphicAndGraphicObjectFromURL_nothrow( uno::Reference< graphic::XGraphicObject >& xOutGraphicObj, const OUString& _rURL )
{
    xOutGraphicObj = nullptr;
    return ImageHelper::getGraphicFromURL_nothrow( _rURL, u""_ustr );
}

css::uno::Reference< css::graphic::XGraphic >
ImageHelper::getGraphicFromURL_nothrow( const OUString& _rURL, OUString const & referer )
{
    uno::Reference< graphic::XGraphic > xGraphic;
    if (_rURL.isEmpty() || SvtSecurityOptions::isUntrustedReferer(referer) || INetURLObject(_rURL).IsExoticProtocol())
        return xGraphic;

    try
    {
        const uno::Reference< uno::XComponentContext >& xContext( ::comphelper::getProcessComponentContext() );
        uno::Reference< graphic::XGraphicProvider > xProvider( graphic::GraphicProvider::create(xContext) );
        xGraphic = xProvider->queryGraphic({ comphelper::makePropertyValue(u"URL"_ustr, _rURL) });
    }
    catch (const Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("toolkit.controls");
    }

    return xGraphic;
}


UnoControlEditModel::UnoControlEditModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXEdit>();
}

OUString UnoControlEditModel::getServiceName( )
{
    return u"stardiv.vcl.controlmodel.Edit"_ustr;
}

uno::Any UnoControlEditModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    uno::Any aReturn;

    switch ( nPropId )
    {
    case BASEPROPERTY_LINE_END_FORMAT:
        aReturn <<= sal_Int16(awt::LineEndFormat::LINE_FEED);   // LF
        break;
    case BASEPROPERTY_DEFAULTCONTROL:
        aReturn <<= u"stardiv.vcl.control.Edit"_ustr;
        break;
    default:
        aReturn = UnoControlModel::ImplGetDefaultValue( nPropId );
        break;
    }
    return aReturn;
}

::cppu::IPropertyArrayHelper& UnoControlEditModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlEditModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlEditModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlEditModel"_ustr;
}

css::uno::Sequence<OUString> UnoControlEditModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlEditModel"_ustr, u"stardiv.vcl.controlmodel.Edit"_ustr };
    return comphelper::concatSequences(UnoControlModel::getSupportedServiceNames(), vals);
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlEditModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlEditModel(context));
}



UnoEditControl::UnoEditControl()
    :maTextListeners( *this )
    ,mnMaxTextLen( 0 )
    ,mbSetTextInPeer( false )
    ,mbSetMaxTextLenInPeer( false )
    ,mbHasTextProperty( false )
{
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 12;
}

uno::Any SAL_CALL UnoEditControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aReturn = UnoControlBase::queryAggregation( rType );
    if ( !aReturn.hasValue() )
        aReturn = UnoEditControl_Base::queryInterface( rType );
    return aReturn;
}

uno::Any SAL_CALL UnoEditControl::queryInterface( const uno::Type & rType )
{
    return UnoControlBase::queryInterface( rType );
}

void SAL_CALL UnoEditControl::acquire(  ) noexcept
{
    UnoControlBase::acquire();
}

void SAL_CALL UnoEditControl::release(  ) noexcept
{
    UnoControlBase::release();
}

IMPLEMENT_FORWARD_XTYPEPROVIDER2( UnoEditControl, UnoControlBase, UnoEditControl_Base )

OUString UnoEditControl::GetComponentServiceName() const
{
    // by default, we want a simple edit field
    OUString sName( u"Edit"_ustr );

    // but maybe we are to display multi-line text?
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_MULTILINE ) );
    bool b = bool();
    if ( ( aVal >>= b ) && b )
        sName = "MultiLineEdit";

    return sName;
}

sal_Bool SAL_CALL UnoEditControl::setModel(const uno::Reference< awt::XControlModel >& _rModel)
{
    bool bReturn = UnoControlBase::setModel( _rModel );
    mbHasTextProperty = ImplHasProperty( BASEPROPERTY_TEXT );
    return bReturn;
}

void UnoEditControl::ImplSetPeerProperty( const OUString& rPropName, const uno::Any& rVal )
{
    bool bDone = false;
    if ( GetPropertyId( rPropName ) == BASEPROPERTY_TEXT )
    {
        // #96986# use setText(), or text listener will not be called.
        uno::Reference < awt::XTextComponent > xTextComponent( getPeer(), uno::UNO_QUERY );
        if ( xTextComponent.is() )
        {
            OUString sText;
            rVal >>= sText;
            ImplCheckLocalize( sText );
            xTextComponent->setText( sText );
            bDone = true;
        }
    }

    if ( !bDone )
        UnoControlBase::ImplSetPeerProperty( rPropName, rVal );
}

void UnoEditControl::dispose()
{
    lang::EventObject aEvt( *this );
    maTextListeners.disposeAndClear( aEvt );
    UnoControl::dispose();
}

void UnoEditControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoControl::createPeer( rxToolkit, rParentPeer );

    uno::Reference< awt::XTextComponent > xText( getPeer(), uno::UNO_QUERY );
    if ( xText.is() )
    {
        xText->addTextListener( this );

        if ( mbSetMaxTextLenInPeer )
            xText->setMaxTextLen( mnMaxTextLen );
        if ( mbSetTextInPeer )
            xText->setText( maText );
    }
}

void UnoEditControl::textChanged(const awt::TextEvent& e)
{
    uno::Reference< awt::XTextComponent > xText( getPeer(), uno::UNO_QUERY );

    if ( mbHasTextProperty )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_TEXT ), uno::Any(xText->getText()), false );
    }
    else
    {
        maText = xText->getText();
    }

    if ( maTextListeners.getLength() )
        maTextListeners.textChanged( e );
}

void UnoEditControl::addTextListener(const uno::Reference< awt::XTextListener > & l)
{
    // tdf#150974 some extensions pass null
    if (!l)
    {
        SAL_WARN("toolkit", "null XTextListener");
        return;
    }
    maTextListeners.addInterface( l );
}

void UnoEditControl::removeTextListener(const uno::Reference< awt::XTextListener > & l)
{
    // tdf#150974 some extensions pass null
    if (!l)
    {
        SAL_WARN("toolkit", "null XTextListener");
        return;
    }
    maTextListeners.removeInterface( l );
}

void UnoEditControl::setText( const OUString& aText )
{
    if ( mbHasTextProperty )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_TEXT ), uno::Any(aText), true );
    }
    else
    {
        maText = aText;
        mbSetTextInPeer = true;
        uno::Reference < awt::XTextComponent > xText( getPeer(), uno::UNO_QUERY );
        if ( xText.is() )
            xText->setText( maText );
        }

    // Setting the property to the VCLXWindow doesn't call textChanged
    if ( maTextListeners.getLength() )
    {
        awt::TextEvent aEvent;
        aEvent.Source = *this;
        maTextListeners.textChanged( aEvent );
    }
}

namespace
{
    void lcl_normalize( awt::Selection& _rSel )
    {
        if ( _rSel.Min > _rSel.Max )
            ::std::swap( _rSel.Min, _rSel.Max );
    }
}

void UnoEditControl::insertText( const awt::Selection& rSel, const OUString& rNewText )
{
    // normalize the selection - OUString::replaceAt has a strange behaviour if the min is greater than the max
    awt::Selection aSelection( rSel );
    lcl_normalize( aSelection );

    OUString aOldText = getText();
    if (aSelection.Min < 0 || aOldText.getLength() < aSelection.Max)
    {
        throw lang::IllegalArgumentException();
    }

    // preserve the selection resp. cursor position
    awt::Selection aNewSelection( getSelection() );
#ifdef ALSO_PRESERVE_COMPLETE_SELECTION
        // (not sure - looks uglier ...)
    sal_Int32 nDeletedCharacters = ( aSelection.Max - aSelection.Min ) - rNewText.getLength();
    if ( aNewSelection.Min > aSelection.Min )
        aNewSelection.Min -= nDeletedCharacters;
    if ( aNewSelection.Max > aSelection.Max )
        aNewSelection.Max -= nDeletedCharacters;
#else
    aNewSelection.Max = ::std::min( aNewSelection.Min, aNewSelection.Max ) + rNewText.getLength();
    aNewSelection.Min = aNewSelection.Max;
#endif

    OUString  aNewText = aOldText.replaceAt( aSelection.Min, aSelection.Max - aSelection.Min, rNewText );
    setText( aNewText );

    setSelection( aNewSelection );
}

OUString UnoEditControl::getText()
{
    OUString aText = maText;

    if ( mbHasTextProperty )
        aText = ImplGetPropertyValue_UString( BASEPROPERTY_TEXT );
    else
    {
        uno::Reference< awt::XTextComponent > xText( getPeer(), uno::UNO_QUERY );
        if ( xText.is() )
            aText = xText->getText();
    }

    return aText;
}

OUString UnoEditControl::getSelectedText()
{
    OUString sSelected;
    uno::Reference< awt::XTextComponent > xText( getPeer(), uno::UNO_QUERY );
    if ( xText.is() )
        sSelected = xText->getSelectedText();

    return sSelected;
}

void UnoEditControl::setSelection( const awt::Selection& aSelection )
{
    uno::Reference< awt::XTextComponent > xText( getPeer(), uno::UNO_QUERY );
    if ( xText.is() )
        xText->setSelection( aSelection );
}

awt::Selection UnoEditControl::getSelection()
{
    awt::Selection aSel;
    uno::Reference< awt::XTextComponent > xText( getPeer(), uno::UNO_QUERY );
    if ( xText.is() )
        aSel = xText->getSelection();
    return aSel;
}

sal_Bool UnoEditControl::isEditable()
{
    return !ImplGetPropertyValue_BOOL( BASEPROPERTY_READONLY );
}

void UnoEditControl::setEditable( sal_Bool bEditable )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_READONLY ), uno::Any(!bEditable), true );
}

sal_Int16 UnoEditControl::getMaxTextLen()
{
    sal_Int16 nMaxLen = mnMaxTextLen;

    if ( ImplHasProperty( BASEPROPERTY_MAXTEXTLEN ) )
        nMaxLen = ImplGetPropertyValue_INT16( BASEPROPERTY_MAXTEXTLEN );

    return nMaxLen;
}

void UnoEditControl::setMaxTextLen( sal_Int16 nLen )
{
    if ( ImplHasProperty( BASEPROPERTY_MAXTEXTLEN) )
    {
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_MAXTEXTLEN ), uno::Any(nLen), true );
    }
    else
    {
        mnMaxTextLen = nLen;
        mbSetMaxTextLenInPeer = true;
        uno::Reference < awt::XTextComponent > xText( getPeer(), uno::UNO_QUERY );
        if ( xText.is() )
            xText->setMaxTextLen( mnMaxTextLen );
    }
}

awt::Size UnoEditControl::getMinimumSize(  )
{
    return Impl_getMinimumSize();
}

awt::Size UnoEditControl::getPreferredSize(  )
{
    return Impl_getPreferredSize();
}

awt::Size UnoEditControl::calcAdjustedSize( const awt::Size& rNewSize )
{
    return Impl_calcAdjustedSize( rNewSize );
}

awt::Size UnoEditControl::getMinimumSize( sal_Int16 nCols, sal_Int16 nLines )
{
    return Impl_getMinimumSize( nCols, nLines );
}

void UnoEditControl::getColumnsAndLines( sal_Int16& nCols, sal_Int16& nLines )
{
    Impl_getColumnsAndLines( nCols, nLines );
}

OUString UnoEditControl::getImplementationName(  )
{
    return u"stardiv.Toolkit.UnoEditControl"_ustr;
}

uno::Sequence< OUString > UnoEditControl::getSupportedServiceNames()
{
    css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlEdit"_ustr, u"stardiv.vcl.control.Edit"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames( ), vals);
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoEditControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoEditControl());
}



UnoControlFileControlModel::UnoControlFileControlModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    ImplRegisterProperty( BASEPROPERTY_ALIGN );
    ImplRegisterProperty( BASEPROPERTY_BACKGROUNDCOLOR );
    ImplRegisterProperty( BASEPROPERTY_BORDER );
    ImplRegisterProperty( BASEPROPERTY_BORDERCOLOR );
    ImplRegisterProperty( BASEPROPERTY_DEFAULTCONTROL );
    ImplRegisterProperty( BASEPROPERTY_ENABLED );
    ImplRegisterProperty( BASEPROPERTY_ENABLEVISIBLE );
    ImplRegisterProperty( BASEPROPERTY_FONTDESCRIPTOR );
    ImplRegisterProperty( BASEPROPERTY_HELPTEXT );
    ImplRegisterProperty( BASEPROPERTY_HELPURL );
    ImplRegisterProperty( BASEPROPERTY_PRINTABLE );
    ImplRegisterProperty( BASEPROPERTY_READONLY );
    ImplRegisterProperty( BASEPROPERTY_TABSTOP );
    ImplRegisterProperty( BASEPROPERTY_TEXT );
    ImplRegisterProperty( BASEPROPERTY_VERTICALALIGN );
    ImplRegisterProperty( BASEPROPERTY_WRITING_MODE );
    ImplRegisterProperty( BASEPROPERTY_CONTEXT_WRITING_MODE );
    ImplRegisterProperty( BASEPROPERTY_HIDEINACTIVESELECTION );
}

OUString UnoControlFileControlModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.FileControl"_ustr;
}

uno::Any UnoControlFileControlModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.FileControl"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlFileControlModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlFileControlModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlFileControlModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlFileControlModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlFileControlModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlFileControlModel"_ustr, u"stardiv.vcl.controlmodel.FileControl"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals);
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlFileControlModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlFileControlModel(context));
}



UnoFileControl::UnoFileControl()
{
}

OUString UnoFileControl::GetComponentServiceName() const
{
    return u"filecontrol"_ustr;
}

OUString UnoFileControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoFileControl"_ustr;
}

css::uno::Sequence<OUString> UnoFileControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlFileControl"_ustr, u"stardiv.vcl.control.FileControl"_ustr };
    return comphelper::concatSequences( UnoEditControl::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoFileControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoFileControl());
}



uno::Any GraphicControlModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_GRAPHIC )
        return uno::Any( uno::Reference< graphic::XGraphic >() );

    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

void GraphicControlModel::setFastPropertyValue_NoBroadcast( std::unique_lock<std::mutex>& rGuard, sal_Int32 nHandle, const css::uno::Any& rValue )
{
    UnoControlModel::setFastPropertyValue_NoBroadcast( rGuard, nHandle, rValue );

    // - ImageAlign and ImagePosition need to correspond to each other
    // - Graphic and ImageURL need to correspond to each other
    try
    {
        switch ( nHandle )
        {
        case BASEPROPERTY_IMAGEURL:
            if ( !mbAdjustingGraphic && ImplHasProperty( BASEPROPERTY_GRAPHIC ) )
            {
                mbAdjustingGraphic = true;
                OUString sImageURL;
                OSL_VERIFY( rValue >>= sImageURL );
                css::uno::Any any;
                getFastPropertyValue(rGuard, any, BASEPROPERTY_REFERER);
                OUString referer;
                any >>= referer;
                setDependentFastPropertyValue( rGuard, BASEPROPERTY_GRAPHIC, uno::Any( ImageHelper::getGraphicFromURL_nothrow( sImageURL, referer ) ) );
                mbAdjustingGraphic = false;
            }
            break;

        case BASEPROPERTY_GRAPHIC:
            if ( !mbAdjustingGraphic && ImplHasProperty( BASEPROPERTY_IMAGEURL ) )
            {
                mbAdjustingGraphic = true;
                setDependentFastPropertyValue( rGuard, BASEPROPERTY_IMAGEURL, uno::Any( OUString() ) );
                mbAdjustingGraphic = false;
            }
            break;

        case BASEPROPERTY_IMAGEALIGN:
            if ( !mbAdjustingImagePosition && ImplHasProperty( BASEPROPERTY_IMAGEPOSITION ) )
            {
                mbAdjustingImagePosition = true;
                sal_Int16 nUNOValue = 0;
                OSL_VERIFY( rValue >>= nUNOValue );
                setDependentFastPropertyValue( rGuard, BASEPROPERTY_IMAGEPOSITION, uno::Any( getExtendedImagePosition( nUNOValue ) ) );
                mbAdjustingImagePosition = false;
            }
            break;
        case BASEPROPERTY_IMAGEPOSITION:
            if ( !mbAdjustingImagePosition && ImplHasProperty( BASEPROPERTY_IMAGEALIGN ) )
            {
                mbAdjustingImagePosition = true;
                sal_Int16 nUNOValue = 0;
                OSL_VERIFY( rValue >>= nUNOValue );
                setDependentFastPropertyValue( rGuard, BASEPROPERTY_IMAGEALIGN, uno::Any( getCompatibleImageAlign( translateImagePosition( nUNOValue ) ) ) );
                mbAdjustingImagePosition = false;
            }
            break;
        }
    }
    catch( const css::uno::Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("toolkit.controls");
        OSL_FAIL( "GraphicControlModel::setFastPropertyValue_NoBroadcast: caught an exception while aligning the ImagePosition/ImageAlign properties!" );
        mbAdjustingImagePosition = false;
    }
}



UnoControlButtonModel::UnoControlButtonModel( const Reference< XComponentContext >& rxContext )
    :GraphicControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXButton>();

    osl_atomic_increment( &m_refCount );
    {
        std::unique_lock aGuard(m_aMutex);
        setFastPropertyValue_NoBroadcast( aGuard, BASEPROPERTY_IMAGEPOSITION, ImplGetDefaultValue( BASEPROPERTY_IMAGEPOSITION ) );
        // this ensures that our ImagePosition is consistent with our ImageAlign property (since both
        // defaults are not per se consistent), since both are coupled in setFastPropertyValue_NoBroadcast
    }
    osl_atomic_decrement( &m_refCount );
}

OUString UnoControlButtonModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.Button"_ustr;
}

uno::Any UnoControlButtonModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    switch ( nPropId )
    {
    case BASEPROPERTY_DEFAULTCONTROL:
        return uno::Any( u"stardiv.vcl.control.Button"_ustr );
    case BASEPROPERTY_TOGGLE:
        return uno::Any( false );
    case BASEPROPERTY_ALIGN:
        return uno::Any( sal_Int16(PROPERTY_ALIGN_CENTER) );
    case BASEPROPERTY_FOCUSONCLICK:
        return uno::Any( true );
    }

    return GraphicControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlButtonModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlButtonModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlButtonModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlButtonModel"_ustr;
}

css::uno::Sequence<OUString> UnoControlButtonModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlButtonModel"_ustr, u"stardiv.vcl.controlmodel.Button"_ustr };
    return comphelper::concatSequences( GraphicControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlButtonModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlButtonModel(context));
}



UnoButtonControl::UnoButtonControl()
    :maActionListeners( *this )
    ,maItemListeners( *this )
{
    maComponentInfos.nWidth = 50;
    maComponentInfos.nHeight = 14;
}

OUString UnoButtonControl::GetComponentServiceName() const
{
    OUString aName( u"pushbutton"_ustr );
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_PUSHBUTTONTYPE ) );
    sal_Int16 n = sal_Int16();
    if ( ( aVal >>= n ) && n )
    {
        // Use PushButtonType later when available...
        switch ( n )
        {
            case 1 /*PushButtonType::OK*/:      aName = "okbutton";
                                                break;
            case 2 /*PushButtonType::CANCEL*/:  aName = "cancelbutton";
                                                break;
            case 3 /*PushButtonType::HELP*/:    aName = "helpbutton";
                                                break;
            default:
            {
                OSL_FAIL( "Unknown Button Type!" );
            }
        }
    }
    return aName;
}

void UnoButtonControl::dispose()
{
    lang::EventObject aEvt;
    aEvt.Source = getXWeak();
    maActionListeners.disposeAndClear( aEvt );
    maItemListeners.disposeAndClear( aEvt );
    UnoControlBase::dispose();
}

void UnoButtonControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoControlBase::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XButton > xButton( getPeer(), uno::UNO_QUERY );
    xButton->setActionCommand( maActionCommand );
    if ( maActionListeners.getLength() )
        xButton->addActionListener( &maActionListeners );

    uno::Reference< XToggleButton > xPushButton( getPeer(), uno::UNO_QUERY );
    if ( xPushButton.is() )
        xPushButton->addItemListener( this );
}

void UnoButtonControl::addActionListener(const uno::Reference< awt::XActionListener > & l)
{
    // tdf#150974 some extensions pass null
    if (!l)
    {
        SAL_WARN("toolkit", "null XActionListener");
        return;
    }

    maActionListeners.addInterface( l );
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XButton >  xButton( getPeer(), uno::UNO_QUERY );
        xButton->addActionListener( &maActionListeners );
    }
}

void UnoButtonControl::removeActionListener(const uno::Reference< awt::XActionListener > & l)
{
    // tdf#150974 some extensions pass null
    if (!l)
    {
        SAL_WARN("toolkit", "null XActionListener");
        return;
    }

    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XButton >  xButton( getPeer(), uno::UNO_QUERY );
        xButton->removeActionListener( &maActionListeners );
    }
    maActionListeners.removeInterface( l );
}

void UnoButtonControl::addItemListener(const uno::Reference< awt::XItemListener > & l)
{
    maItemListeners.addInterface( l );
}

void UnoButtonControl::removeItemListener(const uno::Reference< awt::XItemListener > & l)
{
    maItemListeners.removeInterface( l );
}

void SAL_CALL UnoButtonControl::disposing( const lang::EventObject& Source )
{
    UnoControlBase::disposing( Source );
}

void SAL_CALL UnoButtonControl::itemStateChanged( const awt::ItemEvent& rEvent )
{
    // forward to model
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STATE ), uno::Any(static_cast<sal_Int16>(rEvent.Selected)), false );

    // multiplex
    ItemEvent aEvent( rEvent );
    aEvent.Source = *this;
    maItemListeners.itemStateChanged( aEvent );
}

void UnoButtonControl::setLabel( const OUString&  rLabel )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_LABEL ), uno::Any(rLabel), true );
}

void UnoButtonControl::setActionCommand( const OUString& rCommand )
{
    maActionCommand = rCommand;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XButton >  xButton( getPeer(), uno::UNO_QUERY );
        xButton->setActionCommand( rCommand );
    }
}

awt::Size UnoButtonControl::getMinimumSize(  )
{
    return Impl_getMinimumSize();
}

awt::Size UnoButtonControl::getPreferredSize(  )
{
    return Impl_getPreferredSize();
}

awt::Size UnoButtonControl::calcAdjustedSize( const awt::Size& rNewSize )
{
    return Impl_calcAdjustedSize( rNewSize );
}

OUString UnoButtonControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoButtonControl"_ustr;
}

css::uno::Sequence<OUString> UnoButtonControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlButton"_ustr, u"stardiv.vcl.control.Button"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals);
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoButtonControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoButtonControl());
}



UnoControlImageControlModel::UnoControlImageControlModel( const Reference< XComponentContext >& rxContext )
    :GraphicControlModel( rxContext )
    ,mbAdjustingImageScaleMode( false )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXImageControl>();
}

OUString UnoControlImageControlModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.ImageControl"_ustr;
}

OUString UnoControlImageControlModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlImageControlModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlImageControlModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals {
        u"com.sun.star.awt.UnoControlImageButtonModel"_ustr,
        u"com.sun.star.awt.UnoControlImageControlModel"_ustr,
        u"stardiv.vcl.controlmodel.ImageButton"_ustr,
        u"stardiv.vcl.controlmodel.ImageControl"_ustr
    };
    return comphelper::concatSequences( GraphicControlModel::getSupportedServiceNames(), vals);
}

uno::Any UnoControlImageControlModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
        return uno::Any( u"stardiv.vcl.control.ImageControl"_ustr );

    if ( nPropId == BASEPROPERTY_IMAGE_SCALE_MODE )
        return Any( awt::ImageScaleMode::ANISOTROPIC );

    return GraphicControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlImageControlModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlImageControlModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

void UnoControlImageControlModel::setFastPropertyValue_NoBroadcast( std::unique_lock<std::mutex>& rGuard, sal_Int32 _nHandle, const css::uno::Any& _rValue )
{
    GraphicControlModel::setFastPropertyValue_NoBroadcast( rGuard, _nHandle, _rValue );

    // ScaleImage is an older (and less powerful) version of ScaleMode, but keep both in sync as far as possible
    try
    {
        switch ( _nHandle )
        {
        case BASEPROPERTY_IMAGE_SCALE_MODE:
            if ( !mbAdjustingImageScaleMode && ImplHasProperty( BASEPROPERTY_SCALEIMAGE ) )
            {
                mbAdjustingImageScaleMode = true;
                sal_Int16 nScaleMode( awt::ImageScaleMode::ANISOTROPIC );
                OSL_VERIFY( _rValue >>= nScaleMode );
                setDependentFastPropertyValue( rGuard, BASEPROPERTY_SCALEIMAGE, uno::Any( nScaleMode != awt::ImageScaleMode::NONE ) );
                mbAdjustingImageScaleMode = false;
            }
            break;
        case BASEPROPERTY_SCALEIMAGE:
            if ( !mbAdjustingImageScaleMode && ImplHasProperty( BASEPROPERTY_IMAGE_SCALE_MODE ) )
            {
                mbAdjustingImageScaleMode = true;
                bool bScale = true;
                OSL_VERIFY( _rValue >>= bScale );
                setDependentFastPropertyValue( rGuard, BASEPROPERTY_IMAGE_SCALE_MODE, uno::Any( bScale ? awt::ImageScaleMode::ANISOTROPIC : awt::ImageScaleMode::NONE ) );
                mbAdjustingImageScaleMode = false;
            }
            break;
        }
    }
    catch( const Exception& )
    {
        mbAdjustingImageScaleMode = false;
        throw;
    }
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlImageControlModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlImageControlModel(context));
}



UnoImageControlControl::UnoImageControlControl()
    :maActionListeners( *this )
{
    // TODO: Where should I look for defaults?
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 100;
}

OUString UnoImageControlControl::GetComponentServiceName() const
{
    return u"fixedimage"_ustr;
}

void UnoImageControlControl::dispose()
{
    lang::EventObject aEvt;
    aEvt.Source = getXWeak();
    maActionListeners.disposeAndClear( aEvt );
    UnoControl::dispose();
}

sal_Bool UnoImageControlControl::isTransparent()
{
    return true;
}

awt::Size UnoImageControlControl::getMinimumSize(  )
{
    return Impl_getMinimumSize();
}

awt::Size UnoImageControlControl::getPreferredSize(  )
{
    return Impl_getPreferredSize();
}

awt::Size UnoImageControlControl::calcAdjustedSize( const awt::Size& rNewSize )
{
    return Impl_calcAdjustedSize( rNewSize );
}

OUString UnoImageControlControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoImageControlControl"_ustr;
}

css::uno::Sequence<OUString> UnoImageControlControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals {
        u"com.sun.star.awt.UnoControlImageButton"_ustr,
        u"com.sun.star.awt.UnoControlImageControl"_ustr,
        u"stardiv.vcl.control.ImageButton"_ustr,
        u"stardiv.vcl.control.ImageControl"_ustr
    };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals);
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoImageControlControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoImageControlControl());
}



UnoControlRadioButtonModel::UnoControlRadioButtonModel( const Reference< XComponentContext >& rxContext )
    :GraphicControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXRadioButton>();
}

OUString UnoControlRadioButtonModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.RadioButton"_ustr;
}

uno::Any UnoControlRadioButtonModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    switch ( nPropId )
    {
    case BASEPROPERTY_DEFAULTCONTROL:
        return uno::Any( u"stardiv.vcl.control.RadioButton"_ustr );

    case BASEPROPERTY_VISUALEFFECT:
        return uno::Any( sal_Int16(awt::VisualEffect::LOOK3D) );
    }

    return GraphicControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlRadioButtonModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlRadioButtonModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlRadioButtonModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlRadioButtonModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlRadioButtonModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlRadioButtonModel"_ustr, u"stardiv.vcl.controlmodel.RadioButton"_ustr };
    return comphelper::concatSequences( GraphicControlModel::getSupportedServiceNames(), vals);
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlRadioButtonModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlRadioButtonModel(context));
}



UnoRadioButtonControl::UnoRadioButtonControl()
    :maItemListeners( *this )
    ,maActionListeners( *this )
{
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 12;
}

OUString UnoRadioButtonControl::GetComponentServiceName() const
{
    return u"radiobutton"_ustr;
}

void UnoRadioButtonControl::dispose()
{
    lang::EventObject aEvt;
    aEvt.Source = getXWeak();
    maItemListeners.disposeAndClear( aEvt );
    UnoControlBase::dispose();
}


sal_Bool UnoRadioButtonControl::isTransparent()
{
    return true;
}

void UnoRadioButtonControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoControlBase::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XRadioButton >  xRadioButton( getPeer(), uno::UNO_QUERY );
    xRadioButton->addItemListener( this );

    uno::Reference < awt::XButton > xButton( getPeer(), uno::UNO_QUERY );
    xButton->setActionCommand( maActionCommand );
    if ( maActionListeners.getLength() )
        xButton->addActionListener( &maActionListeners );

    // as default, set the "AutoToggle" to true
    // (it is set to false in VCLXToolkit::ImplCreateWindow because of #87254#, but we want to
    // have it enabled by default because of 85071)
    uno::Reference< awt::XVclWindowPeer >  xVclWindowPeer( getPeer(), uno::UNO_QUERY );
    if ( xVclWindowPeer.is() )
        xVclWindowPeer->setProperty( GetPropertyName( BASEPROPERTY_AUTOTOGGLE ), css::uno::Any(true) );
}

void UnoRadioButtonControl::addItemListener(const uno::Reference < awt::XItemListener > & l)
{
    maItemListeners.addInterface( l );
}

void UnoRadioButtonControl::removeItemListener(const uno::Reference < awt::XItemListener > & l)
{
    maItemListeners.removeInterface( l );
}

void UnoRadioButtonControl::addActionListener(const uno::Reference< awt::XActionListener > & l)
{
    maActionListeners.addInterface( l );
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XButton >  xButton( getPeer(), uno::UNO_QUERY );
        xButton->addActionListener( &maActionListeners );
    }
}

void UnoRadioButtonControl::removeActionListener(const uno::Reference< awt::XActionListener > & l)
{
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XButton >  xButton( getPeer(), uno::UNO_QUERY );
        xButton->removeActionListener( &maActionListeners );
    }
    maActionListeners.removeInterface( l );
}

void UnoRadioButtonControl::setLabel( const OUString&  rLabel )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_LABEL ), uno::Any(rLabel), true );
}

void UnoRadioButtonControl::setActionCommand( const OUString& rCommand )
{
    maActionCommand = rCommand;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XButton >  xButton( getPeer(), uno::UNO_QUERY );
        xButton->setActionCommand( rCommand );
    }
}

void UnoRadioButtonControl::setState( sal_Bool bOn )
{
    sal_Int16 nState = bOn ? 1 : 0;
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STATE ), uno::Any(nState), true );
}

sal_Bool UnoRadioButtonControl::getState()
{
    sal_Int16 nState = 0;
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STATE ) );
    aVal >>= nState;
    return nState != 0;
}

void UnoRadioButtonControl::itemStateChanged( const awt::ItemEvent& rEvent )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STATE ), uno::Any(static_cast<sal_Int16>(rEvent.Selected)), false );

    // compatibility:
    // in OOo 1.0.x, when the user clicked a radio button in a group of buttons, this resulted
    // in _one_ itemStateChanged call for exactly the radio button which's state changed from
    // "0" to "1".
    // Nowadays, since the listener handling changed a lot towards 1.1 (the VCLXWindow reacts on more
    // basic events from the VCL-windows, not anymore on the Link-based events like in 1.0.x), this
    // isn't the case anymore: For instance, this method here gets called for the radio button
    // which is being implicitly _de_selected, too. This is pretty bad for compatibility.
    // Thus, we suppress all events with a new state other than "1". This is unlogical, and weird, when looking
    // from a pure API perspective, but it's _compatible_ with older product versions, and this is
    // all which matters here.
    // #i14703#
    if ( 1 == rEvent.Selected )
    {
        if ( maItemListeners.getLength() )
            maItemListeners.itemStateChanged( rEvent );
    }
        // note that speaking strictly, this is wrong: When in 1.0.x, the user would have de-selected
        // a radio button _without_ selecting another one, this would have caused a notification.
        // With the change done here, this today won't cause a notification anymore.

        // Fortunately, it's not possible for the user to de-select a radio button without selecting another on,
        // at least not via the regular UI. It _would_ be possible via the Accessibility API, which
        // counts as "user input", too. But in 1.0.x, there was no Accessibility API, so there is nothing
        // to be inconsistent with.
}

awt::Size UnoRadioButtonControl::getMinimumSize(  )
{
    return Impl_getMinimumSize();
}

awt::Size UnoRadioButtonControl::getPreferredSize(  )
{
    return Impl_getPreferredSize();
}

awt::Size UnoRadioButtonControl::calcAdjustedSize( const awt::Size& rNewSize )
{
    return Impl_calcAdjustedSize( rNewSize );
}

OUString UnoRadioButtonControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoRadioButtonControl"_ustr;
}

css::uno::Sequence<OUString> UnoRadioButtonControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlRadioButton"_ustr, u"stardiv.vcl.control.RadioButton"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoRadioButtonControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoRadioButtonControl());
}



UnoControlCheckBoxModel::UnoControlCheckBoxModel( const Reference< XComponentContext >& rxContext )
    :GraphicControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXCheckBox>();
}

OUString UnoControlCheckBoxModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.CheckBox"_ustr;
}

uno::Any UnoControlCheckBoxModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    switch ( nPropId )
    {
    case BASEPROPERTY_DEFAULTCONTROL:
        return uno::Any( u"stardiv.vcl.control.CheckBox"_ustr );

    case BASEPROPERTY_VISUALEFFECT:
        return uno::Any( sal_Int16(awt::VisualEffect::LOOK3D) );
    }

    return GraphicControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlCheckBoxModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlCheckBoxModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlCheckBoxModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlCheckBoxModel"_ustr;
}

css::uno::Sequence<OUString> UnoControlCheckBoxModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlCheckBoxModel"_ustr, u"stardiv.vcl.controlmodel.CheckBox"_ustr };
    return comphelper::concatSequences( GraphicControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlCheckBoxModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlCheckBoxModel(context));
}



UnoCheckBoxControl::UnoCheckBoxControl()
    :maItemListeners( *this ), maActionListeners( *this )
{
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 12;
}

OUString UnoCheckBoxControl::GetComponentServiceName() const
{
    return u"checkbox"_ustr;
}

void UnoCheckBoxControl::dispose()
{
    lang::EventObject aEvt;
    aEvt.Source = getXWeak();
    maItemListeners.disposeAndClear( aEvt );
    UnoControlBase::dispose();
}

sal_Bool UnoCheckBoxControl::isTransparent()
{
    return true;
}

void UnoCheckBoxControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoControlBase::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XCheckBox >  xCheckBox( getPeer(), uno::UNO_QUERY );
    xCheckBox->addItemListener( this );

    uno::Reference < awt::XButton > xButton( getPeer(), uno::UNO_QUERY );
    xButton->setActionCommand( maActionCommand );
    if ( maActionListeners.getLength() )
        xButton->addActionListener( &maActionListeners );
}

void UnoCheckBoxControl::addItemListener(const uno::Reference < awt::XItemListener > & l)
{
    maItemListeners.addInterface( l );
}

void UnoCheckBoxControl::removeItemListener(const uno::Reference < awt::XItemListener > & l)
{
    maItemListeners.removeInterface( l );
}

void UnoCheckBoxControl::addActionListener(const uno::Reference< awt::XActionListener > & l)
{
    maActionListeners.addInterface( l );
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XButton >  xButton( getPeer(), uno::UNO_QUERY );
        xButton->addActionListener( &maActionListeners );
    }
}

void UnoCheckBoxControl::removeActionListener(const uno::Reference< awt::XActionListener > & l)
{
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XButton >  xButton( getPeer(), uno::UNO_QUERY );
        xButton->removeActionListener( &maActionListeners );
    }
    maActionListeners.removeInterface( l );
}

void UnoCheckBoxControl::setActionCommand( const OUString& rCommand )
{
    maActionCommand = rCommand;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XButton > xButton( getPeer(), uno::UNO_QUERY );
        xButton->setActionCommand( rCommand );
    }
}


void UnoCheckBoxControl::setLabel( const OUString&  rLabel )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_LABEL ), uno::Any(rLabel), true );
}

void UnoCheckBoxControl::setState( sal_Int16 n )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STATE ), uno::Any(n), true );
}

sal_Int16 UnoCheckBoxControl::getState()
{
    sal_Int16 nState = 0;
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STATE ) );
    aVal >>= nState;
    return nState;
}

void UnoCheckBoxControl::enableTriState( sal_Bool b )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_TRISTATE ), uno::Any(b), true );
}

void UnoCheckBoxControl::itemStateChanged( const awt::ItemEvent& rEvent )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STATE ), uno::Any(static_cast<sal_Int16>(rEvent.Selected)), false );

    if ( maItemListeners.getLength() )
        maItemListeners.itemStateChanged( rEvent );
}

awt::Size UnoCheckBoxControl::getMinimumSize(  )
{
    return Impl_getMinimumSize();
}

awt::Size UnoCheckBoxControl::getPreferredSize(  )
{
    return Impl_getPreferredSize();
}

awt::Size UnoCheckBoxControl::calcAdjustedSize( const awt::Size& rNewSize )
{
    return Impl_calcAdjustedSize( rNewSize );
}

OUString UnoCheckBoxControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoCheckBoxControl"_ustr;
}

css::uno::Sequence<OUString> UnoCheckBoxControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlCheckBox"_ustr, u"stardiv.vcl.control.CheckBox"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoCheckBoxControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoCheckBoxControl());
}



UnoControlFixedHyperlinkModel::UnoControlFixedHyperlinkModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXFixedHyperlink>();
}

OUString UnoControlFixedHyperlinkModel::getServiceName()
{
    return u"com.sun.star.awt.UnoControlFixedHyperlinkModel"_ustr;
}

uno::Any UnoControlFixedHyperlinkModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"com.sun.star.awt.UnoControlFixedHyperlink"_ustr );
    }
    else if ( nPropId == BASEPROPERTY_BORDER )
    {
        return uno::Any(sal_Int16(0));
    }
    else if ( nPropId == BASEPROPERTY_URL )
    {
        return uno::Any( OUString() );
    }

    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlFixedHyperlinkModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlFixedHyperlinkModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlFixedHyperlinkModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlFixedHyperlinkModel(context));
}



UnoFixedHyperlinkControl::UnoFixedHyperlinkControl()
    :maActionListeners( *this )
{
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 12;
}

OUString UnoFixedHyperlinkControl::GetComponentServiceName() const
{
    return u"fixedhyperlink"_ustr;
}

// uno::XInterface
uno::Any UnoFixedHyperlinkControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XFixedHyperlink* >(this),
                                        static_cast< awt::XLayoutConstrains* >(this) );
    return (aRet.hasValue() ? aRet : UnoControlBase::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoFixedHyperlinkControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoFixedHyperlinkControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XFixedHyperlink>::get(),
        cppu::UnoType<awt::XLayoutConstrains>::get(),
        UnoControlBase::getTypes()
    );
    return aTypeList.getTypes();
}

sal_Bool UnoFixedHyperlinkControl::isTransparent()
{
    return true;
}

void UnoFixedHyperlinkControl::setText( const OUString& Text )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_LABEL ), uno::Any(Text), true );
}

OUString UnoFixedHyperlinkControl::getText()
{
    return ImplGetPropertyValue_UString( BASEPROPERTY_LABEL );
}

void UnoFixedHyperlinkControl::setURL( const OUString& URL )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_URL ), uno::Any(URL), true );
}

OUString UnoFixedHyperlinkControl::getURL(  )
{
    return ImplGetPropertyValue_UString( BASEPROPERTY_URL );
}

void UnoFixedHyperlinkControl::setAlignment( sal_Int16 nAlign )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_ALIGN ), uno::Any(nAlign), true );
}

sal_Int16 UnoFixedHyperlinkControl::getAlignment()
{
    sal_Int16 nAlign = 0;
    if ( mxModel.is() )
    {
        uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_ALIGN ) );
        aVal >>= nAlign;
    }
    return nAlign;
}

awt::Size UnoFixedHyperlinkControl::getMinimumSize(  )
{
    return Impl_getMinimumSize();
}

awt::Size UnoFixedHyperlinkControl::getPreferredSize(  )
{
    return Impl_getPreferredSize();
}

awt::Size UnoFixedHyperlinkControl::calcAdjustedSize( const awt::Size& rNewSize )
{
    return Impl_calcAdjustedSize( rNewSize );
}

void UnoFixedHyperlinkControl::dispose()
{
    lang::EventObject aEvt;
    aEvt.Source = getXWeak();
    maActionListeners.disposeAndClear( aEvt );
    UnoControlBase::dispose();
}

void UnoFixedHyperlinkControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoControlBase::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XFixedHyperlink > xFixedHyperlink( getPeer(), uno::UNO_QUERY );
    if ( maActionListeners.getLength() )
        xFixedHyperlink->addActionListener( &maActionListeners );
}

void UnoFixedHyperlinkControl::addActionListener(const uno::Reference< awt::XActionListener > & l)
{
    maActionListeners.addInterface( l );
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XFixedHyperlink >  xFixedHyperlink( getPeer(), uno::UNO_QUERY );
        xFixedHyperlink->addActionListener( &maActionListeners );
    }
}

void UnoFixedHyperlinkControl::removeActionListener(const uno::Reference< awt::XActionListener > & l)
{
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XFixedHyperlink >  xFixedHyperlink( getPeer(), uno::UNO_QUERY );
        xFixedHyperlink->removeActionListener( &maActionListeners );
    }
    maActionListeners.removeInterface( l );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoFixedHyperlinkControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoFixedHyperlinkControl());
}



UnoControlFixedTextModel::UnoControlFixedTextModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXFixedText>();
}

OUString UnoControlFixedTextModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.FixedText"_ustr;
}

uno::Any UnoControlFixedTextModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.FixedText"_ustr );
    }
    else if ( nPropId == BASEPROPERTY_BORDER )
    {
        return uno::Any(sal_Int16(0));
    }

    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlFixedTextModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlFixedTextModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlFixedTextModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlFixedTextModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlFixedTextModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlFixedTextModel"_ustr, u"stardiv.vcl.controlmodel.FixedText"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlFixedTextModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlFixedTextModel(context));
}



UnoFixedTextControl::UnoFixedTextControl()
{
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 12;
}

OUString UnoFixedTextControl::GetComponentServiceName() const
{
    return u"fixedtext"_ustr;
}

// uno::XInterface
uno::Any UnoFixedTextControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XFixedText* >(this),
                                        static_cast< awt::XLayoutConstrains* >(this) );
    return (aRet.hasValue() ? aRet : UnoControlBase::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoFixedTextControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoFixedTextControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XFixedText>::get(),
        cppu::UnoType<awt::XLayoutConstrains>::get(),
        UnoControlBase::getTypes()
    );
    return aTypeList.getTypes();
}

sal_Bool UnoFixedTextControl::isTransparent()
{
    return true;
}

void UnoFixedTextControl::setText( const OUString& Text )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_LABEL ), uno::Any(Text), true );
}

OUString UnoFixedTextControl::getText()
{
    return ImplGetPropertyValue_UString( BASEPROPERTY_LABEL );
}

void UnoFixedTextControl::setAlignment( sal_Int16 nAlign )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_ALIGN ), uno::Any(nAlign), true );
}

sal_Int16 UnoFixedTextControl::getAlignment()
{
    sal_Int16 nAlign = 0;
    if ( mxModel.is() )
    {
        uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_ALIGN ) );
        aVal >>= nAlign;
    }
    return nAlign;
}

awt::Size UnoFixedTextControl::getMinimumSize(  )
{
    return Impl_getMinimumSize();
}

awt::Size UnoFixedTextControl::getPreferredSize(  )
{
    return Impl_getPreferredSize();
}

awt::Size UnoFixedTextControl::calcAdjustedSize( const awt::Size& rNewSize )
{
    return Impl_calcAdjustedSize( rNewSize );
}

OUString UnoFixedTextControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoFixedTextControl"_ustr;
}

css::uno::Sequence<OUString> UnoFixedTextControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlFixedText"_ustr, u"stardiv.vcl.control.FixedText"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoFixedTextControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoFixedTextControl());
}



UnoControlGroupBoxModel::UnoControlGroupBoxModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    ImplRegisterProperty( BASEPROPERTY_DEFAULTCONTROL );
    ImplRegisterProperty( BASEPROPERTY_ENABLED );
    ImplRegisterProperty( BASEPROPERTY_ENABLEVISIBLE );
    ImplRegisterProperty( BASEPROPERTY_FONTDESCRIPTOR );
    ImplRegisterProperty( BASEPROPERTY_HELPTEXT );
    ImplRegisterProperty( BASEPROPERTY_HELPURL );
    ImplRegisterProperty( BASEPROPERTY_LABEL );
    ImplRegisterProperty( BASEPROPERTY_PRINTABLE );
    ImplRegisterProperty( BASEPROPERTY_WRITING_MODE );
    ImplRegisterProperty( BASEPROPERTY_CONTEXT_WRITING_MODE );
}

OUString UnoControlGroupBoxModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.GroupBox"_ustr;
}

uno::Any UnoControlGroupBoxModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any(u"stardiv.vcl.control.GroupBox"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlGroupBoxModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlGroupBoxModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlGroupBoxModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlGroupBoxModel"_ustr;
}

css::uno::Sequence<OUString> UnoControlGroupBoxModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlGroupBoxModel"_ustr, u"stardiv.vcl.controlmodel.GroupBox"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlGroupBoxModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlGroupBoxModel(context));
}



UnoGroupBoxControl::UnoGroupBoxControl()
{
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 100;
}

OUString UnoGroupBoxControl::GetComponentServiceName() const
{
    return u"groupbox"_ustr;
}

sal_Bool UnoGroupBoxControl::isTransparent()
{
    return true;
}

OUString UnoGroupBoxControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoGroupBoxControl"_ustr;
}

css::uno::Sequence<OUString> UnoGroupBoxControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlGroupBox"_ustr, u"stardiv.vcl.control.GroupBox"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoGroupBoxControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoGroupBoxControl());
}


// = UnoControlListBoxModel_Data

namespace {

struct ListItem
{
    OUString ItemText;
    OUString ItemImageURL;
    Any             ItemData;

    ListItem()
    {
    }

    explicit ListItem( OUString i_ItemText )
        :ItemText(std::move( i_ItemText ))
    {
    }
};

}

typedef beans::Pair< OUString, OUString > UnoListItem;

namespace {

struct StripItemData
{
    UnoListItem operator()( const ListItem& i_rItem )
    {
        return UnoListItem( i_rItem.ItemText, i_rItem.ItemImageURL );
    }
};

}

struct UnoControlListBoxModel_Data
{
    explicit UnoControlListBoxModel_Data( UnoControlListBoxModel& i_rAntiImpl )
        :m_bSettingLegacyProperty( false )
        ,m_rAntiImpl( i_rAntiImpl )
    {
    }

    sal_Int32 getItemCount() const { return sal_Int32( m_aListItems.size() ); }

    const ListItem& getItem( const sal_Int32 i_nIndex ) const
    {
        if ( ( i_nIndex < 0 ) || ( o3tl::make_unsigned(i_nIndex) >= m_aListItems.size() ) )
            throw IndexOutOfBoundsException( OUString(), m_rAntiImpl );
        return m_aListItems[ i_nIndex ];
    }

    ListItem& getItem( const sal_Int32 i_nIndex )
    {
        return const_cast< ListItem& >( static_cast< const UnoControlListBoxModel_Data* >( this )->getItem( i_nIndex ) );
    }

    ListItem& insertItem( const sal_Int32 i_nIndex )
    {
        if ( ( i_nIndex < 0 ) || ( o3tl::make_unsigned(i_nIndex) > m_aListItems.size() ) )
            throw IndexOutOfBoundsException( OUString(), m_rAntiImpl );
        return *m_aListItems.insert( m_aListItems.begin() + i_nIndex, ListItem() );
    }

    Sequence< UnoListItem > getAllItems() const
    {
        Sequence< UnoListItem > aItems( sal_Int32( m_aListItems.size() ) );
        ::std::transform( m_aListItems.begin(), m_aListItems.end(), aItems.getArray(), StripItemData() );
        return aItems;
    }

    void copyItems( const UnoControlListBoxModel_Data& i_copySource )
    {
        m_aListItems = i_copySource.m_aListItems;
    }

    void    setAllItems( ::std::vector< ListItem >&& i_rItems )
    {
        m_aListItems = std::move(i_rItems);
    }

    void    removeItem( const sal_Int32 i_nIndex )
    {
        if ( ( i_nIndex < 0 ) || ( o3tl::make_unsigned(i_nIndex) >= m_aListItems.size() ) )
            throw IndexOutOfBoundsException( OUString(), m_rAntiImpl );
        m_aListItems.erase( m_aListItems.begin() + i_nIndex );
    }

    void removeAllItems()
    {
        std::vector<ListItem>().swap(m_aListItems);
    }

public:
    bool                        m_bSettingLegacyProperty;

private:
    UnoControlListBoxModel&     m_rAntiImpl;
    ::std::vector< ListItem >   m_aListItems;
};


// = UnoControlListBoxModel


UnoControlListBoxModel::UnoControlListBoxModel( const Reference< XComponentContext >& rxContext, ConstructorMode const i_mode )
    :UnoControlListBoxModel_Base( rxContext )
    ,m_xData( new UnoControlListBoxModel_Data( *this ) )
{
    if ( i_mode == ConstructDefault )
    {
        UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXListBox>();
    }
}

UnoControlListBoxModel::UnoControlListBoxModel( const UnoControlListBoxModel& i_rSource )
    :UnoControlListBoxModel_Base( i_rSource )
    ,m_xData( new UnoControlListBoxModel_Data( *this ) )
{
    m_xData->copyItems( *i_rSource.m_xData );
}
UnoControlListBoxModel::~UnoControlListBoxModel()
{
}

OUString UnoControlListBoxModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlListBoxModel"_ustr;
}

css::uno::Sequence<OUString> UnoControlListBoxModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlListBoxModel"_ustr, u"stardiv.vcl.controlmodel.ListBox"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

OUString UnoControlListBoxModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.ListBox"_ustr;
}


uno::Any UnoControlListBoxModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.ListBox"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}


::cppu::IPropertyArrayHelper& UnoControlListBoxModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}


// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlListBoxModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}


namespace
{
    struct CreateListItem
    {
        ListItem operator()( const OUString& i_rItemText )
        {
            return ListItem( i_rItemText );
        }
    };
}


void UnoControlListBoxModel::setFastPropertyValue_NoBroadcast( std::unique_lock<std::mutex>& rGuard, sal_Int32 nHandle, const uno::Any& rValue )
{
    UnoControlModel::setFastPropertyValue_NoBroadcast( rGuard, nHandle, rValue );

    if ( nHandle != BASEPROPERTY_STRINGITEMLIST )
        return;

    // reset selection
    uno::Sequence<sal_Int16> aSeq;
    setDependentFastPropertyValue( rGuard, BASEPROPERTY_SELECTEDITEMS, uno::Any(aSeq) );

    if ( m_xData->m_bSettingLegacyProperty )
        return;

    // synchronize the legacy StringItemList property with our list items
    Sequence< OUString > aStringItemList;
    Any aPropValue;
    getFastPropertyValue( rGuard, aPropValue, BASEPROPERTY_STRINGITEMLIST );
    OSL_VERIFY( aPropValue >>= aStringItemList );

    ::std::vector< ListItem > aItems( aStringItemList.getLength() );
    ::std::transform(
        std::cbegin(aStringItemList),
        std::cend(aStringItemList),
        aItems.begin(),
        CreateListItem()
    );
    m_xData->setAllItems( std::move(aItems) );

    // since an XItemListListener does not have a "all items modified" or some such method,
    // we simulate this by notifying removal of all items, followed by insertion of all new
    // items
    lang::EventObject aEvent;
    aEvent.Source = *this;
    m_aItemListListeners.notifyEach( rGuard, &XItemListListener::itemListChanged, aEvent );
    // TODO: OPropertySetHelper calls into this method with the mutex locked ...
    // which is wrong for the above notifications ...
}


void UnoControlListBoxModel::ImplNormalizePropertySequence( const sal_Int32 _nCount, sal_Int32* _pHandles,
    uno::Any* _pValues, sal_Int32* _pValidHandles ) const
{
    // dependencies we know:
    // BASEPROPERTY_STRINGITEMLIST->BASEPROPERTY_SELECTEDITEMS
    ImplEnsureHandleOrder( _nCount, _pHandles, _pValues, BASEPROPERTY_STRINGITEMLIST, BASEPROPERTY_SELECTEDITEMS );
    // BASEPROPERTY_STRINGITEMLIST->BASEPROPERTY_TYPEDITEMLIST
    ImplEnsureHandleOrder( _nCount, _pHandles, _pValues, BASEPROPERTY_STRINGITEMLIST, BASEPROPERTY_TYPEDITEMLIST );

    UnoControlModel::ImplNormalizePropertySequence( _nCount, _pHandles, _pValues, _pValidHandles );
}


::sal_Int32 SAL_CALL UnoControlListBoxModel::getItemCount()
{
    std::unique_lock aGuard( m_aMutex );
    return m_xData->getItemCount();
}


void SAL_CALL UnoControlListBoxModel::insertItem( ::sal_Int32 i_nPosition, const OUString& i_rItemText, const OUString& i_rItemImageURL )
{
    std::unique_lock aGuard( m_aMutex );
    // SYNCHRONIZED ----->
    ListItem& rItem( m_xData->insertItem( i_nPosition ) );
    rItem.ItemText = i_rItemText;
    rItem.ItemImageURL = i_rItemImageURL;

    impl_handleInsert( aGuard, i_nPosition, i_rItemText, i_rItemImageURL );
    // <----- SYNCHRONIZED
}


void SAL_CALL UnoControlListBoxModel::insertItemText( ::sal_Int32 i_nPosition, const OUString& i_rItemText )
{
    std::unique_lock aGuard( m_aMutex );
    // SYNCHRONIZED ----->
    ListItem& rItem( m_xData->insertItem( i_nPosition ) );
    rItem.ItemText = i_rItemText;

    impl_handleInsert( aGuard, i_nPosition, i_rItemText, ::std::optional< OUString >() );
    // <----- SYNCHRONIZED
}


void SAL_CALL UnoControlListBoxModel::insertItemImage( ::sal_Int32 i_nPosition, const OUString& i_rItemImageURL )
{
    std::unique_lock aGuard( m_aMutex );
    // SYNCHRONIZED ----->
    ListItem& rItem( m_xData->insertItem( i_nPosition ) );
    rItem.ItemImageURL = i_rItemImageURL;

    impl_handleInsert( aGuard, i_nPosition, ::std::optional< OUString >(), i_rItemImageURL );
    // <----- SYNCHRONIZED
}


void SAL_CALL UnoControlListBoxModel::removeItem( ::sal_Int32 i_nPosition )
{
    std::unique_lock aGuard( m_aMutex );
    // SYNCHRONIZED ----->
    m_xData->removeItem( i_nPosition );

    impl_handleRemove( i_nPosition, aGuard );
    // <----- SYNCHRONIZED
}


void SAL_CALL UnoControlListBoxModel::removeAllItems(  )
{
    std::unique_lock aGuard( m_aMutex );
    // SYNCHRONIZED ----->
    m_xData->removeAllItems();

    impl_handleRemove( -1, aGuard );
    // <----- SYNCHRONIZED
}


void SAL_CALL UnoControlListBoxModel::setItemText( ::sal_Int32 i_nPosition, const OUString& i_rItemText )
{
    std::unique_lock aGuard( m_aMutex );
    // SYNCHRONIZED ----->
    ListItem& rItem( m_xData->getItem( i_nPosition ) );
    rItem.ItemText = i_rItemText;

    impl_handleModify( i_nPosition, i_rItemText, ::std::optional< OUString >(), aGuard );
    // <----- SYNCHRONIZED
}


void SAL_CALL UnoControlListBoxModel::setItemImage( ::sal_Int32 i_nPosition, const OUString& i_rItemImageURL )
{
    std::unique_lock aGuard( m_aMutex );
    // SYNCHRONIZED ----->
    ListItem& rItem( m_xData->getItem( i_nPosition ) );
    rItem.ItemImageURL = i_rItemImageURL;

    impl_handleModify( i_nPosition, ::std::optional< OUString >(), i_rItemImageURL, aGuard );
    // <----- SYNCHRONIZED
}


void SAL_CALL UnoControlListBoxModel::setItemTextAndImage( ::sal_Int32 i_nPosition, const OUString& i_rItemText, const OUString& i_rItemImageURL )
{
    std::unique_lock aGuard( m_aMutex );
    // SYNCHRONIZED ----->
    ListItem& rItem( m_xData->getItem( i_nPosition ) );
    rItem.ItemText = i_rItemText;
    rItem.ItemImageURL = i_rItemImageURL;

    impl_handleModify( i_nPosition, i_rItemText, i_rItemImageURL, aGuard );
    // <----- SYNCHRONIZED
}


void SAL_CALL UnoControlListBoxModel::setItemData( ::sal_Int32 i_nPosition, const Any& i_rDataValue )
{
    std::unique_lock aGuard( m_aMutex );
    ListItem& rItem( m_xData->getItem( i_nPosition ) );
    rItem.ItemData = i_rDataValue;
}


OUString SAL_CALL UnoControlListBoxModel::getItemText( ::sal_Int32 i_nPosition )
{
    std::unique_lock aGuard( m_aMutex );
    const ListItem& rItem( m_xData->getItem( i_nPosition ) );
    return rItem.ItemText;
}


OUString SAL_CALL UnoControlListBoxModel::getItemImage( ::sal_Int32 i_nPosition )
{
    std::unique_lock aGuard( m_aMutex );
    const ListItem& rItem( m_xData->getItem( i_nPosition ) );
    return rItem.ItemImageURL;
}


beans::Pair< OUString, OUString > SAL_CALL UnoControlListBoxModel::getItemTextAndImage( ::sal_Int32 i_nPosition )
{
    std::unique_lock aGuard( m_aMutex );
    const ListItem& rItem( m_xData->getItem( i_nPosition ) );
    return beans::Pair< OUString, OUString >( rItem.ItemText, rItem.ItemImageURL );
}


Any SAL_CALL UnoControlListBoxModel::getItemData( ::sal_Int32 i_nPosition )
{
    std::unique_lock aGuard( m_aMutex );
    const ListItem& rItem( m_xData->getItem( i_nPosition ) );
    return rItem.ItemData;
}


Sequence< beans::Pair< OUString, OUString > > SAL_CALL UnoControlListBoxModel::getAllItems(  )
{
    std::unique_lock aGuard( m_aMutex );
    return m_xData->getAllItems();
}


void SAL_CALL UnoControlListBoxModel::addItemListListener( const uno::Reference< awt::XItemListListener >& i_Listener )
{
    std::unique_lock aGuard( m_aMutex );
    if ( i_Listener.is() )
        m_aItemListListeners.addInterface( aGuard, i_Listener );
}


void SAL_CALL UnoControlListBoxModel::removeItemListListener( const uno::Reference< awt::XItemListListener >& i_Listener )
{
    std::unique_lock aGuard( m_aMutex );
    if ( i_Listener.is() )
        m_aItemListListeners.removeInterface( aGuard, i_Listener );
}


void UnoControlListBoxModel::impl_getStringItemList( std::unique_lock<std::mutex>& rGuard, ::std::vector< OUString >& o_rStringItems ) const
{
    Sequence< OUString > aStringItemList;
    Any aPropValue;
    getFastPropertyValue( rGuard, aPropValue, BASEPROPERTY_STRINGITEMLIST );
    OSL_VERIFY( aPropValue >>= aStringItemList );

    comphelper::sequenceToContainer(o_rStringItems, aStringItemList);
}


void UnoControlListBoxModel::impl_setStringItemList( std::unique_lock<std::mutex>& rGuard, const ::std::vector< OUString >& i_rStringItems )
{
    Sequence< OUString > aStringItems( comphelper::containerToSequence(i_rStringItems) );
    m_xData->m_bSettingLegacyProperty = true;
    try
    {
        setFastPropertyValueImpl( rGuard, BASEPROPERTY_STRINGITEMLIST, uno::Any( aStringItems ) );
    }
    catch( const Exception& )
    {
        m_xData->m_bSettingLegacyProperty = false;
        throw;
    }
    m_xData->m_bSettingLegacyProperty = false;
}


void UnoControlListBoxModel::impl_handleInsert( std::unique_lock<std::mutex>& rGuard,
        const sal_Int32 i_nItemPosition,
        const ::std::optional< OUString >& i_rItemText,
        const ::std::optional< OUString >& i_rItemImageURL )
{
    // SYNCHRONIZED ----->
    // sync with legacy StringItemList property
    ::std::vector< OUString > aStringItems;
    impl_getStringItemList( rGuard, aStringItems );
    OSL_ENSURE( o3tl::make_unsigned( i_nItemPosition ) <= aStringItems.size(), "UnoControlListBoxModel::impl_handleInsert" );
    if ( o3tl::make_unsigned( i_nItemPosition ) <= aStringItems.size() )
    {
        const OUString sItemText( !!i_rItemText ? *i_rItemText : OUString() );
        aStringItems.insert( aStringItems.begin() + i_nItemPosition, sItemText );
    }

    impl_setStringItemList( rGuard, aStringItems );

    // notify ItemListListeners
    impl_notifyItemListEvent( rGuard, i_nItemPosition, i_rItemText, i_rItemImageURL, &XItemListListener::listItemInserted );
}


void UnoControlListBoxModel::impl_handleRemove(
            const sal_Int32 i_nItemPosition,
            std::unique_lock<std::mutex>& i_rClearBeforeNotify )
{
    // SYNCHRONIZED ----->
    const bool bAllItems = ( i_nItemPosition < 0 );
    // sync with legacy StringItemList property
    ::std::vector< OUString > aStringItems;
    impl_getStringItemList( i_rClearBeforeNotify, aStringItems );
    if ( !bAllItems )
    {
        OSL_ENSURE( o3tl::make_unsigned( i_nItemPosition ) < aStringItems.size(), "UnoControlListBoxModel::impl_handleRemove" );
        if ( o3tl::make_unsigned( i_nItemPosition ) < aStringItems.size() )
        {
            aStringItems.erase( aStringItems.begin() + i_nItemPosition );
        }
    }
    else
    {
        aStringItems.resize(0);
    }

    impl_setStringItemList( i_rClearBeforeNotify, aStringItems );

    // notify ItemListListeners
    if ( bAllItems )
    {
        EventObject aEvent( *this );
        m_aItemListListeners.notifyEach( i_rClearBeforeNotify, &XItemListListener::allItemsRemoved, aEvent );
    }
    else
    {
        impl_notifyItemListEvent( i_rClearBeforeNotify, i_nItemPosition, ::std::optional< OUString >(), ::std::optional< OUString >(),
            &XItemListListener::listItemRemoved );
    }
}


void UnoControlListBoxModel::impl_handleModify(
        const sal_Int32 i_nItemPosition, const ::std::optional< OUString >& i_rItemText,
        const ::std::optional< OUString >& i_rItemImageURL,
        std::unique_lock<std::mutex>& i_rClearBeforeNotify )
{
    // SYNCHRONIZED ----->
    if ( !!i_rItemText )
    {
        // sync with legacy StringItemList property
        ::std::vector< OUString > aStringItems;
        impl_getStringItemList( i_rClearBeforeNotify, aStringItems );
        OSL_ENSURE( o3tl::make_unsigned( i_nItemPosition ) < aStringItems.size(), "UnoControlListBoxModel::impl_handleModify" );
        if ( o3tl::make_unsigned( i_nItemPosition ) < aStringItems.size() )
        {
            aStringItems[ i_nItemPosition] = *i_rItemText;
        }

        impl_setStringItemList( i_rClearBeforeNotify, aStringItems );
    }

    // notify ItemListListeners
    impl_notifyItemListEvent( i_rClearBeforeNotify, i_nItemPosition, i_rItemText, i_rItemImageURL, &XItemListListener::listItemModified );
}


void UnoControlListBoxModel::impl_notifyItemListEvent(
    std::unique_lock<std::mutex>& rGuard,
    const sal_Int32 i_nItemPosition, const ::std::optional< OUString >& i_rItemText,
    const ::std::optional< OUString >& i_rItemImageURL,
    void ( SAL_CALL XItemListListener::*NotificationMethod )( const ItemListEvent& ) )
{
    ItemListEvent aEvent;
    aEvent.Source = *this;
    aEvent.ItemPosition = i_nItemPosition;
    if ( !!i_rItemText )
    {
        aEvent.ItemText.IsPresent = true;
        aEvent.ItemText.Value = *i_rItemText;
    }
    if ( !!i_rItemImageURL )
    {
        aEvent.ItemImageURL.IsPresent = true;
        aEvent.ItemImageURL.Value = *i_rItemImageURL;
    }

    m_aItemListListeners.notifyEach( rGuard, NotificationMethod, aEvent );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlListBoxModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlListBoxModel(context));
}



UnoListBoxControl::UnoListBoxControl()
    :maActionListeners( *this )
    ,maItemListeners( *this )
{
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 12;
}

OUString UnoListBoxControl::GetComponentServiceName() const
{
    return u"listbox"_ustr;
}

OUString UnoListBoxControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoListBoxControl"_ustr;
}

css::uno::Sequence<OUString> UnoListBoxControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlListBox"_ustr, u"stardiv.vcl.control.ListBox"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals);
}

void UnoListBoxControl::dispose()
{
    lang::EventObject aEvt;
    aEvt.Source = getXWeak();
    maActionListeners.disposeAndClear( aEvt );
    maItemListeners.disposeAndClear( aEvt );
    UnoControl::dispose();
}

void UnoListBoxControl::ImplUpdateSelectedItemsProperty()
{
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox > xListBox( getPeer(), uno::UNO_QUERY );
        DBG_ASSERT( xListBox.is(), "XListBox?" );

        uno::Sequence<sal_Int16> aSeq = xListBox->getSelectedItemsPos();
        ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_SELECTEDITEMS ), uno::Any(aSeq), false );
    }
}

void UnoListBoxControl::updateFromModel()
{
    UnoControlBase::updateFromModel();

    Reference< XItemListListener > xItemListListener( getPeer(), UNO_QUERY );
    ENSURE_OR_RETURN_VOID( xItemListListener.is(), "UnoListBoxControl::updateFromModel: a peer which is no ItemListListener?!" );

    EventObject aEvent( getModel() );
    xItemListListener->itemListChanged( aEvent );

    // notify the change of the SelectedItems property, again. While our base class, in updateFromModel,
    // already did this, our peer(s) can only legitimately set the selection after they have the string
    // item list, which we just notified with the itemListChanged call.
    const OUString& sSelectedItemsPropName( GetPropertyName( BASEPROPERTY_SELECTEDITEMS ) );
    ImplSetPeerProperty( sSelectedItemsPropName, ImplGetPropertyValue( sSelectedItemsPropName ) );
}

void UnoListBoxControl::ImplSetPeerProperty( const OUString& rPropName, const uno::Any& rVal )
{
    if ( rPropName == GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) )
        // do not forward this to our peer. We are a XItemListListener at our model, and changes in the string item
        // list (which is a legacy property) will, later, arrive as changes in the ItemList. Those latter changes
        // will be forwarded to the peer, which will update itself accordingly.
        return;

    UnoControl::ImplSetPeerProperty( rPropName, rVal );
}

void UnoListBoxControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoControl::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
    xListBox->addItemListener( this );

    if ( maActionListeners.getLength() )
        xListBox->addActionListener( &maActionListeners );
}

void UnoListBoxControl::addActionListener(const uno::Reference< awt::XActionListener > & l)
{
    maActionListeners.addInterface( l );
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        xListBox->addActionListener( &maActionListeners );
    }
}

void UnoListBoxControl::removeActionListener(const uno::Reference< awt::XActionListener > & l)
{
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        xListBox->removeActionListener( &maActionListeners );
    }
    maActionListeners.removeInterface( l );
}

void UnoListBoxControl::addItemListener(const uno::Reference < awt::XItemListener > & l)
{
    maItemListeners.addInterface( l );
}

void UnoListBoxControl::removeItemListener(const uno::Reference < awt::XItemListener > & l)
{
    maItemListeners.removeInterface( l );
}

void UnoListBoxControl::addItem( const OUString& aItem, sal_Int16 nPos )
{
    uno::Sequence<OUString> aSeq { aItem };
    addItems( aSeq, nPos );
}

void UnoListBoxControl::addItems( const uno::Sequence< OUString>& aItems, sal_Int16 nPos )
{
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    sal_uInt16 nNewItems = static_cast<sal_uInt16>(aItems.getLength());
    sal_uInt16 nOldLen = static_cast<sal_uInt16>(aSeq.getLength());
    sal_uInt16 nNewLen = nOldLen + nNewItems;

    uno::Sequence< OUString> aNewSeq( nNewLen );

    if ( ( nPos < 0 ) || ( nPos > nOldLen ) )
        nPos = nOldLen;

    // Items before the Paste-Position
    auto it = std::copy(std::cbegin(aSeq), std::next(std::cbegin(aSeq), nPos), aNewSeq.getArray());

    // New Items
    it = std::copy(aItems.begin(), aItems.end(), it);

    // Rest of old Items
    std::copy(std::next(std::cbegin(aSeq), nPos), std::cend(aSeq), it);

    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ), uno::Any(aNewSeq), true );
}

void UnoListBoxControl::removeItems( sal_Int16 nPos, sal_Int16 nCount )
{
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    sal_uInt16 nOldLen = static_cast<sal_uInt16>(aSeq.getLength());
    if ( !(nOldLen && ( nPos < nOldLen )) )
        return;

    if ( nCount > ( nOldLen-nPos ) )
        nCount = nOldLen-nPos;

    sal_uInt16 nNewLen = nOldLen - nCount;

    uno::Sequence< OUString> aNewSeq( nNewLen );

    // Items before the Remove-Position
    auto it = std::copy(std::cbegin(aSeq), std::next(std::cbegin(aSeq), nPos), aNewSeq.getArray());

    // Rest of Items
    std::copy(std::next(std::cbegin(aSeq), nPos + nCount), std::cend(aSeq), it);

    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ), uno::Any(aNewSeq), true );
}

sal_Int16 UnoListBoxControl::getItemCount()
{
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    return static_cast<sal_Int16>(aSeq.getLength());
}

OUString UnoListBoxControl::getItem( sal_Int16 nPos )
{
    OUString aItem;
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    if ( nPos < aSeq.getLength() )
        aItem = aSeq[nPos];
    return aItem;
}

uno::Sequence< OUString> UnoListBoxControl::getItems()
{
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    return aSeq;
}

sal_Int16 UnoListBoxControl::getSelectedItemPos()
{
    sal_Int16 n = -1;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        n = xListBox->getSelectedItemPos();
    }
    return n;
}

uno::Sequence<sal_Int16> UnoListBoxControl::getSelectedItemsPos()
{
    uno::Sequence<sal_Int16> aSeq;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        aSeq = xListBox->getSelectedItemsPos();
    }
    return aSeq;
}

OUString UnoListBoxControl::getSelectedItem()
{
    OUString aItem;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        aItem = xListBox->getSelectedItem();
    }
    return aItem;
}

uno::Sequence< OUString> UnoListBoxControl::getSelectedItems()
{
    uno::Sequence< OUString> aSeq;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        aSeq = xListBox->getSelectedItems();
    }
    return aSeq;
}

void UnoListBoxControl::selectItemPos( sal_Int16 nPos, sal_Bool bSelect )
{
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        xListBox->selectItemPos( nPos, bSelect );
    }
    ImplUpdateSelectedItemsProperty();
}

void UnoListBoxControl::selectItemsPos( const uno::Sequence<sal_Int16>& aPositions, sal_Bool bSelect )
{
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        xListBox->selectItemsPos( aPositions, bSelect );
    }
    ImplUpdateSelectedItemsProperty();
}

void UnoListBoxControl::selectItem( const OUString& aItem, sal_Bool bSelect )
{
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        xListBox->selectItem( aItem, bSelect );
    }
    ImplUpdateSelectedItemsProperty();
}

void UnoListBoxControl::makeVisible( sal_Int16 nEntry )
{
    if ( getPeer().is() )
    {
        uno::Reference < awt::XListBox >  xListBox( getPeer(), uno::UNO_QUERY );
        xListBox->makeVisible( nEntry );
    }
}

void UnoListBoxControl::setDropDownLineCount( sal_Int16 nLines )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_LINECOUNT ), uno::Any(nLines), true );
}

sal_Int16 UnoListBoxControl::getDropDownLineCount()
{
    return ImplGetPropertyValue_INT16( BASEPROPERTY_LINECOUNT );
}

sal_Bool UnoListBoxControl::isMutipleMode()
{
    return ImplGetPropertyValue_BOOL( BASEPROPERTY_MULTISELECTION );
}

void UnoListBoxControl::setMultipleMode( sal_Bool bMulti )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_MULTISELECTION ), uno::Any(bMulti), true );
}

void UnoListBoxControl::itemStateChanged( const awt::ItemEvent& rEvent )
{
    ImplUpdateSelectedItemsProperty();
    if ( maItemListeners.getLength() )
    {
        try
        {
            maItemListeners.itemStateChanged( rEvent );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "toolkit", "UnoListBoxControl::itemStateChanged");
        }
    }
}

awt::Size UnoListBoxControl::getMinimumSize(  )
{
    return Impl_getMinimumSize();
}

awt::Size UnoListBoxControl::getPreferredSize(  )
{
    return Impl_getPreferredSize();
}

awt::Size UnoListBoxControl::calcAdjustedSize( const awt::Size& rNewSize )
{
    return Impl_calcAdjustedSize( rNewSize );
}

awt::Size UnoListBoxControl::getMinimumSize( sal_Int16 nCols, sal_Int16 nLines )
{
    return Impl_getMinimumSize( nCols, nLines );
}

void UnoListBoxControl::getColumnsAndLines( sal_Int16& nCols, sal_Int16& nLines )
{
    Impl_getColumnsAndLines( nCols, nLines );
}

sal_Bool SAL_CALL UnoListBoxControl::setModel( const uno::Reference< awt::XControlModel >& i_rModel )
{
    ::osl::MutexGuard aGuard( GetMutex() );

    const Reference< XItemList > xOldItems( getModel(), UNO_QUERY );
    OSL_ENSURE( xOldItems.is() || !getModel().is(), "UnoListBoxControl::setModel: illegal old model!" );
    const Reference< XItemList > xNewItems( i_rModel, UNO_QUERY );
    OSL_ENSURE( xNewItems.is() || !i_rModel.is(), "UnoListBoxControl::setModel: illegal new model!" );

    if ( !UnoListBoxControl_Base::setModel( i_rModel ) )
        return false;

    if ( xOldItems.is() )
        xOldItems->removeItemListListener( this );
    if ( xNewItems.is() )
        xNewItems->addItemListListener( this );

    return true;
}

void SAL_CALL UnoListBoxControl::listItemInserted( const awt::ItemListEvent& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoListBoxControl::listItemInserted: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->listItemInserted( i_rEvent );
}

void SAL_CALL UnoListBoxControl::listItemRemoved( const awt::ItemListEvent& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoListBoxControl::listItemRemoved: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->listItemRemoved( i_rEvent );
}

void SAL_CALL UnoListBoxControl::listItemModified( const awt::ItemListEvent& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoListBoxControl::listItemModified: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->listItemModified( i_rEvent );
}

void SAL_CALL UnoListBoxControl::allItemsRemoved( const lang::EventObject& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoListBoxControl::allItemsRemoved: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->allItemsRemoved( i_rEvent );
}

void SAL_CALL UnoListBoxControl::itemListChanged( const lang::EventObject& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoListBoxControl::itemListChanged: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->itemListChanged( i_rEvent );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoListBoxControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoListBoxControl());
}



UnoControlComboBoxModel::UnoControlComboBoxModel( const Reference< XComponentContext >& rxContext )
    :UnoControlListBoxModel( rxContext, ConstructWithoutProperties )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXComboBox>();
}

OUString UnoControlComboBoxModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlComboBoxModel"_ustr;
}

css::uno::Sequence<OUString> UnoControlComboBoxModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlComboBoxModel"_ustr, u"stardiv.vcl.controlmodel.ComboBox"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals);
}

uno::Reference< beans::XPropertySetInfo > UnoControlComboBoxModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

::cppu::IPropertyArrayHelper& UnoControlComboBoxModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}


OUString UnoControlComboBoxModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.ComboBox"_ustr;
}

void UnoControlComboBoxModel::setFastPropertyValue_NoBroadcast( std::unique_lock<std::mutex>& rGuard, sal_Int32 nHandle, const uno::Any& rValue )
{
    UnoControlModel::setFastPropertyValue_NoBroadcast( rGuard, nHandle, rValue );

    if (nHandle != BASEPROPERTY_STRINGITEMLIST || m_xData->m_bSettingLegacyProperty)
        return;

    // synchronize the legacy StringItemList property with our list items
    Sequence< OUString > aStringItemList;
    Any aPropValue;
    getFastPropertyValue( rGuard, aPropValue, BASEPROPERTY_STRINGITEMLIST );
    OSL_VERIFY( aPropValue >>= aStringItemList );

    ::std::vector< ListItem > aItems( aStringItemList.getLength() );
    ::std::transform(
        std::cbegin(aStringItemList),
        std::cend(aStringItemList),
        aItems.begin(),
        CreateListItem()
    );
    m_xData->setAllItems( std::move(aItems) );

    // since an XItemListListener does not have a "all items modified" or some such method,
    // we simulate this by notifying removal of all items, followed by insertion of all new
    // items
    lang::EventObject aEvent;
    aEvent.Source = *this;
    m_aItemListListeners.notifyEach( rGuard, &XItemListListener::itemListChanged, aEvent );
}

uno::Any UnoControlComboBoxModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.ComboBox"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlComboBoxModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlComboBoxModel(context));
}



UnoComboBoxControl::UnoComboBoxControl()
    :maActionListeners( *this )
    ,maItemListeners( *this )
{
    maComponentInfos.nWidth = 100;
    maComponentInfos.nHeight = 12;
}

OUString UnoComboBoxControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoComboBoxControl"_ustr;
}

css::uno::Sequence<OUString> UnoComboBoxControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlComboBox"_ustr, u"stardiv.vcl.control.ComboBox"_ustr };
    return comphelper::concatSequences( UnoEditControl::getSupportedServiceNames(), vals);
}

OUString UnoComboBoxControl::GetComponentServiceName() const
{
    return u"combobox"_ustr;
}

void UnoComboBoxControl::dispose()
{
    lang::EventObject aEvt;
    aEvt.Source = getXWeak();
    maActionListeners.disposeAndClear( aEvt );
    maItemListeners.disposeAndClear( aEvt );
    UnoControl::dispose();
}
uno::Any UnoComboBoxControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XComboBox* >(this) );
    if ( !aRet.hasValue() )
    {
        aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XItemListener* >(this) );
        if ( !aRet.hasValue() )
        {
            aRet = ::cppu::queryInterface( rType,
                                            static_cast< awt::XItemListListener* >(this) );
        }
    }
    return (aRet.hasValue() ? aRet : UnoEditControl::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoComboBoxControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoComboBoxControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<awt::XComboBox>::get(),
        cppu::UnoType<awt::XItemListener>::get(),
        cppu::UnoType<awt::XItemListListener>::get(),
        UnoEditControl::getTypes()
    );
    return aTypeList.getTypes();
}

void UnoComboBoxControl::updateFromModel()
{
    UnoEditControl::updateFromModel();

    Reference< XItemListListener > xItemListListener( getPeer(), UNO_QUERY );
    ENSURE_OR_RETURN_VOID( xItemListListener.is(), "UnoComboBoxControl::updateFromModel: a peer which is no ItemListListener?!" );

    EventObject aEvent( getModel() );
    xItemListListener->itemListChanged( aEvent );
}
void UnoComboBoxControl::ImplSetPeerProperty( const OUString& rPropName, const uno::Any& rVal )
{
    if ( rPropName == GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) )
        // do not forward this to our peer. We are a XItemListListener at our model, and changes in the string item
        // list (which is a legacy property) will, later, arrive as changes in the ItemList. Those latter changes
        // will be forwarded to the peer, which will update itself accordingly.
        return;

    UnoEditControl::ImplSetPeerProperty( rPropName, rVal );
}
void UnoComboBoxControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoEditControl::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XComboBox >  xComboBox( getPeer(), uno::UNO_QUERY );
    if ( maActionListeners.getLength() )
        xComboBox->addActionListener( &maActionListeners );
    if ( maItemListeners.getLength() )
        xComboBox->addItemListener( &maItemListeners );
}

void UnoComboBoxControl::addActionListener(const uno::Reference< awt::XActionListener > & l)
{
    maActionListeners.addInterface( l );
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XComboBox >  xComboBox( getPeer(), uno::UNO_QUERY );
        xComboBox->addActionListener( &maActionListeners );
    }
}

void UnoComboBoxControl::removeActionListener(const uno::Reference< awt::XActionListener > & l)
{
    if( getPeer().is() && maActionListeners.getLength() == 1 )
    {
        uno::Reference < awt::XComboBox >  xComboBox( getPeer(), uno::UNO_QUERY );
        xComboBox->removeActionListener( &maActionListeners );
    }
    maActionListeners.removeInterface( l );
}

void UnoComboBoxControl::addItemListener(const uno::Reference < awt::XItemListener > & l)
{
    maItemListeners.addInterface( l );
    if( getPeer().is() && maItemListeners.getLength() == 1 )
    {
        uno::Reference < awt::XComboBox >  xComboBox( getPeer(), uno::UNO_QUERY );
        xComboBox->addItemListener( &maItemListeners );
    }
}

void UnoComboBoxControl::removeItemListener(const uno::Reference < awt::XItemListener > & l)
{
    if( getPeer().is() && maItemListeners.getLength() == 1 )
    {
        // This call is prettier than creating a Ref and calling query
        uno::Reference < awt::XComboBox >  xComboBox( getPeer(), uno::UNO_QUERY );
        xComboBox->removeItemListener( &maItemListeners );
    }
    maItemListeners.removeInterface( l );
}
void UnoComboBoxControl::itemStateChanged( const awt::ItemEvent& rEvent )
{
    if ( maItemListeners.getLength() )
    {
        try
        {
            maItemListeners.itemStateChanged( rEvent );
        }
        catch( const Exception& )
        {
            TOOLS_WARN_EXCEPTION( "toolkit", "UnoComboBoxControl::itemStateChanged");
        }
    }
}
sal_Bool SAL_CALL UnoComboBoxControl::setModel( const uno::Reference< awt::XControlModel >& i_rModel )
{
    ::osl::MutexGuard aGuard( GetMutex() );

    const Reference< XItemList > xOldItems( getModel(), UNO_QUERY );
    OSL_ENSURE( xOldItems.is() || !getModel().is(), "UnoComboBoxControl::setModel: illegal old model!" );
    const Reference< XItemList > xNewItems( i_rModel, UNO_QUERY );
    OSL_ENSURE( xNewItems.is() || !i_rModel.is(), "UnoComboBoxControl::setModel: illegal new model!" );

    if ( !UnoEditControl::setModel( i_rModel ) )
        return false;

    if ( xOldItems.is() )
        xOldItems->removeItemListListener( this );
    if ( xNewItems.is() )
        xNewItems->addItemListListener( this );

    return true;
}

void SAL_CALL UnoComboBoxControl::listItemInserted( const awt::ItemListEvent& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoComboBoxControl::listItemInserted: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->listItemInserted( i_rEvent );
}

void SAL_CALL UnoComboBoxControl::listItemRemoved( const awt::ItemListEvent& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoComboBoxControl::listItemRemoved: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->listItemRemoved( i_rEvent );
}

void SAL_CALL UnoComboBoxControl::listItemModified( const awt::ItemListEvent& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoComboBoxControl::listItemModified: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->listItemModified( i_rEvent );
}

void SAL_CALL UnoComboBoxControl::allItemsRemoved( const lang::EventObject& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoComboBoxControl::allItemsRemoved: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->allItemsRemoved( i_rEvent );
}

void SAL_CALL UnoComboBoxControl::itemListChanged( const lang::EventObject& i_rEvent )
{
    const Reference< XItemListListener > xPeerListener( getPeer(), UNO_QUERY );
    OSL_ENSURE( xPeerListener.is() || !getPeer().is(), "UnoComboBoxControl::itemListChanged: invalid peer!" );
    if ( xPeerListener.is() )
        xPeerListener->itemListChanged( i_rEvent );
}

void UnoComboBoxControl::addItem( const OUString& aItem, sal_Int16 nPos )
{
    uno::Sequence<OUString> aSeq { aItem };
    addItems( aSeq, nPos );
}

void UnoComboBoxControl::addItems( const uno::Sequence< OUString>& aItems, sal_Int16 nPos )
{
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    sal_uInt16 nNewItems = static_cast<sal_uInt16>(aItems.getLength());
    sal_uInt16 nOldLen = static_cast<sal_uInt16>(aSeq.getLength());
    sal_uInt16 nNewLen = nOldLen + nNewItems;

    uno::Sequence< OUString> aNewSeq( nNewLen );

    if ( ( nPos < 0 ) || ( nPos > nOldLen ) )
        nPos = nOldLen;

    // items before the insert position
    auto it = std::copy(std::cbegin(aSeq), std::next(std::cbegin(aSeq), nPos), aNewSeq.getArray());

    // New items
    it = std::copy(aItems.begin(), aItems.end(), it);

    // remainder of old items
    std::copy(std::next(std::cbegin(aSeq), nPos), std::cend(aSeq), it);

    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ), Any(aNewSeq), true );
}

void UnoComboBoxControl::removeItems( sal_Int16 nPos, sal_Int16 nCount )
{
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    sal_uInt16 nOldLen = static_cast<sal_uInt16>(aSeq.getLength());
    if ( !nOldLen || ( nPos >= nOldLen ) )
        return;

    if ( nCount > ( nOldLen-nPos ) )
        nCount = nOldLen-nPos;

    sal_uInt16 nNewLen = nOldLen - nCount;

    uno::Sequence< OUString> aNewSeq( nNewLen );

    // items before the deletion position
    auto it = std::copy(std::cbegin(aSeq), std::next(std::cbegin(aSeq), nPos), aNewSeq.getArray());

    // remainder of old items
    std::copy(std::next(std::cbegin(aSeq), nPos + nCount), std::cend(aSeq), it);

    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ), uno::Any(aNewSeq), true );
}

sal_Int16 UnoComboBoxControl::getItemCount()
{
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    return static_cast<sal_Int16>(aSeq.getLength());
}

OUString UnoComboBoxControl::getItem( sal_Int16 nPos )
{
    OUString aItem;
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    if ( nPos < aSeq.getLength() )
        aItem = aSeq[nPos];
    return aItem;
}

uno::Sequence< OUString> UnoComboBoxControl::getItems()
{
    uno::Any aVal = ImplGetPropertyValue( GetPropertyName( BASEPROPERTY_STRINGITEMLIST ) );
    uno::Sequence< OUString> aSeq;
    aVal >>= aSeq;
    return aSeq;
}

void UnoComboBoxControl::setDropDownLineCount( sal_Int16 nLines )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_LINECOUNT ), uno::Any(nLines), true );
}

sal_Int16 UnoComboBoxControl::getDropDownLineCount()
{
    return ImplGetPropertyValue_INT16( BASEPROPERTY_LINECOUNT );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoComboBoxControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoComboBoxControl());
}


//  UnoSpinFieldControl

UnoSpinFieldControl::UnoSpinFieldControl()
    :maSpinListeners( *this )
{
    mbRepeat = false;
}

// uno::XInterface
uno::Any UnoSpinFieldControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XSpinField* >(this) );
    return (aRet.hasValue() ? aRet : UnoEditControl::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoSpinFieldControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoSpinFieldControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XSpinField>::get(),
        UnoEditControl::getTypes()
    );
    return aTypeList.getTypes();
}

void UnoSpinFieldControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoEditControl::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XSpinField > xField( getPeer(), uno::UNO_QUERY );
    xField->enableRepeat( mbRepeat );
    if ( maSpinListeners.getLength() )
        xField->addSpinListener( &maSpinListeners );
}

    // css::awt::XSpinField
void UnoSpinFieldControl::addSpinListener( const css::uno::Reference< css::awt::XSpinListener >& l )
{
    maSpinListeners.addInterface( l );
    if( getPeer().is() && maSpinListeners.getLength() == 1 )
    {
        uno::Reference < awt::XSpinField > xField( getPeer(), uno::UNO_QUERY );
        xField->addSpinListener( &maSpinListeners );
    }
}

void UnoSpinFieldControl::removeSpinListener( const css::uno::Reference< css::awt::XSpinListener >& l )
{
    if( getPeer().is() && maSpinListeners.getLength() == 1 )
    {
        uno::Reference < awt::XSpinField > xField( getPeer(), uno::UNO_QUERY );
        xField->removeSpinListener( &maSpinListeners );
    }
    maSpinListeners.removeInterface( l );
}

void UnoSpinFieldControl::up()
{
    uno::Reference < awt::XSpinField > xField( getPeer(), uno::UNO_QUERY );
    if ( xField.is() )
        xField->up();
}

void UnoSpinFieldControl::down()
{
    uno::Reference < awt::XSpinField > xField( getPeer(), uno::UNO_QUERY );
    if ( xField.is() )
        xField->down();
}

void UnoSpinFieldControl::first()
{
    uno::Reference < awt::XSpinField > xField( getPeer(), uno::UNO_QUERY );
    if ( xField.is() )
        xField->first();
}

void UnoSpinFieldControl::last()
{
    uno::Reference < awt::XSpinField > xField( getPeer(), uno::UNO_QUERY );
    if ( xField.is() )
        xField->last();
}

void UnoSpinFieldControl::enableRepeat( sal_Bool bRepeat )
{
    mbRepeat = bRepeat;

    uno::Reference < awt::XSpinField > xField( getPeer(), uno::UNO_QUERY );
    if ( xField.is() )
        xField->enableRepeat( bRepeat );
}



UnoControlDateFieldModel::UnoControlDateFieldModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXDateField>();
}

OUString UnoControlDateFieldModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.DateField"_ustr;
}

uno::Any UnoControlDateFieldModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.DateField"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}


::cppu::IPropertyArrayHelper& UnoControlDateFieldModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlDateFieldModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlDateFieldModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlDateFieldModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlDateFieldModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlDateFieldModel"_ustr, u"stardiv.vcl.controlmodel.DateField"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlDateFieldModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlDateFieldModel(context));
}



UnoDateFieldControl::UnoDateFieldControl()
{
    mnFirst = util::Date( 1, 1, 1900 );
    mnLast = util::Date( 31, 12, 2200 );
    mbLongFormat = TRISTATE_INDET;
}

OUString UnoDateFieldControl::GetComponentServiceName() const
{
    return u"datefield"_ustr;
}

// uno::XInterface
uno::Any UnoDateFieldControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XDateField* >(this) );
    return (aRet.hasValue() ? aRet : UnoSpinFieldControl::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoDateFieldControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoDateFieldControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XDateField>::get(),
        UnoSpinFieldControl::getTypes()
    );
    return aTypeList.getTypes();
}

void UnoDateFieldControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoSpinFieldControl::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XDateField > xField( getPeer(), uno::UNO_QUERY );
    xField->setFirst( mnFirst );
    xField->setLast( mnLast );
    if ( mbLongFormat != TRISTATE_INDET )
        xField->setLongFormat( mbLongFormat != TRISTATE_FALSE);
}


void UnoDateFieldControl::textChanged( const awt::TextEvent& e )
{
    uno::Reference< awt::XVclWindowPeer > xPeer( getPeer(), uno::UNO_QUERY );

    // also change the text property (#i25106#)
    if ( xPeer.is() )
    {
        const OUString& sTextPropertyName = GetPropertyName( BASEPROPERTY_TEXT );
        ImplSetPropertyValue( sTextPropertyName, xPeer->getProperty( sTextPropertyName ), false );
    }

    // re-calc the Date property
    uno::Reference < awt::XDateField > xField( getPeer(), uno::UNO_QUERY );
    uno::Any aValue;
    if ( xField->isEmpty() )
    {
        // the field says it's empty
        bool bEnforceFormat = true;
        if ( xPeer.is() )
            xPeer->getProperty( GetPropertyName( BASEPROPERTY_ENFORCE_FORMAT ) ) >>= bEnforceFormat;
        if ( !bEnforceFormat )
        {
            // and it also says that it is currently accepting invalid inputs, without
            // forcing it to a valid date
            uno::Reference< awt::XTextComponent > xText( xPeer, uno::UNO_QUERY );
            if ( xText.is() && xText->getText().getLength() )
                // and in real, the text of the peer is *not* empty
                // -> simulate an invalid date, which is different from "no date"
                aValue <<= util::Date();
        }
    }
    else
        aValue <<= xField->getDate();

    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_DATE ), aValue, false );

    // multiplex the event
    if ( GetTextListeners().getLength() )
        GetTextListeners().textChanged( e );
}

void UnoDateFieldControl::setDate( const util::Date& Date )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_DATE ), uno::Any(Date), true );
}

util::Date UnoDateFieldControl::getDate()
{
    return ImplGetPropertyValue_Date( BASEPROPERTY_DATE );
}

void UnoDateFieldControl::setMin( const util::Date& Date )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_DATEMIN ), uno::Any(Date), true );
}

util::Date UnoDateFieldControl::getMin()
{
    return ImplGetPropertyValue_Date( BASEPROPERTY_DATEMIN );
}

void UnoDateFieldControl::setMax( const util::Date& Date )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_DATEMAX ), uno::Any(Date), true );
}

util::Date UnoDateFieldControl::getMax()
{
    return ImplGetPropertyValue_Date( BASEPROPERTY_DATEMAX );
}

void UnoDateFieldControl::setFirst( const util::Date& Date )
{
    mnFirst = Date;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XDateField > xField( getPeer(), uno::UNO_QUERY );
        xField->setFirst( Date );
    }
}

util::Date UnoDateFieldControl::getFirst()
{
    return mnFirst;
}

void UnoDateFieldControl::setLast( const util::Date& Date )
{
    mnLast = Date;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XDateField > xField( getPeer(), uno::UNO_QUERY );
        xField->setLast( Date );
    }
}

util::Date UnoDateFieldControl::getLast()
{
    return mnLast;
}

void UnoDateFieldControl::setLongFormat( sal_Bool bLong )
{
    mbLongFormat = bLong ? TRISTATE_TRUE : TRISTATE_FALSE;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XDateField > xField( getPeer(), uno::UNO_QUERY );
        xField->setLongFormat( bLong );
    }
}

sal_Bool UnoDateFieldControl::isLongFormat()
{
    return mbLongFormat == TRISTATE_TRUE;
}

void UnoDateFieldControl::setEmpty()
{
    if ( getPeer().is() )
    {
        uno::Reference < awt::XDateField >  xField( getPeer(), uno::UNO_QUERY );
        xField->setEmpty();
    }
}

sal_Bool UnoDateFieldControl::isEmpty()
{
    bool bEmpty = false;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XDateField > xField( getPeer(), uno::UNO_QUERY );
        bEmpty = xField->isEmpty();
    }
    return bEmpty;
}

void UnoDateFieldControl::setStrictFormat( sal_Bool bStrict )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRICTFORMAT ), uno::Any(bStrict), true );
}

sal_Bool UnoDateFieldControl::isStrictFormat()
{
    return ImplGetPropertyValue_BOOL( BASEPROPERTY_STRICTFORMAT );
}

OUString UnoDateFieldControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoDateFieldControl"_ustr;
}

css::uno::Sequence<OUString> UnoDateFieldControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlDateField"_ustr, u"stardiv.vcl.control.DateField"_ustr };
    return comphelper::concatSequences( UnoSpinFieldControl::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoDateFieldControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoDateFieldControl());
}



UnoControlTimeFieldModel::UnoControlTimeFieldModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXTimeField>();
}

OUString UnoControlTimeFieldModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.TimeField"_ustr;
}

uno::Any UnoControlTimeFieldModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.TimeField"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}


::cppu::IPropertyArrayHelper& UnoControlTimeFieldModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlTimeFieldModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlTimeFieldModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlTimeFieldModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlTimeFieldModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlTimeFieldModel"_ustr, u"stardiv.vcl.controlmodel.TimeField"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlTimeFieldModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlTimeFieldModel(context));
}



UnoTimeFieldControl::UnoTimeFieldControl()
{
    mnFirst = util::Time( 0, 0, 0, 0, false );
    mnLast = util::Time( 999999999, 59, 59, 23, false );
}

OUString UnoTimeFieldControl::GetComponentServiceName() const
{
    return u"timefield"_ustr;
}

// uno::XInterface
uno::Any UnoTimeFieldControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XTimeField* >(this) );
    return (aRet.hasValue() ? aRet : UnoSpinFieldControl::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoTimeFieldControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoTimeFieldControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XTimeField>::get(),
        UnoSpinFieldControl::getTypes()
    );
    return aTypeList.getTypes();
}

void UnoTimeFieldControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoSpinFieldControl::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XTimeField > xField( getPeer(), uno::UNO_QUERY );
    xField->setFirst( mnFirst );
    xField->setLast( mnLast );
}

void UnoTimeFieldControl::textChanged( const awt::TextEvent& e )
{
    // also change the text property (#i25106#)
    uno::Reference< awt::XVclWindowPeer > xPeer( getPeer(), uno::UNO_QUERY );
    const OUString& sTextPropertyName = GetPropertyName( BASEPROPERTY_TEXT );
    ImplSetPropertyValue( sTextPropertyName, xPeer->getProperty( sTextPropertyName ), false );

    // re-calc the Time property
    uno::Reference < awt::XTimeField >  xField( getPeer(), uno::UNO_QUERY );
    uno::Any aValue;
    if ( !xField->isEmpty() )
        aValue <<= xField->getTime();
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_TIME ), aValue, false );

    // multiplex the event
    if ( GetTextListeners().getLength() )
        GetTextListeners().textChanged( e );
}

void UnoTimeFieldControl::setTime( const util::Time& Time )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_TIME ), Any(Time), true );
}

util::Time UnoTimeFieldControl::getTime()
{
    return ImplGetPropertyValue_Time( BASEPROPERTY_TIME );
}

void UnoTimeFieldControl::setMin( const util::Time& Time )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_TIMEMIN ), uno::Any(Time), true );
}

util::Time UnoTimeFieldControl::getMin()
{
    return ImplGetPropertyValue_Time( BASEPROPERTY_TIMEMIN );
}

void UnoTimeFieldControl::setMax( const util::Time& Time )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_TIMEMAX ), uno::Any(Time), true );
}

util::Time UnoTimeFieldControl::getMax()
{
    return ImplGetPropertyValue_Time( BASEPROPERTY_TIMEMAX );
}

void UnoTimeFieldControl::setFirst( const util::Time& Time )
{
    mnFirst = Time;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XTimeField > xField( getPeer(), uno::UNO_QUERY );
        xField->setFirst( mnFirst );
    }
}

util::Time UnoTimeFieldControl::getFirst()
{
    return mnFirst;
}

void UnoTimeFieldControl::setLast( const util::Time& Time )
{
    mnLast = Time;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XTimeField > xField( getPeer(), uno::UNO_QUERY );
        xField->setFirst( mnLast );
    }
}

util::Time UnoTimeFieldControl::getLast()
{
    return mnLast;
}

void UnoTimeFieldControl::setEmpty()
{
    if ( getPeer().is() )
    {
        uno::Reference < awt::XTimeField >  xField( getPeer(), uno::UNO_QUERY );
        xField->setEmpty();
    }
}

sal_Bool UnoTimeFieldControl::isEmpty()
{
    bool bEmpty = false;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XTimeField >  xField( getPeer(), uno::UNO_QUERY );
        bEmpty = xField->isEmpty();
    }
    return bEmpty;
}

void UnoTimeFieldControl::setStrictFormat( sal_Bool bStrict )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRICTFORMAT ), uno::Any(bStrict), true );
}

sal_Bool UnoTimeFieldControl::isStrictFormat()
{
    return ImplGetPropertyValue_BOOL( BASEPROPERTY_STRICTFORMAT );
}

OUString UnoTimeFieldControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoTimeFieldControl"_ustr;
}

css::uno::Sequence<OUString> UnoTimeFieldControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlTimeField"_ustr, u"stardiv.vcl.control.TimeField"_ustr };
    return comphelper::concatSequences( UnoSpinFieldControl::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoTimeFieldControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoTimeFieldControl());
}



UnoControlNumericFieldModel::UnoControlNumericFieldModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXNumericField>();
}

OUString UnoControlNumericFieldModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.NumericField"_ustr;
}

uno::Any UnoControlNumericFieldModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.NumericField"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}


::cppu::IPropertyArrayHelper& UnoControlNumericFieldModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlNumericFieldModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlNumericFieldModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlNumericFieldModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlNumericFieldModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"stardiv.vcl.controlmodel.NumericField"_ustr, u"com.sun.star.awt.UnoControlNumericFieldModel"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlNumericFieldModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlNumericFieldModel(context));
}



UnoNumericFieldControl::UnoNumericFieldControl()
{
    mnFirst = 0;
    mnLast = 0x7FFFFFFF;
}

OUString UnoNumericFieldControl::GetComponentServiceName() const
{
    return u"numericfield"_ustr;
}

// uno::XInterface
uno::Any UnoNumericFieldControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XNumericField* >(this) );
    return (aRet.hasValue() ? aRet : UnoSpinFieldControl::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoNumericFieldControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoNumericFieldControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XNumericField>::get(),
        UnoSpinFieldControl::getTypes()
    );
    return aTypeList.getTypes();
}

void UnoNumericFieldControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoSpinFieldControl::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XNumericField > xField( getPeer(), uno::UNO_QUERY );
    xField->setFirst( mnFirst );
    xField->setLast( mnLast );
}


void UnoNumericFieldControl::textChanged( const awt::TextEvent& e )
{
    uno::Reference < awt::XNumericField >  xField( getPeer(), uno::UNO_QUERY );
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUE_DOUBLE ), uno::Any(xField->getValue()), false );

    if ( GetTextListeners().getLength() )
        GetTextListeners().textChanged( e );
}

void UnoNumericFieldControl::setValue( double Value )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUE_DOUBLE ), uno::Any(Value), true );
}

double UnoNumericFieldControl::getValue()
{
    return ImplGetPropertyValue_DOUBLE( BASEPROPERTY_VALUE_DOUBLE );
}

void UnoNumericFieldControl::setMin( double Value )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUEMIN_DOUBLE ), uno::Any(Value), true );
}

double UnoNumericFieldControl::getMin()
{
    return ImplGetPropertyValue_DOUBLE( BASEPROPERTY_VALUEMIN_DOUBLE );
}

void UnoNumericFieldControl::setMax( double Value )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUEMAX_DOUBLE ), uno::Any(Value), true );
}

double UnoNumericFieldControl::getMax()
{
    return ImplGetPropertyValue_DOUBLE( BASEPROPERTY_VALUEMAX_DOUBLE );
}

void UnoNumericFieldControl::setFirst( double Value )
{
    mnFirst = Value;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XNumericField > xField( getPeer(), uno::UNO_QUERY );
        xField->setFirst( mnFirst );
    }
}

double UnoNumericFieldControl::getFirst()
{
    return mnFirst;
}

void UnoNumericFieldControl::setLast( double Value )
{
    mnLast = Value;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XNumericField > xField( getPeer(), uno::UNO_QUERY );
        xField->setLast( mnLast );
    }
}

double UnoNumericFieldControl::getLast()
{
    return mnLast;
}

void UnoNumericFieldControl::setStrictFormat( sal_Bool bStrict )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRICTFORMAT ), uno::Any(bStrict), true );
}

sal_Bool UnoNumericFieldControl::isStrictFormat()
{
    return ImplGetPropertyValue_BOOL( BASEPROPERTY_STRICTFORMAT );
}

OUString UnoNumericFieldControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoNumericFieldControl"_ustr;
}

css::uno::Sequence<OUString> UnoNumericFieldControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlNumericField"_ustr, u"stardiv.vcl.control.NumericField"_ustr };
    return comphelper::concatSequences( UnoSpinFieldControl::getSupportedServiceNames(), vals );
}

void UnoNumericFieldControl::setSpinSize( double Digits )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUESTEP_DOUBLE ), uno::Any(Digits), true );
}

double UnoNumericFieldControl::getSpinSize()
{
    return ImplGetPropertyValue_DOUBLE( BASEPROPERTY_VALUESTEP_DOUBLE );
}

void UnoNumericFieldControl::setDecimalDigits( sal_Int16 Digits )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_DECIMALACCURACY ), uno::Any(Digits), true );
}

sal_Int16 UnoNumericFieldControl::getDecimalDigits()
{
    return ImplGetPropertyValue_INT16( BASEPROPERTY_DECIMALACCURACY );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoNumericFieldControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoNumericFieldControl());
}

UnoControlCurrencyFieldModel::UnoControlCurrencyFieldModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<SVTXCurrencyField>();
}

OUString UnoControlCurrencyFieldModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.CurrencyField"_ustr;
}

uno::Any UnoControlCurrencyFieldModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.CurrencyField"_ustr );
    }
    if ( nPropId == BASEPROPERTY_CURSYM_POSITION )
    {
        return uno::Any(false);
    }

    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlCurrencyFieldModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlCurrencyFieldModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlCurrencyFieldModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlCurrencyFieldModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlCurrencyFieldModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlCurrencyFieldModel"_ustr, u"stardiv.vcl.controlmodel.CurrencyField"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlCurrencyFieldModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlCurrencyFieldModel(context));
}



UnoCurrencyFieldControl::UnoCurrencyFieldControl()
{
    mnFirst = 0;
    mnLast = 0x7FFFFFFF;
}

OUString UnoCurrencyFieldControl::GetComponentServiceName() const
{
    return u"longcurrencyfield"_ustr;
}

// uno::XInterface
uno::Any UnoCurrencyFieldControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XCurrencyField* >(this) );
    return (aRet.hasValue() ? aRet : UnoSpinFieldControl::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoCurrencyFieldControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoCurrencyFieldControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XCurrencyField>::get(),
        UnoSpinFieldControl::getTypes()
    );
    return aTypeList.getTypes();
}

void UnoCurrencyFieldControl::createPeer( const uno::Reference< awt::XToolkit > & rxToolkit, const uno::Reference< awt::XWindowPeer >  & rParentPeer )
{
    UnoSpinFieldControl::createPeer( rxToolkit, rParentPeer );

    uno::Reference < awt::XCurrencyField > xField( getPeer(), uno::UNO_QUERY );
    xField->setFirst( mnFirst );
    xField->setLast( mnLast );
}

void UnoCurrencyFieldControl::textChanged( const awt::TextEvent& e )
{
    uno::Reference < awt::XCurrencyField >  xField( getPeer(), uno::UNO_QUERY );
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUE_DOUBLE ), uno::Any(xField->getValue()), false );

    if ( GetTextListeners().getLength() )
        GetTextListeners().textChanged( e );
}

void UnoCurrencyFieldControl::setValue( double Value )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUE_DOUBLE ), Any(Value), true );
}

double UnoCurrencyFieldControl::getValue()
{
    return ImplGetPropertyValue_DOUBLE( BASEPROPERTY_VALUE_DOUBLE );
}

void UnoCurrencyFieldControl::setMin( double Value )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUEMIN_DOUBLE ), uno::Any(Value), true );
}

double UnoCurrencyFieldControl::getMin()
{
    return ImplGetPropertyValue_DOUBLE( BASEPROPERTY_VALUEMIN_DOUBLE );
}

void UnoCurrencyFieldControl::setMax( double Value )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUEMAX_DOUBLE ), uno::Any(Value), true );
}

double UnoCurrencyFieldControl::getMax()
{
    return ImplGetPropertyValue_DOUBLE( BASEPROPERTY_VALUEMAX_DOUBLE );
}

void UnoCurrencyFieldControl::setFirst( double Value )
{
    mnFirst = Value;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XCurrencyField > xField( getPeer(), uno::UNO_QUERY );
        xField->setFirst( mnFirst );
    }
}

double UnoCurrencyFieldControl::getFirst()
{
    return mnFirst;
}

void UnoCurrencyFieldControl::setLast( double Value )
{
    mnLast = Value;
    if ( getPeer().is() )
    {
        uno::Reference < awt::XCurrencyField > xField( getPeer(), uno::UNO_QUERY );
        xField->setLast( mnLast );
    }
}

double UnoCurrencyFieldControl::getLast()
{
    return mnLast;
}

void UnoCurrencyFieldControl::setStrictFormat( sal_Bool bStrict )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRICTFORMAT ), uno::Any(bStrict), true );
}

sal_Bool UnoCurrencyFieldControl::isStrictFormat()
{
    return ImplGetPropertyValue_BOOL( BASEPROPERTY_STRICTFORMAT );
}

OUString UnoCurrencyFieldControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoCurrencyFieldControl"_ustr;
}

css::uno::Sequence<OUString>
UnoCurrencyFieldControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlCurrencyField"_ustr, u"stardiv.vcl.control.CurrencyField"_ustr };
    return comphelper::concatSequences( UnoSpinFieldControl::getSupportedServiceNames(), vals );
}

void UnoCurrencyFieldControl::setSpinSize( double Digits )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_VALUESTEP_DOUBLE ), uno::Any(Digits), true );
}

double UnoCurrencyFieldControl::getSpinSize()
{
    return ImplGetPropertyValue_DOUBLE( BASEPROPERTY_VALUESTEP_DOUBLE );
}

void UnoCurrencyFieldControl::setDecimalDigits( sal_Int16 Digits )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_DECIMALACCURACY ), uno::Any(Digits), true );
}

sal_Int16 UnoCurrencyFieldControl::getDecimalDigits()
{
    return ImplGetPropertyValue_INT16( BASEPROPERTY_DECIMALACCURACY );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoCurrencyFieldControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoCurrencyFieldControl());
}



UnoControlPatternFieldModel::UnoControlPatternFieldModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    UNO_CONTROL_MODEL_REGISTER_PROPERTIES<VCLXPatternField>();
}

OUString UnoControlPatternFieldModel::getServiceName()
{
    return u"stardiv.vcl.controlmodel.PatternField"_ustr;
}

uno::Any UnoControlPatternFieldModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.PatternField"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlPatternFieldModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlPatternFieldModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlPatternFieldModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlPatternFieldModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlPatternFieldModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlPatternFieldModel"_ustr, u"stardiv.vcl.controlmodel.PatternField"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlPatternFieldModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlPatternFieldModel(context));
}



UnoPatternFieldControl::UnoPatternFieldControl()
{
}

OUString UnoPatternFieldControl::GetComponentServiceName() const
{
    return u"patternfield"_ustr;
}

void UnoPatternFieldControl::ImplSetPeerProperty( const OUString& rPropName, const uno::Any& rVal )
{
    sal_uInt16 nType = GetPropertyId( rPropName );
    if ( ( nType == BASEPROPERTY_TEXT ) || ( nType == BASEPROPERTY_EDITMASK ) || ( nType == BASEPROPERTY_LITERALMASK ) )
    {
        // These masks cannot be set consecutively
        OUString Text = ImplGetPropertyValue_UString( BASEPROPERTY_TEXT );
        OUString EditMask = ImplGetPropertyValue_UString( BASEPROPERTY_EDITMASK );
        OUString LiteralMask = ImplGetPropertyValue_UString( BASEPROPERTY_LITERALMASK );

        uno::Reference < awt::XPatternField >  xPF( getPeer(), uno::UNO_QUERY );
        if (xPF.is())
        {
            // same comment as in UnoControl::ImplSetPeerProperty - see there
            ImplCheckLocalize( Text );
            xPF->setString( Text );
            xPF->setMasks( EditMask, LiteralMask );
        }
    }
    else
        UnoSpinFieldControl::ImplSetPeerProperty( rPropName, rVal );
}


// uno::XInterface
uno::Any UnoPatternFieldControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XPatternField* >(this) );
    return (aRet.hasValue() ? aRet : UnoSpinFieldControl::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoPatternFieldControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoPatternFieldControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XPatternField>::get(),
        UnoSpinFieldControl::getTypes()
    );
    return aTypeList.getTypes();
}

void UnoPatternFieldControl::setString( const OUString& rString )
{
    setText( rString );
}

OUString UnoPatternFieldControl::getString()
{
    return getText();
}

void UnoPatternFieldControl::setMasks( const OUString& EditMask, const OUString& LiteralMask )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_EDITMASK ), uno::Any(EditMask), true );
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_LITERALMASK ), uno::Any(LiteralMask), true );
}

void UnoPatternFieldControl::getMasks( OUString& EditMask, OUString& LiteralMask )
{
    EditMask = ImplGetPropertyValue_UString( BASEPROPERTY_EDITMASK );
    LiteralMask = ImplGetPropertyValue_UString( BASEPROPERTY_LITERALMASK );
}

void UnoPatternFieldControl::setStrictFormat( sal_Bool bStrict )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_STRICTFORMAT ), uno::Any(bStrict), true );
}

sal_Bool UnoPatternFieldControl::isStrictFormat()
{
    return ImplGetPropertyValue_BOOL( BASEPROPERTY_STRICTFORMAT );
}

OUString UnoPatternFieldControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoPatternFieldControl"_ustr;
}

css::uno::Sequence<OUString> UnoPatternFieldControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlPatternField"_ustr, u"stardiv.vcl.control.PatternField"_ustr };
    return comphelper::concatSequences( UnoSpinFieldControl::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoPatternFieldControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoPatternFieldControl());
}



UnoControlProgressBarModel::UnoControlProgressBarModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    ImplRegisterProperty( BASEPROPERTY_BACKGROUNDCOLOR );
    ImplRegisterProperty( BASEPROPERTY_BORDER );
    ImplRegisterProperty( BASEPROPERTY_BORDERCOLOR );
    ImplRegisterProperty( BASEPROPERTY_DEFAULTCONTROL );
    ImplRegisterProperty( BASEPROPERTY_ENABLED );
    ImplRegisterProperty( BASEPROPERTY_ENABLEVISIBLE );
    ImplRegisterProperty( BASEPROPERTY_FILLCOLOR );
    ImplRegisterProperty( BASEPROPERTY_HELPTEXT );
    ImplRegisterProperty( BASEPROPERTY_HELPURL );
    ImplRegisterProperty( BASEPROPERTY_PRINTABLE );
    ImplRegisterProperty( BASEPROPERTY_PROGRESSVALUE );
    ImplRegisterProperty( BASEPROPERTY_PROGRESSVALUE_MAX );
    ImplRegisterProperty( BASEPROPERTY_PROGRESSVALUE_MIN );
}

OUString UnoControlProgressBarModel::getServiceName( )
{
    return u"stardiv.vcl.controlmodel.ProgressBar"_ustr;
}

uno::Any UnoControlProgressBarModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.ProgressBar"_ustr );
    }

    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlProgressBarModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlProgressBarModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlProgressBarModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlProgressBarModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlProgressBarModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlProgressBarModel"_ustr, u"stardiv.vcl.controlmodel.ProgressBar"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlProgressBarModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlProgressBarModel(context));
}



UnoProgressBarControl::UnoProgressBarControl()
{
}

OUString UnoProgressBarControl::GetComponentServiceName() const
{
    return u"ProgressBar"_ustr;
}

// uno::XInterface
uno::Any UnoProgressBarControl::queryAggregation( const uno::Type & rType )
{
    uno::Any aRet = ::cppu::queryInterface( rType,
                                        static_cast< awt::XProgressBar* >(this) );
    return (aRet.hasValue() ? aRet : UnoControlBase::queryAggregation( rType ));
}

IMPL_IMPLEMENTATION_ID( UnoProgressBarControl )

// lang::XTypeProvider
css::uno::Sequence< css::uno::Type > UnoProgressBarControl::getTypes()
{
    static const ::cppu::OTypeCollection aTypeList(
        cppu::UnoType<css::lang::XTypeProvider>::get(),
        cppu::UnoType<awt::XProgressBar>::get(),
        UnoControlBase::getTypes()
    );
    return aTypeList.getTypes();
}

// css::awt::XProgressBar
void UnoProgressBarControl::setForegroundColor( sal_Int32 nColor )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_FILLCOLOR ), uno::Any(nColor), true );
}

void UnoProgressBarControl::setBackgroundColor( sal_Int32 nColor )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_BACKGROUNDCOLOR ), uno::Any(nColor), true );
}

void UnoProgressBarControl::setValue( sal_Int32 nValue )
{
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_PROGRESSVALUE ), uno::Any(nValue), true );
}

void UnoProgressBarControl::setRange( sal_Int32 nMin, sal_Int32 nMax )
{
    uno::Any aMin;
    uno::Any aMax;

    if ( nMin < nMax )
    {
        // take correct min and max
        aMin <<= nMin;
        aMax <<= nMax;
    }
    else
    {
        // change min and max
        aMin <<= nMax;
        aMax <<= nMin;
    }

    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_PROGRESSVALUE_MIN ), aMin, true );
    ImplSetPropertyValue( GetPropertyName( BASEPROPERTY_PROGRESSVALUE_MAX ), aMax, true );
}

sal_Int32 UnoProgressBarControl::getValue()
{
    return ImplGetPropertyValue_INT32( BASEPROPERTY_PROGRESSVALUE );
}

OUString UnoProgressBarControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoProgressBarControl"_ustr;
}

css::uno::Sequence<OUString> UnoProgressBarControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlProgressBar"_ustr, u"stardiv.vcl.control.ProgressBar"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoProgressBarControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoProgressBarControl());
}



UnoControlFixedLineModel::UnoControlFixedLineModel( const Reference< XComponentContext >& rxContext )
    :UnoControlModel( rxContext )
{
    ImplRegisterProperty( BASEPROPERTY_BACKGROUNDCOLOR );
    ImplRegisterProperty( BASEPROPERTY_DEFAULTCONTROL );
    ImplRegisterProperty( BASEPROPERTY_ENABLED );
    ImplRegisterProperty( BASEPROPERTY_ENABLEVISIBLE );
    ImplRegisterProperty( BASEPROPERTY_FONTDESCRIPTOR );
    ImplRegisterProperty( BASEPROPERTY_HELPTEXT );
    ImplRegisterProperty( BASEPROPERTY_HELPURL );
    ImplRegisterProperty( BASEPROPERTY_LABEL );
    ImplRegisterProperty( BASEPROPERTY_ORIENTATION );
    ImplRegisterProperty( BASEPROPERTY_PRINTABLE );
}

OUString UnoControlFixedLineModel::getServiceName( )
{
    return u"stardiv.vcl.controlmodel.FixedLine"_ustr;
}

uno::Any UnoControlFixedLineModel::ImplGetDefaultValue( sal_uInt16 nPropId ) const
{
    if ( nPropId == BASEPROPERTY_DEFAULTCONTROL )
    {
        return uno::Any( u"stardiv.vcl.control.FixedLine"_ustr );
    }
    return UnoControlModel::ImplGetDefaultValue( nPropId );
}

::cppu::IPropertyArrayHelper& UnoControlFixedLineModel::getInfoHelper()
{
    static UnoPropertyArrayHelper aHelper( ImplGetPropertyIds() );
    return aHelper;
}

// beans::XMultiPropertySet
uno::Reference< beans::XPropertySetInfo > UnoControlFixedLineModel::getPropertySetInfo(  )
{
    static uno::Reference< beans::XPropertySetInfo > xInfo( createPropertySetInfo( getInfoHelper() ) );
    return xInfo;
}

OUString UnoControlFixedLineModel::getImplementationName()
{
    return u"stardiv.Toolkit.UnoControlFixedLineModel"_ustr;
}

css::uno::Sequence<OUString>
UnoControlFixedLineModel::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlFixedLineModel"_ustr, u"stardiv.vcl.controlmodel.FixedLine"_ustr };
    return comphelper::concatSequences( UnoControlModel::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoControlFixedLineModel_get_implementation(
    css::uno::XComponentContext *context,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoControlFixedLineModel(context));
}



UnoFixedLineControl::UnoFixedLineControl()
{
    maComponentInfos.nWidth = 100;      // ??
    maComponentInfos.nHeight = 100;     // ??
}

OUString UnoFixedLineControl::GetComponentServiceName() const
{
    return u"FixedLine"_ustr;
}

sal_Bool UnoFixedLineControl::isTransparent()
{
    return true;
}

OUString UnoFixedLineControl::getImplementationName()
{
    return u"stardiv.Toolkit.UnoFixedLineControl"_ustr;
}

css::uno::Sequence<OUString> UnoFixedLineControl::getSupportedServiceNames()
{
    const css::uno::Sequence<OUString> vals { u"com.sun.star.awt.UnoControlFixedLine"_ustr, u"stardiv.vcl.control.FixedLine"_ustr };
    return comphelper::concatSequences( UnoControlBase::getSupportedServiceNames(), vals );
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface *
stardiv_Toolkit_UnoFixedLineControl_get_implementation(
    css::uno::XComponentContext *,
    css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new UnoFixedLineControl());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
