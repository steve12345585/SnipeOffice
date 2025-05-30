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

#include "common.hxx"
#include "imp_share.hxx"
#include <utility>
#include <xml_import.hxx>
#include <xmlscript/xmlns.h>

#include <com/sun/star/awt/CharSet.hpp>
#include <com/sun/star/awt/FontFamily.hpp>
#include <com/sun/star/awt/FontPitch.hpp>
#include <com/sun/star/awt/FontSlant.hpp>
#include <com/sun/star/awt/FontStrikeout.hpp>
#include <com/sun/star/awt/FontType.hpp>
#include <com/sun/star/awt/FontUnderline.hpp>
#include <com/sun/star/awt/ImagePosition.hpp>
#include <com/sun/star/awt/ImageScaleMode.hpp>
#include <com/sun/star/awt/LineEndFormat.hpp>
#include <com/sun/star/awt/PushButtonType.hpp>
#include <com/sun/star/awt/VisualEffect.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/style/VerticalAlignment.hpp>
#include <com/sun/star/util/Date.hpp>
#include <com/sun/star/util/Time.hpp>
#include <sal/log.hxx>
#include <tools/date.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/time.hxx>
#include <osl/diagnose.h>

#include <com/sun/star/script/XScriptEventsSupplier.hpp>
#include <com/sun/star/script/ScriptEventDescriptor.hpp>

#include <com/sun/star/view/SelectionType.hpp>
#include <com/sun/star/form/binding/XBindableValue.hpp>
#include <com/sun/star/form/binding/XValueBinding.hpp>
#include <com/sun/star/form/binding/XListEntrySink.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/table/CellAddress.hpp>
#include <com/sun/star/table/CellRangeAddress.hpp>
#include <com/sun/star/document/XGraphicStorageHandler.hpp>
#include <com/sun/star/document/XStorageBasedDocument.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/util/NumberFormatsSupplier.hpp>

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;

namespace xmlscript
{

void EventElement::endElement()
{
    static_cast< ControlElement * >( m_pParent )->_events.emplace_back(this );
}

ControlElement::ControlElement(
    OUString const & rLocalName,
    Reference< xml::input::XAttributes > const & xAttributes,
    ElementBase * pParent, DialogImport * pImport )
    : ElementBase(
        pImport->XMLNS_DIALOGS_UID, rLocalName, xAttributes, pParent, pImport )
{
    if (m_pParent)
    {
        // inherit position
        _nBasePosX = static_cast< ControlElement * >( m_pParent )->_nBasePosX;
        _nBasePosY = static_cast< ControlElement * >( m_pParent )->_nBasePosY;
    }
    else
    {
        _nBasePosX = 0;
        _nBasePosY = 0;
    }
}

Reference< xml::input::XElement > ControlElement::getStyle(
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aStyleId( xAttributes->getValueByUidName( m_pImport->XMLNS_DIALOGS_UID,u"style-id"_ustr ) );
    if (!aStyleId.isEmpty())
    {
        return m_pImport->getStyle( aStyleId );
    }
    return Reference< xml::input::XElement >();
}

OUString ControlElement::getControlId(
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aId( xAttributes->getValueByUidName( m_pImport->XMLNS_DIALOGS_UID, u"id"_ustr ) );
    if (aId.isEmpty())
    {
        throw xml::sax::SAXException( u"missing id attribute!"_ustr, Reference< XInterface >(), Any() );
    }
    return aId;
}

OUString ControlElement::getControlModelName(
    OUString const& rDefaultModel,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aModel = xAttributes->getValueByUidName( m_pImport->XMLNS_DIALOGS_UID, u"control-implementation"_ustr);
    if (aModel.isEmpty())
        aModel = rDefaultModel;
    return aModel;
}

void StyleElement::importTextColorStyle(
    Reference< beans::XPropertySet > const & xProps )
{
    if ((_inited & 0x2) != 0)
    {
        if ((_hasValue & 0x2) != 0)
        {
            xProps->setPropertyValue(u"TextColor"_ustr, Any( _textColor ) );
        }
        return;
    }
    _inited |= 0x2;

    if (getLongAttr( &_textColor, u"text-color"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _hasValue |= 0x2;
        xProps->setPropertyValue( u"TextColor"_ustr, Any( _textColor ) );
        return;
    }
}

void StyleElement::importTextLineColorStyle(
    Reference< beans::XPropertySet > const & xProps )
{
    if ((_inited & 0x20) != 0)
    {
        if ((_hasValue & 0x20) != 0)
        {
            xProps->setPropertyValue( u"TextLineColor"_ustr, Any( _textLineColor ) );
        }
        return;
    }
    _inited |= 0x20;

    if (getLongAttr( &_textLineColor, u"textline-color"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _hasValue |= 0x20;
        xProps->setPropertyValue( u"TextLineColor"_ustr, Any( _textLineColor ) );
    }
}

void StyleElement::importFillColorStyle(
    Reference< beans::XPropertySet > const & xProps )
{
    if ((_inited & 0x10) != 0)
    {
        if ((_hasValue & 0x10) != 0)
        {
            xProps->setPropertyValue( u"FillColor"_ustr, Any( _fillColor ) );
        }
        return;
    }
    _inited |= 0x10;

    if (getLongAttr( &_fillColor, u"fill-color"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _hasValue |= 0x10;
        xProps->setPropertyValue( u"FillColor"_ustr, Any( _fillColor ) );
    }
}

void StyleElement::importBackgroundColorStyle(
    Reference< beans::XPropertySet > const & xProps )
{
    if ((_inited & 0x1) != 0)
    {
        if ((_hasValue & 0x1) != 0)
        {
            xProps->setPropertyValue( u"BackgroundColor"_ustr, Any( _backgroundColor ) );
        }
        return;
    }
    _inited |= 0x1;

    if (getLongAttr( &_backgroundColor, u"background-color"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _hasValue |= 0x1;
        xProps->setPropertyValue( u"BackgroundColor"_ustr, Any( _backgroundColor ) );
    }
}

void StyleElement::importBorderStyle(
    Reference< beans::XPropertySet > const & xProps )
{
    if ((_inited & 0x4) != 0)
    {
        if ((_hasValue & 0x4) != 0)
        {
            xProps->setPropertyValue( u"Border"_ustr, Any( _border == BORDER_SIMPLE_COLOR ? BORDER_SIMPLE : _border ) );
            if (_border == BORDER_SIMPLE_COLOR)
                xProps->setPropertyValue( u"BorderColor"_ustr, Any(_borderColor) );
        }
        return;
    }
    _inited |= 0x4;

    OUString aValue;
    if (!getStringAttr(&aValue, u"border"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
        return;

    if ( aValue == "none" )
        _border = BORDER_NONE;
    else if ( aValue == "3d" )
        _border = BORDER_3D;
    else if ( aValue == "simple" )
        _border = BORDER_SIMPLE;
    else {
        _border = BORDER_SIMPLE_COLOR;
        _borderColor = toInt32(aValue);
    }

    _hasValue |= 0x4;
    importBorderStyle(xProps); // write values
}

void StyleElement::importVisualEffectStyle(
    Reference<beans::XPropertySet> const & xProps )
{
    if ((_inited & 0x40) != 0)
    {
        if ((_hasValue & 0x40) != 0)
        {
            xProps->setPropertyValue( u"VisualEffect"_ustr, Any(_visualEffect) );
        }
        return;
    }
    _inited |= 0x40;

    OUString aValue;
    if (!getStringAttr( &aValue, u"look"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
        return;

    if ( aValue == "none" )
    {
        _visualEffect = awt::VisualEffect::NONE;
    }
    else if ( aValue == "3d" )
    {
        _visualEffect = awt::VisualEffect::LOOK3D;
    }
    else if ( aValue == "simple" )
    {
        _visualEffect = awt::VisualEffect::FLAT;
    }
    else
        OSL_ASSERT( false );

    _hasValue |= 0x40;
    xProps->setPropertyValue( u"VisualEffect"_ustr, Any(_visualEffect) );
}

void StyleElement::setFontProperties(
    Reference< beans::XPropertySet > const & xProps ) const
{
    xProps->setPropertyValue(u"FontDescriptor"_ustr, Any( _descr ) );
    xProps->setPropertyValue(u"FontEmphasisMark"_ustr, Any( _fontEmphasisMark ) );
    xProps->setPropertyValue(u"FontRelief"_ustr, Any( _fontRelief ) );
}

void StyleElement::importFontStyle(
    Reference< beans::XPropertySet > const & xProps )
{
    if ((_inited & 0x8) != 0)
    {
        if ((_hasValue & 0x8) != 0)
        {
            setFontProperties( xProps );
        }
        return;
    }
    _inited |= 0x8;

    OUString aValue;
    bool bFontImport;

    // dialog:font-name CDATA #IMPLIED
    bFontImport = getStringAttr( &_descr.Name, u"font-name"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID );

    // dialog:font-height %numeric; #IMPLIED
    if (getStringAttr( &aValue, u"font-height"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _descr.Height = static_cast<sal_Int16>(toInt32( aValue ));
        bFontImport = true;
    }
    // dialog:font-width %numeric; #IMPLIED
    if (getStringAttr(&aValue, u"font-width"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _descr.Width = static_cast<sal_Int16>(toInt32( aValue ));
        bFontImport = true;
    }
    // dialog:font-stylename CDATA #IMPLIED
    bFontImport |= getStringAttr( &_descr.StyleName, u"font-stylename"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID );

    // dialog:font-family "(decorative|modern|roman|script|swiss|system)" #IMPLIED
    if (getStringAttr(&aValue, u"font-family"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "decorative" )
        {
            _descr.Family = awt::FontFamily::DECORATIVE;
        }
        else if ( aValue == "modern" )
        {
            _descr.Family = awt::FontFamily::MODERN;
        }
        else if ( aValue == "roman" )
        {
            _descr.Family = awt::FontFamily::ROMAN;
        }
        else if ( aValue == "script" )
        {
            _descr.Family = awt::FontFamily::SCRIPT;
        }
        else if ( aValue == "swiss" )
        {
            _descr.Family = awt::FontFamily::SWISS;
        }
        else if ( aValue == "system" )
        {
            _descr.Family = awt::FontFamily::SYSTEM;
        }
        else
        {
            throw xml::sax::SAXException(u"invalid font-family style!"_ustr, Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }

    // dialog:font-charset "(ansi|mac|ibmpc_437|ibmpc_850|ibmpc_860|ibmpc_861|ibmpc_863|ibmpc_865|system|symbol)" #IMPLIED
    if (getStringAttr(&aValue, u"font-charset"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "ansi" )
        {
            _descr.CharSet = awt::CharSet::ANSI;
        }
        else if ( aValue == "mac" )
        {
            _descr.CharSet = awt::CharSet::MAC;
        }
        else if ( aValue == "ibmpc_437" )
        {
            _descr.CharSet = awt::CharSet::IBMPC_437;
        }
        else if ( aValue == "ibmpc_850" )
        {
            _descr.CharSet = awt::CharSet::IBMPC_850;
        }
        else if ( aValue == "ibmpc_860" )
        {
            _descr.CharSet = awt::CharSet::IBMPC_860;
        }
        else if ( aValue == "ibmpc_861" )
        {
            _descr.CharSet = awt::CharSet::IBMPC_861;
        }
        else if ( aValue == "ibmpc_863" )
        {
            _descr.CharSet = awt::CharSet::IBMPC_863;
        }
        else if ( aValue == "ibmpc_865" )
        {
            _descr.CharSet = awt::CharSet::IBMPC_865;
        }
        else if ( aValue == "system" )
        {
            _descr.CharSet = awt::CharSet::SYSTEM;
        }
        else if ( aValue == "symbol" )
        {
            _descr.CharSet = awt::CharSet::SYMBOL;
        }
        else
        {
            throw xml::sax::SAXException(u"invalid font-charset style!"_ustr, Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }

    // dialog:font-pitch "(fixed|variable)" #IMPLIED
    if (getStringAttr( &aValue, u"font-pitch"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "fixed" )
        {
            _descr.Pitch = awt::FontPitch::FIXED;
        }
        else if ( aValue == "variable" )
        {
            _descr.Pitch = awt::FontPitch::VARIABLE;
        }
        else
        {
            throw xml::sax::SAXException(u"invalid font-pitch style!"_ustr, Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }

    // dialog:font-charwidth CDATA #IMPLIED
    if (getStringAttr( &aValue, u"font-charwidth"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _descr.CharacterWidth = aValue.toFloat();
        bFontImport = true;
    }
    // dialog:font-weight CDATA #IMPLIED
    if (getStringAttr( &aValue, u"font-weight"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _descr.Weight = aValue.toFloat();
        bFontImport = true;
    }

    // dialog:font-slant "(oblique|italic|reverse_oblique|reverse_italic)" #IMPLIED
    if (getStringAttr( &aValue, u"font-slant"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "oblique" )
        {
            _descr.Slant = awt::FontSlant_OBLIQUE;
        }
        else if ( aValue == "italic" )
        {
            _descr.Slant = awt::FontSlant_ITALIC;
        }
        else if ( aValue == "reverse_oblique" )
        {
            _descr.Slant = awt::FontSlant_REVERSE_OBLIQUE;
        }
        else if ( aValue == "reverse_italic" )
        {
            _descr.Slant = awt::FontSlant_REVERSE_ITALIC;
        }
        else
        {
            throw xml::sax::SAXException(u"invalid font-slant style!"_ustr, Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }

    // dialog:font-underline "(single|double|dotted|dash|longdash|dashdot|dashdotdot|smallwave|wave|doublewave|bold|bolddotted|bolddash|boldlongdash|bolddashdot|bolddashdotdot|boldwave)" #IMPLIED
    if (getStringAttr( &aValue, u"font-underline"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "single" )
        {
            _descr.Underline = awt::FontUnderline::SINGLE;
        }
        else if ( aValue == "double" )
        {
            _descr.Underline = awt::FontUnderline::DOUBLE;
        }
        else if ( aValue == "dotted" )
        {
            _descr.Underline = awt::FontUnderline::DOTTED;
        }
        else if ( aValue == "dash" )
        {
            _descr.Underline = awt::FontUnderline::DASH;
        }
        else if ( aValue == "longdash" )
        {
            _descr.Underline = awt::FontUnderline::LONGDASH;
        }
        else if ( aValue == "dashdot" )
        {
            _descr.Underline = awt::FontUnderline::DASHDOT;
        }
        else if ( aValue == "dashdotdot" )
        {
            _descr.Underline = awt::FontUnderline::DASHDOTDOT;
        }
        else if ( aValue == "smallwave" )
        {
            _descr.Underline = awt::FontUnderline::SMALLWAVE;
        }
        else if ( aValue == "wave" )
        {
            _descr.Underline = awt::FontUnderline::WAVE;
        }
        else if ( aValue == "doublewave" )
        {
            _descr.Underline = awt::FontUnderline::DOUBLEWAVE;
        }
        else if ( aValue == "bold" )
        {
            _descr.Underline = awt::FontUnderline::BOLD;
        }
        else if ( aValue == "bolddotted" )
        {
            _descr.Underline = awt::FontUnderline::BOLDDOTTED;
        }
        else if ( aValue == "bolddash" )
        {
            _descr.Underline = awt::FontUnderline::BOLDDASH;
        }
        else if ( aValue == "boldlongdash" )
        {
            _descr.Underline = awt::FontUnderline::BOLDLONGDASH;
        }
        else if ( aValue == "bolddashdot" )
        {
            _descr.Underline = awt::FontUnderline::BOLDDASHDOT;
        }
        else if ( aValue == "bolddashdotdot" )
        {
            _descr.Underline = awt::FontUnderline::BOLDDASHDOTDOT;
        }
        else if ( aValue == "boldwave" )
        {
            _descr.Underline = awt::FontUnderline::BOLDWAVE;
        }
        else
        {
            throw xml::sax::SAXException(u"invalid font-underline style!"_ustr, Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }

    // dialog:font-strikeout "(single|double|bold|slash|x)" #IMPLIED
    if (getStringAttr( &aValue, u"font-strikeout"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "single" )
        {
            _descr.Strikeout = awt::FontStrikeout::SINGLE;
        }
        else if ( aValue == "double" )
        {
            _descr.Strikeout = awt::FontStrikeout::DOUBLE;
        }
        else if ( aValue == "bold" )
        {
            _descr.Strikeout = awt::FontStrikeout::BOLD;
        }
        else if ( aValue == "slash" )
        {
            _descr.Strikeout = awt::FontStrikeout::SLASH;
        }
        else if ( aValue == "x" )
        {
            _descr.Strikeout = awt::FontStrikeout::X;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid font-strikeout style!"_ustr , Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }

    // dialog:font-orientation CDATA #IMPLIED
    if (getStringAttr( &aValue, u"font-orientation"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        _descr.Orientation = aValue.toFloat();
        bFontImport = true;
    }
    // dialog:font-kerning %boolean; #IMPLIED
    bFontImport |= getBoolAttr( &_descr.Kerning, u"font-kerning"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID );
    // dialog:font-wordlinemode %boolean; #IMPLIED
    bFontImport |= getBoolAttr( &_descr.WordLineMode,u"font-wordlinemode"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID );

    // dialog:font-type "(raster|device|scalable)" #IMPLIED
    if (getStringAttr( &aValue, u"font-type"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "raster" )
        {
            _descr.Type = awt::FontType::RASTER;
        }
        else if ( aValue == "device" )
        {
            _descr.Type = awt::FontType::DEVICE;
        }
        else if ( aValue == "scalable" )
        {
            _descr.Type = awt::FontType::SCALABLE;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid font-type style!"_ustr, Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }

    // additional properties which are not part of the FontDescriptor struct
    // dialog:font-relief (none|embossed|engraved) #IMPLIED
    if (getStringAttr( &aValue, u"font-relief"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "none" )
        {
            _fontRelief = awt::FontRelief::NONE;
        }
        else if ( aValue == "embossed" )
        {
            _fontRelief = awt::FontRelief::EMBOSSED;
        }
        else if ( aValue == "engraved" )
        {
            _fontRelief = awt::FontRelief::ENGRAVED;
        }
        else
        {
            throw xml::sax::SAXException(u"invalid font-relief style!"_ustr, Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }
    // dialog:font-emphasismark (none|dot|circle|disc|accent|above|below) #IMPLIED
    if (getStringAttr(&aValue, u"font-emphasismark"_ustr, _xAttributes, m_pImport->XMLNS_DIALOGS_UID ))
    {
        if ( aValue == "none" )
        {
            _fontEmphasisMark = awt::FontEmphasisMark::NONE;
        }
        else if ( aValue == "dot" )
        {
            _fontEmphasisMark = awt::FontEmphasisMark::DOT;
        }
        else if ( aValue == "circle" )
        {
            _fontEmphasisMark = awt::FontEmphasisMark::CIRCLE;
        }
        else if ( aValue == "disc" )
        {
            _fontEmphasisMark = awt::FontEmphasisMark::DISC;
        }
        else if ( aValue == "accent" )
        {
            _fontEmphasisMark = awt::FontEmphasisMark::ACCENT;
        }
        else if ( aValue == "above" )
        {
            _fontEmphasisMark = awt::FontEmphasisMark::ABOVE;
        }
        else if ( aValue == "below" )
        {
            _fontEmphasisMark = awt::FontEmphasisMark::BELOW;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid font-emphasismark style!"_ustr, Reference< XInterface >(), Any() );
        }
        bFontImport = true;
    }

    if (bFontImport)
    {
        _hasValue |= 0x8;
        setFontProperties( xProps );
    }
}

bool ImportContext::importStringProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aValue(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aValue.isEmpty())
    {
        _xControlModel->setPropertyValue( rPropName, Any( aValue ) );
        return true;
    }
    return false;
}

bool ImportContext::importDoubleProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aValue(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aValue.isEmpty())
    {
        _xControlModel->setPropertyValue( rPropName, Any( aValue.toDouble() ) );
        return true;
    }
    return false;
}

bool ImportContext::importBooleanProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    sal_Bool bBool;
    if (getBoolAttr(
            &bBool, rAttrName, xAttributes, _pImport->XMLNS_DIALOGS_UID ))
    {
        _xControlModel->setPropertyValue( rPropName, Any( bBool ) );
        return true;
    }
    return false;
}

bool ImportContext::importLongProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aValue(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aValue.isEmpty())
    {
        _xControlModel->setPropertyValue( rPropName, Any( toInt32( aValue ) ) );
        return true;
    }
    return false;
}

bool ImportContext::importLongProperty(
    sal_Int32 nOffset,
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aValue(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aValue.isEmpty())
    {
        _xControlModel->setPropertyValue( rPropName, Any( toInt32( aValue ) + nOffset ) );
        return true;
    }
    return false;
}

bool ImportContext::importHexLongProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aValue(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aValue.isEmpty())
    {
        _xControlModel->setPropertyValue( rPropName, Any( toInt32( aValue ) ) );
        return true;
    }
    return false;
}

bool ImportContext::importShortProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aValue(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aValue.isEmpty())
    {
        _xControlModel->setPropertyValue( rPropName, Any( static_cast<sal_Int16>(toInt32( aValue )) ) );
        return true;
    }
    return false;
}

bool ImportContext::importAlignProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aAlign(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aAlign.isEmpty())
    {
        sal_Int16 nAlign;
        if ( aAlign == "left" )
        {
            nAlign = 0;
        }
        else if ( aAlign == "center" )
        {
            nAlign = 1;
        }
        else if ( aAlign == "right" )
        {
            nAlign = 2;
        }
        else if ( aAlign == "none" )
        {
            nAlign = 0; // default
        }
        else
        {
            throw xml::sax::SAXException(u"invalid align value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( nAlign ) );
        return true;
    }
    return false;
}

bool ImportContext::importVerticalAlignProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aAlign(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aAlign.isEmpty())
    {
        style::VerticalAlignment eAlign;

        if ( aAlign == "top" )
        {
            eAlign = style::VerticalAlignment_TOP;
        }
        else if ( aAlign == "center" )
        {
            eAlign = style::VerticalAlignment_MIDDLE;
        }
        else if ( aAlign == "bottom" )
        {
            eAlign = style::VerticalAlignment_BOTTOM;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid vertical align value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( eAlign ) );
        return true;
    }
    return false;
}

bool ImportContext::importGraphicOrImageProperty(
    OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString sURL = xAttributes->getValueByUidName( _pImport->XMLNS_DIALOGS_UID, rAttrName );
    if ( !sURL.isEmpty() )
    {
        Reference< document::XStorageBasedDocument > xDocStorage( _pImport->getDocOwner(), UNO_QUERY );

        uno::Reference<graphic::XGraphic> xGraphic;

        uno::Reference<document::XGraphicStorageHandler> xGraphicStorageHandler;
        if ( xDocStorage.is() )
        {
            uno::Sequence< Any > aArgs{ Any(xDocStorage->getDocumentStorage()) };
            xGraphicStorageHandler.set(
                _pImport->getComponentContext()->getServiceManager()->createInstanceWithArgumentsAndContext( u"com.sun.star.comp.Svx.GraphicImportHelper"_ustr , aArgs, _pImport->getComponentContext() ),
                UNO_QUERY );
            if (xGraphicStorageHandler.is())
            {
                try
                {
                    xGraphic = xGraphicStorageHandler->loadGraphic(sURL);
                }
                catch( const uno::Exception& )
                {
                    return false;
                }
            }
        }
        if (xGraphic.is())
        {
            Reference<beans::XPropertySet> xProps = getControlModel();
            if (xProps.is())
            {
                xProps->setPropertyValue(u"Graphic"_ustr, Any(xGraphic));
                return true;
            }
        }
        else if (!sURL.isEmpty())
        {
            // tdf#130793 Above fails if the dialog is not part of a document.
            // In this case we need to set the ImageURL.
            Reference<beans::XPropertySet> xProps = getControlModel();
            if (xProps.is())
            {
                xProps->setPropertyValue(u"ImageURL"_ustr, Any(sURL));
                return true;
            }
        }
    }
    return false;
}

bool ImportContext::importDataAwareProperty(
        OUString const & rPropName,
        Reference<xml::input::XAttributes> const & xAttributes )
{
    OUString sLinkedCell;
    OUString sCellRange;
    if ( rPropName == "linked-cell" )
       sLinkedCell = xAttributes->getValueByUidName( _pImport->XMLNS_DIALOGS_UID, rPropName );
    if ( rPropName == "source-cell-range" )
        sCellRange = xAttributes->getValueByUidName( _pImport->XMLNS_DIALOGS_UID, rPropName );
    bool bRes = false;
    Reference< lang::XMultiServiceFactory > xFac( _pImport->getDocOwner(), UNO_QUERY );
    if ( xFac.is() && ( !sLinkedCell.isEmpty() ||  !sCellRange.isEmpty() ) )
    {
        // Set up cell link
        if ( !sLinkedCell.isEmpty() )
        {
            Reference< form::binding::XBindableValue > xBindable( getControlModel(), uno::UNO_QUERY );
            Reference< beans::XPropertySet > xConvertor( xFac->createInstance( u"com.sun.star.table.CellAddressConversion"_ustr ), uno::UNO_QUERY );
            if ( xBindable.is() && xConvertor.is() )
            {
                table::CellAddress aAddress;
                xConvertor->setPropertyValue( u"PersistentRepresentation"_ustr , uno::Any( sLinkedCell ) );
                xConvertor->getPropertyValue( u"Address"_ustr ) >>= aAddress;
                beans::NamedValue aArg1;
                aArg1.Name = "BoundCell";
                aArg1.Value <<= aAddress;

                uno::Reference< form::binding::XValueBinding > xBinding( xFac->createInstanceWithArguments( u"com.sun.star.table.CellValueBinding"_ustr , { uno::Any(aArg1) }), uno::UNO_QUERY );
                xBindable->setValueBinding( xBinding );
                bRes = true;
            }
        }
        // Set up CellRange
        if ( !sCellRange.isEmpty() )
        {
            Reference< form::binding::XListEntrySink  > xListEntrySink( getControlModel(), uno::UNO_QUERY );
            Reference< beans::XPropertySet > xConvertor( xFac->createInstance( u"com.sun.star.table.CellRangeAddressConversion"_ustr ), uno::UNO_QUERY );
            if ( xListEntrySink.is() && xConvertor.is() )
            {
                table::CellRangeAddress aAddress;
                xConvertor->setPropertyValue( u"PersistentRepresentation"_ustr , uno::Any( sCellRange ) );
                xConvertor->getPropertyValue( u"Address"_ustr ) >>= aAddress;
                beans::NamedValue aArg1;
                aArg1.Name = "CellRange";
                aArg1.Value <<= aAddress;

                uno::Reference< form::binding::XListEntrySource > xSource( xFac->createInstanceWithArguments( u"com.sun.star.table.CellRangeListSource"_ustr , { uno::Any(aArg1) } ), uno::UNO_QUERY );
                xListEntrySink->setListEntrySource( xSource );
                bRes = true;
            }
        }
    }
    return bRes;
}

bool ImportContext::importImageAlignProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aAlign(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aAlign.isEmpty())
    {
        sal_Int16 nAlign;
        if ( aAlign == "left" )
        {
            nAlign = 0;
        }
        else if ( aAlign == "top" )
        {
            nAlign = 1;
        }
        else if ( aAlign == "right" )
        {
            nAlign = 2;
        }
        else if ( aAlign == "bottom" )
        {
            nAlign = 3;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid image align value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( nAlign ) );
        return true;
    }
    return false;
}

bool ImportContext::importImagePositionProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aPosition(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aPosition.isEmpty())
    {
        sal_Int16 nPosition;
        if ( aPosition == "left-top" )
        {
            nPosition = awt::ImagePosition::LeftTop;
        }
        else if ( aPosition == "left-center" )
        {
            nPosition = awt::ImagePosition::LeftCenter;
        }
        else if ( aPosition == "left-bottom" )
        {
            nPosition = awt::ImagePosition::LeftBottom;
        }
        else if ( aPosition == "right-top" )
        {
            nPosition = awt::ImagePosition::RightTop;
        }
        else if ( aPosition == "right-center" )
        {
            nPosition = awt::ImagePosition::RightCenter;
        }
        else if ( aPosition == "right-bottom" )
        {
            nPosition = awt::ImagePosition::RightBottom;
        }
        else if ( aPosition == "top-left" )
        {
            nPosition = awt::ImagePosition::AboveLeft;
        }
        else if ( aPosition == "top-center" )
        {
            nPosition = awt::ImagePosition::AboveCenter;
        }
        else if ( aPosition == "top-right" )
        {
            nPosition = awt::ImagePosition::AboveRight;
        }
        else if ( aPosition == "bottom-left" )
        {
            nPosition = awt::ImagePosition::BelowLeft;
        }
        else if ( aPosition == "bottom-center" )
        {
            nPosition = awt::ImagePosition::BelowCenter;
        }
        else if ( aPosition == "bottom-right" )
        {
            nPosition = awt::ImagePosition::BelowRight;
        }
        else if ( aPosition == "center" )
        {
            nPosition = awt::ImagePosition::Centered;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid image position value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( nPosition ) );
        return true;
    }
    return false;
}

bool ImportContext::importButtonTypeProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString buttonType(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!buttonType.isEmpty())
    {
        awt::PushButtonType nButtonType;
        if ( buttonType == "standard" )
        {
            nButtonType = awt::PushButtonType_STANDARD;
        }
        else if ( buttonType == "ok" )
        {
            nButtonType = awt::PushButtonType_OK;
        }
        else if ( buttonType == "cancel" )
        {
            nButtonType = awt::PushButtonType_CANCEL;
        }
        else if ( buttonType == "help" )
        {
            nButtonType = awt::PushButtonType_HELP;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid button-type value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( static_cast<sal_Int16>(nButtonType) ) );
        return true;
    }
    return false;
}

bool ImportContext::importDateFormatProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aFormat(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aFormat.isEmpty())
    {
        sal_Int16 nFormat;
        if ( aFormat == "system_short" )
        {
            nFormat = 0;
        }
        else if ( aFormat == "system_short_YY" )
        {
            nFormat = 1;
        }
        else if ( aFormat == "system_short_YYYY" )
        {
            nFormat = 2;
        }
        else if ( aFormat == "system_long" )
        {
            nFormat = 3;
        }
        else if ( aFormat == "short_DDMMYY" )
        {
            nFormat = 4;
        }
        else if ( aFormat == "short_MMDDYY" )
        {
            nFormat = 5;
        }
        else if ( aFormat == "short_YYMMDD" )
        {
            nFormat = 6;
        }
        else if ( aFormat == "short_DDMMYYYY" )
        {
            nFormat = 7;
        }
        else if ( aFormat == "short_MMDDYYYY" )
        {
            nFormat = 8;
        }
        else if ( aFormat == "short_YYYYMMDD" )
        {
            nFormat = 9;
        }
        else if ( aFormat == "short_YYMMDD_DIN5008" )
        {
            nFormat = 10;
        }
        else if ( aFormat == "short_YYYYMMDD_DIN5008" )
        {
            nFormat = 11;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid date-format value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( nFormat ) );
        return true;
    }
    return false;
}

bool ImportContext::importTimeProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aValue(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aValue.isEmpty())
    {
        // Is it really the legacy "encoded value in centiseconds"?
        tools::Time aTTime(tools::Time::fromEncodedTime(toInt32(aValue) * tools::Time::nanoPerCenti));
        util::Time aUTime(aTTime.GetUNOTime());
        _xControlModel->setPropertyValue( rPropName, Any( aUTime ) );
        return true;
    }
    return false;
}

bool ImportContext::importDateProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aValue(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aValue.isEmpty())
    {
        ::Date aTDate(toInt32( aValue ));
        util::Date aUDate(aTDate.GetUNODate());
        _xControlModel->setPropertyValue( rPropName, Any( aUDate ) );
        return true;
    }
    return false;
}

bool ImportContext::importTimeFormatProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aFormat(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aFormat.isEmpty())
    {
        sal_Int16 nFormat;
        if ( aFormat == "24h_short" )
        {
            nFormat = 0;
        }
        else if ( aFormat == "24h_long" )
        {
            nFormat = 1;
        }
        else if ( aFormat == "12h_short" )
        {
            nFormat = 2;
        }
        else if ( aFormat == "12h_long" )
        {
            nFormat = 3;
        }
        else if ( aFormat == "Duration_short" )
        {
            nFormat = 4;
        }
        else if ( aFormat == "Duration_long" )
        {
            nFormat = 5;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid time-format value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( nFormat ) );
        return true;
    }
    return false;
}

bool ImportContext::importOrientationProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aOrient(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aOrient.isEmpty())
    {
        sal_Int32 nOrient;
        if ( aOrient == "horizontal" )
        {
            nOrient = 0;
        }
        else if ( aOrient == "vertical" )
        {
            nOrient = 1;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid orientation value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( nOrient ) );
        return true;
    }
    return false;
}

bool ImportContext::importLineEndFormatProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aFormat(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aFormat.isEmpty())
    {
        sal_Int16 nFormat;
        if ( aFormat == "carriage-return" )
        {
            nFormat = awt::LineEndFormat::CARRIAGE_RETURN;
        }
        else if ( aFormat == "line-feed" )
        {
            nFormat = awt::LineEndFormat::LINE_FEED;
        }
        else if ( aFormat == "carriage-return-line-feed" )
        {
            nFormat = awt::LineEndFormat::CARRIAGE_RETURN_LINE_FEED;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid line end format value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( nFormat ) );
        return true;
    }
    return false;
}

bool ImportContext::importSelectionTypeProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aSelectionType(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aSelectionType.isEmpty())
    {
        view::SelectionType eSelectionType;

        if ( aSelectionType == "none" )
        {
            eSelectionType = view::SelectionType_NONE;
        }
        else if ( aSelectionType == "single" )
        {
            eSelectionType = view::SelectionType_SINGLE;
        }
        else if ( aSelectionType == "multi" )
        {
            eSelectionType = view::SelectionType_MULTI;
        }
        else  if ( aSelectionType == "range" )
        {
            eSelectionType = view::SelectionType_RANGE;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid selection type value!"_ustr, Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( eSelectionType ) );
        return true;
    }
    return false;
}

bool ImportContext::importImageScaleModeProperty(
    OUString const & rPropName, OUString const & rAttrName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    OUString aImageScaleMode(
        xAttributes->getValueByUidName(
            _pImport->XMLNS_DIALOGS_UID, rAttrName ) );
    if (!aImageScaleMode.isEmpty())
    {
        sal_Int16 nImageScaleMode;

        if (aImageScaleMode == "none")
        {
            nImageScaleMode = awt::ImageScaleMode::NONE;
        }
        else if (aImageScaleMode == "isotropic")
        {
            nImageScaleMode = awt::ImageScaleMode::ISOTROPIC;
        }
        else if (aImageScaleMode == "anisotropic")
        {
            nImageScaleMode = awt::ImageScaleMode::ANISOTROPIC;
        }
        else
        {
            throw xml::sax::SAXException( u"invalid scale image mode value!"_ustr,
                Reference< XInterface >(), Any() );
        }

        _xControlModel->setPropertyValue( rPropName, Any( nImageScaleMode ) );
        return true;
    }
    return false;
}

StringTriple const s_aEventTranslations[] =
{
    // from xmloff/source/forms/formevents.cxx
    // 28.09.2001 tbe added on-adjustmentvaluechange
    { "com.sun.star.form.XApproveActionListener", "approveAction", "on-approveaction" },
    { "com.sun.star.awt.XActionListener", "actionPerformed", "on-performaction" },
    { "com.sun.star.form.XChangeListener", "changed", "on-change" },
    { "com.sun.star.awt.XTextListener", "textChanged", "on-textchange" },
    { "com.sun.star.awt.XItemListener", "itemStateChanged", "on-itemstatechange" },
    { "com.sun.star.awt.XFocusListener", "focusGained", "on-focus" },
    { "com.sun.star.awt.XFocusListener", "focusLost", "on-blur" },
    { "com.sun.star.awt.XKeyListener", "keyPressed", "on-keydown" },
    { "com.sun.star.awt.XKeyListener", "keyReleased", "on-keyup" },
    { "com.sun.star.awt.XMouseListener", "mouseEntered", "on-mouseover" },
    { "com.sun.star.awt.XMouseMotionListener", "mouseDragged", "on-mousedrag" },
    { "com.sun.star.awt.XMouseMotionListener", "mouseMoved", "on-mousemove" },
    { "com.sun.star.awt.XMouseListener", "mousePressed", "on-mousedown" },
    { "com.sun.star.awt.XMouseListener", "mouseReleased", "on-mouseup" },
    { "com.sun.star.awt.XMouseListener", "mouseExited", "on-mouseout" },
    { "com.sun.star.form.XResetListener", "approveReset", "on-approvereset" },
    { "com.sun.star.form.XResetListener", "resetted", "on-reset" },
    { "com.sun.star.form.XSubmitListener", "approveSubmit", "on-submit" },
    { "com.sun.star.form.XUpdateListener", "approveUpdate", "on-approveupdate" },
    { "com.sun.star.form.XUpdateListener", "updated", "on-update" },
    { "com.sun.star.form.XLoadListener", "loaded", "on-load" },
    { "com.sun.star.form.XLoadListener", "reloading", "on-startreload" },
    { "com.sun.star.form.XLoadListener", "reloaded", "on-reload" },
    { "com.sun.star.form.XLoadListener", "unloading", "on-startunload" },
    { "com.sun.star.form.XLoadListener", "unloaded", "on-unload" },
    { "com.sun.star.form.XConfirmDeleteListener", "confirmDelete", "on-confirmdelete" },
    { "com.sun.star.sdb.XRowSetApproveListener", "approveRowChange", "on-approverowchange" },
    { "com.sun.star.sdbc.XRowSetListener", "rowChanged", "on-rowchange" },
    { "com.sun.star.sdb.XRowSetApproveListener", "approveCursorMove", "on-approvecursormove" },
    { "com.sun.star.sdbc.XRowSetListener", "cursorMoved", "on-cursormove" },
    { "com.sun.star.form.XDatabaseParameterListener", "approveParameter", "on-supplyparameter" },
    { "com.sun.star.sdb.XSQLErrorListener", "errorOccured", "on-error" },
    { "com.sun.star.awt.XAdjustmentListener", "adjustmentValueChanged", "on-adjustmentvaluechange" },
    { nullptr, nullptr, nullptr }
};

StringTriple const * const g_pEventTranslations = s_aEventTranslations;

void ImportContext::importEvents(
    std::vector< Reference< xml::input::XElement > > const & rEvents )
{
    Reference< script::XScriptEventsSupplier > xSupplier(
        _xControlModel, UNO_QUERY );
    if (!xSupplier.is())
        return;

    Reference< container::XNameContainer > xEvents( xSupplier->getEvents() );
    if (!xEvents.is())
        return;

    for (const auto & rEvent : rEvents)
    {
        script::ScriptEventDescriptor descr;

        EventElement * pEventElement = static_cast< EventElement * >( rEvent.get() );
        sal_Int32 nUid = pEventElement->getUid();
        OUString aLocalName( pEventElement->getLocalName() );
        Reference< xml::input::XAttributes > xAttributes( pEventElement->getAttributes() );

        // nowadays script events
        if (_pImport->XMLNS_SCRIPT_UID == nUid)
        {
            if (!getStringAttr( &descr.ScriptType, u"language"_ustr  , xAttributes, _pImport->XMLNS_SCRIPT_UID ) ||
                !getStringAttr( &descr.ScriptCode, u"macro-name"_ustr, xAttributes, _pImport->XMLNS_SCRIPT_UID ))
            {
                throw xml::sax::SAXException( u"missing language or macro-name attribute(s) of event!"_ustr, Reference< XInterface >(), Any() );
            }
            if ( descr.ScriptType == "StarBasic" )
            {
                OUString aLocation;
                if (getStringAttr( &aLocation, u"location"_ustr, xAttributes, _pImport->XMLNS_SCRIPT_UID ))
                {
                    // prepend location
                    descr.ScriptCode = aLocation + ":" + descr.ScriptCode;
                }
            }
            else if ( descr.ScriptType == "Script" )
            {
                // Check if there is a protocol, if not assume
                // this is an early scripting framework url ( without
                // the protocol ) and fix it up!!
                if ( descr.ScriptCode.indexOf( ':' ) == -1 )
                {
                    descr.ScriptCode = "vnd.sun.start.script:" + descr.ScriptCode;
                }
            }

            // script:event element
            if ( aLocalName == "event" )
            {
                OUString aEventName;
                if (! getStringAttr( &aEventName, u"event-name"_ustr, xAttributes, _pImport->XMLNS_SCRIPT_UID ))
                {
                    throw xml::sax::SAXException( u"missing event-name attribute!"_ustr, Reference< XInterface >(), Any() );
                }

                // lookup in table
                OString str( OUStringToOString( aEventName, RTL_TEXTENCODING_ASCII_US ) );
                StringTriple const * p = g_pEventTranslations;
                while (p->first)
                {
                    if (0 == ::rtl_str_compare( p->third, str.getStr() ))
                    {
                        descr.ListenerType = OUString(
                            p->first, ::rtl_str_getLength( p->first ),
                            RTL_TEXTENCODING_ASCII_US );
                        descr.EventMethod = OUString(
                            p->second, ::rtl_str_getLength( p->second ),
                            RTL_TEXTENCODING_ASCII_US );
                        break;
                    }
                    ++p;
                }

                if (! p->first)
                {
                    throw xml::sax::SAXException( u"no matching event-name found!"_ustr, Reference< XInterface >(), Any() );
                }
            }
            else // script:listener-event element
            {
                SAL_WARN_IF( aLocalName != "listener-event", "xmlscript.xmldlg", "aLocalName != listener-event" );

                if (!getStringAttr( &descr.ListenerType, u"listener-type"_ustr  , xAttributes, _pImport->XMLNS_SCRIPT_UID ) ||
                    !getStringAttr( &descr.EventMethod , u"listener-method"_ustr, xAttributes, _pImport->XMLNS_SCRIPT_UID ))
                {
                    throw xml::sax::SAXException(u"missing listener-type or listener-method attribute(s)!"_ustr, Reference< XInterface >(), Any() );
                }
                // optional listener param
                getStringAttr( &descr.AddListenerParam,  u"listener-param"_ustr, xAttributes, _pImport->XMLNS_SCRIPT_UID );
            }
        }
        else // deprecated dlg:event element
        {
            SAL_WARN_IF( _pImport->XMLNS_DIALOGS_UID != nUid || aLocalName != "event", "xmlscript.xmldlg", "_pImport->XMLNS_DIALOGS_UID != nUid || aLocalName != \"event\"" );

            if (!getStringAttr( &descr.ListenerType, u"listener-type"_ustr, xAttributes, _pImport->XMLNS_DIALOGS_UID ) ||
                !getStringAttr( &descr.EventMethod,  u"event-method"_ustr,  xAttributes, _pImport->XMLNS_DIALOGS_UID ))
            {
                throw xml::sax::SAXException(u"missing listener-type or event-method attribute(s)!"_ustr, Reference< XInterface >(), Any() );
            }

            getStringAttr( &descr.ScriptType, u"script-type"_ustr, xAttributes, _pImport->XMLNS_DIALOGS_UID );
            getStringAttr( &descr.ScriptCode, u"script-code"_ustr, xAttributes, _pImport->XMLNS_DIALOGS_UID );
            getStringAttr( &descr.AddListenerParam, u"param"_ustr, xAttributes, _pImport->XMLNS_DIALOGS_UID );
        }

        xEvents->insertByName( descr.ListenerType + "::" + descr.EventMethod, Any( descr ) );
    }
}
void ImportContext::importScollableSettings(
    Reference< xml::input::XAttributes > const & _xAttributes )
{
    importLongProperty( u"ScrollHeight"_ustr,
                        u"scrollheight"_ustr,
                        _xAttributes );
    importLongProperty( u"ScrollWidth"_ustr,
                        u"scrollwidth"_ustr,
                        _xAttributes );
    importLongProperty( u"ScrollTop"_ustr,
                        u"scrolltop"_ustr,
                        _xAttributes );
    importLongProperty( u"ScrollLeft"_ustr,
                        u"scrollleft"_ustr,
                        _xAttributes );
    importBooleanProperty( u"HScroll"_ustr,
                           u"hscroll"_ustr,
                           _xAttributes );
    importBooleanProperty( u"VScroll"_ustr,
                           u"vscroll"_ustr,
                           _xAttributes );
}

void ImportContext::importDefaults(
    sal_Int32 nBaseX, sal_Int32 nBaseY,
    Reference< xml::input::XAttributes > const & xAttributes,
    bool supportPrintable )
{
    _xControlModel->setPropertyValue( u"Name"_ustr, Any( _aId ) );

    importShortProperty( u"TabIndex"_ustr, u"tab-index"_ustr, xAttributes );

    sal_Bool bDisable = false;
    if (getBoolAttr( &bDisable,u"disabled"_ustr, xAttributes, _pImport->XMLNS_DIALOGS_UID ) && bDisable)
    {
        _xControlModel->setPropertyValue( u"Enabled"_ustr, Any( false ) );
    }

    sal_Bool bVisible = true;
    if (getBoolAttr( &bVisible, u"visible"_ustr, xAttributes, _pImport->XMLNS_DIALOGS_UID ) && !bVisible)
    {
        try
        {
                _xControlModel->setPropertyValue( u"EnableVisible"_ustr, Any( false ) );
        }
        catch( Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("xmlscript.xmldlg");
        }
    }

    if (!importLongProperty( nBaseX, u"PositionX"_ustr, u"left"_ustr, xAttributes ) ||
        !importLongProperty( nBaseY, u"PositionY"_ustr, u"top"_ustr,  xAttributes ) ||
        !importLongProperty( u"Width"_ustr, u"width"_ustr, xAttributes ) ||
        !importLongProperty( u"Height"_ustr, u"height"_ustr, xAttributes ))
    {
        throw xml::sax::SAXException( u"missing pos size attribute(s)!"_ustr, Reference< XInterface >(), Any() );
    }

    if (supportPrintable)
    {
        importBooleanProperty(u"Printable"_ustr, u"printable"_ustr, xAttributes );
    }

    sal_Int32 nLong;
    if (! getLongAttr( &nLong, u"page"_ustr, xAttributes, _pImport->XMLNS_DIALOGS_UID ))
    {
        nLong = 0;
    }
    _xControlModel->setPropertyValue( u"Step"_ustr, Any( nLong ) );

    importStringProperty(u"Tag"_ustr, u"tag"_ustr, xAttributes );
    importStringProperty( u"HelpText"_ustr, u"help-text"_ustr, xAttributes );
    importStringProperty( u"HelpURL"_ustr, u"help-url"_ustr, xAttributes );
}

Reference< xml::input::XElement > ElementBase::getParent()
{
    return static_cast< xml::input::XElement * >( m_pParent );
}

OUString ElementBase::getLocalName()
{
    return _aLocalName;
}

sal_Int32 ElementBase::getUid()
{
    return _nUid;
}

Reference< xml::input::XAttributes > ElementBase::getAttributes()
{
    return _xAttributes;
}

void ElementBase::ignorableWhitespace(
    OUString const & /*rWhitespaces*/ )
{
    // not used
}

void ElementBase::characters( OUString const & /*rChars*/ )
{
    // not used, all characters ignored
}

void ElementBase::endElement()
{
}

void ElementBase::processingInstruction(
    OUString const & /*Target*/, OUString const & /*Data*/ )
{
}

Reference< xml::input::XElement > ElementBase::startChildElement(
    sal_Int32 /*nUid*/, OUString const & /*rLocalName*/,
    Reference< xml::input::XAttributes > const & /*xAttributes*/ )
{
    throw xml::sax::SAXException( u"unexpected element!"_ustr, Reference< XInterface >(), Any() );
}

ElementBase::ElementBase(
    sal_Int32 nUid, OUString aLocalName,
    Reference< xml::input::XAttributes > const & xAttributes,
    ElementBase * pParent, DialogImport * pImport )
    : m_pImport( pImport )
    , m_pParent( pParent )
    , _nUid( nUid )
    , _aLocalName(std::move(aLocalName ))
    , _xAttributes( xAttributes )
{
}

ElementBase::~ElementBase()
{
    SAL_INFO("xmlscript.xmldlg", "ElementBase::~ElementBase(): " << _aLocalName );
}

// XRoot

void DialogImport::startDocument(
    Reference< xml::input::XNamespaceMapping > const & xNamespaceMapping )
{
    XMLNS_DIALOGS_UID = xNamespaceMapping->getUidByUri( XMLNS_DIALOGS_URI );
    XMLNS_SCRIPT_UID = xNamespaceMapping->getUidByUri( XMLNS_SCRIPT_URI );
}

void DialogImport::endDocument()
{
    // ignored
}

void DialogImport::processingInstruction(
    OUString const & /*rTarget*/, OUString const & /*rData*/ )
{
    // ignored for now: xxx todo
}

void DialogImport::setDocumentLocator(
    Reference< xml::sax::XLocator > const & /*xLocator*/ )
{
    // ignored for now: xxx todo
}

Reference< xml::input::XElement > DialogImport::startRootElement(
    sal_Int32 nUid, OUString const & rLocalName,
    Reference< xml::input::XAttributes > const & xAttributes )
{
    if (XMLNS_DIALOGS_UID != nUid)
    {
        throw xml::sax::SAXException( u"illegal namespace!"_ustr, Reference< XInterface >(), Any() );
    }
    // window
    else if ( rLocalName == "window" )
    {
        return new WindowElement( rLocalName, xAttributes, this );
    }
    else
    {
        throw xml::sax::SAXException( "illegal root element (expected window) given: " + rLocalName, Reference< XInterface >(), Any() );
    }
}

DialogImport::~DialogImport()
{
    SAL_INFO("xmlscript.xmldlg", "DialogImport::~DialogImport()." );
}

Reference< util::XNumberFormatsSupplier > const & DialogImport::getNumberFormatsSupplier()
{
    if (! _xSupplier.is())
    {
        Reference< util::XNumberFormatsSupplier > xSupplier = util::NumberFormatsSupplier::createWithDefaultLocale( getComponentContext() );

        ::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
        if (! _xSupplier.is())
        {
            _xSupplier = std::move(xSupplier);
        }
    }
    return _xSupplier;
}

void DialogImport::addStyle(
    OUString const & rStyleId,
    Reference< xml::input::XElement > const & xStyle )
{
    (*_pStyleNames).push_back( rStyleId );
    (*_pStyles).push_back( xStyle );
}

Reference< xml::input::XElement > DialogImport::getStyle(
    std::u16string_view rStyleId ) const
{
    for ( size_t nPos = 0; nPos < (*_pStyleNames).size(); ++nPos )
    {
        if ( (*_pStyleNames)[ nPos ] == rStyleId)
        {
            return (*_pStyles)[ nPos ];
        }
    }
    return nullptr;
}

Reference< xml::sax::XDocumentHandler > importDialogModel(
    Reference< container::XNameContainer > const & xDialogModel,
    Reference< XComponentContext > const & xContext,
    Reference< XModel > const & xDocument )
{
    // single set of styles and stylenames apply to all containers
    auto xStyleNames = std::make_shared<std::vector< OUString >>();
    auto xStyles = std::make_shared<std::vector< css::uno::Reference< css::xml::input::XElement > >>();
    return ::xmlscript::createDocumentHandler(
        new DialogImport(xContext, xDialogModel, std::move(xStyleNames), std::move(xStyles), xDocument));
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
