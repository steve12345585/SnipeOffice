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

#pragma once

#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/security/DocumentSignatureInformation.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <vcl/weld.hxx>

namespace com::sun::star::security { class XCertificate; }


class MacroWarning : public weld::MessageDialogController
{
private:
    std::unique_ptr<weld::Widget> mxGrid;
    std::unique_ptr<weld::Label> mxSignsFI;
    std::unique_ptr<weld::Label> mxNotYetValid;
    std::unique_ptr<weld::Label> mxNoLongerValid;
    std::unique_ptr<weld::Button> mxViewSignsBtn;
    std::unique_ptr<weld::Button> mxViewCertBtn;
    std::unique_ptr<weld::CheckButton> mxAlwaysTrustCB;
    std::unique_ptr<weld::Button> mxEnableBtn;
    std::unique_ptr<weld::Button> mxDisableBtn;

    css::uno::Reference< css::security::XCertificate >  mxCert;
    css::uno::Reference< css::embed::XStorage >         mxStore;
    OUString                                 maODFVersion;
    const css::uno::Sequence< css::security::DocumentSignatureInformation >*    mpInfos;

    const bool          mbShowSignatures;
    sal_Int32           mnActSecLevel;

    DECL_LINK(ViewSignsBtnHdl, weld::Button&, void);
    DECL_LINK(EnableBtnHdl, weld::Button&, void);
    DECL_LINK(DisableBtnHdl, weld::Button&, void);
    DECL_LINK(AlwaysTrustCheckHdl, weld::Toggleable&, void);
    DECL_STATIC_LINK(MacroWarning, InstallLOKNotifierHdl, void*, vcl::ILibreOfficeKitNotifier*);

    void                InitControls();
    void EnableOkBtn(bool bEnable);

public:
    MacroWarning(weld::Window* pParent, bool _bShowSignatures);

    void    SetDocumentURL( const OUString& rDocURL );

    void    SetStorage( const css::uno::Reference < css::embed::XStorage >& rxStore,
                        const OUString& aODFVersion,
                        const css::uno::Sequence< css::security::DocumentSignatureInformation >& _rInfos );
    void    SetCertificate( const css::uno::Reference< css::security::XCertificate >& _rxCert );
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
