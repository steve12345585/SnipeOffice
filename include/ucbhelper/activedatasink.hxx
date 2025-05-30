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

#ifndef INCLUDED_UCBHELPER_ACTIVEDATASINK_HXX
#define INCLUDED_UCBHELPER_ACTIVEDATASINK_HXX

#include <config_options.h>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <ucbhelper/ucbhelperdllapi.h>
#include <cppuhelper/implbase.hxx>

namespace ucbhelper
{
// workaround for incremental linking bugs in MSVC2015
class SAL_DLLPUBLIC_TEMPLATE ActiveDataSink_Base
    : public cppu::WeakImplHelper<css::io::XActiveDataSink>
{
};

/**
  * This class implements the interface css::io::XActiveDataSink.
  * Instances of this class can be passed with the parameters of an
  * "open" command.
  */
class UNLESS_MERGELIBS(UCBHELPER_DLLPUBLIC) ActiveDataSink final : public ActiveDataSink_Base
{
    css::uno::Reference<css::io::XInputStream> m_xStream;

public:
    // XActiveDataSink methods.
    virtual void SAL_CALL
    setInputStream(const css::uno::Reference<css::io::XInputStream>& aStream) override;

    virtual css::uno::Reference<css::io::XInputStream> SAL_CALL getInputStream() override;
};

} /* namespace ucbhelper */

#endif /* ! INCLUDED_UCBHELPER_ACTIVEDATASINK_HXX */

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
