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

#ifndef INCLUDED_XMLOFF_XMLNUME_HXX
#define INCLUDED_XMLOFF_XMLNUME_HXX

#include <rtl/ustring.hxx>
#include <xmloff/dllapi.h>

namespace com::sun::star {
    namespace style { class XStyle; }
    namespace container { class XIndexReplace; }
    namespace beans { struct PropertyValue; }
}

namespace com::sun::star::uno { template <class E> class Sequence; }
namespace com::sun::star::uno { template <class interface_type> class Reference; }

class SvXMLExport;

class XMLOFF_DLLPUBLIC SvxXMLNumRuleExport final
{
    SvXMLExport& m_rExport;
    // Boolean indicating, if properties for position-and-space-mode LABEL_ALIGNMENT
    // are exported or not. (#i89178#)
    // These properties have been introduced in ODF 1.2. Thus, its export have
    // to be suppressed on writing ODF 1.0 respectively ODF 1.1
    bool mbExportPositionAndSpaceModeLabelAlignment;

    SAL_DLLPRIVATE void exportLevelStyle(
            sal_Int32 nLevel,
            const css::uno::Sequence< css::beans::PropertyValue>& rProps,
            bool bOutline );

    SAL_DLLPRIVATE void exportStyle( const css::uno::Reference< css::style::XStyle >& rStyle );
    SAL_DLLPRIVATE void exportOutline();

    SvXMLExport& GetExport() { return m_rExport; }

public:

    SvxXMLNumRuleExport( SvXMLExport& rExport );

    // should be private but sw::StoredChapterNumberingExport needs it
    void exportLevelStyles(
            const css::uno::Reference< css::container::XIndexReplace > & xNumRule,
            bool bOutline=false );

    void exportStyles(bool bUsed, bool bExportChapterNumbering);
    void exportNumberingRule(
            const OUString& rName, bool bIsHidden,
            const css::uno::Reference< css::container::XIndexReplace > & xNumRule );
};

#endif // INCLUDED_XMLOFF_XMLNUME_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
