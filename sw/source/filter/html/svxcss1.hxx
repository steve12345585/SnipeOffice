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
#ifndef INCLUDED_SW_SOURCE_FILTER_HTML_SVXCSS1_HXX
#define INCLUDED_SW_SOURCE_FILTER_HTML_SVXCSS1_HXX

#include <svl/itemset.hxx>
#include <editeng/svxenum.hxx>
#include <rtl/textenc.h>
#include "parcss1.hxx"
#include <o3tl/typed_flags_set.hxx>
#include <o3tl/unit_conversion.hxx>

#include <array>
#include <map>
#include <memory>
#include <vector>

class SfxItemPool;
class SvxBoxItem;
class FontList;
enum class SvxBoxItemLine;

enum SvxCSS1Position
{
    SVX_CSS1_POS_NONE,          // nothing specified
    SVX_CSS1_POS_STATIC,        // normal
    SVX_CSS1_POS_ABSOLUTE,      // absolute
    SVX_CSS1_POS_RELATIVE,      // relative
};

enum SvxCSS1LengthType
{
    SVX_CSS1_LTYPE_NONE,        // nothing specified
    SVX_CSS1_LTYPE_AUTO,        // automatic
    SVX_CSS1_LTYPE_TWIP,        // twip
    SVX_CSS1_LTYPE_PERCENTAGE,  // percentage value
};

// Feature: PrintExt
enum SvxCSS1SizeType
{
    SVX_CSS1_STYPE_NONE,        // nothing specified
    SVX_CSS1_STYPE_AUTO,        // automatic
    SVX_CSS1_STYPE_TWIP,        // twip
    SVX_CSS1_STYPE_LANDSCAPE,   // landscape
    SVX_CSS1_STYPE_PORTRAIT,    // portrait
};

enum SvxCSS1PageBreak
{
    SVX_CSS1_PBREAK_NONE,       // nothing specified
    SVX_CSS1_PBREAK_AUTO,       // automatic
    SVX_CSS1_PBREAK_ALWAYS,     // always
    SVX_CSS1_PBREAK_AVOID,      // never
    SVX_CSS1_PBREAK_LEFT,       // next page is a left one
    SVX_CSS1_PBREAK_RIGHT,      // next page is a right one
};


enum class Css1ScriptFlags {
    Western = 0x01,
    CJK     = 0x02,
    CTL     = 0x04,
    AllMask = Western | CJK | CTL,
};
namespace o3tl {
    template<> struct typed_flags<Css1ScriptFlags> : is_typed_flags<Css1ScriptFlags, 0x07> {};
}

struct CSS1PropertyEnum
{
    const char *pName;  // property value
    sal_uInt16 nEnum;       // and the corresponding value of enum
};

namespace editeng { class SvxBorderLine; }

#define SVX_CSS1_BORDERINFO_WIDTH 1
#define SVX_CSS1_BORDERINFO_COLOR 2
#define SVX_CSS1_BORDERINFO_STYLE 4

struct SvxCSS1BorderInfo;
class SvxCSS1PropertyInfo
{
    std::array<std::unique_ptr<SvxCSS1BorderInfo>,4> m_aBorderInfos;

    void DestroyBorderInfos();

public:
    static constexpr sal_uInt16 UNSET_BORDER_DISTANCE = SAL_MAX_UINT16;

    OUString m_aId;             // ID for bookmarks, frame, and so

    bool m_bTopMargin : 1;
    bool m_bBottomMargin : 1;

    bool m_bLeftMargin : 1;
    bool m_bRightMargin : 1;
    bool m_bTextIndent : 1;
    bool m_bNumbering : 1;
    bool m_bBullet : 1;
    bool m_bPreserveSpace : 1 = false;

    SvxAdjust m_eFloat;

    SvxCSS1Position m_ePosition;

    sal_uInt16 m_nTopBorderDistance;
    sal_uInt16 m_nBottomBorderDistance;
    sal_uInt16 m_nLeftBorderDistance;
    sal_uInt16 m_nRightBorderDistance;

    SvxNumType m_nNumberingType;
    sal_Unicode m_cBulletChar;

    sal_uInt16 m_nColumnCount;

    tools::Long m_nLeft, m_nTop;
    tools::Long m_nWidth, m_nHeight;
    tools::Long m_nLeftMargin, m_nRightMargin;

    SvxCSS1LengthType m_eLeftType, m_eTopType;
    SvxCSS1LengthType m_eWidthType, m_eHeightType;
    SvxCSS1LengthType m_eLeftMarginType;
    SvxCSS1LengthType m_eRightMarginType;

    SvxCSS1SizeType m_eSizeType;

    SvxCSS1PageBreak m_ePageBreakBefore;
    SvxCSS1PageBreak m_ePageBreakAfter;

    bool m_bVisible = true;

    SvxCSS1PropertyInfo();
    SvxCSS1PropertyInfo( const SvxCSS1PropertyInfo& rProp );
    ~SvxCSS1PropertyInfo();

    void Merge( const SvxCSS1PropertyInfo& rProp );

    void Clear();

    SvxCSS1BorderInfo *GetBorderInfo( SvxBoxItemLine nLine, bool bCreate=true );
    void CopyBorderInfo( SvxBoxItemLine nSrcLine, SvxBoxItemLine nDstLine, sal_uInt16 nWhat );
    void CopyBorderInfo( sal_uInt16 nCount, sal_uInt16 nWhat );

    void SetBoxItem( SfxItemSet& rItemSet, sal_uInt16 nMinBorderDist,
                     const SvxBoxItem* pDflt=nullptr );

};

class SvxCSS1MapEntry
{
    SfxItemSet m_aItemSet;
    SvxCSS1PropertyInfo m_aPropInfo;

public:
    SvxCSS1MapEntry( SfxItemSet aItemSet,
                     const SvxCSS1PropertyInfo& rProp );

    const SfxItemSet& GetItemSet() const { return m_aItemSet; }
    SfxItemSet& GetItemSet() { return m_aItemSet; }

    const SvxCSS1PropertyInfo& GetPropertyInfo() const { return m_aPropInfo; }
    SvxCSS1PropertyInfo& GetPropertyInfo() { return m_aPropInfo; }
};

// Class is processing the CSS1-Parser output by converting the CSS1 properties
// into SvxItem(Set). Also the selectors together with associated ItemSet are
// saved.
// A derived parser can suppress this for certain selectors by overriding
// the method StyleParsed.

class SvxCSS1Parser : public CSS1Parser
{
    typedef std::vector<std::unique_ptr<CSS1Selector>> CSS1Selectors;
    typedef std::map<OUString, std::unique_ptr<SvxCSS1MapEntry>> CSS1Map;
    CSS1Selectors m_Selectors;   // List of "open" Selectors

    CSS1Map m_Ids;
    CSS1Map m_Classes;
    CSS1Map m_Pages;
    CSS1Map m_Tags;

    OUString m_sBaseURL;

    std::unique_ptr<SfxItemSet> m_pSheetItemSet;  // item set of Style-Sheet
    SfxItemSet *m_pItemSet;       // current item set

    std::unique_ptr<SvxCSS1PropertyInfo> m_pSheetPropInfo;
    SvxCSS1PropertyInfo *m_pPropInfo;

    // minimum spacing for fixed line spacing
    static constexpr sal_uInt16 gnMinFixLineSpace = o3tl::toTwips(25, o3tl::Length::mm10);

    rtl_TextEncoding    m_eDefaultEnc;
    bool m_bIgnoreFontFamily;
    WhichRangesContainer m_aWhichMap;        // Which-Map of Parser

    using CSS1Parser::ParseStyleOption;

protected:

    using CSS1Parser::ParseStyleSheet;

    // This method is called for every selector with according item set.
    // For a selector multiple calls are possible.
    // If true is returned then the item set resp. the selector isn't saved anymore!
    // The ItemSet may be modified accordingly!
    // The implementation returns false.
    virtual void StyleParsed( const CSS1Selector *pSelector,
                              SfxItemSet& rItemSet,
                              SvxCSS1PropertyInfo& rPropInfo );

    /// Will be called when a Selector is parsed.  If bFirst is true,
    /// the content of the aItemSet will be copied into all recently
    /// created Styles.
    /// Derived classes should not override this method!
    virtual void SelectorParsed( std::unique_ptr<CSS1Selector> pSelector, bool bFirst ) override;

    /// Will be called for every parsed Property.  Adds the item to the
    /// pItemSet.
    /// Derived classes should not override this method!
    virtual void DeclarationParsed( const OUString& rProperty,
                                    std::unique_ptr<CSS1Expression> pExpr ) override;

public:

    SvxCSS1Parser( SfxItemPool& rPool,
                    OUString aBaseURL,
                   sal_uInt16 const *pWhichIds, sal_uInt16 nWhichIds );
    virtual ~SvxCSS1Parser() override;

    bool IsIgnoreFontFamily() const { return m_bIgnoreFontFamily; }
    void SetIgnoreFontFamily( bool bSet ) { m_bIgnoreFontFamily = bSet; }

    // Parse a style sheet. For every found selector a StyleParsed with
    // according item set is called.
    virtual bool ParseStyleSheet( const OUString& rIn );

    // Parse style option. Here only the item set is filled.
    void ParseStyleOption( const OUString& rIn, SfxItemSet& rItemSet,
                           SvxCSS1PropertyInfo& rPropInfo );

    // convert a string to enum value
    static bool GetEnum( const CSS1PropertyEnum *pPropTable,
                         std::u16string_view rValue, sal_uInt16 &rEnum );

    static void PixelToTwip( tools::Long &nWidth, tools::Long &nHeight );

    // determine the font height of a certain font size (0-6)
    virtual sal_uInt32 GetFontHeight( sal_uInt16 nSize ) const;

    virtual const FontList *GetFontList() const;

    const WhichRangesContainer& GetWhichMap() const { return m_aWhichMap; }

    static void InsertMapEntry( const OUString& rKey, const SfxItemSet& rItemSet,
                         const SvxCSS1PropertyInfo& rProp, CSS1Map& rMap );

    void InsertId( const OUString& rId, const SfxItemSet& rItemSet,
                   const SvxCSS1PropertyInfo& rProp );

    const SvxCSS1MapEntry* GetId( const OUString& rId ) const;

    void InsertClass( const OUString& rClass, const SfxItemSet& rItemSet,
                      const SvxCSS1PropertyInfo& rProp );

    const SvxCSS1MapEntry* GetClass( const OUString& rClass ) const;

    void InsertPage( const OUString& rPage, bool bPseudo,
                            const SfxItemSet& rItemSet,
                            const SvxCSS1PropertyInfo& rProp );

    SvxCSS1MapEntry* GetPage( const OUString& rPage, bool bPseudo );

    void InsertTag( const OUString& rTag, const SfxItemSet& rItemSet,
                      const SvxCSS1PropertyInfo& rProp );

    SvxCSS1MapEntry* GetTag( const OUString& rTag );

    static void MergeStyles( const SfxItemSet& rSrcSet,
                      const SvxCSS1PropertyInfo& rSrcInfo,
                      SfxItemSet& rTargetSet,
                      SvxCSS1PropertyInfo& rTargetInfo,
                      bool bSmart );

    static sal_uInt16 GetMinFixLineSpace() { return gnMinFixLineSpace; }

    virtual void SetDfltEncoding( rtl_TextEncoding eEnc );
    rtl_TextEncoding GetDfltEncoding() const { return m_eDefaultEnc; }

    const OUString& GetBaseURL() const { return m_sBaseURL;}

};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
