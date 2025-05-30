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
#ifndef INCLUDED_VBAHELPER_VBADOCUMENTBASE_HXX
#define INCLUDED_VBAHELPER_VBADOCUMENTBASE_HXX

#include <exception>

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/XInterface.hpp>
#include <ooo/vba/XDocumentBase.hpp>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <vbahelper/vbadllapi.h>
#include <vbahelper/vbahelper.hxx>
#include <vbahelper/vbahelperinterface.hxx>

namespace com::sun::star {
    namespace frame { class XModel; }
    namespace uno { class XComponentContext; }
}

namespace ooo::vba {
    class XDocumentBase;
    class XHelperInterface;
}

typedef InheritedHelperInterfaceWeakImpl< ooo::vba::XDocumentBase > VbaDocumentBase_BASE;

class VBAHELPER_DLLPUBLIC VbaDocumentBase : public VbaDocumentBase_BASE
{
protected:
    css::uno::Reference< css::uno::XInterface > mxVBProject;
protected:
    virtual css::uno::Reference< css::frame::XModel > getModel() const = 0;
public:
    VbaDocumentBase(    const css::uno::Reference< ov::XHelperInterface >& xParent, const css::uno::Reference< css::uno::XComponentContext >& xContext );
    VbaDocumentBase(    css::uno::Sequence< css::uno::Any > const& aArgs, css::uno::Reference< css::uno::XComponentContext >const& xContext );

    // Attributes
    virtual OUString SAL_CALL getName() override;
    virtual OUString SAL_CALL getPath() override;
    virtual OUString SAL_CALL getFullName() override;
    virtual sal_Bool SAL_CALL getSaved() override;
    virtual void SAL_CALL setSaved( sal_Bool bSave ) override;
    virtual css::uno::Any SAL_CALL getVBProject() override;

    // Methods
    virtual void SAL_CALL Close( const css::uno::Any &bSaveChanges,
                                 const css::uno::Any &aFileName,
                                 const css::uno::Any &bRouteWorkbook ) override;
    /// @throws css::uno::RuntimeException
    virtual void SAL_CALL Protect( const css::uno::Any &aPassword );
    virtual void SAL_CALL Unprotect( const css::uno::Any &aPassword ) override;
    virtual void SAL_CALL Save() override;
    virtual void SAL_CALL Activate() override;

    // XHelperInterface
    virtual OUString getServiceImplName() override;
    virtual css::uno::Sequence<OUString> getServiceNames() override;

    static OUString getNameFromModel( const css::uno::Reference< css::frame::XModel >& xModel );
};

#endif // INCLUDED_VBAHELPER_VBADOCUMENTBASE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
