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

#include <TableConnectionData.hxx>
#include <TableFieldDescription.hxx>
#include <QEnumTypes.hxx>

namespace dbaui
{
    class OQueryTableConnectionData final : public OTableConnectionData
    {
        sal_Int32       m_nFromEntryIndex;
        sal_Int32       m_nDestEntryIndex;
        EJoinType       m_eJoinType;
        bool            m_bNatural;

        OQueryTableConnectionData& operator=( const OQueryTableConnectionData& rConnData );
    public:
        OQueryTableConnectionData();
        OQueryTableConnectionData( const OQueryTableConnectionData& rConnData );
        OQueryTableConnectionData( const TTableWindowData::value_type& _pReferencingTable,const TTableWindowData::value_type& _pReferencedTable );
        virtual ~OQueryTableConnectionData() override;

        virtual void CopyFrom(const OTableConnectionData& rSource) override;
        virtual std::shared_ptr<OTableConnectionData> NewInstance() const override;


        /** Update create a new connection

            @return true if successful
        */
        virtual bool Update() override;

        OUString const & GetAliasName(EConnectionSide nWhich) const;

        sal_Int32       GetFieldIndex(EConnectionSide nWhich) const { return nWhich==JTCS_TO ? m_nDestEntryIndex : m_nFromEntryIndex; }
        void            SetFieldIndex(EConnectionSide nWhich, sal_Int32 nVal) { if (nWhich==JTCS_TO) m_nDestEntryIndex=nVal; else m_nFromEntryIndex=nVal; }

        void            InitFromDrag(const OTableFieldDescRef& rDragLeft, const OTableFieldDescRef& rDragRight);

        EJoinType       GetJoinType() const { return m_eJoinType; };
        void            SetJoinType(const EJoinType& eJT) { m_eJoinType = eJT; };

        void setNatural(bool _bNatural) { m_bNatural = _bNatural; }
        bool isNatural() const { return m_bNatural; }
    };

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
