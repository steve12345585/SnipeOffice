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

#include <sal/config.h>

#include <com/sun/star/beans/PropertyValue.hpp>

#include <xmloff/formlayerimport.hxx>
#include "layerimport.hxx"

namespace xmloff
{

    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::drawing;
    using namespace ::com::sun::star;

    OFormLayerXMLImport::OFormLayerXMLImport(SvXMLImport& _rImporter)
        : m_pImpl( new OFormLayerXMLImport_Impl(_rImporter) )
    {
    }

    OFormLayerXMLImport::~OFormLayerXMLImport()
    {
    }

    void OFormLayerXMLImport::setAutoStyleContext(SvXMLStylesContext* _pNewContext)
    {
        m_pImpl->setAutoStyleContext(_pNewContext);
    }

    void OFormLayerXMLImport::startPage(const Reference< XDrawPage >& _rxDrawPage)
    {
        m_pImpl->startPage(_rxDrawPage);
    }

    void OFormLayerXMLImport::endPage()
    {
        m_pImpl->endPage();
    }

    Reference< XPropertySet > OFormLayerXMLImport::lookupControl(const OUString& _rId)
    {
        return m_pImpl->lookupControlId(_rId);
    }

    SvXMLImportContext* OFormLayerXMLImport::createOfficeFormsContext(
        SvXMLImport& _rImport)
    {
        return OFormLayerXMLImport_Impl::createOfficeFormsContext(_rImport);
    }

    SvXMLImportContext* OFormLayerXMLImport::createContext(sal_Int32 nElement,
        const Reference< xml::sax::XFastAttributeList >& _rxAttribs)
    {
        return m_pImpl->createContext(nElement, _rxAttribs);
    }

    void OFormLayerXMLImport::applyControlNumberStyle(const Reference< XPropertySet >& _rxControlModel, const OUString& _rControlNumberStyleName)
    {
        m_pImpl->applyControlNumberStyle(_rxControlModel, _rControlNumberStyleName);
    }

    void OFormLayerXMLImport::documentDone( )
    {
        m_pImpl->documentDone( );
    }

}   // namespace xmloff

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
