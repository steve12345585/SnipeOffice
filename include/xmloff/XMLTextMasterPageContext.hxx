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
#ifndef INCLUDED_XMLOFF_XMLTEXTMASTERPAGECONTEXT_HXX
#define INCLUDED_XMLOFF_XMLTEXTMASTERPAGECONTEXT_HXX

#include <sal/config.h>
#include <xmloff/dllapi.h>
#include <sal/types.h>

#include <xmloff/xmlstyle.hxx>

struct ContextID_Index_Pair;

namespace com::sun::star {
    namespace style { class XStyle; }
}

class XMLOFF_DLLPUBLIC XMLTextMasterPageContext : public SvXMLStyleContext
{
    OUString       m_sFollow;
    OUString       m_sPageMasterName;
    OUString m_sDrawingPageStyle;

    css::uno::Reference < css::style::XStyle > m_xStyle;

    bool m_bInsertHeader;
    bool m_bInsertFooter;
    bool m_bInsertHeaderLeft;
    bool m_bInsertFooterLeft;
    bool m_bInsertHeaderFirst;
    bool m_bInsertFooterFirst;
    bool m_bHeaderInserted;
    bool m_bFooterInserted;

    SAL_DLLPRIVATE css::uno::Reference< css::style::XStyle > Create();
protected:
    const css::uno::Reference< css::style::XStyle >& GetStyle() const { return m_xStyle; }
public:


    XMLTextMasterPageContext( SvXMLImport& rImport, sal_Int32 nElement,
            const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList,
            bool bOverwrite );
    virtual ~XMLTextMasterPageContext() override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext(
        sal_Int32 nElement,
        const css::uno::Reference< css::xml::sax::XFastAttributeList >& AttrList ) override final;

    virtual SvXMLImportContext *CreateHeaderFooterContext(
            sal_Int32 nElement,
            const css::uno::Reference< css::xml::sax::XFastAttributeList > & xAttrList,
            const bool bFooter,
            const bool bLeft,
            const bool bFirst );

    virtual void Finish( bool bOverwrite ) override;
};

XMLOFF_DLLPUBLIC extern ContextID_Index_Pair const g_MasterPageContextIDs[];
XMLOFF_DLLPUBLIC extern XmlStyleFamily const g_MasterPageFamilies[];

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
