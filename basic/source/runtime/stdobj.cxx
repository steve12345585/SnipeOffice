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


#include <runtime.hxx>
#include <stdobj.hxx>
#include <sbstdobj.hxx>
#include <rtlproto.hxx>
#include <sbintern.hxx>
// The nArgs-field of a table entry is encrypted as follows:
// At the moment it is assumed that properties don't need any
// parameters!

// previously ARGSMASK_ was 0x007F ( e.g. up to 127 args ) however 63 should be
// enough, if not we need to increase the size of nArgs member in the Methods
// struct below.
// note: the limitation of 63 args is only for RTL functions defined here and
// does NOT impose a limit on User defined procedures ). This changes is to
// allow us space for a flag to denylist some functions in vba mode

namespace {

enum Flags
{
    ARGSMASK_ = 0x003F,  // 63 Arguments

    NORMONLY_   = 0x0040,  // procedure is visible in normal mode only
    COMPATONLY_ = 0x0080,  // procedure is visible in vba mode only
    COMPTMASK_  = (COMPATONLY_ | NORMONLY_),  // COMPATIBILITY mask

    READ_       = 0x0100,  // parameter allows read
    WRITE_      = 0x0200,  // parameter allows write
    OPT_        = 0x0400,  // parameter is optional
    CONST_      = 0x0800,  // property is const
    RWMASK_     = (READ_ | WRITE_ | OPT_ | CONST_), // mask for R/W-bits

    FUNC_TYPE_  = 0x1000,  // functional type
    SUB_TYPE_   = 0x2000,  // sub type
    METHOD_     = (FUNC_TYPE_ | SUB_TYPE_),
    PROPERTY_   = 0x4000,
    OBJECT_     = 0x8000,
    TYPEMASK_   = (METHOD_ | PROPERTY_ | OBJECT_), // mask for the entry's type

    // combination of bits above
    FUNCTION_   = (FUNC_TYPE_ | READ_),
    LFUNCTION_  = (FUNC_TYPE_ | READ_ | WRITE_), // mask for function which also works as Lvalue (statement)
    SUB_        = SUB_TYPE_,
    ROPROP_     = (PROPERTY_ | READ_), // mask Read Only-Property
    RWPROP_     = (PROPERTY_ | READ_ | WRITE_), // mask Read/Write-Property
    CPROP_      = (PROPERTY_ | READ_ | CONST_) // mask for constant
};

struct Method {
    RtlCall     pFunc;
    std::u16string_view sName;
    SbxDataType eType;
    short       nArgs;
    sal_uInt16      nHash;
    constexpr Method(std::u16string_view name, SbxDataType type, short args, RtlCall func)
        : pFunc(func)
        , sName(name)
        , eType(type)
        , nArgs(args)
        , nHash(SbxVariable::MakeHashCode(name))
    {
    }
};

constexpr Method arg(std::u16string_view name, SbxDataType type, short args = 0)
{
    return Method(name, type, args, nullptr);
}

template <int N> constexpr bool MethodsTableValid(const Method (&rMethods)[N])
{
    int nCurMethArgs = 0;
    int nArgsChecked = 0;
    for (const auto& m : rMethods)
    {
        if (m.pFunc) // main (function/sub/etc) entry
        {
            assert(nCurMethArgs == nArgsChecked); // previous method had correct # of arguments
            if (nCurMethArgs != nArgsChecked)
                return false;
            nCurMethArgs = m.nArgs & ARGSMASK_;
            nArgsChecked = 0;
        }
        else // subordinate (argument) entry
            ++nArgsChecked;
    }
    assert(nCurMethArgs == nArgsChecked); // last method had correct # of arguments
    return nCurMethArgs == nArgsChecked;
}

template <bool N> void ConstBool(StarBASIC*, SbxArray& par, bool) { par.Get(0)->PutBool(N); }
template <sal_Int16 N> void ConstInt(StarBASIC*, SbxArray& par, bool) { par.Get(0)->PutInteger(N); }

constexpr Method aMethods[] = {

{ u"Abs",                           SbxDOUBLE,   1 | FUNCTION_,        SbRtl_Abs                  },
    arg(u"number", SbxDOUBLE),

{ u"Array",                         SbxOBJECT,       FUNCTION_,        SbRtl_Array                },
{ u"Asc",                           SbxLONG,     1 | FUNCTION_,        SbRtl_Asc                  },
    arg(u"string", SbxSTRING),

{ u"AscW",                          SbxLONG,     1 | FUNCTION_ | COMPATONLY_, SbRtl_Asc           },
    arg(u"string", SbxSTRING),

{ u"Atn",                           SbxDOUBLE,   1 | FUNCTION_,        SbRtl_Atn                  },
    arg(u"number", SbxDOUBLE),

// Related to: Dir, GetAttr, SetAttr
{ u"ATTR_ARCHIVE",                  SbxINTEGER,      CPROP_,   ConstInt<SbAttributes::ARCHIVE>    },
{ u"ATTR_DIRECTORY",                SbxINTEGER,      CPROP_,   ConstInt<SbAttributes::DIRECTORY>  },
{ u"ATTR_HIDDEN",                   SbxINTEGER,      CPROP_,   ConstInt<SbAttributes::HIDDEN>     },
{ u"ATTR_NORMAL",                   SbxINTEGER,      CPROP_,   ConstInt<SbAttributes::NORMAL>     },
{ u"ATTR_READONLY",                 SbxINTEGER,      CPROP_,   ConstInt<SbAttributes::READONLY>   },
{ u"ATTR_SYSTEM",                   SbxINTEGER,      CPROP_,   ConstInt<SbAttributes::SYSTEM>     },
{ u"ATTR_VOLUME",                   SbxINTEGER,      CPROP_,   ConstInt<SbAttributes::VOLUME>     },

{ u"Beep",                          SbxNULL,         FUNCTION_,        SbRtl_Beep                 },
{ u"Blue",                          SbxINTEGER,  1 | FUNCTION_ | NORMONLY_, SbRtl_Blue            },
    arg(u"RGB-Value", SbxLONG),

{ u"CallByName",                    SbxVARIANT,  3 | FUNCTION_,        SbRtl_CallByName           },
    arg(u"Object",        SbxOBJECT),
    arg(u"ProcName", SbxSTRING),
    arg(u"CallType",      SbxINTEGER),

{ u"CBool",                         SbxBOOL,     1 | FUNCTION_,        SbRtl_CBool                },
    arg(u"expression", SbxVARIANT),

{ u"CByte",                         SbxBYTE,     1 | FUNCTION_,        SbRtl_CByte                },
    arg(u"expression", SbxVARIANT),

{ u"CCur",                          SbxCURRENCY, 1 | FUNCTION_,        SbRtl_CCur                 },
    arg(u"expression", SbxVARIANT),

{ u"CDate",                         SbxDATE,     1 | FUNCTION_,        SbRtl_CDate                },
    arg(u"expression", SbxVARIANT),

{ u"CDateFromUnoDate",              SbxDATE,     1 | FUNCTION_,        SbRtl_CDateFromUnoDate     },
    arg(u"UnoDate", SbxOBJECT),

{ u"CDateToUnoDate",                SbxOBJECT,   1 | FUNCTION_,        SbRtl_CDateToUnoDate       },
    arg(u"Date", SbxDATE),

{ u"CDateFromUnoTime",              SbxDATE,     1 | FUNCTION_,        SbRtl_CDateFromUnoTime     },
    arg(u"UnoTime", SbxOBJECT),

{ u"CDateToUnoTime",                SbxOBJECT,   1 | FUNCTION_,        SbRtl_CDateToUnoTime       },
    arg(u"Time", SbxDATE),

{ u"CDateFromUnoDateTime",          SbxDATE,     1 | FUNCTION_,        SbRtl_CDateFromUnoDateTime },
    arg(u"UnoDateTime", SbxOBJECT),

{ u"CDateToUnoDateTime",            SbxOBJECT,   1 | FUNCTION_,        SbRtl_CDateToUnoDateTime   },
    arg(u"DateTime", SbxDATE),

{ u"CDateFromIso",                  SbxDATE,     1 | FUNCTION_,        SbRtl_CDateFromIso         },
    arg(u"IsoDate", SbxSTRING),

{ u"CDateToIso",                    SbxSTRING,   1 | FUNCTION_,        SbRtl_CDateToIso           },
    arg(u"Date", SbxDATE),

{ u"CDec",                          SbxDECIMAL,  1 | FUNCTION_,        SbRtl_CDec                 },
    arg(u"expression", SbxVARIANT),

{ u"CDbl",                          SbxDOUBLE,   1 | FUNCTION_,        SbRtl_CDbl                 },
    arg(u"expression", SbxVARIANT),

// FIXME: CF_* are for what??? They duplicate WinAPI clipboard constants, but why?
{ u"CF_BITMAP",                     SbxINTEGER,      CPROP_,           ConstInt<1>                },
{ u"CF_METAFILEPICT",               SbxINTEGER,      CPROP_,           ConstInt<2>                },
{ u"CF_TEXT",                       SbxINTEGER,      CPROP_,           ConstInt<3>                },

{ u"ChDir",                         SbxNULL,     1 | FUNCTION_,        SbRtl_ChDir                },
    arg(u"string", SbxSTRING),

{ u"ChDrive",                       SbxNULL,     1 | FUNCTION_,        SbRtl_ChDrive              },
    arg(u"string", SbxSTRING),

{ u"Choose",                        SbxVARIANT,  2 | FUNCTION_,        SbRtl_Choose               },
    arg(u"Index",      SbxINTEGER),
    arg(u"Expression", SbxVARIANT),

{ u"Chr",                           SbxSTRING,   1 | FUNCTION_,        SbRtl_Chr                  },
    arg(u"charcode", SbxLONG),

{ u"ChrW",                          SbxSTRING,   1 | FUNCTION_ | COMPATONLY_, SbRtl_ChrW          },
    arg(u"charcode", SbxLONG),

{ u"CInt",                          SbxINTEGER,  1 | FUNCTION_,        SbRtl_CInt                 },
    arg(u"expression", SbxVARIANT),

// FIXME: what for are these???
{ u"SET_TAB",                       SbxINTEGER,      CPROP_,           ConstInt<0>                },
{ u"CLEAR_TAB",                     SbxINTEGER,      CPROP_,           ConstInt<1>                },
{ u"CLEAR_ALLTABS",                 SbxINTEGER,      CPROP_,           ConstInt<2>                },

{ u"CLng",                          SbxLONG,     1 | FUNCTION_,        SbRtl_CLng                 },
    arg(u"expression", SbxVARIANT),

{ u"CompatibilityMode",             SbxBOOL,     1 | FUNCTION_,        SbRtl_CompatibilityMode    },
    arg(u"bEnable", SbxBOOL),

{ u"ConvertFromUrl",                SbxSTRING,   1 | FUNCTION_,        SbRtl_ConvertFromUrl       },
    arg(u"Url", SbxSTRING),

{ u"ConvertToUrl",                  SbxSTRING,   1 | FUNCTION_,        SbRtl_ConvertToUrl         },
    arg(u"SystemPath", SbxSTRING),

{ u"Cos",                           SbxDOUBLE,   1 | FUNCTION_,        SbRtl_Cos                  },
    arg(u"number", SbxDOUBLE),

{ u"CreateObject",                  SbxOBJECT,   1 | FUNCTION_,        SbRtl_CreateObject         },
    arg(u"class", SbxSTRING),

{ u"CreateUnoListener",             SbxOBJECT,   2 | FUNCTION_,        SbRtl_CreateUnoListener    },
    arg(u"prefix",   SbxSTRING),
    arg(u"typename", SbxSTRING),

{ u"CreateUnoDialog",               SbxOBJECT,   2 | FUNCTION_,        SbRtl_CreateUnoDialog      },
    arg(u"dialoglibrary", SbxOBJECT),
    arg(u"dialogname",    SbxSTRING),

{ u"CreateUnoService",              SbxOBJECT,   1 | FUNCTION_,        SbRtl_CreateUnoService     },
    arg(u"servicename", SbxSTRING),

{ u"CreateUnoServiceWithArguments", SbxOBJECT, 2 | FUNCTION_, SbRtl_CreateUnoServiceWithArguments },
    arg(u"servicename", SbxSTRING),
    arg(u"arguments",   SbxARRAY),

{ u"CreateUnoStruct",               SbxOBJECT,   1 | FUNCTION_,        SbRtl_CreateUnoStruct      },
    arg(u"classname", SbxSTRING),

{ u"CreateUnoValue",                SbxOBJECT,   2 | FUNCTION_,        SbRtl_CreateUnoValue       },
    arg(u"type",  SbxSTRING),
    arg(u"value", SbxVARIANT),

{ u"CreatePropertySet",             SbxOBJECT,   1 | FUNCTION_,        SbRtl_CreatePropertySet    },
    arg(u"values", SbxARRAY),

{ u"CSng",                          SbxSINGLE,   1 | FUNCTION_,        SbRtl_CSng                 },
    arg(u"expression", SbxVARIANT),

{ u"CStr",                          SbxSTRING,   1 | FUNCTION_,        SbRtl_CStr                 },
    arg(u"expression", SbxVARIANT),

{ u"CurDir",                        SbxSTRING,   1 | FUNCTION_,        SbRtl_CurDir               },
    arg(u"string", SbxSTRING),

{ u"CVar",                          SbxVARIANT,  1 | FUNCTION_,        SbRtl_CVar                 },
    arg(u"expression", SbxVARIANT),

{ u"CVErr",                         SbxVARIANT,  1 | FUNCTION_,        SbRtl_CVErr                },
    arg(u"expression", SbxVARIANT),

{ u"DDB",                           SbxDOUBLE,   5 | FUNCTION_ | COMPATONLY_, SbRtl_DDB           },
    arg(u"Cost",    SbxDOUBLE),
    arg(u"Salvage", SbxDOUBLE),
    arg(u"Life",    SbxDOUBLE),
    arg(u"Period",  SbxDOUBLE),
    arg(u"Factor",  SbxVARIANT, OPT_),

{ u"Date",                          SbxDATE,         LFUNCTION_,       SbRtl_Date                 },
{ u"DateAdd",                       SbxDATE,     3 | FUNCTION_,        SbRtl_DateAdd              },
    arg(u"Interval", SbxSTRING),
    arg(u"Number",   SbxLONG),
    arg(u"Date",     SbxDATE),

{ u"DateDiff",                      SbxDOUBLE,   5 | FUNCTION_,        SbRtl_DateDiff             },
    arg(u"Interval",        SbxSTRING),
    arg(u"Date1",           SbxDATE),
    arg(u"Date2",           SbxDATE),
    arg(u"Firstdayofweek",  SbxINTEGER, OPT_),
    arg(u"Firstweekofyear", SbxINTEGER, OPT_),

{ u"DatePart",                      SbxLONG,     4 | FUNCTION_,        SbRtl_DatePart             },
    arg(u"Interval",        SbxSTRING),
    arg(u"Date",            SbxDATE),
    arg(u"Firstdayofweek",  SbxINTEGER, OPT_),
    arg(u"Firstweekofyear", SbxINTEGER, OPT_),

{ u"DateSerial",                    SbxDATE,     3 | FUNCTION_,        SbRtl_DateSerial           },
    arg(u"Year",  SbxINTEGER),
    arg(u"Month", SbxINTEGER),
    arg(u"Day",   SbxINTEGER),

{ u"DateValue",                     SbxDATE,     1 | FUNCTION_,        SbRtl_DateValue            },
    arg(u"String", SbxSTRING),

{ u"Day",                           SbxINTEGER,  1 | FUNCTION_,        SbRtl_Day                  },
    arg(u"Date", SbxDATE),

{ u"Ddeexecute",                    SbxNULL,     2 | FUNCTION_,        SbRtl_DDEExecute           },
    arg(u"Channel", SbxLONG),
    arg(u"Command", SbxSTRING),

{ u"Ddeinitiate",                   SbxINTEGER,  2 | FUNCTION_,        SbRtl_DDEInitiate          },
    arg(u"Application", SbxSTRING),
    arg(u"Topic",       SbxSTRING),

{ u"Ddepoke",                       SbxNULL,     3 | FUNCTION_,        SbRtl_DDEPoke              },
    arg(u"Channel", SbxLONG),
    arg(u"Item",    SbxSTRING),
    arg(u"Data",    SbxSTRING),

{ u"Dderequest",                    SbxSTRING,   2 | FUNCTION_,        SbRtl_DDERequest           },
    arg(u"Channel", SbxLONG),
    arg(u"Item",    SbxSTRING),

{ u"Ddeterminate",                  SbxNULL,     1 | FUNCTION_,        SbRtl_DDETerminate         },
    arg(u"Channel", SbxLONG),

{ u"Ddeterminateall",               SbxNULL,         FUNCTION_,        SbRtl_DDETerminateAll      },
{ u"DimArray",                      SbxOBJECT,       FUNCTION_,        SbRtl_DimArray             },
{ u"Dir",                           SbxSTRING,   2 | FUNCTION_,        SbRtl_Dir                  },
    arg(u"Pathname",   SbxSTRING,  OPT_),
    arg(u"Attributes", SbxINTEGER, OPT_),

{ u"DoEvents",                      SbxINTEGER,      FUNCTION_,        SbRtl_DoEvents             },
{ u"DumpAllObjects",                SbxEMPTY,    2 | SUB_,             SbRtl_DumpAllObjects       },
    arg(u"FileSpec", SbxSTRING),
    arg(u"DumpAll",  SbxINTEGER, OPT_),

{ u"Empty",                         SbxVARIANT,      CPROP_,           SbRtl_Empty                },
{ u"EqualUnoObjects",               SbxBOOL,     2 | FUNCTION_,        SbRtl_EqualUnoObjects      },
    arg(u"Variant", SbxVARIANT),
    arg(u"Variant", SbxVARIANT),

{ u"EnableReschedule",              SbxNULL,     1 | FUNCTION_,        SbRtl_EnableReschedule     },
    arg(u"bEnable", SbxBOOL),

{ u"Environ",                       SbxSTRING,   1 | FUNCTION_,        SbRtl_Environ              },
    arg(u"Environmentstring", SbxSTRING),

{ u"EOF",                           SbxBOOL,     1 | FUNCTION_,        SbRtl_EOF                  },
    arg(u"Channel", SbxINTEGER),

{ u"Erl",                           SbxLONG,         ROPROP_,          SbRtl_Erl                  },
{ u"Err",                           SbxVARIANT,      RWPROP_,          SbRtl_Err                  },
{ u"Error",                         SbxSTRING,   1 | FUNCTION_,        SbRtl_Error                },
    arg(u"code", SbxLONG),

{ u"Exp",                           SbxDOUBLE,   1 | FUNCTION_,        SbRtl_Exp                  },
    arg(u"number", SbxDOUBLE),

{ u"False",                         SbxBOOL,         CPROP_,           ConstBool<false>           },
{ u"True",                          SbxBOOL,         CPROP_,           ConstBool<true>            },

{ u"FileAttr",                      SbxINTEGER,  2 | FUNCTION_,        SbRtl_FileAttr             },
    arg(u"Channel",    SbxINTEGER),
    arg(u"Attributes", SbxINTEGER),

{ u"FileCopy",                      SbxNULL,     2 | FUNCTION_,        SbRtl_FileCopy             },
    arg(u"Source",      SbxSTRING),
    arg(u"Destination", SbxSTRING),

{ u"FileDateTime",                  SbxSTRING,   1 | FUNCTION_,        SbRtl_FileDateTime         },
    arg(u"filename", SbxSTRING),

{ u"FileExists",                    SbxBOOL,     1 | FUNCTION_,        SbRtl_FileExists           },
    arg(u"filename", SbxSTRING),

{ u"FileLen",                       SbxLONG,     1 | FUNCTION_,        SbRtl_FileLen              },
    arg(u"filename", SbxSTRING),

{ u"FindObject",                    SbxOBJECT,   1 | FUNCTION_,        SbRtl_FindObject           },
    arg(u"Name", SbxSTRING),

{ u"FindPropertyObject",            SbxOBJECT,   2 | FUNCTION_,        SbRtl_FindPropertyObject   },
    arg(u"Object", SbxOBJECT),
    arg(u"Name",   SbxSTRING),

{ u"Fix",                           SbxDOUBLE,   1 | FUNCTION_,        SbRtl_Fix                  },
    arg(u"number", SbxDOUBLE),

{ u"Format",                        SbxSTRING,   2 | FUNCTION_,        SbRtl_Format               },
    arg(u"expression", SbxVARIANT),
    arg(u"format",     SbxSTRING, OPT_),

{ u"FormatDateTime",                SbxSTRING,   2 | FUNCTION_ | COMPATONLY_, SbRtl_FormatDateTime},
    arg(u"Date",        SbxDATE),
    arg(u"NamedFormat", SbxINTEGER, OPT_),

{ u"FormatNumber",                  SbxSTRING,   5 | FUNCTION_ | COMPATONLY_, SbRtl_FormatNumber  },
    arg(u"expression",                  SbxDOUBLE),
    arg(u"numDigitsAfterDecimal",       SbxINTEGER, OPT_),
    arg(u"includeLeadingDigit",         SbxINTEGER, OPT_), // vbTriState
    arg(u"useParensForNegativeNumbers", SbxINTEGER, OPT_), // vbTriState
    arg(u"groupDigits",                 SbxINTEGER, OPT_), // vbTriState

{ u"FormatPercent",                  SbxSTRING,   5 | FUNCTION_ | COMPATONLY_, SbRtl_FormatPercent  },
    arg(u"expression",                  SbxDOUBLE),
    arg(u"numDigitsAfterDecimal",       SbxINTEGER, OPT_),
    arg(u"includeLeadingDigit",         SbxINTEGER, OPT_), // vbTriState
    arg(u"useParensForNegativeNumbers", SbxINTEGER, OPT_), // vbTriState
    arg(u"groupDigits",                 SbxINTEGER, OPT_), // vbTriState

{ u"Frac",                          SbxDOUBLE,   1 | FUNCTION_,        SbRtl_Frac                 },
    arg(u"number", SbxDOUBLE),

// FIXME: what for are these???
{ u"FRAMEANCHORPAGE",               SbxINTEGER,      CPROP_,           ConstInt<1>                },
{ u"FRAMEANCHORCHAR",               SbxINTEGER,      CPROP_,           ConstInt<15>               },
{ u"FRAMEANCHORPARA",               SbxINTEGER,      CPROP_,           ConstInt<14>               },

{ u"FreeFile",                      SbxINTEGER,      FUNCTION_,        SbRtl_FreeFile             },
{ u"FreeLibrary",                   SbxNULL,     1 | FUNCTION_,        SbRtl_FreeLibrary          },
    arg(u"Modulename", SbxSTRING),

{ u"FV",                            SbxDOUBLE,   5 | FUNCTION_ | COMPATONLY_, SbRtl_FV            },
    arg(u"Rate", SbxDOUBLE),
    arg(u"NPer", SbxDOUBLE),
    arg(u"Pmt",  SbxDOUBLE),
    arg(u"PV",   SbxVARIANT, OPT_),
    arg(u"Due",  SbxVARIANT, OPT_),

{ u"Get",                           SbxNULL,     3 | FUNCTION_,        SbRtl_Get                  },
    arg(u"filenumber",   SbxINTEGER),
    arg(u"recordnumber", SbxLONG),
    arg(u"variablename", SbxVARIANT),

{ u"GetAttr",                       SbxINTEGER,  1 | FUNCTION_,        SbRtl_GetAttr              },
    arg(u"filename", SbxSTRING),

{ u"GetDefaultContext",             SbxOBJECT,   0 | FUNCTION_,        SbRtl_GetDefaultContext    },
{ u"GetDialogZoomFactorX",          SbxDOUBLE,       FUNCTION_,        SbRtl_GetDialogZoomFactorX },
{ u"GetDialogZoomFactorY",          SbxDOUBLE,       FUNCTION_,        SbRtl_GetDialogZoomFactorY },
{ u"GetGUIType",                    SbxINTEGER,      FUNCTION_,        SbRtl_GetGUIType           },
{ u"GetGUIVersion",                 SbxLONG,         FUNCTION_,        SbRtl_GetGUIVersion        },
{ u"GetPathSeparator",              SbxSTRING,       FUNCTION_,        SbRtl_GetPathSeparator     },
{ u"GetProcessServiceManager",      SbxOBJECT,   0 | FUNCTION_,    SbRtl_GetProcessServiceManager },
{ u"GetSolarVersion",               SbxLONG,         FUNCTION_,        SbRtl_GetSolarVersion      },
{ u"GetSystemTicks",                SbxLONG,         FUNCTION_,        SbRtl_GetSystemTicks       },
{ u"GetSystemType",                 SbxINTEGER,      FUNCTION_,        SbRtl_GetSystemType        },
{ u"GlobalScope",                   SbxOBJECT,       FUNCTION_,        SbRtl_GlobalScope          },
{ u"Green",                         SbxINTEGER,  1 | FUNCTION_ | NORMONLY_, SbRtl_Green           },
    arg(u"RGB-Value", SbxLONG),

{ u"HasUnoInterfaces",              SbxBOOL,     1 | FUNCTION_,        SbRtl_HasUnoInterfaces     },
    arg(u"InterfaceName", SbxSTRING),

{ u"Hex",                           SbxSTRING,   1 | FUNCTION_,        SbRtl_Hex                  },
    arg(u"number", SbxLONG),

{ u"Hour",                          SbxINTEGER,  1 | FUNCTION_,        SbRtl_Hour                 },
    arg(u"Date", SbxDATE),

// Related to: MsgBox (return value)
{ u"IDABORT",                       SbxINTEGER,      CPROP_,     ConstInt<SbMB::Response::ABORT>  },
{ u"IDCANCEL",                      SbxINTEGER,      CPROP_,     ConstInt<SbMB::Response::CANCEL> },
{ u"IDIGNORE",                      SbxINTEGER,      CPROP_,     ConstInt<SbMB::Response::IGNORE> },
{ u"IDNO",                          SbxINTEGER,      CPROP_,     ConstInt<SbMB::Response::NO>     },
{ u"IDOK",                          SbxINTEGER,      CPROP_,     ConstInt<SbMB::Response::OK>     },
{ u"IDRETRY",                       SbxINTEGER,      CPROP_,     ConstInt<SbMB::Response::RETRY>  },
{ u"IDYES",                         SbxINTEGER,      CPROP_,     ConstInt<SbMB::Response::YES>    },

{ u"Iif",                           SbxVARIANT,   3 | FUNCTION_,       SbRtl_Iif                  },
    arg(u"Bool",     SbxBOOL),
    arg(u"Variant1", SbxVARIANT),
    arg(u"Variant2", SbxVARIANT),

{ u"Input",                         SbxSTRING,    2 | FUNCTION_ | COMPATONLY_, SbRtl_Input        },
    arg(u"Number",     SbxLONG),
    arg(u"FileNumber", SbxLONG),

{ u"InputBox",                      SbxSTRING,    5 | FUNCTION_,       SbRtl_InputBox             },
    arg(u"Prompt",    SbxSTRING),
    arg(u"Title",     SbxSTRING, OPT_),
    arg(u"Default",   SbxSTRING, OPT_),
    arg(u"XPosTwips", SbxLONG,   OPT_),
    arg(u"YPosTwips", SbxLONG,   OPT_),

{ u"InStr",                         SbxLONG,      4 | FUNCTION_,       SbRtl_InStr                },
    arg(u"Start",   SbxSTRING,  OPT_),
    arg(u"String1", SbxSTRING),
    arg(u"String2", SbxSTRING),
    arg(u"Compare", SbxINTEGER, OPT_),

{ u"InStrRev",                      SbxLONG,      4 | FUNCTION_ | COMPATONLY_, SbRtl_InStrRev     },
    arg(u"StringCheck", SbxSTRING),
    arg(u"StringMatch", SbxSTRING),
    arg(u"Start",       SbxSTRING,  OPT_),
    arg(u"Compare",     SbxINTEGER, OPT_),

{ u"Int",                           SbxDOUBLE,    1 | FUNCTION_,       SbRtl_Int                  },
    arg(u"number", SbxDOUBLE),

{ u"IPmt",                          SbxDOUBLE,    6 | FUNCTION_ | COMPATONLY_, SbRtl_IPmt         },
    arg(u"Rate", SbxDOUBLE),
    arg(u"Per",  SbxDOUBLE),
    arg(u"NPer", SbxDOUBLE),
    arg(u"PV",   SbxDOUBLE),
    arg(u"FV",   SbxVARIANT, OPT_),
    arg(u"Due",  SbxVARIANT, OPT_),

{ u"IRR",                           SbxDOUBLE,    2 | FUNCTION_ | COMPATONLY_, SbRtl_IRR          },
    arg(u"ValueArray", SbxARRAY),
    arg(u"Guess", SbxVARIANT, OPT_),

{ u"IsArray",                       SbxBOOL,      1 | FUNCTION_,       SbRtl_IsArray              },
    arg(u"Variant", SbxVARIANT),

{ u"IsDate",                        SbxBOOL,      1 | FUNCTION_,       SbRtl_IsDate               },
    arg(u"Variant", SbxVARIANT),

{ u"IsEmpty",                       SbxBOOL,      1 | FUNCTION_,       SbRtl_IsEmpty              },
    arg(u"Variant", SbxVARIANT),

{ u"IsError",                       SbxBOOL,      1 | FUNCTION_,       SbRtl_IsError              },
    arg(u"Variant", SbxVARIANT),

{ u"IsMissing",                     SbxBOOL,      1 | FUNCTION_,       SbRtl_IsMissing            },
    arg(u"Variant", SbxVARIANT),

{ u"IsNull",                        SbxBOOL,      1 | FUNCTION_,       SbRtl_IsNull               },
    arg(u"Variant", SbxVARIANT),

{ u"IsNumeric",                     SbxBOOL,      1 | FUNCTION_,       SbRtl_IsNumeric            },
    arg(u"Variant", SbxVARIANT),

{ u"IsObject",                      SbxBOOL,      1 | FUNCTION_,       SbRtl_IsObject             },
    arg(u"Variant", SbxVARIANT),

{ u"IsUnoStruct",                   SbxBOOL,      1 | FUNCTION_,       SbRtl_IsUnoStruct          },
    arg(u"Variant", SbxVARIANT),

{ u"Join",                          SbxSTRING,    2 | FUNCTION_,       SbRtl_Join                 },
    arg(u"SourceArray", SbxOBJECT),
    arg(u"Delimiter",   SbxSTRING),

{ u"Kill",                          SbxNULL,      1 | FUNCTION_,       SbRtl_Kill                 },
    arg(u"filespec", SbxSTRING),

{ u"LBound",                        SbxLONG,      1 | FUNCTION_,       SbRtl_LBound               },
    arg(u"Variant", SbxVARIANT),

{ u"LCase",                         SbxSTRING,    1 | FUNCTION_,       SbRtl_LCase                },
    arg(u"string", SbxSTRING),

{ u"Left",                          SbxSTRING,    2 | FUNCTION_,       SbRtl_Left                 },
    arg(u"String", SbxSTRING),
    arg(u"Length", SbxLONG),

{ u"Len",                           SbxLONG,      1 | FUNCTION_,       SbRtl_Len                  },
    arg(u"StringOrVariant", SbxVARIANT),

{ u"LenB",                          SbxLONG,      1 | FUNCTION_,       SbRtl_Len                  },
    arg(u"StringOrVariant", SbxVARIANT),

{ u"Load",                          SbxNULL,      1 | FUNCTION_,       SbRtl_Load                 },
    arg(u"object", SbxOBJECT),

{ u"LoadPicture",                   SbxOBJECT,    1 | FUNCTION_,       SbRtl_LoadPicture          },
    arg(u"string", SbxSTRING),

{ u"Loc",                           SbxLONG,      1 | FUNCTION_,       SbRtl_Loc                  },
    arg(u"Channel", SbxINTEGER),

{ u"Lof",                           SbxLONG,      1 | FUNCTION_,       SbRtl_Lof                  },
    arg(u"Channel", SbxINTEGER),

{ u"Log",                           SbxDOUBLE,    1 | FUNCTION_,       SbRtl_Log                  },
    arg(u"number", SbxDOUBLE),

{ u"LTrim",                         SbxSTRING,    1 | FUNCTION_,       SbRtl_LTrim                },
    arg(u"string", SbxSTRING),

// Related to: MsgBox (Buttons argument)
{ u"MB_ABORTRETRYIGNORE",           SbxINTEGER,       CPROP_,    ConstInt<SbMB::ABORTRETRYIGNORE> },
{ u"MB_APPLMODAL",                  SbxINTEGER,       CPROP_,    ConstInt<SbMB::APPLMODAL>        },
{ u"MB_DEFBUTTON1",                 SbxINTEGER,       CPROP_,    ConstInt<SbMB::DEFBUTTON1>       },
{ u"MB_DEFBUTTON2",                 SbxINTEGER,       CPROP_,    ConstInt<SbMB::DEFBUTTON2>       },
{ u"MB_DEFBUTTON3",                 SbxINTEGER,       CPROP_,    ConstInt<SbMB::DEFBUTTON3>       },
{ u"MB_ICONEXCLAMATION",            SbxINTEGER,       CPROP_,    ConstInt<SbMB::ICONEXCLAMATION>  },
{ u"MB_ICONINFORMATION",            SbxINTEGER,       CPROP_,    ConstInt<SbMB::ICONINFORMATION>  },
{ u"MB_ICONQUESTION",               SbxINTEGER,       CPROP_,    ConstInt<SbMB::ICONQUESTION>     },
{ u"MB_ICONSTOP",                   SbxINTEGER,       CPROP_,    ConstInt<SbMB::ICONSTOP>         },
{ u"MB_OK",                         SbxINTEGER,       CPROP_,    ConstInt<SbMB::OK>               },
{ u"MB_OKCANCEL",                   SbxINTEGER,       CPROP_,    ConstInt<SbMB::OKCANCEL>         },
{ u"MB_RETRYCANCEL",                SbxINTEGER,       CPROP_,    ConstInt<SbMB::RETRYCANCEL>      },
{ u"MB_SYSTEMMODAL",                SbxINTEGER,       CPROP_,    ConstInt<SbMB::SYSTEMMODAL>      },
{ u"MB_YESNO",                      SbxINTEGER,       CPROP_,    ConstInt<SbMB::YESNO>            },
{ u"MB_YESNOCANCEL",                SbxINTEGER,       CPROP_,    ConstInt<SbMB::YESNOCANCEL>      },

{ u"Me",                            SbxOBJECT,    0 | FUNCTION_ | COMPATONLY_, SbRtl_Me           },
{ u"Mid",                           SbxSTRING,    3 | LFUNCTION_,      SbRtl_Mid                  },
    arg(u"String", SbxSTRING),
    arg(u"Start",  SbxLONG),
    arg(u"Length", SbxLONG, OPT_),

{ u"Minute",                        SbxINTEGER,   1 | FUNCTION_,       SbRtl_Minute               },
    arg(u"Date", SbxDATE),

{ u"MIRR",                          SbxDOUBLE,    3 | FUNCTION_ | COMPATONLY_, SbRtl_MIRR         },
    arg(u"ValueArray",   SbxARRAY),
    arg(u"FinanceRate",  SbxDOUBLE),
    arg(u"ReinvestRate", SbxDOUBLE),

{ u"MkDir",                         SbxNULL,      1 | FUNCTION_,       SbRtl_MkDir                },
    arg(u"pathname", SbxSTRING),

{ u"Month",                         SbxINTEGER,   1 | FUNCTION_,       SbRtl_Month                },
    arg(u"Date", SbxDATE),

{ u"MonthName",                     SbxSTRING,    2 | FUNCTION_ | COMPATONLY_, SbRtl_MonthName    },
    arg(u"Month",      SbxINTEGER),
    arg(u"Abbreviate", SbxBOOL, OPT_),

{ u"MsgBox",                        SbxINTEGER,   5 | FUNCTION_,       SbRtl_MsgBox               },
    arg(u"Prompt",   SbxSTRING),
    arg(u"Buttons",  SbxINTEGER, OPT_),
    arg(u"Title",    SbxSTRING,  OPT_),
    arg(u"Helpfile", SbxSTRING,  OPT_),
    arg(u"Context",  SbxINTEGER, OPT_),

{ u"Nothing",                       SbxOBJECT,        CPROP_,          SbRtl_Nothing              },
{ u"Now",                           SbxDATE,          FUNCTION_,       SbRtl_Now                  },
{ u"NPer",                          SbxDOUBLE,    5 | FUNCTION_ | COMPATONLY_, SbRtl_NPer         },
    arg(u"Rate", SbxDOUBLE),
    arg(u"Pmt",  SbxDOUBLE),
    arg(u"PV",   SbxDOUBLE),
    arg(u"FV",   SbxVARIANT, OPT_),
    arg(u"Due",  SbxVARIANT, OPT_),

{ u"NPV",                           SbxDOUBLE,    2 | FUNCTION_ | COMPATONLY_, SbRtl_NPV          },
    arg(u"Rate", SbxDOUBLE),
    arg(u"ValueArray", SbxARRAY),

{ u"Null",                          SbxNULL,          CPROP_,          SbRtl_Null                 },

{ u"Oct",                           SbxSTRING,    1 | FUNCTION_,       SbRtl_Oct                  },
    arg(u"number", SbxLONG),

{ u"Partition",                     SbxSTRING,    4 | FUNCTION_,       SbRtl_Partition            },
    arg(u"number",   SbxLONG),
    arg(u"start",    SbxLONG),
    arg(u"stop",     SbxLONG),
    arg(u"interval", SbxLONG),

{ u"Pi",                            SbxDOUBLE,        CPROP_,          SbRtl_PI                   },

{ u"Pmt",                           SbxDOUBLE,    5 | FUNCTION_ | COMPATONLY_, SbRtl_Pmt          },
    arg(u"Rate", SbxDOUBLE),
    arg(u"NPer", SbxDOUBLE),
    arg(u"PV",   SbxDOUBLE),
    arg(u"FV",   SbxVARIANT, OPT_),
    arg(u"Due",  SbxVARIANT, OPT_),

{ u"PPmt",                          SbxDOUBLE,    6 | FUNCTION_ | COMPATONLY_, SbRtl_PPmt         },
    arg(u"Rate", SbxDOUBLE),
    arg(u"Per",  SbxDOUBLE),
    arg(u"NPer", SbxDOUBLE),
    arg(u"PV",   SbxDOUBLE),
    arg(u"FV",   SbxVARIANT, OPT_),
    arg(u"Due",  SbxVARIANT, OPT_),

{ u"Put",                           SbxNULL,      3 | FUNCTION_,       SbRtl_Put                  },
    arg(u"filenumber",   SbxINTEGER),
    arg(u"recordnumber", SbxLONG),
    arg(u"variablename", SbxVARIANT),

{ u"PV",                            SbxDOUBLE,    5 | FUNCTION_ | COMPATONLY_, SbRtl_PV           },
    arg(u"Rate", SbxDOUBLE),
    arg(u"NPer", SbxDOUBLE),
    arg(u"Pmt",  SbxDOUBLE),
    arg(u"FV",   SbxVARIANT, OPT_),
    arg(u"Due",  SbxVARIANT, OPT_),

{ u"QBColor",                       SbxLONG,      1 | FUNCTION_,       SbRtl_QBColor              },
    arg(u"number", SbxINTEGER),

{ u"Randomize",                     SbxNULL,      1 | FUNCTION_,       SbRtl_Randomize            },
    arg(u"Number", SbxDOUBLE, OPT_),

{ u"Rate",                          SbxDOUBLE,    6 | FUNCTION_ | COMPATONLY_, SbRtl_Rate         },
    arg(u"NPer",  SbxDOUBLE),
    arg(u"Pmt",   SbxDOUBLE),
    arg(u"PV",    SbxDOUBLE),
    arg(u"FV",    SbxVARIANT, OPT_),
    arg(u"Due",   SbxVARIANT, OPT_),
    arg(u"Guess", SbxVARIANT, OPT_),

{ u"Red",                           SbxINTEGER,   1 | FUNCTION_ | NORMONLY_, SbRtl_Red            },
    arg(u"RGB-Value", SbxLONG),

{ u"Reset",                         SbxNULL,      0 | FUNCTION_,       SbRtl_Reset                },
{ u"ResolvePath",                   SbxSTRING,    1 | FUNCTION_,       SbRtl_ResolvePath          },
    arg(u"Path", SbxSTRING),

{ u"RGB",                           SbxLONG,      3 | FUNCTION_,       SbRtl_RGB                  },
    arg(u"Red",   SbxINTEGER),
    arg(u"Green", SbxINTEGER),
    arg(u"Blue",  SbxINTEGER),

{ u"Replace",                       SbxSTRING,    6 | FUNCTION_,       SbRtl_Replace              },
    arg(u"Expression", SbxSTRING),
    arg(u"Find",       SbxSTRING),
    arg(u"Replace",    SbxSTRING),
    arg(u"Start",      SbxINTEGER, OPT_),
    arg(u"Count",      SbxINTEGER, OPT_),
    arg(u"Compare",    SbxINTEGER, OPT_),

{ u"Right",                         SbxSTRING,    2 | FUNCTION_,       SbRtl_Right                },
    arg(u"String", SbxSTRING),
    arg(u"Length", SbxLONG),

{ u"RmDir",                         SbxNULL,      1 | FUNCTION_,       SbRtl_RmDir                },
    arg(u"pathname", SbxSTRING),

{ u"Round",                         SbxDOUBLE,    2 | FUNCTION_ | COMPATONLY_, SbRtl_Round        },
    arg(u"Expression",       SbxDOUBLE),
    arg(u"Numdecimalplaces", SbxINTEGER, OPT_),

{ u"Rnd",                           SbxDOUBLE,    1 | FUNCTION_,       SbRtl_Rnd                  },
    arg(u"Number", SbxDOUBLE, OPT_),

{ u"RTL",                           SbxOBJECT,    0 | FUNCTION_ | COMPATONLY_, SbRtl_RTL          },
{ u"RTrim",                         SbxSTRING,    1 | FUNCTION_,       SbRtl_RTrim                },
    arg(u"string", SbxSTRING),

{ u"SavePicture",                   SbxNULL,      2 | FUNCTION_,       SbRtl_SavePicture          },
    arg(u"object", SbxOBJECT),
    arg(u"string", SbxSTRING),

{ u"Second",                        SbxINTEGER,   1 | FUNCTION_,       SbRtl_Second               },
    arg(u"Date", SbxDATE),

{ u"Seek",                          SbxLONG,      1 | FUNCTION_,       SbRtl_Seek                 },
    arg(u"Channel", SbxINTEGER),

{ u"SendKeys",                      SbxNULL,      2 | FUNCTION_,       SbRtl_SendKeys             },
    arg(u"String", SbxSTRING),
    arg(u"Wait",   SbxBOOL, OPT_),

{ u"SetAttr",                       SbxNULL,      2 | FUNCTION_,       SbRtl_SetAttr              },
    arg(u"PathName",   SbxSTRING),
    arg(u"Attributes", SbxINTEGER),

// FIXME: what for are these???
{ u"SET_OFF",                       SbxINTEGER,       CPROP_,          ConstInt<0>                },
{ u"SET_ON",                        SbxINTEGER,       CPROP_,          ConstInt<1>                },
{ u"TOGGLE",                        SbxINTEGER,       CPROP_,          ConstInt<2>                },

{ u"Sgn",                           SbxINTEGER,   1 | FUNCTION_,       SbRtl_Sgn                  },
    arg(u"number", SbxDOUBLE),

{ u"Shell",                         SbxLONG,      2 | FUNCTION_,       SbRtl_Shell                },
    arg(u"PathName",    SbxSTRING),
    arg(u"WindowStyle", SbxINTEGER, OPT_),

{ u"Sin",                           SbxDOUBLE,    1 | FUNCTION_,       SbRtl_Sin                  },
    arg(u"number", SbxDOUBLE),

{ u"SLN",                           SbxDOUBLE,    3 |  FUNCTION_ | COMPATONLY_, SbRtl_SLN         },
    arg(u"Cost",   SbxDOUBLE),
    arg(u"Double", SbxDOUBLE),
    arg(u"Life",   SbxDOUBLE),

{ u"SYD",                           SbxDOUBLE,    4 |  FUNCTION_ | COMPATONLY_, SbRtl_SYD         },
    arg(u"Cost",    SbxDOUBLE),
    arg(u"Salvage", SbxDOUBLE),
    arg(u"Life",    SbxDOUBLE),
    arg(u"Period",  SbxDOUBLE),

{ u"Space",                         SbxSTRING,    1 | FUNCTION_,       SbRtl_Space                },
    arg(u"Number", SbxLONG),

{ u"Spc",                           SbxSTRING,    1 | FUNCTION_,       SbRtl_Space                },
    arg(u"Number", SbxLONG),

{ u"Split",                         SbxOBJECT,    3 | FUNCTION_,       SbRtl_Split                },
    arg(u"expression", SbxSTRING),
    arg(u"delimiter",  SbxSTRING),
    arg(u"Limit",      SbxLONG),

{ u"Sqr",                           SbxDOUBLE,    1 | FUNCTION_,       SbRtl_Sqr                  },
    arg(u"number", SbxDOUBLE),

{ u"Str",                           SbxSTRING,    1 | FUNCTION_,       SbRtl_Str                  },
    arg(u"number", SbxDOUBLE),

{ u"StrComp",                       SbxINTEGER,   3 | FUNCTION_,       SbRtl_StrComp              },
    arg(u"String1", SbxSTRING),
    arg(u"String2", SbxSTRING),
    arg(u"Compare", SbxINTEGER, OPT_),

{ u"StrConv",                       SbxOBJECT,    3 | FUNCTION_,       SbRtl_StrConv              },
    arg(u"String",     SbxSTRING),
    arg(u"Conversion", SbxSTRING),
    arg(u"LCID",       SbxINTEGER, OPT_),

{ u"String",                        SbxSTRING,    2 | FUNCTION_,       SbRtl_String               },
    arg(u"Number",    SbxLONG),
    arg(u"Character", SbxVARIANT),

{ u"StrReverse",                    SbxSTRING,    1 | FUNCTION_ | COMPATONLY_, SbRtl_StrReverse   },
    arg(u"String1", SbxSTRING),

{ u"Switch",                        SbxVARIANT,   2 | FUNCTION_,       SbRtl_Switch               },
    arg(u"Expression", SbxVARIANT),
    arg(u"Value",      SbxVARIANT),

{ u"Tab",                           SbxSTRING,    1 | FUNCTION_,       SbRtl_Tab                  },
    arg(u"Count", SbxLONG),

{ u"Tan",                           SbxDOUBLE,    1 | FUNCTION_,       SbRtl_Tan                  },
    arg(u"number", SbxDOUBLE),

{ u"Time",                          SbxVARIANT,       LFUNCTION_,      SbRtl_Time                 },
{ u"Timer",                         SbxDATE,          FUNCTION_,       SbRtl_Timer                },
{ u"TimeSerial",                    SbxDATE,      3 | FUNCTION_,       SbRtl_TimeSerial           },
    arg(u"Hour",   SbxLONG),
    arg(u"Minute", SbxLONG),
    arg(u"Second", SbxLONG),

{ u"TimeValue",                     SbxDATE,      1 | FUNCTION_,       SbRtl_TimeValue            },
    arg(u"String", SbxSTRING),

{ u"Trim",                          SbxSTRING,    1 | FUNCTION_,       SbRtl_Trim                 },
    arg(u"String", SbxSTRING),

{ u"TwipsPerPixelX",                SbxLONG,          FUNCTION_,       SbRtl_TwipsPerPixelX       },
{ u"TwipsPerPixelY",                SbxLONG,          FUNCTION_,       SbRtl_TwipsPerPixelY       },

// Related to: SwFieldTypesEnum in sw/inc/fldbas.hxx, .uno:InsertField (Type param), .uno:InsertDBField (Type param)
{ u"TYP_AUTHORFLD",                 SbxINTEGER,       CPROP_, ConstInt<SbTYP::AUTHOR>             },
{ u"TYP_CHAPTERFLD",                SbxINTEGER,       CPROP_, ConstInt<SbTYP::CHAPTER>            },
{ u"TYP_CONDTXTFLD",                SbxINTEGER,       CPROP_, ConstInt<SbTYP::CONDITIONALTEXT>    },
{ u"TYP_DATEFLD",                   SbxINTEGER,       CPROP_, ConstInt<SbTYP::DATE>               },
{ u"TYP_DBFLD",                     SbxINTEGER,       CPROP_, ConstInt<SbTYP::DATABASE>           },
{ u"TYP_DBNAMEFLD",                 SbxINTEGER,       CPROP_, ConstInt<SbTYP::DATABASENAME>       },
{ u"TYP_DBNEXTSETFLD",              SbxINTEGER,       CPROP_, ConstInt<SbTYP::DATABASENEXTSET>    },
{ u"TYP_DBNUMSETFLD",               SbxINTEGER,       CPROP_, ConstInt<SbTYP::DATABASENUMBERSET>  },
{ u"TYP_DBSETNUMBERFLD",            SbxINTEGER,       CPROP_, ConstInt<SbTYP::DATABASESETNUMBER>  },
{ u"TYP_DDEFLD",                    SbxINTEGER,       CPROP_, ConstInt<SbTYP::DDE>                },
{ u"TYP_DOCINFOFLD",                SbxINTEGER,       CPROP_, ConstInt<SbTYP::DOCUMENTINFO>       },
{ u"TYP_DOCSTATFLD",                SbxINTEGER,       CPROP_, ConstInt<SbTYP::DOCUMENTSTATISTICS> },
{ u"TYP_EXTUSERFLD",                SbxINTEGER,       CPROP_, ConstInt<SbTYP::EXTENDEDUSER>       },
{ u"TYP_FILENAMEFLD",               SbxINTEGER,       CPROP_, ConstInt<SbTYP::FILENAME>           },
{ u"TYP_FIXDATEFLD",                SbxINTEGER,       CPROP_, ConstInt<SbTYP::FIXEDDATE>          },
{ u"TYP_FIXTIMEFLD",                SbxINTEGER,       CPROP_, ConstInt<SbTYP::FIXEDTIME>          },
{ u"TYP_FORMELFLD",                 SbxINTEGER,       CPROP_, ConstInt<SbTYP::FORMEL>             },
{ u"TYP_GETFLD",                    SbxINTEGER,       CPROP_, ConstInt<SbTYP::GET>                },
{ u"TYP_GETREFFLD",                 SbxINTEGER,       CPROP_, ConstInt<SbTYP::GETREF>             },
{ u"TYP_GETREFPAGEFLD",             SbxINTEGER,       CPROP_, ConstInt<SbTYP::GETREFPAGE>         },
{ u"TYP_HIDDENPARAFLD",             SbxINTEGER,       CPROP_, ConstInt<SbTYP::HIDDENPARAGRAPH>    },
{ u"TYP_HIDDENTXTFLD",              SbxINTEGER,       CPROP_, ConstInt<SbTYP::HIDDENTEXT>         },
{ u"TYP_INPUTFLD",                  SbxINTEGER,       CPROP_, ConstInt<SbTYP::INPUT>              },
{ u"TYP_INTERNETFLD",               SbxINTEGER,       CPROP_, ConstInt<SbTYP::INTERNET>           },
{ u"TYP_JUMPEDITFLD",               SbxINTEGER,       CPROP_, ConstInt<SbTYP::JUMPEDIT>           },
{ u"TYP_MACROFLD",                  SbxINTEGER,       CPROP_, ConstInt<SbTYP::MACRO>              },
{ u"TYP_NEXTPAGEFLD",               SbxINTEGER,       CPROP_, ConstInt<SbTYP::NEXTPAGE>           },
{ u"TYP_PAGENUMBERFLD",             SbxINTEGER,       CPROP_, ConstInt<SbTYP::PAGENUMBER>         },
{ u"TYP_POSTITFLD",                 SbxINTEGER,       CPROP_, ConstInt<SbTYP::POSTIT>             },
{ u"TYP_PREVPAGEFLD",               SbxINTEGER,       CPROP_, ConstInt<SbTYP::PREVIOUSPAGE>       },
{ u"TYP_SEQFLD",                    SbxINTEGER,       CPROP_, ConstInt<SbTYP::SEQUENCE>           },
{ u"TYP_SETFLD",                    SbxINTEGER,       CPROP_, ConstInt<SbTYP::SET>                },
{ u"TYP_SETINPFLD",                 SbxINTEGER,       CPROP_, ConstInt<SbTYP::SETINPUT>           },
{ u"TYP_SETREFFLD",                 SbxINTEGER,       CPROP_, ConstInt<SbTYP::SETREF>             },
{ u"TYP_SETREFPAGEFLD",             SbxINTEGER,       CPROP_, ConstInt<SbTYP::SETREFPAGE>         },
{ u"TYP_TEMPLNAMEFLD",              SbxINTEGER,       CPROP_, ConstInt<SbTYP::TEMPLATENAME>       },
{ u"TYP_TIMEFLD",                   SbxINTEGER,       CPROP_, ConstInt<SbTYP::TIME>               },
{ u"TYP_USERFLD",                   SbxINTEGER,       CPROP_, ConstInt<SbTYP::USER>               },
{ u"TYP_USRINPFLD",                 SbxINTEGER,       CPROP_, ConstInt<SbTYP::USERINPUT>          },

{ u"TypeLen",                       SbxINTEGER,   1 | FUNCTION_,       SbRtl_TypeLen              },
    arg(u"Var", SbxVARIANT),

{ u"TypeName",                      SbxSTRING,    1 | FUNCTION_,       SbRtl_TypeName             },
    arg(u"Varname", SbxVARIANT),

{ u"UBound",                        SbxLONG,      1 | FUNCTION_,       SbRtl_UBound               },
    arg(u"Var", SbxVARIANT),

{ u"UCase",                         SbxSTRING,    1 | FUNCTION_,       SbRtl_UCase                },
    arg(u"String", SbxSTRING),

{ u"Unload",                        SbxNULL,      1 | FUNCTION_,       SbRtl_Unload               },
    arg(u"Dialog", SbxOBJECT),

{ u"Val",                           SbxDOUBLE,    1 | FUNCTION_,       SbRtl_Val                  },
    arg(u"String", SbxSTRING),

{ u"VarType",                       SbxINTEGER,   1 | FUNCTION_,       SbRtl_VarType              },
    arg(u"Varname", SbxVARIANT),

// Related to: VarType
{ u"V_EMPTY",                       SbxINTEGER,       CPROP_,          ConstInt<SbxEMPTY>         },
{ u"V_NULL",                        SbxINTEGER,       CPROP_,          ConstInt<SbxNULL>          },
{ u"V_INTEGER",                     SbxINTEGER,       CPROP_,          ConstInt<SbxINTEGER>       },
{ u"V_LONG",                        SbxINTEGER,       CPROP_,          ConstInt<SbxLONG>          },
{ u"V_SINGLE",                      SbxINTEGER,       CPROP_,          ConstInt<SbxSINGLE>        },
{ u"V_DOUBLE",                      SbxINTEGER,       CPROP_,          ConstInt<SbxDOUBLE>        },
{ u"V_CURRENCY",                    SbxINTEGER,       CPROP_,          ConstInt<SbxCURRENCY>      },
{ u"V_DATE",                        SbxINTEGER,       CPROP_,          ConstInt<SbxDATE>          },
{ u"V_STRING",                      SbxINTEGER,       CPROP_,          ConstInt<SbxSTRING>        },

{ u"Wait",                          SbxNULL,      1 | FUNCTION_,       SbRtl_Wait                 },
    arg(u"Milliseconds", SbxLONG),

{ u"FuncCaller",                    SbxVARIANT,       FUNCTION_,       SbRtl_FuncCaller           },
//#i64882#
{ u"WaitUntil",                     SbxNULL,      1 | FUNCTION_,       SbRtl_WaitUntil            },
    arg(u"Date", SbxDOUBLE),

{ u"Weekday",                       SbxINTEGER,   2 | FUNCTION_,       SbRtl_Weekday              },
    arg(u"Date",           SbxDATE),
    arg(u"Firstdayofweek", SbxINTEGER, OPT_),

{ u"WeekdayName",                   SbxSTRING,    3 | FUNCTION_ | COMPATONLY_, SbRtl_WeekdayName  },
    arg(u"Weekday",        SbxINTEGER),
    arg(u"Abbreviate",     SbxBOOL,    OPT_),
    arg(u"Firstdayofweek", SbxINTEGER, OPT_),

{ u"Year",                          SbxINTEGER,   1 | FUNCTION_,       SbRtl_Year                 },
    arg(u"Date", SbxDATE),
};  // end of the table

static_assert(MethodsTableValid(aMethods));

// building the info-structure for single elements
// if nIdx = 0, don't create anything (Std-Props!)

SbxInfo* GetMethodInfo(std::size_t nIdx)
{
    if (!nIdx)
        return nullptr;
    assert(nIdx <= std::size(aMethods));
    const Method* p = &aMethods[nIdx - 1];
    SbxInfo* pInfo_ = new SbxInfo;
    short nPar = p->nArgs & ARGSMASK_;
    for (short i = 0; i < nPar; i++)
    {
        p++;
        SbxFlagBits nFlags_ = static_cast<SbxFlagBits>((p->nArgs >> 8) & 0x03);
        if (p->nArgs & OPT_)
            nFlags_ |= SbxFlagBits::Optional;
        pInfo_->AddParam(OUString(p->sName), p->eType, nFlags_);
    }
    return pInfo_;
}
}

SbiStdObject::SbiStdObject( const OUString& r, StarBASIC* pb ) : SbxObject( r )
{
    // #i92642: Remove default properties
    Remove( u"Name"_ustr, SbxClassType::DontCare );
    Remove( u"Parent"_ustr, SbxClassType::DontCare );

    SetParent( pb );

    pStdFactory.emplace();
    SbxBase::AddFactory( &*pStdFactory );

    Insert( new SbStdClipboard );
}

SbiStdObject::~SbiStdObject()
{
    SbxBase::RemoveFactory( &*pStdFactory );
    pStdFactory.reset();
}

// Finding an element:
// It runs linearly through the method table here until an
// adequate method is has been found. Because of the bits in
// the nArgs-field the adequate instance of an SbxObjElement
// is created then. If the method/property hasn't been found,
// return NULL without error code, so that a whole chain of
// objects can be asked for the method/property.

SbxVariable* SbiStdObject::Find( const OUString& rName, SbxClassType t )
{
    // entered already?
    SbxVariable* pVar = SbxObject::Find( rName, t );
    if( !pVar )
    {
        // else search one
        sal_uInt16 nHash_ = SbxVariable::MakeHashCode( rName );
        auto p = std::begin(aMethods);
        bool bFound = false;
        short nIndex = 0;
        sal_uInt16 nSrchMask = TYPEMASK_;
        switch( t )
        {
            case SbxClassType::Method:   nSrchMask = METHOD_; break;
            case SbxClassType::Property: nSrchMask = PROPERTY_; break;
            case SbxClassType::Object:   nSrchMask = OBJECT_; break;
            default: break;
        }
        while (p != std::end(aMethods))
        {
            assert(p < std::end(aMethods));
            if( ( p->nArgs & nSrchMask )
             && ( p->nHash == nHash_ )
                && (rName.equalsIgnoreAsciiCase(p->sName)))
            {
                bFound = true;
                if( p->nArgs & COMPTMASK_ )
                {
                    bool bCompatibility = false;
                    SbiInstance* pInst = GetSbData()->pInst;
                    if (pInst)
                    {
                        bCompatibility = pInst->IsCompatibility();
                    }
                    else
                    {
                        // No instance running => compiling a source on module level.
                        const SbModule* pModule = GetSbData()->pCompMod;
                        if (pModule)
                            bCompatibility = pModule->IsVBASupport();
                    }
                    if ((bCompatibility && (NORMONLY_ & p->nArgs)) || (!bCompatibility && (COMPATONLY_ & p->nArgs)))
                        bFound = false;
                }
                break;
            }
            nIndex += ( p->nArgs & ARGSMASK_ ) + 1;
            p = aMethods + nIndex;
        }

        if( bFound )
        {
            // isolate Args-fields:
            SbxFlagBits nAccess = static_cast<SbxFlagBits>(( p->nArgs & RWMASK_ ) >> 8);
            short nType   = ( p->nArgs & TYPEMASK_ );
            if( p->nArgs & CONST_ )
                nAccess |= SbxFlagBits::Const;
            SbxClassType eCT = SbxClassType::Object;
            if( nType & PROPERTY_ )
            {
                eCT = SbxClassType::Property;
            }
            else if( nType & METHOD_ )
            {
                eCT = SbxClassType::Method;
            }
            pVar = Make(OUString(p->sName), eCT, p->eType, (p->nArgs & FUNCTION_) == FUNCTION_);
            pVar->SetUserData( nIndex + 1 );
            pVar->SetFlags( nAccess );
        }
    }
    return pVar;
}

// SetModified must be pinched off at the RTL
void SbiStdObject::SetModified( bool )
{
}


void SbiStdObject::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )

{
    const SbxHint* pHint = dynamic_cast<const SbxHint*>(&rHint);
    if( !pHint )
        return;

    SbxVariable* pVar = pHint->GetVar();
    SbxArray* pPar_ = pVar->GetParameters();
    const std::size_t nCallId = pVar->GetUserData();
    if( nCallId )
    {
        const SfxHintId t = pHint->GetId();
        if( t == SfxHintId::BasicInfoWanted )
            pVar->SetInfo(GetMethodInfo(nCallId));
        else
        {
            assert(nCallId <= std::size(aMethods));
            bool bWrite = false;
            if( t == SfxHintId::BasicDataChanged )
                bWrite = true;
            if( t == SfxHintId::BasicDataWanted || bWrite )
            {
                RtlCall p = aMethods[ nCallId-1 ].pFunc;
                SbxArrayRef rPar( pPar_ );
                if( !pPar_ )
                {
                    rPar = pPar_ = new SbxArray;
                    pPar_->Put(pVar, 0);
                }
                p( static_cast<StarBASIC*>(GetParent()), *pPar_, bWrite );
                return;
            }
        }
    }
    SbxObject::Notify( rBC, rHint );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
