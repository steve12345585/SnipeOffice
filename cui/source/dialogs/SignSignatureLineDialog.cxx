/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <SignSignatureLineDialog.hxx>

#include <sal/log.hxx>
#include <sal/types.h>

#include <dialmgr.hxx>
#include <strings.hrc>

#include <comphelper/graphicmimetype.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propertyvalue.hxx>
#include <sfx2/filedlghelper.hxx>
#include <sfx2/objsh.hxx>
#include <svx/xoutbmp.hxx>
#include <utility>
#include <vcl/graph.hxx>
#include <vcl/weld.hxx>
#include <svx/signaturelinehelper.hxx>
#include <tools/urlobj.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/graphic/GraphicProvider.hpp>
#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <com/sun/star/security/CertificateKind.hpp>
#include <com/sun/star/security/XCertificate.hpp>
#include <com/sun/star/ui/dialogs/TemplateDescription.hpp>
#include <com/sun/star/ui/dialogs/XFilePicker3.hpp>

using namespace comphelper;
using namespace css;
using namespace css::uno;
using namespace css::beans;
using namespace css::frame;
using namespace css::frame;
using namespace css::text;
using namespace css::graphic;
using namespace css::security;
using namespace css::ui::dialogs;

SignSignatureLineDialog::SignSignatureLineDialog(weld::Widget* pParent, Reference<XModel> xModel)
    : SignatureLineDialogBase(pParent, std::move(xModel), u"cui/ui/signsignatureline.ui"_ustr,
                              u"SignSignatureLineDialog"_ustr)
    , m_xEditName(m_xBuilder->weld_entry(u"edit_name"_ustr))
    , m_xEditComment(m_xBuilder->weld_text_view(u"edit_comment"_ustr))
    , m_xBtnLoadImage(m_xBuilder->weld_button(u"btn_load_image"_ustr))
    , m_xBtnClearImage(m_xBuilder->weld_button(u"btn_clear_image"_ustr))
    , m_xBtnChooseCertificate(m_xBuilder->weld_button(u"btn_select_certificate"_ustr))
    , m_xBtnSign(m_xBuilder->weld_button(u"ok"_ustr))
    , m_xLabelHint(m_xBuilder->weld_label(u"label_hint"_ustr))
    , m_xLabelHintText(m_xBuilder->weld_label(u"label_hint_text"_ustr))
    , m_xLabelAddComment(m_xBuilder->weld_label(u"label_add_comment"_ustr))
    , m_bShowSignDate(false)
{
    Reference<container::XIndexAccess> xIndexAccess(m_xModel->getCurrentSelection(),
                                                    UNO_QUERY_THROW);
    m_xShapeProperties.set(xIndexAccess->getByIndex(0), UNO_QUERY_THROW);

    bool bIsSignatureLine(false);
    m_xShapeProperties->getPropertyValue(u"IsSignatureLine"_ustr) >>= bIsSignatureLine;
    if (!bIsSignatureLine)
    {
        SAL_WARN("cui.dialogs", "No signature line selected!");
        return;
    }

    m_xBtnLoadImage->connect_clicked(LINK(this, SignSignatureLineDialog, loadImage));
    m_xBtnClearImage->connect_clicked(LINK(this, SignSignatureLineDialog, clearImage));
    m_xBtnChooseCertificate->connect_clicked(
        LINK(this, SignSignatureLineDialog, chooseCertificate));
    m_xEditName->connect_changed(LINK(this, SignSignatureLineDialog, entryChanged));

    // Read properties from selected signature line
    m_xShapeProperties->getPropertyValue(u"SignatureLineId"_ustr) >>= m_aSignatureLineId;
    m_xShapeProperties->getPropertyValue(u"SignatureLineSuggestedSignerName"_ustr)
        >>= m_aSuggestedSignerName;
    m_xShapeProperties->getPropertyValue(u"SignatureLineSuggestedSignerTitle"_ustr)
        >>= m_aSuggestedSignerTitle;
    OUString aSigningInstructions;
    m_xShapeProperties->getPropertyValue(u"SignatureLineSigningInstructions"_ustr)
        >>= aSigningInstructions;
    m_xShapeProperties->getPropertyValue(u"SignatureLineShowSignDate"_ustr) >>= m_bShowSignDate;
    bool bCanAddComment(false);
    m_xShapeProperties->getPropertyValue(u"SignatureLineCanAddComment"_ustr) >>= bCanAddComment;

    if (aSigningInstructions.isEmpty())
    {
        m_xLabelHint->hide();
        m_xLabelHintText->hide();
    }
    else
    {
        m_xLabelHintText->set_label(aSigningInstructions);
    }

    if (bCanAddComment)
    {
        m_xEditComment->set_size_request(m_xEditComment->get_approximate_digit_width() * 48,
                                         m_xEditComment->get_text_height() * 5);
    }
    else
    {
        m_xLabelAddComment->hide();
        m_xEditComment->hide();
        m_xEditComment->set_size_request(0, 0);
    }

    ValidateFields();
}

IMPL_LINK_NOARG(SignSignatureLineDialog, loadImage, weld::Button&, void)
{
    const Reference<XComponentContext>& xContext = comphelper::getProcessComponentContext();
    sfx2::FileDialogHelper aHelper(TemplateDescription::FILEOPEN_PREVIEW, FileDialogFlags::NONE,
                                   m_xDialog.get());
    aHelper.SetContext(sfx2::FileDialogHelper::SignatureLine);
    Reference<XFilePicker3> xFilePicker = aHelper.GetFilePicker();
    if (!xFilePicker->execute())
        return;

    Sequence<OUString> aSelectedFiles = xFilePicker->getSelectedFiles();
    if (!aSelectedFiles.hasElements())
        return;

    Reference<XGraphicProvider> xProvider = GraphicProvider::create(xContext);
    Sequence<PropertyValue> aMediaProperties{ comphelper::makePropertyValue(u"URL"_ustr,
                                                                            aSelectedFiles[0]) };
    m_xSignatureImage = xProvider->queryGraphic(aMediaProperties);
    m_sOriginalImageBtnLabel = m_xBtnLoadImage->get_label();

    INetURLObject aObj(aSelectedFiles[0]);
    m_xBtnLoadImage->set_label(aObj.GetLastName());

    ValidateFields();
}

IMPL_LINK_NOARG(SignSignatureLineDialog, clearImage, weld::Button&, void)
{
    m_xSignatureImage.clear();
    m_xBtnLoadImage->set_label(m_sOriginalImageBtnLabel);
    ValidateFields();
}

IMPL_LINK_NOARG(SignSignatureLineDialog, chooseCertificate, weld::Button&, void)
{
    // Document needs to be saved before selecting a certificate
    SfxObjectShell* pShell = SfxObjectShell::Current();
    if (!pShell || !pShell->PrepareForSigning(m_xDialog.get()))
        return;

    Reference<XCertificate> xSignCertificate
        = svx::SignatureLineHelper::getSignatureCertificate(pShell, nullptr, m_xDialog.get());

    if (xSignCertificate.is())
    {
        m_xSelectedCertifate = xSignCertificate;
        svl::crypto::CertificateOrName aCertificateOrName;
        aCertificateOrName.m_xCertificate = std::move(xSignCertificate);
        m_xBtnChooseCertificate->set_label(
            svx::SignatureLineHelper::getSignerName(aCertificateOrName));
    }
    ValidateFields();
}

IMPL_LINK_NOARG(SignSignatureLineDialog, entryChanged, weld::Entry&, void) { ValidateFields(); }

void SignSignatureLineDialog::ValidateFields()
{
    bool bEnableSignBtn = m_xSelectedCertifate.is()
                          && (!m_xEditName->get_text().isEmpty() || m_xSignatureImage.is());
    m_xBtnSign->set_sensitive(bEnableSignBtn);

    m_xEditName->set_sensitive(!m_xSignatureImage.is());
    m_xBtnLoadImage->set_sensitive(m_xEditName->get_text().isEmpty());
    m_xBtnClearImage->set_sensitive(m_xSignatureImage.is());
}

void SignSignatureLineDialog::Apply()
{
    if (!m_xSelectedCertifate.is())
    {
        SAL_WARN("cui.dialogs", "No certificate selected!");
        return;
    }

    SfxObjectShell* pShell = SfxObjectShell::Current();
    if (!pShell)
    {
        SAL_WARN("cui.dialogs", "No SfxObjectShell!");
        return;
    }

    Reference<XGraphic> xValidGraphic = getSignedGraphic(true);
    Reference<XGraphic> xInvalidGraphic = getSignedGraphic(false);
    pShell->SignSignatureLine(m_xDialog.get(), m_aSignatureLineId, m_xSelectedCertifate,
                              xValidGraphic, xInvalidGraphic, m_xEditComment->get_text());
}

css::uno::Reference<css::graphic::XGraphic> SignSignatureLineDialog::getSignedGraphic(bool bValid)
{
    // Read svg and replace placeholder texts
    OUString aSvgImage(svx::SignatureLineHelper::getSignatureImage());
    aSvgImage = aSvgImage.replaceAll("[SIGNER_NAME]", getCDataString(m_aSuggestedSignerName));
    aSvgImage = aSvgImage.replaceAll("[SIGNER_TITLE]", getCDataString(m_aSuggestedSignerTitle));

    svl::crypto::CertificateOrName aCertificateOrName;
    aCertificateOrName.m_xCertificate = m_xSelectedCertifate;
    OUString aIssuerLine
        = CuiResId(RID_CUISTR_SIGNATURELINE_SIGNED_BY)
              .replaceFirst("%1", svx::SignatureLineHelper::getSignerName(aCertificateOrName));
    aSvgImage = aSvgImage.replaceAll("[SIGNED_BY]", getCDataString(aIssuerLine));
    if (bValid)
        aSvgImage = aSvgImage.replaceAll("[INVALID_SIGNATURE]", "");

    OUString aDate;
    if (m_bShowSignDate && bValid)
    {
        aDate = svx::SignatureLineHelper::getLocalizedDate();
    }
    aSvgImage = aSvgImage.replaceAll("[DATE]", aDate);

    // Custom signature image
    if (m_xSignatureImage.is())
    {
        OUString aGraphicInBase64;
        Graphic aGraphic(m_xSignatureImage);
        if (!XOutBitmap::GraphicToBase64(aGraphic, aGraphicInBase64, false))
            SAL_WARN("cui.dialogs", "Could not convert graphic to base64");

        OUString aImagePart = u"<image y=\"825\" x=\"1300\" "
                              "xlink:href=\"data:[MIMETYPE];base64,[BASE64_IMG]>\" "
                              "preserveAspectRatio=\"xMidYMid\" height=\"1520\" "
                              "width=\"7600\" />"_ustr;
        aImagePart = aImagePart.replaceAll(
            "[MIMETYPE]", GraphicMimeTypeHelper::GetMimeTypeForXGraphic(m_xSignatureImage));
        aImagePart = aImagePart.replaceAll("[BASE64_IMG]", aGraphicInBase64);
        aSvgImage = aSvgImage.replaceAll("[SIGNATURE_IMAGE]", aImagePart);

        aSvgImage = aSvgImage.replaceAll("[SIGNATURE]", "");
    }
    else
    {
        aSvgImage = aSvgImage.replaceAll("[SIGNATURE_IMAGE]", "");
        aSvgImage = aSvgImage.replaceAll("[SIGNATURE]", getCDataString(m_xEditName->get_text()));
    }

    // Create graphic
    return svx::SignatureLineHelper::importSVG(aSvgImage);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
