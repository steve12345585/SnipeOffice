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

#ifndef INCLUDED_OOX_DRAWINGML_CHART_DATASOURCECONVERTER_HXX
#define INCLUDED_OOX_DRAWINGML_CHART_DATASOURCECONVERTER_HXX

#include <drawingml/chart/converterbase.hxx>

namespace com::sun::star {
    namespace chart2::data { class XDataSequence; }
}

namespace oox::drawingml::chart {


struct DataSequenceModel;

class DataSequenceConverter final : public ConverterBase< DataSequenceModel >
{
public:
    explicit            DataSequenceConverter( const ConverterRoot& rParent, DataSequenceModel& rModel );
    virtual             ~DataSequenceConverter() override;

    /** Creates a data sequence object from the contained formula link. */
    css::uno::Reference< css::chart2::data::XDataSequence >
                        createDataSequence( const OUString& rRole );
};


struct DataSourceModel;

class DataSourceConverter final : public ConverterBase< DataSourceModel >
{
public:
    explicit            DataSourceConverter( const ConverterRoot& rParent, DataSourceModel& rModel );
    virtual             ~DataSourceConverter() override;

    /** Creates a data sequence object from the contained series data. */
    css::uno::Reference< css::chart2::data::XDataSequence >
                        createDataSequence( const OUString& rRole );
};


} // namespace oox::drawingml::chart

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
