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

#ifndef INCLUDED_SLIDESHOW_SOURCE_ENGINE_ANIMATIONNODES_ANIMATIONTRANSITIONFILTERNODE_HXX
#define INCLUDED_SLIDESHOW_SOURCE_ENGINE_ANIMATIONNODES_ANIMATIONTRANSITIONFILTERNODE_HXX

#include "animationbasenode.hxx"
#include <com/sun/star/animations/XTransitionFilter.hpp>

namespace slideshow::internal {

class AnimationTransitionFilterNode : public AnimationBaseNode
{
public:
    AnimationTransitionFilterNode(
        css::uno::Reference<css::animations::XAnimationNode> const& xNode,
        ::std::shared_ptr<BaseContainerNode> const& pParent,
        NodeContext const& rContext )
        : AnimationBaseNode( xNode, pParent, rContext ),
          mxTransitionFilterNode( xNode, css::uno::UNO_QUERY_THROW)
        {}

#if OSL_DEBUG_LEVEL >= 2
    virtual const char* getDescription() const
        { return "AnimationTransitionFilterNode"; }
#endif

protected:
    virtual void dispose() override;

private:
    virtual AnimationActivitySharedPtr createActivity() const override;

    css::uno::Reference<css::animations::XTransitionFilter> mxTransitionFilterNode;
};

} // namespace slideshow::internal

#endif // INCLUDED_SLIDESHOW_SOURCE_ENGINE_ANIMATIONNODES_ANIMATIONTRANSITIONFILTERNODE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
