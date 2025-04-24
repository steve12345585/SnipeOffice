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

#ifndef INCLUDED_XMLOFF_TABLE_XMLTABLEIMPORT_HXX
#define INCLUDED_XMLOFF_TABLE_XMLTABLEIMPORT_HXX

#include <com/sun/star/table/XColumnRowRange.hpp>

#include <xmloff/dllapi.h>
#include <xmloff/xmlictxt.hxx>
#include <salhelper/simplereferenceobject.hxx>
#include <xmloff/xmlimppr.hxx>
#include <xmloff/prhdlfac.hxx>

#include <rtl/ref.hxx>

#include <map>
#include <memory>

class SvXMLStyleContext;

typedef std::map< OUString, OUString > XMLTableTemplate;
// Not using a map here, as we want the templates to be
// inserted in the same order they were defined in the
// xml (at least for sd built in templates):
typedef std::vector< std::pair< OUString, std::shared_ptr< XMLTableTemplate > > > XMLTableTemplateMap;

class XMLOFF_DLLPUBLIC XMLTableImport final : public salhelper::SimpleReferenceObject
{
    friend class XMLTableImportContext;

public:
    XMLTableImport( SvXMLImport& rImport, const rtl::Reference< XMLPropertySetMapper >& xCellPropertySetMapper, const rtl::Reference< XMLPropertyHandlerFactory >& xFactoryRef );
    virtual ~XMLTableImport() override;

    SvXMLImportContext* CreateTableContext( css::uno::Reference< css::table::XColumnRowRange > const & xColumnRowRange );

    SvXMLStyleContext* CreateTableTemplateContext( sal_Int32 nElement, const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList );

    SvXMLImportPropertyMapper* GetCellImportPropertySetMapper() const { return mxCellImportPropertySetMapper.get(); }
    SvXMLImportPropertyMapper* GetRowImportPropertySetMapper() const { return mxRowImportPropertySetMapper.get(); }
    SvXMLImportPropertyMapper* GetColumnImportPropertySetMapper() const { return mxColumnImportPropertySetMapper.get(); }

    void addTableTemplate( const OUString& rsStyleName, XMLTableTemplate& xTableTemplate );
    /// Inserts all table templates.
    void finishStyles();

private:
    SvXMLImport&                                 mrImport;
    bool                                        mbWriter;
    std::unique_ptr< SvXMLImportPropertyMapper > mxCellImportPropertySetMapper;
    std::unique_ptr< SvXMLImportPropertyMapper > mxRowImportPropertySetMapper;
    std::unique_ptr< SvXMLImportPropertyMapper > mxColumnImportPropertySetMapper;

    XMLTableTemplateMap                         maTableTemplates;
};

#endif // INCLUDED_XMLOFF_TABLE_XMLTABLEIMPORT_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
