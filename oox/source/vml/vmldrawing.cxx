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

#include <oox/vml/vmldrawing.hxx>

#include <algorithm>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/XControlShape.hpp>
#include <com/sun/star/drawing/XDrawPage.hpp>
#include <com/sun/star/drawing/XShapes.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/text/HoriOrientation.hpp>
#include <com/sun/star/text/RelOrientation.hpp>
#include <com/sun/star/text/VertOrientation.hpp>
#include <osl/diagnose.h>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <oox/core/xmlfilterbase.hxx>
#include <oox/helper/containerhelper.hxx>
#include <oox/ole/axcontrol.hxx>
#include <oox/vml/vmlshape.hxx>
#include <oox/vml/vmlshapecontainer.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/gen.hxx>
#include <o3tl/string_view.hxx>

namespace oox::vml {

using namespace ::com::sun::star;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::text;
using namespace ::com::sun::star::uno;

using ::oox::core::XmlFilterBase;

namespace {

/** Returns the textual representation of a numeric VML shape identifier. */
OUString lclGetShapeId( sal_Int32 nShapeId )
{
    // identifier consists of a literal NUL character, a lowercase 's', and the id
    static constexpr OUStringLiteral aStr = u"\0s";
    return aStr + OUString::number( nShapeId );
}

/** Returns the numeric VML shape identifier from its textual representation. */
sal_Int32 lclGetShapeId( std::u16string_view rShapeId )
{
    // identifier consists of a literal NUL character, a lowercase 's', and the id
    return ((rShapeId.size() >= 3) && (rShapeId[ 0 ] == '\0') && (rShapeId[ 1 ] == 's')) ? o3tl::toInt32(rShapeId.substr( 2 )) : -1;
}

} // namespace

OleObjectInfo::OleObjectInfo( bool bDmlShape ) :
    mbAutoLoad( false ),
    mbDmlShape( bDmlShape )
{
}

void OleObjectInfo::setShapeId( sal_Int32 nShapeId )
{
    maShapeId = lclGetShapeId( nShapeId );
}

ControlInfo::ControlInfo()
    : mbTextContentShape(false)
{
}

void ControlInfo::setShapeId( sal_Int32 nShapeId )
{
    maShapeId = lclGetShapeId(nShapeId);
}

Drawing::Drawing( XmlFilterBase& rFilter, const Reference< XDrawPage >& rxDrawPage, DrawingType eType ) :
    mrFilter( rFilter ),
    mxDrawPage( rxDrawPage ),
    mxShapes( new ShapeContainer( *this ) ),
    meType( eType )
{
    OSL_ENSURE( mxDrawPage.is(), "Drawing::Drawing - missing UNO draw page" );
}

Drawing::~Drawing()
{
}

::oox::ole::EmbeddedForm& Drawing::getControlForm() const
{
    if (!mxCtrlForm)
        mxCtrlForm.reset( new ::oox::ole::EmbeddedForm(
            mrFilter.getModel(), mxDrawPage, mrFilter.getGraphicHelper() ) );
    return *mxCtrlForm;
}

void Drawing::registerBlockId( sal_Int32 nBlockId )
{
    OSL_ENSURE( nBlockId > 0, "Drawing::registerBlockId - invalid block index" );
    if( nBlockId > 0 )
    {
        // lower_bound() returns iterator pointing to element equal to nBlockId, if existing
        BlockIdVector::iterator aIt = ::std::lower_bound( maBlockIds.begin(), maBlockIds.end(), nBlockId );
        if( (aIt == maBlockIds.end()) || (nBlockId != *aIt) )
            maBlockIds.insert( aIt, nBlockId );
    }
}

void Drawing::registerOleObject( const OleObjectInfo& rOleObject )
{
    OSL_ENSURE( !rOleObject.maShapeId.isEmpty(), "Drawing::registerOleObject - missing OLE object shape id" );
    OSL_ENSURE( maOleObjects.count( rOleObject.maShapeId ) == 0, "Drawing::registerOleObject - OLE object already registered" );
    maOleObjects.emplace( rOleObject.maShapeId, rOleObject );
}

void Drawing::registerControl( const ControlInfo& rControl )
{
    OSL_ENSURE( !rControl.maShapeId.isEmpty(), "Drawing::registerControl - missing form control shape id" );
    OSL_ENSURE( !rControl.maName.isEmpty(), "Drawing::registerControl - missing form control name" );
    OSL_ENSURE( maControls.count( rControl.maShapeId ) == 0, "Drawing::registerControl - form control already registered" );
    maControls.emplace( rControl.maShapeId, rControl );
}

void Drawing::finalizeFragmentImport()
{
    mxShapes->finalizeFragmentImport();
}

void Drawing::convertAndInsert() const
{
    Reference< XShapes > xShapes( mxDrawPage );
    mxShapes->convertAndInsert( xShapes );

    // Group together form control radio buttons that are in the same groupBox
    std::map<OUString, tools::Rectangle> GroupBoxMap;
    std::map<Reference< XPropertySet >, tools::Rectangle> RadioButtonMap;
    for ( sal_Int32 i = 0; i < xShapes->getCount(); ++i )
    {
        try
        {
            Reference< XControlShape > xCtrlShape( xShapes->getByIndex(i), UNO_QUERY );
            if (!xCtrlShape.is())
                continue;
            Reference< XControlModel > xCtrlModel( xCtrlShape->getControl(), UNO_SET_THROW );
            Reference< XServiceInfo > xModelSI (xCtrlModel, UNO_QUERY_THROW );
            Reference< XPropertySet >  aProps( xCtrlModel, UNO_QUERY_THROW );

            OUString sName;
            aProps->getPropertyValue(u"Name"_ustr) >>= sName;
            const ::Point aPoint( xCtrlShape->getPosition().X, xCtrlShape->getPosition().Y );
            const ::Size aSize( xCtrlShape->getSize().Width, xCtrlShape->getSize().Height );
            const tools::Rectangle aRect( aPoint, aSize );
            if ( !sName.isEmpty()
                 && xModelSI->supportsService(u"com.sun.star.awt.UnoControlGroupBoxModel"_ustr) )
            {
                GroupBoxMap[sName] = aRect;
            }
            else if ( xModelSI->supportsService(u"com.sun.star.awt.UnoControlRadioButtonModel"_ustr) )
            {
                OUString sGroupName;
                aProps->getPropertyValue(u"GroupName"_ustr) >>= sGroupName;
                // only Form Controls are affected by Group Boxes - see drawingfragment.cxx
                if ( sGroupName == "autoGroup_formControl" )
                    RadioButtonMap[aProps] = aRect;
            }
        }
        catch (uno::Exception&)
        {
            DBG_UNHANDLED_EXCEPTION("oox.vml");
        }
    }
    for ( const auto& BoxItr : GroupBoxMap )
    {
        const uno::Any aGroup( "autoGroup_" + BoxItr.first );
        for ( auto RadioItr = RadioButtonMap.begin(); RadioItr != RadioButtonMap.end(); )
        {
            if ( BoxItr.second.Contains(RadioItr->second) )
            {
                RadioItr->first->setPropertyValue(u"GroupName"_ustr, aGroup );
                // If conflict, first created GroupBox wins
                RadioItr = RadioButtonMap.erase(RadioItr);
            }
            else
                ++RadioItr;
        }
    }

}

sal_Int32 Drawing::getLocalShapeIndex( std::u16string_view rShapeId ) const
{
    sal_Int32 nShapeId = lclGetShapeId( rShapeId );
    if( nShapeId <= 0 ) return -1;

    /*  Shapes in a drawing are counted per registered shape identifier blocks
        as stored in the o:idmap element. The contents of this element have
        been stored in our member maBlockIds. Each block represents 1024 shape
        identifiers, starting with identifier 1 for the block #0. This means,
        block #0 represents the identifiers 1-1024, block #1 represents the
        identifiers 1025-2048, and so on. The local shape index has to be
        calculated according to all blocks registered for this drawing.

        Example:
            Registered for this drawing are blocks #1 and #3 (shape identifiers
            1025-2048 and 3073-4096).
            Shape identifier 1025 -> local shape index 1.
            Shape identifier 1026 -> local shape index 2.
            ...
            Shape identifier 2048 -> local shape index 1024.
            Shape identifier 3073 -> local shape index 1025.
            ...
            Shape identifier 4096 -> local shape index 2048.
     */

    // get block id from shape id and find its index in the list of used blocks
    sal_Int32 nBlockId = (nShapeId - 1) / 1024;
    BlockIdVector::iterator aIt = ::std::lower_bound( maBlockIds.begin(), maBlockIds.end(), nBlockId );
    sal_Int32 nIndex = static_cast< sal_Int32 >( aIt - maBlockIds.begin() );

    // block id not found in set -> register it now (value of nIndex remains valid)
    if( (aIt == maBlockIds.end()) || (*aIt != nBlockId) )
        maBlockIds.insert( aIt, nBlockId );

    // get one-based offset of shape id in its block
    sal_Int32 nBlockOffset = (nShapeId - 1) % 1024 + 1;

    // calculate the local shape index
    sal_Int32 nRet;
    if (o3tl::checked_add(1024 * nIndex, nBlockOffset, nRet))
    {
        SAL_WARN("oox", "getLocalShapeIndex: overflow on " << 1024 * nIndex << " + " << nBlockOffset);
        nRet = -1;
    }
    return nRet;
}

const OleObjectInfo* Drawing::getOleObjectInfo( const OUString& rShapeId ) const
{
    return ContainerHelper::getMapElement( maOleObjects, rShapeId );
}

const ControlInfo* Drawing::getControlInfo( const OUString& rShapeId ) const
{
    return ContainerHelper::getMapElement( maControls, rShapeId );
}

Reference< XShape > Drawing::createAndInsertXShape( const OUString& rService,
        const Reference< XShapes >& rxShapes, const awt::Rectangle& rShapeRect ) const
{
    OSL_ENSURE( !rService.isEmpty(), "Drawing::createAndInsertXShape - missing UNO shape service name" );
    OSL_ENSURE( rxShapes.is(), "Drawing::createAndInsertXShape - missing XShapes container" );
    Reference< XShape > xShape;
    if( !rService.isEmpty() && rxShapes.is() ) try
    {
        Reference< XMultiServiceFactory > xModelFactory( mrFilter.getModelFactory(), UNO_SET_THROW );
        xShape.set( xModelFactory->createInstance( rService ), UNO_QUERY_THROW );
        if ( rService != "com.sun.star.text.TextFrame" )
        {
            // insert shape into passed shape collection (maybe drawpage or group shape)
            rxShapes->add( xShape );
            xShape->setPosition( awt::Point( rShapeRect.X, rShapeRect.Y ) );
        }
        else
        {
            Reference< XPropertySet > xPropSet( xShape, UNO_QUERY_THROW );
            xPropSet->setPropertyValue( u"HoriOrient"_ustr, Any( HoriOrientation::NONE ) );
            xPropSet->setPropertyValue( u"VertOrient"_ustr, Any( VertOrientation::NONE ) );
            xPropSet->setPropertyValue( u"HoriOrientPosition"_ustr, Any( rShapeRect.X ) );
            xPropSet->setPropertyValue( u"VertOrientPosition"_ustr, Any( rShapeRect.Y ) );
            xPropSet->setPropertyValue( u"HoriOrientRelation"_ustr, Any( RelOrientation::FRAME ) );
            xPropSet->setPropertyValue( u"VertOrientRelation"_ustr, Any( RelOrientation::FRAME ) );
        }
        xShape->setSize( awt::Size( rShapeRect.Width, rShapeRect.Height ) );
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "oox", "Drawing::createAndInsertXShape - error during shape object creation" );
    }
    OSL_ENSURE( xShape.is(), "Drawing::createAndInsertXShape - cannot instantiate shape object" );
    return xShape;
}

Reference< XShape > Drawing::createAndInsertXControlShape( const ::oox::ole::EmbeddedControl& rControl,
        const Reference< XShapes >& rxShapes, const awt::Rectangle& rShapeRect, sal_Int32& rnCtrlIndex ) const
{
    Reference< XShape > xShape;
    try
    {
        // create control model and insert it into the form of the draw page
        Reference< XControlModel > xCtrlModel( getControlForm().convertAndInsert( rControl, rnCtrlIndex ), UNO_SET_THROW );

        // create the control shape
        xShape = createAndInsertXShape( u"com.sun.star.drawing.ControlShape"_ustr, rxShapes, rShapeRect );

        // set the control model at the shape
        Reference< XControlShape >( xShape, UNO_QUERY_THROW )->setControl( xCtrlModel );
    }
    catch (Exception const&)
    {
        TOOLS_WARN_EXCEPTION("oox", "exception inserting Shape");
    }
    return xShape;
}

bool Drawing::isShapeSupported( const ShapeBase& /*rShape*/ ) const
{
    return true;
}

OUString Drawing::getShapeBaseName( const ShapeBase& /*rShape*/ ) const
{
    return OUString();
}

bool Drawing::convertClientAnchor( awt::Rectangle& /*orShapeRect*/, const OUString& /*rShapeAnchor*/ ) const
{
    return false;
}

Reference< XShape > Drawing::createAndInsertClientXShape( const ShapeBase& /*rShape*/,
        const Reference< XShapes >& /*rxShapes*/, const awt::Rectangle& /*rShapeRect*/ ) const
{
    return Reference< XShape >();
}

void Drawing::notifyXShapeInserted( const Reference< XShape >& /*rxShape*/,
        const awt::Rectangle& /*rShapeRect*/, const ShapeBase& /*rShape*/, bool /*bGroupChild*/ )
{
}

} // namespace oox::vml

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
