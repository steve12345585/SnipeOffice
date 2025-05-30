/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>

#include <string_view>
#include <utility>

#include <rtl/strbuf.hxx>
#include <rtl/string.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>

void call_view(std::u16string_view) {}
void call_view(std::string_view) {}
struct ConstructWithView
{
    ConstructWithView(std::u16string_view) {}
    ConstructWithView(std::string_view) {}
};

namespace test1
{
void f1(std::u16string_view s1)
{
    // no warning expected
    call_view(s1);
}
void f1(std::string_view s1)
{
    // no warning expected
    call_view(s1);
}
void f1(OUString s1)
{
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    call_view(s1.copy(1, 2));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    call_view(s1.copy(1));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    ConstructWithView(s1.copy(1, 2));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    ConstructWithView(s1.copy(1));
}
void f1(OString s1)
{
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    call_view(s1.copy(1, 2));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    call_view(s1.copy(1));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    ConstructWithView(s1.copy(1, 2));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    ConstructWithView(s1.copy(1));
}
void f1(OUStringBuffer s1)
{
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    call_view(s1.copy(1, 2));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    call_view(s1.copy(1));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    ConstructWithView(s1.copy(1, 2));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    ConstructWithView(s1.copy(1));
    // expected-error@+1 {{rather than call toString, pass with a view [loplugin:stringview]}}
    call_view(s1.toString());
    // expected-error@+1 {{rather than call toString, pass with a view [loplugin:stringview]}}
    ConstructWithView(s1.toString());
}
void f1(OStringBuffer s1)
{
    // expected-error@+1 {{rather than call toString, pass with a view [loplugin:stringview]}}
    call_view(s1.toString());
    // expected-error@+1 {{rather than call toString, pass with a view [loplugin:stringview]}}
    ConstructWithView(s1.toString());
}
void makeStringAndClear(OUStringBuffer s)
{
    call_view(s.makeStringAndClear());
    ConstructWithView(s.makeStringAndClear());
    call_view((&s)->makeStringAndClear());
    ConstructWithView((&s)->makeStringAndClear());
    // expected-error@+1 {{rather than call makeStringAndClear on an rvalue, pass with a view [loplugin:stringview]}}
    call_view(std::move(s).makeStringAndClear());
    // expected-error@+1 {{rather than call makeStringAndClear on an rvalue, pass with a view [loplugin:stringview]}}
    ConstructWithView(std::move(s).makeStringAndClear());
    // expected-error@+1 {{rather than call makeStringAndClear on an rvalue, pass with a view [loplugin:stringview]}}
    call_view((s).copy(1).makeStringAndClear());
    // expected-error@+1 {{rather than call makeStringAndClear on an rvalue, pass with a view [loplugin:stringview]}}
    ConstructWithView(s.copy(1).makeStringAndClear());
}
void makeStringAndClear(OStringBuffer s)
{
    call_view(s.makeStringAndClear());
    ConstructWithView(s.makeStringAndClear());
    call_view((&s)->makeStringAndClear());
    ConstructWithView((&s)->makeStringAndClear());
    // expected-error@+1 {{rather than call makeStringAndClear on an rvalue, pass with a view [loplugin:stringview]}}
    call_view(std::move(s).makeStringAndClear());
    // expected-error@+1 {{rather than call makeStringAndClear on an rvalue, pass with a view [loplugin:stringview]}}
    ConstructWithView(std::move(s).makeStringAndClear());
}
}

namespace test2
{
void f3(OUString s1)
{
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    OUString s2 = s1.copy(1, 2) + "xxx";
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    s2 = s1.copy(1) + "xxx";
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    s2 = "xxx" + s1.copy(1);
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    s2 += s1.copy(1);
    (void)s2;
}
void f3(OString s1)
{
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    OString s2 = s1.copy(1, 2) + "xxx";
    (void)s2;
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    OString s3 = s1.copy(1) + "xxx";
    (void)s3;
}
}

namespace test3
{
void f4(OUString s1, OUString s2)
{
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    sal_Unicode x = s2.copy(1, 2)[12];
    (void)x;
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    if (s2.copy(1, 2) < s1)
        ;
}
}

void f5(char const* s1, sal_Int32 n1, char16_t const* s2, sal_Int32 n2, OString s3, OUString s4)
{
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OString', pass a 'std::string_view' [loplugin:stringview]}}
    call_view(OString());
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OString' constructed from a 'const char{{ ?}}[4]', pass a 'std::string_view' [loplugin:stringview]}}
    call_view(OString("foo"));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OString' constructed from a 'const char', pass a 'std::string_view' (or an 'rtl::OStringChar') [loplugin:stringview]}}
    call_view(OString(*s1));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OString' constructed from a 'const char *', pass a 'std::string_view' [loplugin:stringview]}}
    call_view(OString(s1));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OString' constructed from a 'const char *', pass a 'std::string_view' [loplugin:stringview]}}
    call_view(OString(s1, n1));
    constexpr OStringLiteral l1("foo");
    call_view(OString(l1));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OString' constructed from a 'std::string_view' (aka 'basic_string_view<char>'), pass a 'std::string_view' [loplugin:stringview]}}
    call_view(OString(std::string_view("foo")));
    // expected-error-re@+1 {{instead of an {{'(rtl::)?}}OString' constructed from a {{'(rtl::)?StringNumber<char, 33>'|'OStringNumber<33>' \(aka 'StringNumber<char, 33ULL?>'\)}}, pass a 'std::string_view' [loplugin:stringview]}}
    call_view(OString(OString::number(0)));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OString' constructed from a 'OStringConcat<{{(rtl::)?}}OString, {{(rtl::)?}}OString>' (aka 'StringConcat<char, rtl::OString, rtl::OString>'), pass a 'std::string_view' via 'rtl::Concat2View' [loplugin:stringview]}}
    call_view(OString(s3 + s3));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString', pass a 'std::u16string_view' [loplugin:stringview]}}
    call_view(OUString());
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'const char{{ ?}}[4]', pass a 'std::u16string_view' [loplugin:stringview]}}
    call_view(OUString("foo"));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'const char16_t{{ ?}}[4]', pass a 'std::u16string_view' [loplugin:stringview]}}
    call_view(OUString(u"foo", 3));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'const char', pass a 'std::u16string_view' (or an 'rtl::OUStringChar') [loplugin:stringview]}}
    call_view(OUString(*s1));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'const char16_t', pass a 'std::u16string_view' (or an 'rtl::OUStringChar') [loplugin:stringview]}}
    call_view(OUString(*s2));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'const char16_t *', pass a 'std::u16string_view' [loplugin:stringview]}}
    call_view(OUString(s2));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'const char16_t *', pass a 'std::u16string_view' [loplugin:stringview]}}
    call_view(OUString(s2, n2));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'const char16_t *', pass a 'std::u16string_view' (or an 'rtl::OUStringChar') [loplugin:stringview]}}
    call_view(OUString(s2, 1));
    constexpr OUStringLiteral l2(u"foo");
    call_view(OUString(l2));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'std::u16string_view' (aka 'basic_string_view<char16_t>'), pass a 'std::u16string_view' [loplugin:stringview]}}
    call_view(OUString(std::u16string_view(u"foo")));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a {{'(rtl::)?StringNumber<char16_t, 33>'|'OUStringNumber<33>' \(aka 'StringNumber<char16_t, 33ULL?>'\)}}, pass a 'std::u16string_view' [loplugin:stringview]}}
    call_view(OUString(OUString::number(0)));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'OUStringConcat<{{(rtl::)?}}OUString, {{(rtl::)?}}OUString>' (aka 'StringConcat<char16_t, rtl::OUString, rtl::OUString>'), pass a 'std::u16string_view' via 'rtl::Concat2View' [loplugin:stringview]}}
    call_view(OUString(s4 + s4));

    (void)(s3 == l1);
    (void)(s4 == l2);
}

void f5(OUString s)
{
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    OUStringBuffer buf(s.copy(5));
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    buf = s.copy(5);
    // expected-error@+1 {{rather than copy, pass with a view using subView() [loplugin:stringview]}}
    buf.append(s.copy(12));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'std::u16string_view' (aka 'basic_string_view<char16_t>'), pass a 'std::u16string_view' [loplugin:stringview]}}
    buf.append(OUString(std::u16string_view(u"foo")));
    // expected-error-re@+1 {{instead of an '{{(rtl::)?}}OUString' constructed from a 'std::u16string_view' (aka 'basic_string_view<char16_t>'), pass a 'std::u16string_view' [loplugin:stringview]}}
    s += OUString(std::u16string_view(u"foo"));
}

void f6(OUString s)
{
    // expected-error@+1 {{rather than getToken, pass with a view using o3tl::getToken() [loplugin:stringview]}}
    s.getToken(1, ' ').toInt32();
}
/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
