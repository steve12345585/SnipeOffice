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

#include <sal/config.h>

#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>

#include "node.hxx"
#include "nodemap.hxx"

namespace configmgr
{
class GroupNode : public Node
{
public:
    GroupNode(int layer, bool extensible, OUString templateName);

    virtual rtl::Reference<Node> clone(bool keepTemplateName) const override;

    virtual NodeMap& getMembers() override;

    virtual OUString getTemplateName() const override;

    virtual void setMandatory(int layer) override;

    virtual int getMandatory() const override;

    bool isExtensible() const { return extensible_; }

private:
    GroupNode(GroupNode const& other, bool keepTemplateName);

    virtual ~GroupNode() override;

    virtual Kind kind() const override;

    bool extensible_;
    NodeMap members_;
    OUString templateName_; // non-empty if this node is a template, free node, or set member
    int mandatory_;
};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
