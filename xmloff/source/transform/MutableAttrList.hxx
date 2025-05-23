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

#include <com/sun/star/util/XCloneable.hpp>
#include <com/sun/star/xml/sax/XAttributeList.hpp>

#include <cppuhelper/implbase.hxx>
#include <rtl/ref.hxx>

namespace comphelper { class AttributeList; }

class XMLMutableAttributeList : public ::cppu::WeakImplHelper<
        css::xml::sax::XAttributeList,
        css::util::XCloneable>
{
    css::uno::Reference< css::xml::sax::XAttributeList> m_xAttrList;

    rtl::Reference<comphelper::AttributeList> m_pMutableAttrList;

    comphelper::AttributeList *GetMutableAttrList();

public:
    XMLMutableAttributeList();
    XMLMutableAttributeList( const css::uno::Reference<
        css::xml::sax::XAttributeList> & rAttrList,
           bool bClone=false );
    virtual ~XMLMutableAttributeList() override;

    // css::xml::sax::XAttributeList
    virtual sal_Int16 SAL_CALL getLength() override;
    virtual OUString SAL_CALL getNameByIndex(sal_Int16 i) override;
    virtual OUString SAL_CALL getTypeByIndex(sal_Int16 i) override;
    virtual OUString SAL_CALL getTypeByName(const OUString& aName) override;
    virtual OUString SAL_CALL getValueByIndex(sal_Int16 i) override;
    virtual OUString SAL_CALL getValueByName(const OUString& aName) override;

    // css::util::XCloneable
    virtual css::uno::Reference< css::util::XCloneable > SAL_CALL createClone() override;

    // methods that are not contained in any interface
    void SetValueByIndex( sal_Int16 i, const OUString& rValue );
    void AddAttribute( const OUString &sName , const OUString &sValue );
//  void Clear();
    void RemoveAttributeByIndex( sal_Int16 i );
    void RenameAttributeByIndex( sal_Int16 i, const OUString& rNewName );
    void AppendAttributeList( const css::uno::Reference< css::xml::sax::XAttributeList > & );

    sal_Int16 GetIndexByName( const OUString& rName ) const;
};


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
