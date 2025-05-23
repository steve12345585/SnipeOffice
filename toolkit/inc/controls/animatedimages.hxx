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

#include <toolkit/controls/unocontrolmodel.hxx>
#include <com/sun/star/awt/XAnimatedImages.hpp>
#include <cppuhelper/implbase1.hxx>

namespace com::sun::star::container { class XContainerListener; }
namespace com::sun::star::uno { class XComponentContext; }

namespace toolkit
{


    typedef ::cppu::AggImplInheritanceHelper1   <   UnoControlModel
                                                ,   css::awt::XAnimatedImages
                                                >   AnimatedImagesControlModel_Base;
    class AnimatedImagesControlModel : public AnimatedImagesControlModel_Base
    {
    public:
                                        AnimatedImagesControlModel( css::uno::Reference< css::uno::XComponentContext > const & i_factory );
                                        AnimatedImagesControlModel( const AnimatedImagesControlModel& i_copySource );

        virtual rtl::Reference<UnoControlModel> Clone() const override;

        // XPropertySet
        css::uno::Reference< css::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) override;

        // XPersistObject
        OUString SAL_CALL getServiceName() override;

        // XServiceInfo
        OUString SAL_CALL getImplementationName(  ) override;
        css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

        // XAnimatedImages
        virtual ::sal_Int32 SAL_CALL getStepTime() override;
        virtual void SAL_CALL setStepTime( ::sal_Int32 _steptime ) override;
        virtual sal_Bool SAL_CALL getAutoRepeat() override;
        virtual void SAL_CALL setAutoRepeat( sal_Bool _autorepeat ) override;
        virtual ::sal_Int16 SAL_CALL getScaleMode() override;
        virtual void SAL_CALL setScaleMode( ::sal_Int16 _scalemode ) override;
        virtual ::sal_Int32 SAL_CALL getImageSetCount(  ) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getImageSet( ::sal_Int32 i_index ) override;
        virtual void SAL_CALL insertImageSet( ::sal_Int32 i_index, const css::uno::Sequence< OUString >& i_imageURLs ) override;
        virtual void SAL_CALL replaceImageSet( ::sal_Int32 i_index, const css::uno::Sequence< OUString >& i_imageURLs ) override;
        virtual void SAL_CALL removeImageSet( ::sal_Int32 i_index ) override;

        // XAnimatedImages::XContainer
        virtual void SAL_CALL addContainerListener( const css::uno::Reference< css::container::XContainerListener >& i_listener ) override;
        virtual void SAL_CALL removeContainerListener( const css::uno::Reference< css::container::XContainerListener >& i_listener ) override;

    protected:
                                        virtual ~AnimatedImagesControlModel() override;

        css::uno::Any      ImplGetDefaultValue( sal_uInt16 nPropId ) const override;
        ::cppu::IPropertyArrayHelper& getInfoHelper() override;
        void setFastPropertyValue_NoBroadcast(
                    std::unique_lock<std::mutex>& rGuard,
                    sal_Int32 nHandle, const css::uno::Any& rValue ) override;

    private:
        comphelper::OInterfaceContainerHelper4<css::container::XContainerListener> maContainerListeners;
        std::vector< css::uno::Sequence< OUString > >    maImageSets;
    };


} // namespace toolkit


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
