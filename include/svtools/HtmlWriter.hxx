/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#pragma once

#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <string_view>
#include <vector>
#include <svtools/svtdllapi.h>

class SvStream;

class SVT_DLLPUBLIC HtmlWriter final
{
private:
    std::vector<OString> maElementStack;

    SvStream& mrStream;

    bool mbOpeningTagOpen = false;
    bool mbPrettyPrint;
    /// XML namespace, in case of XHTML.
    OString maNamespace;

public:
    HtmlWriter(SvStream& rStream, std::string_view rNamespace = std::string_view());
    ~HtmlWriter();

    void prettyPrint(bool b);

    void start(const OString& aElement);

    bool end(const OString& aElement);
    void end();

    void flushStack();

    void attribute(std::string_view aAttribute, sal_Int32 aValue);
    void attribute(std::string_view aAttribute, std::string_view aValue);
    void attribute(std::string_view aAttribute, const OUString& aValue);
    template <size_t N> void attribute(std::string_view aAttribute, const char (&aValue)[N])
    {
        attribute(aAttribute, OUString(aValue));
    }
    // boolean attribute e.g. <img ismap>
    void attribute(std::string_view aAttribute);

    void single(const OString& aContent);

    /// Writes character data.
    void characters(std::string_view rChars);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
