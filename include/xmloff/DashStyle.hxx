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

#ifndef INCLUDED_XMLOFF_DASHSTYLE_HXX
#define INCLUDED_XMLOFF_DASHSTYLE_HXX

#include <rtl/ustring.hxx>
#include <xmloff/dllapi.h>

class SvXMLImport;
class SvXMLExport;
namespace com::sun::star {
    namespace uno { template<class A> class Reference; }
    namespace xml::sax { class XFastAttributeList; }
    namespace uno { class Any; }
}


class XMLOFF_DLLPUBLIC XMLDashStyleImport
{
    SvXMLImport& m_rImport;

public:
    XMLDashStyleImport( SvXMLImport& rImport );

    void importXML(
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& xAttrList,
        css::uno::Any& rValue,
        OUString& rStrName );
};


class XMLOFF_DLLPUBLIC XMLDashStyleExport
{
    SvXMLExport& m_rExport;

public:
    XMLDashStyleExport( SvXMLExport& rExport );

    void exportXML( const OUString& rStrName,
                        const css::uno::Any& rValue );
};

#endif // INCLUDED_XMLOFF_DASHSTYLE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
