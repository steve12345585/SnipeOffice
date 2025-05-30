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

#include <drawingml/embeddedwavaudiofile.hxx>
#include <oox/helper/attributelist.hxx>
#include <oox/token/namespaces.hxx>
#include <oox/token/tokens.hxx>

namespace oox::drawingml
{
// CT_EmbeddedWAVAudioFile
OUString getEmbeddedWAVAudioFile(const core::Relations& rRelations, const AttributeList& rAttribs)
{
    if (rAttribs.getBool(XML_builtIn, false))
        return rAttribs.getStringDefaulted(XML_name);
    else
        return rRelations.getFragmentPathFromRelId(rAttribs.getStringDefaulted(R_TOKEN(embed)));
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
