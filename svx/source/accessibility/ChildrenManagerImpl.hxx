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

#ifndef INCLUDED_SVX_SOURCE_ACCESSIBILITY_CHILDRENMANAGERIMPL_HXX
#define INCLUDED_SVX_SOURCE_ACCESSIBILITY_CHILDRENMANAGERIMPL_HXX

#include <svx/AccessibleShape.hxx>
#include <svx/IAccessibleViewForwarderListener.hxx>
#include <svx/IAccessibleParent.hxx>
#include <svx/AccessibleShapeTreeInfo.hxx>
#include <editeng/AccessibleContextBase.hxx>
#include <comphelper/compbase.hxx>
#include <tools/gen.hxx>
#include <vector>
#include <com/sun/star/drawing/XShape.hpp>
#include <com/sun/star/drawing/XShapes.hpp>
#include <com/sun/star/document/XEventListener.hpp>
#include <com/sun/star/view/XSelectionChangeListener.hpp>
#include <com/sun/star/accessibility/XAccessible.hpp>

namespace accessibility {

class ChildDescriptor; // See below for declaration.
typedef ::std::vector<ChildDescriptor> ChildDescriptorListType;

// Re-using MutexOwner class defined in AccessibleContextBase.hxx

/** This class contains the actual implementation of the children manager.

    <p>It maintains a set of visible accessible shapes in
    <member>maVisibleChildren</member>.  The objects in this list stem from
    two sources.  The first is a list of UNO shapes like the list of shapes
    in a draw page.  A reference to this list is held in
    <member>maShapeList</member>.  Accessible objects for these shapes are
    created on demand.  The list can be replaced by calls to the
    <member>SetShapeList</member> method.  The second source is a list of
    already accessible objects.  It can be modified by calls to the
    <member>AddAccessibleShape</member> and
    <member>ClearAccessibleShapeList</member> methods.</p>

    <p>Each call of the <member>Update</member> method leads to a
    re-calculation of the visible shapes which then can be queried with the
    <member>GetChildCount</member> and <member>GetChild</member> methods.
    Events are sent informing all listeners about the removed shapes which are
    not visible anymore and about the added shapes.</p>

    <p> The visible area which is used to determine the visibility of the
    shapes is taken from the view forwarder.  Thus, to signal a change of
    the visible area call <member>ViewForwarderChanged</member>.</p>

    <p>The children manager adds itself as disposing() listener at every UNO
    shape it creates an accessible object for so that when the UNO shape
    passes away it can dispose() the associated accessible object.</p>

    @see ChildrenManager
*/
class ChildrenManagerImpl final
    :   public comphelper::WeakComponentImplHelper<
            css::document::XEventListener,
            css::view::XSelectionChangeListener>,
        public IAccessibleViewForwarderListener,
        public IAccessibleParent
{
public:
    /** Create a children manager, which manages the children of the given
        parent.  The parent is used for creating accessible objects.  The
        list of shapes for which to create those objects is not derived from
        the parent and has to be provided separately by calling one of the
        update methods.
        @param rxParent
            The parent of the accessible objects which will be created
            on demand at some point of time in the future.
        @param rxShapeList
            List of UNO shapes to manage.
        @param rShapeTreeInfo
            Bundle of information passed down the shape tree.
        @param rContext
            An accessible context object that is called for firing events
            for new and deleted children, i.e. that holds a list of
            listeners to be informed.
    */
    ChildrenManagerImpl (css::uno::Reference<css::accessibility::XAccessible> xParent,
        css::uno::Reference<css::drawing::XShapes> xShapeList,
        const AccessibleShapeTreeInfo& rShapeTreeInfo,
        AccessibleContextBase& rContext);

    /** If there still are managed children these are disposed and
        released.
    */
    virtual ~ChildrenManagerImpl() override;

    /** Do that part of the initialization that you can not or should not do
        in the constructor like registering at broadcasters.
    */
    void Init();

    /** Return the number of currently visible accessible children.
        @return
            If there are no children a 0 is returned.
    */
    sal_Int64 GetChildCount() const noexcept;

    /// @throws css::uno::RuntimeException
    /// @throws css::lang::IndexOutOfBoundsException
    const css::uno::Reference<css::drawing::XShape>& GetChildShape(sal_Int64 nIndex);
    /** Return the requested accessible child or throw and
        IndexOutOfBoundsException if the given index is invalid.
        @param nIndex
            Index of the requested child.  Call getChildCount for obtaining
            the number of children.
        @return
            In case of a valid index this method returns a reference to the
            requested accessible child.  This reference is empty if it has
            not been possible to create the accessible object of the
            corresponding shape.
        @throws
            Throws an IndexOutOfBoundsException if the index is not valid.
    */
    css::uno::Reference<css::accessibility::XAccessible>
        GetChild (sal_Int64 nIndex);

    /** Return the requested accessible child.
        @param aChildDescriptor
            This object contains references to the original shape and its
            associated accessible object.
        @param  _nIndex
            The index which will be used in getAccessibleIndexInParent of the accessible shape.
        @return
            Returns a reference to the requested accessible child.  This
            reference is empty if it has not been possible to create the
            accessible object of the corresponding shape.
        @throws css::uno::RuntimeException
    */
    css::uno::Reference<css::accessibility::XAccessible>
        GetChild (ChildDescriptor& aChildDescriptor,sal_Int32 _nIndex);

    /** Update the child manager.  Take care of a modified set of children
        and modified visible area.  This method can optimize the update
        process with respect separate updates of a modified children list
        and visible area.
        @param bCreateNewObjectsOnDemand
            If </true> then accessible objects associated with the visible
            shapes are created only when asked for.  No event is sent on
            creation.  If </false> then the accessible objects are created
            before this method returns and events are sent to inform the
            listeners of the new object.
    */
    void Update (bool bCreateNewObjectsOnDemand);

    /** Set the list of UNO shapes to the given list.  This removes the old
        list and does not add to it. The list of accessible shapes that is
        build up by calls to <member>AddAccessibleShape</member> is not
        modified.  Neither is the list of visible children.  Accessible
        objects are created on demand.
        @param xShapeList
            The list of UNO shapes that replaces the old list.
    */
    void SetShapeList (const css::uno::Reference<css::drawing::XShapes>& xShapeList);

    /** Add an accessible shape.  This does not modify the list of UNO shapes
        or the list of visible shapes.  Accessible shapes are, at the
        moment, not tested against the visible area but are always appended
        to the list of visible children.
        @param shape
            The new shape that is added to the list of accessible shapes; must
            be non-null.
    */
    void AddAccessibleShape (rtl::Reference<AccessibleShape> const & shape);

    /** Clear the lists of accessible shapes and that of visible accessible
        shapes.  The list of UNO shapes is not modified.
    */
    void ClearAccessibleShapeList();

    /** Set a new event shape tree info.  Call this method to inform the
        children manager of a change of the info bundle.
        @param rShapeTreeInfo
            The new info that replaces the current one.
    */
    void SetInfo (const AccessibleShapeTreeInfo& rShapeTreeInfo);

    /** Update the SELECTED and FOCUSED states of all visible children
        according to the given selection.  This includes setting
        <em>and</em> resetting the states.
    */
    void UpdateSelection();

    /** Return whether one of the shapes managed by this object has
        currently the focus.
        @return
            Returns <true/> when there is a shape that has the focus and
            <false/> when there is no such shape.
    */
    bool HasFocus() const;

    /** When there is a shape that currently has the focus,
        i.e. <member>HasFocus()</member> returns <true/> then remove the
        focus from that shape.  Otherwise nothing changes.
    */
    void RemoveFocus();

    // lang::XEventListener
    virtual void SAL_CALL
        disposing (const css::lang::EventObject& rEventObject) override;

    // document::XEventListener
    virtual void SAL_CALL
        notifyEvent (const css::document::EventObject& rEventObject) override;

    // view::XSelectionChangeListener
    virtual void  SAL_CALL
        selectionChanged (const css::lang::EventObject& rEvent) override;

    // IAccessibleViewForwarderListener
    /** Informs this children manager and its children about a change of one
        (or more) aspect of the view forwarder.
        @param aChangeType
            A change type of <const>VISIBLE_AREA</const> leads to a call to
            the <member>Update</member> which creates accessible objects of
            new shapes immediately.  Other change types are passed to the
            visible accessible children without calling
            <member>Update</member>.
        @param pViewForwarder
            The modified view forwarder.  Use this one from now on.
    */
    virtual void ViewForwarderChanged() override;

    // IAccessibleParent
    /** Replace the specified child with a replacement.
        @param pCurrentChild
            This child is to be replaced.
        @param pReplacement
            The replacement for the current child.
        @return
            The returned value indicates whether the replacement has been
            finished successfully.
    */
    virtual bool ReplaceChild (
        AccessibleShape* pCurrentChild,
        const css::uno::Reference< css::drawing::XShape >& _rxShape,
        const tools::Long _nIndex,
        const AccessibleShapeTreeInfo& _rShapeTreeInfo
    ) override;

    // Add the impl method for IAccessibleParent interface
    virtual AccessibleControlShape* GetAccControlShapeFromModel
        (css::beans::XPropertySet* pSet) override;
    virtual AccessibleShape*
        GetAccessibleCaption (const css::uno::Reference<css::drawing::XShape>& xShape) override;

private:
    /** This list holds the descriptors of all currently visible shapes and
        associated accessible object.

        <p>With the descriptors it maintains a mapping of shapes to
        accessible objects.  It acts as a cache in that accessible objects
        are only created on demand and released with every update (where the
        latter may be optimized by the update methods).<p>

        <p>The list is realized as a vector because it remains unchanged
        between updates (i.e. complete rebuilds of the list) and allows a
        fast (constant time) access to its elements for given indices.</p>
    */
    ChildDescriptorListType maVisibleChildren;

    /** The original list of UNO shapes.  The visible shapes are inserted
        into the list of visible children
        <member>maVisibleChildren</member>.
    */
    css::uno::Reference<css::drawing::XShapes> mxShapeList;

    /** This list of additional accessible shapes that can or shall not be
        created by the shape factory.
    */
    typedef std::vector< rtl::Reference< AccessibleShape> > AccessibleShapeList;
    AccessibleShapeList maAccessibleShapes;

    /** Rectangle that describes the visible area in which a shape has to lie
        at least partly, to be accessible through this class.  Used to
        detect changes of the visible area after changes of the view forwarder.
    */
    tools::Rectangle maVisibleArea;

    /** The parent of the shapes.  It is used for creating accessible
        objects for given shapes.
    */
    css::uno::Reference<css::accessibility::XAccessible> mxParent;

    /** Bundle of information passed down the shape tree.
    */
    AccessibleShapeTreeInfo maShapeTreeInfo;

    /** Reference to an accessible context object that is used to inform its
        listeners of new and removed children.
    */
    AccessibleContextBase& mrContext;

    /** This method is called from the component helper base class while
        disposing.
    */
    virtual void disposing(std::unique_lock<std::mutex>&) override;

    void impl_dispose();

    ChildrenManagerImpl (const ChildrenManagerImpl&) = delete;
    ChildrenManagerImpl& operator= (const ChildrenManagerImpl&) = delete;

    /** This member points to the currently focused shape.  It is NULL when
        there is no focused shape.
    */
    AccessibleShape* mpFocusedShape;

    /** Three helper functions for the <member>Update</member> method.
    */

    /** Create a list of visible shapes from the list of UNO shapes
        <member>maShapeList</member> and the list of accessible objects.
        @param raChildList
            For every visible shape from the two sources mentioned above one
            descriptor is added to this list.
    */
    void CreateListOfVisibleShapes (ChildDescriptorListType& raChildList);

    /** From the old list of (former) visible shapes remove those that
        are not member of the new list.  Send appropriate events for every
        such shape.
        @param raNewChildList
            The new list of visible children against which the old one
            is compared.
        @param raOldChildList
            The old list of visible children against which the new one
            is compared.
    */
    void RemoveNonVisibleChildren (
        const std::vector<ChildDescriptor*>& rNonVisibleChildren);

    /** Merge the information that is already known about the visible shapes
        from the old list into the current list, and return a list of
        children that are in the old list, but not the current one.
        @param raChildList
            Information is merged to the current list of visible children
            from this list. The old list can get reordered.
        @return
            Vector of children that are in the old list, but not the current
            one.
    */
    std::vector<ChildDescriptor*> MergeAccessibilityInformation (ChildDescriptorListType& raChildList);

    /** If the visible area has changed then send events that signal a
        change of their bounding boxes for all shapes that are members of
        both the current and the new list of visible shapes.
        @param raChildList
            Events are sent to all entries of this list that already contain
            an accessible object.
    */
    static void SendVisibleAreaEvents (ChildDescriptorListType& raChildList);

    /** If children have to be created immediately and not on demand the
        create the missing accessible objects now.
        @param raDescriptorList
            Create an accessible object for every member of this list where
            that object does not already exist.
    */
    void CreateAccessibilityObjects (ChildDescriptorListType& raChildList);

    /** Add a single shape.  Update all relevant data structures
        accordingly.  Use this method instead of <member>Update()</member>
        when only a single shape has been added.
    */
    void AddShape (const css::uno::Reference<css::drawing::XShape>& xShape);

    /** Remove a single shape.  Update all relevant data structures
        accordingly.  Use this method instead of <member>Update()</member>
        when only a single shape has been removed.
    */
    void RemoveShape (const css::uno::Reference<css::drawing::XShape>& xShape);

    /** Add the children manager as dispose listener at the given shape so
        that the associated accessible object can be disposed when the shape
        is disposed.
        @param xShape
            Register at this shape as dispose listener.
    */
    void RegisterAsDisposeListener (const css::uno::Reference<css::drawing::XShape>& xShape);

    /** Remove the children manager as dispose listener at the given shape
        @param xShape
            Unregister at this shape as dispose listener.
    */
    void UnregisterAsDisposeListener (const css::uno::Reference<css::drawing::XShape>& xShape);
};


/** A child descriptor holds a reference to a UNO shape and the
    corresponding accessible object.  There are two use cases:
    <ol><li>The accessible object is only created on demand and is then
    initially empty.</li>
    <li>There is no UNO shape.  The accessible object is given as argument
    to the constructor.</li>
    </ol>
    In both cases the child descriptor assumes ownership over the accessible
    object.
*/
class ChildDescriptor
{
public:
    /** Reference to a (partially) visible shape.
    */
    css::uno::Reference<css::drawing::XShape> mxShape;

    /** The corresponding accessible object.  This reference is initially
        empty and only replaced by a reference to a new object when that is
        requested from the outside.
    */
    rtl::Reference<AccessibleShape> mxAccessibleShape;

    /** Return a pointer to the implementation object of the accessible
        shape of this descriptor.
        @return
            The result is NULL if either the UNO reference to the accessible
            shape is empty or it can not be transformed into a pointer to
            the desired class.
    */
    AccessibleShape* GetAccessibleShape() const { return mxAccessibleShape.get(); }

    /** set the index _nIndex at the accessible shape
        @param  _nIndex
            The new index in parent.
    */
    void setIndexAtAccessibleShape(sal_Int32 _nIndex);

    /** This flag is set during the visibility calculation and indicates
        that at one time in this process an event is sent that informs the
        listeners of the creation of a new accessible object.  This flags is
        not reset afterwards.  Don't use it unless you know exactly what you
        are doing.
    */
    bool mbCreateEventPending;

    /** Create a new descriptor for the specified shape with empty reference
        to accessible object.
    */
    explicit ChildDescriptor (const css::uno::Reference<css::drawing::XShape>& xShape);

    /** Create a new descriptor for the specified shape with empty reference
        to the original shape.
    */
    explicit ChildDescriptor (const rtl::Reference<AccessibleShape>& rxAccessibleShape);

    /** Dispose the accessible object of this descriptor.  If that object
        does not exist then do nothing.
        @param rParent
            The parent of the accessible object to dispose.  A child event
            is sent in its name.
    */
    void disposeAccessibleObject (AccessibleContextBase& rParent);

    /** Compare two child descriptors.  Take into account that a child
        descriptor may be based on a UNO shape or, already, on an accessible
        shape.
    */
    bool operator == (const ChildDescriptor& aDescriptor) const
    {
        return (
                this == &aDescriptor ||
                (
                 (mxShape.get() == aDescriptor.mxShape.get() ) &&
                 (mxShape.is() || mxAccessibleShape.get() == aDescriptor.mxAccessibleShape.get())
                )
               );
    }

};


} // end of namespace accessibility

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
