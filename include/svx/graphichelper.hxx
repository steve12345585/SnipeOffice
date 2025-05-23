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

#ifndef INCLUDED_SVX_GRAPHICHELPER_HXX
#define INCLUDED_SVX_GRAPHICHELPER_HXX

#include <vcl/graph.hxx>
#include <svx/svxdllapi.h>

namespace com::sun::star::drawing { class XShape; }
namespace com::sun::star::lang
{
class XComponent;
}
namespace weld { class Widget; }
namespace weld { class Window; }

class SVXCORE_DLLPUBLIC GraphicHelper
{

public:
    static void GetPreferredExtension( OUString& rExtension, const Graphic& rGraphic );
    static OUString GetImageType(const Graphic& rGraphic);
    static OUString ExportGraphic(weld::Window* pWin, const Graphic& rGraphic, const OUString& rGraphicName);
    static void
    SaveShapeAsGraphicToPath(const css::uno::Reference<css::lang::XComponent>& xComponent,
                             const css::uno::Reference<css::drawing::XShape>& xShape,
                             const OUString& rMimeType, const OUString& rPath);
    static void SaveShapeAsGraphic(weld::Window* pWin,
                                   const css::uno::Reference<css::lang::XComponent>& xComponent,
                                   const css::uno::Reference<css::drawing::XShape>& xShape);
    static short HasToSaveTransformedImage(weld::Widget* pWin);
};


#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
