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

#include <oox/helper/propertymap.hxx>

#if OSL_DEBUG_LEVEL > 0
# include <cstdio>
# include <com/sun/star/style/LineSpacing.hpp>
# include <com/sun/star/text/WritingMode.hpp>
using ::com::sun::star::style::LineSpacing;
using ::com::sun::star::text::WritingMode;
#include <comphelper/anytostring.hxx>
#include <iostream>
#endif

#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/container/XIndexReplace.hpp>
#include <com/sun/star/awt/Rectangle.hpp>
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/drawing/TextHorizontalAdjust.hpp>
#include <com/sun/star/drawing/TextVerticalAdjust.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeAdjustmentValue.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeSegment.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeTextFrame.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeParameterPair.hpp>
#include <com/sun/star/drawing/EnhancedCustomShapeParameterType.hpp>
#include <com/sun/star/drawing/HomogenMatrix3.hpp>
#include <cppuhelper/implbase.hxx>
#include <osl/diagnose.h>
#include <mutex>
#include <sal/log.hxx>
#include <oox/token/properties.hxx>
#include <oox/token/propertynames.hxx>
using ::com::sun::star::uno::Any;
using ::com::sun::star::uno::Reference;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::beans::Property;
using ::com::sun::star::beans::PropertyValue;
using ::com::sun::star::beans::UnknownPropertyException;
using ::com::sun::star::beans::XPropertyChangeListener;
using ::com::sun::star::beans::XPropertySet;
using ::com::sun::star::beans::XPropertySetInfo;
using ::com::sun::star::beans::XVetoableChangeListener;
using ::com::sun::star::container::XIndexReplace;

#if OSL_DEBUG_LEVEL > 0
#define USS(x) OUStringToOString( x, RTL_TEXTENCODING_UTF8 ).getStr()
using namespace ::com::sun::star;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::uno;
using ::com::sun::star::style::LineSpacing;
using ::com::sun::star::text::WritingMode;
using ::com::sun::star::drawing::TextHorizontalAdjust;
using ::com::sun::star::drawing::TextVerticalAdjust;
#endif

namespace oox {

using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::drawing;
using namespace ::com::sun::star::uno;

namespace {

/** This class implements a generic XPropertySet.

    Properties of all names and types can be set and later retrieved.
    TODO: move this to comphelper or better find an existing implementation
 */
class GenericPropertySet : public ::cppu::WeakImplHelper< XPropertySet, XPropertySetInfo >
{
public:
    explicit            GenericPropertySet( const PropertyMap& rPropMap );

    // XPropertySet
    virtual Reference< XPropertySetInfo > SAL_CALL getPropertySetInfo() override;
    virtual void SAL_CALL setPropertyValue( const OUString& aPropertyName, const Any& aValue ) override;
    virtual Any SAL_CALL getPropertyValue( const OUString& PropertyName ) override;
    virtual void SAL_CALL addPropertyChangeListener( const OUString& aPropertyName, const Reference< XPropertyChangeListener >& xListener ) override;
    virtual void SAL_CALL removePropertyChangeListener( const OUString& aPropertyName, const Reference< XPropertyChangeListener >& aListener ) override;
    virtual void SAL_CALL addVetoableChangeListener( const OUString& PropertyName, const Reference< XVetoableChangeListener >& aListener ) override;
    virtual void SAL_CALL removeVetoableChangeListener( const OUString& PropertyName, const Reference< XVetoableChangeListener >& aListener ) override;

    // XPropertySetInfo
    virtual Sequence< Property > SAL_CALL getProperties() override;
    virtual Property SAL_CALL getPropertyByName( const OUString& aName ) override;
    virtual sal_Bool SAL_CALL hasPropertyByName( const OUString& Name ) override;

private:
    std::mutex mMutex;
    PropertyNameMap     maPropMap;
};

GenericPropertySet::GenericPropertySet( const PropertyMap& rPropMap )
{
    rPropMap.fillPropertyNameMap(maPropMap);
}

Reference< XPropertySetInfo > SAL_CALL GenericPropertySet::getPropertySetInfo()
{
    return this;
}

void SAL_CALL GenericPropertySet::setPropertyValue( const OUString& rPropertyName, const Any& rValue )
{
    std::scoped_lock aGuard( mMutex );
    maPropMap[ rPropertyName ] = rValue;
}

Any SAL_CALL GenericPropertySet::getPropertyValue( const OUString& rPropertyName )
{
    PropertyNameMap::iterator aIt = maPropMap.find( rPropertyName );
    if( aIt == maPropMap.end() )
        throw UnknownPropertyException(rPropertyName);
    return aIt->second;
}

// listeners are not supported by this implementation
void SAL_CALL GenericPropertySet::addPropertyChangeListener( const OUString& , const Reference< XPropertyChangeListener >& ) {}
void SAL_CALL GenericPropertySet::removePropertyChangeListener( const OUString& , const Reference< XPropertyChangeListener >&  ) {}
void SAL_CALL GenericPropertySet::addVetoableChangeListener( const OUString& , const Reference< XVetoableChangeListener >&  ) {}
void SAL_CALL GenericPropertySet::removeVetoableChangeListener( const OUString& , const Reference< XVetoableChangeListener >&  ) {}

// XPropertySetInfo
Sequence< Property > SAL_CALL GenericPropertySet::getProperties()
{
    Sequence< Property > aSeq( static_cast< sal_Int32 >( maPropMap.size() ) );
    Property* pProperty = aSeq.getArray();
    for (auto const& prop : maPropMap)
    {
        pProperty->Name = prop.first;
        pProperty->Handle = 0;
        pProperty->Type = prop.second.getValueType();
        pProperty->Attributes = 0;
        ++pProperty;
    }
    return aSeq;
}

Property SAL_CALL GenericPropertySet::getPropertyByName( const OUString& rPropertyName )
{
    PropertyNameMap::iterator aIt = maPropMap.find( rPropertyName );
    if( aIt == maPropMap.end() )
        throw UnknownPropertyException(rPropertyName);
    Property aProperty;
    aProperty.Name = aIt->first;
    aProperty.Handle = 0;
    aProperty.Type = aIt->second.getValueType();
    aProperty.Attributes = 0;
    return aProperty;
}

sal_Bool SAL_CALL GenericPropertySet::hasPropertyByName( const OUString& rPropertyName )
{
    return maPropMap.contains(rPropertyName);
}

} // namespace

PropertyMap::PropertyMap() :
    mpPropNames( &GetPropertyNameVector() ) // pointer instead reference to get compiler generated copy c'tor and operator=
{
}

bool PropertyMap::hasProperty( sal_Int32 nPropId ) const
{
    return maProperties.find( nPropId ) != maProperties.end();
}

bool PropertyMap::setAnyProperty( sal_Int32 nPropId, const Any& rValue )
{
    if( nPropId < 0 )
        return false;

    maProperties[ nPropId ] = rValue;
    return true;
}

Any PropertyMap::getProperty( sal_Int32 nPropId )
{
    return maProperties[ nPropId ];
}

void PropertyMap::erase( sal_Int32 nPropId )
{
    maProperties.erase(nPropId);
}

bool PropertyMap::empty() const
{
    return maProperties.empty();
}

void PropertyMap::assignUsed( const PropertyMap& rPropMap )
{
    maProperties.insert(rPropMap.maProperties.begin(), rPropMap.maProperties.end());
}

const OUString& PropertyMap::getPropertyName( sal_Int32 nPropId )
{
    OSL_ENSURE( (0 <= nPropId) && (nPropId < PROP_COUNT), "PropertyMap::getPropertyName - invalid property identifier" );
    return GetPropertyNameVector()[ nPropId ];
}

sal_Int32 PropertyMap::getPropertyId( std::u16string_view sPropName )
{
    // This may use a std::map to get faster from String to ID in the
    // future, inside the [0..PROP_COUNT[ entries. Since it is currently
    // only used for Diagram re-creation I opted for less memory usage here
    if(sPropName.empty())
        return -1;

    const std::vector<OUString>& rVec(GetPropertyNameVector());
    for(size_t a(0); a < rVec.size(); a++)
        if(rVec[a] == sPropName)
            return a;

    return -1;
}

void PropertyMap::assignAll( const PropertyMap& rPropMap )
{
    for (auto const& prop : rPropMap.maProperties)
        maProperties[prop.first] = prop.second;
}

Sequence< PropertyValue > PropertyMap::makePropertyValueSequence() const
{
    Sequence< PropertyValue > aSeq( static_cast< sal_Int32 >( maProperties.size() ) );
    PropertyValue* pValues = aSeq.getArray();
    for (auto const& prop : maProperties)
    {
        OSL_ENSURE( (0 <= prop.first) && (prop.first < PROP_COUNT), "PropertyMap::makePropertyValueSequence - invalid property identifier" );
        pValues->Name = (*mpPropNames)[ prop.first ];
        pValues->Value = prop.second;
        pValues->State = PropertyState_DIRECT_VALUE;
        ++pValues;
    }
    return aSeq;
}

void PropertyMap::fillSequences( Sequence< OUString >& rNames, Sequence< Any >& rValues ) const
{
    rNames.realloc( static_cast< sal_Int32 >( maProperties.size() ) );
    rValues.realloc( static_cast< sal_Int32 >( maProperties.size() ) );
    if( maProperties.empty() )
        return;

    OUString* pNames = rNames.getArray();
    Any* pValues = rValues.getArray();
    for (auto const& prop : maProperties)
    {
        OSL_ENSURE( (0 <= prop.first) && (prop.first < PROP_COUNT), "PropertyMap::fillSequences - invalid property identifier" );
        *pNames = (*mpPropNames)[ prop.first ];
        *pValues = prop.second;
        ++pNames;
        ++pValues;
    }
}

void PropertyMap::fillPropertyNameMap(PropertyNameMap& rMap) const
{
    for (auto const& prop : maProperties)
    {
        rMap.insert(std::pair<OUString, Any>((*mpPropNames)[prop.first], prop.second));
    }
}

Reference< XPropertySet > PropertyMap::makePropertySet() const
{
    return new GenericPropertySet( *this );
}

#if OSL_DEBUG_LEVEL > 0
static void lclDumpAnyValue( const Any& value)
{
    OUString strValue;
    Sequence< OUString > strArray;
    Sequence< Any > anyArray;
    Sequence< PropertyValue > propArray;
    Sequence< Sequence< PropertyValue > > propArrayArray;
    Sequence< EnhancedCustomShapeAdjustmentValue > adjArray;
    Sequence< EnhancedCustomShapeSegment > segArray;
    Sequence< EnhancedCustomShapeParameterPair > ppArray;
    EnhancedCustomShapeSegment segment;
    EnhancedCustomShapeParameterPair pp;
    EnhancedCustomShapeParameter par;
    HomogenMatrix3 aMatrix;
    sal_Int32 intValue = 0;
    sal_uInt32 uintValue = 0;
    sal_Int16 int16Value = 0;
    sal_uInt16 uint16Value = 0;
    float floatValue = 0;
    bool boolValue = false;
    LineSpacing spacing;
//         RectanglePoint pointValue;
    WritingMode aWritingMode;
    TextVerticalAdjust aTextVertAdj;
    TextHorizontalAdjust aTextHorizAdj;
    Reference< XIndexReplace > xNumRule;

    if( value >>= strValue )
            fprintf (stderr,"\"%s\"\n", USS( strValue ) );
    else if( value >>= strArray ) {
            fprintf (stderr,"%s\n", USS(value.getValueTypeName()));
            for( int i=0; i<strArray.getLength(); i++ )
                fprintf (stderr,"\t\t\t[%3d] \"%s\"\n", i, USS( strArray[i] ) );
    } else if( value >>= propArray ) {
            fprintf (stderr,"%s\n", USS(value.getValueTypeName()));
            for( int i=0; i<propArray.getLength(); i++ ) {
                fprintf (stderr,"\t\t\t[%3d] %s (%s) ", i, USS( propArray[i].Name ), USS(propArray[i].Value.getValueTypeName()) );
                lclDumpAnyValue( propArray[i].Value );
            }
    } else if( value >>= propArrayArray ) {
            fprintf (stderr,"%s\n", USS(value.getValueTypeName()));
            for( int i=0; i<propArrayArray.getLength(); i++ ) {
                fprintf (stderr,"\t\t\t[%3d] ", i);
                lclDumpAnyValue( Any (propArrayArray[i]) );
            }
    } else if( value >>= anyArray ) {
            fprintf (stderr,"%s\n", USS(value.getValueTypeName()));
            for( int i=0; i<anyArray.getLength(); i++ ) {
                fprintf (stderr,"\t\t\t[%3d] (%s) ", i, USS(value.getValueTypeName()) );
                lclDumpAnyValue( anyArray[i] );
            }
    } else if( value >>= adjArray ) {
            fprintf (stderr,"%s\n", USS(value.getValueTypeName()));
            for( int i=0; i<adjArray.getLength(); i++ ) {
                fprintf (stderr,"\t\t\t[%3d] (%s) ", i, USS(adjArray[i].Value.getValueTypeName()) );
                lclDumpAnyValue( adjArray[i].Value );
            }
    } else if( value >>= segArray ) {
            fprintf (stderr,"%s\n", USS(value.getValueTypeName()));
            for( int i=0; i<segArray.getLength(); i++ ) {
                fprintf (stderr,"\t\t\t[%3d] ", i );
                lclDumpAnyValue( Any( segArray[i] ) );
            }
    } else if( value >>= ppArray ) {
            fprintf (stderr,"%s\n", USS(value.getValueTypeName()));
            for( int i=0; i<ppArray.getLength(); i++ ) {
                fprintf (stderr,"\t\t\t[%3d] ", i );
                lclDumpAnyValue( Any( ppArray[i] ) );
            }
    } else if( value >>= segment ) {
            fprintf (stderr,"Command: %d Count: %d\n", segment.Command, segment.Count);
    } else if( value >>= pp ) {
            fprintf (stderr,"First: ");
            lclDumpAnyValue( Any (pp.First) );
            fprintf (stderr,"\t\t\t      Second: ");
            lclDumpAnyValue( Any (pp.Second) );
    } else if( value >>= par ) {
            fprintf (stderr,"Parameter (%s): ", USS(par.Value.getValueTypeName()));
            lclDumpAnyValue( par.Value );
    } else if( value >>= aMatrix ) {
            fprintf (stderr,"Matrix\n%f %f %f\n%f %f %f\n%f %f %f\n", aMatrix.Line1.Column1, aMatrix.Line1.Column2, aMatrix.Line1.Column3, aMatrix.Line2.Column1, aMatrix.Line2.Column2, aMatrix.Line2.Column3, aMatrix.Line3.Column1, aMatrix.Line3.Column2, aMatrix.Line3.Column3);
    } else if( value >>= intValue )
            fprintf (stderr,"%-10" SAL_PRIdINT32 "  (hex: %" SAL_PRIxUINT32 ")\n", intValue, intValue);
    else if( value >>= uintValue )
            fprintf (stderr,"%-10" SAL_PRIuUINT32 "  (hex: %" SAL_PRIxUINT32 ")\n", uintValue, uintValue);
    else if( value >>= int16Value )
            fprintf (stderr,"%-10d  (hex: %x)\n", int16Value, int16Value);
    else if( value >>= uint16Value )
            fprintf (stderr,"%-10d  (hex: %x)\n", uint16Value, uint16Value);
    else if( value >>= floatValue )
            fprintf (stderr,"%f\n", floatValue);
    else if( value >>= boolValue )
            fprintf (stderr,"%-10d  (bool)\n", boolValue);
    else if( value >>= xNumRule ) {
            fprintf (stderr, "XIndexReplace\n");
            if (xNumRule.is()) {
                for (int k=0; k<xNumRule->getCount(); k++) {
                    Sequence< PropertyValue > aBulletPropSeq;
                    fprintf (stderr, "level %d\n", k);
                    if (xNumRule->getByIndex (k) >>= aBulletPropSeq) {
                        for (const PropertyValue& rProp : aBulletPropSeq) {
                            fprintf(stderr, "%46s = ", USS (rProp.Name));
                            lclDumpAnyValue (rProp.Value);
                        }
                    }
                }
            } else {
                fprintf (stderr, "empty reference\n");
            }
    } else if( value >>= aWritingMode )
            fprintf(stderr, "%d writing mode\n", static_cast<int>(aWritingMode));
    else if( value >>= aTextVertAdj ) {
            const char* s = "unknown";
            switch( aTextVertAdj ) {
            case TextVerticalAdjust_TOP:
                s = "top";
                break;
            case TextVerticalAdjust_CENTER:
                s = "center";
                break;
            case TextVerticalAdjust_BOTTOM:
                s = "bottom";
                break;
            case TextVerticalAdjust_BLOCK:
                s = "block";
                break;
            case TextVerticalAdjust::TextVerticalAdjust_MAKE_FIXED_SIZE:
                s = "make_fixed_size";
                break;
            }
            fprintf (stderr, "%s\n", s);
    } else if( value >>= aTextHorizAdj ) {
        const char* s = "unknown";
        switch( aTextHorizAdj ) {
            case TextHorizontalAdjust_LEFT:
                s = "left";
                break;
            case TextHorizontalAdjust_CENTER:
                s = "center";
                break;
            case TextHorizontalAdjust_RIGHT:
                s = "right";
                break;
            case TextHorizontalAdjust_BLOCK:
                s = "block";
                break;
            case TextHorizontalAdjust::TextHorizontalAdjust_MAKE_FIXED_SIZE:
                s = "make_fixed_size";
                break;
        }
        fprintf (stderr, "%s\n", s);
    } else if( value >>= spacing ) {
        fprintf (stderr, "mode: %d value: %d\n", spacing.Mode, spacing.Height);
    } else if( value.isExtractableTo(::cppu::UnoType<sal_Int32>::get())) {
        fprintf (stderr,"is extractable to int32\n");
    }
//         else if( value >>= pointValue )
//             fprintf (stderr,"%d            (RectanglePoint)\n", pointValue);
        else
      fprintf (stderr,"???           <unhandled type %s>\n", USS(value.getValueTypeName()));
}

#ifdef DBG_UTIL
void PropertyMap::dump( const Reference< XPropertySet >& rXPropSet )
{
    Reference< XPropertySetInfo > info = rXPropSet->getPropertySetInfo ();
    const Sequence< Property > props = info->getProperties ();

    SAL_INFO("oox", "dump props, len: " << props.getLength ());

    for (Property const & prop : props) {
        OString name = OUStringToOString( prop.Name, RTL_TEXTENCODING_UTF8);
        fprintf (stderr,"%30s = ", name.getStr() );

        try {
            lclDumpAnyValue (rXPropSet->getPropertyValue( prop.Name ));
        } catch (const Exception&) {
            fprintf (stderr,"unable to get '%s' value\n", USS(prop.Name));
        }
    }
}
#endif

static void printLevel (int level)
{
    for (int i=0; i<level; i++)
        fprintf (stderr, "    ");
}

static const char *lclGetEnhancedParameterType( sal_uInt16 nType )
{
    const char* type;
    switch (nType) {
    case EnhancedCustomShapeParameterType::NORMAL:
        type = "EnhancedCustomShapeParameterType::NORMAL";
        break;
    case EnhancedCustomShapeParameterType::EQUATION:
        type = "EnhancedCustomShapeParameterType::EQUATION";
        break;
    case EnhancedCustomShapeParameterType::ADJUSTMENT:
        type = "EnhancedCustomShapeParameterType::ADJUSTMENT";
        break;
    case EnhancedCustomShapeParameterType::LEFT:
        type = "EnhancedCustomShapeParameterType::LEFT";
        break;
    case EnhancedCustomShapeParameterType::TOP:
        type = "EnhancedCustomShapeParameterType::TOP";
        break;
    case EnhancedCustomShapeParameterType::RIGHT:
        type = "EnhancedCustomShapeParameterType::RIGHT";
        break;
    case EnhancedCustomShapeParameterType::BOTTOM:
        type = "EnhancedCustomShapeParameterType::BOTTOM";
        break;
    case EnhancedCustomShapeParameterType::XSTRETCH:
        type = "EnhancedCustomShapeParameterType::XSTRETCH";
        break;
    case EnhancedCustomShapeParameterType::YSTRETCH:
        type = "EnhancedCustomShapeParameterType::YSTRETCH";
        break;
    case EnhancedCustomShapeParameterType::HASSTROKE:
        type = "EnhancedCustomShapeParameterType::HASSTROKE";
        break;
    case EnhancedCustomShapeParameterType::HASFILL:
        type = "EnhancedCustomShapeParameterType::HASFILL";
        break;
    case EnhancedCustomShapeParameterType::WIDTH:
        type = "EnhancedCustomShapeParameterType::WIDTH";
        break;
    case EnhancedCustomShapeParameterType::HEIGHT:
        type = "EnhancedCustomShapeParameterType::HEIGHT";
        break;
    case EnhancedCustomShapeParameterType::LOGWIDTH:
        type = "EnhancedCustomShapeParameterType::LOGWIDTH";
        break;
    case EnhancedCustomShapeParameterType::LOGHEIGHT:
        type = "EnhancedCustomShapeParameterType::LOGHEIGHT";
        break;
    default:
        type = "unknown";
        break;
    }
    return type;
}

static void printParameterPairData(int level, EnhancedCustomShapeParameterPair const &pp)
{
    // These are always sal_Int32s so let's depend on that for our packing...
    sal_Int32 nFirstValue = {};
    sal_Int32 nSecondValue = {}; // spurious -Werror=maybe-uninitialized
    if (!(pp.First.Value >>= nFirstValue))
        assert (false);
    if (!(pp.Second.Value >>= nSecondValue))
        assert (false);

    printLevel (level);
    fprintf (stderr, "{\n");
    printLevel (level + 1);
    fprintf (stderr, "%s,\n", lclGetEnhancedParameterType(pp.First.Type));
    printLevel (level + 1);
    fprintf (stderr, "%s,\n", lclGetEnhancedParameterType(pp.Second.Type));
    printLevel (level + 1);
    fprintf (stderr, "%d, %d\n", static_cast<int>(nFirstValue), static_cast<int>(nSecondValue));
    printLevel (level);
    fprintf (stderr, "}");
}

static const char* lclDumpAnyValueCode( const Any& value, int level)
{
    OUString strValue;
    Sequence< OUString > strArray;
    Sequence< Any > anyArray;
    Sequence< awt::Size > sizeArray;
    Sequence< PropertyValue > propArray;
    Sequence< Sequence< PropertyValue > > propArrayArray;
    Sequence< EnhancedCustomShapeAdjustmentValue > adjArray;
    Sequence< EnhancedCustomShapeTextFrame > segTextFrame;
    Sequence< EnhancedCustomShapeSegment > segArray;
    Sequence< EnhancedCustomShapeParameterPair > ppArray;
    EnhancedCustomShapeSegment segment;
    EnhancedCustomShapeTextFrame textFrame;
    EnhancedCustomShapeParameterPair pp;
    EnhancedCustomShapeParameter par;
    awt::Rectangle rect;
    awt::Size size;
    sal_Int32 intValue;
    sal_uInt32 uintValue;
    sal_Int16 int16Value;
    sal_uInt16 uint16Value;
    sal_Int64 int64Value;
    float floatValue = 0;
    bool boolValue;
    LineSpacing spacing;
//         RectanglePoint pointValue;
    WritingMode aWritingMode;
    TextVerticalAdjust aTextVertAdj;
    TextHorizontalAdjust aTextHorizAdj;
    Reference< XIndexReplace > xNumRule;

    if( value >>= strValue )
    {
        printLevel (level);
        fprintf (stderr,"OUString str = \"%s\";\n", USS( strValue ) );
        return "Any (str)";
    }
    else if( value >>= strArray )
    {
        if (strArray.getLength() == 0)
            return "Sequence< OUString >(0)";

        printLevel (level);
        fprintf (stderr,"static const char *aStrings[] = {\n");
        for( int i=0; i<strArray.getLength(); i++ ) {
            printLevel (level + 1);
            fprintf (stderr,"\"%s\"%s\n", USS( strArray[i] ), i < strArray.getLength() - 1 ? "," : "" );
        }
        printLevel (level);
        fprintf (stderr,"};\n");
        return "createStringSequence( SAL_N_ELEMENTS( aStrings ), aStrings )";
    }
    else if( value >>= propArray )
    {
        printLevel (level);
        fprintf (stderr,"Sequence< PropertyValue > aPropSequence (%" SAL_PRIdINT32 ");\n", propArray.getLength());
        for( int i=0; i<propArray.getLength(); i++ ) {
            printLevel (level);
            fprintf (stderr, "{\n");
            printLevel (level + 1);
            fprintf (stderr, "aPropSequence [%d].Name = \"%s\";\n", i, USS( propArray[i].Name ));
            const char *var = lclDumpAnyValueCode( propArray[i].Value, level + 1 );
            printLevel (level + 1);
            fprintf (stderr, "aPropSequence [%d].Value = makeAny (%s);\n", i, var);
            printLevel (level);
            fprintf (stderr, "}\n");
        }
        return "aPropSequence";
    }
    else if( value >>= sizeArray )
    {
        printLevel (level);
        fprintf (stderr, "Sequence< awt::Size > aSizeSequence (%" SAL_PRIdINT32 ");\n", sizeArray.getLength());
        for( int i=0; i<sizeArray.getLength(); i++ ) {
            printLevel (level);
            fprintf (stderr, "{\n");
            const char *var = lclDumpAnyValueCode (Any (sizeArray[i]), level + 1);
            printLevel (level + 1);
            fprintf (stderr, "aSizeSequence [%d] = %s;\n", i, var);
            printLevel (level);
            fprintf (stderr, "}\n");
        }
        return "aSizeSequence";
    }
    else if( value >>= propArrayArray )
    {
        printLevel (level);
        fprintf (stderr,"Sequence< Sequence < PropertyValue > > aPropSequenceSequence (%" SAL_PRIdINT32 ");\n", propArrayArray.getLength());
        for( int i=0; i<propArrayArray.getLength(); i++ ) {
            printLevel (level);
            fprintf (stderr, "{\n");
            const char *var = lclDumpAnyValueCode( Any (propArrayArray[i]), level + 1 );
            printLevel (level + 1);
            fprintf (stderr, "aPropSequenceSequence [%d] = %s;\n", i, var);
            printLevel (level);
            fprintf (stderr, "}\n");
        }
        return "aPropSequenceSequence";
    }
    else if( value >>= anyArray )
    {
        fprintf (stderr,"%s\n", USS(value.getValueTypeName()));
        for( int i=0; i<anyArray.getLength(); i++ ) {
            fprintf (stderr,"\t\t\t[%3d] (%s) ", i, USS(value.getValueTypeName()) );
            lclDumpAnyValue( anyArray[i] );
        }
    }
    else if( value >>= adjArray )
    {
        printLevel (level);
        fprintf (stderr,"Sequence< EnhancedCustomShapeAdjustmentValue > aAdjSequence (%" SAL_PRIdINT32 ");\n", adjArray.getLength());
        for( int i=0; i<adjArray.getLength(); i++ ) {
            printLevel (level);
            fprintf (stderr, "{\n");
            const char *var = lclDumpAnyValueCode( adjArray[i].Value, level + 1 );
            printLevel (level + 1);
            fprintf (stderr, "aAdjSequence [%d].Value = %s;\n", i, var);
            if (adjArray[i].Name.getLength() > 0) {
                printLevel (level + 1);
                fprintf (stderr, "aAdjSequence [%d].Name = \"%s\";\n", i, USS (adjArray[i].Name));
            }
            printLevel (level);
            fprintf (stderr, "}\n");
        }
        return "aAdjSequence";
    }
    else if( value >>= segArray )
    {
        if (segArray.getLength() == 0)
            return "Sequence< EnhancedCustomShapeSegment >(0)";

        printLevel (level);
        fprintf (stderr,"static const sal_uInt16 nValues[] = {\n");
        printLevel (level);
        fprintf (stderr,"// Command, Count\n");
        for( int i = 0; i < segArray.getLength(); i++ ) {
            printLevel (level + 1);
            fprintf (stderr,"%d,%d%s\n", segArray[i].Command,
                    segArray[i].Count, i < segArray.getLength() - 1 ? "," : "");
        }
        printLevel (level);
        fprintf (stderr,"};\n");
        return "createSegmentSequence( SAL_N_ELEMENTS( nValues ), nValues )";
    }
    else if( value >>= segTextFrame )
    {
        printLevel (level);
        fprintf (stderr, "Sequence< EnhancedCustomShapeTextFrame > aTextFrameSeq (%" SAL_PRIdINT32 ");\n", segTextFrame.getLength());
        for( int i=0; i<segTextFrame.getLength(); i++ ) {
            printLevel (level);
            fprintf (stderr, "{\n");
            const char *var = lclDumpAnyValueCode (Any (segTextFrame[i]), level + 1);
            printLevel (level + 1);
            fprintf (stderr, "aTextFrameSeq [%d] = %s;\n", i, var);
            printLevel (level);
            fprintf (stderr, "}\n");
        }
        return "aTextFrameSeq";
    }
    else if( value >>= ppArray )
    {
        printLevel (level);
        if (ppArray.getLength() == 0)
            return "Sequence< EnhancedCustomShapeParameterPair >(0)";

        fprintf (stderr, "static const CustomShapeProvider::ParameterPairData aData[] = {\n");
        for( int i = 0; i < ppArray.getLength(); i++ ) {
            printParameterPairData(level + 1, ppArray[i]);
            fprintf (stderr,"%s\n", i < ppArray.getLength() - 1 ? "," : "");
        }
        printLevel (level);
        fprintf (stderr,"};\n");

        return "createParameterPairSequence(SAL_N_ELEMENTS(aData), aData)";
    }
    else if( value >>= segment )
    {
        printLevel (level);
        fprintf (stderr, "EnhancedCustomShapeSegment aSegment;\n");
        printLevel (level);
        // TODO: use EnhancedCustomShapeSegmentCommand constants
        fprintf (stderr, "aSegment.Command = %d;\n", segment.Command);
        printLevel (level);
        fprintf (stderr, "aSegment.Count = %d;\n", segment.Count);
        return "aSegment";
    }
    else if( value >>= textFrame )
    {
        printLevel (level);
        fprintf (stderr, "EnhancedCustomShapeTextFrame aTextFrame;\n");
        printLevel (level);
        fprintf (stderr, "{\n");
        {
            const char* var = lclDumpAnyValueCode( Any (textFrame.TopLeft), level + 1 );
            printLevel (level + 1);
            fprintf (stderr, "aTextFrame.TopLeft = %s;\n", var);
        }
        printLevel (level);
        fprintf (stderr, "}\n");

        printLevel (level);
        fprintf (stderr, "{\n");
        {
            const char* var = lclDumpAnyValueCode( Any (textFrame.BottomRight), level + 1 );
            printLevel (level + 1);
            fprintf (stderr, "aTextFrame.BottomRight = %s;\n", var);
        }
        printLevel (level);
        fprintf (stderr, "}\n");

        return "aTextFrame";
    }
    else if( value >>= pp )
    {
        printLevel (level);
        fprintf (stderr, "static const CustomShapeProvider::ParameterPairData aData =\n");
        printParameterPairData(level, pp);
        fprintf (stderr, ";\n");

        return "createParameterPair(&aData)";
    }
    else if( value >>= par )
    {
        printLevel (level);
        fprintf (stderr,"EnhancedCustomShapeParameter aParameter;\n");
        const char* var = lclDumpAnyValueCode( par.Value, level );
        printLevel (level);
        fprintf (stderr,"aParameter.Value = %s;\n", var);
        printLevel (level);
        fprintf (stderr,"aParameter.Type = %s;\n",
                lclGetEnhancedParameterType(par.Type));
        return "aParameter";
    }
    else if( value >>= int64Value )
    {
        printLevel (level);
        fprintf (stderr,"Any aAny ((sal_Int64) %" SAL_PRIdINT64 ");\n", int64Value);
        return "aAny";
    }
    else if( value >>= intValue )
        fprintf (stderr,"%" SAL_PRIdINT32 "            (hex: %" SAL_PRIxUINT32 ")\n", intValue, intValue);
    else if( value >>= uintValue )
        fprintf (stderr,"%" SAL_PRIdINT32 "            (hex: %" SAL_PRIxUINT32 ")\n", uintValue, uintValue);
    else if( value >>= int16Value )
        fprintf (stderr,"%d            (hex: %x)\n", int16Value, int16Value);
    else if( value >>= uint16Value )
        fprintf (stderr,"%d            (hex: %x)\n", uint16Value, uint16Value);
    else if( value >>= floatValue )
        fprintf (stderr,"%f\n", floatValue);
    else if( value >>= boolValue ) {
        if (boolValue)
            return "(sal_Bool) sal_True";
        else
            return "(sal_Bool) sal_False";
    }
    else if( value >>= xNumRule ) {
        fprintf (stderr, "XIndexReplace\n");
        for (int k=0; k<xNumRule->getCount(); k++) {
            Sequence< PropertyValue > aBulletPropSeq;
            fprintf (stderr, "level %d\n", k);
            if (xNumRule->getByIndex (k) >>= aBulletPropSeq) {
                for (const PropertyValue& rProp : aBulletPropSeq) {
                    fprintf(stderr, "%46s = ", USS (rProp.Name));
                    lclDumpAnyValue (rProp.Value);
                }
            }
        }
    }
    else if( value >>= aWritingMode )
        fprintf (stderr, "%d writing mode\n", static_cast<int>(aWritingMode));
    else if( value >>= aTextVertAdj ) {
        const char* s = "unknown";
        switch( aTextVertAdj ) {
            case TextVerticalAdjust_TOP:
                s = "top";
                break;
            case TextVerticalAdjust_CENTER:
                s = "center";
                break;
            case TextVerticalAdjust_BOTTOM:
                s = "bottom";
                break;
            case TextVerticalAdjust_BLOCK:
                s = "block";
                break;
            case TextVerticalAdjust::TextVerticalAdjust_MAKE_FIXED_SIZE:
                s = "make_fixed_size";
                break;
        }
        fprintf (stderr, "%s\n", s);
    }
    else if( value >>= aTextHorizAdj ) {
        const char* s = "unknown";
        switch( aTextHorizAdj ) {
            case TextHorizontalAdjust_LEFT:
                s = "left";
                break;
            case TextHorizontalAdjust_CENTER:
                s = "center";
                break;
            case TextHorizontalAdjust_RIGHT:
                s = "right";
                break;
            case TextHorizontalAdjust_BLOCK:
                s = "block";
                break;
            case TextHorizontalAdjust::TextHorizontalAdjust_MAKE_FIXED_SIZE:
                s = "make_fixed_size";
                break;
        }
        fprintf (stderr, "%s\n", s);
    }
    else if( value >>= spacing ) {
        fprintf (stderr, "mode: %d value: %d\n", spacing.Mode, spacing.Height);
    }
    else if( value >>= rect ) {
        printLevel (level);
        fprintf (stderr, "awt::Rectangle aRectangle;\n");
        printLevel (level);
        fprintf (stderr, "aRectangle.X = %" SAL_PRIdINT32 ";\n", rect.X);
        printLevel (level);
        fprintf (stderr, "aRectangle.Y = %" SAL_PRIdINT32 ";\n", rect.Y);
        printLevel (level);
        fprintf (stderr, "aRectangle.Width = %" SAL_PRIdINT32 ";\n", rect.Width);
        printLevel (level);
        fprintf (stderr, "aRectangle.Height = %" SAL_PRIdINT32 ";\n", rect.Height);
        return "aRectangle";
    }
    else if( value >>= size ) {
        printLevel (level);
        fprintf (stderr, "awt::Size aSize;\n");
        printLevel (level);
        fprintf (stderr, "aSize.Width = %" SAL_PRIdINT32 ";\n", size.Width);
        printLevel (level);
        fprintf (stderr, "aSize.Height = %" SAL_PRIdINT32 ";\n", size.Height);
        return "aSize";
    }
    else if( value.isExtractableTo(::cppu::UnoType<sal_Int32>::get())) {
        fprintf (stderr,"is extractable to int32\n");
    }
    else
        fprintf (stderr,"???           <unhandled type %s>\n", USS(value.getValueTypeName()));

    return "";
}

void PropertyMap::dumpCode( const Reference< XPropertySet >& rXPropSet )
{
    Reference< XPropertySetInfo > info = rXPropSet->getPropertySetInfo ();
    const Sequence< Property > props = info->getProperties ();
    static constexpr OUStringLiteral sType = u"Type";

    for (const Property& rProp : props) {

        // ignore Type, it is set elsewhere
        if (rProp.Name == sType)
            continue;

        OString name = OUStringToOString( rProp.Name, RTL_TEXTENCODING_UTF8);

        try {
            int level = 1;
            printLevel (level);
            fprintf (stderr, "{\n");
            const char* var = lclDumpAnyValueCode (rXPropSet->getPropertyValue (rProp.Name), level + 1);
            printLevel (level + 1);
            fprintf (stderr,"aPropertyMap.setProperty(PROP_%s, %s);\n", name.getStr(), var);
            printLevel (level);
            fprintf (stderr, "}\n");
        } catch (const Exception&) {
            fprintf (stderr,"unable to get '%s' value\n", USS(rProp.Name));
        }
    }
}

void PropertyMap::dumpData(const Reference<XPropertySet>& xPropertySet)
{
    Reference<XPropertySetInfo> xPropertySetInfo = xPropertySet->getPropertySetInfo();
    const Sequence<Property> aProperties = xPropertySetInfo->getProperties();

    for (const Property& rProp : aProperties)
    {
        std::cerr << rProp.Name << std::endl;
        std::cerr << comphelper::anyToString(xPropertySet->getPropertyValue(rProp.Name)) << std::endl;
    }
}

#endif

} // namespace oox

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
