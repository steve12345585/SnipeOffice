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

#include <xmloff/formlayerexport.hxx>
#include <xmloff/xmlexp.hxx>
#include "layerexport.hxx"
#include <osl/diagnose.h>
#include "officeforms.hxx"

namespace xmloff
{

    using namespace ::com::sun::star::uno;
    using namespace ::com::sun::star::awt;
    using namespace ::com::sun::star::beans;
    using namespace ::com::sun::star::drawing;

    //= OFormLayerXMLExport

    OFormLayerXMLExport::OFormLayerXMLExport(SvXMLExport& _rContext)
        :m_pImpl(new OFormLayerXMLExport_Impl(_rContext))
    {
    }

    OFormLayerXMLExport::~OFormLayerXMLExport()
    {
    }

    bool OFormLayerXMLExport::seekPage(const Reference< XDrawPage >& _rxDrawPage)
    {
        return m_pImpl->seekPage(_rxDrawPage);
    }

    OUString OFormLayerXMLExport::getControlId(const Reference< XPropertySet >& _rxControl)
    {
        return m_pImpl->getControlId(_rxControl);
    }

    OUString OFormLayerXMLExport::getControlNumberStyle( const Reference< XPropertySet >& _rxControl )
    {
        return m_pImpl->getControlNumberStyle(_rxControl);
    }

    void OFormLayerXMLExport::examineForms(const Reference< XDrawPage >& _rxDrawPage)
    {
        try
        {
            m_pImpl->examineForms(_rxDrawPage);
        }
        catch(Exception&)
        {
            OSL_FAIL("OFormLayerXMLExport::examine: could not examine the draw page!");
        }
    }

    void OFormLayerXMLExport::exportForms(const Reference< XDrawPage >& _rxDrawPage)
    {
        m_pImpl->exportForms(_rxDrawPage);
    }

    void OFormLayerXMLExport::exportXForms() const
    {
        m_pImpl->exportXForms();
    }

    bool OFormLayerXMLExport::pageContainsForms( const Reference< XDrawPage >& _rxDrawPage )
    {
        return OFormLayerXMLExport_Impl::pageContainsForms( _rxDrawPage );
    }

    bool OFormLayerXMLExport::documentContainsXForms() const
    {
        return m_pImpl->documentContainsXForms();
    }

    void OFormLayerXMLExport::exportAutoControlNumberStyles()
    {
        m_pImpl->exportAutoControlNumberStyles();
    }

    void OFormLayerXMLExport::exportAutoStyles()
    {
        m_pImpl->exportAutoStyles();
    }

    void OFormLayerXMLExport::excludeFromExport( const Reference< XControlModel >& _rxControl )
    {
        m_pImpl->excludeFromExport( _rxControl );
    }

    //= OOfficeFormsExport
    OOfficeFormsExport::OOfficeFormsExport( SvXMLExport& _rExp )
        :m_pImpl( new OFormsRootExport(_rExp) )
    {
    }

    OOfficeFormsExport::~OOfficeFormsExport()
    {
    }

}   // namespace xmloff

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
