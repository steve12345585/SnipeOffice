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

#include <comphelper/processfactory.hxx>
#include <editeng/acorrcfg.hxx>
#include <o3tl/any.hxx>
#include <tools/debug.hxx>
#include <tools/urlobj.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <ucbhelper/content.hxx>
#include <unotools/pathoptions.hxx>
#include <unotools/ucbhelper.hxx>
#include <svtools/langtab.hxx>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/sdbc/XRow.hpp>

#include <editeng/svxacorr.hxx>
#include <com/sun/star/uno/Sequence.hxx>

using namespace utl;
using namespace com::sun::star;
using namespace com::sun::star::uno;


/** An autocorrection file dropped into such directory may create a language
    list entry if one didn't exist already.
 */
static void scanAutoCorrectDirForLanguageTags( const OUString& rURL )
{
    // Silently ignore all errors.
    try
    {
        ::ucbhelper::Content aContent( rURL,
                uno::Reference<ucb::XCommandEnvironment>(), comphelper::getProcessComponentContext());
        if (aContent.isFolder())
        {
            // Title is file name here.
            uno::Reference<sdbc::XResultSet> xResultSet = aContent.createCursor(
                    {u"Title"_ustr}, ::ucbhelper::INCLUDE_DOCUMENTS_ONLY);
            uno::Reference<sdbc::XRow> xRow( xResultSet, UNO_QUERY);
            if (xResultSet.is() && xRow.is())
            {
                while (xResultSet->next())
                {
                    try
                    {
                        const OUString aTitle( xRow->getString(1));
                        if (aTitle.getLength() <= 9 || !(aTitle.startsWith("acor_") && aTitle.endsWith(".dat")))
                            continue;

                        const OUString aBcp47( aTitle.copy( 5, aTitle.getLength() - 9));
                        // Ignore invalid langtags and canonicalize for good,
                        // allow private-use tags.
                        const LanguageTag aLanguageTag (aBcp47, true);
                        if (!aLanguageTag.isValidBcp47())
                            continue;

                        if (SvtLanguageTable::HasLanguageType( aLanguageTag.getLanguageType()))
                            continue;

                        // Insert language(-script)-only tags only if there is
                        // no known matching fallback locale, otherwise we'd
                        // end up with unwanted entries where a language
                        // autocorrection file covers several locales. We do
                        // know a few art-x-... though so exclude those and any
                        // other private-use tag (which should not fallback,
                        // but avoid).
                        if (aLanguageTag.getCountry().isEmpty()
                                && LanguageTag::isValidBcp47( aLanguageTag.getBcp47(), nullptr,
                                    LanguageTag::PrivateUse::DISALLOW))
                        {
                            LanguageTag aFallback( aLanguageTag);
                            aFallback.makeFallback();
                            if (aFallback.getLanguageAndScript() == aLanguageTag.getLanguageAndScript())
                                continue;
                        }

                        // Finally add this one.
                        SvtLanguageTable::AddLanguageTag( aLanguageTag);
                    }
                    catch (const uno::Exception&)
                    {
                        TOOLS_WARN_EXCEPTION("editeng", "Unable to get a directory entry from '" << rURL << "'");
                    }
                }
            }
        }
    }
    catch (const uno::Exception&)
    {
        TOOLS_WARN_EXCEPTION("editeng", "Unable to iterate directory '" << rURL << "'");
    }
}

SvxAutoCorrCfg::SvxAutoCorrCfg() :
    aBaseConfig(*this),
    aSwConfig(*this),
    bFileRel(true),
    bNetRel(true),
    bAutoTextTip(true),
    bAutoTextPreview(false),
    bAutoFmtByInput(true),
    bSearchInAllCategories(false)
{
    SvtPathOptions aPathOpt;
    OUString sSharePath, sUserPath;
    OUString const & sAutoPath( aPathOpt.GetAutoCorrectPath() );

    sSharePath = sAutoPath.getToken(0, ';');
    sUserPath = sAutoPath.getToken(1, ';');

    //fdo#67743 ensure the userdir exists so that any later attempt to copy the
    //shared autocorrect file into the user dir will succeed
    ::ucbhelper::Content aContent;
    Reference < ucb::XCommandEnvironment > xEnv;
    ::utl::UCBContentHelper::ensureFolder(comphelper::getProcessComponentContext(), xEnv, sUserPath, aContent);

    for( OUString* pS : { &sSharePath, &sUserPath } )
    {
        INetURLObject aPath( *pS );
        scanAutoCorrectDirForLanguageTags( aPath.GetMainURL(INetURLObject::DecodeMechanism::ToIUri));
        aPath.insertName(u"acor");
        *pS = aPath.GetMainURL(INetURLObject::DecodeMechanism::ToIUri);
    }
    pAutoCorrect.reset( new SvxAutoCorrect( sSharePath, sUserPath ) );

    aBaseConfig.Load(true);
    aSwConfig.Load(true);
}

SvxAutoCorrCfg::~SvxAutoCorrCfg()
{
}

void SvxAutoCorrCfg::SetAutoCorrect(SvxAutoCorrect *const pNew)
{
    if (pNew != pAutoCorrect.get())
    {
        if (pNew && (pAutoCorrect->GetFlags() != pNew->GetFlags()))
        {
            aBaseConfig.SetModified();
            aSwConfig.SetModified();
        }
        pAutoCorrect.reset( pNew );
    }
}

Sequence<OUString>  SvxBaseAutoCorrCfg::GetPropertyNames()
{
    static constexpr OUString aPropNames[]
    {
        u"Exceptions/TwoCapitalsAtStart"_ustr,        //  0
        u"Exceptions/CapitalAtStartSentence"_ustr,    //  1
        u"UseReplacementTable"_ustr,                  //  2
        u"TwoCapitalsAtStart"_ustr,                   //  3
        u"CapitalAtStartSentence"_ustr,               //  4
        u"ChangeUnderlineWeight"_ustr,                //  5
        u"SetInetAttribute"_ustr,                     //  6
        u"ChangeOrdinalNumber"_ustr,                  //  7
        u"AddNonBreakingSpace"_ustr,                  //  8
        u"ChangeDash"_ustr,                           //  9
        u"RemoveDoubleSpaces"_ustr,                   // 10
        u"ReplaceSingleQuote"_ustr,                   // 11
        u"SingleQuoteAtStart"_ustr,                   // 12
        u"SingleQuoteAtEnd"_ustr,                     // 13
        u"ReplaceDoubleQuote"_ustr,                   // 14
        u"DoubleQuoteAtStart"_ustr,                   // 15
        u"DoubleQuoteAtEnd"_ustr,                     // 16
        u"CorrectAccidentalCapsLock"_ustr,            // 17
        u"TransliterateRTL"_ustr,                     // 18
        u"ChangeAngleQuotes"_ustr,                    // 19
        u"SetDOIAttribute"_ustr,                      // 20
    };
    const int nCount = 21;
    Sequence<OUString> aNames(nCount);
    OUString* pNames = aNames.getArray();
    for(int i = 0; i < nCount; i++)
        pNames[i] = aPropNames[i];
    return aNames;
}

void SvxBaseAutoCorrCfg::Load(bool bInit)
{
    Sequence<OUString> aNames = GetPropertyNames();
    Sequence<Any> aValues = GetProperties(aNames);
    if(bInit)
        EnableNotification(aNames);
    const Any* pValues = aValues.getConstArray();
    DBG_ASSERT(aValues.getLength() == aNames.getLength(), "GetProperties failed");
    if(aValues.getLength() != aNames.getLength())
        return;

    ACFlags nFlags = ACFlags::NONE;        // default all off
    sal_Int32 nTemp = 0;
    for(int nProp = 0; nProp < aNames.getLength(); nProp++)
    {
        if(pValues[nProp].hasValue())
        {
            switch(nProp)
            {
                case  0:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::SaveWordCplSttLst;
                break;//"Exceptions/TwoCapitalsAtStart",
                case  1:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::SaveWordWordStartLst;
                break;//"Exceptions/CapitalAtStartSentence",
                case  2:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::Autocorrect;
                break;//"UseReplacementTable",
                case  3:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::CapitalStartWord;
                break;//"TwoCapitalsAtStart",
                case  4:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::CapitalStartSentence;
                break;//"CapitalAtStartSentence",
                case  5:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::ChgWeightUnderl;
                break;//"ChangeUnderlineWeight",
                case  6:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::SetINetAttr;
                break;//"SetInetAttribute",
                case  7:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::ChgOrdinalNumber;
                break;//"ChangeOrdinalNumber",
                case 8:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                         nFlags |= ACFlags::AddNonBrkSpace;
                break;//"AddNonBreakingSpace"
                case  9:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::ChgToEnEmDash;
                break;//"ChangeDash",
                case 10:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::IgnoreDoubleSpace;
                break;//"RemoveDoubleSpaces",
                case 11:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::ChgSglQuotes;
                break;//"ReplaceSingleQuote",
                case 12:
                    pValues[nProp] >>= nTemp;
                    rParent.pAutoCorrect->SetStartSingleQuote(
                        sal::static_int_cast< sal_Unicode >( nTemp ) );
                break;//"SingleQuoteAtStart",
                case 13:
                    pValues[nProp] >>= nTemp;
                    rParent.pAutoCorrect->SetEndSingleQuote(
                        sal::static_int_cast< sal_Unicode >( nTemp ) );
                break;//"SingleQuoteAtEnd",
                case 14:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::ChgQuotes;
                break;//"ReplaceDoubleQuote",
                case 15:
                    pValues[nProp] >>= nTemp;
                    rParent.pAutoCorrect->SetStartDoubleQuote(
                        sal::static_int_cast< sal_Unicode >( nTemp ) );
                break;//"DoubleQuoteAtStart",
                case 16:
                    pValues[nProp] >>= nTemp;
                    rParent.pAutoCorrect->SetEndDoubleQuote(
                        sal::static_int_cast< sal_Unicode >( nTemp ) );
                break;//"DoubleQuoteAtEnd"
                case 17:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::CorrectCapsLock;
                break;//"CorrectAccidentalCapsLock"
                case 18:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::TransliterateRTL;
                break;//"TransliterateRTL"
                case 19:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::ChgAngleQuotes;
                break;//"ChangeAngleQuotes"
                case  20:
                    if(*o3tl::doAccess<bool>(pValues[nProp]))
                        nFlags |= ACFlags::SetDOIAttr;
                break;//"SetDOIAttr",
            }
        }
    }
    if( nFlags != ACFlags::NONE )
        rParent.pAutoCorrect->SetAutoCorrFlag( nFlags );
    rParent.pAutoCorrect->SetAutoCorrFlag( ( static_cast<ACFlags>(0xffff) & ~nFlags ), false );
}

SvxBaseAutoCorrCfg::SvxBaseAutoCorrCfg(SvxAutoCorrCfg& rPar) :
    utl::ConfigItem(u"Office.Common/AutoCorrect"_ustr),
    rParent(rPar)
{
}

SvxBaseAutoCorrCfg::~SvxBaseAutoCorrCfg()
{
}

void SvxBaseAutoCorrCfg::ImplCommit()
{
    const ACFlags nFlags = rParent.pAutoCorrect->GetFlags();
    PutProperties(
        GetPropertyNames(),
        {css::uno::Any(bool(nFlags & ACFlags::SaveWordCplSttLst)),
            // "Exceptions/TwoCapitalsAtStart"
         css::uno::Any(bool(nFlags & ACFlags::SaveWordWordStartLst)),
            // "Exceptions/CapitalAtStartSentence"
         css::uno::Any(bool(nFlags & ACFlags::Autocorrect)), // "UseReplacementTable"
         css::uno::Any(bool(nFlags & ACFlags::CapitalStartWord)),
            // "TwoCapitalsAtStart"
         css::uno::Any(bool(nFlags & ACFlags::CapitalStartSentence)),
            // "CapitalAtStartSentence"
         css::uno::Any(bool(nFlags & ACFlags::ChgWeightUnderl)),
            // "ChangeUnderlineWeight"
         css::uno::Any(bool(nFlags & ACFlags::SetINetAttr)), // "SetInetAttribute"
         css::uno::Any(bool(nFlags & ACFlags::ChgOrdinalNumber)),
            // "ChangeOrdinalNumber"
         css::uno::Any(bool(nFlags & ACFlags::AddNonBrkSpace)), // "AddNonBreakingSpace"
         css::uno::Any(bool(nFlags & ACFlags::ChgToEnEmDash)), // "ChangeDash"
         css::uno::Any(bool(nFlags & ACFlags::IgnoreDoubleSpace)),
            // "RemoveDoubleSpaces"
         css::uno::Any(bool(nFlags & ACFlags::ChgSglQuotes)), // "ReplaceSingleQuote"
         css::uno::Any(sal_Int32(rParent.pAutoCorrect->GetStartSingleQuote())),
            // "SingleQuoteAtStart"
         css::uno::Any(sal_Int32(rParent.pAutoCorrect->GetEndSingleQuote())),
            // "SingleQuoteAtEnd"
         css::uno::Any(bool(nFlags & ACFlags::ChgQuotes)), // "ReplaceDoubleQuote"
         css::uno::Any(sal_Int32(rParent.pAutoCorrect->GetStartDoubleQuote())),
            // "DoubleQuoteAtStart"
         css::uno::Any(sal_Int32(rParent.pAutoCorrect->GetEndDoubleQuote())),
            // "DoubleQuoteAtEnd"
         css::uno::Any(bool(nFlags & ACFlags::CorrectCapsLock)),
            // "CorrectAccidentalCapsLock"
         css::uno::Any(bool(nFlags & ACFlags::TransliterateRTL)),
            // "TransliterateRTL"
         css::uno::Any(bool(nFlags & ACFlags::ChgAngleQuotes)),
            // "ChangeAngleQuotes"
         css::uno::Any(bool(nFlags & ACFlags::SetDOIAttr)), // "SetDOIAttribute"
        });
}

void SvxBaseAutoCorrCfg::Notify( const Sequence<OUString>& /* aPropertyNames */)
{
    Load(false);
}

Sequence<OUString>  SvxSwAutoCorrCfg::GetPropertyNames()
{
    static constexpr OUString aPropNames[]
    {
        u"Text/FileLinks"_ustr,                             // 0
        u"Text/InternetLinks"_ustr,                         // 1
        u"Text/ShowPreview"_ustr,                           // 2
        u"Text/ShowToolTip"_ustr,                           // 3
        u"Text/SearchInAllCategories"_ustr,                                   // 4
        u"Format/Option/UseReplacementTable"_ustr,                            // 5
        u"Format/Option/TwoCapitalsAtStart"_ustr,                             // 6
        u"Format/Option/CapitalAtStartSentence"_ustr,                         // 7
        u"Format/Option/ChangeUnderlineWeight"_ustr,                          // 8
        u"Format/Option/SetInetAttribute"_ustr,                               // 9
        u"Format/Option/ChangeOrdinalNumber"_ustr,                            //10
        u"Format/Option/AddNonBreakingSpace"_ustr,                            //11
        u"Format/Option/ChangeDash"_ustr,                                     //12
        u"Format/Option/DelEmptyParagraphs"_ustr,                             //13
        u"Format/Option/ReplaceUserStyle"_ustr,                               //14
        u"Format/Option/ChangeToBullets/Enable"_ustr,                         //15
        u"Format/Option/ChangeToBullets/SpecialCharacter/Char"_ustr,          //16
        u"Format/Option/ChangeToBullets/SpecialCharacter/Font"_ustr,          //17
        u"Format/Option/ChangeToBullets/SpecialCharacter/FontFamily"_ustr,    //18
        u"Format/Option/ChangeToBullets/SpecialCharacter/FontCharset"_ustr,   //19
        u"Format/Option/ChangeToBullets/SpecialCharacter/FontPitch"_ustr,     //20
        u"Format/Option/CombineParagraphs"_ustr,                              //21
        u"Format/Option/CombineValue"_ustr,                                   //22
        u"Format/Option/DelSpacesAtStartEnd"_ustr,                            //23
        u"Format/Option/DelSpacesBetween"_ustr,                               //24
        u"Format/ByInput/Enable"_ustr,                                        //25
        u"Format/ByInput/ChangeDash"_ustr,                                    //26
        u"Format/ByInput/ApplyNumbering/Enable"_ustr,                         //27
        u"Format/ByInput/ChangeToBorders"_ustr,                               //28
        u"Format/ByInput/ChangeToTable"_ustr,                                 //29
        u"Format/ByInput/ReplaceStyle"_ustr,                                  //30
        u"Format/ByInput/DelSpacesAtStartEnd"_ustr,                           //31
        u"Format/ByInput/DelSpacesBetween"_ustr,                              //32
        u"Completion/Enable"_ustr,                                            //33
        u"Completion/MinWordLen"_ustr,                                        //34
        u"Completion/MaxListLen"_ustr,                                        //35
        u"Completion/CollectWords"_ustr,                                      //36
        u"Completion/EndlessList"_ustr,                                       //37
        u"Completion/AppendBlank"_ustr,                                       //38
        u"Completion/ShowAsTip"_ustr,                                         //39
        u"Completion/AcceptKey"_ustr,                                         //40
        u"Completion/KeepList"_ustr,                                          //41
        u"Format/ByInput/ApplyNumbering/SpecialCharacter/Char"_ustr,          //42
        u"Format/ByInput/ApplyNumbering/SpecialCharacter/Font"_ustr,          //43
        u"Format/ByInput/ApplyNumbering/SpecialCharacter/FontFamily"_ustr,    //44
        u"Format/ByInput/ApplyNumbering/SpecialCharacter/FontCharset"_ustr,   //45
        u"Format/ByInput/ApplyNumbering/SpecialCharacter/FontPitch"_ustr,     //46
        u"Format/Option/SetDOIAttribute"_ustr,                                //47
        u"Format/ByInput/ApplyBulletsAfterSpace"_ustr,                        //48
    };
    const int nCount = 49;
    Sequence<OUString> aNames(nCount);
    OUString* pNames = aNames.getArray();
    for(int i = 0; i < nCount; i++)
        pNames[i] = aPropNames[i];
    return aNames;
}

void SvxSwAutoCorrCfg::Load(bool bInit)
{
    Sequence<OUString> aNames = GetPropertyNames();
    Sequence<Any> aValues = GetProperties(aNames);
    if(bInit)
        EnableNotification(aNames);
    const Any* pValues = aValues.getConstArray();
    DBG_ASSERT(aValues.getLength() == aNames.getLength(), "GetProperties failed");
    if(aValues.getLength() != aNames.getLength())
        return;

    SvxSwAutoFormatFlags& rSwFlags = rParent.pAutoCorrect->GetSwFlags();
    for(int nProp = 0; nProp < aNames.getLength(); nProp++)
    {
        if(pValues[nProp].hasValue())
        {
            switch(nProp)
            {
                case   0: rParent.bFileRel = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Text/FileLinks",
                case   1: rParent.bNetRel = *o3tl::doAccess<bool>(pValues[nProp]);  break; // "Text/InternetLinks",
                case   2: rParent.bAutoTextPreview = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Text/ShowPreview",
                case   3: rParent.bAutoTextTip = *o3tl::doAccess<bool>(pValues[nProp]);  break; // "Text/ShowToolTip",
                case   4: rParent.bSearchInAllCategories = *o3tl::doAccess<bool>(pValues[nProp]);  break; //"Text/SearchInAllCategories"
                case   5: rSwFlags.bAutoCorrect = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/UseReplacementTable",
                case   6: rSwFlags.bCapitalStartSentence = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/TwoCapitalsAtStart",
                case   7: rSwFlags.bCapitalStartWord = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/CapitalAtStartSentence",
                case   8: rSwFlags.bChgWeightUnderl = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/ChangeUnderlineWeight",
                case   9: rSwFlags.bSetINetAttr = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/SetInetAttribute",
                case  10: rSwFlags.bChgOrdinalNumber = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/ChangeOrdinalNumber",
                case  11: rSwFlags.bAddNonBrkSpace = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/AddNonBreakingSpace",
// it doesn't exist here - the common flags are used for that -> LM
//                  case  12: rSwFlags.bChgToEnEmDash = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/ChangeDash",
                case  13: rSwFlags.bDelEmptyNode = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/DelEmptyParagraphs",
                case  14: rSwFlags.bChgUserColl = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/ReplaceUserStyle",
                case  15: rSwFlags.bChgEnumNum = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/ChangeToBullets/Enable",
                case  16:
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.cBullet =
                        sal::static_int_cast< sal_Unicode >(nVal);
                }
                break; // "Format/Option/ChangeToBullets/SpecialCharacter/Char",
                case  17:
                {
                    OUString sTemp; pValues[nProp] >>= sTemp;
                    rSwFlags.aBulletFont.SetFamilyName(sTemp);
                }
                break; // "Format/Option/ChangeToBullets/SpecialCharacter/Font",
                case  18:
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.aBulletFont.SetFamily(FontFamily(nVal));
                }
                break; // "Format/Option/ChangeToBullets/SpecialCharacter/FontFamily",
                case  19:
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.aBulletFont.SetCharSet(rtl_TextEncoding(nVal));
                }
                break; // "Format/Option/ChangeToBullets/SpecialCharacter/FontCharset",
                case  20:
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.aBulletFont.SetPitch(FontPitch(nVal));
                }
                break; // "Format/Option/ChangeToBullets/SpecialCharacter/FontPitch",
                case  21: rSwFlags.bRightMargin = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/CombineParagraphs",
                case  22:
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.nRightMargin =
                        sal::static_int_cast< sal_uInt8 >(nVal);
                }
                break; // "Format/Option/CombineValue",
                case  23: rSwFlags.bAFormatDelSpacesAtSttEnd =  *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/DelSpacesAtStartEnd",
                case  24: rSwFlags.bAFormatDelSpacesBetweenLines = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/DelSpacesBetween",
                case  25: rParent.bAutoFmtByInput = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/Enable",
                case  26: rSwFlags.bChgToEnEmDash = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/ChangeDash",
                case  27: rSwFlags.bSetNumRule = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/ApplyNumbering/Enable",
                case  28: rSwFlags.bSetBorder = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/ChangeToBorders",
                case  29: rSwFlags.bCreateTable = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/ChangeToTable",
                case  30: rSwFlags.bReplaceStyles =  *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/ReplaceStyle",
                case  31: rSwFlags.bAFormatByInpDelSpacesAtSttEnd =  *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/DelSpacesAtStartEnd",
                case  32: rSwFlags.bAFormatByInpDelSpacesBetweenLines = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/DelSpacesBetween",
                case  33: rSwFlags.bAutoCompleteWords = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Completion/Enable",
                case  34:
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.nAutoCmpltWordLen =
                        sal::static_int_cast< sal_uInt16 >(nVal);
                }
                break; // "Completion/MinWordLen",
                case  35:
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.nAutoCmpltListLen =
                        sal::static_int_cast< sal_uInt32 >(nVal);
                }
                break; // "Completion/MaxListLen",
                case  36: rSwFlags.bAutoCmpltCollectWords = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Completion/CollectWords",
                case  37: rSwFlags.bAutoCmpltEndless = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Completion/EndlessList",
                case  38: rSwFlags.bAutoCmpltAppendBlank = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Completion/AppendBlank",
                case  39: rSwFlags.bAutoCmpltShowAsTip = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Completion/ShowAsTip",
                case  40:
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.nAutoCmpltExpandKey =
                        sal::static_int_cast< sal_uInt16 >(nVal);
                }
                break; // "Completion/AcceptKey"
                case 41 :rSwFlags.bAutoCmpltKeepList = *o3tl::doAccess<bool>(pValues[nProp]); break;//"Completion/KeepList"
                case 42 :
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.cByInputBullet =
                        sal::static_int_cast< sal_Unicode >(nVal);
                }
                break;// "Format/ByInput/ApplyNumbering/SpecialCharacter/Char",
                case 43 :
                {
                    OUString sTemp; pValues[nProp] >>= sTemp;
                    rSwFlags.aByInputBulletFont.SetFamilyName(sTemp);
                }
                break;// "Format/ByInput/ApplyNumbering/SpecialCharacter/Font",
                case 44 :
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.aByInputBulletFont.SetFamily(FontFamily(nVal));
                }
                break;// "Format/ByInput/ApplyNumbering/SpecialCharacter/FontFamily",
                case 45 :
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.aByInputBulletFont.SetCharSet(rtl_TextEncoding(nVal));
                }
                break;// "Format/ByInput/ApplyNumbering/SpecialCharacter/FontCharset",
                case 46 :
                {
                    sal_Int32 nVal = 0; pValues[nProp] >>= nVal;
                    rSwFlags.aByInputBulletFont.SetPitch(FontPitch(nVal));
                }
                break;// "Format/ByInput/ApplyNumbering/SpecialCharacter/FontPitch",
                case   47: rSwFlags.bSetDOIAttr = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/Option/SetDOIAttribute",
                case 48 : rSwFlags.bSetNumRuleAfterSpace = *o3tl::doAccess<bool>(pValues[nProp]); break; // "Format/ByInput/ApplyNumberingAfterSpace",
            }
        }
    }
}

SvxSwAutoCorrCfg::SvxSwAutoCorrCfg(SvxAutoCorrCfg& rPar) :
    utl::ConfigItem(u"Office.Writer/AutoFunction"_ustr),
    rParent(rPar)
{
}

SvxSwAutoCorrCfg::~SvxSwAutoCorrCfg()
{
}

void SvxSwAutoCorrCfg::ImplCommit()
{
    SvxSwAutoFormatFlags& rSwFlags = rParent.pAutoCorrect->GetSwFlags();
    PutProperties(
        GetPropertyNames(),
        {css::uno::Any(rParent.bFileRel), // "Text/FileLinks"
         css::uno::Any(rParent.bNetRel), // "Text/InternetLinks"
         css::uno::Any(rParent.bAutoTextPreview), // "Text/ShowPreview"
         css::uno::Any(rParent.bAutoTextTip), // "Text/ShowToolTip"
         css::uno::Any(rParent.bSearchInAllCategories),
            // "Text/SearchInAllCategories"
         css::uno::Any(rSwFlags.bAutoCorrect),
            // "Format/Option/UseReplacementTable"
         css::uno::Any(rSwFlags.bCapitalStartSentence),
            // "Format/Option/TwoCapitalsAtStart"
         css::uno::Any(rSwFlags.bCapitalStartWord),
            // "Format/Option/CapitalAtStartSentence"
         css::uno::Any(rSwFlags.bChgWeightUnderl),
            // "Format/Option/ChangeUnderlineWeight"
         css::uno::Any(rSwFlags.bSetINetAttr),
            // "Format/Option/SetInetAttribute"
         css::uno::Any(rSwFlags.bChgOrdinalNumber),
            // "Format/Option/ChangeOrdinalNumber"
         css::uno::Any(rSwFlags.bAddNonBrkSpace),
            // "Format/Option/AddNonBreakingSpace"
         css::uno::Any(true),
            // "Format/Option/ChangeDash"; it doesn't exist here - the common
            // flags are used for that -> LM
         css::uno::Any(rSwFlags.bDelEmptyNode),
            // "Format/Option/DelEmptyParagraphs"
         css::uno::Any(rSwFlags.bChgUserColl),
            // "Format/Option/ReplaceUserStyle"
         css::uno::Any(rSwFlags.bChgEnumNum),
            // "Format/Option/ChangeToBullets/Enable"
         css::uno::Any(sal_Int32(rSwFlags.cBullet)),
            // "Format/Option/ChangeToBullets/SpecialCharacter/Char"
         css::uno::Any(rSwFlags.aBulletFont.GetFamilyName()),
            // "Format/Option/ChangeToBullets/SpecialCharacter/Font"
         css::uno::Any(sal_Int32(rSwFlags.aBulletFont.GetFamilyTypeMaybeAskConfig())),
            // "Format/Option/ChangeToBullets/SpecialCharacter/FontFamily"
         css::uno::Any(sal_Int32(rSwFlags.aBulletFont.GetCharSet())),
            // "Format/Option/ChangeToBullets/SpecialCharacter/FontCharset"
         css::uno::Any(sal_Int32(rSwFlags.aBulletFont.GetPitchMaybeAskConfig())),
            // "Format/Option/ChangeToBullets/SpecialCharacter/FontPitch"
         css::uno::Any(rSwFlags.bRightMargin),
            // "Format/Option/CombineParagraphs"
         css::uno::Any(sal_Int32(rSwFlags.nRightMargin)),
            // "Format/Option/CombineValue"
         css::uno::Any(rSwFlags.bAFormatDelSpacesAtSttEnd),
            // "Format/Option/DelSpacesAtStartEnd"
         css::uno::Any(rSwFlags.bAFormatDelSpacesBetweenLines),
            // "Format/Option/DelSpacesBetween"
         css::uno::Any(rParent.bAutoFmtByInput), // "Format/ByInput/Enable"
         css::uno::Any(rSwFlags.bChgToEnEmDash), // "Format/ByInput/ChangeDash"
         css::uno::Any(rSwFlags.bSetNumRule),
            // "Format/ByInput/ApplyNumbering/Enable"
         css::uno::Any(rSwFlags.bSetBorder), // "Format/ByInput/ChangeToBorders"
         css::uno::Any(rSwFlags.bCreateTable), // "Format/ByInput/ChangeToTable"
         css::uno::Any(rSwFlags.bReplaceStyles),
            // "Format/ByInput/ReplaceStyle"
         css::uno::Any(rSwFlags.bAFormatByInpDelSpacesAtSttEnd),
            // "Format/ByInput/DelSpacesAtStartEnd"
         css::uno::Any(rSwFlags.bAFormatByInpDelSpacesBetweenLines),
            // "Format/ByInput/DelSpacesBetween"
         css::uno::Any(rSwFlags.bAutoCompleteWords), // "Completion/Enable"
         css::uno::Any(sal_Int32(rSwFlags.nAutoCmpltWordLen)),
            // "Completion/MinWordLen"
         css::uno::Any(sal_Int32(rSwFlags.nAutoCmpltListLen)),
            // "Completion/MaxListLen"
         css::uno::Any(rSwFlags.bAutoCmpltCollectWords),
            // "Completion/CollectWords"
         css::uno::Any(rSwFlags.bAutoCmpltEndless), // "Completion/EndlessList"
         css::uno::Any(rSwFlags.bAutoCmpltAppendBlank),
            // "Completion/AppendBlank"
         css::uno::Any(rSwFlags.bAutoCmpltShowAsTip), // "Completion/ShowAsTip"
         css::uno::Any(sal_Int32(rSwFlags.nAutoCmpltExpandKey)),
            // "Completion/AcceptKey"
         css::uno::Any(rSwFlags.bAutoCmpltKeepList), // "Completion/KeepList"
         css::uno::Any(sal_Int32(rSwFlags.cByInputBullet)),
            // "Format/ByInput/ApplyNumbering/SpecialCharacter/Char"
         css::uno::Any(rSwFlags.aByInputBulletFont.GetFamilyName()),
            // "Format/ByInput/ApplyNumbering/SpecialCharacter/Font"
         css::uno::Any(sal_Int32(rSwFlags.aByInputBulletFont.GetFamilyTypeMaybeAskConfig())),
            // "Format/ByInput/ApplyNumbering/SpecialCharacter/FontFamily"
         css::uno::Any(sal_Int32(rSwFlags.aByInputBulletFont.GetCharSet())),
            // "Format/ByInput/ApplyNumbering/SpecialCharacter/FontCharset"
         css::uno::Any(sal_Int32(rSwFlags.aByInputBulletFont.GetPitchMaybeAskConfig())),
            // "Format/ByInput/ApplyNumbering/SpecialCharacter/FontPitch"
         css::uno::Any(rSwFlags.bSetDOIAttr),
         css::uno::Any(rSwFlags.bSetNumRuleAfterSpace), // "Format/ByInput/ApplyNumberingAfterSpace"
        });
            // "Format/Option/SetDOIAttribute"
}

void SvxSwAutoCorrCfg::Notify( const Sequence<OUString>& /* aPropertyNames */ )
{
    Load(false);
}

SvxAutoCorrCfg& SvxAutoCorrCfg::Get()
{
    static SvxAutoCorrCfg theSvxAutoCorrCfg;
    return theSvxAutoCorrCfg;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
