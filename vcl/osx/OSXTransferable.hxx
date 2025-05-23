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

#include <com/sun/star/datatransfer/XTransferable.hpp>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/datatransfer/XMimeContentTypeFactory.hpp>
#include <com/sun/star/datatransfer/XMimeContentType.hpp>

#include "DataFlavorMapping.hxx"

#include <premac.h>
#import <Cocoa/Cocoa.h>
#include <postmac.h>

#include <memory>
#include <vector>

class OSXTransferable : public ::cppu::WeakImplHelper<css::datatransfer::XTransferable>
{
public:
  explicit OSXTransferable(css::uno::Reference< css::datatransfer::XMimeContentTypeFactory> const & rXMimeCntFactory,
                           DataFlavorMapperPtr_t pDataFlavorMapper,
                           NSPasteboard* pasteboard);

  virtual ~OSXTransferable() override;
  OSXTransferable(const OSXTransferable&) = delete;
  OSXTransferable& operator=(const OSXTransferable&) = delete;

  // XTransferable

  virtual css::uno::Any SAL_CALL getTransferData( const css::datatransfer::DataFlavor& aFlavor ) override;

  virtual css::uno::Sequence< css::datatransfer::DataFlavor > SAL_CALL getTransferDataFlavors(  ) override;

  virtual sal_Bool SAL_CALL isDataFlavorSupported( const css::datatransfer::DataFlavor& aFlavor ) override;

  // Helper functions not part of the XTransferable interface

  void initClipboardItemList();

  //css::uno::Any getClipboardItemData(ClipboardItemPtr_t clipboardItem);

  bool compareDataFlavors( const css::datatransfer::DataFlavor& lhs,
                           const css::datatransfer::DataFlavor& rhs );

private:
  css::uno::Sequence< css::datatransfer::DataFlavor > mFlavorList;
  css::uno::Reference< css::datatransfer::XMimeContentTypeFactory> mrXMimeCntFactory;
  DataFlavorMapperPtr_t mDataFlavorMapper;
  NSPasteboard* mPasteboard;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
