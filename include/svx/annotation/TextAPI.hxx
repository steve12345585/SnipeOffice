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

#include <editeng/unotext.hxx>
#include <rtl/ref.hxx>
#include <editeng/outliner.hxx>
#include <svx/svxdllapi.h>

class SdrModel;

namespace sdr::annotation
{
class TextAPIEditSource;

class SVXCORE_DLLPUBLIC TextApiObject final : public SvxUnoText
{
public:
    static rtl::Reference<TextApiObject> create(SdrModel* pModel);

    virtual ~TextApiObject() noexcept override;

    /// @throws css::uno::RuntimeException
    void dispose();

    std::optional<OutlinerParaObject> CreateText();
    void SetText(OutlinerParaObject const& rText);
    OUString GetText() const;

    static TextApiObject* getImplementation(const css::uno::Reference<css::text::XText>&);

private:
    std::unique_ptr<TextAPIEditSource> mpSource;
    TextApiObject(std::unique_ptr<TextAPIEditSource> pEditSource);
};

} // namespace sdr::annotation

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
