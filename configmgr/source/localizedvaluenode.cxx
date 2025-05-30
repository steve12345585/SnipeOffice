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

#include <com/sun/star/uno/Any.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <utility>

#include "localizedvaluenode.hxx"
#include "node.hxx"

namespace configmgr {

LocalizedValueNode::LocalizedValueNode(int layer, css::uno::Any value):
    Node(layer), value_(std::move(value)), modified_(false)
{}

LocalizedValueNode::LocalizedValueNode(int layer):
    Node(layer), modified_(false)
{}

rtl::Reference< Node > LocalizedValueNode::clone(bool) const {
    return new LocalizedValueNode(*this);
}

OUString LocalizedValueNode::getTemplateName() const {
    return u"*"_ustr;
}


void LocalizedValueNode::setValue(int layer, css::uno::Any const & value, bool bIsUserModification)
{
    setLayer(layer);
    modified_ = bIsUserModification;
    if (&value != &value_)
        value_ = value;
}

LocalizedValueNode::~LocalizedValueNode() {}

Node::Kind LocalizedValueNode::kind() const {
    return KIND_LOCALIZED_VALUE;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
