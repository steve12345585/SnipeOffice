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

#ifndef INCLUDED_SLIDESHOW_SOURCE_ENGINE_ANIMATIONNODES_ANIMATIONCOMMANDNODE_HXX
#define INCLUDED_SLIDESHOW_SOURCE_ENGINE_ANIMATIONNODES_ANIMATIONCOMMANDNODE_HXX

#include <slideshowdllapi.h>
#include <basecontainernode.hxx>
#include <iexternalmediashapebase.hxx>
#include <com/sun/star/animations/XCommand.hpp>

namespace slideshow::internal {

/** Command node.

    This animation node encapsulates a command. Not yet implemented:
    verb & custom.
*/
class SLIDESHOW_DLLPUBLIC AnimationCommandNode : public BaseNode
{
public:
    AnimationCommandNode(
        css::uno::Reference<css::animations::XAnimationNode> const& xNode,
        ::std::shared_ptr<BaseContainerNode> const& pParent,
        NodeContext const& rContext );

    /// Assuming that xCommandNode is a play command, determines if an audio node wants looping when
    /// xShape plays.
    static bool
    GetLoopingFromAnimation(const css::uno::Reference<css::animations::XCommand>& xCommandNode,
                            const css::uno::Reference<css::drawing::XShape>& xShape);

protected:
    virtual void dispose() override;

private:
    virtual void activate_st() override;
    virtual bool hasPendingAnimation() const override;

private:
    IExternalMediaShapeBaseSharedPtr mpShape;
    css::uno::Reference<css::animations::XCommand > mxCommandNode;
    css::uno::Reference<css::drawing::XShape> mxShape;
};

} // namespace slideshow::internal

#endif // INCLUDED_SLIDESHOW_SOURCE_ENGINE_ANIMATIONNODES_ANIMATIONCOMMANDNODE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
