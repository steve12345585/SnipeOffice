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

#include "DataFlavorMapping.hxx"
#include <cppuhelper/compbase.hxx>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/datatransfer/dnd/XDropTarget.hpp>

#include <com/sun/star/datatransfer/dnd/XDropTargetListener.hpp>
#include <com/sun/star/datatransfer/dnd/DropTargetDragEnterEvent.hpp>
#include <com/sun/star/datatransfer/dnd/XDropTargetDragContext.hpp>
#include <com/sun/star/datatransfer/dnd/XDropTargetDropContext.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <cppuhelper/basemutex.hxx>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>

#include <premac.h>
#import <Cocoa/Cocoa.h>
#include <postmac.h>

class DropTarget;
class AquaSalFrame;

/* The functions declared in this protocol are actually
   declared in vcl/inc/osx/salframe.h. Because we want
   to avoid importing VCL headers in UNO services and
   on the other hand want to avoid warnings caused by
   gcc complaining about unknowness of these functions
   we declare them in a protocol here and cast at the
   appropriate places.
*/
@protocol DraggingDestinationHandler
-(void)registerDraggingDestinationHandler:(id)theHandler;
-(void)unregisterDraggingDestinationHandler:(id)theHandler;
@end

@interface DropTargetHelper : NSObject
{
  DropTarget* mDropTarget;
}

-(DropTargetHelper*)initWithDropTarget:(DropTarget*)pdt;

-(NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender;
-(NSDragOperation)draggingUpdated:(id <NSDraggingInfo>)sender;
-(void)draggingExited:(id <NSDraggingInfo>)sender;
-(BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender;
-(BOOL)performDragOperation:(id <NSDraggingInfo>)sender;
-(void)concludeDragOperation:(id <NSDraggingInfo>)sender;

@end

class DropTarget: public cppu::BaseMutex,
                  public cppu::WeakComponentImplHelper< css::lang::XInitialization,
                                                         css::datatransfer::dnd::XDropTarget,
                                                         css::datatransfer::dnd::XDropTargetDragContext,
                                                         css::datatransfer::dnd::XDropTargetDropContext,
                                                         css::lang::XServiceInfo >
{
public:
  DropTarget();
  virtual ~DropTarget() override;
  DropTarget(const DropTarget&) = delete;
  DropTarget& operator=(const DropTarget&) = delete;

  // Overrides WeakComponentImplHelper::disposing which is called by
  // WeakComponentImplHelper::dispose
  // Must be called.
  virtual void SAL_CALL disposing() override;

  // XInitialization
  virtual void SAL_CALL initialize( const css::uno::Sequence< css::uno::Any >& aArguments ) override;

  // XDropTarget
  virtual void SAL_CALL addDropTargetListener( const css::uno::Reference< css::datatransfer::dnd::XDropTargetListener >& dtl ) override;

  virtual void SAL_CALL removeDropTargetListener( const css::uno::Reference<  css::datatransfer::dnd::XDropTargetListener >& dtl ) override;

  // Default is not active
  virtual sal_Bool SAL_CALL isActive() override;
  virtual void SAL_CALL setActive(sal_Bool isActive) override;
  virtual sal_Int8 SAL_CALL getDefaultActions() override;
  virtual void SAL_CALL setDefaultActions(sal_Int8 actions) override;

  // XDropTargetDragContext
  virtual void SAL_CALL acceptDrag(sal_Int8 dragOperation) override;
  virtual void SAL_CALL rejectDrag() override;

  // XDropTargetDragContext
  virtual void SAL_CALL acceptDrop(sal_Int8 dropOperation) override;
  virtual void SAL_CALL rejectDrop() override;
  virtual void SAL_CALL dropComplete(sal_Bool success) override;

  // XServiceInfo
  virtual OUString SAL_CALL getImplementationName() override;
  virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
  virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames() override;

  // NSDraggingDestination protocol functions
  NSDragOperation draggingEntered(id sender);
  NSDragOperation draggingUpdated(id sender);
  void draggingExited(id sender);
  static BOOL prepareForDragOperation();
  BOOL performDragOperation();
  void concludeDragOperation(id sender);

  /* If multiple actions are supported by the drag source and
     the user did not choose a specific action by pressing a
     modifier key choose a default action to be proposed to
     the application.
  */
  sal_Int8 determineDropAction(sal_Int8 dropActions, id sender) const;

private:
  void fire_drop(const css::datatransfer::dnd::DropTargetDropEvent& dte);
  void fire_dragEnter(const css::datatransfer::dnd::DropTargetDragEnterEvent& dtdee);
  void fire_dragExit(const css::datatransfer::dnd::DropTargetEvent& dte);
  void fire_dragOver(const css::datatransfer::dnd::DropTargetDragEvent& dtde);
  void fire_dropActionChanged(const css::datatransfer::dnd::DropTargetDragEvent& dtde);

private:
  css::uno::Reference< css::datatransfer::dnd::XDropTargetDragContext > mXCurrentDragContext;
  css::uno::Reference< css::datatransfer::dnd::XDropTargetDropContext > mXCurrentDropContext;
  css::uno::Reference< css::datatransfer::clipboard::XClipboard > mXCurrentDragClipboard;
  DataFlavorMapperPtr_t mDataFlavorMapper;
  id  mView;
  AquaSalFrame* mpFrame;
  DropTargetHelper* mDropTargetHelper;
  bool mbActive;
  sal_Int8 mDragSourceSupportedActions;
  sal_Int8 mSelectedDropAction;
  sal_Int8 mDefaultActions;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
