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

#ifndef INCLUDED_SVX_SOURCE_INC_FMDOCUMENTCLASSIFICATION_HXX
#define INCLUDED_SVX_SOURCE_INC_FMDOCUMENTCLASSIFICATION_HXX

#include <com/sun/star/frame/XModel.hpp>


namespace svxform
{

    enum DocumentType
    {
        eTextDocument,
        eWebDocument,
        eSpreadsheetDocument,
        eDrawingDocument,
        ePresentationDocument,
        eEnhancedForm,
        eDatabaseForm,
        eDatabaseReport,

        eUnknownDocumentType
    };

    class DocumentClassification
    {
    public:
        /** classifies a document model
        */
        static DocumentType classifyDocument(
                                const css::uno::Reference< css::frame::XModel >& _rxDocumentModel
                            );

        static DocumentType classifyHostDocument(
                                const css::uno::Reference< css::uno::XInterface >& _rxFormComponent
                            );

        static  DocumentType getDocumentTypeForModuleIdentifier(
                                std::u16string_view _rModuleIdentifier
                            );

        static  OUString getModuleIdentifierForDocumentType(
                                DocumentType _eType
                            );
    };


}


#endif // INCLUDED_SVX_SOURCE_INC_FMDOCUMENTCLASSIFICATION_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
