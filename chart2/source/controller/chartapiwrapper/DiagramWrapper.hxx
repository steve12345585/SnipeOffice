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

#include <WrappedPropertySet.hxx>

#include <comphelper/interfacecontainer4.hxx>
#include <com/sun/star/chart/XDiagramPositioning.hpp>
#include <com/sun/star/chart2/XDiagramProvider.hpp>
#include <com/sun/star/chart/XDiagram.hpp>
#include <com/sun/star/chart/XAxisSupplier.hpp>
#include <com/sun/star/chart/XAxisZSupplier.hpp>
#include <com/sun/star/chart/XTwoAxisXSupplier.hpp>
#include <com/sun/star/chart/XTwoAxisYSupplier.hpp>
#include <com/sun/star/chart/XStatisticDisplay.hpp>
#include <com/sun/star/chart/X3DDisplay.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/chart/XSecondAxisTitleSupplier.hpp>
#include <com/sun/star/chart/X3DDefaultSetter.hpp>
#include <rtl/ref.hxx>
#include <memory>

namespace com::sun::star::chart2 { class XDiagram; }
namespace com::sun::star::lang { class XEventListener; }
namespace chart { class Diagram; }

namespace chart::wrapper
{
class AxisWrapper;
class WallFloorWrapper;
class MinMaxLineWrapper;
class UpDownBarWrapper;
class Chart2ModelContact;

class DiagramWrapper : public cppu::ImplInheritanceHelper<
                      WrappedPropertySet
                     , css::chart::XDiagram
                     , css::chart::XAxisSupplier
                     , css::chart::XAxisZSupplier
                     , css::chart::XTwoAxisXSupplier   //  : XAxisXSupplier
                     , css::chart::XTwoAxisYSupplier   //  : XAxisYSupplier
                     , css::chart::XStatisticDisplay
                     , css::chart::X3DDisplay
                     , css::chart::X3DDefaultSetter
                     , css::lang::XServiceInfo
                     , css::lang::XComponent
                     , css::chart::XDiagramPositioning
                     , css::chart2::XDiagramProvider
                     , css::chart::XSecondAxisTitleSupplier
                    >
{
public:
    explicit DiagramWrapper(std::shared_ptr<Chart2ModelContact> spChart2ModelContact);
    virtual ~DiagramWrapper() override;

    /// XServiceInfo declarations
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

    // ____ XComponent ____
    virtual void SAL_CALL dispose() override;
    virtual void SAL_CALL addEventListener( const css::uno::Reference<
                                            css::lang::XEventListener >& xListener ) override;
    virtual void SAL_CALL removeEventListener( const css::uno::Reference<
                                               css::lang::XEventListener >& aListener ) override;

    // ____ XDiagram ____
    virtual OUString SAL_CALL getDiagramType() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getDataRowProperties( sal_Int32 nRow ) override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getDataPointProperties( sal_Int32 nCol, sal_Int32 nRow ) override;

    // ____ XShape (base of XDiagram) ____
    virtual css::awt::Point SAL_CALL getPosition() override;
    virtual void SAL_CALL setPosition( const css::awt::Point& aPosition ) override;
    virtual css::awt::Size SAL_CALL getSize() override;
    virtual void SAL_CALL setSize( const css::awt::Size& aSize ) override;

    // ____ XShapeDescriptor (base of XShape) ____
    virtual OUString SAL_CALL getShapeType() override;

    // ____ XAxisSupplier ____
    virtual css::uno::Reference<
        css::chart::XAxis > SAL_CALL getAxis( sal_Int32 nDimensionIndex ) override;
    virtual css::uno::Reference<
        css::chart::XAxis > SAL_CALL getSecondaryAxis( sal_Int32 nDimensionIndex ) override;

    // ____ XAxisZSupplier ____
    virtual css::uno::Reference<
        css::drawing::XShape > SAL_CALL getZAxisTitle() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getZMainGrid() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getZHelpGrid() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getZAxis() override;

    // ____ XTwoAxisXSupplier ____
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getSecondaryXAxis() override;

    // ____ XAxisXSupplier (base of XTwoAxisXSupplier) ____
    virtual css::uno::Reference<
        css::drawing::XShape > SAL_CALL getXAxisTitle() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getXAxis() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getXMainGrid() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getXHelpGrid() override;

    // ____ XTwoAxisYSupplier ____
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getSecondaryYAxis() override;

    // ____ XAxisYSupplier (base of XTwoAxisYSupplier) ____
    virtual css::uno::Reference<
        css::drawing::XShape > SAL_CALL getYAxisTitle() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getYAxis() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getYHelpGrid() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getYMainGrid() override;

   // ____ XSecondAxisTitleSupplier ____
    virtual css::uno::Reference<
        css::drawing::XShape > SAL_CALL getSecondXAxisTitle() override;
    virtual css::uno::Reference<
        css::drawing::XShape > SAL_CALL getSecondYAxisTitle() override;

    // ____ XStatisticDisplay ____
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getUpBar() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getDownBar() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getMinMaxLine() override;

    // ____ X3DDisplay ____
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getWall() override;
    virtual css::uno::Reference<
        css::beans::XPropertySet > SAL_CALL getFloor() override;

    // ____ X3DDefaultSetter ____
    virtual void SAL_CALL set3DSettingsToDefault() override;
    virtual void SAL_CALL setDefaultRotation() override;
    virtual void SAL_CALL setDefaultIllumination() override;

    // ____ XDiagramPositioning ____
    virtual void SAL_CALL setAutomaticDiagramPositioning(  ) override;
    virtual sal_Bool SAL_CALL isAutomaticDiagramPositioning(  ) override;
    virtual void SAL_CALL setDiagramPositionExcludingAxes( const css::awt::Rectangle& PositionRect ) override;
    virtual sal_Bool SAL_CALL isExcludingDiagramPositioning(  ) override;
    virtual css::awt::Rectangle SAL_CALL calculateDiagramPositionExcludingAxes(  ) override;
    virtual void SAL_CALL setDiagramPositionIncludingAxes( const css::awt::Rectangle& PositionRect ) override;
    virtual css::awt::Rectangle SAL_CALL calculateDiagramPositionIncludingAxes(  ) override;
    virtual void SAL_CALL setDiagramPositionIncludingAxesAndAxisTitles( const css::awt::Rectangle& PositionRect ) override;
    virtual css::awt::Rectangle SAL_CALL calculateDiagramPositionIncludingAxesAndAxisTitles(  ) override;

    // ____ XDiagramProvider ____
    virtual css::uno::Reference< css::chart2::XDiagram > SAL_CALL getDiagram() override;
    virtual void SAL_CALL setDiagram( const css::uno::Reference< css::chart2::XDiagram >& xDiagram ) override;

    rtl::Reference< ::chart::Diagram > getUnderlyingDiagram();

protected:
    // ____ WrappedPropertySet ____
    virtual const css::uno::Sequence< css::beans::Property >& getPropertySequence() override;
    virtual std::vector< std::unique_ptr<WrappedProperty> > createWrappedProperties() override;
    virtual css::uno::Reference< css::beans::XPropertySet > getInnerPropertySet() override;

private:
    std::shared_ptr< Chart2ModelContact >   m_spChart2ModelContact;
    ::comphelper::OInterfaceContainerHelper4<css::lang::XEventListener> m_aEventListenerContainer;

    rtl::Reference< AxisWrapper >        m_xXAxis;
    rtl::Reference< AxisWrapper >        m_xYAxis;
    rtl::Reference< AxisWrapper >        m_xZAxis;
    rtl::Reference< AxisWrapper >        m_xSecondXAxis;
    rtl::Reference< AxisWrapper >        m_xSecondYAxis;
    rtl::Reference< WallFloorWrapper > m_xWall;
    rtl::Reference< WallFloorWrapper > m_xFloor;
    rtl::Reference< MinMaxLineWrapper > m_xMinMaxLineWrapper;
    rtl::Reference< UpDownBarWrapper > m_xUpBarWrapper;
    rtl::Reference< UpDownBarWrapper > m_xDownBarWrapper;
};

} //  namespace chart::wrapper

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
