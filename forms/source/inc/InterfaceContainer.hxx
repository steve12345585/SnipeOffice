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

#include <comphelper/uno3.hxx>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/lang/EventObject.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/io/XPersistObject.hpp>
#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <com/sun/star/beans/PropertyChangeEvent.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/script/XEventAttacherManager.hpp>
#include <com/sun/star/script/ScriptEventDescriptor.hpp>
#include <com/sun/star/container/XContainer.hpp>
#include <com/sun/star/container/XIndexContainer.hpp>
#include <com/sun/star/form/XFormComponent.hpp>
#include <com/sun/star/util/XCloneable.hpp>
#include <osl/mutex.hxx>
#include <comphelper/interfacecontainer3.hxx>
#include <cppuhelper/component.hxx>
#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/implbase8.hxx>
#include <unordered_map>

namespace com::sun::star::uno { class XComponentContext; }

using namespace comphelper;


namespace frm
{


    struct ElementDescription
    {
    public:
        css::uno::Reference< css::uno::XInterface >       xInterface;
        css::uno::Reference< css::beans::XPropertySet >   xPropertySet;
        css::uno::Reference< css::container::XChild >     xChild;
        css::uno::Any                                     aElementTypeInterface;

    public:
        ElementDescription( );

    private:
        ElementDescription( const ElementDescription& ) = delete;
        ElementDescription& operator=( const ElementDescription& ) = delete;
    };

typedef std::vector<css::uno::Reference<css::uno::XInterface>> OInterfaceArray;
typedef std::unordered_multimap< OUString, css::uno::Reference<css::uno::XInterface> > OInterfaceMap;


// OInterfaceContainer
// implements a container for form components

typedef ::cppu::ImplHelper8 <   css::container::XNameContainer
                            ,   css::container::XIndexContainer
                            ,   css::container::XContainer
                            ,   css::container::XEnumerationAccess
                            ,   css::script::XEventAttacherManager
                            ,   css::beans::XPropertyChangeListener
                            ,   css::io::XPersistObject
                            ,   css::util::XCloneable
                            > OInterfaceContainer_BASE;

class OInterfaceContainer : public OInterfaceContainer_BASE
{
protected:
    ::osl::Mutex&                           m_rMutex;

    OInterfaceArray                         m_aItems;
    OInterfaceMap                           m_aMap;
    ::comphelper::OInterfaceContainerHelper3<css::container::XContainerListener> m_aContainerListeners;

    const css::uno::Type                    m_aElementType;

    css::uno::Reference< css::uno::XComponentContext>     m_xContext;


    // EventManager
    css::uno::Reference< css::script::XEventAttacherManager>  m_xEventAttacher;

public:
    OInterfaceContainer(
        const css::uno::Reference< css::uno::XComponentContext>& _rxFactory,
        ::osl::Mutex& _rMutex,
        const css::uno::Type& _rElementType);

    OInterfaceContainer( ::osl::Mutex& _rMutex, const OInterfaceContainer& _cloneSource );

    // late constructor for cloning
    /// @throws css::uno::RuntimeException
    void clonedFrom(const OInterfaceContainer& _cloneSource);

protected:
    virtual ~OInterfaceContainer();

public:
// css::io::XPersistObject
    virtual OUString SAL_CALL getServiceName(  ) override = 0;
    virtual void SAL_CALL write( const css::uno::Reference< css::io::XObjectOutputStream >& OutStream ) override;
    virtual void SAL_CALL read( const css::uno::Reference< css::io::XObjectInputStream >& InStream ) override;

// css::lang::XEventListener
    virtual void SAL_CALL disposing(const css::lang::EventObject& _rSource) override;

// css::beans::XPropertyChangeListener
    virtual void SAL_CALL propertyChange(const css::beans::PropertyChangeEvent& evt) override;

// css::container::XElementAccess
    virtual css::uno::Type SAL_CALL getElementType() override ;
    virtual sal_Bool SAL_CALL hasElements() override;

// css::container::XEnumerationAccess
    virtual css::uno::Reference< css::container::XEnumeration> SAL_CALL createEnumeration() override;

// css::container::XNameAccess
    virtual css::uno::Any SAL_CALL getByName( const OUString& aName ) override;
    virtual css::uno::Sequence<OUString> SAL_CALL getElementNames(  ) override;
    virtual sal_Bool SAL_CALL hasByName( const OUString& aName ) override;

// css::container::XNameReplace
    virtual void SAL_CALL replaceByName(const OUString& Name, const css::uno::Any& _rElement) override;

// css::container::XNameContainer
    virtual void SAL_CALL insertByName(const OUString& Name, const css::uno::Any& _rElement) override;
    virtual void SAL_CALL removeByName(const OUString& Name) override;

// css::container::XIndexAccess
    virtual sal_Int32 SAL_CALL getCount() override;
    virtual css::uno::Any SAL_CALL getByIndex(sal_Int32 _nIndex) override;

// css::container::XIndexReplace
    virtual void SAL_CALL replaceByIndex(sal_Int32 _nIndex, const css::uno::Any& _rElement) override;

// css::container::XIndexContainer
    virtual void SAL_CALL insertByIndex(sal_Int32 _nIndex, const css::uno::Any& Element) override;
    virtual void SAL_CALL removeByIndex(sal_Int32 _nIndex) override;

// css::container::XContainer
    virtual void SAL_CALL addContainerListener(const css::uno::Reference< css::container::XContainerListener>& _rxListener) override;
    virtual void SAL_CALL removeContainerListener(const css::uno::Reference< css::container::XContainerListener>& _rxListener) override;

// css::script::XEventAttacherManager
    virtual void SAL_CALL registerScriptEvent( sal_Int32 nIndex, const css::script::ScriptEventDescriptor& aScriptEvent ) override;
    virtual void SAL_CALL registerScriptEvents( sal_Int32 nIndex, const css::uno::Sequence< css::script::ScriptEventDescriptor >& aScriptEvents ) override;
    virtual void SAL_CALL revokeScriptEvent( sal_Int32 nIndex, const OUString& aListenerType, const OUString& aEventMethod, const OUString& aRemoveListenerParam ) override;
    virtual void SAL_CALL revokeScriptEvents( sal_Int32 nIndex ) override;
    virtual void SAL_CALL insertEntry( sal_Int32 nIndex ) override;
    virtual void SAL_CALL removeEntry( sal_Int32 nIndex ) override;
    virtual css::uno::Sequence< css::script::ScriptEventDescriptor > SAL_CALL getScriptEvents( sal_Int32 Index ) override;
    virtual void SAL_CALL attach( sal_Int32 nIndex, const css::uno::Reference< css::uno::XInterface >& xObject, const css::uno::Any& aHelper ) override;
    virtual void SAL_CALL detach( sal_Int32 nIndex, const css::uno::Reference< css::uno::XInterface >& xObject ) override;
    virtual void SAL_CALL addScriptListener( const css::uno::Reference< css::script::XScriptListener >& xListener ) override;
    virtual void SAL_CALL removeScriptListener( const css::uno::Reference< css::script::XScriptListener >& Listener ) override;

protected:
    // helper
    virtual void SAL_CALL disposing();
    void removeElementsNoEvents();

    /** to be overridden if elements which are to be inserted into the container shall be checked

        <p>the ElementDescription given can be used to cache information about the object - it will be passed
        later on to implInserted/implReplaced.</p>
    */
    virtual void approveNewElement(
                    const css::uno::Reference< css::beans::XPropertySet >& _rxObject,
                    ElementDescription* _pElement
                );

    virtual ElementDescription* createElementMetaData( );

    /** inserts an object into our internal structures

        @param _nIndex
            the index at which position it should be inserted
        @param _bEvents
            if <TRUE/>, event knittings will be done
        @param _pApprovalResult
            must contain the result of an approveNewElement call. Can be <NULL/>, in this case, the approval
            is done within implInsert.
        @param _bFire
            if <TRUE/>, a notification about the insertion will be fired
        @throws css::lang::IllegalArgumentException
    */
            void implInsert(
                sal_Int32 _nIndex,
                const css::uno::Reference< css::beans::XPropertySet >& _rxObject,
                bool _bEvents /* = sal_True */,
                ElementDescription* _pApprovalResult /* = NULL */ ,
                bool _bFire /* = sal_True */
            );

    // called after the object is inserted, but before the "real listeners" are notified
    virtual void implInserted( const ElementDescription* _pElement );
    // called after the object is removed, but before the "real listeners" are notified
    virtual void implRemoved(const css::uno::Reference<css::uno::XInterface>& _rxObject);

    /** called after an object was replaced. The default implementation notifies our listeners, after releasing
        the instance lock.
    */
    virtual void impl_replacedElement(
                    const css::container::ContainerEvent& _rEvent,
                    ::osl::ClearableMutexGuard& _rInstanceLock
                );

    void SAL_CALL writeEvents(const css::uno::Reference< css::io::XObjectOutputStream>& _rxOutStream);
    void SAL_CALL readEvents(const css::uno::Reference< css::io::XObjectInputStream>& _rxInStream);

    /** replace an element, specified by position

        @precond <arg>_nIndex</arg> is a valid index
        @precond our mutex is locked exactly once, by the guard specified with <arg>_rClearBeforeNotify</arg>

    */
    void implReplaceByIndex(
            const sal_Int32 _nIndex,
            const css::uno::Any& _rNewElement,
            ::osl::ClearableMutexGuard& _rClearBeforeNotify
        );

    /** removes an element, specified by position

        @precond <arg>_nIndex</arg> is a valid index
        @precond our mutex is locked exactly once, by the guard specified with <arg>_rClearBeforeNotify</arg>

    */
    void implRemoveByIndex(
            const sal_Int32 _nIndex,
            ::osl::ClearableMutexGuard& _rClearBeforeNotify
        );

    /** validates the given index
        @throws css::lang::IndexOutOfBoundsException
            if the given index does not denote a valid position in our children array
    */
    void implCheckIndex( const sal_Int32 _nIndex );

private:
    // hack for Vba Events
    void impl_addVbEvents_nolck_nothrow( const sal_Int32 i_nIndex );

    void    transformEvents();

    void    impl_createEventAttacher_nothrow();
};

typedef ::cppu::ImplHelper1< css::form::XFormComponent> OFormComponents_BASE;
class OFormComponents   :public ::cppu::OComponentHelper
                        ,public OInterfaceContainer
                        ,public OFormComponents_BASE
{
protected:
    ::osl::Mutex                               m_aMutex;
    css::uno::Reference<css::uno::XInterface>  m_xParent;

public:
    OFormComponents(const css::uno::Reference< css::uno::XComponentContext>& _rxFactory);
    OFormComponents( const OFormComponents& _cloneSource );
    virtual ~OFormComponents() override;

    DECLARE_UNO3_AGG_DEFAULTS(OFormComponents, ::cppu::OComponentHelper)

    virtual css::uno::Any SAL_CALL queryAggregation(const css::uno::Type& _rType) override;
    virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes(  ) override;

// OComponentHelper
    virtual void SAL_CALL disposing() override;

// css::form::XFormComponent
    virtual css::uno::Reference<css::uno::XInterface> SAL_CALL getParent() override;
    virtual void SAL_CALL setParent(const css::uno::Reference<css::uno::XInterface>& Parent) override;

    // XEventListener
    using OInterfaceContainer::disposing;
};

}   // namespace frm


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
