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

#include <X11/X.h>
#include <sal/types.h>

#include <unx/i18n_keysym.hxx>

// convert keysyms to unicode
// for all keysyms with byte1 and byte2 equal zero, and of course only for
// keysyms that have a unicode counterpart

namespace {

struct keymap_t {
    const int first; const int last;
    const sal_Unicode *map;
};

}

// Latin-1      Byte 3 = 0x00
const sal_Unicode keymap00_map[] = {
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x00a0, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x00ad, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff };
const keymap_t keymap00 = { 32, 255, keymap00_map };

// Latin-2      Byte 3 = 0x01
const sal_Unicode keymap01_map[] = {
    0x0104, 0x02d8, 0x0141, 0x0000, 0x013d, 0x015a, 0x0000, 0x0000,
    0x0160, 0x015e, 0x0164, 0x0179, 0x0000, 0x017d, 0x017b, 0x0000,
    0x0105, 0x02db, 0x0142, 0x0000, 0x013e, 0x015b, 0x02c7, 0x0000,
    0x0161, 0x015f, 0x0165, 0x017a, 0x02dd, 0x017e, 0x017c, 0x0154,
    0x0000, 0x0000, 0x0102, 0x0000, 0x0139, 0x0106, 0x0000, 0x010c,
    0x0000, 0x0118, 0x0000, 0x011a, 0x0000, 0x0000, 0x010e, 0x0110,
    0x0143, 0x0147, 0x0000, 0x0000, 0x0150, 0x0000, 0x0000, 0x0158,
    0x016e, 0x0000, 0x0170, 0x0000, 0x0000, 0x0162, 0x0000, 0x0155,
    0x0000, 0x0000, 0x0103, 0x0000, 0x013a, 0x0107, 0x0000, 0x010d,
    0x0000, 0x0119, 0x0000, 0x011b, 0x0000, 0x0000, 0x010f, 0x0111,
    0x0144, 0x0148, 0x0000, 0x0000, 0x0151, 0x0000, 0x0000, 0x0159,
    0x016f, 0x0000, 0x0171, 0x0000, 0x0000, 0x0163, 0x02d9 };
const keymap_t keymap01 = { 161, 255, keymap01_map };

// Latin-3      Byte 3 = 0x02
const sal_Unicode keymap02_map[] = {
    0x0126, 0x0000, 0x0000, 0x0000, 0x0000, 0x0124, 0x0000, 0x0000,
    0x0130, 0x0000, 0x011e, 0x0134, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0127, 0x0000, 0x0000, 0x0000, 0x0000, 0x0125, 0x0000, 0x0000,
    0x0131, 0x0000, 0x011f, 0x0135, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x010a, 0x0108, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0120, 0x0000, 0x0000, 0x011c,
    0x0000, 0x0000, 0x0000, 0x0000, 0x016c, 0x015c, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x010b, 0x0109, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0121, 0x0000, 0x0000, 0x011d,
    0x0000, 0x0000, 0x0000, 0x0000, 0x016d, 0x015d };
const keymap_t keymap02 = { 161, 254, keymap02_map };

// Latin-4      Byte 3 = 0x03
const sal_Unicode keymap03_map[] = {
    0x0138, 0x0156, 0x0000, 0x0128, 0x013b, 0x0000, 0x0000, 0x0000,
    0x0112, 0x0122, 0x0166, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0157, 0x0000, 0x0129, 0x013c, 0x0000, 0x0000, 0x0000,
    0x0113, 0x0123, 0x0167, 0x014a, 0x0000, 0x014b, 0x0100, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012e, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0116, 0x0000, 0x0000, 0x012a, 0x0000, 0x0145,
    0x014c, 0x0136, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0172,
    0x0000, 0x0000, 0x0000, 0x0168, 0x016a, 0x0000, 0x0101, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x012f, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0117, 0x0000, 0x0000, 0x012b, 0x0000, 0x0146,
    0x014d, 0x0137, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0173,
    0x0000, 0x0000, 0x0000, 0x0169, 0x016b };
const keymap_t keymap03 = { 162, 254, keymap03_map };

// Kana         Byte 3 = 0x04
const sal_Unicode keymap04_map[] = {
    0x203e, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x3002, 0x300c, 0x300d, 0x3001, 0x30fb,
    0x30f2, 0x30a1, 0x30a3, 0x30a5, 0x30a7, 0x30a9, 0x30e3, 0x30e5,
    0x30e7, 0x30c3, 0x30fc, 0x30a2, 0x30a4, 0x30a6, 0x30a8, 0x30aa,
    0x30ab, 0x30ad, 0x30af, 0x30b1, 0x30b3, 0x30b5, 0x30b7, 0x30b9,
    0x30bb, 0x30bd, 0x30bf, 0x30c1, 0x30c4, 0x30c6, 0x30c8, 0x30ca,
    0x30cb, 0x30cc, 0x30cd, 0x30ce, 0x30cf, 0x30d2, 0x30d5, 0x30d8,
    0x30db, 0x30de, 0x30df, 0x30e0, 0x30e1, 0x30e2, 0x30e4, 0x30e6,
    0x30e8, 0x30e9, 0x30ea, 0x30eb, 0x30ec, 0x30ed, 0x30ef, 0x30f3,
    0x309b, 0x309c };
const keymap_t keymap04 = { 126, 223, keymap04_map };

// Arabic       Byte 3 = 0x05
const sal_Unicode keymap05_map[] = {
    0x060c, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x061b,
    0x0000, 0x0000, 0x0000, 0x061f, 0x0000, 0x0621, 0x0622, 0x0623,
    0x0624, 0x0625, 0x0626, 0x0627, 0x0628, 0x0629, 0x062a, 0x062b,
    0x062c, 0x062d, 0x062e, 0x062f, 0x0630, 0x0631, 0x0632, 0x0633,
    0x0634, 0x0635, 0x0636, 0x0637, 0x0638, 0x0639, 0x063a, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0640, 0x0641, 0x0642, 0x0643,
    0x0644, 0x0645, 0x0646, 0x0647, 0x0648, 0x0649, 0x064a, 0x064b,
    0x064c, 0x064d, 0x064e, 0x064f, 0x0650, 0x0651, 0x0652 };
const keymap_t keymap05 = { 172, 242, keymap05_map };

// Cyrillic     Byte 3 = 0x06
const sal_Unicode keymap06_map[] = {
    0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457, 0x0458,
    0x0459, 0x045a, 0x045b, 0x045c, 0x0000, 0x045e, 0x045f, 0x2116,
    0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407, 0x0408,
    0x0409, 0x040a, 0x040b, 0x040c, 0x0000, 0x040e, 0x040f, 0x044e,
    0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433, 0x0445,
    0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
    0x044f, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432, 0x044c,
    0x044b, 0x0437, 0x0448, 0x044d, 0x0449, 0x0447, 0x044a, 0x042e,
    0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413, 0x0425,
    0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f,
    0x042f, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412, 0x042c,
    0x042b, 0x0417, 0x0428, 0x042d, 0x0429, 0x0427, 0x042a };
const keymap_t keymap06 = { 161, 255, keymap06_map };

// Greek        Byte 3 = 0x07
const sal_Unicode keymap07_map[] = {
    0x0386, 0x0388, 0x0389, 0x038a, 0x03aa, 0x0000, 0x038c, 0x038e,
    0x03ab, 0x0000, 0x038f, 0x0000, 0x0000, 0x0385, 0x2015, 0x0000,
    0x03ac, 0x03ad, 0x03ae, 0x03af, 0x03ca, 0x0390, 0x03cc, 0x03cd,
    0x03cb, 0x03b0, 0x03ce, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397, 0x0398,
    0x0399, 0x039a, 0x039b, 0x039c, 0x039d, 0x039e, 0x039f, 0x03a0,
    0x03a1, 0x03a3, 0x0000, 0x03a4, 0x03a5, 0x03a6, 0x03a7, 0x03a8,
    0x03a9, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x03b1, 0x03b2, 0x03b3, 0x03b4, 0x03b5, 0x03b6, 0x03b7, 0x03b8,
    0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf, 0x03c0,
    0x03c1, 0x03c3, 0x03c2, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8,
    0x03c9 };
const keymap_t keymap07 = { 161, 249, keymap07_map };

// Technical    Byte 3 = 0x08
const sal_Unicode keymap08_map[] = {
    0x23b7, 0x250c, 0x2500, 0x2320, 0x2321, 0x2502, 0x23a1, 0x23a3,
    0x23a4, 0x23a6, 0x239b, 0x239d, 0x239e, 0x23a0, 0x23a8, 0x23ac,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x2264, 0x2260, 0x2265, 0x222b, 0x2234,
    0x221d, 0x221e, 0x0000, 0x0000, 0x2207, 0x0000, 0x0000, 0x223c,
    0x2243, 0x0000, 0x0000, 0x0000, 0x21d4, 0x21d2, 0x2261, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x221a, 0x0000, 0x0000,
    0x0000, 0x2282, 0x2283, 0x2229, 0x222a, 0x2227, 0x2228, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2202, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0192, 0x0000, 0x0000,
    0x0000, 0x0000, 0x2190, 0x2191, 0x2192, 0x2193 };
const keymap_t keymap08 = { 161, 254, keymap08_map };

// Special      Byte 3 = 0x09
const sal_Unicode keymap09_map[] = {
    0x25c6, 0x2592, 0x2409, 0x240c, 0x240d, 0x240a, 0x0000, 0x0000,
    0x2424, 0x240b, 0x2518, 0x2510, 0x250c, 0x2514, 0x253c, 0x23ba,
    0x23bb, 0x2500, 0x23bc, 0x23bd, 0x251c, 0x2524, 0x2534, 0x252c,
    0x2502 };
const keymap_t keymap09 = { 224, 248, keymap09_map };

// Publishing   Byte 3 = 0x0a = 10
const sal_Unicode keymap10_map[] = {
    0x2003, 0x2002, 0x2004, 0x2005, 0x2007, 0x2008, 0x2009, 0x200a,
    0x2014, 0x2013, 0x0000, 0x0000, 0x0000, 0x2026, 0x2025, 0x2153,
    0x2154, 0x2155, 0x2156, 0x2157, 0x2158, 0x2159, 0x215a, 0x2105,
    0x0000, 0x0000, 0x2012, 0x2329, 0x0000, 0x232a, 0x0000, 0x0000,
    0x0000, 0x0000, 0x215b, 0x215c, 0x215d, 0x215e, 0x0000, 0x0000,
    0x2122, 0x2613, 0x0000, 0x25c1, 0x25b7, 0x25cb, 0x25af, 0x2018,
    0x2019, 0x201c, 0x201d, 0x211e, 0x0000, 0x2032, 0x2033, 0x0000,
    0x271d, 0x0000, 0x25ac, 0x25c0, 0x25b6, 0x25cf, 0x25ae, 0x25e6,
    0x25ab, 0x25ad, 0x25b3, 0x25bd, 0x2606, 0x2022, 0x25aa, 0x25b2,
    0x25bc, 0x261c, 0x261e, 0x2663, 0x2666, 0x2665, 0x0000, 0x2720,
    0x2020, 0x2021, 0x2713, 0x2717, 0x266f, 0x266d, 0x2642, 0x2640,
    0x260e, 0x2315, 0x2117, 0x2038, 0x201a, 0x201e };
const keymap_t keymap10 = { 161, 254, keymap10_map };

// APL      Byte 3 = 0x0b = 11
const sal_Unicode keymap11_map[] = {
    0x003c, 0x0000, 0x0000, 0x003e, 0x0000, 0x2228, 0x2227, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00af, 0x0000, 0x22a5,
    0x2229, 0x230a, 0x0000, 0x005f, 0x0000, 0x0000, 0x0000, 0x2218,
    0x0000, 0x2395, 0x0000, 0x22a4, 0x25cb, 0x0000, 0x0000, 0x0000,
    0x2308, 0x0000, 0x0000, 0x222a, 0x0000, 0x2283, 0x0000, 0x2282,
    0x0000, 0x22a2, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x22a3 };
const keymap_t keymap11 = { 163, 252, keymap11_map };

// Hebrew   Byte 3 = 0x0c = 12
const sal_Unicode keymap12_map[] = {
    0x2017, 0x05d0, 0x05d1, 0x05d2, 0x05d3, 0x05d4, 0x05d5, 0x05d6,
    0x05d7, 0x05d8, 0x05d9, 0x05da, 0x05db, 0x05dc, 0x05dd, 0x05de,
    0x05df, 0x05e0, 0x05e1, 0x05e2, 0x05e3, 0x05e4, 0x05e5, 0x05e6,
    0x05e7, 0x05e8, 0x05e9, 0x05ea };
const keymap_t keymap12 = { 223, 250, keymap12_map };

// Thai     Byte 3 = 0x0d = 13
const sal_Unicode keymap13_map[] = {
    0x0e01, 0x0e02, 0x0e03, 0x0e04, 0x0e05, 0x0e06, 0x0e07, 0x0e08,
    0x0e09, 0x0e0a, 0x0e0b, 0x0e0c, 0x0e0d, 0x0e0e, 0x0e0f, 0x0e10,
    0x0e11, 0x0e12, 0x0e13, 0x0e14, 0x0e15, 0x0e16, 0x0e17, 0x0e18,
    0x0e19, 0x0e1a, 0x0e1b, 0x0e1c, 0x0e1d, 0x0e1e, 0x0e1f, 0x0e20,
    0x0e21, 0x0e22, 0x0e23, 0x0e24, 0x0e25, 0x0e26, 0x0e27, 0x0e28,
    0x0e29, 0x0e2a, 0x0e2b, 0x0e2c, 0x0e2d, 0x0e2e, 0x0e2f, 0x0e30,
    0x0e31, 0x0e32, 0x0e33, 0x0e34, 0x0e35, 0x0e36, 0x0e37, 0x0e38,
    0x0e39, 0x0e3a, 0x0000, 0x0000, 0x0000, 0x0000, 0x0e3f, 0x0e40,
    0x0e41, 0x0e42, 0x0e43, 0x0e44, 0x0e45, 0x0e46, 0x0e47, 0x0e48,
    0x0e49, 0x0e4a, 0x0e4b, 0x0e4c, 0x0e4d, 0x0000, 0x0000, 0x0e50,
    0x0e51, 0x0e52, 0x0e53, 0x0e54, 0x0e55, 0x0e56, 0x0e57, 0x0e58,
    0x0e59 };
const keymap_t keymap13 = { 161, 249, keymap13_map };

// Korean       Byte 3 = 0x0e = 14
const sal_Unicode keymap14_map[] = {
    0x3131, 0x3132, 0x3133, 0x3134, 0x3135, 0x3136, 0x3137, 0x3138,
    0x3139, 0x313a, 0x313b, 0x313c, 0x313d, 0x313e, 0x313f, 0x3140,
    0x3141, 0x3142, 0x3143, 0x3144, 0x3145, 0x3146, 0x3147, 0x3148,
    0x3149, 0x314a, 0x314b, 0x314c, 0x314d, 0x314e, 0x314f, 0x3150,
    0x3151, 0x3152, 0x3153, 0x3154, 0x3155, 0x3156, 0x3157, 0x3158,
    0x3159, 0x315a, 0x315b, 0x315c, 0x315d, 0x315e, 0x315f, 0x3160,
    0x3161, 0x3162, 0x3163, 0x11a8, 0x11a9, 0x11aa, 0x11ab, 0x11ac,
    0x11ad, 0x11ae, 0x11af, 0x11b0, 0x11b1, 0x11b2, 0x11b3, 0x11b4,
    0x11b5, 0x11b6, 0x11b7, 0x11b8, 0x11b9, 0x11ba, 0x11bb, 0x11bc,
    0x11bd, 0x11be, 0x11bf, 0x11c0, 0x11c1, 0x11c2, 0x316d, 0x3171,
    0x3178, 0x317f, 0x3181, 0x3184, 0x3186, 0x318d, 0x318e, 0x11eb,
    0x11f0, 0x11f9, 0x0000, 0x0000, 0x0000, 0x0000, 0x20a9 };
const keymap_t keymap14 = { 161, 255, keymap14_map };

// missing:
// Latin-8      Byte 3 = 0x12 = 18

// Latin-9      Byte 3 = 0x13 = 19
const sal_Unicode keymap19_map[] = {
    0x0152, 0x0153, 0x0178 };
const keymap_t keymap19 = { 188, 190, keymap19_map };

// missing:
// Armenian     Byte 3 = 0x14 = 20
// Georgian     Byte 3 = 0x15 = 21
// Azeri        Byte 3 = 0x16 = 22
// Vietnamese   Byte 3 = 0x1e = 30

// Currency     Byte 3 = 0x20 = 32
const sal_Unicode keymap32_map[] = {
    0x20a0, 0x20a1, 0x20a2, 0x20a3, 0x20a4, 0x20a5, 0x20a6, 0x20a7,
    0x20a8, 0x0000, 0x20aa, 0x20ab, 0x20ac };
const keymap_t keymap32 = { 160, 172, keymap32_map };

// Keyboard (Keypad mappings) Byte 3 = 0xff = 255
const sal_Unicode keymap255_map[] = {
    0x0020, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x0000, 0x0000, 0x0000, 0x003d };
const keymap_t keymap255 = { 128, 189, keymap255_map };

#define INITIAL_KEYMAPS 33
const keymap_t* const p_keymap[INITIAL_KEYMAPS] = {
    &keymap00, &keymap01, &keymap02, &keymap03,                         /* 00 -- 03 */
    &keymap04, &keymap05, &keymap06, &keymap07,                         /* 04 -- 07 */
    &keymap08, &keymap09, &keymap10, &keymap11,                         /* 08 -- 11 */
    &keymap12, &keymap13, &keymap14, nullptr,                   /* 12 -- 15 */
    nullptr, nullptr, nullptr, &keymap19,       /* 16 -- 19 */
    nullptr, nullptr, nullptr, nullptr, /* 20 -- 23 */
    nullptr, nullptr, nullptr, nullptr, /* 24 -- 27 */
    nullptr, nullptr, nullptr, nullptr, /* 28 -- 31 */
    &keymap32                                                           /* 32 */
};

sal_Unicode
KeysymToUnicode (KeySym nKeySym)
{
    // keysym is already unicode
    if ((nKeySym & 0xff000000) == 0x01000000)
    {
        // strip off group indicator and iso10646 plane
        // FIXME can't handle chars from surrogate area.
        if (! (nKeySym & 0x00ff0000) )
            return static_cast<sal_Unicode>(nKeySym & 0x0000ffff);
    }
    // legacy keysyms, switch to appropriate codeset
    else
    {
        unsigned char n_byte1 = (nKeySym & 0xff000000) >> 24;
        unsigned char n_byte2 = (nKeySym & 0x00ff0000) >> 16;
        unsigned char n_byte3 = (nKeySym & 0x0000ff00) >>  8;
        unsigned char n_byte4 = (nKeySym & 0x000000ff);

        if (n_byte1 != 0)
            return 0;
        if (n_byte2 != 0)
            return 0;

        keymap_t const* p_map = nullptr;
        if (n_byte3 < INITIAL_KEYMAPS)
            p_map = p_keymap[n_byte3];
        else if (n_byte3 == 255)
            p_map = &keymap255;

        if ((p_map != nullptr) && (n_byte4 >= p_map->first) && (n_byte4 <= p_map->last) )
            return p_map->map[n_byte4 - p_map->first];
    }

    return 0;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
