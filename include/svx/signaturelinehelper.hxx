/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SVX_SIGNATURELINEHELPER_HXX
#define INCLUDED_SVX_SIGNATURELINEHELPER_HXX

#include <sal/config.h>

#include <string_view>

#include <rtl/ustring.hxx>
#include <svx/svxdllapi.h>
#include <svl/cryptosign.hxx>

#include <com/sun/star/graphic/XGraphic.hpp>
#include <com/sun/star/security/XCertificate.hpp>

namespace weld
{
class Window;
}
class SdrView;
class SfxObjectShell;
class SfxViewShell;

namespace svx::SignatureLineHelper
{
/**
 * Returns an SVG template. Once placeholders are replaced with real content, the result can be used
 * as the graphic of a signature line shape.
 */
SVX_DLLPUBLIC OUString getSignatureImage(const OUString& rType = OUString());

/**
 * Choose a signature for signature line purposes.
 */
SVX_DLLPUBLIC css::uno::Reference<css::security::XCertificate>
getSignatureCertificate(SfxObjectShell* pShell, SfxViewShell* pViewShell, weld::Window* pParent);

/**
 * Get a signer name out of a certificate.
 */
SVX_DLLPUBLIC OUString getSignerName(const svl::crypto::CertificateOrName& rCertificateOrName);

/**
 * Gets a localized date string.
 */
SVX_DLLPUBLIC OUString getLocalizedDate();

/**
 * Interprets rSVG as a graphic and gives back the resulting UNO wrapper.
 */
SVX_DLLPUBLIC css::uno::Reference<css::graphic::XGraphic> importSVG(std::u16string_view rSVG);

/**
 * Sets xCertificate as the signing certificate of the selected shape on pView.
 */
SVX_DLLPUBLIC void setShapeCertificate(SfxViewShell* pViewShell,
                                       const svl::crypto::CertificateOrName& rCertificateOrName);
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
