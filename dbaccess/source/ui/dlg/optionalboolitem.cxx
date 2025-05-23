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

#include "optionalboolitem.hxx"

namespace dbaui
{

    // OptionalBoolItem
    OptionalBoolItem::OptionalBoolItem( sal_uInt16 _nWhich )
        :SfxPoolItem( _nWhich )
    {
    }

    bool OptionalBoolItem::operator==( const SfxPoolItem& _rItem ) const
    {
        return SfxPoolItem::operator==(_rItem) &&
            static_cast<const OptionalBoolItem&>( _rItem  ).m_aValue == m_aValue;
    }

    OptionalBoolItem* OptionalBoolItem::Clone( SfxItemPool* /*_pPool*/ ) const
    {
        return new OptionalBoolItem( *this );
    }

} // namespace dbaui

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
