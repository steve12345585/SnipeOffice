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

#include <cassert>

#include <com/sun/star/beans/Optional.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <utility>

#include "components.hxx"
#include "node.hxx"
#include "propertynode.hxx"
#include "type.hxx"

namespace configmgr {

PropertyNode::PropertyNode(
    int layer, Type staticType, bool nillable, css::uno::Any value,
    bool extension):
    Node(layer), staticType_(staticType), nillable_(nillable),
    extension_(extension), modified_(false), value_(std::move(value))
{}

rtl::Reference< Node > PropertyNode::clone(bool) const {
    return new PropertyNode(*this);
}


css::uno::Any const & PropertyNode::getValue(Components & components) {
    if (!externalDescriptor_.isEmpty()) {
        css::beans::Optional< css::uno::Any > val(
            components.getExternalValue(externalDescriptor_));
        if (val.IsPresent) {
            value_ = val.Value; //TODO: check value type
        }
        externalDescriptor_.clear(); // must not throw
    }
    SAL_WARN_IF(
        !(value_.hasValue() || nillable_), "configmgr",
        "non-nillable property without value");
    return value_;
}

void PropertyNode::setValue(int layer, css::uno::Any const & value, bool bIsUserModification) {
    setLayer(layer);
    value_ = value;
    // Consider as modified when modified during runtime or by user registry modifications
    modified_ = bIsUserModification;
    externalDescriptor_.clear();
}

css::uno::Any *PropertyNode::getValuePtr(int layer, bool bIsUserModification)
{
    setLayer(layer);
    modified_ = bIsUserModification;
    externalDescriptor_.clear();
    return &value_;
}

void PropertyNode::setExternal(int layer, OUString const & descriptor) {
    assert(!descriptor.isEmpty());
    setLayer(layer);
    externalDescriptor_ = descriptor;
}

PropertyNode::~PropertyNode() {}

Node::Kind PropertyNode::kind() const {
    return KIND_PROPERTY;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
