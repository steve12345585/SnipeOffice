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

#ifndef INCLUDED_OOX_VML_VMLSHAPECONTEXT_HXX
#define INCLUDED_OOX_VML_VMLSHAPECONTEXT_HXX

#include <oox/core/contexthandler.hxx>
#include <oox/core/contexthandler2.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>
//#include <oox/dllapi.h>

#include <optional>

namespace oox { class AttributeList; }

namespace oox::vml {

class Drawing;

struct ShapeTypeModel;
class ShapeType;

struct ClientData;
struct ShapeModel;
class ShapeBase;
class GroupShape;
class RectangleShape;

class ShapeContainer;


class ShapeLayoutContext final : public ::oox::core::ContextHandler2
{
public:
    explicit            ShapeLayoutContext(
                            ::oox::core::ContextHandler2Helper const & rParent,
                            Drawing& rDrawing );

    virtual ::oox::core::ContextHandlerRef
                        onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;

private:
    Drawing&            mrDrawing;
};


class ClientDataContext final : public ::oox::core::ContextHandler2
{
public:
    explicit            ClientDataContext(
                            ::oox::core::ContextHandler2Helper const & rParent,
                            ClientData& rClientData,
                            const AttributeList& rAttribs );

    virtual ::oox::core::ContextHandlerRef
                        onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;
    virtual void        onCharacters( const OUString& rChars ) override;
    virtual void        onEndElement() override;

private:
    ClientData&         mrClientData;
    OUString     maElementText;
};


class SAL_DLLPUBLIC_RTTI ShapeContextBase : public ::oox::core::ContextHandler2
{
public:
    static ::oox::core::ContextHandlerRef
                        createShapeContext(
                            ::oox::core::ContextHandler2Helper const & rParent,
                            ShapeContainer& rShapes,
                            sal_Int32 nElement,
                            const AttributeList& rAttribs );

protected:
    explicit            ShapeContextBase( ::oox::core::ContextHandler2Helper const & rParent );
};


class SAL_DLLPUBLIC_RTTI ShapeTypeContext : public ShapeContextBase
{
public:
    explicit            ShapeTypeContext(
                            ::oox::core::ContextHandler2Helper const & rParent,
                            std::shared_ptr<ShapeType> const& pShapeType,
                            const AttributeList& rAttribs );

    virtual ::oox::core::ContextHandlerRef
                        onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;

private:
    /** Processes the 'style' attribute. */
    void                setStyle( std::u16string_view rStyle );

    /** Resolve a relation identifier to a fragment path. */
    std::optional< OUString > decodeFragmentPath( const AttributeList& rAttribs, sal_Int32 nToken ) const;

private:
    std::shared_ptr<ShapeType> m_pShapeType;
    ShapeTypeModel&     mrTypeModel;
};


class SAL_DLLPUBLIC_RTTI ShapeContext : public ShapeTypeContext
{
public:
    explicit ShapeContext(::oox::core::ContextHandler2Helper const& rParent,
                          const std::shared_ptr<ShapeBase>& pShape, const AttributeList& rAttribs);

    virtual ::oox::core::ContextHandlerRef
                        onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;

    void OOX_DLLPUBLIC setWriterShape();

private:
    /** Processes the 'points' attribute. */
    void                setPoints( std::u16string_view rPoints );
    /** Processes the 'from' attribute. */
    void                setFrom( const OUString& rPoints );
    /** Processes the 'to' attribute. */
    void                setTo( const OUString& rPoints );
    /** Processes the 'control1' attribute. */
    void                setControl1( const OUString& rPoints );
    /** Processes the 'control2' attribute. */
    void                setControl2( const OUString& rPoints );
    /** Processes the 'path' attribute. */
    void                setVmlPath( const OUString& rPath );
    /** Processes the 'href' attribute. */
    void                setHyperlink( const OUString& rHyperlink );

private:
    ShapeBase&          mrShape;
    ShapeModel&         mrShapeModel;
};


class GroupShapeContext final : public ShapeContext
{
public:
    explicit GroupShapeContext(::oox::core::ContextHandler2Helper const& rParent,
                               const std::shared_ptr<GroupShape>& pShape,
                               const AttributeList& rAttribs);

    virtual ::oox::core::ContextHandlerRef
                        onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;

private:
    ShapeContainer&     mrShapes;
};


class RectangleShapeContext final : public ShapeContext
{
public:
    explicit RectangleShapeContext(::oox::core::ContextHandler2Helper const& rParent,
                                   const AttributeList& rAttribs,
                                   const std::shared_ptr<RectangleShape>& pShape);

    virtual ::oox::core::ContextHandlerRef
                        onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs ) override;
};

class ControlShapeContext final : public ShapeContextBase
{
public:
    explicit            ControlShapeContext(
                            ::oox::core::ContextHandler2Helper const & rParent,
                            ShapeContainer& rShapes,
                            const AttributeList& rAttribs );
};


} // namespace oox::vml

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
