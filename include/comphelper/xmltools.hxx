/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_COMPHELPER_XMLTOOLS_HXX
#define INCLUDED_COMPHELPER_XMLTOOLS_HXX

#include <rtl/string.hxx>
#include <comphelper/comphelperdllapi.h>

namespace comphelper::xml
{
        COMPHELPER_DLLPUBLIC OString makeXMLChaff();
        COMPHELPER_DLLPUBLIC OString generateGUIDString();
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
