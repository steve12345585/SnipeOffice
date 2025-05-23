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


#include <vcl/print.hxx>

#include <osx/printview.h>
#include <osx/salprn.h>

@implementation AquaPrintView

-(id)initWithController: (vcl::PrinterController*)pController
        withInfoPrinter: (AquaSalInfoPrinter*)pInfoPrinter
{
    NSRect aRect = { NSZeroPoint, [pInfoPrinter->getPrintInfo() paperSize] };
    if( (self = [super initWithFrame: aRect]) != nil )
    {
        mpController = pController;
        mpInfoPrinter = pInfoPrinter;
    }
    return self;
}

-(BOOL)knowsPageRange: (NSRangePointer)range
{
    range->location = 1;
    range->length = mpInfoPrinter->getCurPageRangeCount();
    return YES;
}

-(NSRect)rectForPage: (int)page
{
    NSSize aPaperSize =  [mpInfoPrinter->getPrintInfo() paperSize];
    int nWidth = static_cast<int>(aPaperSize.width);
    // #i101108# sanity check
    if( nWidth < 1 )
        nWidth = 1;
    NSRect aRect = { { static_cast<CGFloat>(page % nWidth),
                       static_cast<CGFloat>(page / nWidth) },
                     aPaperSize };
    return aRect;
}

-(NSPoint)locationOfPrintRect: (NSRect)aRect
{
    (void)aRect;
    return NSZeroPoint;
}

-(void)drawRect: (NSRect)rect
{
    mpInfoPrinter->setStartPageOffset( static_cast<int>(rect.origin.x),
                                       static_cast<int>(rect.origin.y) );
    NSSize aPaperSize =  [mpInfoPrinter->getPrintInfo() paperSize];
    int nPage = static_cast<int>(aPaperSize.width * rect.origin.y + rect.origin.x);
    
    // Notes:
    // - Page count is 1 based
    // - Print jobs with different paper sizes are broken up into a separate
    //   NSPrintOperation for each set of contiguous pages with the same size
    //   so the page number needs to be offset by the current page start range
    if( nPage - 1 < (mpInfoPrinter->getCurPageRangeStart() + mpInfoPrinter->getCurPageRangeCount() ) )
        mpController->printFilteredPage( mpInfoPrinter->getCurPageRangeStart() + nPage - 1 );
}

@end

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
