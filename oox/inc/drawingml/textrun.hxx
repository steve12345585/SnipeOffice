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

#ifndef INCLUDED_OOX_DRAWINGML_TEXTRUN_HXX
#define INCLUDED_OOX_DRAWINGML_TEXTRUN_HXX

#include <memory>

#include <com/sun/star/text/XTextCursor.hpp>
#include <com/sun/star/text/XText.hpp>
#include <drawingml/textcharacterproperties.hxx>

namespace oox::drawingml {

class TextRun
{
public:
    TextRun();
    virtual ~TextRun();

    OUString&         getText() { return msText; }
    const OUString&   getText() const { return msText; }

    TextCharacterProperties&         getTextCharacterProperties() { return maTextCharacterProperties; }
    const TextCharacterProperties&   getTextCharacterProperties() const { return maTextCharacterProperties; }

    void                 setLineBreak() { mbIsLineBreak = true; }
    bool isLineBreak() const { return mbIsLineBreak; }

    /** Returns whether the textrun had properties that alter it visually in its rPr tag
     *
     *  For instance _lang_ doesn't have a visual effect.
     */
    bool hasVisualRunProperties() const { return maTextCharacterProperties.mbHasVisualRunProperties; }

    virtual sal_Int32               insertAt(
                                    const ::oox::core::XmlFilterBase& rFilterBase,
                                    const css::uno::Reference < css::text::XText >& xText,
                                    const css::uno::Reference < css::text::XTextCursor >& xAt,
                                    const TextCharacterProperties& rTextCharacterStyle,
                                    float nDefaultCharHeight) const;

private:
    OUString             msText;
    TextCharacterProperties     maTextCharacterProperties;
    bool                        mbIsLineBreak;
};

typedef std::shared_ptr< TextRun > TextRunPtr;

}

#endif // INCLUDED_OOX_DRAWINGML_TEXTRUN_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
