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

#pragma once

#include <osx/a11ywrapper.h>

enum class SalEvent;

@interface SalFrameWindow : NSWindow<NSWindowDelegate>
{
    AquaSalFrame*       mpFrame;
    id mDraggingDestinationHandler;
    BOOL                mbInWindowDidResize;
    NSTimer*            mpLiveResizeTimer;
    NSTimer*            mpResetParentWindowTimer;
    BOOL                mbInSetFrame;
}
-(id)initWithSalFrame: (AquaSalFrame*)pFrame;
-(void)clearLiveResizeTimer;
-(void)clearResetParentWindowTimer;
-(void)dealloc;
-(BOOL)canBecomeKeyWindow;
-(void)displayIfNeeded;
-(void)windowDidBecomeKey: (NSNotification*)pNotification;
-(void)windowDidResignKey: (NSNotification*)pNotification;
-(void)windowDidChangeScreen: (NSNotification*)pNotification;
-(void)windowDidMove: (NSNotification*)pNotification;
-(void)windowDidResize: (NSNotification*)pNotification;
-(void)windowDidMiniaturize: (NSNotification*)pNotification;
-(void)windowDidDeminiaturize: (NSNotification*)pNotification;
-(BOOL)windowShouldClose: (NSNotification*)pNotification;
-(void)windowDidChangeBackingProperties:(NSNotification *)pNotification;
-(void)windowWillStartLiveResize:(NSNotification *)pNotification;
-(void)windowDidEndLiveResize:(NSNotification *)pNotification;
//-(void)willEncodeRestorableState:(NSCoder*)pCoderState;
//-(void)didDecodeRestorableState:(NSCoder*)pCoderState;
//-(void)windowWillEnterVersionBrowser:(NSNotification*)pNotification;
-(void)dockMenuItemTriggered: (id)sender;
-(AquaSalFrame*)getSalFrame;
-(BOOL)containsMouse;
-(css::uno::Reference < css::accessibility::XAccessibleContext >)accessibleContext;

/* NSDraggingDestination protocol methods
 */
-(NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
-(NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender;
-(void)draggingExited:(id <NSDraggingInfo>)sender;
-(BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender;
-(BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
-(void)concludeDragOperation:(id <NSDraggingInfo>)sender;

-(void)registerDraggingDestinationHandler:(id)theHandler;
-(void)unregisterDraggingDestinationHandler:(id)theHandler;

-(void)endExtTextInput;
-(void)endExtTextInput:(EndExtTextInputFlags)nFlags;

-(void)windowDidResizeWithTimer:(NSTimer *)pTimer;

-(BOOL)isIgnoredWindow;

// NSAccessibilityElement overrides
-(id)accessibilityApplicationFocusedUIElement;
-(id)accessibilityFocusedUIElement;
-(BOOL)accessibilityIsIgnored;
-(BOOL)isAccessibilityElement;

-(void)resetParentWindow;

@end

@interface SalFrameView : NSView <NSTextInputClient>
{
    AquaSalFrame*       mpFrame;
    AquaA11yWrapper*    mpChildWrapper;
    BOOL                mbNeedChildWrapper;

    // for NSTextInput/NSTextInputClient
    NSEvent*        mpLastEvent;
    BOOL            mbNeedSpecialKeyHandle;
    BOOL            mbInKeyInput;
    BOOL            mbKeyHandled;
    NSRange         mMarkedRange;
    NSRange         mSelectedRange;
    id              mpMouseEventListener;
    id              mDraggingDestinationHandler;
    NSEvent*        mpLastSuperEvent;

    // #i102807# used by magnify event handler
    NSTimeInterval  mfLastMagnifyTime;
    float           mfMagnifyDeltaSum;

    BOOL            mbInEndExtTextInput;
    BOOL            mbInCommitMarkedText;
    NSAttributedString* mpLastMarkedText;
    BOOL            mbTextInputWantsNonRepeatKeyDown;
    NSTrackingArea* mpLastTrackingArea;

    BOOL            mbInViewDidChangeEffectiveAppearance;
}
+(void)unsetMouseFrame: (AquaSalFrame*)pFrame;
-(id)initWithSalFrame: (AquaSalFrame*)pFrame;
-(void)dealloc;
-(AquaSalFrame*)getSalFrame;
-(BOOL)acceptsFirstResponder;
-(BOOL)acceptsFirstMouse: (NSEvent *)pEvent;
-(BOOL)isOpaque;
-(void)drawRect: (NSRect)aRect;
-(void)mouseDown: (NSEvent*)pEvent;
-(void)mouseDragged: (NSEvent*)pEvent;
-(void)mouseUp: (NSEvent*)pEvent;
-(void)mouseMoved: (NSEvent*)pEvent;
-(void)mouseEntered: (NSEvent*)pEvent;
-(void)mouseExited: (NSEvent*)pEvent;
-(void)rightMouseDown: (NSEvent*)pEvent;
-(void)rightMouseDragged: (NSEvent*)pEvent;
-(void)rightMouseUp: (NSEvent*)pEvent;
-(void)otherMouseDown: (NSEvent*)pEvent;
-(void)otherMouseDragged: (NSEvent*)pEvent;
-(void)otherMouseUp: (NSEvent*)pEvent;
-(void)scrollWheel: (NSEvent*)pEvent;
-(void)magnifyWithEvent: (NSEvent*)pEvent;
-(void)rotateWithEvent: (NSEvent*)pEvent;
-(void)swipeWithEvent: (NSEvent*)pEvent;
-(void)keyDown: (NSEvent*)pEvent;
-(void)flagsChanged: (NSEvent*)pEvent;
-(void)sendMouseEventToFrame:(NSEvent*)pEvent button:(sal_uInt16)nButton eventtype:(SalEvent)nEvent;
-(BOOL)sendKeyInputAndReleaseToFrame: (sal_uInt16)nKeyCode character: (sal_Unicode)aChar;
-(BOOL)sendKeyInputAndReleaseToFrame: (sal_uInt16)nKeyCode character: (sal_Unicode)aChar modifiers: (unsigned int)nMod;
-(BOOL)sendKeyToFrameDirect: (sal_uInt16)nKeyCode character: (sal_Unicode)aChar modifiers: (unsigned int)nMod;
-(BOOL)sendSingleCharacter:(NSEvent*)pEvent;
-(BOOL)handleKeyDownException:(NSEvent*)pEvent;
-(void)clearLastEvent;
-(void)clearLastMarkedText;
-(void)clearLastTrackingArea;
-(void)updateTrackingAreas;
/*
    text action methods
*/
-(void)insertText:(id)aString replacementRange:(NSRange)replacementRange;
-(void)insertTab: (id)aSender;
-(void)insertBacktab: (id)aSender;
-(void)moveLeft: (id)aSender;
-(void)moveLeftAndModifySelection: (id)aSender;
-(void)moveBackwardAndModifySelection: (id)aSender;
-(void)moveRight: (id)aSender;
-(void)moveRightAndModifySelection: (id)aSender;
-(void)moveForwardAndModifySelection: (id)aSender;
-(void)moveUp: (id)aSender;
-(void)moveDown: (id)aSender;
-(void)moveWordBackward: (id)aSender;
-(void)moveWordBackwardAndModifySelection: (id)aSender;
-(void)moveWordLeftAndModifySelection: (id)aSender;
-(void)moveWordForward: (id)aSender;
-(void)moveWordForwardAndModifySelection: (id)aSender;
-(void)moveWordRightAndModifySelection: (id)aSender;
-(void)moveToEndOfLine: (id)aSender;
-(void)moveToRightEndOfLine: (id)aSender;
-(void)moveToLeftEndOfLine: (id)aSender;
-(void)moveToEndOfLineAndModifySelection: (id)aSender;
-(void)moveToRightEndOfLineAndModifySelection: (id)aSender;
-(void)moveToLeftEndOfLineAndModifySelection: (id)aSender;
-(void)moveToBeginningOfLine: (id)aSender;
-(void)moveToBeginningOfLineAndModifySelection: (id)aSender;
-(void)moveToEndOfParagraph: (id)aSender;
-(void)moveToEndOfParagraphAndModifySelection: (id)aSender;
-(void)moveToBeginningOfParagraph: (id)aSender;
-(void)moveToBeginningOfParagraphAndModifySelection: (id)aSender;
-(void)moveParagraphForward: (id)aSender;
-(void)moveParagraphForwardAndModifySelection: (id)aSender;
-(void)moveParagraphBackward: (id)aSender;
-(void)moveParagraphBackwardAndModifySelection: (id)aSender;
-(void)moveToEndOfDocument: (id)aSender;
-(void)scrollToEndOfDocument: (id)aSender;
-(void)moveToEndOfDocumentAndModifySelection: (id)aSender;
-(void)moveToBeginningOfDocument: (id)aSender;
-(void)scrollToBeginningOfDocument: (id)aSender;
-(void)moveToBeginningOfDocumentAndModifySelection: (id)aSender;
-(void)insertNewline: (id)aSender;
-(void)deleteBackward: (id)aSender;
-(void)deleteForward: (id)aSender;
-(void)cancelOperation: (id)aSender;
-(void)deleteBackwardByDecomposingPreviousCharacter: (id)aSender;
-(void)deleteWordBackward: (id)aSender;
-(void)deleteWordForward: (id)aSender;
-(void)deleteToBeginningOfLine: (id)aSender;
-(void)deleteToEndOfLine: (id)aSender;
-(void)deleteToBeginningOfParagraph: (id)aSender;
-(void)deleteToEndOfParagraph: (id)aSender;
-(void)insertLineBreak: (id)aSender;
-(void)insertParagraphSeparator: (id)aSender;
-(void)selectWord: (id)aSender;
-(void)selectLine: (id)aSender;
-(void)selectParagraph: (id)aSender;
-(void)selectAll: (id)aSender;
-(void)noop: (id)aSender;
/* set the correct pointer for our view */
-(void)resetCursorRects;
-(css::accessibility::XAccessibleContext *)accessibleContext;
-(id)parentAttribute;
-(NSWindow*)windowForParent;
/*
  Event hook for D&D service.

  A drag operation will be invoked on a NSView using
  the method 'dragImage'. This method requires the
  actual mouse event initiating this drag operation.
  Mouse events can only be received by subclassing
  NSView and overriding methods like 'mouseDown' etc.
  hence we implement an event hook here so that the
  D&D service can register as listener for mouse
  messages and use the last 'mouseDown' or
  'mouseDragged' message to initiate the drag
  operation.
*/
-(void)registerMouseEventListener: (id)theListener;
-(void)unregisterMouseEventListener: (id)theListener;

/* NSDraggingDestination protocol methods
 */
-(NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
-(NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender;
-(void)draggingExited:(id <NSDraggingInfo>)sender;
-(BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender;
-(BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
-(void)concludeDragOperation:(id <NSDraggingInfo>)sender;

-(void)registerDraggingDestinationHandler:(id)theHandler;
-(void)unregisterDraggingDestinationHandler:(id)theHandler;

-(void)endExtTextInput;
-(void)endExtTextInput:(EndExtTextInputFlags)nFlags;
-(void)deleteTextInputWantsNonRepeatKeyDown;

-(void)insertRegisteredWrapperIntoWrapperRepository;
-(void)registerWrapper;
-(void)revokeWrapper;

// NSAccessibilityElement overrides
-(id)accessibilityAttributeValue:(NSString *)pAttribute;
-(BOOL)accessibilityIsIgnored;
-(NSArray *)accessibilityAttributeNames;
-(BOOL)accessibilityIsAttributeSettable:(NSString *)pAttribute;
-(NSArray *)accessibilityParameterizedAttributeNames;
-(BOOL)accessibilitySetOverrideValue:(id)pValue forAttribute:(NSString *)pAttribute;
-(void)accessibilitySetValue:(id)pValue forAttribute:(NSString *)pAttribute;
-(id)accessibilityAttributeValue:(NSString *)pAttribute forParameter:(id)pParameter;
-(id)accessibilityFocusedUIElement;
-(NSString *)accessibilityActionDescription:(NSString *)pAction;
-(void)accessibilityPerformAction:(NSString *)pAction;
-(NSArray *)accessibilityActionNames;
-(id)accessibilityHitTest:(NSPoint)aPoint;
-(NSArray *)accessibilityVisibleChildren;
-(NSArray *)accessibilitySelectedChildren;
-(NSArray *)accessibilityChildren;
-(NSArray <id<NSAccessibilityElement>> *)accessibilityChildrenInNavigationOrder;

-(void)viewDidChangeEffectiveAppearance;

@end

@interface SalFrameViewA11yWrapper : AquaA11yWrapper
{
    SalFrameView*       mpParentView;
}
-(id)initWithParent:(SalFrameView*)pParentView accessibleContext:(::com::sun::star::uno::Reference<::com::sun::star::accessibility::XAccessibleContext>&)rxAccessibleContext;
-(void)dealloc;
@end

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
