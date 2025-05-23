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

#ifndef INCLUDED_OOX_EXPORT_VMLEXPORT_HXX
#define INCLUDED_OOX_EXPORT_VMLEXPORT_HXX

#include <sal/config.h>

#include <string_view>

#include <com/sun/star/uno/Reference.hxx>
#include <editeng/outlobj.hxx>
#include <filter/msfilter/escherex.hxx>
#include <oox/dllapi.h>
#include <rtl/strbuf.hxx>
#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <sax/fshelper.hxx>
#include <rtl/ref.hxx>

namespace com::sun::star {
    namespace drawing {
        class XShape;
    }
}

namespace oox::drawingml {
   class DrawingML;
}


namespace sax_fastparser {
    class FastAttributeList;
}

namespace tools { class Rectangle; }
class SdrObject;

namespace oox::vml {

/// Interface to be implemented by the parent exporter that knows how to handle shape text.
class OOX_DLLPUBLIC VMLTextExport
{
public:
    virtual void WriteOutliner(const OutlinerParaObject& rParaObj) = 0;
    virtual oox::drawingml::DrawingML& GetDrawingML() = 0;
    /// Write the contents of the textbox that is associated to this shape in VML format.
    virtual void WriteVMLTextBox(css::uno::Reference<css::drawing::XShape> xShape) = 0;
protected:
    VMLTextExport() {}
    virtual ~VMLTextExport() {}
};

class OOX_DLLPUBLIC VMLExport : public EscherEx
{
    /// Fast serializer to output the data
    ::sax_fastparser::FSHelperPtr m_pSerializer;

    /// Parent exporter, used for text callback.
    VMLTextExport* m_pTextExport;

    /// Anchoring - Writer specific properties
    sal_Int16 m_eHOri, m_eVOri, m_eHRel, m_eVRel;
    rtl::Reference<sax_fastparser::FastAttributeList> m_pWrapAttrList;
    bool m_bInline; // css::text::TextContentAnchorType_AS_CHARACTER
    bool m_IsFollowingTextFlow = false;

    /// The object we're exporting.
    const SdrObject* m_pSdrObject;

    /// Fill the shape attributes as they come.
    rtl::Reference<::sax_fastparser::FastAttributeList> m_pShapeAttrList;

    /// Remember the shape type.
    sal_uInt32 m_nShapeType;

    /// Remember the shape flags.
    ShapeFlag m_nShapeFlags;

    /// Remember style, the most important shape attribute ;-)
    OStringBuffer m_ShapeStyle;

    /// style for textbox
    OStringBuffer m_TextboxStyle;

    /// Remember the generated shape id.
    OString m_sShapeId;

    /// Remember which shape types we had already written.
    std::vector<bool> m_aShapeTypeWritten;

    /// It seems useless to write out an XML_ID attribute next to XML_id which defines the actual shape id
    bool m_bSkipwzName;

    /// Use '#' mark for type attribute (check Type Attribute of VML shape in OOXML documentation)
    bool m_bUseHashMarkForType;

    /** There is a shapeid generation mechanism in EscherEx, but it does not seem to work
    *   so override the existing behavior to get actually unique ids.
    */
    bool m_bOverrideShapeIdGeneration;

    /// Prefix for overridden shape id generation (used if m_bOverrideShapeIdGeneration is true)
    OString m_sShapeIDPrefix;

    /// Counter for generating shape ids (used if m_bOverrideShapeIdGeneration is true)
    sal_uInt64 m_nShapeIDCounter;

public:
                        VMLExport( ::sax_fastparser::FSHelperPtr pSerializer, VMLTextExport* pTextExport = nullptr);
    virtual             ~VMLExport() override;

    const ::sax_fastparser::FSHelperPtr&
                        GetFS() const { return m_pSerializer; }

    void SetFS(const ::sax_fastparser::FSHelperPtr& pSerializer);

    /// Export the sdr object as VML.
    ///
    /// Call this when you need to export the object as VML.
    OString const & AddSdrObject( const SdrObject& rObj,
            bool const bIsFollowingTextFlow = false,
            sal_Int16 eHOri = -1, sal_Int16 eVOri = -1, sal_Int16 eHRel = -1,
            sal_Int16 eVRel = -1,
            sax_fastparser::FastAttributeList* pWrapAttrList = nullptr,
            const bool bOOxmlExport = false, sal_uInt32 nId = 0);
    OString const & AddInlineSdrObject( const SdrObject& rObj, const bool bOOxmlExport );
    virtual void  AddSdrObjectVMLObject( const SdrObject& rObj) override;
    static bool IsWaterMarkShape(std::u16string_view rStr);

    void    SetSkipwzName(bool bSkipwzName) { m_bSkipwzName = bSkipwzName; }
    void    SetHashMarkForType(bool bUseHashMarkForType) { m_bUseHashMarkForType = bUseHashMarkForType; }
    void    OverrideShapeIDGen(bool bOverrideShapeIdGeneration,
                            const OString& sShapeIDPrefix = OString());
    static OString GetVMLShapeTypeDefinition(std::string_view sShapeID, const bool bIsPictureFrame);

protected:
    /// Add an attribute to the generated <v:shape/> element.
    ///
    /// This should be called from within StartShape() to ensure that the
    /// added attribute is preserved.
    void AddShapeAttribute(sal_Int32 nAttribute, std::string_view sValue);

    using EscherEx::StartShape;
    using EscherEx::EndShape;

    /// Override shape ID generation when m_bOverrideShapeIdGeneration is set to true
    virtual sal_uInt32   GenerateShapeId() override;

    /// Start the shape for which we just collected the information.
    ///
    /// Returns the element's tag number, -1 means we wrote nothing.
    virtual sal_Int32   StartShape();

    /// End the shape.
    ///
    /// The parameter is just what we got from StartShape().
    virtual void        EndShape( sal_Int32 nShapeElement );
    virtual void        Commit( EscherPropertyContainer& rProps, const tools::Rectangle& rRect ) override;

private:

    virtual void OpenContainer( sal_uInt16 nEscherContainer, int nRecInstance = 0 ) override;
    virtual void CloseContainer() override;

    virtual sal_uInt32 EnterGroup( const OUString& rShapeName, const tools::Rectangle* pBoundRect ) override;
    virtual void LeaveGroup() override;

    virtual void AddShape( sal_uInt32 nShapeType, ShapeFlag nShapeFlags, sal_uInt32 nShapeId = 0 ) override;

private:
    /// Create an OString representing the id from a numerical id.
    OString ShapeIdString( sal_uInt32 nId );

    /// Add flip X and\or flip Y
    void AddFlipXY( );

    /// Add starting and ending point of a line to the m_pShapeAttrList.
    void AddLineDimensions( const tools::Rectangle& rRectangle );

    /// Add position and size to the OStringBuffer.
    void AddRectangleDimensions( OStringBuffer& rBuffer, const tools::Rectangle& rRectangle, bool rbAbsolutePos = true );
};


} // namespace oox::vml

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
