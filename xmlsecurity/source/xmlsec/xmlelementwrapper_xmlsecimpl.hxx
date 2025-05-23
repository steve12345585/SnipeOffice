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

#include <com/sun/star/xml/wrapper/XXMLElementWrapper.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <cppuhelper/implbase.hxx>

#include <libxml/tree.h>

class XMLElementWrapper_XmlSecImpl : public cppu::WeakImplHelper
<
    css::xml::wrapper::XXMLElementWrapper,
    css::lang::XServiceInfo
>
/****** XMLElementWrapper_XmlSecImpl.hxx/CLASS XMLElementWrapper_XmlSecImpl ***
 *
 *   NAME
 *  XMLElementWrapper_XmlSecImpl -- Class to wrap a libxml2 node
 *
 *   FUNCTION
 *  Used as a wrapper class to transfer a libxml2 node structure
 *  between different UNO components.
 ******************************************************************************/
{
private:
    /* the libxml2 node wrapped by this object */
    xmlNodePtr const m_pElement;

public:
    explicit XMLElementWrapper_XmlSecImpl(const xmlNodePtr pNode);

    /* XXMLElementWrapper */

    /* css::lang::XServiceInfo */
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

public:
    /*
    *   NAME
    *  getNativeElement -- Retrieves the libxml2 node wrapped by this object
    *
    *   SYNOPSIS
    *  pNode = getNativeElement();
    *
    *   RESULT
    *  pNode - the libxml2 node wrapped by this object
    */
    xmlNodePtr getNativeElement( ) const { return m_pElement;}
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
