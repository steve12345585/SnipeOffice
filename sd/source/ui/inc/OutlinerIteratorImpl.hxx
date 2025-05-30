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

#include <svx/svditer.hxx>
#include <OutlinerIterator.hxx>
#include <optional>

class SdDrawDocument;
class SdPage;

namespace sd {

class ViewShell;

namespace outliner {

/** Base class for the polymorphic implementation class of the
    <type>Iterator</type> class.  The iterators based on this class are
    basically uni directional iterators.  Their direction can, however, be
    reversed at any point of their life time.
*/
class IteratorImplBase
{
public:
    /** The constructor stores the given arguments to be used by the derived
        classes.
        @param pDocument
            The document provides the information to be iterated on.
        @param pViewShellWeak
            Some information has to be taken from the view shell.
        @param bDirectionIsForward
            This flag defines the iteration direction.  When <TRUE/> then
            the direction is forwards otherwise it is backwards.
    */
    IteratorImplBase (SdDrawDocument* pDocument,
        const std::weak_ptr<ViewShell>& rpViewShellWeak,
        bool bDirectionIsForward);
    IteratorImplBase (SdDrawDocument* pDocument,
        std::weak_ptr<ViewShell> pViewShellWeak,
        bool bDirectionIsForward, PageKind ePageKind, EditMode eEditMode);
    virtual ~IteratorImplBase();

    /** Advance to the next text of the current object or to the next object.
        This takes the iteration direction into
        account.  The new object pointed to can be retrieved (among other
        information) by calling the <member>GetPosition</member> method.
    */
    virtual void GotoNextText() = 0;
    /** Return an object that describes the current object.
        @return
            The returned object describes the current object pointed to by
            the iterator.  See the description of
            <type>IteratorPosition</type> for details on the available
            information.
    */
    virtual const IteratorPosition& GetPosition();
    /** Create an exact copy of this object.  No argument should be
        specified when called from the outside.  It then creates an object
        first and passes that to the inherited <member>Clone()</member>
        methods to fill in class specific information.
        @return
            Returns a copy of this object.  When this method is called with
            an argument then this value will be returned.
    */
    virtual IteratorImplBase* Clone (IteratorImplBase* pObject=nullptr) const;
    /** Test the equality of the this object and the given iterator.  Two
        iterators are taken to be equal when they point to the same object.
        Iteration direction is not taken into account.
        @param rIterator
            The iterator to compare to.
        @return
            When both iterators are equal <TRUE/> is returned, <FALSE/> otherwise.
    */
    virtual bool operator== (const IteratorImplBase& rIterator) const;
    /** This method is used by the equality operator. It is part of a "multimethod" pattern.
        @param rIterator
            The iterator to compare to.
        @return
            Returns <TRUE/> when both iterators point to the same object.
    */
    virtual bool IsEqualSelection(const IteratorImplBase& rIterator) const;
    /** Reverse the direction of iteration.  The current object stays the same.
    */
    virtual void Reverse();

protected:
    /// The current position as returned by <member>GetPosition()</member>.
    IteratorPosition maPosition;
    /// The document on whose data the iterator operates.
    SdDrawDocument* mpDocument;
    /// Necessary secondary source of information.
    std::weak_ptr<ViewShell> mpViewShellWeak;
    /// Specifies the search direction.
    bool mbDirectionIsForward;
};

/** Iterator all objects that belong to the current mark list
    a.k.a. selection.  It is assumed that all marked objects belong to the
    same page.  It is further assumed that the mark list does not change
    while an iterator is alive.  It is therefore the responsibility of an
    iterator's owner to handle the case of a changed mark list.

    <p>For documentation of the methods please refer to the base class
    <type>IteratorImplBase</type>.</p>
*/
class SelectionIteratorImpl final
    : public IteratorImplBase
{
public:
    SelectionIteratorImpl (
        const ::std::vector< ::unotools::WeakReference<SdrObject> >& rObjectList,
        sal_Int32 nObjectIndex,
        SdDrawDocument* pDocument,
        const std::weak_ptr<ViewShell>& rpViewShellWeak,
        bool bDirectionIsForward);
    SelectionIteratorImpl (const SelectionIteratorImpl& rObject);
    virtual ~SelectionIteratorImpl() override;

    virtual void GotoNextText() override;
    virtual const IteratorPosition& GetPosition() override;
    virtual IteratorImplBase* Clone (IteratorImplBase* pObject = nullptr) const override;
    virtual bool operator== (const IteratorImplBase& rIterator) const override;

private:
    const ::std::vector<::unotools::WeakReference<SdrObject>>& mrObjectList;
    sal_Int32 mnObjectIndex;

    /** Compare the given iterator with this object.  This method handles
        only the case that the given iterator is an instance of this class.
        @param rIterator
            The iterator to compare to.
        @return
            Returns <TRUE/> when both iterators point to the same object.
    */
    virtual bool IsEqualSelection(const IteratorImplBase& rIterator) const override;

    IteratorImplBase& operator= (const IteratorImplBase& rIterator);
};

/** Iterator for iteration over all objects in all views. It switches views when
    appropriate.

    Iterates in the following pattern
    1-) Alternating Normal View and Notes View for each page
    2-) Master Pages
    3-) Notes Masters
    4-) The Handout Master

    <p>For documentation of the methods please refer to the base class
    <type>IteratorImplBase</type>.</p>
*/
class DocumentIteratorImpl final : public IteratorImplBase
{
public:
    DocumentIteratorImpl (
        sal_Int32 nPageIndex,
        PageKind ePageKind,
        EditMode eEditMode,
        SdDrawDocument* pDocument,
        const std::weak_ptr<ViewShell>& rpViewShellWeak,
        bool bDirectionIsForward);
    virtual ~DocumentIteratorImpl() override;

    virtual void GotoNextText() override;
    virtual IteratorImplBase* Clone (IteratorImplBase* pObject = nullptr) const override;
    virtual void Reverse() override;

private:
    /** Set up page pointer and object list iterator for the specified
        page.
        @param nPageIndex
            Index of the new page.  It may lie outside the valid range for
            page indices.
    */
    void SetPage (sal_Int32 nPageIndex);

    /// Iterator of all objects on the current page.
    std::optional<SdrObjListIter> moObjectIterator;

    /// Pointer to the page associated with the current page index. May be NULL.
    SdPage* mpPage;

    /// Number of pages in the view that is specified by <member>maPosition</member>.
    sal_Int32 mnPageCount;

    // Don't use this operator.
    DocumentIteratorImpl& operator= (const DocumentIteratorImpl& ) = delete;
};

} } // end of namespace ::sd::outliner

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
