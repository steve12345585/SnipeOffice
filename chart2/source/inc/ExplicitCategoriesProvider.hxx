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

#include <config_options.h>
#include <unotools/weakref.hxx>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/uno/Sequence.h>

#include <utility>
#include <vector>

namespace chart { class ChartModel; }
namespace com::sun::star::chart2::data { class XDataProvider; }
namespace com::sun::star::chart2::data { class XDataSequence; }
namespace com::sun::star::chart2::data { class XLabeledDataSequence; }
namespace com::sun::star::uno { class Any; }

namespace chart
{
class BaseCoordinateSystem;

struct ComplexCategory
{
    OUString Text;
    sal_Int32 Count;

    ComplexCategory( OUString aText, sal_Int32 nCount ) : Text(std::move( aText )), Count (nCount)
    {}
};

class SplitCategoriesProvider
{
public:
    virtual ~SplitCategoriesProvider();

    virtual sal_Int32 getLevelCount() const = 0;
    virtual css::uno::Sequence< OUString > getStringsForLevel( sal_Int32 nIndex ) const = 0;
};

class ExplicitCategoriesProvider final
{
public:
    ExplicitCategoriesProvider( const rtl::Reference< ::chart::BaseCoordinateSystem >& xCooSysModel
                       , ChartModel& rChartModel
                       );
    ~ExplicitCategoriesProvider();

    void init();

    css::uno::Reference< css::chart2::data::XDataSequence > getOriginalCategories();

    css::uno::Sequence< OUString > const & getSimpleCategories();
    const std::vector<ComplexCategory>* getCategoriesByLevel( sal_Int32 nLevel );

    static OUString getCategoryByIndex(
          const rtl::Reference< ::chart::BaseCoordinateSystem >& xCooSysModel
        , ChartModel& rModel
        , sal_Int32 nIndex );

    static css::uno::Sequence< OUString > getExplicitSimpleCategories(
            const SplitCategoriesProvider& rSplitCategoriesProvider );

    static void convertCategoryAnysToText( css::uno::Sequence< OUString >& rOutTexts
        , const css::uno::Sequence< css::uno::Any >& rInAnys
        , ChartModel& rModel );

    bool hasComplexCategories() const;
    sal_Int32 getCategoryLevelCount() const;

    const std::vector< css::uno::Reference< css::chart2::data::XLabeledDataSequence> >&
            getSplitCategoriesList() const { return m_aSplitCategoriesList;}

    bool isDateAxis();
    const std::vector< double >&  getDateCategories();

private:
    ExplicitCategoriesProvider(ExplicitCategoriesProvider const &) = delete;
    ExplicitCategoriesProvider& operator =(ExplicitCategoriesProvider const &) = delete;

    void implInitSplit();

    bool volatile m_bDirty;
    unotools::WeakReference< ::chart::BaseCoordinateSystem >   m_xCooSysModel;
    ChartModel& mrModel;
    css::uno::Reference< css::chart2::data::XLabeledDataSequence> m_xOriginalCategories;

    bool m_bIsExplicitCategoriesInited;
    css::uno::Sequence< OUString >  m_aExplicitCategories;
    std::vector< std::vector< ComplexCategory > >   m_aComplexCats;
    std::vector< css::uno::Reference< css::chart2::data::XLabeledDataSequence> > m_aSplitCategoriesList;

    bool m_bIsDateAxis;
    bool m_bIsAutoDate;
    std::vector< double >  m_aDateCategories;
};

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
