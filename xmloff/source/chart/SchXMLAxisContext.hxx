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

#include <xmloff/SchXMLImportHelper.hxx>
#include <xmloff/xmlictxt.hxx>

#include "transporttypes.hxx"

#include <com/sun/star/chart/XDiagram.hpp>

class SchXMLAxisContext : public SvXMLImportContext
{
public:
    SchXMLAxisContext( SchXMLImportHelper& rImpHelper,
                       SvXMLImport& rImport,
                       css::uno::Reference< css::chart::XDiagram > const & xDiagram,
                       std::vector< SchXMLAxis >& aAxes,
                       OUString& rCategoriesAddress,
                       bool bAddMissingXAxisForNetCharts,
                       bool bAdaptWrongPercentScaleValues,
                       bool bAdaptXAxisOrientationForOld2DBarCharts,
                       bool& rbAxisPositionAttributeImported );
    virtual ~SchXMLAxisContext() override;

    virtual void SAL_CALL startFastElement( sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;
    virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;
    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& AttrList ) override;

    static void CorrectAxisPositions( const css::uno::Reference< css::chart2::XChartDocument >& xNewDoc,
                          std::u16string_view rChartTypeServiceName,
                          std::u16string_view rODFVersionOfFile,
                          bool bAxisPositionAttributeImported );

private:
    SchXMLImportHelper& m_rImportHelper;
    css::uno::Reference< css::chart::XDiagram > m_xDiagram;
    SchXMLAxis m_aCurrentAxis;
    std::vector< SchXMLAxis >& m_rAxes;
    css::uno::Reference< css::beans::XPropertySet > m_xAxisProps;
    OUString m_aAutoStyleName;
    OUString& m_rCategoriesAddress;
    sal_Int32 m_nAxisType;//css::chart::ChartAxisType
    bool m_bAxisTypeImported;
    bool m_bDateScaleImported;
    bool m_bAddMissingXAxisForNetCharts; //to correct errors from older versions
    bool m_bAdaptWrongPercentScaleValues; //to correct errors from older versions
    bool m_bAdaptXAxisOrientationForOld2DBarCharts; //to correct different behaviour from older versions
    bool& m_rbAxisPositionAttributeImported;

    css::uno::Reference< css::drawing::XShape > getTitleShape() const;
    void CreateGrid( const OUString& sAutoStyleName, bool bIsMajor );
    void CreateAxis();
    void SetAxisTitle();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
