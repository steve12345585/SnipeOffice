/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <svx/signaturelinehelper.hxx>

#include <com/sun/star/drawing/XShape.hpp>
#include <com/sun/star/graphic/GraphicProvider.hpp>
#include <com/sun/star/security/DocumentDigitalSignatures.hpp>

#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <comphelper/storagehelper.hxx>
#include <comphelper/xmlsechelper.hxx>
#include <config_folders.h>
#include <rtl/bootstrap.hxx>
#include <sal/log.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/docfilt.hxx>
#include <sfx2/objsh.hxx>
#include <svx/dialmgr.hxx>
#include <svx/strings.hrc>
#include <svx/svdmark.hxx>
#include <svx/svdview.hxx>
#include <tools/stream.hxx>
#include <unotools/localedatawrapper.hxx>
#include <unotools/streamwrap.hxx>
#include <unotools/syslocale.hxx>
#include <vcl/weld.hxx>
#include <sfx2/digitalsignatures.hxx>
#include <sfx2/viewsh.hxx>

using namespace com::sun::star;

namespace svx::SignatureLineHelper
{
OUString getSignatureImage(const OUString& rType)
{
    OUString aType = rType;
    if (aType.isEmpty())
    {
        aType = "signature-line.svg";
    }
    OUString aPath("$BRAND_BASE_DIR/" LIBO_SHARE_FOLDER "/filter/" + aType);
    rtl::Bootstrap::expandMacros(aPath);
    SvFileStream aStream(aPath, StreamMode::READ);
    if (aStream.GetError() != ERRCODE_NONE)
    {
        SAL_WARN("cui.dialogs", "failed to open " << aType);
    }

    OString const svg = read_uInt8s_ToOString(aStream, aStream.remainingSize());
    return OUString::fromUtf8(svg);
}

uno::Reference<security::XCertificate>
getSignatureCertificate(SfxObjectShell* pShell, SfxViewShell* pViewShell, weld::Window* pParent)
{
    if (!pShell)
    {
        return {};
    }

    if (!pParent)
    {
        return {};
    }

    uno::Reference<security::XDocumentDigitalSignatures> xSigner;
    if (pShell->GetMedium()->GetFilter()->IsAlienFormat())
    {
        xSigner = security::DocumentDigitalSignatures::createDefault(
            comphelper::getProcessComponentContext());
    }
    else
    {
        OUString const aODFVersion(
            comphelper::OStorageHelper::GetODFVersionFromStorage(pShell->GetStorage()));
        xSigner = security::DocumentDigitalSignatures::createWithVersion(
            comphelper::getProcessComponentContext(), aODFVersion);
    }
    xSigner->setParentWindow(pParent->GetXWindow());
    OUString aDescription;
    security::CertificateKind certificateKind = security::CertificateKind_NONE;
    // When signing ooxml, we only want X.509 certificates
    if (pShell->GetMedium()->GetFilter()->IsAlienFormat())
    {
        certificateKind = security::CertificateKind_X509;
    }
    auto xModelSigner = dynamic_cast<sfx2::DigitalSignatures*>(xSigner.get());
    assert(xModelSigner);
    uno::Reference<security::XCertificate> xSignCertificate
        = xModelSigner->SelectSigningCertificateWithType(pViewShell, certificateKind, aDescription);
    return xSignCertificate;
}

OUString getSignerName(const svl::crypto::CertificateOrName& rCertificateOrName)
{
    if (rCertificateOrName.m_xCertificate.is())
    {
        return comphelper::xmlsec::GetContentPart(
            rCertificateOrName.m_xCertificate->getSubjectName(),
            rCertificateOrName.m_xCertificate->getCertificateKind());
    }

    return rCertificateOrName.m_aName;
}

OUString getLocalizedDate()
{
    const SvtSysLocale aSysLocale;
    const LocaleDataWrapper& rLocaleData = aSysLocale.GetLocaleData();
    Date aDateTime(Date::SYSTEM);
    return rLocaleData.getDate(aDateTime);
}

uno::Reference<graphic::XGraphic> importSVG(std::u16string_view rSVG)
{
    SvMemoryStream aSvgStream(4096, 4096);
    aSvgStream.WriteOString(OUStringToOString(rSVG, RTL_TEXTENCODING_UTF8));
    uno::Reference<io::XInputStream> xInputStream(new utl::OSeekableInputStreamWrapper(aSvgStream));
    const uno::Reference<uno::XComponentContext>& xContext(
        comphelper::getProcessComponentContext());
    uno::Reference<graphic::XGraphicProvider> xProvider
        = graphic::GraphicProvider::create(xContext);

    uno::Sequence<beans::PropertyValue> aMediaProperties{ comphelper::makePropertyValue(
        u"InputStream"_ustr, xInputStream) };
    uno::Reference<graphic::XGraphic> xGraphic(xProvider->queryGraphic(aMediaProperties));
    return xGraphic;
}

void setShapeCertificate(SfxViewShell* pViewShell,
                         const svl::crypto::CertificateOrName& rCertificateOrName)
{
    const SdrView* pView = pViewShell->GetDrawView();
    const SdrMarkList& rMarkList = pView->GetMarkedObjectList();
    if (rMarkList.GetMarkCount() < 1)
    {
        return;
    }

    const SdrMark* pMark = rMarkList.GetMark(0);
    SdrObject* pSignatureLine = pMark->GetMarkedSdrObj();
    if (!pSignatureLine)
    {
        return;
    }

    // Remember the selected certificate.
    uno::Reference<drawing::XShape> xShape = pSignatureLine->getUnoShape();
    uno::Reference<beans::XPropertySet> xShapeProps(xShape, uno::UNO_QUERY);
    pViewShell->SetSignPDFCertificate(rCertificateOrName);

    // Read svg and replace placeholder texts.
    OUString aSvgImage(
        svx::SignatureLineHelper::getSignatureImage(u"signature-line-draw.svg"_ustr));
    aSvgImage = aSvgImage.replaceAll("[SIGNED_BY]", SvxResId(RID_SVXSTR_SIGNATURELINE_DSIGNED_BY));
    OUString aSignerName = svx::SignatureLineHelper::getSignerName(rCertificateOrName);
    aSvgImage = aSvgImage.replaceAll("[SIGNER_NAME]", aSignerName);
    OUString aDate = svx::SignatureLineHelper::getLocalizedDate();
    aDate = SvxResId(RID_SVXSTR_SIGNATURELINE_DATE).replaceFirst("%1", aDate);
    aSvgImage = aSvgImage.replaceAll("[DATE]", aDate);

    uno::Reference<graphic::XGraphic> xGraphic = svx::SignatureLineHelper::importSVG(aSvgImage);
    xShapeProps->setPropertyValue(u"Graphic"_ustr, uno::Any(xGraphic));
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
