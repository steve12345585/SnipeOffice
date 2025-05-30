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

#ifndef INCLUDED_OOX_DRAWINGML_TEXTPARAGRAPHPROPERTIES_HXX
#define INCLUDED_OOX_DRAWINGML_TEXTPARAGRAPHPROPERTIES_HXX

#include <com/sun/star/beans/XPropertySet.hpp>
#include <drawingml/textcharacterproperties.hxx>
#include <com/sun/star/style/ParagraphAdjust.hpp>
#include <drawingml/textfont.hxx>
#include <drawingml/textspacing.hxx>
#include <optional>

namespace com::sun::star {
    namespace graphic { class XGraphic; }
}

namespace oox::drawingml {

class BulletList
{
public:
    BulletList( );
    bool is() const;
    void apply( const BulletList& );
    void pushToPropMap( const ::oox::core::XmlFilterBase* pFilterBase, PropertyMap& xPropMap ) const;
    void setBulletChar( const OUString & sChar );
    void setStartAt( sal_Int32 nStartAt ){ mnStartAt <<= static_cast< sal_Int16 >( nStartAt ); }
    void setType( sal_Int32 nType );
    void setNone( );
    void setSuffixParenBoth();
    void setSuffixParenRight();
    void setSuffixPeriod();
    void setSuffixNone();
    void setSuffixMinusRight();
    void setBulletSize(sal_Int16 nSize);
    void setBulletAspectRatio(double nAspectRatio);
    void setFontSize(sal_Int16 nSize);
    void setStyleName( const OUString& rStyleName ) { maStyleName <<= rStyleName; }
    void setGraphic( css::uno::Reference< css::graphic::XGraphic > const & rXGraphic );

    std::shared_ptr< ::oox::drawingml::Color > maBulletColorPtr;
    css::uno::Any               mbBulletColorFollowText;
    css::uno::Any               mbBulletFontFollowText;
    css::uno::Any               mbBulletSizeFollowText;
    ::oox::drawingml::TextFont  maBulletFont;
    css::uno::Any               msBulletChar;
    css::uno::Any               mnStartAt;
    css::uno::Any               mnNumberingType;
    css::uno::Any               msNumberingPrefix;
    css::uno::Any               msNumberingSuffix;
    css::uno::Any               mnSize;
    css::uno::Any               mnAspectRatio; // Width/Height
    css::uno::Any               mnFontSize;
    css::uno::Any               maStyleName;
    css::uno::Any               maGraphic;
};

class TextParagraphProperties
{
public:

    TextParagraphProperties();

    void                                setLevel( sal_Int16 nLevel ) { mnLevel = nLevel; }
    sal_Int16                           getLevel( ) const { return mnLevel; }
    PropertyMap&                        getTextParagraphPropertyMap() { return maTextParagraphPropertyMap; }
    BulletList&                         getBulletList() { return maBulletList; }
    TextCharacterProperties&            getTextCharacterProperties() { return maTextCharacterProperties; }
    const TextCharacterProperties&      getTextCharacterProperties() const { return maTextCharacterProperties; }

    TextSpacing&                        getParaTopMargin() { return maParaTopMargin; }
    TextSpacing&                        getParaBottomMargin() { return maParaBottomMargin; }
    std::optional< sal_Int32 >&       getParaLeftMargin(){ return moParaLeftMargin; }
    std::optional< sal_Int32 >&       getFirstLineIndentation(){ return moFirstLineIndentation; }
    std::optional<sal_Int32>&         getDefaultTabSize() { return moDefaultTabSize; }

    std::optional< css::style::ParagraphAdjust >&       getParaAdjust() { return moParaAdjust; }
    void                                setParaAdjust( css::style::ParagraphAdjust nParaAdjust ) { moParaAdjust = nParaAdjust; }

    TextSpacing&                        getLineSpacing() { return maLineSpacing; }
    void                                setLineSpacing( const TextSpacing& rLineSpacing ) { maLineSpacing = rLineSpacing; }

    void                                apply( const TextParagraphProperties& rSourceProps );
    void                                pushToPropSet( const ::oox::core::XmlFilterBase* pFilterBase,
                                                const css::uno::Reference < css::beans::XPropertySet > & xPropSet,
                                                PropertyMap& rioBulletList,
                                                const BulletList* pMasterBuList,
                                                bool bApplyBulletList,
                                                float fFontSize,
                                                bool bPushDefaultValues = false ) const;

    /** Returns the largest character size of this paragraph. If possible the
        masterstyle should have been applied before, otherwise the character
        size can be zero and the default value is returned. */
    float                               getCharHeightPoints( float fDefault ) const;

#ifdef DBG_UTIL
    void dump() const;
#endif

private:

    TextCharacterProperties         maTextCharacterProperties;
    PropertyMap                     maTextParagraphPropertyMap;
    BulletList                      maBulletList;
    TextSpacing                     maParaTopMargin;
    TextSpacing                     maParaBottomMargin;
    std::optional< sal_Int32 >    moParaLeftMargin;
    std::optional< sal_Int32 >    moFirstLineIndentation;
    std::optional< css::style::ParagraphAdjust >    moParaAdjust;
    std::optional< sal_Int32 >      moDefaultTabSize;
    sal_Int16                       mnLevel;
    TextSpacing                     maLineSpacing;
};

}

#endif // INCLUDED_OOX_DRAWINGML_TEXTPARAGRAPHPROPERTIES_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
