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

#pragma once

#include <drawinglayer/drawinglayerdllapi.h>
#include <drawinglayer/primitive2d/CommonTypes.hxx>

namespace drawinglayer::primitive2d
{
class Primitive2DContainer;

// Visitor class for walking a tree of Primitive2DReference
class DRAWINGLAYERCORE_DLLPUBLIC Primitive2DDecompositionVisitor
{
public:
    virtual void visit(const Primitive2DReference&) = 0;
    virtual void visit(const Primitive2DContainer&) = 0;
    virtual void visit(Primitive2DContainer&&) = 0;
    virtual ~Primitive2DDecompositionVisitor() {}
};

} // end of namespace drawinglayer::primitive2d

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
