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

#include <document.hxx>
#include <docsh.hxx>
#include <scerrors.hxx>
#include <fprogressbar.hxx>
#include <globstr.hrc>
#include <xlcontent.hxx>
#include <xltracer.hxx>
#include <xltable.hxx>
#include <xihelper.hxx>
#include <xipage.hxx>
#include <xiview.hxx>
#include <xilink.hxx>
#include <xiname.hxx>
#include <xlname.hxx>
#include <xicontent.hxx>
#include <xiescher.hxx>
#include <xipivot.hxx>
#include <xistyle.hxx>
#include <XclImpChangeTrack.hxx>
#include <documentimport.hxx>

#include <root.hxx>
#include <imp_op.hxx>
#include <excimp8.hxx>

#include <comphelper/configuration.hxx>

#include <memory>

namespace
{
    bool TryStartNextRecord(XclImpStream& rIn, std::size_t nProgressBasePos)
    {
        bool bValid = true;
        // i#115255 fdo#40304 BOUNDSHEET doesn't point to a valid
        // BOF record position.  Scan the records manually (from
        // the BOUNDSHEET position) until we find a BOF.  Some 3rd
        // party Russian programs generate invalid xls docs with
        // this kind of silliness.
        if (rIn.PeekRecId(nProgressBasePos) == EXC_ID5_BOF)
            // BOUNDSHEET points to a valid BOF record.  Good.
            rIn.StartNextRecord(nProgressBasePos);
        else
        {
            while (bValid && rIn.GetRecId() != EXC_ID5_BOF)
                bValid = rIn.StartNextRecord();
        }
        return bValid;
    }
}

ErrCode ImportExcel::Read()
{
    XclImpPageSettings&     rPageSett       = GetPageSettings();
    XclImpTabViewSettings&  rTabViewSett    = GetTabViewSettings();
    XclImpPalette&          rPal            = GetPalette();
    XclImpFontBuffer&       rFontBfr        = GetFontBuffer();
    XclImpNumFmtBuffer&     rNumFmtBfr      = GetNumFmtBuffer();
    XclImpXFBuffer&         rXFBfr          = GetXFBuffer();
    XclImpNameManager&      rNameMgr        = GetNameManager();
    // call to GetCurrSheetDrawing() cannot be cached (changes in new sheets)

    enum STATE {
        Z_BiffNull, // not a valid Biff-Format
        Z_Biff2,    // Biff2: only one table

        Z_Biff3,    // Biff3: only one table

        Z_Biff4,    // Biff4: only one table
        Z_Biff4W,   // Biff4 Workbook: Globals
        Z_Biff4T,   // Biff4 Workbook: a table itself
        Z_Biff4E,   // Biff4 Workbook: between tables

        Z_Biff5WPre,// Biff5: Prefetch Workbook
        Z_Biff5W,   // Biff5: Globals
        Z_Biff5TPre,// Biff5: Prefetch for Shrfmla/Array Formula
        Z_Biff5T,   // Biff5: a table itself
        Z_Biff5E,   // Biff5: between tables
        Z_Biffn0,   // all Biffs: skip table till next EOF
        Z_End };

    STATE           eCurrent = Z_BiffNull, ePrev = Z_BiffNull;

    ErrCode         eLastErr = ERRCODE_NONE;
    sal_uInt16      nOpcode;
    sal_uInt16      nBofLevel = 0;

    std::unique_ptr< ScfSimpleProgressBar > pProgress( new ScfSimpleProgressBar(
        aIn.GetSvStreamSize(), GetDocShell(), STR_LOAD_DOC ) );

    /*  #i104057# Need to track a base position for progress bar calculation,
        because sheet substreams may not be in order of sheets. */
    std::size_t nProgressBasePos = 0;
    std::size_t nProgressBaseSize = 0;

    for (; eCurrent != Z_End; mnLastRecId = nOpcode)
    {
        if( eCurrent == Z_Biff5E )
        {
            sal_uInt16 nScTab = GetCurrScTab();
            if( nScTab < maSheetOffsets.size()  )
            {
                nProgressBaseSize += (aIn.GetSvStreamPos() - nProgressBasePos);
                nProgressBasePos = maSheetOffsets[ nScTab ];

                bool bValid = TryStartNextRecord(aIn, nProgressBasePos);
                if (!bValid)
                {
                    // Safeguard ourselves from potential infinite loop.
                    eCurrent = Z_End;
                }
            }
            else
                eCurrent = Z_End;
        }
        else
            aIn.StartNextRecord();

        nOpcode = aIn.GetRecId();

        if( !aIn.IsValid() )
        {
            // finalize table if EOF is missing
            switch( eCurrent )
            {
                case Z_Biff2:
                case Z_Biff3:
                case Z_Biff4:
                case Z_Biff4T:
                case Z_Biff5TPre:
                case Z_Biff5T:
                    rNumFmtBfr.CreateScFormats();
                    Eof();
                break;
                default:;
            }
            break;
        }

        if( eCurrent == Z_End )
            break;

        if( eCurrent != Z_Biff5TPre && eCurrent != Z_Biff5WPre )
            pProgress->ProgressAbs( nProgressBaseSize + aIn.GetSvStreamPos() - nProgressBasePos );

        switch( eCurrent )
        {

            case Z_BiffNull:    // ------------------------------- Z_BiffNull -
            {
                switch( nOpcode )
                {
                    case EXC_ID2_BOF:
                    case EXC_ID3_BOF:
                    case EXC_ID4_BOF:
                    case EXC_ID5_BOF:
                    {
                        // #i23425# don't rely on the record ID, but on the detected BIFF version
                        switch( GetBiff() )
                        {
                            case EXC_BIFF2:
                                Bof2();
                                if( pExcRoot->eDateiTyp == Biff2 )
                                {
                                    eCurrent = Z_Biff2;
                                    NewTable();
                                }
                            break;
                            case EXC_BIFF3:
                                Bof3();
                                if( pExcRoot->eDateiTyp == Biff3 )
                                {
                                    eCurrent = Z_Biff3;
                                    NewTable();
                                }
                            break;
                            case EXC_BIFF4:
                                Bof4();
                                if( pExcRoot->eDateiTyp == Biff4 )
                                {
                                    eCurrent = Z_Biff4;
                                    NewTable();
                                }
                                else if( pExcRoot->eDateiTyp == Biff4W )
                                    eCurrent = Z_Biff4W;
                            break;
                            case EXC_BIFF5:
                                Bof5();
                                if( pExcRoot->eDateiTyp == Biff5W )
                                {
                                    eCurrent = Z_Biff5WPre;

                                    nBdshtTab = 0;

                                    aIn.StoreGlobalPosition(); // store position
                                }
                                else if( pExcRoot->eDateiTyp == Biff5 )
                                {
                                    // #i62752# possible to have BIFF5 sheet without globals
                                    NewTable();
                                    eCurrent = Z_Biff5TPre;  // Shrfmla Prefetch, Row-Prefetch
                                    nBofLevel = 0;
                                    aIn.StoreGlobalPosition(); // store position
                                }
                            break;
                            default:
                                DBG_ERROR_BIFF();
                        }
                    }
                    break;
                }
            }
                break;

            case Z_Biff2:       // ---------------------------------- Z_Biff2 -
            {
                switch( nOpcode )
                {
                    case EXC_ID2_DIMENSIONS:
                    case EXC_ID3_DIMENSIONS:    ReadDimensions();       break;
                    case EXC_ID2_BLANK:
                    case EXC_ID3_BLANK:         ReadBlank();            break;
                    case EXC_ID2_INTEGER:       ReadInteger();          break;
                    case EXC_ID2_NUMBER:
                    case EXC_ID3_NUMBER:        ReadNumber();           break;
                    case EXC_ID2_LABEL:
                    case EXC_ID3_LABEL:         ReadLabel();            break;
                    case EXC_ID2_BOOLERR:
                    case EXC_ID3_BOOLERR:       ReadBoolErr();          break;
                    case EXC_ID_RK:             ReadRk();               break;

                    case 0x06:  Formula25(); break;     // FORMULA      [ 2  5]
                    case 0x08:  Row25(); break;         // ROW          [ 2  5]
                    case 0x0A:                          // EOF          [ 2345]
                        rNumFmtBfr.CreateScFormats();
                        rNameMgr.ConvertAllTokens();
                        Eof();
                        eCurrent = Z_End;
                        break;
                    case 0x14:
                    case 0x15:  rPageSett.ReadHeaderFooter( maStrm );   break;
                    case 0x17:  Externsheet(); break;   // EXTERNSHEET  [ 2345]
                    case 0x18:  rNameMgr.ReadName( maStrm );            break;
                    case 0x1C:  GetCurrSheetDrawing().ReadNote( maStrm );break;
                    case 0x1D:  rTabViewSett.ReadSelection( maStrm );   break;
                    case 0x1E:  rNumFmtBfr.ReadFormat( maStrm );        break;
                    case 0x20:  Columndefault(); break; // COLUMNDEFAULT[ 2   ]
                    case 0x21:  Array25(); break;       // ARRAY        [ 2  5]
                    case 0x23:  Externname25(); break;  // EXTERNNAME   [ 2  5]
                    case 0x24:  Colwidth(); break;      // COLWIDTH     [ 2   ]
                    case 0x25:  Defrowheight2(); break; // DEFAULTROWHEI[ 2   ]
                    case 0x26:
                    case 0x27:
                    case 0x28:
                    case 0x29:  rPageSett.ReadMargin( maStrm );         break;
                    case 0x2A:  rPageSett.ReadPrintHeaders( maStrm );   break;
                    case 0x2B:  rPageSett.ReadPrintGridLines( maStrm ); break;
                    case 0x2F:                          // FILEPASS     [ 2345]
                        eLastErr = XclImpDecryptHelper::ReadFilepass( maStrm );
                        if( eLastErr != ERRCODE_NONE )
                            eCurrent = Z_End;
                        break;
                    case EXC_ID2_FONT:  rFontBfr.ReadFont( maStrm );    break;
                    case EXC_ID_EFONT:  rFontBfr.ReadEfont( maStrm );   break;
                    case 0x3E:  rTabViewSett.ReadWindow2( maStrm, false );break;
                    case 0x41:  rTabViewSett.ReadPane( maStrm );        break;
                    case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345]
                    case 0x43:  rXFBfr.ReadXF( maStrm );                break;
                    case 0x44:  Ixfe(); break;          // IXFE         [ 2   ]
                }
            }
                break;

            case Z_Biff3:       // ---------------------------------- Z_Biff3 -
            {
                switch( nOpcode )
                {
                    // skip chart substream
                    case EXC_ID2_BOF:
                    case EXC_ID3_BOF:
                    case EXC_ID4_BOF:
                    case EXC_ID5_BOF:           XclTools::SkipSubStream( maStrm );  break;

                    case EXC_ID2_DIMENSIONS:
                    case EXC_ID3_DIMENSIONS:    ReadDimensions();       break;
                    case EXC_ID2_BLANK:
                    case EXC_ID3_BLANK:         ReadBlank();            break;
                    case EXC_ID2_INTEGER:       ReadInteger();          break;
                    case EXC_ID2_NUMBER:
                    case EXC_ID3_NUMBER:        ReadNumber();           break;
                    case EXC_ID2_LABEL:
                    case EXC_ID3_LABEL:         ReadLabel();            break;
                    case EXC_ID2_BOOLERR:
                    case EXC_ID3_BOOLERR:       ReadBoolErr();          break;
                    case EXC_ID_RK:             ReadRk();               break;

                    case 0x0A:                          // EOF          [ 2345]
                        rNumFmtBfr.CreateScFormats();
                        rNameMgr.ConvertAllTokens();
                        Eof();
                        eCurrent = Z_End;
                        break;
                    case 0x14:
                    case 0x15:  rPageSett.ReadHeaderFooter( maStrm );   break;
                    case 0x17:  Externsheet(); break;   // EXTERNSHEET  [ 2345]
                    case 0x1A:
                    case 0x1B:  rPageSett.ReadPageBreaks( maStrm );     break;
                    case 0x1C:  GetCurrSheetDrawing().ReadNote( maStrm );break;
                    case 0x1D:  rTabViewSett.ReadSelection( maStrm );   break;
                    case 0x1E:  rNumFmtBfr.ReadFormat( maStrm );        break;
                    case 0x22:  Rec1904(); break;       // 1904         [ 2345]
                    case 0x26:
                    case 0x27:
                    case 0x28:
                    case 0x29:  rPageSett.ReadMargin( maStrm );         break;
                    case 0x2A:  rPageSett.ReadPrintHeaders( maStrm );   break;
                    case 0x2B:  rPageSett.ReadPrintGridLines( maStrm ); break;
                    case 0x2F:                          // FILEPASS     [ 2345]
                        eLastErr = XclImpDecryptHelper::ReadFilepass( maStrm );
                        if( eLastErr != ERRCODE_NONE )
                            eCurrent = Z_End;
                        break;
                    case EXC_ID_FILESHARING: ReadFileSharing();         break;
                    case 0x41:  rTabViewSett.ReadPane( maStrm );        break;
                    case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345]
                    case 0x56:  break;                  // BUILTINFMTCNT[  34 ]
                    case 0x5D:  GetCurrSheetDrawing().ReadObj( maStrm );break;
                    case 0x7D:  Colinfo(); break;       // COLINFO      [  345]
                    case 0x8C:  Country(); break;       // COUNTRY      [  345]
                    case 0x92:  rPal.ReadPalette( maStrm );             break;
                    case 0x0206: Formula3(); break;     // FORMULA      [  3  ]
                    case 0x0208: Row34(); break;        // ROW          [  34 ]
                    case 0x0218: rNameMgr.ReadName( maStrm );           break;
                    case 0x0221: Array34(); break;      // ARRAY        [  34 ]
                    case 0x0223: break;                 // EXTERNNAME   [  34 ]
                    case 0x0225: Defrowheight345();break;//DEFAULTROWHEI[  345]
                    case 0x0231: rFontBfr.ReadFont( maStrm );           break;
                    case 0x023E: rTabViewSett.ReadWindow2( maStrm, false );break;
                    case 0x0243: rXFBfr.ReadXF( maStrm );               break;
                    case 0x0293: rXFBfr.ReadStyle( maStrm );            break;
                }
            }
                break;

            case Z_Biff4:       // ---------------------------------- Z_Biff4 -
            {
                switch( nOpcode )
                {
                    // skip chart substream
                    case EXC_ID2_BOF:
                    case EXC_ID3_BOF:
                    case EXC_ID4_BOF:
                    case EXC_ID5_BOF:           XclTools::SkipSubStream( maStrm );  break;

                    case EXC_ID2_DIMENSIONS:
                    case EXC_ID3_DIMENSIONS:    ReadDimensions();       break;
                    case EXC_ID2_BLANK:
                    case EXC_ID3_BLANK:         ReadBlank();            break;
                    case EXC_ID2_INTEGER:       ReadInteger();          break;
                    case EXC_ID2_NUMBER:
                    case EXC_ID3_NUMBER:        ReadNumber();           break;
                    case EXC_ID2_LABEL:
                    case EXC_ID3_LABEL:         ReadLabel();            break;
                    case EXC_ID2_BOOLERR:
                    case EXC_ID3_BOOLERR:       ReadBoolErr();          break;
                    case EXC_ID_RK:             ReadRk();               break;

                    case 0x0A:                          // EOF          [ 2345]
                        rNumFmtBfr.CreateScFormats();
                        rNameMgr.ConvertAllTokens();
                        Eof();
                        eCurrent = Z_End;
                        break;
                    case 0x12:  SheetProtect(); break;       // SHEET PROTECTION
                    case 0x14:
                    case 0x15:  rPageSett.ReadHeaderFooter( maStrm );   break;
                    case 0x17:  Externsheet(); break;   // EXTERNSHEET  [ 2345]
                    case 0x1A:
                    case 0x1B:  rPageSett.ReadPageBreaks( maStrm );     break;
                    case 0x1C:  GetCurrSheetDrawing().ReadNote( maStrm );break;
                    case 0x1D:  rTabViewSett.ReadSelection( maStrm );   break;
                    case 0x22:  Rec1904(); break;       // 1904         [ 2345]
                    case 0x26:
                    case 0x27:
                    case 0x28:
                    case 0x29:  rPageSett.ReadMargin( maStrm );         break;
                    case 0x2A:  rPageSett.ReadPrintHeaders( maStrm );   break;
                    case 0x2B:  rPageSett.ReadPrintGridLines( maStrm ); break;
                    case 0x2F:                          // FILEPASS     [ 2345]
                        eLastErr = XclImpDecryptHelper::ReadFilepass( maStrm );
                        if( eLastErr != ERRCODE_NONE )
                            eCurrent = Z_End;
                        break;
                    case EXC_ID_FILESHARING: ReadFileSharing();         break;
                    case 0x41:  rTabViewSett.ReadPane( maStrm );        break;
                    case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345]
                    case 0x55:  DefColWidth(); break;
                    case 0x56:  break;                  // BUILTINFMTCNT[  34 ]
                    case 0x5D:  GetCurrSheetDrawing().ReadObj( maStrm );break;
                    case 0x7D:  Colinfo(); break;       // COLINFO      [  345]
                    case 0x8C:  Country(); break;       // COUNTRY      [  345]
                    case 0x92:  rPal.ReadPalette( maStrm );             break;
                    case 0x99:  Standardwidth(); break; // STANDARDWIDTH[   45]
                    case 0xA1:  rPageSett.ReadSetup( maStrm );          break;
                    case 0x0208: Row34(); break;        // ROW          [  34 ]
                    case 0x0218: rNameMgr.ReadName( maStrm );           break;
                    case 0x0221: Array34(); break;      // ARRAY        [  34 ]
                    case 0x0223: break;                 // EXTERNNAME   [  34 ]
                    case 0x0225: Defrowheight345();break;//DEFAULTROWHEI[  345]
                    case 0x0231: rFontBfr.ReadFont( maStrm );           break;
                    case 0x023E: rTabViewSett.ReadWindow2( maStrm, false );break;
                    case 0x0406: Formula4(); break;     // FORMULA      [   4 ]
                    case 0x041E: rNumFmtBfr.ReadFormat( maStrm );       break;
                    case 0x0443: rXFBfr.ReadXF( maStrm );               break;
                    case 0x0293: rXFBfr.ReadStyle( maStrm );            break;
                }
            }
                break;

            case Z_Biff4W:      // --------------------------------- Z_Biff4W -
            {
                switch( nOpcode )
                {
                    case 0x0A:                          // EOF          [ 2345]
                        rNameMgr.ConvertAllTokens();
                        eCurrent = Z_End;
                        break;
                    case 0x12:  DocProtect(); break;    // PROTECT      [    5]
                    case 0x2F:                          // FILEPASS     [ 2345]
                        eLastErr = XclImpDecryptHelper::ReadFilepass( maStrm );
                        if( eLastErr != ERRCODE_NONE )
                            eCurrent = Z_End;
                        break;
                    case EXC_ID_FILESHARING: ReadFileSharing();         break;
                    case 0x17:  Externsheet(); break;   // EXTERNSHEET  [ 2345]
                    case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345]
                    case 0x55:  DefColWidth(); break;
                    case 0x56:  break;                  // BUILTINFMTCNT[  34 ]
                    case 0x8C:  Country(); break;       // COUNTRY      [  345]
                    case 0x8F:  break;                  // BUNDLEHEADER [   4 ]
                    case 0x92:  rPal.ReadPalette( maStrm );             break;
                    case 0x99:  Standardwidth(); break; // STANDARDWIDTH[   45]
                    case 0x0218: rNameMgr.ReadName( maStrm );           break;
                    case 0x0223: break;                 // EXTERNNAME   [  34 ]
                    case 0x0225: Defrowheight345();break;//DEFAULTROWHEI[  345]
                    case 0x0231: rFontBfr.ReadFont( maStrm );           break;
                    case EXC_ID4_BOF:                   // BOF          [   4 ]
                        Bof4();
                        if( pExcRoot->eDateiTyp == Biff4 )
                        {
                            eCurrent = Z_Biff4T;
                            NewTable();
                        }
                        else
                            eCurrent = Z_End;
                        break;
                    case 0x041E: rNumFmtBfr.ReadFormat( maStrm );       break;
                    case 0x0443: rXFBfr.ReadXF( maStrm );               break;
                    case 0x0293: rXFBfr.ReadStyle( maStrm );            break;
                }

            }
                break;

            case Z_Biff4T:       // --------------------------------- Z_Biff4T -
            {
                switch( nOpcode )
                {
                    // skip chart substream
                    case EXC_ID2_BOF:
                    case EXC_ID3_BOF:
                    case EXC_ID4_BOF:
                    case EXC_ID5_BOF:           XclTools::SkipSubStream( maStrm );  break;

                    case EXC_ID2_DIMENSIONS:
                    case EXC_ID3_DIMENSIONS:    ReadDimensions();       break;
                    case EXC_ID2_BLANK:
                    case EXC_ID3_BLANK:         ReadBlank();            break;
                    case EXC_ID2_INTEGER:       ReadInteger();          break;
                    case EXC_ID2_NUMBER:
                    case EXC_ID3_NUMBER:        ReadNumber();           break;
                    case EXC_ID2_LABEL:
                    case EXC_ID3_LABEL:         ReadLabel();            break;
                    case EXC_ID2_BOOLERR:
                    case EXC_ID3_BOOLERR:       ReadBoolErr();          break;
                    case EXC_ID_RK:             ReadRk();               break;

                    case 0x0A:                          // EOF          [ 2345]
                        rNameMgr.ConvertAllTokens();
                        Eof();
                        eCurrent = Z_Biff4E;
                    break;
                    case 0x12:  SheetProtect(); break;       // SHEET PROTECTION
                    case 0x14:
                    case 0x15:  rPageSett.ReadHeaderFooter( maStrm );   break;
                    case 0x1A:
                    case 0x1B:  rPageSett.ReadPageBreaks( maStrm );     break;
                    case 0x1C:  GetCurrSheetDrawing().ReadNote( maStrm );break;
                    case 0x1D:  rTabViewSett.ReadSelection( maStrm );   break;
                    case 0x2F:                          // FILEPASS     [ 2345]
                        eLastErr = XclImpDecryptHelper::ReadFilepass( maStrm );
                        if( eLastErr != ERRCODE_NONE )
                            eCurrent = Z_End;
                        break;
                    case 0x41:  rTabViewSett.ReadPane( maStrm );        break;
                    case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345]
                    case 0x55:  DefColWidth(); break;
                    case 0x56:  break;                  // BUILTINFMTCNT[  34 ]
                    case 0x5D:  GetCurrSheetDrawing().ReadObj( maStrm );break;
                    case 0x7D:  Colinfo(); break;       // COLINFO      [  345]
                    case 0x8C:  Country(); break;       // COUNTRY      [  345]
                    case 0x8F:  break;                  // BUNDLEHEADER [   4 ]
                    case 0x92:  rPal.ReadPalette( maStrm );             break;
                    case 0x99:  Standardwidth(); break; // STANDARDWIDTH[   45]
                    case 0xA1:  rPageSett.ReadSetup( maStrm );          break;
                    case 0x0208: Row34(); break;        // ROW          [  34 ]
                    case 0x0218: rNameMgr.ReadName( maStrm );           break;
                    case 0x0221: Array34(); break;
                    case 0x0225: Defrowheight345();break;//DEFAULTROWHEI[  345]
                    case 0x0231: rFontBfr.ReadFont( maStrm );           break;
                    case 0x023E: rTabViewSett.ReadWindow2( maStrm, false );break;
                    case 0x0406: Formula4(); break;
                    case 0x041E: rNumFmtBfr.ReadFormat( maStrm );       break;
                    case 0x0443: rXFBfr.ReadXF( maStrm );               break;
                    case 0x0293: rXFBfr.ReadStyle( maStrm );            break;
                }

            }
                break;

            case Z_Biff4E:      // --------------------------------- Z_Biff4E -
            {
                switch( nOpcode )
                {
                    case 0x0A:                          // EOF          [ 2345]
                        eCurrent = Z_End;
                        break;
                    case 0x8F:  break;                  // BUNDLEHEADER [   4 ]
                    case EXC_ID4_BOF:                   // BOF          [   4 ]
                        Bof4();
                        NewTable();
                        if( pExcRoot->eDateiTyp == Biff4 )
                        {
                            eCurrent = Z_Biff4T;
                        }
                        else
                        {
                            ePrev = eCurrent;
                            eCurrent = Z_Biffn0;
                        }
                        break;
                }

            }
                break;
            case Z_Biff5WPre:   // ------------------------------ Z_Biff5WPre -
            {
                switch( nOpcode )
                {
                    case 0x0A:                          // EOF          [ 2345]
                        eCurrent = Z_Biff5W;
                        aIn.SeekGlobalPosition();  // and back to old position
                        break;
                    case 0x12:  DocProtect(); break;    // PROTECT      [    5]
                    case 0x2F:                          // FILEPASS     [ 2345]
                        eLastErr = XclImpDecryptHelper::ReadFilepass( maStrm );
                        if( eLastErr != ERRCODE_NONE )
                            eCurrent = Z_End;
                        break;
                    case EXC_ID_FILESHARING: ReadFileSharing();         break;
                    case 0x3D:  Window1(); break;
                    case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345]
                    case 0x85:  Boundsheet(); break;    // BOUNDSHEET   [    5]
                    case 0x8C:  Country(); break;       // COUNTRY      [  345]
                    // PALETTE follows XFs, but already needed while reading the XFs
                    case 0x92:  rPal.ReadPalette( maStrm );             break;
                }
            }
                break;
            case Z_Biff5W:      // --------------------------------- Z_Biff5W -
            {
                switch( nOpcode )
                {
                    case 0x0A:                          // EOF          [ 2345]
                        rNumFmtBfr.CreateScFormats();
                        rXFBfr.CreateUserStyles();
                        rNameMgr.ConvertAllTokens();
                        eCurrent = Z_Biff5E;
                        break;
                    case 0x18:  rNameMgr.ReadName( maStrm );            break;
                    case 0x1E:  rNumFmtBfr.ReadFormat( maStrm );        break;
                    case 0x22:  Rec1904(); break;       // 1904         [ 2345]
                    case 0x31:  rFontBfr.ReadFont( maStrm );            break;
                    case 0x56:  break;                  // BUILTINFMTCNT[  34 ]
                    case 0x8D:  Hideobj(); break;       // HIDEOBJ      [  345]
                    case 0xDE:  Olesize(); break;
                    case 0xE0:  rXFBfr.ReadXF( maStrm );                break;
                    case 0x0293: rXFBfr.ReadStyle( maStrm );            break;
                    case 0x041E: rNumFmtBfr.ReadFormat( maStrm );       break;
                }

            }
                break;

            case Z_Biff5TPre:   // ------------------------------- Z_Biff5Pre -
            {
                if (nOpcode == EXC_ID5_BOF)
                    nBofLevel++;
                else if( (nOpcode == 0x000A) && nBofLevel )
                    nBofLevel--;
                else if( !nBofLevel )                       // don't read chart records
                {
                    switch( nOpcode )
                    {
                        case EXC_ID2_DIMENSIONS:
                        case EXC_ID3_DIMENSIONS:    ReadDimensions();       break;
                        case 0x08:  Row25(); break;         // ROW          [ 2  5]
                        case 0x0A:                          // EOF          [ 2345]
                            eCurrent = Z_Biff5T;
                            aIn.SeekGlobalPosition(); // and back to old position
                            break;
                        case 0x12:  SheetProtect(); break;       // SHEET PROTECTION
                        case 0x1A:
                        case 0x1B:  rPageSett.ReadPageBreaks( maStrm );     break;
                        case 0x1D:  rTabViewSett.ReadSelection( maStrm );   break;
                        case 0x17:  Externsheet(); break;   // EXTERNSHEET  [ 2345]
                        case 0x21:  Array25(); break;       // ARRAY        [ 2  5]
                        case 0x23:  Externname25(); break;  // EXTERNNAME   [ 2  5]
                        case 0x41:  rTabViewSett.ReadPane( maStrm );        break;
                        case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345]
                        case 0x55:  DefColWidth(); break;
                        case 0x7D:  Colinfo(); break;       // COLINFO      [  345]
                        case 0x81:  Wsbool(); break;        // WSBOOL       [ 2345]
                        case 0x8C:  Country(); break;       // COUNTRY      [  345]
                        case 0x99:  Standardwidth(); break; // STANDARDWIDTH[   45]
                        case 0x0208: Row34(); break;        // ROW          [  34 ]
                        case 0x0221: Array34(); break;      // ARRAY        [  34 ]
                        case 0x0223: break;                 // EXTERNNAME   [  34 ]
                        case 0x0225: Defrowheight345();break;//DEFAULTROWHEI[  345]
                        case 0x023E: rTabViewSett.ReadWindow2( maStrm, false );break;
                    }
                }
            }
                break;

            case Z_Biff5T:       // --------------------------------- Z_Biff5T -
            {
                switch( nOpcode )
                {
                    case EXC_ID2_BLANK:
                    case EXC_ID3_BLANK:         ReadBlank();            break;
                    case EXC_ID2_INTEGER:       ReadInteger();          break;
                    case EXC_ID2_NUMBER:
                    case EXC_ID3_NUMBER:        ReadNumber();           break;
                    case EXC_ID2_LABEL:
                    case EXC_ID3_LABEL:         ReadLabel();            break;
                    case EXC_ID2_BOOLERR:
                    case EXC_ID3_BOOLERR:       ReadBoolErr();          break;
                    case EXC_ID_RK:             ReadRk();               break;

                    case EXC_ID2_FORMULA:
                    case EXC_ID3_FORMULA:
                    case EXC_ID4_FORMULA:       Formula25(); break;
                    case EXC_ID_SHRFMLA: Shrfmla(); break;
                    case 0x0A:  Eof(); eCurrent = Z_Biff5E;                 break;
                    case 0x14:
                    case 0x15:  rPageSett.ReadHeaderFooter( maStrm );   break;
                    case 0x17:  Externsheet(); break;   // EXTERNSHEET  [ 2345]
                    case 0x1C:  GetCurrSheetDrawing().ReadNote( maStrm );break;
                    case 0x1D:  rTabViewSett.ReadSelection( maStrm );   break;
                    case 0x23:  Externname25(); break;  // EXTERNNAME   [ 2  5]
                    case 0x26:
                    case 0x27:
                    case 0x28:
                    case 0x29:  rPageSett.ReadMargin( maStrm );         break;
                    case 0x2A:  rPageSett.ReadPrintHeaders( maStrm );   break;
                    case 0x2B:  rPageSett.ReadPrintGridLines( maStrm ); break;
                    case 0x2F:                          // FILEPASS     [ 2345]
                        eLastErr = XclImpDecryptHelper::ReadFilepass( maStrm );
                        if( eLastErr != ERRCODE_NONE )
                            eCurrent = Z_End;
                        break;
                    case 0x5D:  GetCurrSheetDrawing().ReadObj( maStrm );break;
                    case 0x83:
                    case 0x84:  rPageSett.ReadCenter( maStrm );         break;
                    case 0xA0:  rTabViewSett.ReadScl( maStrm );         break;
                    case 0xA1:  rPageSett.ReadSetup( maStrm );          break;
                    case 0xBD:  Mulrk(); break;         // MULRK        [    5]
                    case 0xBE:  Mulblank(); break;      // MULBLANK     [    5]
                    case 0xD6:  Rstring(); break;       // RSTRING      [    5]
                    case 0x00E5: Cellmerging();          break;  // #i62300#
                    case 0x0236: TableOp(); break;      // TABLE        [    5]
                    case EXC_ID5_BOF:                   // BOF          [    5]
                        XclTools::SkipSubStream( maStrm );
                        break;
                }

            }
                break;

            case Z_Biff5E:      // --------------------------------- Z_Biff5E -
            {
                switch( nOpcode )
                {
                    case EXC_ID5_BOF:                   // BOF          [    5]
                        Bof5();
                        NewTable();
                        switch( pExcRoot->eDateiTyp )
                        {
                            case Biff5:
                            case Biff5M4:
                                eCurrent = Z_Biff5TPre; // Shrfmla Prefetch, Row-Prefetch
                                nBofLevel = 0;
                                aIn.StoreGlobalPosition(); // store position
                            break;
                            case Biff5C:    // chart sheet
                                GetCurrSheetDrawing().ReadTabChart( maStrm );
                                Eof();
                                GetTracer().TraceChartOnlySheet();
                            break;
                            case Biff5V:
                            default:
                                rD.SetVisible( GetCurrScTab(), false );
                                ePrev = eCurrent;
                                eCurrent = Z_Biffn0;
                        }
                        OSL_ENSURE( pExcRoot->eDateiTyp != Biff5W,
                            "+ImportExcel::Read(): Doppel-Whopper-Workbook!" );

                        break;
                }

            }
                break;
            case Z_Biffn0:      // --------------------------------- Z_Biffn0 -
            {
                switch( nOpcode )
                {
                    case 0x0A:                          // EOF          [ 2345]
                        eCurrent = ePrev;
                        IncCurrScTab();
                        break;
                }

            }
                break;

            case Z_End:        // ----------------------------------- Z_End -
                OSL_FAIL( "*ImportExcel::Read(): Not possible state!" );
                break;
            default: OSL_FAIL( "-ImportExcel::Read(): state forgotten!" );
        }
    }

    if( eLastErr == ERRCODE_NONE )
    {
        pProgress.reset();

        GetDocImport().finalize();
        if (!comphelper::IsFuzzing())
            AdjustRowHeight();
        PostDocLoad();

        rD.CalcAfterLoad(false);

        const XclImpAddressConverter& rAddrConv = GetAddressConverter();
        if( rAddrConv.IsTabTruncated() )
            eLastErr = SCWARN_IMPORT_SHEET_OVERFLOW;
        else if( bTabTruncated || rAddrConv.IsRowTruncated() )
            eLastErr = SCWARN_IMPORT_ROW_OVERFLOW;
        else if( rAddrConv.IsColTruncated() )
            eLastErr = SCWARN_IMPORT_COLUMN_OVERFLOW;
    }

    return eLastErr;
}

ErrCode ImportExcel8::Read()
{
#ifdef EXC_INCL_DUMPER
    {
        Biff8RecDumper aDumper( GetRoot(), sal_True );
        if( aDumper.Dump( aIn ) )
            return ERRCODE_ABORT;
    }
#endif
    // read the entire BIFF8 stream
    // don't look too close - this stuff seriously needs to be reworked

    XclImpPageSettings&     rPageSett       = GetPageSettings();
    XclImpTabViewSettings&  rTabViewSett    = GetTabViewSettings();
    XclImpPalette&          rPal            = GetPalette();
    XclImpFontBuffer&       rFontBfr        = GetFontBuffer();
    XclImpNumFmtBuffer&     rNumFmtBfr      = GetNumFmtBuffer();
    XclImpXFBuffer&         rXFBfr          = GetXFBuffer();
    XclImpSst&              rSst            = GetSst();
    XclImpTabInfo&          rTabInfo        = GetTabInfo();
    XclImpNameManager&      rNameMgr        = GetNameManager();
    XclImpLinkManager&      rLinkMgr        = GetLinkManager();
    XclImpObjectManager&    rObjMgr         = GetObjectManager();
    // call to GetCurrSheetDrawing() cannot be cached (changes in new sheets)
    XclImpCondFormatManager& rCondFmtMgr    = GetCondFormatManager();
    XclImpValidationManager& rValidMgr      = GetValidationManager();
    XclImpPivotTableManager& rPTableMgr     = GetPivotTableManager();
    XclImpWebQueryBuffer&   rWQBfr          = GetWebQueryBuffer();

    bool bInUserView = false;           // true = In USERSVIEW(BEGIN|END) record block.

    enum XclImpReadState
    {
        EXC_STATE_BEFORE_GLOBALS,       /// Before workbook globals (wait for initial BOF).
        EXC_STATE_GLOBALS_PRE,          /// Prefetch for workbook globals.
        EXC_STATE_GLOBALS,              /// Workbook globals.
        EXC_STATE_BEFORE_SHEET,         /// Before worksheet (wait for new worksheet BOF).
        EXC_STATE_SHEET_PRE,            /// Prefetch for worksheet.
        EXC_STATE_SHEET,                /// Worksheet.
        EXC_STATE_END                   /// Stop reading.
    };

    XclImpReadState eCurrent = EXC_STATE_BEFORE_GLOBALS;

    ErrCode eLastErr = ERRCODE_NONE;

    std::unique_ptr< ScfSimpleProgressBar > pProgress( new ScfSimpleProgressBar(
        aIn.GetSvStreamSize(), GetDocShell(), STR_LOAD_DOC ) );

    /*  #i104057# Need to track a base position for progress bar calculation,
        because sheet substreams may not be in order of sheets. */
    std::size_t nProgressBasePos = 0;
    std::size_t nProgressBaseSize = 0;

    bool bSheetHasCodeName = false;

    std::vector<OUString> aCodeNames;
    std::vector < SCTAB > nTabsWithNoCodeName;

    sal_uInt16 nRecId = 0;

    for (; eCurrent != EXC_STATE_END; mnLastRecId = nRecId)
    {
        if( eCurrent == EXC_STATE_BEFORE_SHEET )
        {
            sal_uInt16 nScTab = GetCurrScTab();
            if( nScTab < maSheetOffsets.size() )
            {
                nProgressBaseSize += (maStrm.GetSvStreamPos() - nProgressBasePos);
                nProgressBasePos = maSheetOffsets[ nScTab ];

                bool bValid = TryStartNextRecord(aIn, nProgressBasePos);
                if (!bValid)
                {
                    // Safeguard ourselves from potential infinite loop.
                    eCurrent = EXC_STATE_END;
                }

                // import only 256 sheets
                if( nScTab > GetScMaxPos().Tab() )
                {
                    if( maStrm.GetRecId() != EXC_ID_EOF )
                        XclTools::SkipSubStream( maStrm );
                    // #i29930# show warning box
                    GetAddressConverter().CheckScTab( nScTab );
                    eCurrent = EXC_STATE_END;
                }
                else
                {
                    // #i109800# SHEET record may point to any record inside the
                    // sheet substream
                    bool bIsBof = maStrm.GetRecId() == EXC_ID5_BOF;
                    if( bIsBof )
                        Bof5(); // read the BOF record
                    else
                        pExcRoot->eDateiTyp = Biff8;    // on missing BOF, assume a standard worksheet
                    NewTable();
                    switch( pExcRoot->eDateiTyp )
                    {
                    case Biff8:     // worksheet
                    case Biff8M4:   // macro sheet
                        eCurrent = EXC_STATE_SHEET_PRE;  // Shrfmla Prefetch, Row-Prefetch
                        // go to next record
                        if( bIsBof ) maStrm.StartNextRecord();
                        maStrm.StoreGlobalPosition();
                        break;
                    case Biff8C:    // chart sheet
                        GetCurrSheetDrawing().ReadTabChart( maStrm );
                        Eof();
                        GetTracer().TraceChartOnlySheet();
                        break;
                    case Biff8W:    // workbook
                        OSL_FAIL( "ImportExcel8::Read - double workbook globals" );
                        [[fallthrough]];
                    case Biff8V:    // VB module
                    default:
                        // TODO: do not create a sheet in the Calc document
                        rD.SetVisible( nScTab, false );
                        XclTools::SkipSubStream( maStrm );
                        IncCurrScTab();
                    }
                }
            }
            else
                eCurrent = EXC_STATE_END;
        }
        else
            aIn.StartNextRecord();

        if( !aIn.IsValid() )
        {
            // #i63591# finalize table if EOF is missing
            switch( eCurrent )
            {
                case EXC_STATE_SHEET_PRE:
                    eCurrent = EXC_STATE_SHEET;
                    aIn.SeekGlobalPosition();
                    continue;   // next iteration in while loop
                case EXC_STATE_SHEET:
                    Eof();
                    eCurrent = EXC_STATE_END;
                break;
                default:
                    eCurrent = EXC_STATE_END;
            }
        }

        if( eCurrent == EXC_STATE_END )
            break;

        if( eCurrent != EXC_STATE_SHEET_PRE && eCurrent != EXC_STATE_GLOBALS_PRE )
            pProgress->ProgressAbs( nProgressBaseSize + aIn.GetSvStreamPos() - nProgressBasePos );

        nRecId = aIn.GetRecId();

        /*  #i39464# Ignore records between USERSVIEWBEGIN and USERSVIEWEND
            completely (user specific view settings). Otherwise view settings
            and filters are loaded multiple times, which at least causes
            problems in auto-filters. */
        switch( nRecId )
        {
            case EXC_ID_USERSVIEWBEGIN:
                OSL_ENSURE( !bInUserView, "ImportExcel8::Read - nested user view settings" );
                bInUserView = true;
            break;
            case EXC_ID_USERSVIEWEND:
                OSL_ENSURE( bInUserView, "ImportExcel8::Read - not in user view settings" );
                bInUserView = false;
            break;
        }

        if( !bInUserView ) switch( eCurrent )
        {

            // before workbook globals: wait for initial workbook globals BOF
            case EXC_STATE_BEFORE_GLOBALS:
            {
                if( nRecId == EXC_ID5_BOF )
                {
                    OSL_ENSURE( GetBiff() == EXC_BIFF8, "ImportExcel8::Read - wrong BIFF version" );
                    Bof5();
                    if( pExcRoot->eDateiTyp == Biff8W )
                    {
                        eCurrent = EXC_STATE_GLOBALS_PRE;
                        maStrm.StoreGlobalPosition();
                        nBdshtTab = 0;
                    }
                    else if( pExcRoot->eDateiTyp == Biff8 )
                    {
                        // #i62752# possible to have BIFF8 sheet without globals
                        NewTable();
                        eCurrent = EXC_STATE_SHEET_PRE;  // Shrfmla Prefetch, Row-Prefetch
                        bSheetHasCodeName = false; // reset
                        aIn.StoreGlobalPosition();
                    }
                }
            }
            break;

            // prefetch for workbook globals
            case EXC_STATE_GLOBALS_PRE:
            {
                switch( nRecId )
                {
                    case EXC_ID_EOF:
                    case EXC_ID_EXTSST:
                        /*  #i56376# evil hack: if EOF for globals is missing,
                            simulate it. This hack works only for the bugdoc
                            given in the issue, where the sheet substreams
                            start directly after the EXTSST record. A future
                            implementation should be more robust against
                            missing EOFs. */
                        if( (nRecId == EXC_ID_EOF) ||
                            ((nRecId == EXC_ID_EXTSST) && (maStrm.GetNextRecId() == EXC_ID5_BOF)) )
                        {
                            eCurrent = EXC_STATE_GLOBALS;
                            aIn.SeekGlobalPosition();
                        }
                        break;
                    case 0x12:  DocProtect(); break;    // PROTECT      [    5678]
                    case 0x13:  DocPassword(); break;
                    case 0x19:  WinProtection(); break;
                    case 0x2F:                          // FILEPASS     [ 2345   ]
                        eLastErr = XclImpDecryptHelper::ReadFilepass( maStrm );
                        if( eLastErr != ERRCODE_NONE )
                            eCurrent = EXC_STATE_END;
                        break;
                    case EXC_ID_FILESHARING: ReadFileSharing();         break;
                    case 0x3D:  Window1(); break;
                    case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345   ]
                    case 0x85:  Boundsheet(); break;    // BOUNDSHEET   [    5   ]
                    case 0x8C:  Country(); break;       // COUNTRY      [  345   ]

                    // PALETTE follows XFs, but already needed while reading the XFs
                    case EXC_ID_PALETTE:        rPal.ReadPalette( maStrm );             break;
                }
            }
            break;

            // workbook globals
            case EXC_STATE_GLOBALS:
            {
                switch( nRecId )
                {
                    case EXC_ID_EOF:
                    case EXC_ID_EXTSST:
                        /*  #i56376# evil hack: if EOF for globals is missing,
                            simulate it. This hack works only for the bugdoc
                            given in the issue, where the sheet substreams
                            start directly after the EXTSST record. A future
                            implementation should be more robust against
                            missing EOFs. */
                        if( (nRecId == EXC_ID_EOF) ||
                            ((nRecId == EXC_ID_EXTSST) && (maStrm.GetNextRecId() == EXC_ID5_BOF)) )
                        {
                            rNumFmtBfr.CreateScFormats();
                            rXFBfr.CreateUserStyles();
                            rPTableMgr.ReadPivotCaches( maStrm );
                            rNameMgr.ConvertAllTokens();
                            eCurrent = EXC_STATE_BEFORE_SHEET;
                        }
                    break;
                    case 0x0E:  Precision(); break;     // PRECISION
                    case 0x22:  Rec1904(); break;       // 1904         [ 2345   ]
                    case 0x56:  break;                  // BUILTINFMTCNT[  34    ]
                    case 0x8D:  Hideobj(); break;       // HIDEOBJ      [  345   ]
                    case 0xD3:  SetHasBasic(); break;
                    case 0xDE:  Olesize(); break;

                    case EXC_ID_CODENAME:       ReadCodeName( aIn, true );          break;
                    case EXC_ID_USESELFS:       ReadUsesElfs();                     break;

                    case EXC_ID2_FONT:          rFontBfr.ReadFont( maStrm );        break;
                    case EXC_ID4_FORMAT:        rNumFmtBfr.ReadFormat( maStrm );    break;
                    case EXC_ID5_XF:            rXFBfr.ReadXF( maStrm );            break;
                    case EXC_ID_STYLE:          rXFBfr.ReadStyle( maStrm );         break;

                    case EXC_ID_SST:            rSst.ReadSst( maStrm );             break;
                    case EXC_ID_TABID:          rTabInfo.ReadTabid( maStrm );       break;
                    case EXC_ID_NAME:           rNameMgr.ReadName( maStrm );        break;

                    case EXC_ID_EXTERNSHEET:    rLinkMgr.ReadExternsheet( maStrm ); break;
                    case EXC_ID_SUPBOOK:        rLinkMgr.ReadSupbook( maStrm );     break;
                    case EXC_ID_XCT:            rLinkMgr.ReadXct( maStrm );         break;
                    case EXC_ID_CRN:            rLinkMgr.ReadCrn( maStrm );         break;
                    case EXC_ID_EXTERNNAME:     rLinkMgr.ReadExternname( maStrm, pFormConv.get() );  break;

                    case EXC_ID_MSODRAWINGGROUP:rObjMgr.ReadMsoDrawingGroup( maStrm ); break;

                    case EXC_ID_SXIDSTM:        rPTableMgr.ReadSxidstm( maStrm );   break;
                    case EXC_ID_SXVS:           rPTableMgr.ReadSxvs( maStrm );      break;
                    case EXC_ID_DCONREF:        rPTableMgr.ReadDconref( maStrm );   break;
                    case EXC_ID_DCONNAME:       rPTableMgr.ReadDConName( maStrm );  break;
                }

            }
            break;

            // prefetch for worksheet
            case EXC_STATE_SHEET_PRE:
            {
                switch( nRecId )
                {
                    // skip chart substream
                    case EXC_ID2_BOF:
                    case EXC_ID3_BOF:
                    case EXC_ID4_BOF:
                    case EXC_ID5_BOF:           XclTools::SkipSubStream( maStrm );      break;

                    case EXC_ID_WINDOW2:        rTabViewSett.ReadWindow2( maStrm, false );break;
                    case EXC_ID_SCL:            rTabViewSett.ReadScl( maStrm );         break;
                    case EXC_ID_PANE:           rTabViewSett.ReadPane( maStrm );        break;
                    case EXC_ID_SELECTION:      rTabViewSett.ReadSelection( maStrm );   break;

                    case EXC_ID2_DIMENSIONS:
                    case EXC_ID3_DIMENSIONS:    ReadDimensions();                       break;

                    case EXC_ID_CODENAME:       ReadCodeName( aIn, false ); bSheetHasCodeName = true; break;

                    case 0x0A:                          // EOF          [ 2345   ]
                    {
                        eCurrent = EXC_STATE_SHEET;
                        OUString sName;
                        GetDoc().GetName( GetCurrScTab(), sName );
                        if ( !bSheetHasCodeName )
                        {
                            nTabsWithNoCodeName.push_back( GetCurrScTab() );
                        }
                        else
                        {
                            OUString sCodeName;
                            GetDoc().GetCodeName( GetCurrScTab(), sCodeName );
                            aCodeNames.push_back( sCodeName );
                        }

                        bSheetHasCodeName = false; // reset

                        aIn.SeekGlobalPosition();         // and back to old position
                        break;
                    }
                    case 0x12:  SheetProtect(); break;
                    case 0x13:  SheetPassword(); break;
                    case 0x42:  Codepage(); break;      // CODEPAGE     [ 2345   ]
                    case 0x55:  DefColWidth(); break;
                    case 0x7D:  Colinfo(); break;       // COLINFO      [  345   ]
                    case 0x81:  Wsbool(); break;        // WSBOOL       [ 2345   ]
                    case 0x8C:  Country(); break;       // COUNTRY      [  345   ]
                    case 0x99:  Standardwidth(); break; // STANDARDWIDTH[   45   ]
                    case 0x9B:  FilterMode(); break;    // FILTERMODE
                    case EXC_ID_AUTOFILTERINFO: AutoFilterInfo(); break;// AUTOFILTERINFO
                    case EXC_ID_AUTOFILTER: AutoFilter(); break;    // AUTOFILTER
                    case 0x0208: Row34(); break;        // ROW          [  34    ]
                    case EXC_ID2_ARRAY:
                    case EXC_ID3_ARRAY: Array34(); break;      // ARRAY        [  34    ]
                    case 0x0225: Defrowheight345();break;//DEFAULTROWHEI[  345   ]
                    case 0x0867: FeatHdr(); break;      // FEATHDR
                    case 0x0868: Feat(); break;         // FEAT
                }
            }
            break;

            // worksheet
            case EXC_STATE_SHEET:
            {
                switch( nRecId )
                {
                    // skip unknown substreams
                    case EXC_ID2_BOF:
                    case EXC_ID3_BOF:
                    case EXC_ID4_BOF:
                    case EXC_ID5_BOF:           XclTools::SkipSubStream( maStrm );      break;

                    case EXC_ID_EOF:            Eof(); eCurrent = EXC_STATE_BEFORE_SHEET;   break;

                    case EXC_ID2_BLANK:
                    case EXC_ID3_BLANK:         ReadBlank();            break;
                    case EXC_ID2_INTEGER:       ReadInteger();          break;
                    case EXC_ID2_NUMBER:
                    case EXC_ID3_NUMBER:        ReadNumber();           break;
                    case EXC_ID2_LABEL:
                    case EXC_ID3_LABEL:         ReadLabel();            break;
                    case EXC_ID2_BOOLERR:
                    case EXC_ID3_BOOLERR:       ReadBoolErr();          break;
                    case EXC_ID_RK:             ReadRk();               break;

                    case EXC_ID2_FORMULA:
                    case EXC_ID3_FORMULA:
                    case EXC_ID4_FORMULA:       Formula25();            break;
                    case EXC_ID_SHRFMLA:        Shrfmla();              break;
                    case 0x000C:    Calccount();            break;  // CALCCOUNT
                    case 0x0010:    Delta();                break;  // DELTA
                    case 0x0011:    Iteration();            break;  // ITERATION
                    case 0x007E:
                    case 0x00AE:    Scenman();              break;  // SCENMAN
                    case 0x00AF:    Scenario();             break;  // SCENARIO
                    case 0x00BD:    Mulrk();                break;  // MULRK        [    5   ]
                    case 0x00BE:    Mulblank();             break;  // MULBLANK     [    5   ]
                    case 0x00D6:    Rstring();              break;  // RSTRING      [    5   ]
                    case 0x00E5:    Cellmerging();          break;  // CELLMERGING
                    case 0x00FD:    Labelsst();             break;  // LABELSST     [      8 ]
                    case 0x0236:    TableOp();              break;  // TABLE

                    case EXC_ID_HORPAGEBREAKS:
                    case EXC_ID_VERPAGEBREAKS:  rPageSett.ReadPageBreaks( maStrm );     break;
                    case EXC_ID_HEADER:
                    case EXC_ID_FOOTER:         rPageSett.ReadHeaderFooter( maStrm );   break;
                    case EXC_ID_LEFTMARGIN:
                    case EXC_ID_RIGHTMARGIN:
                    case EXC_ID_TOPMARGIN:
                    case EXC_ID_BOTTOMMARGIN:   rPageSett.ReadMargin( maStrm );         break;
                    case EXC_ID_PRINTHEADERS:   rPageSett.ReadPrintHeaders( maStrm );   break;
                    case EXC_ID_PRINTGRIDLINES: rPageSett.ReadPrintGridLines( maStrm ); break;
                    case EXC_ID_HCENTER:
                    case EXC_ID_VCENTER:        rPageSett.ReadCenter( maStrm );         break;
                    case EXC_ID_SETUP:          rPageSett.ReadSetup( maStrm );          break;
                    case EXC_ID8_IMGDATA:       rPageSett.ReadImgData( maStrm );        break;

                    case EXC_ID_MSODRAWING:     GetCurrSheetDrawing().ReadMsoDrawing( maStrm ); break;
                    // #i61786# weird documents: OBJ without MSODRAWING -> read in BIFF5 format
                    case EXC_ID_OBJ:            GetCurrSheetDrawing().ReadObj( maStrm ); break;
                    case EXC_ID_NOTE:           GetCurrSheetDrawing().ReadNote( maStrm ); break;

                    case EXC_ID_HLINK:          XclImpHyperlink::ReadHlink( maStrm );   break;
                    case EXC_ID_LABELRANGES:    XclImpLabelranges::ReadLabelranges( maStrm ); break;

                    case EXC_ID_CONDFMT:        rCondFmtMgr.ReadCondfmt( maStrm );      break;
                    case EXC_ID_CF:             rCondFmtMgr.ReadCF( maStrm );           break;

                    case EXC_ID_DVAL:           XclImpValidationManager::ReadDval( maStrm );  break;
                    case EXC_ID_DV:             rValidMgr.ReadDV( maStrm );             break;

                    case EXC_ID_QSI:            rWQBfr.ReadQsi( maStrm );               break;
                    case EXC_ID_WQSTRING:       rWQBfr.ReadWqstring( maStrm );          break;
                    case EXC_ID_PQRY:           rWQBfr.ReadParamqry( maStrm );          break;
                    case EXC_ID_WQSETT:         rWQBfr.ReadWqsettings( maStrm );        break;
                    case EXC_ID_WQTABLES:       rWQBfr.ReadWqtables( maStrm );          break;

                    case EXC_ID_SXVIEW:         rPTableMgr.ReadSxview( maStrm );    break;
                    case EXC_ID_SXVD:           rPTableMgr.ReadSxvd( maStrm );      break;
                    case EXC_ID_SXVI:           rPTableMgr.ReadSxvi( maStrm );      break;
                    case EXC_ID_SXIVD:          rPTableMgr.ReadSxivd( maStrm );     break;
                    case EXC_ID_SXPI:           rPTableMgr.ReadSxpi( maStrm );      break;
                    case EXC_ID_SXDI:           rPTableMgr.ReadSxdi( maStrm );      break;
                    case EXC_ID_SXVDEX:         rPTableMgr.ReadSxvdex( maStrm );    break;
                    case EXC_ID_SXEX:           rPTableMgr.ReadSxex( maStrm );      break;
                    case EXC_ID_SHEETEXT:       rTabViewSett.ReadTabBgColor( maStrm, rPal );    break;
                    case EXC_ID_SXVIEWEX9:      rPTableMgr.ReadSxViewEx9( maStrm ); break;
                    case EXC_ID_SXADDL:         rPTableMgr.ReadSxAddl( maStrm ); break;
                }
            }
            break;

            default:;
        }
    }

    if( eLastErr == ERRCODE_NONE )
    {
        // In some strange circumstances the codename might be missing
        // # Create any missing Sheet CodeNames
        for ( const auto& rTab : nTabsWithNoCodeName )
        {
            SCTAB nTab = 1;
            while ( true )
            {
                OUString sTmpName = "Sheet" + OUString::number(static_cast<sal_Int32>(nTab++));

                if ( std::find(aCodeNames.begin(), aCodeNames.end(), sTmpName) == aCodeNames.end() ) // generated codename not found
                {
                    // Set new codename
                    GetDoc().SetCodeName( rTab, sTmpName );
                    // Record newly used codename
                    aCodeNames.push_back(sTmpName);
                    break;
                }
            }
        }
        // #i45843# Convert pivot tables before calculation, so they are available
        // for the GETPIVOTDATA function.
        if( GetBiff() == EXC_BIFF8 )
            GetPivotTableManager().ConvertPivotTables();

        ScDocumentImport& rDoc = GetDocImport();
        rDoc.finalize();
        pProgress.reset();
#if 0
        // Excel documents look much better without this call; better in the
        // sense that the row heights are identical to the original heights in
        // Excel.
        if ( !rD.IsAdjustHeightLocked())
            AdjustRowHeight();
#endif
        PostDocLoad();

        rD.CalcAfterLoad(false);

        // import change tracking data
        XclImpChangeTrack aImpChTr( GetRoot(), maStrm );
        aImpChTr.Apply();

        const XclImpAddressConverter& rAddrConv = GetAddressConverter();
        if( rAddrConv.IsTabTruncated() )
            eLastErr = SCWARN_IMPORT_SHEET_OVERFLOW;
        else if( bTabTruncated || rAddrConv.IsRowTruncated() )
            eLastErr = SCWARN_IMPORT_ROW_OVERFLOW;
        else if( rAddrConv.IsColTruncated() )
            eLastErr = SCWARN_IMPORT_COLUMN_OVERFLOW;

        if( GetBiff() == EXC_BIFF8 )
            GetPivotTableManager().MaybeRefreshPivotTables();
    }

    return eLastErr;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
