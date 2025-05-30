/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_OOX_SOURCE_SHAPE_WPGCONTEXT_HXX
#define INCLUDED_OOX_SOURCE_SHAPE_WPGCONTEXT_HXX

#include <oox/core/fragmenthandler2.hxx>
#include <oox/drawingml/drawingmltypes.hxx>

namespace oox::shape
{
/// Wpg is the drawingML equivalent of v:group.
class WpgContext final : public oox::core::FragmentHandler2
{
public:
    explicit WpgContext(oox::core::FragmentHandler2 const& rParent,
                        const oox::drawingml::ShapePtr& pMaster);
    ~WpgContext() override;

    oox::core::ContextHandlerRef onCreateContext(sal_Int32 nElementToken,
                                                 const oox::AttributeList& rAttribs) override;

    const oox::drawingml::ShapePtr& getShape() const { return mpShape; }

    const bool& isFullWPGSupport() const { return m_bFullWPGSupport; };
    void setFullWPGSupport(bool bUse) { m_bFullWPGSupport = bUse; };

private:
    oox::drawingml::ShapePtr mpShape;

    bool m_bFullWPGSupport;
};
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
