/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/classificationhelper.hxx>

#include <map>
#include <algorithm>
#include <iterator>

#include <com/sun/star/beans/XPropertyContainer.hpp>
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/xml/sax/Parser.hpp>
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>
#include <com/sun/star/xml/sax/SAXParseException.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>

#include <sal/log.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <sfx2/infobar.hxx>
#include <comphelper/processfactory.hxx>
#include <unotools/configmgr.hxx>
#include <unotools/pathoptions.hxx>
#include <unotools/ucbstreamhelper.hxx>
#include <unotools/streamwrap.hxx>
#include <cppuhelper/implbase.hxx>
#include <sfx2/strings.hrc>
#include <sfx2/sfxresid.hxx>
#include <sfx2/viewfrm.hxx>
#include <tools/datetime.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <unotools/datetime.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <vcl/weld.hxx>
#include <svl/fstathelper.hxx>

#include <o3tl/string_view.hxx>
#include <officecfg/Office/Common.hxx>

using namespace com::sun::star;

namespace
{

const OUString& PROP_BACNAME()
{
    static constexpr OUString sProp(u"BusinessAuthorizationCategory:Name"_ustr);
    return sProp;
}

const OUString& PROP_STARTVALIDITY()
{
    static constexpr OUString sProp(u"Authorization:StartValidity"_ustr);
    return sProp;
}

const OUString& PROP_NONE()
{
    static constexpr OUString sProp(u"None"_ustr);
    return sProp;
}

const OUString& PROP_IMPACTSCALE()
{
    static constexpr OUString sProp(u"Impact:Scale"_ustr);
    return sProp;
}

const OUString& PROP_IMPACTLEVEL()
{
    static constexpr OUString sProp(u"Impact:Level:Confidentiality"_ustr);
    return sProp;
}

const OUString& PROP_PREFIX_EXPORTCONTROL()
{
    static constexpr OUString sProp(u"urn:bails:ExportControl:"_ustr);
    return sProp;
}

const OUString& PROP_PREFIX_NATIONALSECURITY()
{
    static constexpr OUString sProp(u"urn:bails:NationalSecurity:"_ustr);
    return sProp;
}

/// Represents one category of a classification policy.
class SfxClassificationCategory
{
public:
    /// PROP_BACNAME() is stored separately for easier lookup.
    OUString m_aName;
    OUString m_aAbbreviatedName; //< An abbreviation to display instead of m_aName.
    OUString m_aIdentifier; //< The Identifier of this entry.
    size_t m_nConfidentiality; //< 0 is the lowest (least-sensitive).
    std::map<OUString, OUString> m_aLabels;
};

/// Parses a policy XML conforming to the TSCP BAF schema.
class SfxClassificationParser : public cppu::WeakImplHelper<xml::sax::XDocumentHandler>
{
public:
    std::vector<SfxClassificationCategory> m_aCategories;
    std::vector<OUString> m_aMarkings;
    std::vector<OUString> m_aIPParts;
    std::vector<OUString> m_aIPPartNumbers;

    OUString m_aPolicyAuthorityName;
    bool m_bInPolicyAuthorityName = false;
    OUString m_aPolicyName;
    bool m_bInPolicyName = false;
    OUString m_aProgramID;
    bool m_bInProgramID = false;
    OUString m_aScale;
    bool m_bInScale = false;
    OUString m_aConfidentalityValue;
    bool m_bInConfidentalityValue = false;
    OUString m_aIdentifier;
    bool m_bInIdentifier = false;
    OUString m_aValue;
    bool m_bInValue = false;

    /// Pointer to a value in m_aCategories, the currently parsed category.
    SfxClassificationCategory* m_pCategory = nullptr;

    SfxClassificationParser();

    void SAL_CALL startDocument() override;

    void SAL_CALL endDocument() override;

    void SAL_CALL startElement(const OUString& rName, const uno::Reference<xml::sax::XAttributeList>& xAttribs) override;

    void SAL_CALL endElement(const OUString& rName) override;

    void SAL_CALL characters(const OUString& rChars) override;

    void SAL_CALL ignorableWhitespace(const OUString& rWhitespaces) override;

    void SAL_CALL processingInstruction(const OUString& rTarget, const OUString& rData) override;

    void SAL_CALL setDocumentLocator(const uno::Reference<xml::sax::XLocator>& xLocator) override;
};

SfxClassificationParser::SfxClassificationParser() = default;

void SAL_CALL SfxClassificationParser::startDocument()
{
}

void SAL_CALL SfxClassificationParser::endDocument()
{
}

void SAL_CALL SfxClassificationParser::startElement(const OUString& rName, const uno::Reference<xml::sax::XAttributeList>& xAttribs)
{
    if (rName == "baf:PolicyAuthorityName")
    {
        m_aPolicyAuthorityName.clear();
        m_bInPolicyAuthorityName = true;
    }
    else if (rName == "baf:PolicyName")
    {
        m_aPolicyName.clear();
        m_bInPolicyName = true;
    }
    else if (rName == "baf:ProgramID")
    {
        m_aProgramID.clear();
        m_bInProgramID = true;
    }
    else if (rName == "baf:BusinessAuthorizationCategory")
    {
        const OUString aName = xAttribs->getValueByName(u"Name"_ustr);
        if (!m_pCategory && !aName.isEmpty())
        {
            OUString aIdentifier = xAttribs->getValueByName(u"Identifier"_ustr);

            // Create a new category and initialize it with the data that's true for all categories.
            m_aCategories.emplace_back();
            SfxClassificationCategory& rCategory = m_aCategories.back();

            rCategory.m_aName = aName;
            // Set the abbreviated name, if any, otherwise fallback on the full name.
            const OUString aAbbreviatedName = xAttribs->getValueByName(u"loextAbbreviatedName"_ustr);
            rCategory.m_aAbbreviatedName = !aAbbreviatedName.isEmpty() ? aAbbreviatedName : aName;
            rCategory.m_aIdentifier = aIdentifier;

            rCategory.m_aLabels[u"PolicyAuthority:Name"_ustr] = m_aPolicyAuthorityName;
            rCategory.m_aLabels[u"Policy:Name"_ustr] = m_aPolicyName;
            rCategory.m_aLabels[u"BusinessAuthorization:Identifier"_ustr] = m_aProgramID;
            rCategory.m_aLabels[u"BusinessAuthorizationCategory:Identifier"_ustr] = aIdentifier;

            // Also initialize defaults.
            rCategory.m_aLabels[u"PolicyAuthority:Identifier"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"PolicyAuthority:Country"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"Policy:Identifier"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"BusinessAuthorization:Name"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"BusinessAuthorization:Locator"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"BusinessAuthorizationCategory:Identifier:OID"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"BusinessAuthorizationCategory:Locator"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"BusinessAuthorization:Locator"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"MarkingPrecedence"_ustr] = PROP_NONE();
            rCategory.m_aLabels[u"Marking:general-summary"_ustr].clear();
            rCategory.m_aLabels[u"Marking:general-warning-statement"_ustr].clear();
            rCategory.m_aLabels[u"Marking:general-warning-statement:ext:2"_ustr].clear();
            rCategory.m_aLabels[u"Marking:general-warning-statement:ext:3"_ustr].clear();
            rCategory.m_aLabels[u"Marking:general-warning-statement:ext:4"_ustr].clear();
            rCategory.m_aLabels[u"Marking:general-distribution-statement"_ustr].clear();
            rCategory.m_aLabels[u"Marking:general-distribution-statement:ext:2"_ustr].clear();
            rCategory.m_aLabels[u"Marking:general-distribution-statement:ext:3"_ustr].clear();
            rCategory.m_aLabels[u"Marking:general-distribution-statement:ext:4"_ustr].clear();
            rCategory.m_aLabels[SfxClassificationHelper::PROP_DOCHEADER()].clear();
            rCategory.m_aLabels[SfxClassificationHelper::PROP_DOCFOOTER()].clear();
            rCategory.m_aLabels[SfxClassificationHelper::PROP_DOCWATERMARK()].clear();
            rCategory.m_aLabels[u"Marking:email-first-line-of-text"_ustr].clear();
            rCategory.m_aLabels[u"Marking:email-last-line-of-text"_ustr].clear();
            rCategory.m_aLabels[u"Marking:email-subject-prefix"_ustr].clear();
            rCategory.m_aLabels[u"Marking:email-subject-suffix"_ustr].clear();
            rCategory.m_aLabels[PROP_STARTVALIDITY()] = PROP_NONE();
            rCategory.m_aLabels[u"Authorization:StopValidity"_ustr] = PROP_NONE();
            m_pCategory = &rCategory;
        }
    }
    else if (rName == "loext:Marking")
    {
        OUString aName = xAttribs->getValueByName(u"Name"_ustr);
        m_aMarkings.push_back(aName);
    }
    else if (rName == "loext:IntellectualPropertyPart")
    {
        OUString aName = xAttribs->getValueByName(u"Name"_ustr);
        m_aIPParts.push_back(aName);
    }
    else if (rName == "loext:IntellectualPropertyPartNumber")
    {
        OUString aName = xAttribs->getValueByName(u"Name"_ustr);
        m_aIPPartNumbers.push_back(aName);
    }
    else if (rName == "baf:Scale")
    {
        m_aScale.clear();
        m_bInScale = true;
    }
    else if (rName == "baf:ConfidentalityValue")
    {
        m_aConfidentalityValue.clear();
        m_bInConfidentalityValue = true;
    }
    else if (rName == "baf:Identifier")
    {
        m_aIdentifier.clear();
        m_bInIdentifier = true;
    }
    else if (rName == "baf:Value")
    {
        m_aValue.clear();
        m_bInValue = true;
    }
}

void SAL_CALL SfxClassificationParser::endElement(const OUString& rName)
{
    if (rName == "baf:PolicyAuthorityName")
        m_bInPolicyAuthorityName = false;
    else if (rName == "baf:PolicyName")
        m_bInPolicyName = false;
    else if (rName == "baf:ProgramID")
        m_bInProgramID = false;
    else if (rName == "baf:BusinessAuthorizationCategory")
        m_pCategory = nullptr;
    else if (rName == "baf:Scale")
    {
        m_bInScale = false;
        if (m_pCategory)
            m_pCategory->m_aLabels[PROP_IMPACTSCALE()] = m_aScale;
    }
    else if (rName == "baf:ConfidentalityValue")
    {
        m_bInConfidentalityValue = false;
        if (m_pCategory)
        {
            std::map<OUString, OUString>& rLabels = m_pCategory->m_aLabels;
            rLabels[PROP_IMPACTLEVEL()] = m_aConfidentalityValue;
            m_pCategory->m_nConfidentiality = m_aConfidentalityValue.toInt32(); // 0-based class sensitivity; 0 is lowest.
            // Set the two other type of levels as well, if they're not set
            // yet: they're optional in BAF, but not in BAILS.
            rLabels.try_emplace(u"Impact:Level:Integrity"_ustr, m_aConfidentalityValue);
            rLabels.try_emplace(u"Impact:Level:Availability"_ustr, m_aConfidentalityValue);
        }
    }
    else if (rName == "baf:Identifier")
        m_bInIdentifier = false;
    else if (rName == "baf:Value")
    {
        if (m_pCategory)
        {
            if (m_aIdentifier == "Document: Header")
                m_pCategory->m_aLabels[SfxClassificationHelper::PROP_DOCHEADER()] = m_aValue;
            else if (m_aIdentifier == "Document: Footer")
                m_pCategory->m_aLabels[SfxClassificationHelper::PROP_DOCFOOTER()] = m_aValue;
            else if (m_aIdentifier == "Document: Watermark")
                m_pCategory->m_aLabels[SfxClassificationHelper::PROP_DOCWATERMARK()] = m_aValue;
        }
    }
}

void SAL_CALL SfxClassificationParser::characters(const OUString& rChars)
{
    if (m_bInPolicyAuthorityName)
        m_aPolicyAuthorityName += rChars;
    else if (m_bInPolicyName)
        m_aPolicyName += rChars;
    else if (m_bInProgramID)
        m_aProgramID += rChars;
    else if (m_bInScale)
        m_aScale += rChars;
    else if (m_bInConfidentalityValue)
        m_aConfidentalityValue += rChars;
    else if (m_bInIdentifier)
        m_aIdentifier += rChars;
    else if (m_bInValue)
        m_aValue += rChars;
}

void SAL_CALL SfxClassificationParser::ignorableWhitespace(const OUString& /*rWhitespace*/)
{
}

void SAL_CALL SfxClassificationParser::processingInstruction(const OUString& /*rTarget*/, const OUString& /*rData*/)
{
}

void SAL_CALL SfxClassificationParser::setDocumentLocator(const uno::Reference<xml::sax::XLocator>& /*xLocator*/)
{
}

} // anonymous namespace

/// Implementation details of SfxClassificationHelper.
class SfxClassificationHelper::Impl
{
public:
    /// Selected categories, one category for each policy type.
    std::map<SfxClassificationPolicyType, SfxClassificationCategory> m_aCategory;
    /// Possible categories of a policy to choose from.
    std::vector<SfxClassificationCategory> m_aCategories;
    std::vector<OUString> m_aMarkings;
    std::vector<OUString> m_aIPParts;
    std::vector<OUString> m_aIPPartNumbers;

    uno::Reference<document::XDocumentProperties> m_xDocumentProperties;

    bool m_bUseLocalized;

    explicit Impl(uno::Reference<document::XDocumentProperties> xDocumentProperties, bool bUseLocalized);
    void parsePolicy();
    /// Synchronize m_aLabels back to the document properties.
    void pushToDocumentProperties();
    /// Set the classification start date to the system time.
    void setStartValidity(SfxClassificationPolicyType eType);
};

SfxClassificationHelper::Impl::Impl(uno::Reference<document::XDocumentProperties> xDocumentProperties, bool bUseLocalized)
    : m_xDocumentProperties(std::move(xDocumentProperties))
    , m_bUseLocalized(bUseLocalized)
{
    parsePolicy();
}

void SfxClassificationHelper::Impl::parsePolicy()
{
    const uno::Reference<uno::XComponentContext>& xComponentContext = comphelper::getProcessComponentContext();
    SvtPathOptions aOptions;
    OUString aPath = aOptions.GetClassificationPath();

    // See if there is a localized variant next to the configured XML.
    OUString aExtension(u".xml"_ustr);
    if (aPath.endsWith(aExtension) && m_bUseLocalized)
    {
        std::u16string_view aBase = aPath.subView(0, aPath.getLength() - aExtension.getLength());
        const LanguageTag& rLanguageTag = Application::GetSettings().GetLanguageTag();
        // Expected format is "<original path>_xx-XX.xml".
        OUString aLocalized = OUString::Concat(aBase) + "_" + rLanguageTag.getBcp47() + aExtension;
        if (FStatHelper::IsDocument(aLocalized))
            aPath = aLocalized;
    }

    xml::sax::InputSource aParserInput;
    std::unique_ptr<SvStream> pStream = utl::UcbStreamHelper::CreateStream(aPath, StreamMode::READ);
    aParserInput.aInputStream.set(new utl::OStreamWrapper(std::move(pStream)));

    uno::Reference<xml::sax::XParser> xParser = xml::sax::Parser::create(xComponentContext);
    rtl::Reference<SfxClassificationParser> xClassificationParser(new SfxClassificationParser());
    xParser->setDocumentHandler(xClassificationParser);
    try
    {
        xParser->parseStream(aParserInput);
    }
    catch (const xml::sax::SAXParseException&)
    {
        TOOLS_WARN_EXCEPTION("sfx.view", "parsePolicy() failed");
    }
    m_aCategories = xClassificationParser->m_aCategories;
    m_aMarkings = xClassificationParser->m_aMarkings;
    m_aIPParts = xClassificationParser->m_aIPParts;
    m_aIPPartNumbers = xClassificationParser->m_aIPPartNumbers;
}

static bool lcl_containsProperty(const uno::Sequence<beans::Property>& rProperties, std::u16string_view rName)
{
    return std::any_of(rProperties.begin(), rProperties.end(), [&](const beans::Property& rProperty)
    {
        return rProperty.Name == rName;
    });
}

void SfxClassificationHelper::Impl::setStartValidity(SfxClassificationPolicyType eType)
{
    auto itCategory = m_aCategory.find(eType);
    if (itCategory == m_aCategory.end())
        return;

    SfxClassificationCategory& rCategory = itCategory->second;
    auto it = rCategory.m_aLabels.find(policyTypeToString(eType) + PROP_STARTVALIDITY());
    if (it != rCategory.m_aLabels.end())
    {
        if (it->second == PROP_NONE())
        {
            // The policy left the start date unchanged, replace it with the system time.
            util::DateTime aDateTime = DateTime(DateTime::SYSTEM).GetUNODateTime();
            it->second = utl::toISO8601(aDateTime);
        }
    }
}

void SfxClassificationHelper::Impl::pushToDocumentProperties()
{
    uno::Reference<beans::XPropertyContainer> xPropertyContainer = m_xDocumentProperties->getUserDefinedProperties();
    uno::Reference<beans::XPropertySet> xPropertySet(xPropertyContainer, uno::UNO_QUERY);
    uno::Sequence<beans::Property> aProperties = xPropertySet->getPropertySetInfo()->getProperties();
    for (auto& rPair : m_aCategory)
    {
        SfxClassificationPolicyType eType = rPair.first;
        SfxClassificationCategory& rCategory = rPair.second;
        std::map<OUString, OUString> aLabels = rCategory.m_aLabels;
        aLabels[policyTypeToString(eType) + PROP_BACNAME()] = rCategory.m_aName;
        for (const auto& rLabel : aLabels)
        {
            try
            {
                if (lcl_containsProperty(aProperties, rLabel.first))
                    xPropertySet->setPropertyValue(rLabel.first, uno::Any(rLabel.second));
                else
                    xPropertyContainer->addProperty(rLabel.first, beans::PropertyAttribute::REMOVABLE, uno::Any(rLabel.second));
            }
            catch (const uno::Exception&)
            {
                TOOLS_WARN_EXCEPTION("sfx.view", "pushDocumentProperties() failed for property " << rLabel.first);
            }
        }
    }
}

bool SfxClassificationHelper::IsClassified(const uno::Reference<document::XDocumentProperties>& xDocumentProperties)
{
    uno::Reference<beans::XPropertyContainer> xPropertyContainer = xDocumentProperties->getUserDefinedProperties();
    if (!xPropertyContainer.is())
        return false;

    uno::Reference<beans::XPropertySet> xPropertySet(xPropertyContainer, uno::UNO_QUERY);
    const uno::Sequence<beans::Property> aProperties = xPropertySet->getPropertySetInfo()->getProperties();
    for (const beans::Property& rProperty : aProperties)
    {
        if (rProperty.Name.startsWith("urn:bails:"))
            return true;
    }

    return false;
}

SfxClassificationCheckPasteResult SfxClassificationHelper::CheckPaste(const uno::Reference<document::XDocumentProperties>& xSource,
        const uno::Reference<document::XDocumentProperties>& xDestination)
{
    if (!SfxClassificationHelper::IsClassified(xSource))
        // No classification on the source side. Return early, regardless the
        // state of the destination side.
        return SfxClassificationCheckPasteResult::None;

    if (!SfxClassificationHelper::IsClassified(xDestination))
    {
        // Paste from a classified document to a non-classified one -> deny.
        return SfxClassificationCheckPasteResult::TargetDocNotClassified;
    }

    // Remaining case: paste between two classified documents.
    SfxClassificationHelper aSource(xSource);
    SfxClassificationHelper aDestination(xDestination);
    if (aSource.GetImpactScale() != aDestination.GetImpactScale())
        // It's possible to compare them if they have the same scale.
        return SfxClassificationCheckPasteResult::None;

    if (aSource.GetImpactLevel() > aDestination.GetImpactLevel())
        // Paste from a doc that has higher classification -> deny.
        return SfxClassificationCheckPasteResult::DocClassificationTooLow;

    return SfxClassificationCheckPasteResult::None;
}

bool SfxClassificationHelper::ShowPasteInfo(SfxClassificationCheckPasteResult eResult)
{
    switch (eResult)
    {
    case SfxClassificationCheckPasteResult::None:
    {
        return true;
    }
    break;
    case SfxClassificationCheckPasteResult::TargetDocNotClassified:
    {
        if (!Application::IsHeadlessModeEnabled())
        {
            std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
                                                                     VclMessageType::Info, VclButtonsType::Ok,
                                                                     SfxResId(STR_TARGET_DOC_NOT_CLASSIFIED)));
            xBox->run();
        }
        return false;
    }
    break;
    case SfxClassificationCheckPasteResult::DocClassificationTooLow:
    {
        if (!Application::IsHeadlessModeEnabled())
        {
            std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
                                                                     VclMessageType::Info, VclButtonsType::Ok,
                                                                     SfxResId(STR_DOC_CLASSIFICATION_TOO_LOW)));
            xBox->run();
        }
        return false;
    }
    break;
    }

    return true;
}

SfxClassificationHelper::SfxClassificationHelper(const uno::Reference<document::XDocumentProperties>& xDocumentProperties, bool bUseLocalizedPolicy)
    : m_pImpl(std::make_unique<Impl>(xDocumentProperties, bUseLocalizedPolicy))
{
    if (!xDocumentProperties.is())
        return;

    uno::Reference<beans::XPropertyContainer> xPropertyContainer = xDocumentProperties->getUserDefinedProperties();
    if (!xPropertyContainer.is())
        return;

    uno::Reference<beans::XPropertySet> xPropertySet(xPropertyContainer, uno::UNO_QUERY);
    const uno::Sequence<beans::Property> aProperties = xPropertySet->getPropertySetInfo()->getProperties();
    for (const beans::Property& rProperty : aProperties)
    {
        if (!rProperty.Name.startsWith("urn:bails:"))
            continue;

        uno::Any aAny = xPropertySet->getPropertyValue(rProperty.Name);
        OUString aValue;
        if (aAny >>= aValue)
        {
            SfxClassificationPolicyType eType = stringToPolicyType(rProperty.Name);
            OUString aPrefix = policyTypeToString(eType);
            if (!rProperty.Name.startsWith(aPrefix))
                // It's a prefix we did not recognize, ignore.
                continue;

            //TODO: Support abbreviated names(?)
            if (rProperty.Name == Concat2View(aPrefix + PROP_BACNAME()))
                m_pImpl->m_aCategory[eType].m_aName = aValue;
            else
                m_pImpl->m_aCategory[eType].m_aLabels[rProperty.Name] = aValue;
        }
    }
}

SfxClassificationHelper::~SfxClassificationHelper() = default;

std::vector<OUString> const & SfxClassificationHelper::GetMarkings() const
{
    return m_pImpl->m_aMarkings;
}

std::vector<OUString> const & SfxClassificationHelper::GetIntellectualPropertyParts() const
{
    return m_pImpl->m_aIPParts;
}

std::vector<OUString> const & SfxClassificationHelper::GetIntellectualPropertyPartNumbers() const
{
    return m_pImpl->m_aIPPartNumbers;
}

const OUString& SfxClassificationHelper::GetBACName(SfxClassificationPolicyType eType) const
{
    return m_pImpl->m_aCategory[eType].m_aName;
}

const OUString& SfxClassificationHelper::GetAbbreviatedBACName(const OUString& sFullName)
{
    for (const auto& category : m_pImpl->m_aCategories)
    {
        if (category.m_aName == sFullName)
            return category.m_aAbbreviatedName;
    }

    return sFullName;
}

OUString SfxClassificationHelper::GetBACNameForIdentifier(std::u16string_view sIdentifier)
{
    if (sIdentifier.empty())
        return u""_ustr;

    for (const auto& category : m_pImpl->m_aCategories)
    {
        if (category.m_aIdentifier == sIdentifier)
            return category.m_aName;
    }

    return u""_ustr;
}

OUString SfxClassificationHelper::GetHigherClass(const OUString& first, const OUString& second)
{
    size_t nFirstConfidentiality = 0;
    size_t nSecondConfidentiality = 0;
    for (const auto& category : m_pImpl->m_aCategories)
    {
        if (category.m_aName == first)
            nFirstConfidentiality = category.m_nConfidentiality;
        if (category.m_aName == second)
            nSecondConfidentiality = category.m_nConfidentiality;
    }

    return nFirstConfidentiality >= nSecondConfidentiality ? first : second;
}

bool SfxClassificationHelper::HasImpactLevel()
{
    auto itCategory = m_pImpl->m_aCategory.find(SfxClassificationPolicyType::IntellectualProperty);
    if (itCategory == m_pImpl->m_aCategory.end())
        return false;

    SfxClassificationCategory& rCategory = itCategory->second;
    auto it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_IMPACTSCALE());
    if (it == rCategory.m_aLabels.end())
        return false;

    it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_IMPACTLEVEL());
    return it != rCategory.m_aLabels.end();
}

bool SfxClassificationHelper::HasDocumentHeader()
{
    auto itCategory = m_pImpl->m_aCategory.find(SfxClassificationPolicyType::IntellectualProperty);
    if (itCategory == m_pImpl->m_aCategory.end())
        return false;

    SfxClassificationCategory& rCategory = itCategory->second;
    auto it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_DOCHEADER());
    return it != rCategory.m_aLabels.end() && !it->second.isEmpty();
}

bool SfxClassificationHelper::HasDocumentFooter()
{
    auto itCategory = m_pImpl->m_aCategory.find(SfxClassificationPolicyType::IntellectualProperty);
    if (itCategory == m_pImpl->m_aCategory.end())
        return false;

    SfxClassificationCategory& rCategory = itCategory->second;
    auto it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_DOCFOOTER());
    return it != rCategory.m_aLabels.end() && !it->second.isEmpty();
}

InfobarType SfxClassificationHelper::GetImpactLevelType()
{
    InfobarType aRet;

    aRet = InfobarType::WARNING;

    auto itCategory = m_pImpl->m_aCategory.find(SfxClassificationPolicyType::IntellectualProperty);
    if (itCategory == m_pImpl->m_aCategory.end())
        return aRet;

    SfxClassificationCategory& rCategory = itCategory->second;
    auto it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_IMPACTSCALE());
    if (it == rCategory.m_aLabels.end())
        return aRet;
    OUString aScale = it->second;

    it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_IMPACTLEVEL());
    if (it == rCategory.m_aLabels.end())
        return aRet;
    OUString aLevel = it->second;

    // The spec defines two valid scale values: FIPS-199 and UK-Cabinet.
    if (aScale == "UK-Cabinet")
    {
        if (aLevel == "0")
            aRet = InfobarType::SUCCESS;
        else if (aLevel == "1")
            aRet = InfobarType::WARNING;
        else if (aLevel == "2")
            aRet = InfobarType::WARNING;
        else if (aLevel == "3")
            aRet = InfobarType::DANGER;
    }
    else if (aScale == "FIPS-199")
    {
        if (aLevel == "Low")
            aRet = InfobarType::SUCCESS;
        else if (aLevel == "Moderate")
            aRet = InfobarType::WARNING;
        else if (aLevel == "High")
            aRet = InfobarType::DANGER;
    }
    return aRet;
}

sal_Int32 SfxClassificationHelper::GetImpactLevel()
{
    sal_Int32 nRet = -1;

    auto itCategory = m_pImpl->m_aCategory.find(SfxClassificationPolicyType::IntellectualProperty);
    if (itCategory == m_pImpl->m_aCategory.end())
        return nRet;

    SfxClassificationCategory& rCategory = itCategory->second;
    auto it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_IMPACTSCALE());
    if (it == rCategory.m_aLabels.end())
        return nRet;
    OUString aScale = it->second;

    it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_IMPACTLEVEL());
    if (it == rCategory.m_aLabels.end())
        return nRet;
    OUString aLevel = it->second;

    if (aScale == "UK-Cabinet")
    {
        sal_Int32 nValue = aLevel.toInt32();
        if (nValue < 0 || nValue > 3)
            return nRet;
        nRet = nValue;
    }
    else if (aScale == "FIPS-199")
    {
        static std::map<OUString, sal_Int32> const aValues
        {
            { "Low", 0 },
            { "Moderate", 1 },
            { "High", 2 }
        };
        auto itValues = aValues.find(aLevel);
        if (itValues == aValues.end())
            return nRet;
        nRet = itValues->second;
    }

    return nRet;
}

OUString SfxClassificationHelper::GetImpactScale()
{
    auto itCategory = m_pImpl->m_aCategory.find(SfxClassificationPolicyType::IntellectualProperty);
    if (itCategory == m_pImpl->m_aCategory.end())
        return OUString();

    SfxClassificationCategory& rCategory = itCategory->second;
    auto it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_IMPACTSCALE());
    if (it != rCategory.m_aLabels.end())
        return it->second;

    return OUString();
}

OUString SfxClassificationHelper::GetDocumentWatermark()
{
    auto itCategory = m_pImpl->m_aCategory.find(SfxClassificationPolicyType::IntellectualProperty);
    if (itCategory == m_pImpl->m_aCategory.end())
        return OUString();

    SfxClassificationCategory& rCategory = itCategory->second;
    auto it = rCategory.m_aLabels.find(PROP_PREFIX_INTELLECTUALPROPERTY() + PROP_DOCWATERMARK());
    if (it != rCategory.m_aLabels.end())
        return it->second;

    return OUString();
}

std::vector<OUString> SfxClassificationHelper::GetBACNames()
{
    if (m_pImpl->m_aCategories.empty())
        m_pImpl->parsePolicy();

    std::vector<OUString> aRet;
    std::transform(m_pImpl->m_aCategories.begin(), m_pImpl->m_aCategories.end(), std::back_inserter(aRet), [](const SfxClassificationCategory& rCategory)
    {
        return rCategory.m_aName;
    });
    return aRet;
}

std::vector<OUString> SfxClassificationHelper::GetBACIdentifiers()
{
    if (m_pImpl->m_aCategories.empty())
        m_pImpl->parsePolicy();

    std::vector<OUString> aRet;
    std::transform(m_pImpl->m_aCategories.begin(), m_pImpl->m_aCategories.end(), std::back_inserter(aRet), [](const SfxClassificationCategory& rCategory)
    {
        return rCategory.m_aIdentifier;
    });
    return aRet;
}

std::vector<OUString> SfxClassificationHelper::GetAbbreviatedBACNames()
{
    if (m_pImpl->m_aCategories.empty())
        m_pImpl->parsePolicy();

    std::vector<OUString> aRet;
    std::transform(m_pImpl->m_aCategories.begin(), m_pImpl->m_aCategories.end(), std::back_inserter(aRet), [](const SfxClassificationCategory& rCategory)
    {
        return rCategory.m_aAbbreviatedName;
    });
    return aRet;
}

void SfxClassificationHelper::SetBACName(const OUString& rName, SfxClassificationPolicyType eType)
{
    if (m_pImpl->m_aCategories.empty())
        m_pImpl->parsePolicy();

    auto it = std::find_if(m_pImpl->m_aCategories.begin(), m_pImpl->m_aCategories.end(), [&](const SfxClassificationCategory& rCategory)
    {
        return rCategory.m_aName == rName;
    });
    if (it == m_pImpl->m_aCategories.end())
    {
        SAL_WARN("sfx.view", "'" << rName << "' is not a recognized category name");
        return;
    }

    m_pImpl->m_aCategory[eType].m_aName = it->m_aName;
    m_pImpl->m_aCategory[eType].m_aAbbreviatedName = it->m_aAbbreviatedName;
    m_pImpl->m_aCategory[eType].m_nConfidentiality = it->m_nConfidentiality;
    m_pImpl->m_aCategory[eType].m_aLabels.clear();
    const OUString& rPrefix = policyTypeToString(eType);
    for (const auto& rLabel : it->m_aLabels)
        m_pImpl->m_aCategory[eType].m_aLabels[rPrefix + rLabel.first] = rLabel.second;

    m_pImpl->setStartValidity(eType);
    m_pImpl->pushToDocumentProperties();
    SfxViewFrame* pViewFrame = SfxViewFrame::Current();
    if (!pViewFrame)
        return;

    UpdateInfobar(*pViewFrame);
}

void SfxClassificationHelper::UpdateInfobar(SfxViewFrame& rViewFrame)
{
    OUString aBACName = GetBACName(SfxClassificationPolicyType::IntellectualProperty);
    bool bImpactLevel = HasImpactLevel();
    if (!aBACName.isEmpty() && bImpactLevel)
    {
        OUString aMessage = SfxResId(STR_CLASSIFIED_DOCUMENT);
        aMessage = aMessage.replaceFirst("%1", aBACName);

        rViewFrame.RemoveInfoBar(u"classification");
        rViewFrame.AppendInfoBar(u"classification"_ustr, u""_ustr, aMessage, GetImpactLevelType());
    }
}

SfxClassificationPolicyType SfxClassificationHelper::stringToPolicyType(std::u16string_view rType)
{
    if (o3tl::starts_with(rType, PROP_PREFIX_EXPORTCONTROL()))
        return SfxClassificationPolicyType::ExportControl;
    else if (o3tl::starts_with(rType, PROP_PREFIX_NATIONALSECURITY()))
        return SfxClassificationPolicyType::NationalSecurity;
    else
        return SfxClassificationPolicyType::IntellectualProperty;
}

const OUString& SfxClassificationHelper::policyTypeToString(SfxClassificationPolicyType eType)
{
    switch (eType)
    {
    case SfxClassificationPolicyType::ExportControl:
        return PROP_PREFIX_EXPORTCONTROL();
    case SfxClassificationPolicyType::NationalSecurity:
        return PROP_PREFIX_NATIONALSECURITY();
    case SfxClassificationPolicyType::IntellectualProperty:
        break;
    }

    return PROP_PREFIX_INTELLECTUALPROPERTY();
}

const OUString& SfxClassificationHelper::PROP_DOCHEADER()
{
    static constexpr OUString sProp(u"Marking:document-header"_ustr);
    return sProp;
}

const OUString& SfxClassificationHelper::PROP_DOCFOOTER()
{
    static constexpr OUString sProp(u"Marking:document-footer"_ustr);
    return sProp;
}

const OUString& SfxClassificationHelper::PROP_DOCWATERMARK()
{
    static constexpr OUString sProp(u"Marking:document-watermark"_ustr);
    return sProp;
}

const OUString& SfxClassificationHelper::PROP_PREFIX_INTELLECTUALPROPERTY()
{
    static constexpr OUString sProp(u"urn:bails:IntellectualProperty:"_ustr);
    return sProp;
}

SfxClassificationPolicyType SfxClassificationHelper::getPolicyType()
{
    if (comphelper::IsFuzzing())
        return SfxClassificationPolicyType::IntellectualProperty;
    sal_Int32 nPolicyTypeNumber = officecfg::Office::Common::Classification::Policy::get();
    auto eType = static_cast<SfxClassificationPolicyType>(nPolicyTypeNumber);
    return eType;
}

namespace sfx
{

namespace
{

OUString getProperty(uno::Reference<beans::XPropertyContainer> const& rxPropertyContainer,
                     OUString const& rName)
{
    try
    {
        uno::Reference<beans::XPropertySet> xPropertySet(rxPropertyContainer, uno::UNO_QUERY);
        return xPropertySet->getPropertyValue(rName).get<OUString>();
    }
    catch (const css::uno::Exception&)
    {
    }

    return OUString();
}

} // end anonymous namespace

sfx::ClassificationCreationOrigin getCreationOriginProperty(uno::Reference<beans::XPropertyContainer> const & rxPropertyContainer,
                                                            sfx::ClassificationKeyCreator const & rKeyCreator)
{
    OUString sValue = getProperty(rxPropertyContainer, rKeyCreator.makeCreationOriginKey());
    if (sValue.isEmpty())
        return sfx::ClassificationCreationOrigin::NONE;

    return (sValue == "BAF_POLICY")
                ? sfx::ClassificationCreationOrigin::BAF_POLICY
                : sfx::ClassificationCreationOrigin::MANUAL;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
