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

#include <helper/property.hxx>

#include <tools/debug.hxx>
#include <com/sun/star/awt/FontDescriptor.hpp>
#include <com/sun/star/awt/XDevice.hpp>
#include <com/sun/star/awt/tree/XTreeDataModel.hpp>
#include <com/sun/star/awt/grid/XGridDataModel.hpp>
#include <com/sun/star/awt/grid/XGridColumnModel.hpp>
#include <com/sun/star/view/SelectionType.hpp>
#include <com/sun/star/style/VerticalAlignment.hpp>
#include <com/sun/star/util/XNumberFormatsSupplier.hpp>
#include <com/sun/star/util/Date.hpp>
#include <com/sun/star/util/Time.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <algorithm>
#include <string_view>
#include <utility>
#include <unordered_map>

using ::com::sun::star::uno::Any;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::awt::XDevice;
using ::com::sun::star::awt::FontDescriptor;
using ::com::sun::star::style::VerticalAlignment;
using ::com::sun::star::graphic::XGraphic;

using namespace com::sun::star;

namespace {

struct ImplPropertyInfo
{
    css::uno::Type           aType;
    sal_uInt16               nPropId;
    sal_Int16                nAttribs;
    bool                     bDependsOnOthers;   // eg. VALUE depends on MIN/MAX and must be set after MIN/MAX.

    ImplPropertyInfo( sal_uInt16 nId, const css::uno::Type& rType,
                        sal_Int16 nAttrs, bool bDepends = false )
         : aType(rType)
         , nPropId(nId)
         , nAttribs(nAttrs)
         , bDependsOnOthers(bDepends)
     {
     }

};

}

#define DECL_PROP_1( asciiname, id, type, attrib1 ) \
    { asciiname, ImplPropertyInfo( BASEPROPERTY_##id, cppu::UnoType<type>::get(), css::beans::PropertyAttribute::attrib1 ) }
#define DECL_PROP_2( asciiname, id, type, attrib1, attrib2 ) \
    { asciiname, ImplPropertyInfo( BASEPROPERTY_##id, cppu::UnoType<type>::get(), css::beans::PropertyAttribute::attrib1 | css::beans::PropertyAttribute::attrib2 ) }
#define DECL_PROP_3( asciiname, id, type, attrib1, attrib2, attrib3 ) \
    { asciiname, ImplPropertyInfo( BASEPROPERTY_##id, cppu::UnoType<type>::get(), css::beans::PropertyAttribute::attrib1 | css::beans::PropertyAttribute::attrib2 | css::beans::PropertyAttribute::attrib3 ) }

#define DECL_DEP_PROP_2( asciiname, id, type, attrib1, attrib2 ) \
    { asciiname, ImplPropertyInfo( BASEPROPERTY_##id, cppu::UnoType<type>::get(), css::beans::PropertyAttribute::attrib1 | css::beans::PropertyAttribute::attrib2, true ) }
#define DECL_DEP_PROP_3( asciiname, id, type, attrib1, attrib2, attrib3 ) \
    { asciiname, ImplPropertyInfo( BASEPROPERTY_##id, cppu::UnoType<type>::get(), css::beans::PropertyAttribute::attrib1 | css::beans::PropertyAttribute::attrib2 | css::beans::PropertyAttribute::attrib3, true ) }

typedef std::unordered_map<OUString, ImplPropertyInfo> ImpPropertyInfoMap;
static const ImpPropertyInfoMap & ImplGetPropertyInfos()
{
    static const ImpPropertyInfoMap aImplPropertyInfos {
        DECL_PROP_2     ( "AccessibleName",         ACCESSIBLENAME,         OUString,       BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "Align",                  ALIGN,                  sal_Int16,      BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "Autocomplete",           AUTOCOMPLETE,           bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "AutoHScroll",            AUTOHSCROLL,            bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_1     ( "AutoMnemonics",          AUTOMNEMONICS,          bool,           BOUND ),
        DECL_PROP_2     ( "AutoToggle",             AUTOTOGGLE,             bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "AutoVScroll",            AUTOVSCROLL,            bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "BackgroundColor",        BACKGROUNDCOLOR,        sal_Int32,      BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_DEP_PROP_2 ( "BlockIncrement",         BLOCKINCREMENT,         sal_Int32,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "Border",                 BORDER,                 sal_Int16,      BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_DEP_PROP_3 ( "BorderColor",            BORDERCOLOR,            sal_Int32,      BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "Closeable",              CLOSEABLE,              bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "CurrencySymbol",         CURRENCYSYMBOL,         OUString,       BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "CustomUnitText",         CUSTOMUNITTEXT,         OUString,       BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_3 ( "Date",                   DATE,                   util::Date,     BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "DateFormat",             EXTDATEFORMAT,          sal_Int16,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "DateMax",                DATEMAX,                util::Date,     BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "DateMin",                DATEMIN,                util::Date,     BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "DateShowCentury",        DATESHOWCENTURY,        bool,           BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "DecimalAccuracy",        DECIMALACCURACY,        sal_Int16,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "DefaultButton",          DEFAULTBUTTON,          bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "DefaultControl",         DEFAULTCONTROL,         OUString,       BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "DesktopAsParent",        DESKTOP_AS_PARENT,      bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "DisplayBackgroundColor", DISPLAYBACKGROUNDCOLOR, sal_Int32,      BOUND, MAYBEVOID ),
        DECL_PROP_2     ( "Dropdown",               DROPDOWN,               bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "EchoChar",               ECHOCHAR,               sal_Int16,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "EditMask",               EDITMASK,               OUString,       BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "EffectiveDefault",       EFFECTIVE_DEFAULT,      Any,            BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "EffectiveMax",           EFFECTIVE_MAX,          double,         BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "EffectiveMin",           EFFECTIVE_MIN,          double,         BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_DEP_PROP_3 ( "EffectiveValue",         EFFECTIVE_VALUE,        Any,            BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "Enabled",                ENABLED,                bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "EnforceFormat",          ENFORCE_FORMAT,         bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "FillColor",              FILLCOLOR,              sal_Int32,      BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "FocusOnClick",           FOCUSONCLICK,           bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontRelief",             FONTRELIEF,             sal_Int16,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontEmphasisMark",       FONTEMPHASISMARK,       sal_Int16,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontDescriptor",         FONTDESCRIPTOR,         FontDescriptor, BOUND, MAYBEDEFAULT ),

        // parts of css::awt::FontDescriptor
        DECL_PROP_2     ( "FontName",               FONTDESCRIPTORPART_NAME,         OUString,  BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontStyleName",          FONTDESCRIPTORPART_STYLENAME,    OUString,  BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontFamily",             FONTDESCRIPTORPART_FAMILY,       sal_Int16, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontCharset",            FONTDESCRIPTORPART_CHARSET,      sal_Int16, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontHeight",             FONTDESCRIPTORPART_HEIGHT,       float,     BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontWidth",              FONTDESCRIPTORPART_WIDTH,        sal_Int16, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontPitch",              FONTDESCRIPTORPART_PITCH,        sal_Int16, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontWeight",             FONTDESCRIPTORPART_WEIGHT,       float,     BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontCharWidth",          FONTDESCRIPTORPART_CHARWIDTH,    float,     BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontOrientation",        FONTDESCRIPTORPART_ORIENTATION,  float,     BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontSlant",              FONTDESCRIPTORPART_SLANT,        sal_Int16, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontUnderline",          FONTDESCRIPTORPART_UNDERLINE,    sal_Int16, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontStrikeout",          FONTDESCRIPTORPART_STRIKEOUT,    sal_Int16, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontKerning",            FONTDESCRIPTORPART_KERNING,      bool,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontWordLineMode",       FONTDESCRIPTORPART_WORDLINEMODE, bool,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "FontType",               FONTDESCRIPTORPART_TYPE,         sal_Int16, BOUND, MAYBEDEFAULT ),

        DECL_PROP_3     ( "FormatKey",              FORMATKEY,          sal_Int32,      BOUND, MAYBEVOID, TRANSIENT ),
        DECL_PROP_3     ( "FormatsSupplier",        FORMATSSUPPLIER,    Reference< css::util::XNumberFormatsSupplier >, BOUND, MAYBEVOID, TRANSIENT ),

        DECL_PROP_2     ( "Graphic",                GRAPHIC,            Reference< XGraphic >, BOUND, TRANSIENT ),
        DECL_PROP_2     ( "GroupName",              GROUPNAME,          OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "HelpText",               HELPTEXT,           OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "HelpURL",                HELPURL,            OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "HideInactiveSelection",  HIDEINACTIVESELECTION, bool,            BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "HighContrastMode",       HIGHCONTRASTMODE,   bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "HScroll",                HSCROLL,            bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "HardLineBreaks",         HARDLINEBREAKS,     bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "HighlightColor",         HIGHLIGHT_COLOR,    sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID),
        DECL_PROP_3     ( "HighlightTextColor",     HIGHLIGHT_TEXT_COLOR, sal_Int32,        BOUND, MAYBEDEFAULT, MAYBEVOID),
        DECL_PROP_2     ( "ImageAlign",             IMAGEALIGN,         sal_Int16,          BOUND, MAYBEDEFAULT),
        DECL_PROP_2     ( "ImagePosition",          IMAGEPOSITION,      sal_Int16,          BOUND, MAYBEDEFAULT),
        DECL_PROP_2     ( "ImageURL",               IMAGEURL,           css::uno::Any,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "ItemSeparatorPos",       ITEM_SEPARATOR_POS, sal_Int16,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "Label",                  LABEL,              OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "LineColor",              LINECOLOR,          sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "LineCount",              LINECOUNT,          sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "LineEndFormat",          LINE_END_FORMAT,    sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_2 ( "LineIncrement",          LINEINCREMENT,      sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "LiteralMask",            LITERALMASK,        OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "LiveScroll",             LIVE_SCROLL,        bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "MaxTextLen",             MAXTEXTLEN,         sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Moveable",               MOVEABLE,           bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_1     ( "MouseTransparent",       MOUSETRANSPARENT,   bool,               BOUND ),
        DECL_PROP_2     ( "MultiLine",              MULTILINE,          bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "MultiSelection",         MULTISELECTION,     bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "MultiSelectionSimpleMode",   MULTISELECTION_SIMPLEMODE,    bool, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "NativeWidgetLook",       NATIVE_WIDGET_LOOK, bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "NoLabel",                NOLABEL,            bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Orientation",            ORIENTATION,        sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "PaintTransparent",       PAINTTRANSPARENT,   bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "PluginParent",           PLUGINPARENT,       sal_Int64,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "PrependCurrencySymbol",  CURSYM_POSITION,    bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Printable",              PRINTABLE,          bool,               BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_3 ( "ProgressValue",          PROGRESSVALUE,      sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "ProgressValueMax",       PROGRESSVALUE_MAX,  sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ProgressValueMin",       PROGRESSVALUE_MIN,  sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "PushButtonType",         PUSHBUTTONTYPE,     sal_Int16,          BOUND, MAYBEDEFAULT),
        DECL_PROP_2     ( "ReadOnly",               READONLY,           bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Repeat",                 REPEAT,             bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "AutoRepeat",             AUTO_REPEAT,        sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "RepeatDelay",            REPEAT_DELAY,       sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ScaleImage",             SCALEIMAGE,         bool,               BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_2 ( "ScaleMode",              IMAGE_SCALE_MODE,   sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_3 ( "ScrollValue",            SCROLLVALUE,        sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "ScrollValueMax",         SCROLLVALUE_MAX,    sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ScrollValueMin",         SCROLLVALUE_MIN,    sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ScrollWidth",            SCROLLWIDTH,        sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ScrollHeight",           SCROLLHEIGHT,       sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ScrollTop",              SCROLLTOP,          sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ScrollLeft",             SCROLLLEFT,         sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_2 ( "SelectedItems",          SELECTEDITEMS,      Sequence<sal_Int16>, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ShowThousandsSeparator", NUMSHOWTHOUSANDSEP, bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Sizeable",               SIZEABLE,           bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Spin",                   SPIN,               bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "SpinIncrement",          SPININCREMENT,      sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_2 ( "SpinValue",              SPINVALUE,          sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "SpinValueMax",           SPINVALUE_MAX,      sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "SpinValueMin",           SPINVALUE_MIN,      sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_2 ( "State",                  STATE,              sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "StrictFormat",           STRICTFORMAT,       bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "StringItemList",         STRINGITEMLIST,     Sequence< OUString >, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "TypedItemList",          TYPEDITEMLIST,      Sequence< Any >,    BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "VisualEffect",           VISUALEFFECT,       sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "SymbolColor",            SYMBOL_COLOR,       sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "Tabstop",                TABSTOP,            bool,               BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "Text",                   TEXT,               OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "TextColor",              TEXTCOLOR,          sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "TextLineColor",          TEXTLINECOLOR,      sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_DEP_PROP_3 ( "Time",                   TIME,               util::Time,         BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "TimeFormat",             EXTTIMEFORMAT,      sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "TimeMax",                TIMEMAX,            util::Time,         BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "TimeMin",                TIMEMIN,            util::Time,         BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Title",                  TITLE,              OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Toggle",                 TOGGLE,             bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "TreatAsNumber",          TREATASNUMBER,      bool,               BOUND, MAYBEDEFAULT,TRANSIENT ),
        DECL_PROP_2     ( "TriState",               TRISTATE,           bool,               BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Unit",                   UNIT,               sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "VScroll",                VSCROLL,            bool,               BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_3 ( "Value",                  VALUE_DOUBLE,       double,             BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "ValueMax",               VALUEMAX_DOUBLE,    double,             BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ValueMin",               VALUEMIN_DOUBLE,    double,             BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ValueStep",              VALUESTEP_DOUBLE,   double,             BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "VerticalAlign",          VERTICALALIGN,      VerticalAlignment,  BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_DEP_PROP_3 ( "VisibleSize",            VISIBLESIZE,        sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "Activated",              ACTIVATED,          sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Complete",               COMPLETE,           sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "CurrentItemID",          CURRENTITEMID,      sal_Int16,          BOUND, MAYBEDEFAULT ),

        DECL_PROP_2     ( "MouseWheelBehavior",     MOUSE_WHEEL_BEHAVIOUR,  sal_Int16,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "StepTime",               STEP_TIME,              sal_Int32,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Decoration",             DECORATION,             sal_Bool,       BOUND, MAYBEDEFAULT ),

        DECL_PROP_2     ( "SelectionType",          TREE_SELECTIONTYPE,     css::view::SelectionType, BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "Editable",               TREE_EDITABLE,          sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "DataModel",              TREE_DATAMODEL,         Reference< css::awt::tree::XTreeDataModel >,BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "RootDisplayed",          TREE_ROOTDISPLAYED,     sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ShowsHandles",           TREE_SHOWSHANDLES,      sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ShowsRootHandles",       TREE_SHOWSROOTHANDLES,  sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "RowHeight",              ROW_HEIGHT,             sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "InvokesStopNodeEditing", TREE_INVOKESSTOPNODEEDITING, sal_Bool,      BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "DialogSourceURL",        DIALOGSOURCEURL,        OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "URL",                    URL,                    OUString,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "WritingMode",            WRITING_MODE,           sal_Int16,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "ContextWritingMode",     CONTEXT_WRITING_MODE,   sal_Int16,          BOUND, MAYBEDEFAULT, TRANSIENT ),
        DECL_PROP_2     ( "ShowRowHeader",          GRID_SHOWROWHEADER,     sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "RowHeaderWidth",         ROW_HEADER_WIDTH,       sal_Int32,          BOUND, MAYBEDEFAULT ),
        DECL_PROP_2     ( "ShowColumnHeader",       GRID_SHOWCOLUMNHEADER,  sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "ColumnHeaderHeight",     COLUMN_HEADER_HEIGHT,   sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_1     ( "GridDataModel",          GRID_DATAMODEL,         Reference< css::awt::grid::XGridDataModel >, BOUND ),
        DECL_PROP_1     ( "ColumnModel",            GRID_COLUMNMODEL,       Reference< css::awt::grid::XGridColumnModel >, BOUND ),
        DECL_PROP_3     ( "SelectionModel",         GRID_SELECTIONMODE,     css::view::SelectionType, BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "EnableVisible",          ENABLEVISIBLE,          sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_PROP_3     ( "ReferenceDevice",        REFERENCE_DEVICE,       Reference< XDevice >,BOUND, MAYBEDEFAULT, TRANSIENT ),
        DECL_PROP_3     ( "HeaderBackgroundColor",  GRID_HEADER_BACKGROUND, sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "HeaderTextColor",        GRID_HEADER_TEXT_COLOR, sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "GridLineColor",          GRID_LINE_COLOR,        sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "RowBackgroundColors",    GRID_ROW_BACKGROUND_COLORS, Sequence< sal_Int32 >, BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_2     ( "UseGridLines",           USE_GRID_LINES,         sal_Bool,           BOUND, MAYBEDEFAULT ),
        DECL_DEP_PROP_3 ( "MultiPageValue",         MULTIPAGEVALUE,         sal_Int32,          BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "AllDialogChildren",      USERFORMCONTAINEES,     Reference< css::container::XNameContainer >, BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "ActiveSelectionBackgroundColor",   ACTIVE_SEL_BACKGROUND_COLOR,   sal_Int32, BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "InactiveSelectionBackgroundColor", INACTIVE_SEL_BACKGROUND_COLOR, sal_Int32, BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "ActiveSelectionTextColor",         ACTIVE_SEL_TEXT_COLOR,         sal_Int32, BOUND, MAYBEDEFAULT, MAYBEVOID ),
        DECL_PROP_3     ( "InactiveSelectionTextColor",       INACTIVE_SEL_TEXT_COLOR,       sal_Int32, BOUND, MAYBEDEFAULT, MAYBEVOID ),

        DECL_PROP_2("Referer", REFERER, OUString, BOUND, MAYBEVOID),
    };
    return aImplPropertyInfos;
}

sal_uInt16 GetPropertyId( const OUString& rPropertyName )
{
    const ImpPropertyInfoMap & rMap = ImplGetPropertyInfos();
    auto it = rMap.find(rPropertyName);
    return it != rMap.end() ? it->second.nPropId : 0;
}

static const ImplPropertyInfo* ImplGetImplPropertyInfo( sal_uInt16 nPropertyId )
{
    const ImpPropertyInfoMap & rMap = ImplGetPropertyInfos();

    for (auto const & rPair : rMap)
        if (rPair.second.nPropId == nPropertyId)
            return &rPair.second;
    return nullptr;
}

const OUString& GetPropertyName( sal_uInt16 nPropertyId )
{
    const ImpPropertyInfoMap & rMap = ImplGetPropertyInfos();

    for (auto const & rPair : rMap)
        if (rPair.second.nPropId == nPropertyId)
            return rPair.first;

    assert(false && "Invalid PropertyId!");
    static const OUString EMPTY;
    return EMPTY;
}

const css::uno::Type* GetPropertyType( sal_uInt16 nPropertyId )
{
    const ImplPropertyInfo* pImplPropertyInfo = ImplGetImplPropertyInfo( nPropertyId );
    DBG_ASSERT( pImplPropertyInfo, "Invalid PropertyId!" );
    return pImplPropertyInfo ? &pImplPropertyInfo->aType : nullptr;
}

sal_Int16 GetPropertyAttribs( sal_uInt16 nPropertyId )
{
    const ImplPropertyInfo* pImplPropertyInfo = ImplGetImplPropertyInfo( nPropertyId );
    DBG_ASSERT( pImplPropertyInfo, "Invalid PropertyId!" );
    return pImplPropertyInfo ? pImplPropertyInfo->nAttribs : 0;
}

bool DoesDependOnOthers( sal_uInt16 nPropertyId )
{
    const ImplPropertyInfo* pImplPropertyInfo = ImplGetImplPropertyInfo( nPropertyId );
    DBG_ASSERT( pImplPropertyInfo, "Invalid PropertyId!" );
    return pImplPropertyInfo && pImplPropertyInfo->bDependsOnOthers;
}

bool CompareProperties( const css::uno::Any& r1, const css::uno::Any& r2 )
{
    return r1 == r2;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
