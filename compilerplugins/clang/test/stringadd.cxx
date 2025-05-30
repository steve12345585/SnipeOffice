/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <rtl/strbuf.hxx>
#include <rtl/string.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>

#pragma clang diagnostic ignored "-Wunknown-warning-option" // for Clang < 13
#pragma clang diagnostic ignored "-Wunused-but-set-parameter"
#pragma clang diagnostic ignored "-Wunused-but-set-variable"

// ---------------------------------------------------------------
// += tests

namespace test1
{
static const char XXX1[] = "xxx";
static constexpr char16_t XXX1u[] = u"xxx";
static const char XXX2[] = "xxx";
void f1(OUString s1, int i, OString o)
{
    OUString s2 = s1;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += s1;
    s2 = s1 + "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += s1;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += OUString::number(i);
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += XXX1;
    // expected-error-re@+2 {{rather use O[U]String::Concat than constructing '{{(rtl::)?}}OUStringLiteral<4>'{{( \(aka 'rtl::OUStringLiteral<4>'\))?}} from 'const char16_t{{ ?}}[4]' on LHS of + (where RHS is of type 'const char{{ ?}}[4]') [loplugin:stringadd]}}
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += OUStringLiteral(XXX1u) + XXX2;

    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += OStringToOUString(o, RTL_TEXTENCODING_UTF8);
}
void f2(OString s1, int i, OUString u)
{
    OString s2 = s1;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += s1;
    s2 = s1 + "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += s1;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += OString::number(i);

    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += OUStringToOString(u, RTL_TEXTENCODING_ASCII_US);
}
void f3(OUString aStr, int nFirstContent)
{
    OUString aFirstStr = aStr.copy(0, nFirstContent);
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    aFirstStr += "...";
}
OUString side_effect();
void f4(int i)
{
    OUString s1;
    OUString s2("xxx");
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += "xxx";
    ++i;
    // any other kind of statement breaks the chain (at least for now)
    s2 += "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s2 += side_effect();
    s1 += "yyy";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s1 += "yyy";
}
}

namespace test2
{
void f(OUString s3)
{
    s3 += "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s3 += "xxx";
}
void g(OString s3)
{
    s3 += "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s3 += "xxx";
}
}

namespace test3
{
struct Bar
{
    OUString m_field;
};
void f(Bar b1, Bar& b2, Bar* b3)
{
    OUString s3 = "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s3 += b1.m_field;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s3 += b2.m_field;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s3 += b3->m_field;
}
OUString side_effect();
void f2(OUString s)
{
    OUString sRet = "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    sRet += side_effect();
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    sRet += "xxx";
    sRet += side_effect();
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    sRet += "xxx";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    sRet += "xxx";
    sRet += s;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    sRet += "xxx";
}
}

// no warning expected
namespace test4
{
OUString side_effect();
void f()
{
    OUString sRet = "xxx";
#if OSL_DEBUG_LEVEL > 0
    sRet += ";";
#endif
    sRet += " ";
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    sRet += side_effect();
}
}

// no warning expected
namespace test5
{
OUString side_effect();
void f()
{
    OUString sRet = side_effect();
    sRet += side_effect();
}
}

namespace test6
{
void f(OUString sComma, OUString maExtension, int mnDocumentIconID)
{
    OUString sValue;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    sValue += sComma + sComma + maExtension + sComma;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    sValue += OUString::number(mnDocumentIconID) + sComma;
}
struct Foo
{
    OUString sFormula1;
};
void g(int x, const Foo& aValidation)
{
    OUString sCondition;
    switch (x)
    {
        case 1:
            sCondition += "cell-content-is-in-list(";
            // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
            sCondition += aValidation.sFormula1 + ")";
    }
}
}

// ---------------------------------------------------------------
// detecting OUString temporary construction in +

namespace test9
{
OUString getByValue();
const OUString& getByRef();
void f1(OUString s, OUString t, int i, const char* pChar)
{
    // no warning expected
    t = t + "xxx";
    // expected-error-re@+1 {{rather use O[U]String::Concat than constructing '{{(rtl::)?}}OUString' from 'const char{{ ?}}[4]' on RHS of + (where LHS is of type '{{(rtl::)?}}OUString') [loplugin:stringadd]}}
    s = s + OUString("xxx");
    // expected-error-re@+1 {{rather use O[U]String::Concat than constructing '{{(rtl::)?}}OUString' from 'const {{(rtl::)?}}OUString' on RHS of + (where LHS is of type '{{(rtl::)?}}OUString') [loplugin:stringadd]}}
    s = s + OUString(getByRef());

    // no warning expected
    OUString a;
    a = a + getByValue();

    // no warning expected
    OUString b;
    b = b + (i == 1 ? "aaa" : "bbb");

    // no warning expected
    OUString c;
    c = c + OUString(pChar, strlen(pChar), RTL_TEXTENCODING_UTF8);

    OUStringBuffer buf;
    // expected-error@+1 {{chained append, rather use single append call and + operator [loplugin:stringadd]}}
    buf.append(" ").append(b);
}
void f2(char ch)
{
    OString s;
    // expected-error-re@+1 {{rather use O[U]String::Concat than constructing '{{(rtl::)?}}OString' from 'const char{{ ?}}[4]' on RHS of + (where LHS is of type '{{(rtl::)?}}OString') [loplugin:stringadd]}}
    s = s + OString("xxx");
    // expected-error-re@+1 {{rather use O[U]String::Concat than constructing '{{(rtl::)?}}OString' from 'char' on RHS of + (where LHS is of type '{{(rtl::)?}}OString') [loplugin:stringadd]}}
    s = s + OString(ch);
}
}

namespace test10
{
struct C
{
    OString constStringFunction(int) const;
    OString nonConstStringFunction();
    int constIntFunction() const;
    int nonConstIntFunction();
};

C getC();

void f1(C c)
{
    OString s;
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    s += c.constStringFunction(c.constIntFunction());
    s += c.constStringFunction(c.nonConstIntFunction());
    s += c.nonConstStringFunction();
    s += getC().constStringFunction(c.constIntFunction());
}
}

namespace test11
{
void f1()
{
    OUStringBuffer aFirstStr1("aaa");
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    aFirstStr1.append("...");
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    aFirstStr1.append("...");
}
}

namespace test12
{
void f1(int j)
{
    OUStringBuffer aFirstStr1(12);
    // no warning expected
    aFirstStr1.append("...");
    // expected-error@+1 {{simplify by merging with the preceding assign/append [loplugin:stringadd]}}
    aFirstStr1.append("...");
    // no warning expected
    aFirstStr1.append(((j + 1) % 15) ? " " : "\n");
}
}

namespace test13
{
void f1()
{
    OUStringBuffer aFirstStr1(12);
    // no warning expected
    aFirstStr1.append("...");
    // because we have a comment between them
    aFirstStr1.append("...");
}
}

namespace test14
{
void f1()
{
    OUStringBuffer b(16);
    b.append("...");
}

void f2(long long n)
{
    OUStringBuffer b(n);
    b.append("...");
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
