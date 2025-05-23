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

#include <sal/types.h>
#include <svl/poolitem.hxx>

namespace frm
{

    /// the id of an attribute
    typedef sal_Int32   AttributeId;
    /// the "which id" of an item in an SfxItemSet
    typedef sal_uInt16      WhichId;
    /// a SFX slot id
    typedef sal_uInt16      SfxSlotId;

    enum AttributeCheckState
    {
        eChecked,
        eUnchecked,
        eIndetermined
    };

    struct AttributeState
    {
    private:
        std::unique_ptr<SfxPoolItem>  pItemHandleItem;

    public:
        AttributeCheckState eSimpleState;

        inline          AttributeState( );
        inline explicit AttributeState( AttributeCheckState _eCheckState );
        inline          AttributeState( const AttributeState& _rSource );

        inline AttributeState& operator=( const AttributeState& _rSource );

        inline bool operator==( const AttributeState& _rRHS );

        inline const SfxPoolItem* getItem() const;
        inline void setItem( const SfxPoolItem* _pItem );
    };

    inline AttributeState::AttributeState( )
        :eSimpleState( eIndetermined )
    {
    }

    inline AttributeState::AttributeState( AttributeCheckState _eCheckState )
        :eSimpleState( _eCheckState )
    {
    }

    inline AttributeState::AttributeState( const AttributeState& _rSource )
        :eSimpleState( eIndetermined )
    {
        operator=( _rSource );
    }

    inline AttributeState& AttributeState::operator=( const AttributeState& _rSource )
    {
        if ( &_rSource == this )
            return *this;

        eSimpleState = _rSource.eSimpleState;
        setItem( _rSource.getItem() );
        return *this;
    }

    inline const SfxPoolItem* AttributeState::getItem() const
    {
        return pItemHandleItem.get();
    }

    inline void AttributeState::setItem( const SfxPoolItem* _pItem )
    {
        if ( _pItem )
            pItemHandleItem.reset(_pItem->Clone());
        else
            pItemHandleItem.reset();
    }

    inline bool AttributeState::operator==( const AttributeState& _rRHS )
    {
        if ( eSimpleState != _rRHS.eSimpleState )
            return false;

        if ( pItemHandleItem && !_rRHS.pItemHandleItem )
            return false;

        if ( !pItemHandleItem && _rRHS.pItemHandleItem )
            return false;

        if ( pItemHandleItem == _rRHS.pItemHandleItem )
            return true;

        assert(pItemHandleItem && _rRHS.pItemHandleItem && "otherwise have already returned");

        return *pItemHandleItem == *_rRHS.pItemHandleItem;
    }

    class IMultiAttributeDispatcher
    {
    public:
        virtual AttributeState  getState( AttributeId _nAttributeId ) const = 0;
        virtual void            executeAttribute( AttributeId _nAttributeId, const SfxPoolItem* _pArgument ) = 0;

    protected:
        ~IMultiAttributeDispatcher() {}
    };

} // namespace frm

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
