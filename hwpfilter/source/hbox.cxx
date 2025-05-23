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

#include "precompile.h"


#include "hwpfile.h"
#include "hbox.h"
#include "hpara.h"
#include "hutil.h"
#include "htags.h"
#include "drawdef.h"
#include "hcode.h"
#include "datecode.h"

#include <o3tl/sprintf.hxx>
#include <rtl/character.hxx>
#include <rtl/strbuf.hxx>
#include <rtl/string.hxx>

int HBox::boxCount = 0;

HBox::HBox(hchar hch)
{
    hh = hch;
    boxCount++;
}


HBox::~HBox()
{
    boxCount--;
}


int HBox::WSize()
{
    static const int wsize[32] =
    {
        1, 4, 4, 4, 4, 4, 4, 42,                  /* dateform */
        48, 4, 4, 4, 4, 1, 4, 4,                  /* hidden */
        4, 4, 4, 4, 4, 4, 12, 5,                  /* chcompose */
        3, 3, 123, 4, 32, 4, 2, 2
    };

    if (hh < 32)
        return wsize[hh];
    else
        return 1;
}


// skip block
SkipData::SkipData(hchar hch)
    : HBox(hch)
{
}

SkipData::~SkipData()
{
}


// FieldCode [5]
FieldCode::FieldCode()
    : HBox(CH_FIELD)
    , location_info(0)
{
}

FieldCode::~FieldCode()
{
}

// book mark(6)
Bookmark::Bookmark()
    : HBox(CH_BOOKMARK)
    , dummy(0)
    , type(0)
{
}

Bookmark::~Bookmark()
{
}

// date format(7)
DateFormat::DateFormat()
    : HBox(CH_DATE_FORM)
    , dummy(0)
{
}

// date code(8)
DateCode::DateCode()
    : HBox(CH_DATE_CODE)
    , format{0}
    , date{0}
    , dummy(0)
    , key(0)
{
}

const hchar kor_week[] =
{
    0xB7A9, 0xB6A9, 0xD1C1, 0xAE81, 0xA1A2, 0x8B71, 0xC9A1
};
const hchar china_week[] =
{
    0x4CC8, 0x4BE4, 0x525A, 0x48D8, 0x45AB, 0x4270, 0x50B4
};
const char eng_week[] = { "SunMonTueWedThuFriSat" };
const char eng_mon[] = { "JanFebMarAprMayJunJulAugSepOctNovDec" };
const char * const en_mon[] =
{
    "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December"
};
const char * const en_week[] =
{
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

hchar_string DateCode::GetString()
{
    hchar_string ret;
    const hchar *fmt;
    int i, num;
    const char *form;
    char cbuf[256];
    bool is_pm, add_zero;

    add_zero = false;
    format[DATE_SIZE - 1] = 0;
    fmt = format[0] ? format : defaultform;

    for (; *fmt && (ret.size() < DATE_SIZE); fmt++)
    {
        form = add_zero ? "%02d" : "%d";

        add_zero = false;
        is_pm = (date[HOUR] >= 12);
        *cbuf = 0;
        num = -1;

        switch (*fmt)
        {
        case '0':
            add_zero = true;
            break;
        case '1':
            num = date[YEAR];
            form = "%04d";
            break;
        case '!':
            num = date[YEAR] % 100;
            break;
        case '2':
            num = date[MONTH];
            break;
        case '@':
        {
            static_assert((std::size(eng_mon) - 1) / 3 == 12);
            size_t nIndex = o3tl::make_unsigned(date[MONTH] - 1) % 12;
            memcpy(cbuf, eng_mon + nIndex * 3, 3);
            cbuf[3] = '.';
            cbuf[4] = 0;
            break;
        }
        case '*':
        {
            size_t nIndex = o3tl::make_unsigned(date[MONTH] - 1) % std::size(en_mon);
            strncat(cbuf, en_mon[nIndex], sizeof(cbuf) - strlen(cbuf) - 1);
            break;
        }
        case '3':                             /* 'D' is day of korean */
            num = date[DAY];
            break;
        case '#':
            num = date[DAY];
            switch (date[DAY] % 10)
            {
            case 1:
                form = "%dst";
                break;
            case 2:
                form = "%dnd";
                break;
            case 3:
                form = "%drd";
                break;
            default:
                form = "%dth";
                break;
            }
            break;
        case '4':
            num = date[HOUR] - ((date[HOUR] > 12) ? 12 : 0);
            break;
        case '$':
            num = date[HOUR];
            break;
        case '5':
        case '%':
            num = date[MIN];
            break;
        case '6':
        {
            size_t nIndex = o3tl::make_unsigned(date[WEEK]) % std::size(kor_week);
            ret.push_back(kor_week[nIndex]);
            break;
        }
        case '^':
        {
            static_assert((std::size(eng_week) - 1) / 3 == 7);
            size_t nIndex = o3tl::make_unsigned(date[WEEK]) % 7;
            memcpy(cbuf, eng_week + nIndex * 3, 3);
            cbuf[3] = '.';
            cbuf[4] = 0;
            break;
        }
        case '_':
        {
            size_t nIndex = o3tl::make_unsigned(date[WEEK]) % std::size(en_week);
            strncat(cbuf, en_week[nIndex], sizeof(cbuf) - strlen(cbuf) - 1);
            break;
        }
        case '7':
            ret.push_back(0xB5A1);
            ret.push_back(is_pm ? 0xD281 : 0xB8E5);
            break;
        case '&':
            strncat(cbuf, is_pm ? "p.m." : "a.m.", sizeof(cbuf) - strlen(cbuf) - 1);
            break;
        case '+':
            strncat(cbuf, is_pm ? "P.M." : "A.M.", sizeof(cbuf) - strlen(cbuf) - 1);
            break;
        case '8':                             // 2.5 feature
        case '9':
#if 0
// LATER
            mkcurfilename(cbuf, *fmt);
            for (i = 0; cbuf[i] != 0 && slen > 1; i++)
            {                                 //for hangle filename
                if (cbuf[i] & 0x80 && cbuf[i + 1] != 0)
                {
                    *d++ = (cbuf[i] << 8) | cbuf[i + 1];
                    i++;
                }
                else
                    *d++ = cbuf[i];
                slen--;
            }
#endif
            cbuf[0] = 0;
            break;
        case '~':                             // 3.0b feature
            if (fmt[1] == 0)
                break;
            fmt++;
            if (*fmt == '6')
            {
                size_t nIndex = o3tl::make_unsigned(date[WEEK]) % std::size(china_week);
                ret.push_back(china_week[nIndex]);
                break;
            }
            break;
        default:
            if (*fmt == '\\' && *++fmt == 0)
                goto done;
            ret.push_back(*fmt);
        }
        if (num != -1)
            o3tl::sprintf(cbuf, form, num);
        for (i = 0; 0 != cbuf[i]; i++)
        {
            ret.push_back(*(cbuf + i));
        }
    }
    done:
    return ret;
}

// tab(9)
Tab::Tab()
    : HBox(CH_TAB)
    , width(0)
    , leader(0)
    , dummy(0)
{
}

// floating box
FBox::FBox(hchar hch)
    : HBox(hch)
    , zorder(0)
    , option(0)
    , ctrl_ch(0)
    , box_xs(0)
    , box_ys(0)
    , cap_xs(0)
    , cap_ys(0)
    , xs(0)
    , ys(0)
    , cap_margin(0)
    , xpos_type(0)
    , ypos_type(0)
    , smart_linesp(0)
    , boundsy(0)
    , boundey(0)
    , boundx(0)
    , draw(0)
    , pgx(0)
    , pgy(0)
    , pgno(0)
    , showpg(0)
{
}

FBox::~FBox()
{
}

// tbox(10) TABLE BOX MATH BUTTON HYPERTEXT
TxtBox::TxtBox()
    : FBox(CH_TEXT_BOX)
    , dummy(0)
    , dummy1(0)
    , cap_len(0)
    , next_box(0)
    , dummy2(0)
    , reserved1(0)
    , cap_pos(0)
    , num(0)
    , dummy3(0)
    , baseline(0)
    , type(0)
    , nCell(0)
    , protect(0)
    , m_pTable(nullptr)
{
    reserved[0] = reserved[1] = 0;
}

TxtBox::~TxtBox()
{
}

// picture(11)

Picture::Picture()
    : FBox(CH_PICTURE)
    , reserved{0}
    , dummy(0)
    , follow_block_size(0)
    , dummy1(0)
    , dummy2(0)
    , reserved1(0)
    , cap_pos(0)
    , num(0)
    , pictype(0)
    , skip{0}
    , scale{0}
    , picinfo{}
    , reserved3{0}
    , ishyper(false)
{
}

Picture::~Picture()
{
    if (pictype == PICTYPE_DRAW)
        delete picinfo.picdraw.hdo;
}


// line(14)
// hidden(15)
Hidden::~Hidden()
{
}


// header/footer(16)
HeaderFooter::~HeaderFooter()
{
}


// footnote(17)
Footnote::~Footnote()
{
}


// auto number(18)
// new number(19)
// show page number (20)
// Start/Hide odd-numbered side (21)

// mail merge(22)
hchar_string MailMerge::GetString()
{
    return hchar_string();
}


// character composition(23)
// hyphen(24)
// toc mark(25)
// index mark(26)
// outline(28)

#define OL_HANGL_JASO   0
#define OL_HANGL_KANATA 1

static hchar olHanglJaso(unsigned int num, int type)
{
    static const unsigned char han_init[] =
        { 0x88, 0x90, 0x94, 0x9c, 0xa0, 0xa4, 0xac, 0xb4, 0xb8, 0xc0, 0xc4, 0xc8, 0xcc, 0xd0 };
    static const unsigned char jung[] = { 3, 5, 7, 11, 13, 19, 20, 26, 27, 29, 30 };
    static const unsigned char jung2[] = { 3, 7, 13, 20, 27, 29, 30 };

    hchar hh = 0;

    if (type == OL_HANGL_JASO)
    {
        num = num % (14 + SAL_N_ELEMENTS(jung));

        if (num < 14)
            hh = (han_init[num] << 8) | 'A';
        else
            hh = (jung[num - 14] << 5) | 0x8401;
    }
    else
    {
        if (num < 14)
            hh = (han_init[num] << 8) | 'a';
        else
        {
            int j = (num / 14) % SAL_N_ELEMENTS(jung2);

            num = num % 14;
            hh = (han_init[num] << 8) | (jung2[j] << 5) | 1;
        }
    }
    return hh;
}


static const hchar *GetOutlineStyleChars(int style)
{
    static const hchar out_bul_style_entry[5][MAX_OUTLINE_LEVEL+1] = // extern
    {
        {                                         // 0 OLSTY_BULLET1
            0x2f18, 0x2f12, 0x2f08, 0x2f02, 0x2f06, 0x2f00, 0x2043, 0x0000
        },
        {                                         // 1
            0x2f18, 0x2f12, 0x2f06, 0x2f00, 0x2f36, 0x2f30, 0x2043, 0x0000
        },
        {                                         // 2
            0x2f26, 0x2f20, 0x2f06, 0x2f00, 0x2f16, 0x2f10, 0x2043, 0x0000
        },
        {                                         // 3
            0x2f18, 0x2f16, 0x2f12, 0x2f10, 0x2f06, 0x2f00, 0x2043, 0x0000
        },
        {
            0xAC61, 0xB677, 0xB861, 0xB8F7, 0xB781, 0x0000, 0x0000, 0x0000
        },
    };
    if (style >= OLSTY_BULLET1 && style <= OLSTY_BULLET5)
        return out_bul_style_entry[style - OLSTY_BULLET1];
    return nullptr;
}


static void getOutlineNumStr(int style, int level, int num, hchar * hstr)
{
    enum
    {
        U_ROM = 0x01, L_ROM = 0x02, U_ENG = 0x04, L_ENG = 0x08,
        HAN = 0x10, NUM = 0x20, L_BR = 0x40, R_BR = 0x80
    };
    static const unsigned char type_tbl[][MAX_OUTLINE_LEVEL] =
    {
        {
            U_ROM, HAN, NUM, HAN | R_BR, L_BR | NUM | R_BR,
            L_BR | HAN | R_BR, L_ROM | R_BR
        },
        {
            U_ROM, U_ENG, NUM, L_ENG | R_BR, L_BR | NUM | R_BR,
            L_BR | L_ENG | R_BR, L_ROM | R_BR
        },
        {
            NUM, HAN, L_BR | NUM | R_BR, L_BR | HAN | R_BR, NUM |
            R_BR, HAN | R_BR, L_ENG
        }
    };
    char fmt = type_tbl[style - OLSTY_NUMSIG1][level];
    char buf[80], *ptr;

    if (num < 1)
        num = 1;
    if (fmt & L_BR)
        *hstr++ = '(';
    if (fmt & NUM)
    {
        auto const numbuf = OString::number(num);
        str2hstr(numbuf.buf, hstr);
        hstr += numbuf.length;
    }
    else if (fmt & (U_ROM | L_ROM))
    {
        num2roman(num, buf);
        if (fmt & U_ROM)
        {
            ptr = buf;
            while (*ptr)
            {
                *ptr = sal::static_int_cast<char>(
                    rtl::toAsciiUpperCase(static_cast<unsigned char>(*ptr)));
                ptr++;
            }
        }
        str2hstr(buf, hstr);
        hstr += strlen(buf);
    }
    else
    {
        num = (num - 1) % 26;
        if (fmt & U_ENG)
            *hstr++ = sal::static_int_cast<hchar>('A' + num);
        else if (fmt & L_ENG)
            *hstr++ = sal::static_int_cast<hchar>('a' + num);
        else if (fmt & HAN)
            *hstr++ = olHanglJaso(num, OL_HANGL_KANATA);
    }
    *hstr++ = (fmt & R_BR) ? ')' : '.';
    *hstr = 0;
}


enum
{ OUTLINE_ON, OUTLINE_NUM };

/* level starts from zero. ex) '1.1.1.' is the level 2.
   number has the value. ex) '1.2.1' has '1,2,1'
   style has the value which starts from 1 according to the definition in hbox.h
 */
OUString Outline::GetUnicode() const
{
    const hchar *p;
    hchar buffer[255];

    buffer[0] = 0;
    if (kind == OUTLINE_NUM)
    {
        int levelnum;
        switch (shape)
        {
            case OLSTY_NUMS1:
            case OLSTY_NUMS2:
            {
                OStringBuffer buf;
                for (unsigned int i = 0; i <= level; ++i)
                {
                    if (i >= std::size(number))
                        break;

                    levelnum = ((number[i] < 1) ? 1 : number[i]);
                    buf.append(OString::number(levelnum));
                    if (!(shape == OLSTY_NUMS2 && i && i == level))
                        buf.append('.');
                }
                str2hstr(buf.getStr(), buffer);
                return hstr2OUString(buffer);
            }
            case OLSTY_NUMSIG1:
            case OLSTY_NUMSIG2:
            case OLSTY_NUMSIG3:
            {
                if (level < std::size(number))
                    getOutlineNumStr(shape, level, number[level], buffer);
                return hstr2OUString(buffer);
            }
            case OLSTY_BULLET1:
            case OLSTY_BULLET2:
            case OLSTY_BULLET3:
            case OLSTY_BULLET4:
            case OLSTY_BULLET5:
            {
                if (level < MAX_OUTLINE_LEVEL)
                {
                    p = GetOutlineStyleChars(shape);
                    buffer[0] = p[level];
                    buffer[1] = 0;
                }
                return hstr2OUString(buffer);
            }
            case OLSTY_USER:
            case OLSTY_BULUSER:
                {
                    char dest[80];
                    int l = 0;
                    unsigned i = level;
                    if (i < std::size(deco) && deco[i][0]) {
                        buffer[l++] = deco[i][0];
                    }
                    if (i < std::size(user_shape))
                    {
                        /* level starts from zero. ex) '1.1.1.' is the level 2.
                           number has the value. ex) '1.2.1' has '1,2,1'
                           style has the value which starts from 1 according to the definition in hbox.h */
                        switch( user_shape[i] )
                        {
                            case 0:
                                buffer[l++] = '1' + number[i] - 1;
                                break;
                            case 1: /* Uppercase Roman */
                            case 2: /* Lowercase Roman */
                                num2roman(number[i], dest);
                                if( user_shape[i] == 1 ){
                                    char *ptr = dest;
                                    while( *ptr )
                                    {
                                        *ptr = sal::static_int_cast<char>(rtl::toAsciiUpperCase(static_cast<unsigned char>(*ptr)));
                                        ptr++;
                                    }
                                }
                                str2hstr(dest, buffer + l);
                                l += strlen(dest);
                                break;
                            case 3:
                                buffer[l++] = 'A' + number[i] -1;
                                break;
                            case 4:
                                buffer[l++] = 'a' + number[i] -1;
                                break;
                            case 5:
                                buffer[l++] = olHanglJaso(number[i] -1, OL_HANGL_KANATA);
                                break;
                            case 6:
                                buffer[l++] = olHanglJaso(number[i] -1, OL_HANGL_JASO);
                                break;
                            case 7: /* Chinese numbers: the number represented by the general */
                                buffer[l++] = '1' + number[i] -1;
                                break;
                            case 8: /* Circled numbers */
                                buffer[l++] = 0x2e00 + number[i];
                                break;
                            case 9: /* Circled lowercase alphabet */
                                buffer[l++] = 0x2c20 + number[i];
                                break;
                            case 10: /* Circled Korean Alphabet */
                                buffer[l++] = 0x2c50 + number[i] -1;
                                break;
                            case 11: /* Circled Korean Characters */
                                buffer[l++] = 0x2c40 + number[i] -1;
                                break;
                            case 12: /* Sequenced numbers. */
                            {
                                 OStringBuffer buf;
                                 int j;
                                 for (j = 0; j <= level; j++)
                                 {
                                      levelnum = ((number[j] < 1) ? 1 : number[j]);
                                      buf.append(OString::number(levelnum));
                                      if (!((j && j == level) || (j == level && deco[i][1])))
                                            buf.append('.');
                                 }
                                 str2hstr(buf.getStr(), buffer + l);
                                 l += buf.getLength();
                                 break;
                            }
                            default:
                                buffer[l++] = user_shape[i];
                                break;
                        }
                    }
                    if (i < std::size(deco) && deco[i][1]) {
                        buffer[l++] = deco[i][1];
                    }
                    buffer[l] = 0;
                    return hstr2OUString(buffer);
                }
        }
    }
    return hstr2OUString(buffer);
}


/* Bundle of spaces (30) */
/* Fixed-width spaces (31) */

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
