/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "importcontext.hxx"
#include "xmlimprt.hxx"

ScXMLImportContext::ScXMLImportContext(SvXMLImport& rImport ) :
    SvXMLImportContext( rImport )
{
}

ScXMLImport& ScXMLImportContext::GetScImport()
{
    return static_cast<ScXMLImport&>(GetImport());
}

const ScXMLImport& ScXMLImportContext::GetScImport() const
{
    return static_cast<const ScXMLImport&>(GetImport());
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
