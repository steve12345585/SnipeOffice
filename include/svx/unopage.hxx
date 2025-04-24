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
#ifndef INCLUDED_SVX_UNOPAGE_HXX
#define INCLUDED_SVX_UNOPAGE_HXX

#include <com/sun/star/lang/XComponent.hpp>
#include <cppuhelper/interfacecontainer.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/drawing/XDrawPage.hpp>
#include <com/sun/star/drawing/XShapes2.hpp>
#include <com/sun/star/drawing/XShapes3.hpp>
#include <com/sun/star/drawing/XShapeGrouper.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/form/XFormsSupplier2.hpp>
#include <svx/svxdllapi.h>
#include <svx/svdobjkind.hxx>
#include <rtl/ref.hxx>

#include <cppuhelper/implbase.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/compbase.hxx>
#include <comphelper/interfacecontainer4.hxx>

#include <memory>

class SdrPage;
class SdrModel;
class SdrView;
class SdrPageView;
class SdrObject;
class SvxShape;
enum class SdrInventor : sal_uInt32;

class SVXCORE_DLLPUBLIC SvxDrawPage :
                                    public ::comphelper::WeakImplHelper< css::drawing::XDrawPage,
                                               css::drawing::XShapeGrouper,
                                               css::drawing::XShapes2,
                                               css::drawing::XShapes3,
                                               css::lang::XServiceInfo,
                                               css::lang::XUnoTunnel,
                                               css::lang::XComponent,
                                               css::form::XFormsSupplier2>

{
 protected:
    comphelper::OInterfaceContainerHelper4<css::lang::XEventListener> maEventListeners;
    SdrPage*        mpPage;     // TTTT should be reference
    SdrModel*       mpModel;    // TTTT probably not needed -> use from SdrPage
    std::unique_ptr<SdrView> mpView;

    void    SelectObjectsInView( const css::uno::Reference< css::drawing::XShapes >& aShapes, SdrPageView*   pPageView ) noexcept;
    void    SelectObjectInView( const css::uno::Reference< css::drawing::XShape >& xShape, SdrPageView*  pPageView ) noexcept;

    virtual void disposing() noexcept;

 public:
    SvxDrawPage(SdrPage* pPage);
    virtual ~SvxDrawPage() noexcept override;

    // Internals
    SdrPage* GetSdrPage() const { return mpPage; }

    // Creation of a SdrObject and insertion into the SdrPage
    rtl::Reference<SdrObject> CreateSdrObject( const css::uno::Reference< css::drawing::XShape >& xShape, bool bBeginning = false ) noexcept;

    // Determine Type and Inventor
    static void GetTypeAndInventor( SdrObjKind& rType, SdrInventor& rInventor, const OUString& aName ) noexcept;

    // Creating a SdrObject using it's Description.
    // Can be used by derived classes to support their own Shapes (e.g. Controls).
    /// @throws css::uno::RuntimeException
    virtual rtl::Reference<SdrObject> CreateSdrObject_( const css::uno::Reference< css::drawing::XShape >& xShape );

    /// @throws css::uno::RuntimeException
    static rtl::Reference<SvxShape> CreateShapeByTypeAndInventor( SdrObjKind nType, SdrInventor nInventor, SdrObject *pObj, SvxDrawPage *pPage = nullptr, OUString const & referer = OUString() );

    // The following method is called if a SvxShape object is to be created.
    // Derived classes can create a derivation or an SvxShape aggregating object.
    /// @throws css::uno::RuntimeException
    virtual css::uno::Reference< css::drawing::XShape > CreateShape( SdrObject *pObj ) const;

    UNO3_GETIMPLEMENTATION_DECL( SvxDrawPage )

    // XShapes
    virtual void SAL_CALL add( const css::uno::Reference< css::drawing::XShape >& xShape ) override;
    virtual void SAL_CALL remove( const css::uno::Reference< css::drawing::XShape >& xShape ) override;

    // XShapes2
    virtual void SAL_CALL addTop( const css::uno::Reference< css::drawing::XShape >& xShape ) override;
    virtual void SAL_CALL addBottom( const css::uno::Reference< css::drawing::XShape >& xShape ) override;

    // XShapes3
    virtual void SAL_CALL sort( const css::uno::Sequence< sal_Int32 >& sortOrder ) override;

    // XElementAccess
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual sal_Bool SAL_CALL hasElements() override;

    // XIndexAccess
    virtual sal_Int32 SAL_CALL getCount() override ;
    virtual css::uno::Any SAL_CALL getByIndex( sal_Int32 Index ) override;

    // XShapeGrouper
    virtual css::uno::Reference< css::drawing::XShapeGroup > SAL_CALL group( const css::uno::Reference< css::drawing::XShapes >& xShapes ) override;
    virtual void SAL_CALL ungroup( const css::uno::Reference< css::drawing::XShapeGroup >& aGroup ) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XComponent
    virtual void SAL_CALL dispose() override;
    virtual void SAL_CALL addEventListener( const css::uno::Reference< css::lang::XEventListener >& aListener ) override;
    virtual void SAL_CALL removeEventListener( const css::uno::Reference< css::lang::XEventListener >& aListener ) override;

    // XFormsSupplier
    virtual css::uno::Reference< css::container::XNameContainer > SAL_CALL getForms() override;

    // XFormsSupplier2
    virtual sal_Bool SAL_CALL hasForms() override;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
