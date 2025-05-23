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


#include <svl/numformat.hxx>
#include <svl/zformat.hxx>
#include <svl/macitem.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>

#include <svtools/HtmlWriter.hxx>
#include <svtools/htmlout.hxx>
#include <svtools/htmlkywd.hxx>
#include <vcl/imap.hxx>
#include <vcl/imaprect.hxx>
#include <vcl/imapcirc.hxx>
#include <vcl/imappoly.hxx>
#include <svl/urihelper.hxx>
#include <rtl/character.hxx>
#include <tools/debug.hxx>
#include <o3tl/string_view.hxx>

#include <sstream>

#define TXTCONV_BUFFER_SIZE 20

static sal_Size convertUnicodeToText(const sal_Unicode* pSrcBuf, sal_Size nSrcChars, char* pDestBuf,
                                     sal_Size nDestBytes, sal_uInt32 nFlags, sal_uInt32* pInfo,
                                     sal_Size* pSrcCvtChars)
{
    static rtl_UnicodeToTextConverter hConverter
        = rtl_createUnicodeToTextConverter(RTL_TEXTENCODING_UTF8);
    static rtl_UnicodeToTextContext hContext = hConverter
                                                   ? rtl_createUnicodeToTextContext(hConverter)
                                                   : reinterpret_cast<rtl_TextToUnicodeContext>(1);

    return rtl_convertUnicodeToText(hConverter, hContext, pSrcBuf, nSrcChars, pDestBuf, nDestBytes,
                                    nFlags, pInfo, pSrcCvtChars);
}

static const char *lcl_svhtml_GetEntityForChar( sal_uInt32 c,
                                             rtl_TextEncoding eDestEnc )
{
    const char* pStr = nullptr;

    // Note: We currently handle special cases for ISO-8859-2 here simply because
    // the code was already submitted.  But we should also handle other code pages
    // as well as the code becomes available.

    if( eDestEnc == RTL_TEXTENCODING_ISO_8859_2 || eDestEnc == RTL_TEXTENCODING_MS_1250 )
    {
        // Don't handle the following characters for Easter European (ISO-8859-2).
        switch ( c )
        {
        case 164: // curren
        case 184: // ccedil
        case 193: // Aacute
        case 194: // Acirc
        case 196: // Auml
        case 199: // Ccedil
        case 201: // Eacute
        case 203: // Euml
        case 205: // Iacute
        case 206: // Icirc
        case 211: // Oacute
        case 212: // Ocirc
        case 214: // Ouml
        case 215: // times
        case 218: // Uacute
        case 220: // Uuml
        case 221: // Yacute
        case 225: // aacute
        case 226: // acirc
        case 228: // auml
        case 233: // eacute
        case 235: // euml
        case 237: // iacute
        case 238: // icirc
        case 243: // oacute
        case 244: // ocirc
        case 246: // ouml
        case 247: // divide
        case 250: // uacute
        case 252: // uuml
        case 253: // yacute
        case 352: // Scaron
        case 353: // scaron
            return pStr;
        }
    }

    // TODO: handle more special cases for other code pages.

    switch( c )
    {
//      case '\x0a':   return HTMLOutFuncs::Out_Tag( rStream, OOO_STRING_SVTOOLS_HTML_linebreak );

    case '<':       pStr = OOO_STRING_SVTOOLS_HTML_C_lt;        break;
    case '>':       pStr = OOO_STRING_SVTOOLS_HTML_C_gt;        break;
    case '&':       pStr = OOO_STRING_SVTOOLS_HTML_C_amp;       break;
    case '"':       pStr = OOO_STRING_SVTOOLS_HTML_C_quot;  break;

    case 161:       pStr = OOO_STRING_SVTOOLS_HTML_S_iexcl; break;
    case 162:       pStr = OOO_STRING_SVTOOLS_HTML_S_cent;  break;
    case 163:       pStr = OOO_STRING_SVTOOLS_HTML_S_pound; break;
    case 164:       pStr = OOO_STRING_SVTOOLS_HTML_S_curren;    break;
    case 165:       pStr = OOO_STRING_SVTOOLS_HTML_S_yen;       break;
    case 166:       pStr = OOO_STRING_SVTOOLS_HTML_S_brvbar;    break;
    case 167:       pStr = OOO_STRING_SVTOOLS_HTML_S_sect;  break;
    case 168:       pStr = OOO_STRING_SVTOOLS_HTML_S_uml;       break;
    case 169:       pStr = OOO_STRING_SVTOOLS_HTML_S_copy;  break;
    case 170:       pStr = OOO_STRING_SVTOOLS_HTML_S_ordf;  break;
    case 171:       pStr = OOO_STRING_SVTOOLS_HTML_S_laquo; break;
    case 172:       pStr = OOO_STRING_SVTOOLS_HTML_S_not;       break;
    case 174:       pStr = OOO_STRING_SVTOOLS_HTML_S_reg;       break;
    case 175:       pStr = OOO_STRING_SVTOOLS_HTML_S_macr;  break;
    case 176:       pStr = OOO_STRING_SVTOOLS_HTML_S_deg;       break;
    case 177:       pStr = OOO_STRING_SVTOOLS_HTML_S_plusmn;    break;
    case 178:       pStr = OOO_STRING_SVTOOLS_HTML_S_sup2;  break;
    case 179:       pStr = OOO_STRING_SVTOOLS_HTML_S_sup3;  break;
    case 180:       pStr = OOO_STRING_SVTOOLS_HTML_S_acute; break;
    case 181:       pStr = OOO_STRING_SVTOOLS_HTML_S_micro; break;
    case 182:       pStr = OOO_STRING_SVTOOLS_HTML_S_para;  break;
    case 183:       pStr = OOO_STRING_SVTOOLS_HTML_S_middot;    break;
    case 184:       pStr = OOO_STRING_SVTOOLS_HTML_S_cedil; break;
    case 185:       pStr = OOO_STRING_SVTOOLS_HTML_S_sup1;  break;
    case 186:       pStr = OOO_STRING_SVTOOLS_HTML_S_ordm;  break;
    case 187:       pStr = OOO_STRING_SVTOOLS_HTML_S_raquo; break;
    case 188:       pStr = OOO_STRING_SVTOOLS_HTML_S_frac14;    break;
    case 189:       pStr = OOO_STRING_SVTOOLS_HTML_S_frac12;    break;
    case 190:       pStr = OOO_STRING_SVTOOLS_HTML_S_frac34;    break;
    case 191:       pStr = OOO_STRING_SVTOOLS_HTML_S_iquest;    break;

    case 192:       pStr = OOO_STRING_SVTOOLS_HTML_C_Agrave;    break;
    case 193:       pStr = OOO_STRING_SVTOOLS_HTML_C_Aacute;    break;
    case 194:       pStr = OOO_STRING_SVTOOLS_HTML_C_Acirc; break;
    case 195:       pStr = OOO_STRING_SVTOOLS_HTML_C_Atilde;    break;
    case 196:       pStr = OOO_STRING_SVTOOLS_HTML_C_Auml;  break;
    case 197:       pStr = OOO_STRING_SVTOOLS_HTML_C_Aring; break;
    case 198:       pStr = OOO_STRING_SVTOOLS_HTML_C_AElig; break;
    case 199:       pStr = OOO_STRING_SVTOOLS_HTML_C_Ccedil;    break;
    case 200:       pStr = OOO_STRING_SVTOOLS_HTML_C_Egrave;    break;
    case 201:       pStr = OOO_STRING_SVTOOLS_HTML_C_Eacute;    break;
    case 202:       pStr = OOO_STRING_SVTOOLS_HTML_C_Ecirc; break;
    case 203:       pStr = OOO_STRING_SVTOOLS_HTML_C_Euml;  break;
    case 204:       pStr = OOO_STRING_SVTOOLS_HTML_C_Igrave;    break;
    case 205:       pStr = OOO_STRING_SVTOOLS_HTML_C_Iacute;    break;
    case 206:       pStr = OOO_STRING_SVTOOLS_HTML_C_Icirc; break;
    case 207:       pStr = OOO_STRING_SVTOOLS_HTML_C_Iuml;  break;
    case 208:       pStr = OOO_STRING_SVTOOLS_HTML_C_ETH;       break;
    case 209:       pStr = OOO_STRING_SVTOOLS_HTML_C_Ntilde;    break;
    case 210:       pStr = OOO_STRING_SVTOOLS_HTML_C_Ograve;    break;
    case 211:       pStr = OOO_STRING_SVTOOLS_HTML_C_Oacute;    break;
    case 212:       pStr = OOO_STRING_SVTOOLS_HTML_C_Ocirc; break;
    case 213:       pStr = OOO_STRING_SVTOOLS_HTML_C_Otilde;    break;
    case 214:       pStr = OOO_STRING_SVTOOLS_HTML_C_Ouml;  break;
    case 215:       pStr = OOO_STRING_SVTOOLS_HTML_S_times; break;
    case 216:       pStr = OOO_STRING_SVTOOLS_HTML_C_Oslash;    break;
    case 217:       pStr = OOO_STRING_SVTOOLS_HTML_C_Ugrave;    break;
    case 218:       pStr = OOO_STRING_SVTOOLS_HTML_C_Uacute;    break;
    case 219:       pStr = OOO_STRING_SVTOOLS_HTML_C_Ucirc; break;
    case 220:       pStr = OOO_STRING_SVTOOLS_HTML_C_Uuml;  break;
    case 221:       pStr = OOO_STRING_SVTOOLS_HTML_C_Yacute;    break;

    case 222:       pStr = OOO_STRING_SVTOOLS_HTML_C_THORN; break;
    case 223:       pStr = OOO_STRING_SVTOOLS_HTML_C_szlig; break;

    case 224:       pStr = OOO_STRING_SVTOOLS_HTML_S_agrave;    break;
    case 225:       pStr = OOO_STRING_SVTOOLS_HTML_S_aacute;    break;
    case 226:       pStr = OOO_STRING_SVTOOLS_HTML_S_acirc; break;
    case 227:       pStr = OOO_STRING_SVTOOLS_HTML_S_atilde;    break;
    case 228:       pStr = OOO_STRING_SVTOOLS_HTML_S_auml;  break;
    case 229:       pStr = OOO_STRING_SVTOOLS_HTML_S_aring; break;
    case 230:       pStr = OOO_STRING_SVTOOLS_HTML_S_aelig; break;
    case 231:       pStr = OOO_STRING_SVTOOLS_HTML_S_ccedil;    break;
    case 232:       pStr = OOO_STRING_SVTOOLS_HTML_S_egrave;    break;
    case 233:       pStr = OOO_STRING_SVTOOLS_HTML_S_eacute;    break;
    case 234:       pStr = OOO_STRING_SVTOOLS_HTML_S_ecirc; break;
    case 235:       pStr = OOO_STRING_SVTOOLS_HTML_S_euml;  break;
    case 236:       pStr = OOO_STRING_SVTOOLS_HTML_S_igrave;    break;
    case 237:       pStr = OOO_STRING_SVTOOLS_HTML_S_iacute;    break;
    case 238:       pStr = OOO_STRING_SVTOOLS_HTML_S_icirc; break;
    case 239:       pStr = OOO_STRING_SVTOOLS_HTML_S_iuml;  break;
    case 240:       pStr = OOO_STRING_SVTOOLS_HTML_S_eth;       break;
    case 241:       pStr = OOO_STRING_SVTOOLS_HTML_S_ntilde;    break;
    case 242:       pStr = OOO_STRING_SVTOOLS_HTML_S_ograve;    break;
    case 243:       pStr = OOO_STRING_SVTOOLS_HTML_S_oacute;    break;
    case 244:       pStr = OOO_STRING_SVTOOLS_HTML_S_ocirc; break;
    case 245:       pStr = OOO_STRING_SVTOOLS_HTML_S_otilde;    break;
    case 246:       pStr = OOO_STRING_SVTOOLS_HTML_S_ouml;  break;
    case 247:       pStr = OOO_STRING_SVTOOLS_HTML_S_divide;    break;
    case 248:       pStr = OOO_STRING_SVTOOLS_HTML_S_oslash;    break;
    case 249:       pStr = OOO_STRING_SVTOOLS_HTML_S_ugrave;    break;
    case 250:       pStr = OOO_STRING_SVTOOLS_HTML_S_uacute;    break;
    case 251:       pStr = OOO_STRING_SVTOOLS_HTML_S_ucirc; break;
    case 252:       pStr = OOO_STRING_SVTOOLS_HTML_S_uuml;  break;
    case 253:       pStr = OOO_STRING_SVTOOLS_HTML_S_yacute;    break;
    case 254:       pStr = OOO_STRING_SVTOOLS_HTML_S_thorn; break;
    case 255:       pStr = OOO_STRING_SVTOOLS_HTML_S_yuml;  break;

    case 338:       pStr = OOO_STRING_SVTOOLS_HTML_S_OElig; break;
    case 339:       pStr = OOO_STRING_SVTOOLS_HTML_S_oelig; break;
    case 352:       pStr = OOO_STRING_SVTOOLS_HTML_S_Scaron;    break;
    case 353:       pStr = OOO_STRING_SVTOOLS_HTML_S_scaron;    break;
    case 376:       pStr = OOO_STRING_SVTOOLS_HTML_S_Yuml;  break;
    case 402:       pStr = OOO_STRING_SVTOOLS_HTML_S_fnof;  break;
    case 710:       pStr = OOO_STRING_SVTOOLS_HTML_S_circ;  break;
    case 732:       pStr = OOO_STRING_SVTOOLS_HTML_S_tilde; break;

    // Greek chars are handled later,
    // since they should *not* be transformed to entities
    // when generating Greek text (== using Greek encoding)

    case 8194:      pStr = OOO_STRING_SVTOOLS_HTML_S_ensp;  break;
    case 8195:      pStr = OOO_STRING_SVTOOLS_HTML_S_emsp;  break;
    case 8201:      pStr = OOO_STRING_SVTOOLS_HTML_S_thinsp;    break;
    case 8204:      pStr = OOO_STRING_SVTOOLS_HTML_S_zwnj;  break;
    case 8205:      pStr = OOO_STRING_SVTOOLS_HTML_S_zwj;       break;
    case 8206:      pStr = OOO_STRING_SVTOOLS_HTML_S_lrm;       break;
    case 8207:      pStr = OOO_STRING_SVTOOLS_HTML_S_rlm;       break;
    case 8211:      pStr = OOO_STRING_SVTOOLS_HTML_S_ndash; break;
    case 8212:      pStr = OOO_STRING_SVTOOLS_HTML_S_mdash; break;
    case 8216:      pStr = OOO_STRING_SVTOOLS_HTML_S_lsquo; break;
    case 8217:      pStr = OOO_STRING_SVTOOLS_HTML_S_rsquo; break;
    case 8218:      pStr = OOO_STRING_SVTOOLS_HTML_S_sbquo; break;
    case 8220:      pStr = OOO_STRING_SVTOOLS_HTML_S_ldquo; break;
    case 8221:      pStr = OOO_STRING_SVTOOLS_HTML_S_rdquo; break;
    case 8222:      pStr = OOO_STRING_SVTOOLS_HTML_S_bdquo; break;
    case 8224:      pStr = OOO_STRING_SVTOOLS_HTML_S_dagger;    break;
    case 8225:      pStr = OOO_STRING_SVTOOLS_HTML_S_Dagger;    break;
    case 8226:      pStr = OOO_STRING_SVTOOLS_HTML_S_bull;  break;
    case 8230:      pStr = OOO_STRING_SVTOOLS_HTML_S_hellip;    break;
    case 8240:      pStr = OOO_STRING_SVTOOLS_HTML_S_permil;    break;
    case 8242:      pStr = OOO_STRING_SVTOOLS_HTML_S_prime; break;
    case 8243:      pStr = OOO_STRING_SVTOOLS_HTML_S_Prime; break;
    case 8249:      pStr = OOO_STRING_SVTOOLS_HTML_S_lsaquo;    break;
    case 8250:      pStr = OOO_STRING_SVTOOLS_HTML_S_rsaquo;    break;
    case 8254:      pStr = OOO_STRING_SVTOOLS_HTML_S_oline; break;
    case 8260:      pStr = OOO_STRING_SVTOOLS_HTML_S_frasl; break;
    case 8364:      pStr = OOO_STRING_SVTOOLS_HTML_S_euro;  break;
    case 8465:      pStr = OOO_STRING_SVTOOLS_HTML_S_image; break;
    case 8472:      pStr = OOO_STRING_SVTOOLS_HTML_S_weierp;    break;
    case 8476:      pStr = OOO_STRING_SVTOOLS_HTML_S_real;  break;
    case 8482:      pStr = OOO_STRING_SVTOOLS_HTML_S_trade; break;
    case 8501:      pStr = OOO_STRING_SVTOOLS_HTML_S_alefsym;   break;
    case 8592:      pStr = OOO_STRING_SVTOOLS_HTML_S_larr;  break;
    case 8593:      pStr = OOO_STRING_SVTOOLS_HTML_S_uarr;  break;
    case 8594:      pStr = OOO_STRING_SVTOOLS_HTML_S_rarr;  break;
    case 8595:      pStr = OOO_STRING_SVTOOLS_HTML_S_darr;  break;
    case 8596:      pStr = OOO_STRING_SVTOOLS_HTML_S_harr;  break;
    case 8629:      pStr = OOO_STRING_SVTOOLS_HTML_S_crarr; break;
    case 8656:      pStr = OOO_STRING_SVTOOLS_HTML_S_lArr;  break;
    case 8657:      pStr = OOO_STRING_SVTOOLS_HTML_S_uArr;  break;
    case 8658:      pStr = OOO_STRING_SVTOOLS_HTML_S_rArr;  break;
    case 8659:      pStr = OOO_STRING_SVTOOLS_HTML_S_dArr;  break;
    case 8660:      pStr = OOO_STRING_SVTOOLS_HTML_S_hArr;  break;
    case 8704:      pStr = OOO_STRING_SVTOOLS_HTML_S_forall;    break;
    case 8706:      pStr = OOO_STRING_SVTOOLS_HTML_S_part;  break;
    case 8707:      pStr = OOO_STRING_SVTOOLS_HTML_S_exist; break;
    case 8709:      pStr = OOO_STRING_SVTOOLS_HTML_S_empty; break;
    case 8711:      pStr = OOO_STRING_SVTOOLS_HTML_S_nabla; break;
    case 8712:      pStr = OOO_STRING_SVTOOLS_HTML_S_isin;  break;
    case 8713:      pStr = OOO_STRING_SVTOOLS_HTML_S_notin; break;
    case 8715:      pStr = OOO_STRING_SVTOOLS_HTML_S_ni;        break;
    case 8719:      pStr = OOO_STRING_SVTOOLS_HTML_S_prod;  break;
    case 8721:      pStr = OOO_STRING_SVTOOLS_HTML_S_sum;       break;
    case 8722:      pStr = OOO_STRING_SVTOOLS_HTML_S_minus; break;
    case 8727:      pStr = OOO_STRING_SVTOOLS_HTML_S_lowast;    break;
    case 8730:      pStr = OOO_STRING_SVTOOLS_HTML_S_radic; break;
    case 8733:      pStr = OOO_STRING_SVTOOLS_HTML_S_prop;  break;
    case 8734:      pStr = OOO_STRING_SVTOOLS_HTML_S_infin; break;
    case 8736:      pStr = OOO_STRING_SVTOOLS_HTML_S_ang;       break;
    case 8743:      pStr = OOO_STRING_SVTOOLS_HTML_S_and;       break;
    case 8744:      pStr = OOO_STRING_SVTOOLS_HTML_S_or;        break;
    case 8745:      pStr = OOO_STRING_SVTOOLS_HTML_S_cap;       break;
    case 8746:      pStr = OOO_STRING_SVTOOLS_HTML_S_cup;       break;
    case 8747:      pStr = OOO_STRING_SVTOOLS_HTML_S_int;       break;
    case 8756:      pStr = OOO_STRING_SVTOOLS_HTML_S_there4;    break;
    case 8764:      pStr = OOO_STRING_SVTOOLS_HTML_S_sim;       break;
    case 8773:      pStr = OOO_STRING_SVTOOLS_HTML_S_cong;  break;
    case 8776:      pStr = OOO_STRING_SVTOOLS_HTML_S_asymp; break;
    case 8800:      pStr = OOO_STRING_SVTOOLS_HTML_S_ne;        break;
    case 8801:      pStr = OOO_STRING_SVTOOLS_HTML_S_equiv; break;
    case 8804:      pStr = OOO_STRING_SVTOOLS_HTML_S_le;        break;
    case 8805:      pStr = OOO_STRING_SVTOOLS_HTML_S_ge;        break;
    case 8834:      pStr = OOO_STRING_SVTOOLS_HTML_S_sub;       break;
    case 8835:      pStr = OOO_STRING_SVTOOLS_HTML_S_sup;       break;
    case 8836:      pStr = OOO_STRING_SVTOOLS_HTML_S_nsub;  break;
    case 8838:      pStr = OOO_STRING_SVTOOLS_HTML_S_sube;  break;
    case 8839:      pStr = OOO_STRING_SVTOOLS_HTML_S_supe;  break;
    case 8853:      pStr = OOO_STRING_SVTOOLS_HTML_S_oplus; break;
    case 8855:      pStr = OOO_STRING_SVTOOLS_HTML_S_otimes;    break;
    case 8869:      pStr = OOO_STRING_SVTOOLS_HTML_S_perp;  break;
    case 8901:      pStr = OOO_STRING_SVTOOLS_HTML_S_sdot;  break;
    case 8968:      pStr = OOO_STRING_SVTOOLS_HTML_S_lceil; break;
    case 8969:      pStr = OOO_STRING_SVTOOLS_HTML_S_rceil; break;
    case 8970:      pStr = OOO_STRING_SVTOOLS_HTML_S_lfloor;    break;
    case 8971:      pStr = OOO_STRING_SVTOOLS_HTML_S_rfloor;    break;
    case 9001:      pStr = OOO_STRING_SVTOOLS_HTML_S_lang;  break;
    case 9002:      pStr = OOO_STRING_SVTOOLS_HTML_S_rang;  break;
    case 9674:      pStr = OOO_STRING_SVTOOLS_HTML_S_loz;       break;
    case 9824:      pStr = OOO_STRING_SVTOOLS_HTML_S_spades;    break;
    case 9827:      pStr = OOO_STRING_SVTOOLS_HTML_S_clubs; break;
    case 9829:      pStr = OOO_STRING_SVTOOLS_HTML_S_hearts;    break;
    case 9830:      pStr = OOO_STRING_SVTOOLS_HTML_S_diams; break;
    }

    // Greek chars: if we do not produce a Greek encoding,
    // transform them into entities
    if( !pStr &&
        ( eDestEnc != RTL_TEXTENCODING_ISO_8859_7 ) &&
        ( eDestEnc != RTL_TEXTENCODING_MS_1253 ) )
    {
        switch( c )
        {
        case 913:       pStr = OOO_STRING_SVTOOLS_HTML_S_Alpha; break;
        case 914:       pStr = OOO_STRING_SVTOOLS_HTML_S_Beta;  break;
        case 915:       pStr = OOO_STRING_SVTOOLS_HTML_S_Gamma; break;
        case 916:       pStr = OOO_STRING_SVTOOLS_HTML_S_Delta; break;
        case 917:       pStr = OOO_STRING_SVTOOLS_HTML_S_Epsilon;   break;
        case 918:       pStr = OOO_STRING_SVTOOLS_HTML_S_Zeta;  break;
        case 919:       pStr = OOO_STRING_SVTOOLS_HTML_S_Eta;       break;
        case 920:       pStr = OOO_STRING_SVTOOLS_HTML_S_Theta; break;
        case 921:       pStr = OOO_STRING_SVTOOLS_HTML_S_Iota;  break;
        case 922:       pStr = OOO_STRING_SVTOOLS_HTML_S_Kappa; break;
        case 923:       pStr = OOO_STRING_SVTOOLS_HTML_S_Lambda;    break;
        case 924:       pStr = OOO_STRING_SVTOOLS_HTML_S_Mu;        break;
        case 925:       pStr = OOO_STRING_SVTOOLS_HTML_S_Nu;        break;
        case 926:       pStr = OOO_STRING_SVTOOLS_HTML_S_Xi;        break;
        case 927:       pStr = OOO_STRING_SVTOOLS_HTML_S_Omicron;   break;
        case 928:       pStr = OOO_STRING_SVTOOLS_HTML_S_Pi;        break;
        case 929:       pStr = OOO_STRING_SVTOOLS_HTML_S_Rho;       break;
        case 931:       pStr = OOO_STRING_SVTOOLS_HTML_S_Sigma; break;
        case 932:       pStr = OOO_STRING_SVTOOLS_HTML_S_Tau;       break;
        case 933:       pStr = OOO_STRING_SVTOOLS_HTML_S_Upsilon;   break;
        case 934:       pStr = OOO_STRING_SVTOOLS_HTML_S_Phi;       break;
        case 935:       pStr = OOO_STRING_SVTOOLS_HTML_S_Chi;       break;
        case 936:       pStr = OOO_STRING_SVTOOLS_HTML_S_Psi;       break;
        case 937:       pStr = OOO_STRING_SVTOOLS_HTML_S_Omega; break;
        case 945:       pStr = OOO_STRING_SVTOOLS_HTML_S_alpha; break;
        case 946:       pStr = OOO_STRING_SVTOOLS_HTML_S_beta;  break;
        case 947:       pStr = OOO_STRING_SVTOOLS_HTML_S_gamma; break;
        case 948:       pStr = OOO_STRING_SVTOOLS_HTML_S_delta; break;
        case 949:       pStr = OOO_STRING_SVTOOLS_HTML_S_epsilon;   break;
        case 950:       pStr = OOO_STRING_SVTOOLS_HTML_S_zeta;  break;
        case 951:       pStr = OOO_STRING_SVTOOLS_HTML_S_eta;       break;
        case 952:       pStr = OOO_STRING_SVTOOLS_HTML_S_theta; break;
        case 953:       pStr = OOO_STRING_SVTOOLS_HTML_S_iota;  break;
        case 954:       pStr = OOO_STRING_SVTOOLS_HTML_S_kappa; break;
        case 955:       pStr = OOO_STRING_SVTOOLS_HTML_S_lambda;    break;
        case 956:       pStr = OOO_STRING_SVTOOLS_HTML_S_mu;        break;
        case 957:       pStr = OOO_STRING_SVTOOLS_HTML_S_nu;        break;
        case 958:       pStr = OOO_STRING_SVTOOLS_HTML_S_xi;        break;
        case 959:       pStr = OOO_STRING_SVTOOLS_HTML_S_omicron;   break;
        case 960:       pStr = OOO_STRING_SVTOOLS_HTML_S_pi;        break;
        case 961:       pStr = OOO_STRING_SVTOOLS_HTML_S_rho;       break;
        case 962:       pStr = OOO_STRING_SVTOOLS_HTML_S_sigmaf;    break;
        case 963:       pStr = OOO_STRING_SVTOOLS_HTML_S_sigma; break;
        case 964:       pStr = OOO_STRING_SVTOOLS_HTML_S_tau;       break;
        case 965:       pStr = OOO_STRING_SVTOOLS_HTML_S_upsilon;   break;
        case 966:       pStr = OOO_STRING_SVTOOLS_HTML_S_phi;       break;
        case 967:       pStr = OOO_STRING_SVTOOLS_HTML_S_chi;       break;
        case 968:       pStr = OOO_STRING_SVTOOLS_HTML_S_psi;       break;
        case 969:       pStr = OOO_STRING_SVTOOLS_HTML_S_omega; break;
        case 977:       pStr = OOO_STRING_SVTOOLS_HTML_S_thetasym;break;
        case 978:       pStr = OOO_STRING_SVTOOLS_HTML_S_upsih; break;
        case 982:       pStr = OOO_STRING_SVTOOLS_HTML_S_piv;       break;
        }
    }

    return pStr;
}

static sal_Size lcl_FlushContext(char* pBuffer, sal_uInt32 nFlags)
{
    sal_uInt32 nInfo = 0;
    sal_Size nSrcChars;
    sal_Size nLen = convertUnicodeToText(nullptr, 0,
                                             pBuffer, TXTCONV_BUFFER_SIZE, nFlags|RTL_UNICODETOTEXT_FLAGS_FLUSH,
                                             &nInfo, &nSrcChars);
    DBG_ASSERT((nInfo & (RTL_UNICODETOTEXT_INFO_ERROR|RTL_UNICODETOTEXT_INFO_DESTBUFFERTOSMALL)) == 0, "HTMLOut: error while flushing");
    return nLen;
}

static OString lcl_ConvertCharToHTML( sal_uInt32 c,
                            OUString *pNonConvertableChars )
{
    assert(rtl::isUnicodeCodePoint(c));

    OStringBuffer aDest;
    const char *pStr = nullptr;
    switch( c )
    {
    case 0xA0:      // is a hard blank
        pStr = OOO_STRING_SVTOOLS_HTML_S_nbsp;
        break;
    case 0x2011:    // is a hard hyphen
        pStr = "#8209";
        break;
    case 0xAD:      // is a soft hyphen
        pStr = OOO_STRING_SVTOOLS_HTML_S_shy;
        break;
    default:
        // There may be an entity for the character.
        // The new HTML4 entities above 255 are not used for UTF-8,
        // because Netscape 4 does support UTF-8 but does not support
        // these entities.
        if( c < 128 )
            pStr = lcl_svhtml_GetEntityForChar( c, RTL_TEXTENCODING_UTF8 );
        break;
    }

    char cBuffer[TXTCONV_BUFFER_SIZE];
    const sal_uInt32 nFlags = RTL_UNICODETOTEXT_FLAGS_NONSPACING_IGNORE|
                              RTL_UNICODETOTEXT_FLAGS_CONTROL_IGNORE|
                              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR|
                              RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR;
    if( pStr )
    {
        sal_Size nLen = lcl_FlushContext(cBuffer, nFlags);
        char *pBuffer = cBuffer;
        while( nLen-- )
            aDest.append(*pBuffer++);
        aDest.append(OString::Concat("&") + pStr + ";");
    }
    else
    {
        sal_uInt32 nInfo = 0;
        sal_Size nSrcChars;

        sal_Unicode utf16[2];
        auto n = rtl::splitSurrogates(c, utf16);
        sal_Size nLen = convertUnicodeToText(utf16, n,
                                                 cBuffer, TXTCONV_BUFFER_SIZE,
                                                 nFlags, &nInfo, &nSrcChars);
        if( nLen > 0 && (nInfo & (RTL_UNICODETOTEXT_INFO_ERROR|RTL_UNICODETOTEXT_INFO_DESTBUFFERTOSMALL)) == 0 )
        {
            char *pBuffer = cBuffer;
            while( nLen-- )
                aDest.append(*pBuffer++);
        }
        else
        {
            // If the character could not be converted to the destination
            // character set, the UNICODE character is exported as character
            // entity.
            // coverity[callee_ptr_arith] - its ok
            nLen = lcl_FlushContext(cBuffer, nFlags);
            char *pBuffer = cBuffer;
            while( nLen-- )
                aDest.append(*pBuffer++);

            aDest.append("&#" + OString::number(static_cast<sal_Int32>(c))
                    // Unicode code points guaranteed to fit into sal_Int32
                 + ";");
            if( pNonConvertableChars )
            {
                OUString cs(&c, 1);
                if( -1 == pNonConvertableChars->indexOf( cs ) )
                    (*pNonConvertableChars) += cs;
            }
        }
    }
    return aDest.makeStringAndClear();
}

static OString lcl_FlushToAscii()
{
    OStringBuffer aDest;

    char cBuffer[TXTCONV_BUFFER_SIZE];
    const sal_uInt32 nFlags = RTL_UNICODETOTEXT_FLAGS_NONSPACING_IGNORE|
                              RTL_UNICODETOTEXT_FLAGS_CONTROL_IGNORE|
                              RTL_UNICODETOTEXT_FLAGS_UNDEFINED_ERROR|
                              RTL_UNICODETOTEXT_FLAGS_INVALID_ERROR;
    sal_Size nLen = lcl_FlushContext(cBuffer, nFlags);
    char *pBuffer = cBuffer;
    while( nLen-- )
        aDest.append(*pBuffer++);
    return aDest.makeStringAndClear();
}

OString HTMLOutFuncs::ConvertStringToHTML( std::u16string_view rSrc,
    OUString *pNonConvertableChars )
{
    OStringBuffer aDest;
    for( sal_Int32 i=0, nLen = rSrc.size(); i < nLen; )
        aDest.append(lcl_ConvertCharToHTML(
            o3tl::iterateCodePoints(rSrc, &i), pNonConvertableChars));
    aDest.append(lcl_FlushToAscii());
    return aDest.makeStringAndClear();
}

SvStream& HTMLOutFuncs::Out_AsciiTag( SvStream& rStream, std::string_view rStr,
                                      bool bOn )
{
    if(bOn)
        rStream.WriteOString("<");
    else
        rStream.WriteOString("</");

    rStream.WriteOString(rStr).WriteChar('>');

    return rStream;
}

SvStream& HTMLOutFuncs::Out_Char( SvStream& rStream, sal_uInt32 c,
                                  OUString *pNonConvertableChars )
{
    OString sOut = lcl_ConvertCharToHTML( c, pNonConvertableChars );
    rStream.WriteOString( sOut );
    return rStream;
}

SvStream& HTMLOutFuncs::Out_String( SvStream& rStream, std::u16string_view rOUStr,
                                    OUString *pNonConvertableChars )
{
    sal_Int32 nLen = rOUStr.size();
    for( sal_Int32 n = 0; n < nLen; )
        HTMLOutFuncs::Out_Char( rStream, o3tl::iterateCodePoints(rOUStr, &n),
                                pNonConvertableChars );
    HTMLOutFuncs::FlushToAscii( rStream );
    return rStream;
}

SvStream& HTMLOutFuncs::FlushToAscii( SvStream& rStream )
{
    OString sOut = lcl_FlushToAscii();

    if (!sOut.isEmpty())
        rStream.WriteOString( sOut );

    return rStream;
}

SvStream& HTMLOutFuncs::Out_Hex( SvStream& rStream, sal_uInt32 nHex, sal_uInt8 nLen )
{                                                  // out into a stream
    char aNToABuf[] = "0000000000000000";

    DBG_ASSERT( nLen < sizeof(aNToABuf), "too many places" );
    if( nLen>=sizeof(aNToABuf) )
        nLen = (sizeof(aNToABuf)-1);

    // set pointer to end of buffer
    char *pStr = aNToABuf + (sizeof(aNToABuf)-1);
    for( sal_uInt8 n = 0; n < nLen; ++n )
    {
        *(--pStr) = static_cast<char>(nHex & 0xf ) + 48;
        if( *pStr > '9' )
            *pStr += 39;
        nHex >>= 4;
    }
    return rStream.WriteOString( pStr );
}


SvStream& HTMLOutFuncs::Out_Color( SvStream& rStream, const Color& rColor, bool bXHTML )
{
    rStream.WriteOString( "\"" );
    if (bXHTML)
        rStream.WriteOString( "color: " );
    rStream.WriteOString( "#" );
    if( rColor == COL_AUTO )
    {
        rStream.WriteOString( "000000" );
    }
    else
    {
        Out_Hex( rStream, rColor.GetRed(), 2 );
        Out_Hex( rStream, rColor.GetGreen(), 2 );
        Out_Hex( rStream, rColor.GetBlue(), 2 );
    }
    rStream.WriteChar( '\"' );

    return rStream;
}

SvStream& HTMLOutFuncs::Out_ImageMap( SvStream& rStream,
                                      const OUString& rBaseURL,
                                      const ImageMap& rIMap,
                                      const OUString& rName,
                                      const HTMLOutEvent *pEventTable,
                                      bool bOutStarBasic,
                                      const char *pDelim,
                                      const char *pIndentArea,
                                      const char *pIndentMap   )
{
    const OUString& rOutName = !rName.isEmpty() ? rName : rIMap.GetName();
    DBG_ASSERT( !rOutName.isEmpty(), "No ImageMap-Name" );
    if( rOutName.isEmpty() )
        return rStream;

    OStringBuffer sOut =
        OString::Concat("<") +
        OOO_STRING_SVTOOLS_HTML_map
        " "
        OOO_STRING_SVTOOLS_HTML_O_name
        "=\"";
    rStream.WriteOString( sOut );
    sOut.setLength(0);
    Out_String( rStream, rOutName );
    rStream.WriteOString( "\">" );

    for( size_t i=0; i<rIMap.GetIMapObjectCount(); i++ )
    {
        const IMapObject* pObj = rIMap.GetIMapObject( i );
        DBG_ASSERT( pObj, "Where is the ImageMap-Object?" );

        if( pObj )
        {
            const char *pShape = nullptr;
            OString aCoords;
            switch( pObj->GetType() )
            {
            case IMapObjectType::Rectangle:
                {
                    const IMapRectangleObject* pRectObj =
                        static_cast<const IMapRectangleObject *>(pObj);
                    pShape = OOO_STRING_SVTOOLS_HTML_SH_rect;
                    tools::Rectangle aRect( pRectObj->GetRectangle() );

                    aCoords =
                        OString::number(static_cast<sal_Int32>(aRect.Left()))
                        + ","
                        + OString::number(static_cast<sal_Int32>(aRect.Top()))
                        + ","
                        + OString::number(static_cast<sal_Int32>(aRect.Right()))
                        + ","
                        + OString::number(static_cast<sal_Int32>(aRect.Bottom()));;
                }
                break;
            case IMapObjectType::Circle:
                {
                    const IMapCircleObject* pCirc =
                        static_cast<const IMapCircleObject *>(pObj);
                    pShape= OOO_STRING_SVTOOLS_HTML_SH_circ;
                    Point aCenter( pCirc->GetCenter() );
                    tools::Long nOff = pCirc->GetRadius();

                    aCoords =
                        OString::number(static_cast<sal_Int32>(aCenter.X()))
                        + ","
                        + OString::number(static_cast<sal_Int32>(aCenter.Y()))
                        + ","
                        + OString::number(static_cast<sal_Int32>(nOff));
                }
                break;
            case IMapObjectType::Polygon:
                {
                    const IMapPolygonObject* pPolyObj =
                        static_cast<const IMapPolygonObject *>(pObj);
                    pShape= OOO_STRING_SVTOOLS_HTML_SH_poly;
                    tools::Polygon aPoly( pPolyObj->GetPolygon() );
                    sal_uInt16 nCount = aPoly.GetSize();
                    OString aTmpBuf;
                    if( nCount>0 )
                    {
                        const Point& rPoint = aPoly[0];
                        aTmpBuf = OString::number(static_cast<sal_Int32>(rPoint.X()))
                            + ","
                            + OString::number(static_cast<sal_Int32>(rPoint.Y()));
                    }
                    for( sal_uInt16 j=1; j<nCount; j++ )
                    {
                        const Point& rPoint = aPoly[j];
                        aTmpBuf =
                            ","
                            + OString::number(static_cast<sal_Int32>(rPoint.X()))
                            + ","
                            + OString::number(static_cast<sal_Int32>(rPoint.Y()));
                    }
                    aCoords = aTmpBuf;
                }
                break;
            default:
                DBG_ASSERT( pShape, "unknown IMapObject" );
                break;
            }

            if( pShape )
            {
                if( pDelim )
                    rStream.WriteOString( pDelim );
                if( pIndentArea )
                    rStream.WriteOString( pIndentArea );

                sOut.append(OString::Concat("<") + OOO_STRING_SVTOOLS_HTML_area
                        " " OOO_STRING_SVTOOLS_HTML_O_shape
                        "=\"" + pShape + "\" "
                        OOO_STRING_SVTOOLS_HTML_O_coords "=\"" +
                        aCoords + "\" ");
                rStream.WriteOString( sOut );
                sOut.setLength(0);

                OUString aURL( pObj->GetURL() );
                if( !aURL.isEmpty() && pObj->IsActive() )
                {
                    aURL = URIHelper::simpleNormalizedMakeRelative(
                        rBaseURL, aURL );
                    sOut.append(OOO_STRING_SVTOOLS_HTML_O_href "=\"");
                    rStream.WriteOString( sOut );
                    sOut.setLength(0);
                    Out_String( rStream, aURL ).WriteChar( '\"' );
                }
                else
                    rStream.WriteOString( OOO_STRING_SVTOOLS_HTML_O_nohref );

                const OUString& rObjName = pObj->GetName();
                if( !rObjName.isEmpty() )
                {
                    sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_name "=\"");
                    rStream.WriteOString( sOut );
                    sOut.setLength(0);
                    Out_String( rStream, rObjName ).WriteChar( '\"' );
                }

                const OUString& rTarget = pObj->GetTarget();
                if( !rTarget.isEmpty() && pObj->IsActive() )
                {
                    sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_target "=\"");
                    rStream.WriteOString( sOut );
                    sOut.setLength(0);
                    Out_String( rStream, rTarget ).WriteChar( '\"' );
                }

                OUString rDesc( pObj->GetAltText() );
                if( rDesc.isEmpty() )
                    rDesc = pObj->GetDesc();

                if( !rDesc.isEmpty() )
                {
                    sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_alt "=\"");
                    rStream.WriteOString( sOut );
                    sOut.setLength(0);
                    Out_String( rStream, rDesc ).WriteChar( '\"' );
                }

                const SvxMacroTableDtor& rMacroTab = pObj->GetMacroTable();
                if( pEventTable && !rMacroTab.empty() )
                    Out_Events( rStream, rMacroTab, pEventTable,
                                bOutStarBasic );

                rStream.WriteOString("/>");
            }
        }

    }

    if( pDelim )
        rStream.WriteOString( pDelim );
    if( pIndentMap )
        rStream.WriteOString( pIndentMap );
    Out_AsciiTag( rStream, OOO_STRING_SVTOOLS_HTML_map, false );

    return rStream;
}

SvStream& HTMLOutFuncs::OutScript( SvStream& rStrm,
                                   const OUString& rBaseURL,
                                   std::u16string_view rSource,
                                   std::u16string_view rLanguage,
                                   ScriptType eScriptType,
                                   const OUString& rSrc,
                                   const OUString *pSBLibrary,
                                   const OUString *pSBModule )
{
    // script is not indented!
    OStringBuffer sOut("<" OOO_STRING_SVTOOLS_HTML_script);

    if( !rLanguage.empty() )
    {
        sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_language "=\"");
        rStrm.WriteOString( sOut );
        sOut.setLength(0);
        Out_String( rStrm, rLanguage );
        sOut.append('\"');
    }

    if( !rSrc.isEmpty() )
    {
        sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_src "=\"");
        rStrm.WriteOString( sOut );
        sOut.setLength(0);
        Out_String( rStrm, URIHelper::simpleNormalizedMakeRelative(rBaseURL, rSrc) );
        sOut.append('\"');
    }

    if( STARBASIC != eScriptType && pSBLibrary )
    {
        sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_sdlibrary "=\"");
        rStrm.WriteOString( sOut );
        sOut.setLength(0);
        Out_String( rStrm, *pSBLibrary );
        sOut.append('\"');
    }

    if( STARBASIC != eScriptType && pSBModule )
    {
        sOut.append(" " OOO_STRING_SVTOOLS_HTML_O_sdmodule "=\"");
        rStrm.WriteOString( sOut );
        sOut.setLength(0);
        Out_String( rStrm, *pSBModule );
        sOut.append('\"');
    }

    sOut.append('>');

    rStrm.WriteOString( sOut );
    sOut.setLength(0);

    if( !rSource.empty() || pSBLibrary || pSBModule )
    {
        rStrm.WriteOString( SAL_NEWLINE_STRING );

        if( JAVASCRIPT != eScriptType )
        {
            rStrm.WriteOString( "<!--" )
                 .WriteOString( SAL_NEWLINE_STRING );
        }

        if( STARBASIC == eScriptType )
        {
            if( pSBLibrary )
            {
                sOut.append("' " OOO_STRING_SVTOOLS_HTML_SB_library " " +
                            OUStringToOString(*pSBLibrary, RTL_TEXTENCODING_UTF8));
                rStrm.WriteOString( sOut ).WriteOString( SAL_NEWLINE_STRING );
                sOut.setLength(0);
            }

            if( pSBModule )
            {
                sOut.append("' " OOO_STRING_SVTOOLS_HTML_SB_module " " +
                        OUStringToOString(*pSBModule, RTL_TEXTENCODING_UTF8));
                rStrm.WriteOString( sOut ).WriteOString( SAL_NEWLINE_STRING );
                sOut.setLength(0);
            }
        }

        if( !rSource.empty() )
        {
            // we write the module in ANSI-charset, but with
            // the system new line.
            const OString sSource(OUStringToOString(rSource, RTL_TEXTENCODING_UTF8));
            rStrm.WriteOString( sSource ).WriteOString( SAL_NEWLINE_STRING );
        }
        rStrm.WriteOString( SAL_NEWLINE_STRING );

        if( JAVASCRIPT != eScriptType )
        {
            // MIB/MM: if it is not StarBasic, a // could be wrong.
            // As the comment is removed during reading, it is not helping us...
            rStrm.WriteOString( STARBASIC == eScriptType ? "' -->" : "// -->" )
                 .WriteOString( SAL_NEWLINE_STRING );
        }
    }

    HTMLOutFuncs::Out_AsciiTag( rStrm, OOO_STRING_SVTOOLS_HTML_script, false );

    return rStrm;
}


SvStream& HTMLOutFuncs::Out_Events( SvStream& rStrm,
                                    const SvxMacroTableDtor& rMacroTable,
                                    const HTMLOutEvent *pEventTable,
                                    bool bOutStarBasic )
{
    sal_uInt16 i=0;
    while( pEventTable[i].pBasicName || pEventTable[i].pJavaName )
    {
        const SvxMacro *pMacro =
            rMacroTable.Get( pEventTable[i].nEvent );

        if( pMacro && pMacro->HasMacro() &&
            ( JAVASCRIPT == pMacro->GetScriptType() || bOutStarBasic ))
        {
            const char *pStr = STARBASIC == pMacro->GetScriptType()
                ? pEventTable[i].pBasicName
                : pEventTable[i].pJavaName;

            if( pStr )
            {
                OString sOut = OString::Concat(" ") + pStr + "=\"";
                rStrm.WriteOString( sOut );

                Out_String( rStrm, pMacro->GetMacName(), /*pNonConvertableChars*/nullptr ).WriteChar( '\"' );
            }
        }
        i++;
    }

    return rStrm;
}

OString HTMLOutFuncs::CreateTableDataOptionsValNum(
            bool bValue,
            double fVal, sal_uInt32 nFormat, SvNumberFormatter& rFormatter,
            OUString* pNonConvertableChars)
{
    OStringBuffer aStrTD;

    if ( bValue )
    {
        // printf / scanf is not precise enough
        OUString aValStr = rFormatter.GetInputLineString( fVal, 0 );
        OString sTmp(OUStringToOString(aValStr, RTL_TEXTENCODING_UTF8));
        aStrTD.append(" " OOO_STRING_SVTOOLS_HTML_O_SDval "=\"" +
                sTmp + "\"");
    }
    if ( bValue || nFormat )
    {
        aStrTD.append(" " OOO_STRING_SVTOOLS_HTML_O_SDnum "=\"" +
            OString::number(static_cast<sal_uInt16>(
                Application::GetSettings().GetLanguageTag().getLanguageType())) +
                ";"); // Language for Format 0
        if ( nFormat )
        {
            OString aNumStr;
            LanguageType nLang;
            const SvNumberformat* pFormatEntry = rFormatter.GetEntry( nFormat );
            if ( pFormatEntry )
            {
                aNumStr = ConvertStringToHTML( pFormatEntry->GetFormatstring(),
                    pNonConvertableChars );
                nLang = pFormatEntry->GetLanguage();
            }
            else
                nLang = LANGUAGE_SYSTEM;
            aStrTD.append(
                OString::number(static_cast<sal_Int32>(static_cast<sal_uInt16>(nLang)))
                + ";"
                + aNumStr);
        }
        aStrTD.append('\"');
    }
    return aStrTD.makeStringAndClear();
}

bool HTMLOutFuncs::PrivateURLToInternalImg( OUString& rURL )
{
    if( rURL.startsWith(OOO_STRING_SVTOOLS_HTML_private_image) )
    {
        rURL = rURL.copy( strlen(OOO_STRING_SVTOOLS_HTML_private_image) );
        return true;
    }

    return false;
}

void HtmlWriterHelper::applyColor(HtmlWriter& rHtmlWriter, std::string_view aAttributeName, const Color& rColor)
{
    OStringBuffer sBuffer;

    if( rColor == COL_AUTO )
    {
        sBuffer.append("#000000");
    }
    else
    {
        sBuffer.append('#');
        std::ostringstream sStringStream;
        sStringStream
            << std::right
            << std::setfill('0')
            << std::setw(6)
            << std::hex
            << sal_uInt32(rColor.GetRGBColor());
        sBuffer.append(sStringStream.str().c_str());
    }

    rHtmlWriter.attribute(aAttributeName, sBuffer);
}


void HtmlWriterHelper::applyEvents(HtmlWriter& rHtmlWriter, const SvxMacroTableDtor& rMacroTable, const HTMLOutEvent* pEventTable, bool bOutStarBasic)
{
    sal_uInt16 i = 0;
    while (pEventTable[i].pBasicName || pEventTable[i].pJavaName)
    {
        const SvxMacro* pMacro = rMacroTable.Get(pEventTable[i].nEvent);

        if (pMacro && pMacro->HasMacro() && (JAVASCRIPT == pMacro->GetScriptType() || bOutStarBasic))
        {
            const char* pAttributeName = nullptr;
            if (STARBASIC == pMacro->GetScriptType())
                pAttributeName = pEventTable[i].pBasicName;
            else
                pAttributeName = pEventTable[i].pJavaName;

            if (pAttributeName)
            {
                rHtmlWriter.attribute(pAttributeName, OUStringToOString(pMacro->GetMacName(), RTL_TEXTENCODING_UTF8));
            }
        }
        i++;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
