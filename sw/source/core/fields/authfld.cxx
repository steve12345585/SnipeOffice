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

#include <sal/config.h>

#include <memory>

#include <libxml/xmlwriter.h>

#include <comphelper/processfactory.hxx>
#include <comphelper/string.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <o3tl/any.hxx>
#include <o3tl/string_view.hxx>
#include <officecfg/Office/Common.hxx>
#include <osl/diagnose.h>
#include <tools/urlobj.hxx>
#include <swtypes.hxx>
#include <strings.hrc>
#include <authfld.hxx>
#include <expfld.hxx>
#include <pam.hxx>
#include <cntfrm.hxx>
#include <rootfrm.hxx>
#include <tox.hxx>
#include <txmsrt.hxx>
#include <fmtfld.hxx>
#include <txtfld.hxx>
#include <ndtxt.hxx>
#include <doc.hxx>
#include <IDocumentFieldsAccess.hxx>
#include <IDocumentLayoutAccess.hxx>
#include <unofldmid.h>
#include <unoprnms.hxx>
#include <docsh.hxx>

#include <com/sun/star/beans/PropertyValues.hpp>
#include <com/sun/star/uri/UriReferenceFactory.hpp>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;

SwAuthEntry::SwAuthEntry(const SwAuthEntry& rCopy)
    : SimpleReferenceObject()
{
    for(int i = 0; i < AUTH_FIELD_END; ++i)
        m_aAuthFields[i] = rCopy.m_aAuthFields[i];
}

bool SwAuthEntry::operator==(const SwAuthEntry& rComp) const
{
    for(int i = 0; i < AUTH_FIELD_END; ++i)
        if(m_aAuthFields[i] != rComp.m_aAuthFields[i])
            return false;
    return true;
}

SwAuthorityFieldType::SwAuthorityFieldType(SwDoc* pDoc)
    : SwFieldType( SwFieldIds::TableOfAuthorities ),
    m_pDoc(pDoc),
    m_cPrefix('['),
    m_cSuffix(']'),
    m_bIsSequence(false),
    m_bSortByDocument(true),
    m_eLanguage(::GetAppLanguage())
{
}

SwAuthorityFieldType::~SwAuthorityFieldType()
{
}

std::unique_ptr<SwFieldType> SwAuthorityFieldType::Copy()  const
{
    return std::make_unique<SwAuthorityFieldType>(m_pDoc);
}

void SwAuthorityFieldType::RemoveField(const SwAuthEntry* pEntry)
{
    for(SwAuthDataArr::size_type j = 0; j < m_DataArr.size(); ++j)
    {
        if(m_DataArr[j].get() == pEntry)
        {
            if (m_DataArr[j]->m_nCount <= 1)
            {
                m_DataArr.erase(m_DataArr.begin() + j);
                //re-generate positions of the fields
                DelSequenceArray();
            }
            return;
        }
    }
    assert(false && "SwAuthorityFieldType::RemoveField: pEntry was not added previously");
}

SwAuthEntry* SwAuthorityFieldType::AddField(std::u16string_view rFieldContents)
{
    rtl::Reference<SwAuthEntry> pEntry(new SwAuthEntry);
    sal_Int32 nIdx{ 0 };
    for( sal_Int32 i = 0; i < AUTH_FIELD_END; ++i )
        pEntry->SetAuthorField( static_cast<ToxAuthorityField>(i),
                        OUString(o3tl::getToken(rFieldContents, 0, TOX_STYLE_DELIMITER, nIdx )));

    for (const auto &rpTemp : m_DataArr)
    {
        if (*rpTemp == *pEntry)
        {
            return rpTemp.get();
        }
    }

    //if it is a new Entry - insert
    m_DataArr.push_back(std::move(pEntry));
    //re-generate positions of the fields
    DelSequenceArray();
    return m_DataArr.back().get();
}

void SwAuthorityFieldType::GetAllEntryIdentifiers(
    std::vector<OUString>& rToFill )const
{
    for (const auto & rpTemp : m_DataArr)
    {
        rToFill.push_back(rpTemp->GetAuthorField(AUTH_FIELD_IDENTIFIER));
    }
}

SwAuthEntry* SwAuthorityFieldType::GetEntryByIdentifier(
                                std::u16string_view rIdentifier)const
{
    for (const auto &rpTemp : m_DataArr)
    {
        if (rIdentifier == rpTemp->GetAuthorField(AUTH_FIELD_IDENTIFIER))
        {
            return rpTemp.get();
        }
    }
    return nullptr;
}

bool SwAuthorityFieldType::ChangeEntryContent(const SwAuthEntry* pNewEntry)
{
    for (auto &rpTemp : m_DataArr)
    {
        if (rpTemp->GetAuthorField(AUTH_FIELD_IDENTIFIER) ==
                    pNewEntry->GetAuthorField(AUTH_FIELD_IDENTIFIER))
        {
            for(int i = 0; i < AUTH_FIELD_END; ++i)
            {
                rpTemp->SetAuthorField(static_cast<ToxAuthorityField>(i),
                    pNewEntry->GetAuthorField(static_cast<ToxAuthorityField>(i)));
            }
            return true;
        }
    }
    return false;
}

/// appends a new entry (if new) and returns the copied entry
SwAuthEntry* SwAuthorityFieldType::AppendField( const SwAuthEntry& rInsert )
{
    for( SwAuthDataArr::size_type nRet = 0; nRet < m_DataArr.size(); ++nRet )
    {
        if( *m_DataArr[ nRet ] == rInsert )
            return m_DataArr[ nRet ].get();
    }

    //if it is a new Entry - insert
    m_DataArr.push_back(new SwAuthEntry(rInsert));
    return m_DataArr.back().get();
}

std::unique_ptr<SwTOXInternational> SwAuthorityFieldType::CreateTOXInternational() const
{
    return std::make_unique<SwTOXInternational>(m_eLanguage, SwTOIOptions::NONE, m_sSortAlgorithm);
}

sal_uInt16 SwAuthorityFieldType::GetSequencePos(const SwAuthEntry* pAuthEntry,
        SwRootFrame const*const pLayout)
{
    //find the field in a sorted array of handles,
    if(!m_SequArr.empty() && m_SequArr.size() != m_DataArr.size())
        DelSequenceArray();
    if(m_SequArr.empty())
    {
        IDocumentRedlineAccess const& rIDRA(m_pDoc->getIDocumentRedlineAccess());
        std::unique_ptr<SwTOXInternational> pIntl = CreateTOXInternational();
        // sw_redlinehide: need 2 arrays because the sorting may be different,
        // if multiple fields refer to the same entry and first one is deleted
        std::vector<std::unique_ptr<SwTOXSortTabBase>> aSortArr;
        std::vector<std::unique_ptr<SwTOXSortTabBase>> aSortArrRLHidden;
        std::vector<SwFormatField*> vFields;
        GatherFields(vFields);
        for(SwFormatField* pFormatField : vFields)
        {
            const SwTextField* pTextField = pFormatField->GetTextField();
            if(!pTextField || !pTextField->GetpTextNode())
            {
                continue;
            }
            const SwTextNode& rFieldTextNode = pTextField->GetTextNode();
            SwPosition aFieldPos(rFieldTextNode);
            SwDoc& rDoc = const_cast<SwDoc&>(rFieldTextNode.GetDoc());
            SwContentFrame *pFrame = rFieldTextNode.getLayoutFrame( rDoc.getIDocumentLayoutAccess().GetCurrentLayout() );
            const SwTextNode* pTextNode = nullptr;
            if(pFrame && !pFrame->IsInDocBody())
                pTextNode = GetBodyTextNode( rDoc, aFieldPos, *pFrame );
            //if no text node could be found or the field is in the document
            //body the directly available text node will be used
            if(!pTextNode)
                pTextNode = &rFieldTextNode;
            if (pTextNode->GetText().isEmpty()
                || !pTextNode->getLayoutFrame(rDoc.getIDocumentLayoutAccess().GetCurrentLayout())
                || !pTextNode->GetNodes().IsDocNodes())
            {
                continue;
            }
            auto const InsertImpl = [&pIntl, pTextNode, pFormatField]
                (std::vector<std::unique_ptr<SwTOXSortTabBase>> & rSortArr)
            {
                std::unique_ptr<SwTOXAuthority> pNew(
                    new SwTOXAuthority(*pTextNode, *pFormatField, *pIntl));

                for (size_t i = 0; i < rSortArr.size(); ++i)
                {
                    SwTOXSortTabBase* pOld = rSortArr[i].get();
                    if (pOld->equivalent(*pNew))
                    {
                        //only the first occurrence in the document
                        //has to be in the array
                        if (pOld->sort_lt(*pNew))
                            pNew.reset();
                        else // remove the old content
                            rSortArr.erase(rSortArr.begin() + i);
                        break;
                    }
                }
                //if it still exists - insert at the correct position
                if (pNew)
                {
                    size_t j {0};

                    while (j < rSortArr.size())
                    {
                        SwTOXSortTabBase* pOld = rSortArr[j].get();
                        if (pNew->sort_lt(*pOld))
                            break;
                        ++j;
                    }
                    rSortArr.insert(rSortArr.begin() + j, std::move(pNew));
                }
            };
            InsertImpl(aSortArr);
            if (!sw::IsFieldDeletedInModel(rIDRA, *pTextField))
            {
                InsertImpl(aSortArrRLHidden);
            }
        }

        for(auto & pBase : aSortArr)
        {
            SwFormatField& rFormatField = static_cast<SwTOXAuthority&>(*pBase).GetFieldFormat();
            SwAuthorityField* pAField = static_cast<SwAuthorityField*>(rFormatField.GetField());
            m_SequArr.push_back(pAField->GetAuthEntry());
        }
        for (auto & pBase : aSortArrRLHidden)
        {
            SwFormatField& rFormatField = static_cast<SwTOXAuthority&>(*pBase).GetFieldFormat();
            SwAuthorityField* pAField = static_cast<SwAuthorityField*>(rFormatField.GetField());
            m_SequArrRLHidden.push_back(pAField->GetAuthEntry());
        }
    }
    //find nHandle
    auto const& rSequArr(pLayout && pLayout->IsHideRedlines() ? m_SequArrRLHidden : m_SequArr);
    for (std::vector<sal_IntPtr>::size_type i = 0; i < rSequArr.size(); ++i)
    {
        if (rSequArr[i] == pAuthEntry)
        {
            return i + 1;
        }
    }
    return 0;
}

void SwAuthorityFieldType::QueryValue( Any& rVal, sal_uInt16 nWhichId ) const
{
    switch( nWhichId )
    {
    case FIELD_PROP_PAR1:
    case FIELD_PROP_PAR2:
        {
            OUString sVal;
            sal_Unicode uRet = FIELD_PROP_PAR1 == nWhichId ? m_cPrefix : m_cSuffix;
            if(uRet)
                sVal = OUString(uRet);
            rVal <<= sVal;
        }
        break;
    case FIELD_PROP_PAR3:
        rVal <<= GetSortAlgorithm();
        break;

    case FIELD_PROP_BOOL1:
        rVal <<= m_bIsSequence;
        break;

    case FIELD_PROP_BOOL2:
        rVal <<= m_bSortByDocument;
        break;

    case FIELD_PROP_LOCALE:
        rVal <<= LanguageTag(GetLanguage()).getLocale();
        break;

    case FIELD_PROP_PROP_SEQ:
        {
            Sequence<PropertyValues> aRet(m_SortKeyArr.size());
            PropertyValues* pValues = aRet.getArray();
            for(SortKeyArr::size_type i = 0; i < m_SortKeyArr.size(); ++i)
            {
                const SwTOXSortKey* pKey = &m_SortKeyArr[i];
                pValues[i].realloc(2);
                PropertyValue* pValue = pValues[i].getArray();
                pValue[0].Name = UNO_NAME_SORT_KEY;
                pValue[0].Value <<= sal_Int16(pKey->eField);
                pValue[1].Name = UNO_NAME_IS_SORT_ASCENDING;
                pValue[1].Value <<= pKey->bSortAscending;
            }
            rVal <<= aRet;
        }
        break;
    default:
        assert(false);
    }
}

void SwAuthorityFieldType::PutValue( const Any& rAny, sal_uInt16 nWhichId )
{
    switch( nWhichId )
    {
    case FIELD_PROP_PAR1:
    case FIELD_PROP_PAR2:
    {
        OUString sTmp;
        rAny >>= sTmp;
        const sal_Unicode uSet = !sTmp.isEmpty() ? sTmp[0] : 0;
        if( FIELD_PROP_PAR1 == nWhichId )
            m_cPrefix = uSet;
        else
            m_cSuffix = uSet;
    }
    break;
    case FIELD_PROP_PAR3:
    {
        OUString sTmp;
        rAny >>= sTmp;
        SetSortAlgorithm(sTmp);
        break;
    }
    case FIELD_PROP_BOOL1:
        m_bIsSequence = *o3tl::doAccess<bool>(rAny);
        break;
    case FIELD_PROP_BOOL2:
        m_bSortByDocument = *o3tl::doAccess<bool>(rAny);
        break;

    case FIELD_PROP_LOCALE:
        {
            css::lang::Locale aLocale;
            if( rAny >>= aLocale )
                SetLanguage( LanguageTag::convertToLanguageType( aLocale ));
        }
        break;

    case FIELD_PROP_PROP_SEQ:
        {
            Sequence<PropertyValues> aSeq;
            if( rAny >>= aSeq )
            {
                m_SortKeyArr.clear();
                const PropertyValues* pValues = aSeq.getConstArray();
                //TODO: Limiting to the first SAL_MAX_UINT16 elements of aSeq so that size of
                // m_SortKeyArr remains in range of sal_uInt16, as GetSortKeyCount and GetSortKey
                // still expect m_SortKeyArr to be indexed by sal_uInt16:
                auto nSize = std::min<sal_Int32>(aSeq.getLength(), SAL_MAX_UINT16);
                for(sal_Int32 i = 0; i < nSize; i++)
                {
                    SwTOXSortKey aSortKey;
                    for(const PropertyValue& rValue : pValues[i])
                    {
                        if(rValue.Name == UNO_NAME_SORT_KEY)
                        {
                            sal_Int16 nVal = -1; rValue.Value >>= nVal;
                            if(nVal >= 0 && nVal < AUTH_FIELD_END)
                                aSortKey.eField = static_cast<ToxAuthorityField>(nVal);
                        }
                        else if(rValue.Name == UNO_NAME_IS_SORT_ASCENDING)
                        {
                            aSortKey.bSortAscending = *o3tl::doAccess<bool>(rValue.Value);
                        }
                    }
                    m_SortKeyArr.push_back(aSortKey);
                }
            }
        }
        break;
    default:
        assert(false);
    }
}

void SwAuthorityFieldType::SwClientNotify(const SwModify&, const SfxHint& rHint)
{
    //re-generate positions of the fields
    DelSequenceArray();
    CallSwClientNotify(rHint);
}

sal_uInt16 SwAuthorityFieldType::GetSortKeyCount() const
{
    return m_SortKeyArr.size();
}

const SwTOXSortKey*  SwAuthorityFieldType::GetSortKey(sal_uInt16 nIdx) const
{
    if(m_SortKeyArr.size() > nIdx)
        return &m_SortKeyArr[nIdx];
    OSL_FAIL("Sort key not found");
    return nullptr;
}

void SwAuthorityFieldType::SetSortKeys(sal_uInt16 nKeyCount, SwTOXSortKey  const aKeys[])
{
    m_SortKeyArr.clear();
    for(sal_uInt16 i = 0; i < nKeyCount; i++)
        if(aKeys[i].eField < AUTH_FIELD_END)
            m_SortKeyArr.push_back(aKeys[i]);
}

SwAuthorityField::SwAuthorityField( SwAuthorityFieldType* pInitType,
                                    std::u16string_view rFieldContents )
    : SwField(pInitType)
    , m_nTempSequencePos( -1 )
    , m_nTempSequencePosRLHidden( -1 )
{
    m_xAuthEntry = pInitType->AddField( rFieldContents );
}

SwAuthorityField::SwAuthorityField( SwAuthorityFieldType* pInitType,
                                    SwAuthEntry* pAuthEntry )
    : SwField( pInitType )
    , m_xAuthEntry( pAuthEntry )
    , m_nTempSequencePos( -1 )
    , m_nTempSequencePosRLHidden( -1 )
{
}

SwAuthorityField::~SwAuthorityField()
{
    static_cast<SwAuthorityFieldType* >(GetTyp())->RemoveField(m_xAuthEntry.get());
}

OUString SwAuthorityField::ExpandImpl(SwRootFrame const*const pLayout) const
{
    return ConditionalExpandAuthIdentifier(pLayout);
}

OUString SwAuthorityField::ConditionalExpandAuthIdentifier(
        SwRootFrame const*const pLayout) const
{
    SwAuthorityFieldType* pAuthType = static_cast<SwAuthorityFieldType*>(GetTyp());
    OUString sRet;
    if(pAuthType->GetPrefix())
        sRet = OUString(pAuthType->GetPrefix());

    if( pAuthType->IsSequence() )
    {
        sal_IntPtr & rnTempSequencePos(pLayout && pLayout->IsHideRedlines()
                ? m_nTempSequencePosRLHidden : m_nTempSequencePos);
        if(!pAuthType->GetDoc()->getIDocumentFieldsAccess().IsExpFieldsLocked())
            rnTempSequencePos = pAuthType->GetSequencePos(m_xAuthEntry.get(), pLayout);
        if (0 <= rnTempSequencePos)
            sRet += OUString::number(rnTempSequencePos);
    }
    else
    {
        //TODO: Expand to: identifier, number sequence, ...
        if(m_xAuthEntry)
        {
            OUString sIdentifier(m_xAuthEntry->GetAuthorField(AUTH_FIELD_IDENTIFIER));
            // tdf#107784 Use title if it's a ooxml citation
            if (o3tl::starts_with(o3tl::trim(sIdentifier), u"CITATION"))
                return m_xAuthEntry->GetAuthorField(AUTH_FIELD_TITLE);
            else
                sRet += sIdentifier;
        }
    }
    if(pAuthType->GetSuffix())
        sRet += OUStringChar(pAuthType->GetSuffix());
    return sRet;
}

OUString SwAuthorityField::ExpandCitation(ToxAuthorityField eField,
        SwRootFrame const*const pLayout) const
{
    SwAuthorityFieldType* pAuthType = static_cast<SwAuthorityFieldType*>(GetTyp());
    OUString sRet;

    if( pAuthType->IsSequence() )
    {
        sal_IntPtr & rnTempSequencePos(pLayout && pLayout->IsHideRedlines()
                ? m_nTempSequencePosRLHidden : m_nTempSequencePos);
        if(!pAuthType->GetDoc()->getIDocumentFieldsAccess().IsExpFieldsLocked())
            rnTempSequencePos = pAuthType->GetSequencePos(m_xAuthEntry.get(), pLayout);
        if (0 <= rnTempSequencePos)
            sRet += OUString::number(rnTempSequencePos);
    }
    else
    {
        //TODO: Expand to: identifier, number sequence, ...
        if(m_xAuthEntry)
            sRet += m_xAuthEntry->GetAuthorField(eField);
    }
    return sRet;
}

std::unique_ptr<SwField> SwAuthorityField::Copy() const
{
    SwAuthorityFieldType* pAuthType = static_cast<SwAuthorityFieldType*>(GetTyp());
    return std::make_unique<SwAuthorityField>(pAuthType, m_xAuthEntry.get());
}

const OUString & SwAuthorityField::GetFieldText(ToxAuthorityField eField) const
{
    return m_xAuthEntry->GetAuthorField( eField );
}

void SwAuthorityField::SetPar1(const OUString& rStr)
{
    SwAuthorityFieldType* pInitType = static_cast<SwAuthorityFieldType* >(GetTyp());
    pInitType->RemoveField(m_xAuthEntry.get());
    m_xAuthEntry = pInitType->AddField(rStr);
}

OUString SwAuthorityField::GetDescription() const
{
    return SwResId(STR_AUTHORITY_ENTRY);
}

OUString SwAuthorityField::GetAuthority(const SwRootFrame* pLayout, const SwForm* pTOX) const
{
    OUString aText;

    std::unique_ptr<SwForm> pDefaultTOX;
    if (!pTOX)
    {
        pDefaultTOX = std::make_unique<SwForm>(TOX_AUTHORITIES);
        pTOX = pDefaultTOX.get();
    }

    SwAuthorityFieldType* pFieldType = static_cast<SwAuthorityFieldType*>(GetTyp());
    std::unique_ptr<SwTOXInternational> pIntl(pFieldType->CreateTOXInternational());

    // This is based on SwTOXAuthority::GetLevel()
    OUString sText = GetFieldText(AUTH_FIELD_AUTHORITY_TYPE);
    ToxAuthorityType nAuthorityKind = AUTH_TYPE_ARTICLE;
    if (pIntl->IsNumeric(sText))
        nAuthorityKind = static_cast<ToxAuthorityType>(sText.toUInt32());
    if (nAuthorityKind > AUTH_TYPE_END)
        nAuthorityKind = AUTH_TYPE_ARTICLE;

    // Must be incremented by 1, since the pattern 0 is for the heading
    const SwFormTokens& aPattern = pTOX->GetPattern(static_cast<int>(nAuthorityKind) + 1);

    for (const auto& rToken : aPattern)
    {
        switch (rToken.eTokenType)
        {
            case TOKEN_TAB_STOP:
            {
                aText += "\t";
                break;
            }
            case TOKEN_TEXT:
            {
                aText += rToken.sText;
                break;
            }
            case TOKEN_AUTHORITY:
            {
                ToxAuthorityField eField = static_cast<ToxAuthorityField>(rToken.nAuthorityField);

                if (AUTH_FIELD_IDENTIFIER == eField)
                {
                    // Why isn't there a way to get the identifier without parentheses???
                    OUString sTmp = ExpandField(true, pLayout);
                    if (sal_Unicode cPref = pFieldType->GetPrefix(); cPref && cPref != ' ')
                        sTmp = sTmp.copy(1);
                    if (sal_Unicode cSuff = pFieldType->GetSuffix(); cSuff && cSuff != ' ')
                        sTmp = sTmp.copy(0, sTmp.getLength() - 1);
                    aText += sTmp;
                }
                else if (AUTH_FIELD_AUTHORITY_TYPE == eField)
                {
                    aText += SwAuthorityFieldType::GetAuthTypeName(nAuthorityKind);
                }
                else if (AUTH_FIELD_URL == eField)
                {
                    aText += GetRelativeURI();
                }
                else
                {
                    aText += GetFieldText(eField);
                }
                break;
            }
            default:
                break;
        }
    }

    return aText;
}

SwAuthorityField::TargetType SwAuthorityField::GetTargetType() const
{
    return SwAuthorityField::TargetType(GetAuthEntry()->GetAuthorField(AUTH_FIELD_TARGET_TYPE).toInt32());
}

OUString SwAuthorityField::GetAbsoluteURL() const
{
    const OUString& rURL = GetAuthEntry()->GetAuthorField(
        GetTargetType() == SwAuthorityField::TargetType::UseDisplayURL
            ? AUTH_FIELD_URL : AUTH_FIELD_TARGET_URL);
    SwDoc* pDoc = static_cast<SwAuthorityFieldType*>(GetTyp())->GetDoc();
    SwDocShell* pDocShell = pDoc->GetDocShell();
    if (!pDocShell)
        return OUString();
    OUString aBasePath = pDocShell->getDocumentBaseURL();
    return INetURLObject::GetAbsURL(aBasePath, rURL, INetURLObject::EncodeMechanism::WasEncoded,
                                    INetURLObject::DecodeMechanism::WithCharset);
}

OUString SwAuthorityField::GetRelativeURI() const
{
    OUString sTmp = GetFieldText(AUTH_FIELD_URL);

    SwDoc* pDoc = static_cast<SwAuthorityFieldType*>(GetTyp())->GetDoc();
    SwDocShell* pDocShell = pDoc->GetDocShell();
    if (!pDocShell)
        return OUString();
    const OUString aBaseURL = pDocShell->getDocumentBaseURL();
    std::u16string_view aBaseURIScheme;
    sal_Int32 nSep = aBaseURL.indexOf(':');
    if (nSep != -1)
    {
        aBaseURIScheme = aBaseURL.subView(0, nSep);
    }

    uno::Reference<uri::XUriReferenceFactory> xUriReferenceFactory
        = uri::UriReferenceFactory::create(comphelper::getProcessComponentContext());
    uno::Reference<uri::XUriReference> xUriRef;
    try
    {
        xUriRef = xUriReferenceFactory->parse(sTmp);
    }
    catch (const uno::Exception& rException)
    {
        SAL_WARN("sw.core",
                 "SwTOXAuthority::GetSourceURL: failed to parse url: " << rException.Message);
    }
    if (xUriRef.is() && xUriRef->getFragment().startsWith("page="))
    {
        xUriRef->clearFragment();
        sTmp = xUriRef->getUriReference();
    }

    // convert to relative
    bool bSaveRelFSys = officecfg::Office::Common::Save::URL::FileSystem::get();
    if (xUriRef.is() && bSaveRelFSys && xUriRef->getScheme() == aBaseURIScheme)
    {
        sTmp = INetURLObject::GetRelURL(aBaseURL, sTmp);
    }
    return sTmp;
}

void SwAuthorityField::dumpAsXml(xmlTextWriterPtr pWriter) const
{
    (void)xmlTextWriterStartElement(pWriter, BAD_CAST("SwAuthorityField"));
    SwField::dumpAsXml(pWriter);

    (void)xmlTextWriterStartElement(pWriter, BAD_CAST("m_xAuthEntry"));
    (void)xmlTextWriterWriteFormatAttribute(pWriter, BAD_CAST("ptr"), "%p", m_xAuthEntry.get());
    if (m_xAuthEntry.is())
    {
        m_xAuthEntry->dumpAsXml(pWriter);
    }
    (void)xmlTextWriterEndElement(pWriter);
    (void)xmlTextWriterStartElement(pWriter, BAD_CAST("m_nTempSequencePos"));
    (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("value"),
                                BAD_CAST(OString::number(m_nTempSequencePos).getStr()));
    (void)xmlTextWriterEndElement(pWriter);
    (void)xmlTextWriterStartElement(pWriter, BAD_CAST("m_nTempSequencePosRLHidden"));
    (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("value"),
                                BAD_CAST(OString::number(m_nTempSequencePosRLHidden).getStr()));
    (void)xmlTextWriterEndElement(pWriter);

    (void)xmlTextWriterEndElement(pWriter);
}

constexpr OUString aFieldNames[]
{
    u"Identifier"_ustr,
    u"BibiliographicType"_ustr,
    u"Address"_ustr,
    u"Annote"_ustr,
    u"Author"_ustr,
    u"Booktitle"_ustr,
    u"Chapter"_ustr,
    u"Edition"_ustr,
    u"Editor"_ustr,
    u"Howpublished"_ustr,
    u"Institution"_ustr,
    u"Journal"_ustr,
    u"Month"_ustr,
    u"Note"_ustr,
    u"Number"_ustr,
    u"Organizations"_ustr,
    u"Pages"_ustr,
    u"Publisher"_ustr,
    u"School"_ustr,
    u"Series"_ustr,
    u"Title"_ustr,
    u"Report_Type"_ustr,
    u"Volume"_ustr,
    u"Year"_ustr,
    u"URL"_ustr,
    u"Custom1"_ustr,
    u"Custom2"_ustr,
    u"Custom3"_ustr,
    u"Custom4"_ustr,
    u"Custom5"_ustr,
    u"ISBN"_ustr,
    u"LocalURL"_ustr,
    u"TargetType"_ustr,
    u"TargetURL"_ustr,
};

void SwAuthEntry::dumpAsXml(xmlTextWriterPtr pWriter) const
{
    (void)xmlTextWriterStartElement(pWriter, BAD_CAST("SwAuthEntry"));

    for (int i = 0; i < AUTH_FIELD_END; ++i)
    {
        (void)xmlTextWriterStartElement(pWriter, BAD_CAST("m_aAuthField"));
        (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("key"), BAD_CAST(aFieldNames[i].toUtf8().getStr()));
        (void)xmlTextWriterWriteAttribute(pWriter, BAD_CAST("value"), BAD_CAST(m_aAuthFields[i].toUtf8().getStr()));
        (void)xmlTextWriterEndElement(pWriter);
    }

    (void)xmlTextWriterEndElement(pWriter);
}

bool    SwAuthorityField::QueryValue( Any& rAny, sal_uInt16 /*nWhichId*/ ) const
{
    if(!GetTyp())
        return false;
    if(!m_xAuthEntry)
        return false;
    Sequence <PropertyValue> aRet(AUTH_FIELD_END);
    PropertyValue* pValues = aRet.getArray();
    for(int i = 0; i < AUTH_FIELD_END; ++i)
    {
        pValues[i].Name = aFieldNames[i];
        const OUString& sField = m_xAuthEntry->GetAuthorField(static_cast<ToxAuthorityField>(i));
        if(i == AUTH_FIELD_AUTHORITY_TYPE)
            pValues[i].Value <<= sal_Int16(sField.toInt32());
        else
            pValues[i].Value <<= sField;
    }
    rAny <<= aRet;
    /* FIXME: it is weird that we always return false here */
    return false;
}

static sal_Int32 lcl_Find(std::u16string_view rFieldName)
{
    for(sal_Int32 i = 0; i < AUTH_FIELD_END; ++i)
        if(aFieldNames[i] == rFieldName)
            return i;
    return -1;
}

bool    SwAuthorityField::PutValue( const Any& rAny, sal_uInt16 /*nWhichId*/ )
{
    if(!GetTyp() || !m_xAuthEntry)
        return false;

    Sequence <PropertyValue> aParam;
    if(!(rAny >>= aParam))
        return false;

    OUStringBuffer sBuf(+(AUTH_FIELD_END - 1));
    comphelper::string::padToLength(sBuf, (AUTH_FIELD_END - 1), TOX_STYLE_DELIMITER);
    OUString sToSet(sBuf.makeStringAndClear());
    for (const PropertyValue& rParam : aParam)
    {
        const sal_Int32 nFound = lcl_Find(rParam.Name);
        if(nFound >= 0)
        {
            OUString sContent;
            if(AUTH_FIELD_AUTHORITY_TYPE == nFound)
            {
                sal_Int16 nVal = 0;
                rParam.Value >>= nVal;
                sContent = OUString::number(nVal);
            }
            else
                rParam.Value >>= sContent;
            sToSet = comphelper::string::setToken(sToSet, nFound, TOX_STYLE_DELIMITER, sContent);
        }
    }

    static_cast<SwAuthorityFieldType*>(GetTyp())->RemoveField(m_xAuthEntry.get());
    m_xAuthEntry = static_cast<SwAuthorityFieldType*>(GetTyp())->AddField(sToSet);

    /* FIXME: it is weird that we always return false here */
    return false;
}

SwFieldType* SwAuthorityField::ChgTyp( SwFieldType* pFieldTyp )
{
    SwAuthorityFieldType* pSrcTyp = static_cast<SwAuthorityFieldType*>(GetTyp()),
                        * pDstTyp = static_cast<SwAuthorityFieldType*>(pFieldTyp);
    if( pSrcTyp != pDstTyp )
    {
        const SwAuthEntry* pSrcEntry = m_xAuthEntry.get();
        m_xAuthEntry = pDstTyp->AppendField( *pSrcEntry );
        pSrcTyp->RemoveField( pSrcEntry );
        SwField::ChgTyp( pFieldTyp );
    }
    return pSrcTyp;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
