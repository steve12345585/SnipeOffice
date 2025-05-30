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

#include <comphelper/broadcasthelper.hxx>
#include <comphelper/propagg.hxx>
#include <comphelper/proparrhlp.hxx>
#include <comphelper/propertycontainer.hxx>
#include <cppuhelper/compbase2.hxx>
#include <com/sun/star/util/XCloneable.hpp>
#include <com/sun/star/script/XScriptEventsSupplier.hpp>
#include <comphelper/IdPropArrayHelper.hxx>
#include <rtl/ref.hxx>

namespace com::sun::star::resource { class XStringResourceResolver; }

namespace com::sun::star {
    namespace uno {
        class XComponentContext;
    }
}
namespace toolkit { class ScriptEventContainer; }

// namespace toolkit
// {


    //= OGeometryControlModel_Base

    typedef ::cppu::WeakAggComponentImplHelper2 <   css::util::XCloneable
                                                ,   css::script::XScriptEventsSupplier
                                                >   OGCM_Base;
    class OGeometryControlModel_Base
        :public ::comphelper::OMutexAndBroadcastHelper
        ,public ::comphelper::OPropertySetAggregationHelper
        ,public ::comphelper::OPropertyContainer
        ,public OGCM_Base
    {
    protected:
        css::uno::Reference< css::uno::XAggregation >
                    m_xAggregate;
        rtl::Reference< toolkit::ScriptEventContainer >
                    mxEventContainer;

        // <properties>
        sal_Int32       m_nPosX;
        sal_Int32       m_nPosY;
        sal_Int32       m_nWidth;
        sal_Int32       m_nHeight;
        OUString m_aName;
        sal_Int16       m_nTabIndex;
        sal_Int32       m_nStep;
        OUString m_aTag;
        css::uno::Reference< css::resource::XStringResourceResolver > m_xStrResolver;
        // </properties>

        bool        m_bCloneable;

    protected:
        static css::uno::Any   ImplGetDefaultValueByHandle(sal_Int32 nHandle);
        css::uno::Any          ImplGetPropertyValueByHandle(sal_Int32 nHandle) const;
        void                   ImplSetPropertyValueByHandle(sal_Int32 nHandle, const css::uno::Any& aValue);

    protected:
        /**
            @param _pAggregateInstance
                the object to be aggregated. The refcount of the instance given MUST be 0!
        */
        OGeometryControlModel_Base(css::uno::XAggregation* _pAggregateInstance);

        /**
            @param _rxAggregateInstance
                is the object to be aggregated. Must be acquired exactly once (by the reference object given).<br/>
                Will be reset to NULL upon leaving
        */
        OGeometryControlModel_Base(css::uno::Reference< css::util::XCloneable >& _rxAggregateInstance);

        /** releases the aggregation
            <p>Can be used if in a derived class, an exception has to be thrown after this base class here already
            did the aggregation</p>
        */
        void releaseAggregation();

    protected:
        virtual ~OGeometryControlModel_Base() override;

        // XAggregation
        css::uno::Any SAL_CALL queryAggregation( const css::uno::Type& _aType ) override;

    public:
        // XInterface
        virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type& aType ) override;
        virtual void SAL_CALL acquire(  ) noexcept override;
        virtual void SAL_CALL release(  ) noexcept override;

    protected:
        // XTypeProvider
        virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes(  ) override;

        // OPropertySetHelper overridables
        virtual sal_Bool SAL_CALL convertFastPropertyValue(
                css::uno::Any& _rConvertedValue, css::uno::Any& _rOldValue,
                sal_Int32 _nHandle, const css::uno::Any& _rValue ) override;

        virtual void SAL_CALL setFastPropertyValue_NoBroadcast(
                sal_Int32 _nHandle, const css::uno::Any& _rValue) override;

        using comphelper::OPropertySetAggregationHelper::getFastPropertyValue;
        virtual void SAL_CALL getFastPropertyValue(
            css::uno::Any& _rValue, sal_Int32 _nHandle) const override;

        // OPropertyStateHelper overridables
        virtual css::beans::PropertyState  getPropertyStateByHandle(sal_Int32 nHandle) override;
        virtual void                                    setPropertyToDefaultByHandle(sal_Int32 nHandle) override;
        virtual css::uno::Any              getPropertyDefaultByHandle(sal_Int32 nHandle) const override;

        // XPropertySet
        virtual css::uno::Reference< css::beans::XPropertySetInfo> SAL_CALL getPropertySetInfo() override;

        // OPropertySetAggregationHelper overridables
        using OPropertySetAggregationHelper::getInfoHelper;

        // XCloneable
        virtual css::uno::Reference< css::util::XCloneable > SAL_CALL createClone(  ) override;

        //XScriptEventsSupplier
        virtual css::uno::Reference< css::container::XNameContainer >
            SAL_CALL getEvents(  ) override;

        // XCloneable implementation - to be overwritten
        virtual rtl::Reference<OGeometryControlModel_Base> createClone_Impl(
            css::uno::Reference< css::util::XCloneable >& _rxAggregateInstance) = 0;

        // XComponent
        using comphelper::OPropertySetAggregationHelper::disposing;
        virtual void SAL_CALL disposing() override;

    private:
        void registerProperties();
    };


    //= OTemplateInstanceDisambiguation

    template <class CONTROLMODEL>
    class OTemplateInstanceDisambiguation
    {
    };


    //= OGeometryControlModel

    /*  example for usage:
            Reference< XAggregation > xIFace = new ::toolkit::OGeometryControlModel< UnoControlButtonModel > ();
    */
    template <class CONTROLMODEL>
    class OGeometryControlModel final
        :public OGeometryControlModel_Base
        ,public ::comphelper::OAggregationArrayUsageHelper< OTemplateInstanceDisambiguation< CONTROLMODEL > >
    {
    public:
        OGeometryControlModel( const css::uno::Reference< css::uno::XComponentContext >& i_factory );

    private:
        OGeometryControlModel(css::uno::Reference< css::util::XCloneable >& _rxAggregateInstance);

        // OAggregationArrayUsageHelper overridables
        virtual void fillProperties(
            css::uno::Sequence< css::beans::Property >& _rProps,
            css::uno::Sequence< css::beans::Property >& _rAggregateProps
            ) const override;

        // OPropertySetAggregationHelper overridables
        virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper() override;

        // OGeometryControlModel_Base
        virtual rtl::Reference<OGeometryControlModel_Base> createClone_Impl(
            css::uno::Reference< css::util::XCloneable >& _rxAggregateInstance) override;

        // XTypeProvider
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId(  ) override;
    };


    //= OCommonGeometryControlModel

    /** allows to extend an arbitrary com.sun.star.awt::UnoControlModel with geometry
        information.
    */
    class OCommonGeometryControlModel final
        :public OGeometryControlModel_Base
        ,public ::comphelper::OIdPropertyArrayUsageHelper< OCommonGeometryControlModel >
    {
    private:
        OUString m_sServiceSpecifier;        // the service specifier of our aggregate
        sal_Int32       m_nPropertyMapId;           // our unique property info id, used to look up in s_aAggregateProperties

    public:
        /** instantiate the model

            @param _rxAgg
                the instance to aggregate. Must support the com.sun.star.awt::UnoControlModel
                (this is not checked here)
        */
        OCommonGeometryControlModel(
                    css::uno::Reference< css::util::XCloneable >& _rxAgg,
            OUString _aServiceSpecifier
        );

        // OIdPropertyArrayUsageHelper overridables
        virtual ::cppu::IPropertyArrayHelper* createArrayHelper(sal_Int32 nId) const override;

        // OPropertySetAggregationHelper overridables
        virtual ::cppu::IPropertyArrayHelper& SAL_CALL getInfoHelper() override;

        // OGeometryControlModel_Base
        virtual rtl::Reference<OGeometryControlModel_Base> createClone_Impl(
            css::uno::Reference< css::util::XCloneable >& _rxAggregateInstance) override;

        // XTypeProvider
        virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId(  ) override;

    private:
        virtual void SAL_CALL setFastPropertyValue_NoBroadcast(
                sal_Int32 _nHandle, const css::uno::Any& _rValue) override;
    };

#include <controls/geometrycontrolmodel_impl.hxx>


// }    // namespace toolkit


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
