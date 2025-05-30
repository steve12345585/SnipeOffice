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

#ifndef INCLUDED_XMLOFF_XMLTEXTLISTAUTOSTYLEPOOL_HXX
#define INCLUDED_XMLOFF_XMLTEXTLISTAUTOSTYLEPOOL_HXX

#include <sal/config.h>
#include <xmloff/dllapi.h>
#include <sal/types.h>
#include <rtl/ustring.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <memory>
#include <set>

namespace com::sun::star::container { class XIndexReplace; }
namespace com::sun::star::ucb { class XAnyCompare; }


class XMLTextListAutoStylePool_Impl;
typedef std::set<OUString> XMLTextListAutoStylePoolNames_Impl;
class XMLTextListAutoStylePoolEntry_Impl;
class SvXMLExport;

class XMLOFF_DLLPUBLIC XMLTextListAutoStylePool
{
    SvXMLExport& m_rExport;

    OUString m_sPrefix;

    std::unique_ptr<XMLTextListAutoStylePool_Impl> m_pPool;
    XMLTextListAutoStylePoolNames_Impl m_aNames;
    sal_uInt32 m_nName;

    /** this is an optional NumRule compare component for applications where
        the NumRules don't have names */
    css::uno::Reference< css::ucb::XAnyCompare > mxNumRuleCompare;

    SAL_DLLPRIVATE sal_uInt32 Find( const XMLTextListAutoStylePoolEntry_Impl* pEntry )
        const;
public:

    XMLTextListAutoStylePool( SvXMLExport& rExport );
    ~XMLTextListAutoStylePool();

    void RegisterName( const OUString& rName );

    OUString Add(
            const css::uno::Reference< css::container::XIndexReplace > & rNumRules );

    OUString Find(
            const css::uno::Reference< css::container::XIndexReplace > & rNumRules ) const;
    OUString Find( const OUString& rInternalName ) const;

    void exportXML() const;
};


#endif // INCLUDED_XMLOFF_XMLTEXTLISTAUTOSTYLEPOOL_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
