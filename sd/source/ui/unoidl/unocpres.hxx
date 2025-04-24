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

#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/container/XNamed.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <comphelper/interfacecontainer4.hxx>
#include <osl/mutex.hxx>

#include <cppuhelper/implbase.hxx>
#include <comphelper/servicehelper.hxx>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>

#include <unomodel.hxx>
#include <drawdoc.hxx>


class SdCustomShow;

class SdXCustomPresentation :   public ::cppu::WeakImplHelper< css::container::XIndexContainer,
                                                                css::container::XNamed,
                                                                css::lang::XComponent,
                                                                css::lang::XServiceInfo >
{
private:
    SdCustomShow*       mpSdCustomShow;
    SdXImpressDocument* mpModel;

    // for xComponent
    std::mutex aDisposeContainerMutex;
    ::comphelper::OInterfaceContainerHelper4<css::lang::XEventListener> aDisposeListeners;
    bool bDisposing;

public:
    SdXCustomPresentation() noexcept;
    explicit SdXCustomPresentation( SdCustomShow* mpSdCustomShow ) noexcept;
    virtual ~SdXCustomPresentation() noexcept override;

    // internal
    SdCustomShow* GetSdCustomShow() const noexcept { return mpSdCustomShow; }
    void SetSdCustomShow( SdCustomShow* pShow ) noexcept { mpSdCustomShow = pShow; }
    SdXImpressDocument* GetModel() const noexcept { return mpModel; }

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XIndexContainer
    virtual void SAL_CALL insertByIndex( sal_Int32 Index, const css::uno::Any& Element ) override;
    virtual void SAL_CALL removeByIndex( sal_Int32 Index ) override;

    // XIndexReplace
    virtual void SAL_CALL replaceByIndex( sal_Int32 Index, const css::uno::Any& Element ) override;

    // XElementAccess
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual sal_Bool SAL_CALL hasElements() override;

    // XIndexAccess
    virtual sal_Int32 SAL_CALL getCount() override ;
    virtual css::uno::Any SAL_CALL getByIndex( sal_Int32 Index ) override;

    // XNamed
    virtual OUString SAL_CALL getName(  ) override;
    virtual void SAL_CALL setName( const OUString& aName ) override;

    // XComponent
    virtual void SAL_CALL dispose(  ) override;
    virtual void SAL_CALL addEventListener( const css::uno::Reference< css::lang::XEventListener >& xListener ) override;
    virtual void SAL_CALL removeEventListener( const css::uno::Reference< css::lang::XEventListener >& aListener ) override;
};

class SdXCustomPresentationAccess final : public ::cppu::WeakImplHelper< css::container::XNameContainer,
                                                                    css::lang::XSingleServiceFactory,
                                                                    css::lang::XServiceInfo >
{
private:
    SdXImpressDocument& mrModel;

    // internal
    inline SdCustomShowList* GetCustomShowList() const noexcept;
    SdCustomShow * getSdCustomShow( std::u16string_view Name ) const noexcept;

public:
    explicit SdXCustomPresentationAccess(SdXImpressDocument& rMyModel) noexcept;
    virtual ~SdXCustomPresentationAccess() noexcept override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // XSingleServiceFactory
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstance(  ) override;
    virtual css::uno::Reference< css::uno::XInterface > SAL_CALL createInstanceWithArguments( const css::uno::Sequence< css::uno::Any >& aArguments ) override;

    // XNameContainer
    virtual void SAL_CALL insertByName( const OUString& aName, const css::uno::Any& aElement ) override;
    virtual void SAL_CALL removeByName( const OUString& Name ) override;

    // XNameReplace
    virtual void SAL_CALL replaceByName( const OUString& aName, const css::uno::Any& aElement ) override;

    // XNameAccess
    virtual css::uno::Any SAL_CALL getByName( const OUString& aName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getElementNames() override;
    virtual sal_Bool SAL_CALL hasByName( const OUString& aName ) override;

    // XElementAccess
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual sal_Bool SAL_CALL hasElements() override;
};

inline SdCustomShowList* SdXCustomPresentationAccess::GetCustomShowList() const noexcept
{
    if(mrModel.GetDoc())
        return mrModel.GetDoc()->GetCustomShowList();
    else
        return nullptr;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
