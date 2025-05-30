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

#include "LoggedResources.hxx"
#include <com/sun/star/awt/Size.hpp>
#include <com/sun/star/drawing/XShape.hpp>
#include <com/sun/star/text/WrapTextMode.hpp>
#include <rtl/ref.hxx>

class SwXTextDocument;
namespace com::sun::star{
    namespace graphic{
        class XGraphic;
    }
    namespace io{
        class XInputStream;
    }
    namespace text{
        class XTextContent;
        class XTextDocument;
    }
    namespace uno {
        class XComponentContext;
    }
}
namespace writerfilter::dmapper
{
class DomainMapper;
/** Handler for OLE objects
 */
class OLEHandler : public LoggedProperties
{
    OUString     m_sProgId;
    OUString     m_sDrawAspect;
    OUString     m_sVisAreaWidth;
    OUString     m_sVisAreaHeight;
    /// The stream URL right after the import of the raw data.
    OUString     m_aURL;

    css::text::WrapTextMode     m_nWrapMode;

    css::uno::Reference<css::drawing::XShape> m_xShape;

    css::uno::Reference<css::io::XInputStream> m_xInputStream;
    DomainMapper& m_rDomainMapper;

    // Properties
    virtual void lcl_attribute(Id Name, const Value & val) override;
    virtual void lcl_sprm(Sprm & sprm) override;

public:
    explicit OLEHandler(DomainMapper& rDomainMapper);
    virtual ~OLEHandler() override;

    const css::uno::Reference<css::drawing::XShape>& getShape() const { return m_xShape; };

    bool isOLEObject() const { return m_xInputStream.is(); }

    /// In case of a valid CLSID, import the native data to the previously created empty OLE object.
    void importStream(const css::uno::Reference<css::uno::XComponentContext>& xComponentContext,
                      const css::uno::Reference<css::text::XTextDocument>& xTextDocument,
                      const css::uno::Reference<css::text::XTextContent>& xOLE);

    /// Get the CLSID of the OLE object, in case we can find one based on m_sProgId.
    OUString getCLSID() const;

    OUString const & GetDrawAspect() const;
    OUString const & GetVisAreaWidth() const;
    OUString const & GetVisAreaHeight() const;

    OUString copyOLEOStream(rtl::Reference<SwXTextDocument> const& xTextDocument);

    css::awt::Size getSize() const;
    css::uno::Reference<css::graphic::XGraphic> getReplacement() const;

};
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
