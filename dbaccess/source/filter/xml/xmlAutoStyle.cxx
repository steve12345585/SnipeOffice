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

#include "xmlAutoStyle.hxx"
#include "xmlHelper.hxx"
#include "xmlExport.hxx"
#include <xmloff/families.hxx>
namespace dbaxml
{
    using namespace ::com::sun::star::uno;

void OXMLAutoStylePoolP::exportStyleAttributes(
            comphelper::AttributeList& rAttrList,
            XmlStyleFamily nFamily,
            const std::vector< XMLPropertyState >& rProperties,
            const SvXMLExportPropertyMapper& rPropExp
            , const SvXMLUnitConverter& rUnitConverter,
            const SvXMLNamespaceMap& rNamespaceMap
            ) const
{
    SvXMLAutoStylePoolP::exportStyleAttributes( rAttrList, nFamily, rProperties, rPropExp, rUnitConverter, rNamespaceMap );
    if ( nFamily != XmlStyleFamily::TABLE_COLUMN )
        return;

    rtl::Reference< XMLPropertySetMapper > aPropMapper = rODBExport.GetColumnStylesPropertySetMapper();
    for (auto const& property : rProperties)
    {
        sal_Int16 nContextID = aPropMapper->GetEntryContextId(property.mnIndex);
        switch (nContextID)
        {
            case CTF_DB_NUMBERFORMAT :
            {
                sal_Int32 nNumberFormat = 0;
                if ( property.maValue >>= nNumberFormat )
                {
                    OUString sAttrValue = rODBExport.getDataStyleName(nNumberFormat);
                    if ( !sAttrValue.isEmpty() )
                    {
                        GetExport().AddAttribute(
                            aPropMapper->GetEntryNameSpace(property.mnIndex),
                            aPropMapper->GetEntryXMLName(property.mnIndex),
                            sAttrValue );
                    }
                }
                break;
            }
        }
    }
}

OXMLAutoStylePoolP::OXMLAutoStylePoolP(ODBExport& rTempODBExport):
    SvXMLAutoStylePoolP(rTempODBExport),
    rODBExport(rTempODBExport)
{

}

OXMLAutoStylePoolP::~OXMLAutoStylePoolP()
{

}

} // namespace dbaxml

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
