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


#include <vcl/svapp.hxx>
#include <osx/salinst.h>

#include "a11ywrapperrow.h"
#include "a11ytextwrapper.h"

// Wrapper for AXRow role

@implementation AquaA11yWrapperRow : AquaA11yWrapper

-(id)disclosingAttribute {
    // TODO: implement
    return nil;
}

-(NSArray *)accessibilityAttributeNames {
    // Related: tdf#148453 Acquire solar mutex during native accessibility calls
    SolarMutexGuard aGuard;
    if ( mIsDisposed )
        return [ NSArray array ];

    // Default Attributes
    NSMutableArray * attributeNames = [ NSMutableArray arrayWithArray: [ super accessibilityAttributeNames ] ];
    // Special Attributes and removing unwanted attributes depending on role
    [ attributeNames removeObjectsInArray: [ AquaA11yTextWrapper specialAttributeNames ] ];
    [ attributeNames removeObject: NSAccessibilityTitleAttribute ];
    [ attributeNames removeObject: NSAccessibilityEnabledAttribute ];
    [ attributeNames removeObject: NSAccessibilityFocusedAttribute ];
    [ attributeNames addObject: NSAccessibilitySelectedAttribute ];
    [ attributeNames addObject: NSAccessibilityDisclosingAttribute ];
    return attributeNames;
}

@end

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
