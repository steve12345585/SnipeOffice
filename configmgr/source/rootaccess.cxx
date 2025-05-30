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

#include <sal/config.h>

#include <cassert>
#include <utility>
#include <vector>

#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/EventObject.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/Type.hxx>
#include <com/sun/star/uno/XInterface.hpp>
#include <com/sun/star/util/ChangesEvent.hpp>
#include <com/sun/star/util/ChangesSet.hpp>
#include <com/sun/star/util/ElementChange.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/util/XChangesListener.hpp>
#include <com/sun/star/util/XChangesNotifier.hpp>
#include <comphelper/sequence.hxx>
#include <cppu/unotype.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/weak.hxx>
#include <osl/mutex.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>

#include "broadcaster.hxx"
#include "components.hxx"
#include "data.hxx"
#include "lock.hxx"
#include "modifications.hxx"
#include "node.hxx"
#include "rootaccess.hxx"

namespace configmgr {

RootAccess::RootAccess(
    Components & components, OUString pathRepresentation,
    OUString locale, bool update):
    Access(components), pathRepresentation_(std::move(pathRepresentation)),
    locale_(std::move(locale)),
    lock_( lock() ),
    update_(update), finalized_(false), alive_(true)
{
}

std::vector<OUString> RootAccess::getAbsolutePath() {
    getNode();
    return path_;
}

void RootAccess::initBroadcaster(
    Modifications::Node const & modifications, Broadcaster * broadcaster)
{
    assert(broadcaster != nullptr);
    std::vector< css::util::ElementChange > changes;
    initBroadcasterAndChanges(
        modifications, broadcaster, changesListeners_.empty() ? nullptr : &changes);
    if (changes.empty())
        return;

    css::util::ChangesSet set(comphelper::containerToSequence(changes));
    for (auto const& changesListener : changesListeners_)
    {
        cppu::OWeakObject* pSource = this;
        css::uno::Reference< css::uno::XInterface > xBase( pSource, css::uno::UNO_QUERY );
        broadcaster->addChangesNotification(
            changesListener,
            css::util::ChangesEvent(
                pSource, css::uno::Any( xBase ), set), path_.empty());
    }
}

void RootAccess::acquire() noexcept {
    Access::acquire();
}

void RootAccess::release() noexcept {
    Access::release();
}

OUString const & RootAccess::getAbsolutePathRepresentation() {
    getNode(); // turn pathRepresentation_ into canonic form
    return pathRepresentation_;
}


void RootAccess::setAlive(bool b) {
    alive_ = b;
}

void RootAccess::addChangesListener(
    css::uno::Reference< css::util::XChangesListener > const & aListener)
{
    assert(thisIs(IS_ANY));
    {
        osl::MutexGuard g(*lock_);
        checkLocalizedPropertyAccess();
        if (!aListener.is()) {
            throw css::uno::RuntimeException(
                u"null listener"_ustr, getXWeak());
        }
        if (!isDisposed()) {
            changesListeners_.insert(aListener);
            return;
        }
    }
    try {
        aListener->disposing(
            css::lang::EventObject(getXWeak()));
    } catch (css::lang::DisposedException &) {}
}

void RootAccess::removeChangesListener(
    css::uno::Reference< css::util::XChangesListener > const & aListener)
{
    assert(thisIs(IS_ANY));
    osl::MutexGuard g(*lock_);
    checkLocalizedPropertyAccess();
    ChangesListeners::iterator i(changesListeners_.find(aListener));
    if (i != changesListeners_.end()) {
        changesListeners_.erase(i);
    }
}

void RootAccess::commitChanges()
{
    assert(thisIs(IS_UPDATE));
    if (!alive_)
    {
        return;
    }
    Broadcaster bc;
    {
        osl::MutexGuard g(*lock_);

        checkLocalizedPropertyAccess();
        int finalizedLayer;
        Modifications globalMods;
        commitChildChanges(
            ((getComponents().resolvePathRepresentation(
                  pathRepresentation_, nullptr, nullptr, &finalizedLayer)
              == node_) &&
             finalizedLayer == Data::NO_LAYER),
            &globalMods);
        getComponents().writeModifications();
        getComponents().initGlobalBroadcaster(globalMods, this, &bc);
    }
    bc.send();
}

sal_Bool RootAccess::hasPendingChanges() {
    assert(thisIs(IS_UPDATE));
    osl::MutexGuard g(*lock_);
    checkLocalizedPropertyAccess();
    //TODO: Optimize:
    std::vector< css::util::ElementChange > changes;
    reportChildChanges(&changes);
    return !changes.empty();
}

css::uno::Sequence< ::css::util::ElementChange > RootAccess::getPendingChanges()
{
    assert(thisIs(IS_UPDATE));
    osl::MutexGuard g(*lock_);
    checkLocalizedPropertyAccess();
    std::vector< css::util::ElementChange > changes;
    reportChildChanges(&changes);
    return comphelper::containerToSequence(changes);
}

RootAccess::~RootAccess()
{
    osl::MutexGuard g(*lock_);
    if (alive_)
        getComponents().removeRootAccess(this);
}

std::vector<OUString> RootAccess::getRelativePath() {
    return std::vector<OUString>();
}

OUString RootAccess::getRelativePathRepresentation() {
    return OUString();
}

const rtl::Reference< Node > & RootAccess::getNode() {
    if (!node_.is()) {
        OUString canonic;
        int finalizedLayer;
        node_ = getComponents().resolvePathRepresentation(
            pathRepresentation_, &canonic, &path_, &finalizedLayer);
        if (!node_.is()) {
            throw css::uno::RuntimeException(
                "cannot find " + pathRepresentation_, nullptr);
                // RootAccess::queryInterface indirectly calls
                // RootAccess::getNode, so if this RootAccess were passed out in
                // RuntimeException.Context, client code that called
                // queryInterface on it would cause trouble; therefore,
                // RuntimeException.Context is left null here
        }
        pathRepresentation_ = canonic;
        assert(!path_.empty() || node_->kind() == Node::KIND_ROOT);
        if (!path_.empty()) {
            name_ = path_.back();
        }
        finalized_ = finalizedLayer != Data::NO_LAYER;
    }
    return node_;
}

bool RootAccess::isFinalized() {
    getNode();
    return finalized_;
}

const OUString & RootAccess::getNameInternal() {
    getNode();
    return name_;
}

rtl::Reference< RootAccess > RootAccess::getRootAccess() {
    return this;
}

rtl::Reference< Access > RootAccess::getParentAccess() {
    return rtl::Reference< Access >();
}

void RootAccess::addTypes(std::vector< css::uno::Type > * types) const {
    assert(types != nullptr);
    types->push_back(cppu::UnoType< css::util::XChangesNotifier >::get());
    types->push_back(cppu::UnoType< css::util::XChangesBatch >::get());
}

void RootAccess::addSupportedServiceNames(
    std::vector<OUString> * services)
{
    assert(services != nullptr);
    services->push_back(u"com.sun.star.configuration.AccessRootElement"_ustr);
    if (update_) {
        services->push_back(u"com.sun.star.configuration.UpdateRootElement"_ustr);
    }
}

void RootAccess::initDisposeBroadcaster(Broadcaster * broadcaster) {
    assert(broadcaster != nullptr);
    for (auto const& changesListener : changesListeners_)
    {
        broadcaster->addDisposeNotification(
            changesListener,
            css::lang::EventObject(getXWeak()));
    }
    Access::initDisposeBroadcaster(broadcaster);
}

void RootAccess::clearListeners() noexcept {
    changesListeners_.clear();
    Access::clearListeners();
}

css::uno::Any RootAccess::queryInterface(css::uno::Type const & aType)
{
    assert(thisIs(IS_ANY));
    osl::MutexGuard g(*lock_);
    checkLocalizedPropertyAccess();
    css::uno::Any res(Access::queryInterface(aType));
    if (res.hasValue()) {
        return res;
    }
    res = cppu::queryInterface(
        aType, static_cast< css::util::XChangesNotifier * >(this));
    if (res.hasValue()) {
        return res;
    }
    if (!res.hasValue() && update_) {
        res = cppu::queryInterface(
            aType, static_cast< css::util::XChangesBatch * >(this));
    }
    return res;
}

OUString RootAccess::getImplementationName()
{
    assert(thisIs(IS_ANY));
    osl::MutexGuard g(*lock_);
    checkLocalizedPropertyAccess();
    return u"configmgr.RootAccess"_ustr;
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
