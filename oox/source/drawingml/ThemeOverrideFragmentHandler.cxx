/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <drawingml/ThemeOverrideFragmentHandler.hxx>
#include <oox/token/namespaces.hxx>
#include <drawingml/themeelementscontext.hxx>

using namespace ::oox::core;

namespace oox::drawingml
{

ThemeOverrideFragmentHandler::ThemeOverrideFragmentHandler(XmlFilterBase& rFilter, const OUString& rFragmentPath, Theme& rOoxTheme, model::Theme& rTheme)
    : FragmentHandler2(rFilter, rFragmentPath)
    , mrOoxTheme(rOoxTheme)
    , mrTheme(rTheme)
{
}

ThemeOverrideFragmentHandler::~ThemeOverrideFragmentHandler()
{
}

ContextHandlerRef ThemeOverrideFragmentHandler::onCreateContext(sal_Int32 nElement, const AttributeList& /*rAttribute*/)
{
    // CT_OfficeStyleSheet
    switch (getCurrentElement())
    {
        case XML_ROOT_CONTEXT:
            switch (nElement)
            {
                case A_TOKEN( themeOverride ): // CT_BaseStylesOverride
                    return new ThemeElementsContext(*this, mrOoxTheme, mrTheme);
            }
        break;
    }
    return nullptr;
}

} // namespace oox::drawingml

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
