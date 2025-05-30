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
#ifndef INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLTABLE_HXX
#define INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLTABLE_HXX

#include <xmloff/xmlictxt.hxx>
#include <com/sun/star/report/XSection.hpp>
#include <vector>

namespace rptxml
{
    class ORptFilter;
    class OXMLTable : public SvXMLImportContext
    {
    public:
        struct TCell
        {
            sal_Int32 nWidth;
            sal_Int32 nHeight;
            sal_Int32 nColSpan;
            sal_Int32 nRowSpan;
            bool  bAutoHeight;
            ::std::vector< css::uno::Reference< css::report::XReportComponent> > xElements;
            TCell() : nWidth(0),nHeight(0),nColSpan(1),nRowSpan(1),bAutoHeight(false){}
        };
    private:
        ::std::vector< ::std::vector<TCell> >                                               m_aGrid;
        ::std::vector<sal_Int32>                                                            m_aHeight;
        ::std::vector<bool>                                                                 m_aAutoHeight;
        ::std::vector<sal_Int32>                                                            m_aWidth;
        css::uno::Reference< css::report::XSection >                                        m_xSection;
        OUString                                                                            m_sStyleName;
        sal_Int32                                                                           m_nColSpan;
        sal_Int32                                                                           m_nRowSpan;
        sal_Int32                                                                           m_nRowIndex;
        sal_Int32                                                                           m_nColumnIndex;
        ORptFilter& GetOwnImport();

        OXMLTable(const OXMLTable&) = delete;
        void operator =(const OXMLTable&) = delete;
    public:

        OXMLTable( ORptFilter& rImport
                    ,const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList
                    ,css::uno::Reference< css::report::XSection > _xSection
                    );
        virtual ~OXMLTable() override;

        virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
                sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList ) override;

        virtual void SAL_CALL endFastElement(sal_Int32 nElement) override;

        void addHeight(sal_Int32 _nHeight) { m_aHeight.push_back(_nHeight); }
        void addAutoHeight(bool _bAutoHeight) { m_aAutoHeight.push_back(_bAutoHeight); }
        void addWidth(sal_Int32 _nWidth)     { m_aWidth.push_back( _nWidth ); }

        void setColumnSpanned(sal_Int32 _nColSpan)     { m_nColSpan = _nColSpan; }
        void setRowSpanned(   sal_Int32 _nRowSpan)     { m_nRowSpan = _nRowSpan; }

        void incrementRowIndex();
        void incrementColumnIndex()  { ++m_nColumnIndex; }

        void addCell(const css::uno::Reference< css::report::XReportComponent>& _xElement);

        const css::uno::Reference< css::report::XSection >& getSection() const { return m_xSection; }
    };

} // namespace rptxml


#endif // INCLUDED_REPORTDESIGN_SOURCE_FILTER_XML_XMLTABLE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
