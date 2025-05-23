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


#include "elementmark.hxx"
#include "elementcollector.hxx"
#include <com/sun/star/xml/crypto/sax/ConstOfSecurityId.hpp>
#include <com/sun/star/xml/crypto/sax/XReferenceResolvedListener.hpp>
#include <utility>

ElementCollector::ElementCollector(
    sal_Int32 nBufferId,
    css::xml::crypto::sax::ElementMarkPriority nPriority,
    bool bToModify,
    css::uno::Reference< css::xml::crypto::sax::XReferenceResolvedListener > xReferenceResolvedListener)
    :ElementMark(css::xml::crypto::sax::ConstOfSecurityId::UNDEFINEDSECURITYID, nBufferId),
     m_nPriority(nPriority),
     m_bToModify(bToModify),
     m_bAbleToNotify(false),
     m_bNotified(false),
     m_xReferenceResolvedListener(std::move(xReferenceResolvedListener))
/****** ElementCollector/ElementCollector *************************************
 *
 *   NAME
 *  ElementCollector -- constructor method
 *
 *   SYNOPSIS
 *  ElementCollector(nSecurityId, nBufferId, nPriority, bToModify
 *                   xReferenceResolvedListener);
 *
 *   FUNCTION
 *  construct an ElementCollector object.
 *
 *   INPUTS
 *  nSecurityId -   represents which security entity the buffer node is
 *          related with. Either a signature or an encryption is
 *          a security entity.
 *  nBufferId - the id of the element buffered in the document
 *          wrapper component. The document wrapper component
 *          uses this id to search the particular buffered
 *          element.
 *  nPriority - the priority value. ElementCollector with lower
 *          priority value can't notify until all ElementCollectors
 *          with higher priority value have notified.
 *  bToModify - A flag representing whether this ElementCollector
 *          notification will cause the modification of its working
 *                  element.
 *  xReferenceResolvedListener
 *            - the listener that this ElementCollector notifies to.
 ******************************************************************************/
{
    m_type = css::xml::crypto::sax::ElementMarkType_ELEMENTCOLLECTOR;
}


void ElementCollector::notifyListener()
/****** ElementCollector/notifyListener ***************************************
 *
 *   NAME
 *  notifyListener -- enable the ability to notify the listener
 *
 *   SYNOPSIS
 *  notifyListener();
 *
 *   FUNCTION
 *  enable the ability to notify the listener and try to notify then.
 ******************************************************************************/
{
    m_bAbleToNotify = true;
    doNotify();
}

void ElementCollector::setReferenceResolvedListener(
    const css::uno::Reference< css::xml::crypto::sax::XReferenceResolvedListener >& xReferenceResolvedListener)
/****** ElementCollector/setReferenceResolvedListener *************************
 *
 *   NAME
 *  setReferenceResolvedListener -- configures a listener for the buffer
 *  node in this object
 *
 *   SYNOPSIS
 *  setReferenceResolvedListener(xReferenceResolvedListener);
 *
 *   FUNCTION
 *  configures a new listener and try to notify then.
 *
 *   INPUTS
 *  xReferenceResolvedListener - the new listener
 ******************************************************************************/
{
    m_xReferenceResolvedListener = xReferenceResolvedListener;
    doNotify();
}

void ElementCollector::doNotify()
/****** ElementCollector/doNotify *********************************************
 *
 *   NAME
 *  doNotify -- tries to notify the listener
 *
 *   SYNOPSIS
 *  doNotify();
 *
 *   FUNCTION
 *  notifies the listener when all below conditions are satisfied:
 *  the listener has not been notified;
 *  the notify right is granted;
 *  the listener has already been configured;
 *  the security id has already been configure
 ******************************************************************************/
{
    if (!m_bNotified &&
        m_bAbleToNotify &&
        m_xReferenceResolvedListener.is() &&
        m_nSecurityId != css::xml::crypto::sax::ConstOfSecurityId::UNDEFINEDSECURITYID)
    {
        m_bNotified = true;
        m_xReferenceResolvedListener->referenceResolved(m_nBufferId);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
