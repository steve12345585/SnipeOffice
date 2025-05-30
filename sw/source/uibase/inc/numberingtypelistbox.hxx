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
#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_NUMBERINGTYPELISTBOX_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_NUMBERINGTYPELISTBOX_HXX

#include <memory>
#include <vcl/weld.hxx>
#include <swdllapi.h>
#include <o3tl/typed_flags_set.hxx>
#include <editeng/svxenum.hxx>

enum class SwInsertNumTypes
{
    NoNumbering              = 0x01,
    Extended                 = 0x02
};

namespace o3tl {
   template<> struct typed_flags<SwInsertNumTypes> : is_typed_flags<SwInsertNumTypes, 0x03> {};
};

struct SwNumberingTypeListBox_Impl;

class SW_DLLPUBLIC SwNumberingTypeListBox
{
    std::unique_ptr<weld::ComboBox> m_xWidget;
    std::unique_ptr<SwNumberingTypeListBox_Impl> m_xImpl;

public:
    SwNumberingTypeListBox(std::unique_ptr<weld::ComboBox> pWidget);
    ~SwNumberingTypeListBox();

    void connect_changed(const Link<weld::ComboBox&, void>& rLink) { m_xWidget->connect_changed(rLink); }

    void          Reload(SwInsertNumTypes nTypeFlags);
    SvxNumType    GetSelectedNumberingType() const;
    bool          SelectNumberingType(SvxNumType nType);
    void          SetNoSelection() { m_xWidget->set_active(-1); }
    void          set_sensitive(bool bEnable) { m_xWidget->set_sensitive(bEnable); }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
