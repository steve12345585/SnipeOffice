/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column:100 -*- */
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

#include <com/sun/star/util/URLTransformer.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XController.hpp>
#include <com/sun/star/frame/XModel2.hpp>
#include <com/sun/star/script/BasicErrorException.hpp>
#include <com/sun/star/script/XDefaultProperty.hpp>
#include <com/sun/star/script/XInvocation.hpp>
#include <com/sun/star/script/Converter.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/theIntrospection.hpp>
#include <com/sun/star/util/MeasureUnit.hpp>
#include <com/sun/star/awt/XControl.hpp>
#include <com/sun/star/awt/XWindow.hpp>
#include <com/sun/star/awt/XDialog.hpp>
#include <com/sun/star/awt/XUnitConversion.hpp>
#include <com/sun/star/drawing/XShape.hpp>
#include <ooo/vba/XHelperInterface.hpp>

#include <comphelper/automationinvokedzone.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/servicehelper.hxx>

#include <sfx2/objsh.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/app.hxx>
#include <sfx2/sfxsids.hrc>
#include <svl/stritem.hxx>
#include <svl/eitem.hxx>
#include <svl/intitem.hxx>
#include <svl/itemset.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/viewfac.hxx>

#include <basic/sbstar.hxx>
#include <basic/basmgr.hxx>
#include <basic/sbxmeth.hxx>
#include <basic/sbmod.hxx>
#include <basic/sbuno.hxx>
#include <basic/sberrors.hxx>
#include <o3tl/unit_conversion.hxx>
#include <rtl/ustrbuf.hxx>
#include <sfx2/viewsh.hxx>
#include <sal/log.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <utility>
#include <vcl/svapp.hxx>
#include <vcl/window.hxx>
#include <vcl/syswin.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/UnitConversion.hxx>
#include <vbahelper/vbahelper.hxx>

using namespace ::com::sun::star;
using namespace ::ooo::vba;


namespace ooo::vba
{

// helper method to determine if the view ( calc ) is in print-preview mode
static bool isInPrintPreview( SfxViewFrame* pView )
{
    sal_uInt16 nViewNo = SID_VIEWSHELL1 - SID_VIEWSHELL0;
    if ( pView->GetObjectShell()->GetFactory().GetViewFactoryCount() >
nViewNo && !pView->GetObjectShell()->IsInPlaceActive() )
    {
        SfxViewFactory &rViewFactory =
            pView->GetObjectShell()->GetFactory().GetViewFactory(nViewNo);
        if (  pView->GetCurViewId() == rViewFactory.GetOrdinal() )
            return true;
    }
    return false;
}

uno::Reference< beans::XIntrospectionAccess >
getIntrospectionAccess( const uno::Any& aObject )
{
    static uno::Reference< beans::XIntrospection > xIntrospection( beans::theIntrospection::get( comphelper::getProcessComponentContext() ) );
    return xIntrospection->inspect( aObject );
}

uno::Reference< script::XTypeConverter > const &
getTypeConverter( const uno::Reference< uno::XComponentContext >& xContext )
{
    static uno::Reference< script::XTypeConverter > xTypeConv( script::Converter::create(xContext) );
    return xTypeConv;
}
const uno::Any&
aNULL()
{
    static  uno::Any aNULLL{ uno::Reference< uno::XInterface >() };
    return aNULLL;
}

void dispatchExecute(SfxViewShell const * pViewShell, sal_uInt16 nSlot)
{
    SfxViewFrame* pViewFrame = nullptr;
    if ( pViewShell )
        pViewFrame = &pViewShell->GetViewFrame();
    if ( pViewFrame )
    {
        SfxDispatcher* pDispatcher = pViewFrame->GetDispatcher();
        if( pDispatcher )
        {
            pDispatcher->Execute( nSlot , SfxCallMode::SYNCHRON );
        }
    }
}

void
dispatchRequests (const uno::Reference< frame::XModel>& xModel, const OUString & aUrl, const uno::Sequence< beans::PropertyValue >& sProps )
{
    util::URL url;
    url.Complete = aUrl;
    uno::Reference<frame::XController> xController = xModel->getCurrentController();
    uno::Reference<frame::XFrame> xFrame = xController->getFrame();
    uno::Reference<frame::XDispatchProvider> xDispatchProvider (xFrame,uno::UNO_QUERY_THROW);
    try
    {
        const uno::Reference<uno::XComponentContext >& xContext(
            comphelper::getProcessComponentContext() );
        uno::Reference<util::XURLTransformer> xParser( util::URLTransformer::create(xContext) );
        xParser->parseStrict (url);
    }
    catch (const uno::Exception&)
    {
        return;
    }

    uno::Reference<frame::XDispatch> xDispatcher = xDispatchProvider->queryDispatch(url,u""_ustr,0);

    sal_Int32 nProps = sProps.getLength();
    uno::Sequence<beans::PropertyValue> dispatchProps(nProps + 1);

    if ( nProps )
    {
        std::copy(sProps.begin(), sProps.end(), dispatchProps.getArray());
    }

    if ( xDispatcher.is() )
    {
        xDispatcher->dispatch( url, dispatchProps );
    }
}

void
dispatchRequests( const uno::Reference< frame::XModel>& xModel, const OUString& aUrl )
{
    uno::Sequence<beans::PropertyValue> dispatchProps;
    dispatchRequests( xModel, aUrl, dispatchProps );
}

uno::Reference< frame::XModel >
getCurrentDoc( const OUString& sKey )
{
    uno::Reference< frame::XModel > xModel;
    StarBASIC* pBasic = SfxApplication::GetBasic();
    if (pBasic == nullptr)
    {
        SAL_INFO("vbahelper", "getModelFromBasic() StarBASIC* is NULL" );
        return xModel;
    }
    SbxObject* basicChosen = pBasic;
    SbxObject* pParent = pBasic->GetParent();
    SbxObject* pParentParent = pParent ? pParent->GetParent() : nullptr;

    if( pParentParent )
    {
        basicChosen = pParentParent;
    }
    else if( pParent )
    {
        basicChosen = pParent;
    }


    uno::Any aModel;
    SbxVariable *pCompVar = basicChosen->Find(  sKey, SbxClassType::Object );

    if ( pCompVar )
    {
        aModel = sbxToUnoValue( pCompVar );
        if ( !( aModel >>= xModel ) || !xModel.is() )
        {
            throw uno::RuntimeException(
                u"Can't extract model from basic ( it's obviously not set yet  therefore don't know the current document context)"_ustr );
        }
        SAL_INFO("vbahelper", "Have model points to url " << xModel->getURL());
    }
    else
    {
        SAL_INFO("vbahelper", "Failed to get " << sKey);
        throw uno::RuntimeException( u"Can't determine the currently selected document"_ustr );
    }
    return xModel;
}

/// @throws uno::RuntimeException
uno::Reference< frame::XModel >
getCurrentDocCtx( const OUString& ctxName, const uno::Reference< uno::XComponentContext >& xContext )
{
    uno::Reference< frame::XModel > xModel;
    // try fallback to calling doc
    css::uno::Reference< css::container::XNameAccess > xNameAccess( xContext, css::uno::UNO_QUERY_THROW );
    xModel.set( xNameAccess->getByName( ctxName ), uno::UNO_QUERY_THROW );
    return xModel;
}

uno::Reference< frame::XModel >
getThisExcelDoc( const uno::Reference< uno::XComponentContext >& xContext )
{
    return getCurrentDocCtx( u"ExcelDocumentContext"_ustr , xContext );
}

 uno::Reference< frame::XModel >
getCurrentExcelDoc( const uno::Reference< uno::XComponentContext >& xContext )
{
    uno::Reference< frame::XModel > xModel;
    try
    {
        xModel = getCurrentDoc( u"ThisExcelDoc"_ustr );
    }
    catch (const uno::Exception&)
    {
        try
        {
            xModel = getThisExcelDoc( xContext );
        }
        catch (const uno::Exception&)
        {
        }
    }
    return xModel;
}

sal_Int32
OORGBToXLRGB( sal_Int32 nCol )
{
    sal_Int32 nAutoBits = nCol;
    nAutoBits &= 0xFF000000;
    sal_Int32 nRed = nCol;
    nRed &= 0x00FF0000;
    nRed >>= 16;
    sal_Int32 nGreen = nCol;
    nGreen &= 0x0000FF00;
    nGreen >>= 8;
    sal_Int32 nBlue = nCol;
    nBlue &= 0x000000FF;
    sal_Int32 nRGB =  ( nAutoBits | (nBlue << 16) | (nGreen << 8) | nRed );
    return nRGB;
}

sal_Int32
XLRGBToOORGB( sal_Int32 nCol )
{
    sal_Int32 nAutoBits = nCol;
    nAutoBits &= 0xFF000000;

    sal_Int32 nBlue = nCol;
    nBlue &= 0x00FF0000;
    nBlue >>= 16;
    sal_Int32 nGreen = nCol;
    nGreen &= 0x0000FF00;
    nGreen >>= 8;
    sal_Int32 nRed = nCol;
    nRed &= 0x000000FF;
    sal_Int32 nRGB =  ( nAutoBits | (nRed << 16) | (nGreen << 8) | nBlue );
    return nRGB;
}
uno::Any
OORGBToXLRGB( const uno::Any& aCol )
{
    sal_Int32 nCol(0);
    aCol >>= nCol;
    nCol = OORGBToXLRGB( nCol );
    return uno::Any( nCol );
}
uno::Any
XLRGBToOORGB(  const uno::Any& aCol )
{
    sal_Int32 nCol(0);
    aCol >>= nCol;
    nCol = XLRGBToOORGB( nCol );
    return uno::Any( nCol );
}

void PrintOutHelper( SfxViewShell const * pViewShell, const uno::Any& From, const uno::Any& To, const uno::Any& Copies, const uno::Any& Preview, const uno::Any& /*ActivePrinter*/, const uno::Any& /*PrintToFile*/, const uno::Any& Collate, const uno::Any& PrToFileName, bool bUseSelection  )
{
    sal_Int32 nTo = 0;
    sal_Int32 nFrom = 0;
    sal_Int16 nCopies = 1;
    bool bPreview = false;
    bool bCollate = false;
    bool bSelection = bUseSelection;
    From >>= nFrom;
    To >>= nTo;
    Copies >>= nCopies;
    Preview >>= bPreview;
    if ( nCopies > 1 ) // Collate only useful when more that 1 copy
        Collate >>= bCollate;

    OUString sRange(   u"-"_ustr  );
    OUString sFileName;

    if ( nFrom || nTo )
    {
        if ( nFrom )
            sRange = OUString::number( nFrom ) + sRange;
        if ( nTo )
            sRange += OUString::number( nTo );
    }

    PrToFileName >>= sFileName;
    SfxViewFrame* pViewFrame = nullptr;
    if ( pViewShell )
        pViewFrame = &pViewShell->GetViewFrame();
    if ( !pViewFrame )
        return;

    SfxAllItemSet aArgs( SfxGetpApp()->GetPool() );

    SfxBoolItem sfxCollate( SID_PRINT_COLLATE, bCollate );
    aArgs.Put( sfxCollate );
    SfxInt16Item sfxCopies( SID_PRINT_COPIES, nCopies );
    aArgs.Put( sfxCopies );
    if ( !sFileName.isEmpty() )
    {
        SfxStringItem sfxFileName( SID_FILE_NAME, sFileName);
        aArgs.Put( sfxFileName );

    }
    if (  !sRange.isEmpty() )
    {
        SfxStringItem sfxRange( SID_PRINT_PAGES, sRange );
        aArgs.Put( sfxRange );
    }
    SfxBoolItem sfxSelection( SID_SELECTION, bSelection );
    aArgs.Put( sfxSelection );
    SfxBoolItem sfxAsync( SID_ASYNCHRON, false );
    aArgs.Put( sfxAsync );
    SfxDispatcher* pDispatcher = pViewFrame->GetDispatcher();

    if ( !pDispatcher )
        return;

    if ( bPreview )
    {
        if ( !pViewFrame->GetFrame().IsInPlace() )
        {
            // #TODO is this necessary ( calc specific )
//                  ScModule::get()->InputEnterHandler();
            pViewFrame->GetDispatcher()->Execute( SID_VIEWSHELL1, SfxCallMode::SYNCHRON );
            WaitUntilPreviewIsClosed( pViewFrame );
        }
    }
    else
        pDispatcher->Execute( sal_uInt16(SID_PRINTDOC), SfxCallMode::SYNCHRON, aArgs );


    // #FIXME #TODO
    // 1 ActivePrinter ( how/can we switch a printer via API? )
    // 2 PrintToFile ( ms behaviour if this option is specified but no
    //   filename supplied 'PrToFileName' then the user will be prompted )
    // 3 Need to check behaviour of Selected sheets with range ( e.g. From & To
    //    values ) in oOO these options are mutually exclusive
    // 4 There is a pop up to do with transparent objects in the print source
    //   should be able to disable that via configuration for the duration
    //   of this method
}

void PrintPreviewHelper( const css::uno::Any& /*EnableChanges*/, SfxViewShell const * pViewShell )
{
    SfxViewFrame* pViewFrame = nullptr;
    if ( pViewShell )
        pViewFrame = &pViewShell->GetViewFrame();
    if ( pViewFrame )
    {
        if ( !pViewFrame->GetFrame().IsInPlace() )
        {
            dispatchExecute( pViewShell, SID_VIEWSHELL1 );
            WaitUntilPreviewIsClosed( pViewFrame );
        }
    }
}

void WaitUntilPreviewIsClosed( SfxViewFrame* pViewFrame )
{
    while ( pViewFrame && isInPrintPreview( pViewFrame ) && !Application::IsQuit())
        Application::Yield();
}

bool extractBoolFromAny( const uno::Any& rAny )
{
    switch( rAny.getValueTypeClass() )
    {
        case uno::TypeClass_BOOLEAN:
            return rAny.get< bool >();
        case uno::TypeClass_FLOAT:
            return rAny.get< float >() != 0.0;
        case uno::TypeClass_DOUBLE:
            return rAny.get< double >() != 0.0;
        case uno::TypeClass_BYTE:
        case uno::TypeClass_SHORT:
        case uno::TypeClass_LONG:
            return rAny.get< sal_Int32 >() != 0;
        case uno::TypeClass_HYPER:
            return rAny.get< sal_Int64 >() != 0;
        default:;
    }
    throw uno::RuntimeException( u"Invalid type, cannot convert to boolean."_ustr , nullptr );
}

OUString extractStringFromAny( const uno::Any& rAny, bool bUppercaseBool )
{
    switch( rAny.getValueTypeClass() )
    {
        case uno::TypeClass_STRING:
            return rAny.get< OUString >();
        case uno::TypeClass_BOOLEAN:
            return bUppercaseBool ?
                (rAny.get< bool >() ? u"TRUE"_ustr : u"FALSE"_ustr) :
                OUString::boolean( rAny.get< bool >() );
        case uno::TypeClass_FLOAT:
            return OUString::number( rAny.get< float >() );
        case uno::TypeClass_DOUBLE:
            return OUString::number( rAny.get< double >() );
        case uno::TypeClass_BYTE:
        case uno::TypeClass_SHORT:
        case uno::TypeClass_LONG:
            return OUString::number( rAny.get< sal_Int32 >() );
        case uno::TypeClass_HYPER:
            return OUString::number( rAny.get< sal_Int64 >() );
        default:;
    }
    throw uno::RuntimeException( u"Invalid type, cannot convert to string."_ustr , nullptr );
}

OUString extractStringFromAny( const uno::Any& rAny, const OUString& rDefault, bool bUppercaseBool )
{
    return rAny.hasValue() ? extractStringFromAny( rAny, bUppercaseBool ) : rDefault;
}

OUString getAnyAsString( const uno::Any& pvargItem )
{
    return extractStringFromAny( pvargItem );
}


OUString
ContainerUtilities::getUniqueName( const uno::Sequence< OUString >&  _slist, const OUString& _sElementName, std::u16string_view _sSuffixSeparator)
{
    return getUniqueName(_slist, _sElementName, _sSuffixSeparator, sal_Int32(2));
}

OUString
ContainerUtilities::getUniqueName( const uno::Sequence< OUString >& _slist, const OUString& _sElementName, std::u16string_view _sSuffixSeparator, sal_Int32 _nStartSuffix)
{
    if ( !_slist.hasElements() )
        return _sElementName;

    OUString scompname = _sElementName;
    sal_Int32 a = _nStartSuffix;

    for (;;)
    {
        if (FieldInList(_slist, scompname) == -1)
            return scompname;

        scompname = _sElementName + _sSuffixSeparator + OUString::number( a++ );
    }
}

sal_Int32
ContainerUtilities::FieldInList( const uno::Sequence< OUString >& SearchList, const OUString& SearchString )
{
    // I wonder why comparing lexicographically is done
    // when it's a match, is it interesting?
    return comphelper::findValue(SearchList, SearchString);
}

static bool NeedEsc(sal_Unicode cCode)
{
    return u".^$+\\|{}()"_ustr.indexOf(cCode) != -1;
}

OUString VBAToRegexp(const OUString &rIn)
{
    OUStringBuffer sResult;
    const sal_Unicode *start = rIn.getStr();
    const sal_Unicode *end = start + rIn.getLength();

    int seenright = 0;

    while (start < end)
    {
        switch (*start)
        {
            case '?':
                sResult.append('.');
                start++;
                break;
            case '*':
                sResult.append(".*");
                start++;
                break;
            case '#':
                sResult.append("[0-9]");
                start++;
                break;
            case '~':
                sResult.append('\\');
                sResult.append(*(++start));
                start++;
                break;
                // dump the ~ and escape the next character
            case ']':
                sResult.append('\\');
                sResult.append(*start++);
                break;
            case '[':
                sResult.append(*start++);
                seenright = 0;
                while (start < end && !seenright)
                {
                    switch (*start)
                    {
                        case '[':
                        case '?':
                        case '*':
                        sResult.append('\\');
                        sResult.append(*start);
                            break;
                        case ']':
                        sResult.append(*start);
                            seenright = 1;
                            break;
                        case '!':
                            sResult.append('^');
                            break;
                        default:
                        if (NeedEsc(*start))
                            sResult.append('\\');
                        sResult.append(*start);
                            break;
                    }
                    start++;
                }
                break;
            default:
                if (NeedEsc(*start))
                    sResult.append('\\');
                sResult.append(*start++);
        }
    }

    return sResult.makeStringAndClear( );
}

static double getPixelToMeterConversionFactor( const css::uno::Reference< css::awt::XDevice >& xDevice, bool bVertical)
{
    return bVertical ? xDevice->getInfo().PixelPerMeterY : xDevice->getInfo().PixelPerMeterX;
}

double PointsToPixels( const css::uno::Reference< css::awt::XDevice >& xDevice, double fPoints, bool bVertical)
{
    double fConvertFactor = getPixelToMeterConversionFactor( xDevice, bVertical );
    return o3tl::convert(fPoints, o3tl::Length::pt, o3tl::Length::m) * fConvertFactor;
}
double PixelsToPoints( const css::uno::Reference< css::awt::XDevice >& xDevice, double fPixels, bool bVertical)
{
    double fConvertFactor = getPixelToMeterConversionFactor( xDevice, bVertical );
    return o3tl::convert(fPixels / fConvertFactor, o3tl::Length::m, o3tl::Length::pt);
}

ConcreteXShapeGeometryAttributes::ConcreteXShapeGeometryAttributes( const css::uno::Reference< css::drawing::XShape >& xShape )
  : m_aShapeHelper( xShape )
{
}
ConcreteXShapeGeometryAttributes::~ConcreteXShapeGeometryAttributes()
{
}

PointerStyle getPointerStyle( const uno::Reference< frame::XModel >& xModel )
{

    PointerStyle nPointerStyle( PointerStyle::Arrow );
    try
    {
        const uno::Reference< frame::XController > xController( xModel->getCurrentController(),     uno::UNO_SET_THROW );
        const uno::Reference< frame::XFrame >      xFrame     ( xController->getFrame(),        uno::UNO_SET_THROW );
        const uno::Reference< awt::XWindow >       xWindow    ( xFrame->getContainerWindow(),   uno::UNO_SET_THROW );
        // why the heck isn't there an XWindowPeer::getPointer, but a setPointer only?
        const vcl::Window* pWindow = VCLUnoHelper::GetWindow( xWindow );
        if ( pWindow )
            nPointerStyle = pWindow->GetSystemWindow()->GetPointer();
    }
    catch (const uno::Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("vbahelper");
    }
    return nPointerStyle;
}

// #FIXME this method looks wrong, shouldn't it just affect calc *or* writer
// document/frame/window(s) but not both ( and depending on what api called
// this )
void setCursorHelper( const uno::Reference< frame::XModel >& xModel, PointerStyle nPointer, bool bOverWrite )
{
    ::std::vector< uno::Reference< frame::XController > > aControllers;

    uno::Reference< frame::XModel2 > xModel2( xModel, uno::UNO_QUERY );
    if ( xModel2.is() )
    {
        const uno::Reference< container::XEnumeration > xEnumControllers( xModel2->getControllers(), uno::UNO_SET_THROW );
        while ( xEnumControllers->hasMoreElements() )
        {
            const uno::Reference< frame::XController > xController( xEnumControllers->nextElement(), uno::UNO_QUERY_THROW );
            aControllers.push_back( xController );
        }
    }
    else
    {
        if ( xModel.is() )
        {
            const uno::Reference< frame::XController > xController( xModel->getCurrentController(), uno::UNO_SET_THROW );
            aControllers.push_back( xController );
        }
    }

    for ( const auto& rController : aControllers )
    {
        const uno::Reference< frame::XFrame >      xFrame     ( rController->getFrame(),         uno::UNO_SET_THROW   );
        const uno::Reference< awt::XWindow >       xWindow    ( xFrame->getContainerWindow(),    uno::UNO_SET_THROW   );

        VclPtr<vcl::Window> pWindow = VCLUnoHelper::GetWindow( xWindow );
        SAL_WARN_IF( !pWindow, "vbahelper", "ScVbaApplication::setCursor: no window!" );
        if ( !pWindow )
            continue;

        pWindow->GetSystemWindow()->SetPointer( nPointer );
        pWindow->GetSystemWindow()->EnableChildPointerOverwrite( bOverWrite );
    }
}

void setDefaultPropByIntrospection( const uno::Any& aObj, const uno::Any& aValue  )
{
    uno::Reference< beans::XIntrospectionAccess > xUnoAccess( getIntrospectionAccess( aObj ) );

    // #MAYBE #FIXME sort of a bit of a hack,
    uno::Reference<  script::XDefaultProperty > xDflt( aObj, uno::UNO_QUERY_THROW );
    uno::Reference< beans::XPropertySet > xPropSet;

    if ( xUnoAccess.is() )
        xPropSet.set( xUnoAccess->queryAdapter( cppu::UnoType<beans::XPropertySet>::get()), uno::UNO_QUERY);

    if ( !xPropSet.is() )
        throw uno::RuntimeException();

    xPropSet->setPropertyValue( xDflt->getDefaultPropertyName(), aValue );
}

uno::Any getPropertyValue( const uno::Sequence< beans::PropertyValue >& aProp, const OUString& aName )
{
    auto pProp = std::find_if(aProp.begin(), aProp.end(),
        [&aName](const beans::PropertyValue& rProp) { return rProp.Name == aName; });
    if (pProp != aProp.end())
        return pProp->Value;
    return uno::Any();
}

bool setPropertyValue( uno::Sequence< beans::PropertyValue >& aProp, const OUString& aName, const uno::Any& aValue )
{
    auto [begin, end] = asNonConstRange(aProp);
    auto pProp = std::find_if(begin, end,
        [&aName](const beans::PropertyValue& rProp) { return rProp.Name == aName; });
    if (pProp != end)
    {
        pProp->Value = aValue;
        return true;
    }
    return false;
}

void setOrAppendPropertyValue( uno::Sequence< beans::PropertyValue >& aProp, const OUString& aName, const uno::Any& aValue )
{
    if( setPropertyValue( aProp, aName, aValue ) )
        return;

    // append the property
    sal_Int32 nLength = aProp.getLength();
    aProp.realloc( nLength + 1 );
    auto pProp = aProp.getArray();
    pProp[ nLength ].Name = aName;
    pProp[ nLength ].Value = aValue;
}

bool executeRunTimeLibrary(std::u16string_view rSbRtl_command, SbxArray* pParameters)
{
    StarBASIC* pBasic = dynamic_cast< StarBASIC* >(StarBASIC::GetActiveModule()->GetParent());
    if (!pBasic)
        return false;

    SbxObject* pRunTimeLibrary = pBasic->GetRtl();
    if (!pRunTimeLibrary)
        return false;

    SbxVariable* pFound = pRunTimeLibrary->Find(OUString(rSbRtl_command), SbxClassType::Method);
    SbxMethod* pMethod = dynamic_cast<SbxMethod*>(pFound);
    if (!pMethod)
        return false;

    pMethod->SetParameters(pParameters);
    // Believe it or not, this actually runs the command
    pMethod->Broadcast(SfxHintId::BasicDataWanted);
    return true;
}

// ====UserFormGeomentryHelper====

UserFormGeometryHelper::UserFormGeometryHelper(
        const uno::Reference< awt::XControl >& xControl,
        double fOffsetX, double fOffsetY ) :
    mfOffsetX( fOffsetX ),
    mfOffsetY( fOffsetY ),
    mbDialog( uno::Reference< awt::XDialog >( xControl, uno::UNO_QUERY ).is() )
{
    if ( !xControl.is() )
        throw uno::RuntimeException( u"No control is provided!"_ustr );

    mxWindow.set( xControl->getPeer(), uno::UNO_QUERY_THROW );
    mxModelProps.set( xControl->getModel(), uno::UNO_QUERY_THROW );
    mxUnitConv.set( mxWindow, uno::UNO_QUERY_THROW );
}

double UserFormGeometryHelper::getLeft() const
{
    return implGetPos( false );
}

void UserFormGeometryHelper::setLeft( double fLeft )
{
    implSetPos( fLeft, false );
}

double UserFormGeometryHelper::getTop() const
{
    return implGetPos( true );
}

void UserFormGeometryHelper::setTop( double fTop )
{
    implSetPos( fTop, true );
}

double UserFormGeometryHelper::getWidth() const
{
    return implGetSize( false, true );
}

void UserFormGeometryHelper::setWidth( double fWidth )
{
    implSetSize( fWidth, false, true );
}

double UserFormGeometryHelper::getHeight() const
{
    return implGetSize( true, true );
}

void UserFormGeometryHelper::setHeight( double fHeight )
{
    implSetSize( fHeight, true, true );
}

double UserFormGeometryHelper::getInnerWidth() const
{
    return implGetSize( false, false );
}

void UserFormGeometryHelper::setInnerWidth( double fWidth )
{
    implSetSize( fWidth, false, false );
}

double UserFormGeometryHelper::getInnerHeight() const
{
    return implGetSize( true, false );
}

void UserFormGeometryHelper::setInnerHeight( double fHeight )
{
    implSetSize( fHeight, true, false );
}

double UserFormGeometryHelper::getOffsetX() const
{
    return mfOffsetX;
}

double UserFormGeometryHelper::getOffsetY() const
{
    return mfOffsetY;
}

constexpr OUString saPosXName = u"PositionX"_ustr;
constexpr OUString saPosYName = u"PositionY"_ustr;
constexpr OUString saWidthName = u"Width"_ustr;
constexpr OUString saHeightName = u"Height"_ustr;

double UserFormGeometryHelper::implGetPos( bool bPosY ) const
{
    sal_Int32 nPosAppFont = mxModelProps->getPropertyValue( bPosY ? saPosYName : saPosXName ).get< sal_Int32 >();
    // appfont to pixel
    awt::Point aPosPixel = mxUnitConv->convertPointToPixel( awt::Point( nPosAppFont, nPosAppFont ), util::MeasureUnit::APPFONT );
    // pixel to VBA points
    awt::Point aPosPoint = mxUnitConv->convertPointToLogic( aPosPixel, util::MeasureUnit::POINT );
    return bPosY ? (aPosPoint.Y - mfOffsetY) : (aPosPoint.X - mfOffsetX);
}

void UserFormGeometryHelper::implSetPos( double fPos, bool bPosY )
{
    // convert passed VBA points to pixels
    sal_Int32 nPosPixel = static_cast< sal_Int32 >( fPos + (bPosY ? mfOffsetY : mfOffsetX) );
    awt::Point aPosPixel = mxUnitConv->convertPointToPixel( awt::Point( nPosPixel, nPosPixel ), util::MeasureUnit::POINT );
    // pixel to appfont
    awt::Point aPosAppFont = mxUnitConv->convertPointToLogic( aPosPixel, util::MeasureUnit::APPFONT );
    mxModelProps->setPropertyValue( bPosY ? saPosYName : saPosXName, uno::Any( bPosY ? aPosAppFont.Y : aPosAppFont.X ) );
}

double UserFormGeometryHelper::implGetSize( bool bHeight, bool bOuter ) const
{
    sal_Int32 nSizeAppFont = mxModelProps->getPropertyValue( bHeight ? saHeightName : saWidthName ).get< sal_Int32 >();
    // appfont to pixel
    awt::Size aSizePixel = mxUnitConv->convertSizeToPixel( awt::Size( nSizeAppFont, nSizeAppFont ), util::MeasureUnit::APPFONT );

    /*  The VBA symbols 'Width' and 'Height' return the outer size including
        window decoration (in difference to the symbols 'InnerWidth' and
        'InnerHeight'), but the window API returns the inner size. */
    if( mbDialog && bOuter )
    {
        if( const vcl::Window* pWindow = VCLUnoHelper::GetWindow( mxWindow ) )
        {
            AbsoluteScreenPixelRectangle aOuterRect = pWindow->GetWindowExtentsAbsolute();
            aSizePixel = awt::Size( aOuterRect.getOpenWidth(), aOuterRect.getOpenHeight() );
        }
    }

    // pixel to VBA points
    awt::Size aSizePoint = mxUnitConv->convertSizeToLogic( aSizePixel, util::MeasureUnit::POINT );
    return bHeight ? aSizePoint.Height : aSizePoint.Width;
}

void UserFormGeometryHelper::implSetSize( double fSize, bool bHeight, bool bOuter )
{
    // convert passed VBA points to pixels
    sal_Int32 nSize = static_cast< sal_Int32 >( fSize );
    awt::Size aSizePixel = mxUnitConv->convertSizeToPixel( awt::Size( nSize, nSize ), util::MeasureUnit::POINT );

    /*  The VBA symbols 'Width' and 'Height' set the outer size (in difference
        to the symbols 'InnerWidth' and 'InnerHeight'), but the dialog model
        expects the inner size. We have to remove the window extents from the
        pixel height to get the same result. */
    if ( mbDialog && bOuter )
    {
        if( const vcl::Window* pWindow = VCLUnoHelper::GetWindow( mxWindow ) )
        {
            AbsoluteScreenPixelRectangle aOuterRect = pWindow->GetWindowExtentsAbsolute();
            if( !aOuterRect.IsEmpty() )
            {
                awt::Rectangle aInnerRect = mxWindow->getPosSize();
                sal_Int32 nDecorWidth = aOuterRect.getOpenWidth() - aInnerRect.Width;
                sal_Int32 nDecorHeight = aOuterRect.getOpenHeight() - aInnerRect.Height;
                aSizePixel.Width = ::std::max< sal_Int32 >( aSizePixel.Width - nDecorWidth, 1 );
                aSizePixel.Height = ::std::max< sal_Int32 >( aSizePixel.Height - nDecorHeight, 1 );
            }
        }
    }

    awt::Size aSizeAppFont = mxUnitConv->convertSizeToLogic( aSizePixel, util::MeasureUnit::APPFONT );
    mxModelProps->setPropertyValue( bHeight ? saHeightName : saWidthName, uno::Any( bHeight ? aSizeAppFont.Height : aSizeAppFont.Width ) );
}


double ConcreteXShapeGeometryAttributes::getLeft() const
{
    return m_aShapeHelper.getLeft();
}
void ConcreteXShapeGeometryAttributes::setLeft( double nLeft )
{
    m_aShapeHelper.setLeft( nLeft );
}
double ConcreteXShapeGeometryAttributes::getTop() const
{
    return m_aShapeHelper.getTop();
}
void ConcreteXShapeGeometryAttributes::setTop( double nTop )
{
    m_aShapeHelper.setTop( nTop );
}

double ConcreteXShapeGeometryAttributes::getHeight() const
{
    return m_aShapeHelper.getHeight();
}
void ConcreteXShapeGeometryAttributes::setHeight( double nHeight )
{
    m_aShapeHelper.setHeight( nHeight );
}
double ConcreteXShapeGeometryAttributes::getWidth() const
{
    return m_aShapeHelper.getWidth();
}
void ConcreteXShapeGeometryAttributes::setWidth( double nWidth)
{
    m_aShapeHelper.setWidth( nWidth );
}


ShapeHelper::ShapeHelper( css::uno::Reference< css::drawing::XShape > _xShape)
    : xShape(std::move( _xShape ))
{
    if( !xShape.is() )
        throw css::uno::RuntimeException( u"No valid shape for helper"_ustr );
}

double ShapeHelper::getHeight() const
{
   return  Millimeter::getInPoints(xShape->getSize().Height);
}

void ShapeHelper::setHeight(double _fheight)
{
    css::awt::Size aSize = xShape->getSize();
    aSize.Height = Millimeter::getInHundredthsOfOneMillimeter(_fheight);
    xShape->setSize(aSize);
}

double ShapeHelper::getWidth() const
{
    return Millimeter::getInPoints(xShape->getSize().Width);
}

void ShapeHelper::setWidth(double _fWidth)
{
    css::awt::Size aSize = xShape->getSize();
    aSize.Width = Millimeter::getInHundredthsOfOneMillimeter(_fWidth);
    xShape->setSize(aSize);
}

double ShapeHelper::getLeft() const
{
    return Millimeter::getInPoints(xShape->getPosition().X);
}

void ShapeHelper::setLeft(double _fLeft)
{
    css::awt::Point aPoint = xShape->getPosition();
    aPoint.X = Millimeter::getInHundredthsOfOneMillimeter(_fLeft);
    xShape->setPosition(aPoint);
}

double ShapeHelper::getTop() const
{
    return Millimeter::getInPoints(xShape->getPosition().Y);
}

void ShapeHelper::setTop(double _fTop)
{
    css::awt::Point aPoint = xShape->getPosition();
    aPoint.Y = Millimeter::getInHundredthsOfOneMillimeter(_fTop);
    xShape->setPosition(aPoint);
}

void DebugHelper::basicexception( const css::uno::Exception& ex, ErrCode err, std::u16string_view /*additionalArgument*/ )
{
    // #TODO #FIXME ( do we want to support additionalArg here )
    throw css::script::BasicErrorException( ex.Message, css::uno::Reference< css::uno::XInterface >(), sal_uInt32(err), OUString() );
}

void DebugHelper::basicexception( ErrCode err,  std::u16string_view additionalArgument )
{
    basicexception( css::uno::Exception(), err, additionalArgument );
}

void DebugHelper::basicexception( const css::uno::Exception& ex )
{
    basicexception( ex, ERRCODE_BASIC_INTERNAL_ERROR, {} );
}

void DebugHelper::runtimeexception( ErrCode err )
{
    // #TODO #FIXME ( do we want to support additionalArg here )
    throw css::uno::RuntimeException( css::uno::Exception().Message + " " + OUString::number(sal_uInt32(err)),
                                      css::uno::Reference< css::uno::XInterface >() );
}

Millimeter::Millimeter():m_nMillimeter(0) {}

Millimeter::Millimeter(double mm):m_nMillimeter(mm) {}

void Millimeter::setInPoints(double points)
{
    m_nMillimeter = convertPointToMm100(points) / 100.0;
}

double Millimeter::getInHundredthsOfOneMillimeter() const
{
    return m_nMillimeter * 100;
}

sal_Int32 Millimeter::getInHundredthsOfOneMillimeter(double points)
{
    sal_Int32 mm = std::round(convertPointToMm100(points));
    return mm;
}

double Millimeter::getInPoints(int _hmm)
{
    double points = convertMm100ToPoint<double>(_hmm);
    return points;
}

uno::Reference< XHelperInterface > getVBADocument( const uno::Reference< frame::XModel >& xModel )
{
    uno::Reference< XHelperInterface > xIf;
    try
    {
        uno::Reference< beans::XPropertySet > xDocProps( xModel, uno::UNO_QUERY );
        if (!xDocProps)
            return xIf;
        OUString aCodeName;
        xDocProps->getPropertyValue( u"CodeName"_ustr ) >>= aCodeName;
        xIf = getUnoDocModule( aCodeName, getSfxObjShell( xModel ) );
    }
    catch (const uno::Exception&)
    {
    }
    return xIf;
}

uno::Reference< XHelperInterface > getUnoDocModule( std::u16string_view aModName, SfxObjectShell const * pShell )
{
    uno::Reference< XHelperInterface > xIf;
    if ( pShell )
    {
        OUString sProj( u"Standard"_ustr );
        // GetBasicManager() causes a SolarMutex assertion failure in some use cases from
        // Automation, at least when opening a Calc Document through ooo::vba::excel::
        // XWorkbooks::Open(). Let's see if this check is a good way around that. It does seem that
        // callers are prepared for this to return null?
        if (comphelper::Automation::AutomationInvokedZone::isActive())
            return xIf;
        BasicManager* pBasMgr = pShell->GetBasicManager();
        if ( pBasMgr && !pBasMgr->GetName().isEmpty() )
            sProj = pBasMgr->GetName();
        if( StarBASIC* pBasic = pShell->GetBasicManager()->GetLib( sProj ) )
            if( SbModule* pMod = pBasic->FindModule( aModName ) )
                xIf.set( pMod->GetUnoModule(), uno::UNO_QUERY );
    }
    return xIf;
}

SfxObjectShell* getSfxObjShell( const uno::Reference< frame::XModel >& xModel )
{
    SfxObjectShell* pFoundShell = comphelper::getFromUnoTunnel<SfxObjectShell>(xModel);
    if ( !pFoundShell )
        throw uno::RuntimeException();
    return pFoundShell;
}

} //org

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
