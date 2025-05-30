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

#include <map>

#include <sal/macros.h>
#include <fmundo.hxx>
#include <fmpgeimp.hxx>
#include <svx/svditer.hxx>
#include <fmobj.hxx>
#include <fmprop.hxx>
#include <svx/strings.hrc>
#include <svx/dialmgr.hxx>
#include <svx/fmmodel.hxx>
#include <svx/fmpage.hxx>

#include <com/sun/star/util/XModifyBroadcaster.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/container/XContainer.hpp>
#include <com/sun/star/container/XContainerListener.hpp>
#include <com/sun/star/script/XEventAttacherManager.hpp>
#include <com/sun/star/form/binding/XBindableValue.hpp>
#include <com/sun/star/form/binding/XListEntrySink.hpp>
#include <com/sun/star/sdbc/XConnection.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/script/XScriptListener.hpp>

#include <svx/fmtools.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/event.hxx>
#include <osl/mutex.hxx>
#include <comphelper/property.hxx>
#include <comphelper/types.hxx>
#include <connectivity/dbtools.hxx>
#include <vcl/svapp.hxx>
#include <comphelper/processfactory.hxx>
#include <cppuhelper/implbase.hxx>


using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::awt;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::script;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::form;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::form::binding;
using namespace ::com::sun::star::sdbc;
using namespace ::svxform;
using namespace ::dbtools;


class ScriptEventListenerWrapper : public cppu::WeakImplHelper< XScriptListener >
{
public:
    /// @throws css::uno::RuntimeException
    explicit ScriptEventListenerWrapper( FmFormModel& _rModel)
        :m_rModel( _rModel )
        ,m_attemptedListenerCreation( false )
    {

    }
    // XEventListener
    virtual void SAL_CALL disposing(const EventObject& ) override {}

    // XScriptListener
    virtual void SAL_CALL firing(const  ScriptEvent& evt) override
    {
        attemptListenerCreation();
        if ( m_vbaListener.is() )
        {
            m_vbaListener->firing( evt );
        }
    }

    virtual Any SAL_CALL approveFiring(const ScriptEvent& evt) override
    {
        attemptListenerCreation();
        if ( m_vbaListener.is() )
        {
            return m_vbaListener->approveFiring( evt );
        }
        return Any();
    }

private:
    void attemptListenerCreation()
    {
        if ( m_attemptedListenerCreation )
            return;
        m_attemptedListenerCreation = true;

        try
        {
            const css::uno::Reference<css::uno::XComponentContext>& context(
                comphelper::getProcessComponentContext());
            Reference< XScriptListener > const xScriptListener(
                context->getServiceManager()->createInstanceWithContext(
                    u"ooo.vba.EventListener"_ustr, context),
                UNO_QUERY_THROW);
            Reference< XPropertySet > const xListenerProps( xScriptListener, UNO_QUERY_THROW );
            // SfxObjectShellRef is good here since the model controls the lifetime of the shell
            SfxObjectShellRef const xObjectShell = m_rModel.GetObjectShell();
            ENSURE_OR_THROW( xObjectShell.is(), "no object shell!" );
            xListenerProps->setPropertyValue(u"Model"_ustr, Any( xObjectShell->GetModel() ) );

            m_vbaListener = xScriptListener;
        }
        catch( Exception const & )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }
    FmFormModel&                    m_rModel;
    Reference< XScriptListener >    m_vbaListener;
    bool                            m_attemptedListenerCreation;


};


namespace {

// some helper structs for caching property infos

struct PropertyInfo
{
    bool    bIsTransientOrReadOnly  : 1;    // the property is transient or read-only, thus we need no undo action for it
    bool    bIsValueProperty        : 1;    // the property is the special value property, thus it may be handled
                                            // as if it's transient or persistent
};

}


struct PropertySetInfo
{
    typedef std::map<OUString, PropertyInfo> AllProperties;

    AllProperties   aProps;                 // all properties of this set which we know so far
    bool            bHasEmptyControlSource; // sal_True -> the set has a DataField property, and the current value is an empty string
                                            // sal_False -> the set has _no_ such property or its value isn't empty
};

static OUString static_STR_UNDO_PROPERTY;


FmXUndoEnvironment::FmXUndoEnvironment(FmFormModel& _rModel)
                   :rModel( _rModel )
                   ,m_pScriptingEnv( new svxform::FormScriptingEnvironment( _rModel ) )
                   ,m_Locks( 0 )
                   ,bReadOnly( false )
                   ,m_bDisposed( false )
{
    try
    {
        m_vbaListener =  new ScriptEventListenerWrapper( _rModel );
    }
    catch( Exception& )
    {
    }
}

FmXUndoEnvironment::~FmXUndoEnvironment()
{
    if ( !m_bDisposed )   // i120746, call FormScriptingEnvironment::dispose to avoid memory leak
        m_pScriptingEnv->dispose();
}

void FmXUndoEnvironment::dispose()
{
    OSL_ENSURE( !m_bDisposed, "FmXUndoEnvironment::dispose: disposed twice?" );
    if ( !m_bDisposed )
        return;

    Lock();

    sal_uInt16 nCount = rModel.GetPageCount();
    sal_uInt16 i;
    for (i = 0; i < nCount; i++)
    {
        FmFormPage* pPage = dynamic_cast<FmFormPage*>( rModel.GetPage(i)  );
        if ( pPage )
        {
            Reference< css::form::XForms > xForms = pPage->GetForms( false );
            if ( xForms.is() )
                RemoveElement( xForms );
        }
    }

    nCount = rModel.GetMasterPageCount();
    for (i = 0; i < nCount; i++)
    {
        FmFormPage* pPage = dynamic_cast<FmFormPage*>( rModel.GetMasterPage(i)  );
        if ( pPage )
        {
            Reference< css::form::XForms > xForms = pPage->GetForms( false );
            if ( xForms.is() )
                RemoveElement( xForms );
        }
    }

    UnLock();

    OSL_PRECOND( rModel.GetObjectShell(), "FmXUndoEnvironment::dispose: no object shell anymore!" );
    if ( rModel.GetObjectShell() )
        EndListening( *rModel.GetObjectShell() );

    if ( IsListening( rModel ) )
        EndListening( rModel );

    m_pScriptingEnv->dispose();

    m_bDisposed = true;
}


void FmXUndoEnvironment::ModeChanged()
{
    OSL_PRECOND( rModel.GetObjectShell(), "FmXUndoEnvironment::ModeChanged: no object shell anymore!" );
    if ( !rModel.GetObjectShell() )
        return;

    if (bReadOnly == (rModel.GetObjectShell()->IsReadOnly() || rModel.GetObjectShell()->IsReadOnlyUI()))
        return;

    bReadOnly = !bReadOnly;

    sal_uInt16 nCount = rModel.GetPageCount();
    sal_uInt16 i;
    for (i = 0; i < nCount; i++)
    {
        FmFormPage* pPage = dynamic_cast<FmFormPage*>( rModel.GetPage(i)  );
        if ( pPage )
        {
            Reference< css::form::XForms > xForms = pPage->GetForms( false );
            if ( xForms.is() )
                TogglePropertyListening( xForms );
        }
    }

    nCount = rModel.GetMasterPageCount();
    for (i = 0; i < nCount; i++)
    {
        FmFormPage* pPage = dynamic_cast<FmFormPage*>( rModel.GetMasterPage(i)  );
        if ( pPage )
        {
            Reference< css::form::XForms > xForms = pPage->GetForms( false );
            if ( xForms.is() )
                TogglePropertyListening( xForms );
        }
    }

    if (!bReadOnly)
        StartListening(rModel);
    else
        EndListening(rModel);
}


void FmXUndoEnvironment::Notify( SfxBroadcaster& /*rBC*/, const SfxHint& rHint )
{
    if (rHint.GetId() == SfxHintId::ThisIsAnSdrHint)
    {
        const SdrHint* pSdrHint = static_cast<const SdrHint*>(&rHint);
        switch (pSdrHint->GetKind())
        {
            case SdrHintKind::ObjectInserted:
            {
                SdrObject* pSdrObj = const_cast<SdrObject*>(pSdrHint->GetObject());
                Inserted( pSdrObj );
            }   break;
            case SdrHintKind::ObjectRemoved:
            {
                SdrObject* pSdrObj = const_cast<SdrObject*>(pSdrHint->GetObject());
                Removed( pSdrObj );
            }
            break;
            default:
                break;
        }
    }
    else if (rHint.GetId() == SfxHintId::ThisIsAnSfxEventHint)
    {
        switch (static_cast<const SfxEventHint&>(rHint).GetEventId())
        {
            case SfxEventHintId::CreateDoc:
            case SfxEventHintId::OpenDoc:
                ModeChanged();
                break;
            default: break;
        }
    }
    else if (rHint.GetId() != SfxHintId::NONE)
    {
        switch (rHint.GetId())
        {
            case SfxHintId::Dying:
                dispose();
                rModel.SetObjectShell( nullptr );
                break;
            case SfxHintId::ModeChanged:
                ModeChanged();
                break;
            default: break;
        }
    }
}

void FmXUndoEnvironment::Inserted(SdrObject* pObj)
{
    if (pObj->GetObjInventor() == SdrInventor::FmForm)
    {
        FmFormObj* pFormObj = dynamic_cast<FmFormObj*>( pObj );
        Inserted( pFormObj );
    }
    else if (pObj->IsGroupObject())
    {
        SdrObjListIter aIter(pObj->GetSubList());
        while ( aIter.IsMore() )
            Inserted( aIter.Next() );
    }
}


namespace
{
    bool lcl_searchElement(const Reference< XIndexAccess>& xCont, const Reference< XInterface >& xElement)
    {
        if (!xCont.is() || !xElement.is())
            return false;

        sal_Int32 nCount = xCont->getCount();
        Reference< XInterface > xComp;
        for (sal_Int32 i = 0; i < nCount; i++)
        {
            try
            {
                xCont->getByIndex(i) >>= xComp;
                if (xComp.is())
                {
                    if ( xElement == xComp )
                        return true;
                    else
                    {
                        Reference< XIndexAccess> xCont2(xComp, UNO_QUERY);
                        if (xCont2.is() && lcl_searchElement(xCont2, xElement))
                            return true;
                    }
                }
            }
            catch(const Exception&)
            {
                DBG_UNHANDLED_EXCEPTION("svx");
            }
        }
        return false;
    }
}


void FmXUndoEnvironment::Inserted(FmFormObj* pObj)
{
    DBG_ASSERT( pObj, "FmXUndoEnvironment::Inserted: invalid object!" );
    if ( !pObj )
        return;

    // is the control still assigned to a form
    Reference< XInterface >  xModel(pObj->GetUnoControlModel(), UNO_QUERY);
    Reference< XFormComponent >  xContent(xModel, UNO_QUERY);
    if (!(xContent.is() && pObj->getSdrPageFromSdrObject()))
        return;

    // if the component doesn't belong to a form, yet, find one to insert into
    if (!xContent->getParent().is())
    {
        try
        {
            const Reference< XIndexContainer >& xObjectParent = pObj->GetOriginalParent();

            FmFormPage& rPage(dynamic_cast< FmFormPage& >( *pObj->getSdrPageFromSdrObject()));
            Reference< XIndexAccess >  xForms( rPage.GetForms(), UNO_QUERY_THROW );

            Reference< XIndexContainer > xNewParent;
            Reference< XForm >           xForm;
            sal_Int32 nPos = -1;
            if ( lcl_searchElement( xForms, xObjectParent ) )
            {
                // the form which was the parent of the object when it was removed is still
                // part of the form component hierarchy of the current page
                xNewParent = xObjectParent;
                xForm.set( xNewParent, UNO_QUERY_THROW );
                nPos = ::std::min( pObj->GetOriginalIndex(), xNewParent->getCount() );
            }
            else
            {
                xForm.set( rPage.GetImpl().findPlaceInFormComponentHierarchy( xContent ), UNO_SET_THROW );
                xNewParent.set( xForm, UNO_QUERY_THROW );
                nPos = xNewParent->getCount();
            }

            FmFormPageImpl::setUniqueName( xContent, xForm );
            xNewParent->insertByIndex( nPos, Any( xContent ) );

            Reference< XEventAttacherManager >  xManager( xNewParent, UNO_QUERY_THROW );
            xManager->registerScriptEvents( nPos, pObj->GetOriginalEvents() );
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
        }
    }

    // reset FormObject
    pObj->ClearObjEnv();
}


void FmXUndoEnvironment::Removed(SdrObject* pObj)
{
    if ( pObj->IsVirtualObj() )
        // for virtual objects, we've already been notified of the removal of the master
        // object, which is sufficient here
        return;

    if (pObj->GetObjInventor() == SdrInventor::FmForm)
    {
        FmFormObj* pFormObj = dynamic_cast<FmFormObj*>( pObj );
        Removed(pFormObj);
    }
    else if (pObj->IsGroupObject())
    {
        SdrObjListIter aIter(pObj->GetSubList());
        while ( aIter.IsMore() )
            Removed( aIter.Next() );
    }
}


void FmXUndoEnvironment::Removed(FmFormObj* pObj)
{
    DBG_ASSERT( pObj, "FmXUndoEnvironment::Removed: invalid object!" );
    if ( !pObj )
        return;

    // is the control still assigned to a form
    Reference< XFormComponent >  xContent(pObj->GetUnoControlModel(), UNO_QUERY);
    if (!xContent.is())
        return;

    // The object is taken out of a list.
    // If a father exists, the object is removed at the father and
    // noted at the FormObject!

    // If the object is reinserted and a parent exists, this parent is set though.
    Reference< XIndexContainer >  xForm(xContent->getParent(), UNO_QUERY);
    if (!xForm.is())
        return;

    // determine which position the child was at
    const sal_Int32 nPos = getElementPos(xForm, xContent);
    if (nPos < 0)
        return;

    Sequence< ScriptEventDescriptor > aEvts;
    Reference< XEventAttacherManager >  xManager(xForm, UNO_QUERY);
    if (xManager.is())
        aEvts = xManager->getScriptEvents(nPos);

    try
    {
        pObj->SetObjEnv(xForm, nPos, aEvts);
        xForm->removeByIndex(nPos);
    }
    catch(Exception&)
    {
        DBG_UNHANDLED_EXCEPTION("svx");
    }
}

//  XEventListener

void SAL_CALL FmXUndoEnvironment::disposing(const EventObject& e)
{
    // check if it's an object we have cached information about
    if (m_pPropertySetCache)
    {
        Reference< XPropertySet > xSourceSet(e.Source, UNO_QUERY);
        if (xSourceSet.is())
        {
            PropertySetInfoCache::iterator aSetPos = m_pPropertySetCache->find(xSourceSet);
            if (aSetPos != m_pPropertySetCache->end())
                m_pPropertySetCache->erase(aSetPos);
        }
    }
}

// XPropertyChangeListener

void SAL_CALL FmXUndoEnvironment::propertyChange(const PropertyChangeEvent& evt)
{
    ::osl::ClearableMutexGuard aGuard( m_aMutex );

    if (!IsLocked())
    {
        Reference< XPropertySet >  xSet(evt.Source, UNO_QUERY);
        if (!xSet.is())
            return;

        // if it's a "default value" property of a control model, set the according "value" property
        static constexpr OUString pDefaultValueProperties[] = {
            FM_PROP_DEFAULT_TEXT, FM_PROP_DEFAULTCHECKED, FM_PROP_DEFAULT_DATE, FM_PROP_DEFAULT_TIME,
            FM_PROP_DEFAULT_VALUE, FM_PROP_DEFAULT_SELECT_SEQ, FM_PROP_EFFECTIVE_DEFAULT
        };
        static constexpr OUString aValueProperties[] = {
            FM_PROP_TEXT, FM_PROP_STATE, FM_PROP_DATE, FM_PROP_TIME,
            FM_PROP_VALUE, FM_PROP_SELECT_SEQ, FM_PROP_EFFECTIVE_VALUE
        };
        sal_Int32 nDefaultValueProps = SAL_N_ELEMENTS(pDefaultValueProperties);
        OSL_ENSURE(SAL_N_ELEMENTS(aValueProperties) == nDefaultValueProps,
            "FmXUndoEnvironment::propertyChange: inconsistence!");
        for (sal_Int32 i=0; i<nDefaultValueProps; ++i)
        {
            if (evt.PropertyName == pDefaultValueProperties[i])
            {
                try
                {
                    xSet->setPropertyValue(aValueProperties[i], evt.NewValue);
                }
                catch(const Exception&)
                {
                    OSL_FAIL("FmXUndoEnvironment::propertyChange: could not adjust the value property!");
                }
            }
        }

        // no Undo for transient and readonly props. But unfortunately "transient" is not only that the
        // "transient" flag is set for the property in question, instead it is somewhat more complex
        // Transience criterions are:
        // - the "transient" flag is set for the property
        // - OR the control has a non-empty COntrolSource property, i.e. is intended to be bound
        //   to a database column. Note that it doesn't matter here whether the control actually
        //   *is* bound to a column
        // - OR the control is bound to an external value via XBindableValue/XValueBinding
        //   which does not have a "ExternalData" property being <TRUE/>

        if (!m_pPropertySetCache)
            m_pPropertySetCache = std::make_unique<PropertySetInfoCache>();

        // let's see if we know something about the set
        PropertySetInfoCache::iterator aSetPos = m_pPropertySetCache->find(xSet);
        if (aSetPos == m_pPropertySetCache->end())
        {
            PropertySetInfo aNewEntry;
            if (!::comphelper::hasProperty(FM_PROP_CONTROLSOURCE, xSet))
            {
                aNewEntry.bHasEmptyControlSource = false;
            }
            else
            {
                try
                {
                    Any aCurrentControlSource = xSet->getPropertyValue(FM_PROP_CONTROLSOURCE);
                    aNewEntry.bHasEmptyControlSource = !aCurrentControlSource.hasValue() || ::comphelper::getString(aCurrentControlSource).isEmpty();
                }
                catch(const Exception&)
                {
                    DBG_UNHANDLED_EXCEPTION("svx");
                }
            }
            aSetPos = m_pPropertySetCache->emplace(xSet,aNewEntry).first;
            DBG_ASSERT(aSetPos != m_pPropertySetCache->end(), "FmXUndoEnvironment::propertyChange : just inserted it ... why it's not there ?");
        }
        else
        {   // is it the DataField property ?
            if (evt.PropertyName == FM_PROP_CONTROLSOURCE)
            {
                aSetPos->second.bHasEmptyControlSource = !evt.NewValue.hasValue() || ::comphelper::getString(evt.NewValue).isEmpty();
            }
        }

        // now we have access to the cached info about the set
        // let's see what we know about the property
        PropertySetInfo::AllProperties& rPropInfos = aSetPos->second.aProps;
        PropertySetInfo::AllProperties::iterator aPropertyPos = rPropInfos.find(evt.PropertyName);
        if (aPropertyPos == rPropInfos.end())
        {   // nothing 'til now ... have to change this...
            PropertyInfo aNewEntry;

            // the attributes
            sal_Int32 nAttributes = xSet->getPropertySetInfo()->getPropertyByName(evt.PropertyName).Attributes;
            aNewEntry.bIsTransientOrReadOnly = ((nAttributes & PropertyAttribute::READONLY) != 0) || ((nAttributes & PropertyAttribute::TRANSIENT) != 0);

            // check if it is the special "DataFieldProperty"
            aNewEntry.bIsValueProperty = false;
            try
            {
                if (::comphelper::hasProperty(FM_PROP_CONTROLSOURCEPROPERTY, xSet))
                {
                    Any aControlSourceProperty = xSet->getPropertyValue(FM_PROP_CONTROLSOURCEPROPERTY);
                    OUString sControlSourceProperty;
                    aControlSourceProperty >>= sControlSourceProperty;

                    aNewEntry.bIsValueProperty = (sControlSourceProperty == evt.PropertyName);
                }
            }
            catch(const Exception&)
            {
                DBG_UNHANDLED_EXCEPTION("svx");
            }

            // insert the new entry
            aPropertyPos = rPropInfos.emplace(evt.PropertyName,aNewEntry).first;
            DBG_ASSERT(aPropertyPos != rPropInfos.end(), "FmXUndoEnvironment::propertyChange : just inserted it ... why it's not there ?");
        }

        // now we have access to the cached info about the property affected
        // and are able to decide whether or not we need an undo action

        bool bAddUndoAction = rModel.IsUndoEnabled();
        // no UNDO for transient/readonly properties
        if ( bAddUndoAction && aPropertyPos->second.bIsTransientOrReadOnly )
            bAddUndoAction = false;

        if ( bAddUndoAction && aPropertyPos->second.bIsValueProperty )
        {
            // no UNDO when the "value" property changes, but the ControlSource is non-empty
            // (in this case the control is intended to be bound to a database column)
            if ( !aSetPos->second.bHasEmptyControlSource )
                bAddUndoAction = false;

            // no UNDO if the control is currently bound to an external value
            if ( bAddUndoAction )
            {
                Reference< XBindableValue > xBindable( evt.Source, UNO_QUERY );
                Reference< XValueBinding > xBinding;
                if ( xBindable.is() )
                    xBinding = xBindable->getValueBinding();

                Reference< XPropertySet > xBindingProps;
                Reference< XPropertySetInfo > xBindingPropsPSI;
                if ( xBindable.is() )
                    xBindingProps.set( xBinding, UNO_QUERY );
                if ( xBindingProps.is() )
                    xBindingPropsPSI = xBindingProps->getPropertySetInfo();
                // TODO: we should cache all those things, else this might be too expensive.
                // However, this requires we're notified of changes in the value binding

                static constexpr OUString s_sExternalData = u"ExternalData"_ustr;
                if ( xBindingPropsPSI.is() && xBindingPropsPSI->hasPropertyByName( s_sExternalData ) )
                {
                    bool bExternalData = true;
                    OSL_VERIFY( xBindingProps->getPropertyValue( s_sExternalData ) >>= bExternalData );
                    bAddUndoAction = !bExternalData;
                }
                else
                    bAddUndoAction = !xBinding.is();
            }
        }

        if ( bAddUndoAction && ( evt.PropertyName == FM_PROP_STRINGITEMLIST ) )
        {
            Reference< XListEntrySink > xSink( evt.Source, UNO_QUERY );
            if ( xSink.is() && xSink->getListEntrySource().is() )
                // #i41029# / 2005-01-31 / frank.schoenheit@sun.com
                bAddUndoAction = false;
        }

        if ( bAddUndoAction )
        {
            aGuard.clear();
            // TODO: this is a potential race condition: two threads here could in theory
            // add their undo actions out-of-order

            SolarMutexGuard aSolarGuard;
            rModel.AddUndo(std::make_unique<FmUndoPropertyAction>(rModel, evt));
        }
    }
    else
    {
        // if it's the DataField property we may have to adjust our cache
        if (m_pPropertySetCache && evt.PropertyName == FM_PROP_CONTROLSOURCE)
        {
            Reference< XPropertySet >  xSet(evt.Source, UNO_QUERY);
            PropertySetInfo& rSetInfo = (*m_pPropertySetCache)[xSet];
            rSetInfo.bHasEmptyControlSource = !evt.NewValue.hasValue() || ::comphelper::getString(evt.NewValue).isEmpty();
        }
    }
}

// XContainerListener

void SAL_CALL FmXUndoEnvironment::elementInserted(const ContainerEvent& evt)
{
    SolarMutexGuard aSolarGuard;
    ::osl::MutexGuard aGuard( m_aMutex );

    // new object for listening
    Reference< XInterface >  xIface;
    evt.Element >>= xIface;
    OSL_ENSURE(xIface.is(), "FmXUndoEnvironment::elementInserted: invalid container notification!");
    AddElement(xIface);

    implSetModified();
}


void FmXUndoEnvironment::implSetModified()
{
    if ( !IsLocked() && rModel.GetObjectShell() )
    {
        rModel.GetObjectShell()->SetModified();
    }
}


void SAL_CALL FmXUndoEnvironment::elementReplaced(const ContainerEvent& evt)
{
    SolarMutexGuard aSolarGuard;
    ::osl::MutexGuard aGuard( m_aMutex );

    Reference< XInterface >  xIface;
    evt.ReplacedElement >>= xIface;
    OSL_ENSURE(xIface.is(), "FmXUndoEnvironment::elementReplaced: invalid container notification!");
    RemoveElement(xIface);

    evt.Element >>= xIface;
    AddElement(xIface);

    implSetModified();
}


void SAL_CALL FmXUndoEnvironment::elementRemoved(const ContainerEvent& evt)
{
    SolarMutexGuard aSolarGuard;
    ::osl::MutexGuard aGuard( m_aMutex );

    Reference< XInterface >  xIface( evt.Element, UNO_QUERY );
    OSL_ENSURE(xIface.is(), "FmXUndoEnvironment::elementRemoved: invalid container notification!");
    RemoveElement(xIface);

    implSetModified();
}


void SAL_CALL FmXUndoEnvironment::modified( const EventObject& /*aEvent*/ )
{
    implSetModified();
}


void FmXUndoEnvironment::AddForms(const Reference< XNameContainer > & rForms)
{
    Lock();
    AddElement(Reference<XInterface>( rForms, UNO_QUERY ));
    UnLock();
}


void FmXUndoEnvironment::RemoveForms(const Reference< XNameContainer > & rForms)
{
    Lock();
    RemoveElement(Reference<XInterface>( rForms, UNO_QUERY ));
    UnLock();
}


void FmXUndoEnvironment::TogglePropertyListening(const Reference< XInterface > & Element)
{
    // listen at the container
    Reference< XIndexContainer >  xContainer(Element, UNO_QUERY);
    if (xContainer.is())
    {
        sal_uInt32 nCount = xContainer->getCount();
        Reference< XInterface >  xIface;
        for (sal_uInt32 i = 0; i < nCount; i++)
        {
            xContainer->getByIndex(i) >>= xIface;
            TogglePropertyListening(xIface);
        }
    }

    Reference< XPropertySet >  xSet(Element, UNO_QUERY);
    if (xSet.is())
    {
        if (!bReadOnly)
            xSet->addPropertyChangeListener( OUString(), this );
        else
            xSet->removePropertyChangeListener( OUString(), this );
    }
}


void FmXUndoEnvironment::switchListening( const Reference< XIndexContainer >& _rxContainer, bool _bStartListening )
{
    OSL_PRECOND( _rxContainer.is(), "FmXUndoEnvironment::switchListening: invalid container!" );
    if ( !_rxContainer.is() )
        return;

    try
    {
        // if it's an EventAttacherManager, then we need to listen for
        // script events
        Reference< XEventAttacherManager > xManager( _rxContainer, UNO_QUERY );
        if ( xManager.is() )
        {
            if ( _bStartListening )
            {
                m_pScriptingEnv->registerEventAttacherManager( xManager );
                if ( m_vbaListener.is() )
                    xManager->addScriptListener( m_vbaListener );
            }
            else
            {
                m_pScriptingEnv->revokeEventAttacherManager( xManager );
                if ( m_vbaListener.is() )
                    xManager->removeScriptListener( m_vbaListener );
            }
        }

        // also handle all children of this element
        sal_uInt32 nCount = _rxContainer->getCount();
        Reference< XInterface > xInterface;
        for ( sal_uInt32 i = 0; i < nCount; ++i )
        {
            _rxContainer->getByIndex( i ) >>= xInterface;
            if ( _bStartListening )
                AddElement( xInterface );
            else
                RemoveElement( xInterface );
        }

        // be notified of any changes in the container elements
        Reference< XContainer > xSimpleContainer( _rxContainer, UNO_QUERY );
        OSL_ENSURE( xSimpleContainer.is(), "FmXUndoEnvironment::switchListening: how are we expected to be notified of changes in the container?" );
        if ( xSimpleContainer.is() )
        {
            if ( _bStartListening )
                xSimpleContainer->addContainerListener( this );
            else
                xSimpleContainer->removeContainerListener( this );
        }
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "svx", "FmXUndoEnvironment::switchListening" );
    }
}


void FmXUndoEnvironment::switchListening( const Reference< XInterface >& _rxObject, bool _bStartListening )
{
    OSL_PRECOND( _rxObject.is(), "FmXUndoEnvironment::switchListening: how should I listen at a NULL object?" );

    try
    {
        if ( !bReadOnly )
        {
            Reference< XPropertySet > xProps( _rxObject, UNO_QUERY );
            if ( xProps.is() )
            {
                if ( _bStartListening )
                    xProps->addPropertyChangeListener( OUString(), this );
                else
                    xProps->removePropertyChangeListener( OUString(), this );
            }
        }

        Reference< XModifyBroadcaster > xBroadcaster( _rxObject, UNO_QUERY );
        if ( xBroadcaster.is() )
        {
            if ( _bStartListening )
                xBroadcaster->addModifyListener( this );
            else
                xBroadcaster->removeModifyListener( this );
        }
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "svx", "FmXUndoEnvironment::switchListening" );
    }
}


void FmXUndoEnvironment::AddElement(const Reference< XInterface >& _rxElement )
{
    OSL_ENSURE( !m_bDisposed, "FmXUndoEnvironment::AddElement: not when I'm already disposed!" );

    // listen at the container
    Reference< XIndexContainer > xContainer( _rxElement, UNO_QUERY );
    if ( xContainer.is() )
        switchListening( xContainer, true );

    switchListening( _rxElement, true );
}


void FmXUndoEnvironment::RemoveElement(const Reference< XInterface >& _rxElement)
{
    if ( m_bDisposed )
        return;

    switchListening( _rxElement, false );

    if (!bReadOnly)
    {
        // reset the ActiveConnection if the form is to be removed. This will (should) free the resources
        // associated with this connection
        // 86299 - 05/02/2001 - frank.schoenheit@germany.sun.com
        Reference< XForm > xForm( _rxElement, UNO_QUERY );
        Reference< XPropertySet > xFormProperties( xForm, UNO_QUERY );
        if ( xFormProperties.is() )
        {
            Reference< XConnection > xDummy;
            if ( !isEmbeddedInDatabase( _rxElement, xDummy ) )
                // (if there is a connection in the context of the component, setting
                // a new connection would be vetoed, anyway)
                // #i34196#
                xFormProperties->setPropertyValue( FM_PROP_ACTIVE_CONNECTION, Any() );
        }
    }

    Reference< XIndexContainer > xContainer( _rxElement, UNO_QUERY );
    if ( xContainer.is() )
        switchListening( xContainer, false );
}


FmUndoPropertyAction::FmUndoPropertyAction(FmFormModel& rNewMod, const PropertyChangeEvent& evt)
                     :SdrUndoAction(rNewMod)
                     ,xObj(evt.Source, UNO_QUERY)
                     ,aPropertyName(evt.PropertyName)
                     ,aNewValue(evt.NewValue)
                     ,aOldValue(evt.OldValue)
{
    if (rNewMod.GetObjectShell())
        rNewMod.GetObjectShell()->SetModified();
    if(static_STR_UNDO_PROPERTY.isEmpty())
        static_STR_UNDO_PROPERTY = SvxResId(RID_STR_UNDO_PROPERTY);
}


void FmUndoPropertyAction::Undo()
{
    FmXUndoEnvironment& rEnv = static_cast<FmFormModel&>(m_rMod).GetUndoEnv();

    if (!xObj.is() || rEnv.IsLocked())
        return;

    rEnv.Lock();
    try
    {
        xObj->setPropertyValue( aPropertyName, aOldValue );
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "svx", "FmUndoPropertyAction::Undo" );
    }
    rEnv.UnLock();
}


void FmUndoPropertyAction::Redo()
{
    FmXUndoEnvironment& rEnv = static_cast<FmFormModel&>(m_rMod).GetUndoEnv();

    if (!xObj.is() || rEnv.IsLocked())
        return;

    rEnv.Lock();
    try
    {
        xObj->setPropertyValue( aPropertyName, aNewValue );
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "svx", "FmUndoPropertyAction::Redo" );
    }
    rEnv.UnLock();
}


OUString FmUndoPropertyAction::GetComment() const
{
    OUString aStr = static_STR_UNDO_PROPERTY.replaceFirst( "#", aPropertyName );
    return aStr;
}


FmUndoContainerAction::FmUndoContainerAction(FmFormModel& _rMod,
                                             Action _eAction,
                                             const Reference< XIndexContainer > & xCont,
                                             const Reference< XInterface > & xElem,
                                             sal_Int32 nIdx)
                      :SdrUndoAction( _rMod )
                      ,m_xContainer( xCont )
                      ,m_nIndex( nIdx )
                      ,m_eAction( _eAction )
{
    OSL_ENSURE( nIdx >= 0, "FmUndoContainerAction::FmUndoContainerAction: invalid index!" );
        // some old code suggested this could be a valid argument. However, this code was
        // buggy, and it *seemed* that nobody used it - so it was removed.

    if ( !(xCont.is() && xElem.is()) )
        return;

    // normalize
    m_xElement = xElem;
    if ( m_eAction != Removed )
        return;

    if (m_nIndex >= 0)
    {
        Reference< XEventAttacherManager >  xManager( xCont, UNO_QUERY );
        if ( xManager.is() )
            m_aEvents = xManager->getScriptEvents(m_nIndex);
    }
    else
        m_xElement = nullptr;

    // we now own the element
    m_xOwnElement = m_xElement;
}


FmUndoContainerAction::~FmUndoContainerAction()
{
    // if we own the object...
    DisposeElement( m_xOwnElement );
}


void FmUndoContainerAction::DisposeElement( const Reference< XInterface > & xElem )
{
    Reference< XComponent > xComp( xElem, UNO_QUERY );
    if ( xComp.is() )
    {
        // and the object does not have a parent
        Reference< XChild >  xChild( xElem, UNO_QUERY );
        if ( xChild.is() && !xChild->getParent().is() )
            // -> dispose it
            xComp->dispose();
    }
}


void FmUndoContainerAction::implReInsert( )
{
    if ( m_xContainer->getCount() < m_nIndex )
        return;

    // insert the element
    Any aVal;
    if ( m_xContainer->getElementType() == cppu::UnoType<XFormComponent>::get() )
    {
        aVal <<= Reference< XFormComponent >( m_xElement, UNO_QUERY );
    }
    else
    {
        aVal <<= Reference< XForm >( m_xElement, UNO_QUERY );
    }
    m_xContainer->insertByIndex( m_nIndex, aVal );

    OSL_ENSURE( getElementPos( m_xContainer, m_xElement ) == m_nIndex, "FmUndoContainerAction::implReInsert: insertion did not work!" );

    // register the events
    Reference< XEventAttacherManager >  xManager( m_xContainer, UNO_QUERY );
    if ( xManager.is() )
        xManager->registerScriptEvents( m_nIndex, m_aEvents );

    // we don't own the object anymore
    m_xOwnElement = nullptr;
}


void FmUndoContainerAction::implReRemove( )
{
    Reference< XInterface > xElement;
    if ( ( m_nIndex >= 0 ) && ( m_nIndex < m_xContainer->getCount() ) )
        m_xContainer->getByIndex( m_nIndex ) >>= xElement;

    if ( xElement != m_xElement )
    {
        // the indexes in the container changed. Okay, so go the long way and
        // manually determine the index
        m_nIndex = getElementPos( m_xContainer, m_xElement );
        if ( m_nIndex != -1 )
            xElement = m_xElement;
    }

    OSL_ENSURE( xElement == m_xElement, "FmUndoContainerAction::implReRemove: cannot find the element which I'm responsible for!" );
    if ( xElement == m_xElement )
    {
        Reference< XEventAttacherManager >  xManager( m_xContainer, UNO_QUERY );
        if ( xManager.is() )
            m_aEvents = xManager->getScriptEvents( m_nIndex );
        m_xContainer->removeByIndex( m_nIndex );
        // from now on, we own this object
        m_xOwnElement = m_xElement;
    }
}


void FmUndoContainerAction::Undo()
{
    FmXUndoEnvironment& rEnv = static_cast< FmFormModel& >( m_rMod ).GetUndoEnv();

    if ( !(m_xContainer.is() && !rEnv.IsLocked() && m_xElement.is()) )
        return;

    rEnv.Lock();
    try
    {
        switch ( m_eAction )
        {
        case Inserted:
            implReRemove();
            break;

        case Removed:
            implReInsert();
            break;
        }
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "svx", "FmUndoContainerAction::Undo" );
    }
    rEnv.UnLock();
}


void FmUndoContainerAction::Redo()
{
    FmXUndoEnvironment& rEnv = static_cast< FmFormModel& >( m_rMod ).GetUndoEnv();
    if ( !(m_xContainer.is() && !rEnv.IsLocked() && m_xElement.is()) )
        return;

    rEnv.Lock();
    try
    {
        switch ( m_eAction )
        {
        case Inserted:
            implReInsert();
            break;

        case Removed:
            implReRemove();
            break;
        }
    }
    catch( const Exception& )
    {
        TOOLS_WARN_EXCEPTION( "svx", "FmUndoContainerAction::Redo" );
    }
    rEnv.UnLock();
}


FmUndoModelReplaceAction::FmUndoModelReplaceAction(FmFormModel& _rMod, SdrUnoObj* _pObject, const Reference< XControlModel > & _xReplaced)
    :SdrUndoAction(_rMod)
    ,m_xReplaced(_xReplaced)
    ,m_pObject(_pObject)
{
}


FmUndoModelReplaceAction::~FmUndoModelReplaceAction()
{
    // dispose our element if nobody else is responsible for
    DisposeElement(m_xReplaced);
}


void FmUndoModelReplaceAction::DisposeElement( const css::uno::Reference< css::awt::XControlModel>& xReplaced )
{
    Reference< XComponent >  xComp(xReplaced, UNO_QUERY);
    if (xComp.is())
    {
        Reference< XChild >  xChild(xReplaced, UNO_QUERY);
        if (!xChild.is() || !xChild->getParent().is())
            xComp->dispose();
    }
}


void FmUndoModelReplaceAction::Undo()
{
    try
    {
        Reference< XControlModel > xCurrentModel( m_pObject->GetUnoControlModel() );

        // replace the model within the parent
        Reference< XChild > xCurrentAsChild( xCurrentModel, UNO_QUERY );
        Reference< XNameContainer > xCurrentsParent;
        if ( xCurrentAsChild.is() )
            xCurrentsParent.set(xCurrentAsChild->getParent(), css::uno::UNO_QUERY);
        DBG_ASSERT( xCurrentsParent.is(), "FmUndoModelReplaceAction::Undo: invalid current model!" );

        if ( xCurrentsParent.is() )
        {
            // the form container works with FormComponents
            Reference< XFormComponent > xComponent( m_xReplaced, UNO_QUERY );
            DBG_ASSERT( xComponent.is(), "FmUndoModelReplaceAction::Undo: the new model is no form component !" );

            Reference< XPropertySet > xCurrentAsSet( xCurrentModel, UNO_QUERY );
            DBG_ASSERT( ::comphelper::hasProperty(FM_PROP_NAME, xCurrentAsSet ), "FmUndoModelReplaceAction::Undo : one of the models is invalid !");

            OUString sName;
            xCurrentAsSet->getPropertyValue( FM_PROP_NAME ) >>= sName;
            xCurrentsParent->replaceByName( sName, Any( xComponent ) );

            m_pObject->SetUnoControlModel(m_xReplaced);
            m_pObject->SetChanged();

            m_xReplaced = std::move(xCurrentModel);
        }
    }
    catch(Exception&)
    {
        OSL_FAIL("FmUndoModelReplaceAction::Undo : could not replace the model !");
    }
}


OUString FmUndoModelReplaceAction::GetComment() const
{
    return SvxResId(RID_STR_UNDO_MODEL_REPLACE);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
