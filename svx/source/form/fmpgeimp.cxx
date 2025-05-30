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


#include <fmpgeimp.hxx>
#include <fmundo.hxx>
#include <svx/fmtools.hxx>
#include <fmprop.hxx>
#include <fmservs.hxx>
#include <fmobj.hxx>
#include <formcontrolfactory.hxx>
#include <svx/svditer.hxx>
#include <svx/strings.hrc>
#include <treevisitor.hxx>

#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/sdbc/XRowSet.hpp>
#include <com/sun/star/container/EnumerableMap.hpp>
#include <com/sun/star/drawing/XControlShape.hpp>
#include <com/sun/star/form/Forms.hpp>
#include <com/sun/star/form/FormComponentType.hpp>

#include <sal/log.hxx>
#include <sfx2/objsh.hxx>
#include <svx/fmpage.hxx>
#include <svx/fmmodel.hxx>
#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <svx/dialmgr.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/types.hxx>
#include <connectivity/dbtools.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::form;
using ::com::sun::star::awt::XControlModel;
using ::com::sun::star::container::XMap;
using ::com::sun::star::container::EnumerableMap;
using ::com::sun::star::drawing::XControlShape;
using namespace ::svxform;
using namespace ::dbtools;


FmFormPageImpl::FmFormPageImpl( FmFormPage& _rPage )
               :m_rPage( _rPage )
               ,m_bFirstActivation( true )
               ,m_bAttemptedFormCreation( false )
{
}


namespace
{
    class FormComponentInfo
    {
    public:
        static size_t childCount( const Reference< XInterface >& _component )
        {
            Reference< XIndexAccess > xContainer( _component, UNO_QUERY );
            if ( xContainer.is() )
                return xContainer->getCount();
            return 0;
        }

        static Reference< XInterface > getChild( const Reference< XInterface >& _component, size_t _index )
        {
            Reference< XIndexAccess > xContainer( _component, UNO_QUERY_THROW );
            return Reference< XInterface >( xContainer->getByIndex( _index ), UNO_QUERY );
        }
    };

    typedef ::std::pair< Reference< XInterface >, Reference< XInterface > > FormComponentPair;

    class FormHierarchyComparator
    {
    public:
        FormHierarchyComparator()
        {
        }

        static size_t childCount( const FormComponentPair& _components )
        {
            size_t lhsCount = FormComponentInfo::childCount( _components.first );
            size_t rhsCount = FormComponentInfo::childCount( _components.second );
            if  ( lhsCount != rhsCount )
                throw RuntimeException( u"Found inconsistent form component hierarchies (1)!"_ustr );
            return lhsCount;
        }

        static FormComponentPair getChild( const FormComponentPair& _components, size_t _index )
        {
            return FormComponentPair(
                FormComponentInfo::getChild( _components.first, _index ),
                FormComponentInfo::getChild( _components.second, _index )
            );
        }
    };

    typedef ::std::map< Reference< XControlModel >, Reference< XControlModel > > MapControlModels;

    class FormComponentAssignment
    {
    public:
        explicit FormComponentAssignment( MapControlModels& _out_controlModelMap )
            :m_rControlModelMap( _out_controlModelMap )
        {
        }

        void    process( const FormComponentPair& _component )
        {
            Reference< XControlModel > lhsControlModel( _component.first, UNO_QUERY );
            Reference< XControlModel > rhsControlModel( _component.second, UNO_QUERY );
            if ( lhsControlModel.is() != rhsControlModel.is() )
                throw RuntimeException( u"Found inconsistent form component hierarchies (2)!"_ustr );

            if ( lhsControlModel.is() )
                m_rControlModelMap[ lhsControlModel ] = std::move(rhsControlModel);
        }

    private:
        MapControlModels&   m_rControlModelMap;
    };
}


void FmFormPageImpl::initFrom( FmFormPageImpl& i_foreignImpl )
{
    // clone the Forms collection
    const Reference< css::form::XForms > xForeignForms( i_foreignImpl.getForms( false ) );

    if ( !xForeignForms.is() )
        return;

    try
    {
        m_xForms.set( xForeignForms->createClone(), UNO_QUERY_THROW );

        // create a mapping between the original control models and their clones
        MapControlModels aModelAssignment;

        typedef TreeVisitor< FormComponentPair, FormHierarchyComparator, FormComponentAssignment >   FormComponentVisitor;
        FormComponentVisitor aVisitor{ FormHierarchyComparator() };

        FormComponentAssignment aAssignmentProcessor( aModelAssignment );
        aVisitor.process( FormComponentPair( xForeignForms, m_xForms ), aAssignmentProcessor );

        // assign the cloned models to their SdrObjects
        SdrObjListIter aForeignIter( &i_foreignImpl.m_rPage );
        SdrObjListIter aOwnIter( &m_rPage );

        OSL_ENSURE( aForeignIter.IsMore() == aOwnIter.IsMore(), "FmFormPageImpl::FmFormPageImpl: inconsistent number of objects (1)!" );
        while ( aForeignIter.IsMore() && aOwnIter.IsMore() )
        {
            FmFormObj* pForeignObj = dynamic_cast< FmFormObj* >( aForeignIter.Next() );
            FmFormObj* pOwnObj = dynamic_cast< FmFormObj* >( aOwnIter.Next() );

            bool bForeignIsForm = pForeignObj && ( pForeignObj->GetObjInventor() == SdrInventor::FmForm );
            bool bOwnIsForm = pOwnObj && ( pOwnObj->GetObjInventor() == SdrInventor::FmForm );

            if ( bForeignIsForm != bOwnIsForm )
            {
                // if this fires, don't attempt to do further assignments, something's completely messed up
                SAL_WARN( "svx.form", "FmFormPageImpl::FmFormPageImpl: inconsistent ordering of objects!" );
                break;
            }

            if ( !bForeignIsForm )
                // no form control -> next round
                continue;

            Reference< XControlModel > xForeignModel( pForeignObj->GetUnoControlModel() );
            if ( !xForeignModel.is() )
            {
                // if this fires, the SdrObject does not have a UNO Control Model. This is pathological, but well ...
                // So the cloned SdrObject will also not have a UNO Control Model.
                SAL_WARN( "svx.form", "FmFormPageImpl::FmFormPageImpl: control shape without control!" );
                continue;
            }

            MapControlModels::const_iterator assignment = aModelAssignment.find( xForeignModel );
            if ( assignment == aModelAssignment.end() )
            {
                // if this fires, the source SdrObject has a model, but it is not part of the model hierarchy in
                // i_foreignImpl.getForms().
                // Pathological, too ...
                SAL_WARN( "svx.form", "FmFormPageImpl::FmFormPageImpl: no clone found for this model!" );
                continue;
            }

            pOwnObj->SetUnoControlModel( assignment->second );
        }
        OSL_ENSURE( aForeignIter.IsMore() == aOwnIter.IsMore(), "FmFormPageImpl::FmFormPageImpl: inconsistent number of objects (2)!" );
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("svx");
    }
}


Reference< XMap > FmFormPageImpl::getControlToShapeMap()
{
    Reference< XMap > xControlShapeMap( m_aControlShapeMap.get(), UNO_QUERY );
    if ( xControlShapeMap.is() )
        return xControlShapeMap;

    xControlShapeMap = impl_createControlShapeMap_nothrow();
    m_aControlShapeMap = xControlShapeMap;
    return xControlShapeMap;
}


namespace
{
    void lcl_insertFormObject_throw( const FmFormObj& _object, const Reference< XMap >& _map )
    {
        // the control model
        const Reference< XControlModel >& xControlModel = _object.GetUnoControlModel();
        OSL_ENSURE( xControlModel.is(), "lcl_insertFormObject_throw: suspicious: no control model!" );
        if ( !xControlModel.is() )
            return;

        Reference< XControlShape > xControlShape( const_cast< FmFormObj& >( _object ).getUnoShape(), UNO_QUERY );
        OSL_ENSURE( xControlShape.is(), "lcl_insertFormObject_throw: suspicious: no control shape!" );
        if ( !xControlShape.is() )
            return;

        _map->put( Any( xControlModel ), Any( xControlShape ) );
    }

    void lcl_removeFormObject_throw( const FmFormObj& _object, const Reference< XMap >& _map )
    {
        // the control model
        const Reference< XControlModel >& xControlModel = _object.GetUnoControlModel();
        OSL_ENSURE( xControlModel.is(), "lcl_removeFormObject: suspicious: no control model!" );
        if ( !xControlModel.is() )
        {
            return;
        }

        Any aOldAssignment = _map->remove( Any( xControlModel ) );
        OSL_ENSURE(
            aOldAssignment == Any( Reference< XControlShape >( const_cast< FmFormObj& >( _object ).getUnoShape(), UNO_QUERY ) ),
                "lcl_removeFormObject: map was inconsistent!" );
    }
}


Reference< XMap > FmFormPageImpl::impl_createControlShapeMap_nothrow()
{
    Reference< XMap > xMap;

    try
    {
        xMap = EnumerableMap::create( comphelper::getProcessComponentContext(),
            ::cppu::UnoType< XControlModel >::get(),
            ::cppu::UnoType< XControlShape >::get()
        );

        SdrObjListIter aPageIter( &m_rPage );
        while ( aPageIter.IsMore() )
        {
            // only FmFormObjs are what we're interested in
            FmFormObj* pCurrent = FmFormObj::GetFormObject( aPageIter.Next() );
            if ( !pCurrent )
                continue;

            lcl_insertFormObject_throw( *pCurrent, xMap );
        }
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("svx");
    }
    return xMap;
}


const Reference< css::form::XForms >& FmFormPageImpl::getForms( bool _bForceCreate )
{
    if ( m_xForms.is() || !_bForceCreate )
        return m_xForms;

    if ( !m_bAttemptedFormCreation )
    {
        m_bAttemptedFormCreation = true;

        const Reference<XComponentContext>& xContext = comphelper::getProcessComponentContext();
        m_xForms = css::form::Forms::create( xContext );

        if ( m_aFormsCreationHdl.IsSet() )
        {
            m_aFormsCreationHdl.Call( *this );
        }

        FmFormModel& rFmFormModel(dynamic_cast< FmFormModel& >(m_rPage.getSdrModelFromSdrPage()));

        // give the newly created collection a place in the universe
        SfxObjectShell* pObjShell(rFmFormModel.GetObjectShell());
        if ( pObjShell )
            m_xForms->setParent( pObjShell->GetModel() );

        // tell the UNDO environment that we have a new forms collection
        rFmFormModel.GetUndoEnv().AddForms( Reference<XNameContainer>(m_xForms,UNO_QUERY_THROW) );
    }
    return m_xForms;
}


FmFormPageImpl::~FmFormPageImpl()
{
    xCurrentForm = nullptr;

    ::comphelper::disposeComponent( m_xForms );
}


bool FmFormPageImpl::validateCurForm()
{
    if ( !xCurrentForm.is() )
        return false;

    if ( !xCurrentForm->getParent().is() )
        xCurrentForm.clear();

    return xCurrentForm.is();
}


void FmFormPageImpl::setCurForm(const Reference< css::form::XForm >&  xForm)
{
    xCurrentForm = xForm;
}


Reference< XForm >  FmFormPageImpl::getDefaultForm()
{
    Reference< XForm > xForm;

    Reference< XForms > xForms( getForms() );

    // by default, we use our "current form"
    if ( !validateCurForm() )
    {
        // check whether there is a "standard" form
        if ( Reference<XNameAccess>(xForms,UNO_QUERY_THROW)->hasElements() )
        {
            // find the standard form
            OUString sStandardFormname = SvxResId(RID_STR_STDFORMNAME);

            try
            {
                if ( xForms->hasByName( sStandardFormname ) )
                    xForm.set( xForms->getByName( sStandardFormname ), UNO_QUERY_THROW );
                else
                {
                    xForm.set( xForms->getByIndex(0), UNO_QUERY_THROW );
                }
            }
            catch( const Exception& )
            {
                DBG_UNHANDLED_EXCEPTION("svx");
            }
        }
    }
    else
    {
        xForm = xCurrentForm;
    }

    // did not find an existing suitable form -> create a new one
    if ( !xForm.is() )
    {
        SdrModel& rModel(m_rPage.getSdrModelFromSdrPage());

        if( rModel.IsUndoEnabled() )
        {
            OUString aStr(SvxResId(RID_STR_FORM));
            OUString aUndoStr(SvxResId(RID_STR_UNDO_CONTAINER_INSERT));
            rModel.BegUndo(aUndoStr.replaceFirst("'#'", aStr));
        }

        try
        {
            xForm.set( ::comphelper::getProcessServiceFactory()->createInstance( FM_SUN_COMPONENT_FORM ), UNO_QUERY );

            // a form should always have the command type table as default
            Reference< XPropertySet > xFormProps( xForm, UNO_QUERY_THROW );
            xFormProps->setPropertyValue( FM_PROP_COMMANDTYPE, Any( sal_Int32( CommandType::TABLE ) ) );

            // and the "Standard" name
            OUString sName = SvxResId(RID_STR_STDFORMNAME);
            xFormProps->setPropertyValue( FM_PROP_NAME, Any( sName ) );

            if( rModel.IsUndoEnabled() )
            {
                rModel.AddUndo(
                    std::make_unique<FmUndoContainerAction>(
                        static_cast< FmFormModel& >(rModel),
                        FmUndoContainerAction::Inserted,
                        xForms,
                        xForm,
                        xForms->getCount()));
            }
            xForms->insertByName( sName, Any( xForm ) );
            xCurrentForm = xForm;
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("svx");
            xForm.clear();
        }

        if( rModel.IsUndoEnabled() )
            rModel.EndUndo();
    }

    return xForm;
}


Reference< css::form::XForm >  FmFormPageImpl::findPlaceInFormComponentHierarchy(
    const Reference< XFormComponent > & rContent, const Reference< XDataSource > & rDatabase,
    const OUString& rDBTitle, const OUString& rCursorSource, sal_Int32 nCommandType )
{
    // if the control already is child of a form, don't do anything
    if (!rContent.is() || rContent->getParent().is())
        return nullptr;

    Reference< XForm >  xForm;

    // If database and CursorSource are set, the form is searched for using
    // these criteria, otherwise only current and the DefaultForm.
    if (rDatabase.is() && !rCursorSource.isEmpty())
    {
        validateCurForm();

        // first search in the current form
        xForm = findFormForDataSource( xCurrentForm, rDatabase, rCursorSource, nCommandType );

        Reference< css::container::XIndexAccess >  xFormsByIndex = getForms();
        DBG_ASSERT(xFormsByIndex.is(), "FmFormPageImpl::findPlaceInFormComponentHierarchy : no index access for my forms collection !");
        sal_Int32 nCount = xFormsByIndex->getCount();
        for (sal_Int32 i = 0; !xForm.is() && i < nCount; i++)
        {
            Reference< css::form::XForm >  xToSearch;
            xFormsByIndex->getByIndex(i) >>= xToSearch;
            xForm = findFormForDataSource( xToSearch, rDatabase, rCursorSource, nCommandType );
        }

        // If no css::form found, then create a new one
        if (!xForm.is())
        {
            SdrModel& rModel(m_rPage.getSdrModelFromSdrPage());
            const bool bUndo(rModel.IsUndoEnabled());

            if( bUndo )
            {
                OUString aStr(SvxResId(RID_STR_FORM));
                OUString aUndoStr(SvxResId(RID_STR_UNDO_CONTAINER_INSERT));
                aUndoStr = aUndoStr.replaceFirst("#", aStr);
                rModel.BegUndo(aUndoStr);
            }

            xForm.set(::comphelper::getProcessServiceFactory()->createInstance(FM_SUN_COMPONENT_FORM), UNO_QUERY);
            // a form should always have the command type table as default
            Reference< css::beans::XPropertySet > xFormProps(xForm, UNO_QUERY);
            try { xFormProps->setPropertyValue(FM_PROP_COMMANDTYPE, Any(sal_Int32(CommandType::TABLE))); }
            catch(Exception&) { }

            if (!rDBTitle.isEmpty())
                xFormProps->setPropertyValue(FM_PROP_DATASOURCE,Any(rDBTitle));
            else
            {
                Reference< css::beans::XPropertySet >  xDatabaseProps(rDatabase, UNO_QUERY);
                Any aDatabaseUrl = xDatabaseProps->getPropertyValue(FM_PROP_URL);
                xFormProps->setPropertyValue(FM_PROP_URL, aDatabaseUrl);
            }

            xFormProps->setPropertyValue(FM_PROP_COMMAND,Any(rCursorSource));
            xFormProps->setPropertyValue(FM_PROP_COMMANDTYPE, Any(nCommandType));

            Reference< css::container::XNameAccess >  xNamedSet = getForms();

            const bool bTableOrQuery = ( CommandType::TABLE == nCommandType ) || ( CommandType::QUERY == nCommandType );
            OUString sName = FormControlFactory::getUniqueName( xNamedSet,
                bTableOrQuery ? rCursorSource : SvxResId(RID_STR_STDFORMNAME) );

            xFormProps->setPropertyValue( FM_PROP_NAME, Any( sName ) );

            if( bUndo )
            {
                Reference< css::container::XIndexContainer >  xContainer = getForms();
                rModel.AddUndo(
                    std::make_unique<FmUndoContainerAction>(
                        static_cast< FmFormModel& >(rModel),
                        FmUndoContainerAction::Inserted,
                        xContainer,
                        xForm,
                        xContainer->getCount()));
            }

            getForms()->insertByName( sName, Any( xForm ) );

            if( bUndo )
                rModel.EndUndo();
        }
        xCurrentForm = xForm;
    }

    xForm = getDefaultForm();
    return xForm;
}


Reference< XForm >  FmFormPageImpl::findFormForDataSource(
        const Reference< XForm > & rForm, const Reference< XDataSource > & _rxDatabase,
        const OUString& _rCursorSource, sal_Int32 nCommandType)
{
    Reference< XForm >          xResultForm;
    Reference< XRowSet >        xDBForm(rForm, UNO_QUERY);
    Reference< XPropertySet >   xFormProps(rForm, UNO_QUERY);
    if (!xDBForm.is() || !xFormProps.is())
        return xResultForm;

    OSL_ENSURE(_rxDatabase.is(), "FmFormPageImpl::findFormForDataSource: invalid data source!");
    OUString sLookupName;            // the name of the data source we're looking for
    OUString sFormDataSourceName;    // the name of the data source the current connection in the form is based on
    try
    {
        Reference< XPropertySet > xDSProps(_rxDatabase, UNO_QUERY);
        if (xDSProps.is())
            xDSProps->getPropertyValue(FM_PROP_NAME) >>= sLookupName;

        xFormProps->getPropertyValue(FM_PROP_DATASOURCE) >>= sFormDataSourceName;
        // if there's no DataSourceName set at the form, check whether we can deduce one from its
        // ActiveConnection
        if (sFormDataSourceName.isEmpty())
        {
            Reference< XConnection > xFormConnection;
            xFormProps->getPropertyValue( FM_PROP_ACTIVE_CONNECTION ) >>= xFormConnection;
            if ( !xFormConnection.is() )
                isEmbeddedInDatabase( xFormProps, xFormConnection );
            if (xFormConnection.is())
            {
                Reference< XChild > xConnAsChild(xFormConnection, UNO_QUERY);
                if (xConnAsChild.is())
                {
                    Reference< XDataSource > xFormDS(xConnAsChild->getParent(), UNO_QUERY);
                    if (xFormDS.is())
                    {
                        xDSProps.set(xFormDS, css::uno::UNO_QUERY);
                        if (xDSProps.is())
                            xDSProps->getPropertyValue(FM_PROP_NAME) >>= sFormDataSourceName;
                    }
                }
            }
        }
    }
    catch(const Exception&)
    {
        TOOLS_WARN_EXCEPTION("svx", "FmFormPageImpl::findFormForDataSource");
    }

    if (sLookupName == sFormDataSourceName)
    {
        // now check whether CursorSource and type match
        OUString aCursorSource = ::comphelper::getString(xFormProps->getPropertyValue(FM_PROP_COMMAND));
        sal_Int32 nType = ::comphelper::getINT32(xFormProps->getPropertyValue(FM_PROP_COMMANDTYPE));
        if (aCursorSource.isEmpty() || ((nType == nCommandType) && (aCursorSource == _rCursorSource))) // found the form
        {
            xResultForm = rForm;
            // if no data source is set yet, it is done here
            if (aCursorSource.isEmpty())
            {
                xFormProps->setPropertyValue(FM_PROP_COMMAND, Any(_rCursorSource));
                xFormProps->setPropertyValue(FM_PROP_COMMANDTYPE, Any(nCommandType));
            }
        }
    }

    // as long as xResultForm is NULL, search the child forms of rForm
    Reference< XIndexAccess >  xComponents(rForm, UNO_QUERY);
    sal_Int32 nCount = xComponents->getCount();
    for (sal_Int32 i = 0; !xResultForm.is() && i < nCount; ++i)
    {
        Reference< css::form::XForm >  xSearchForm;
        xComponents->getByIndex(i) >>= xSearchForm;
        // continue searching in the sub form
        if (xSearchForm.is())
            xResultForm = findFormForDataSource( xSearchForm, _rxDatabase, _rCursorSource, nCommandType );
    }
    return xResultForm;
}


OUString FmFormPageImpl::setUniqueName(const Reference< XFormComponent > & xFormComponent, const Reference< XForm > & xControls)
{
#if OSL_DEBUG_LEVEL > 0
    try
    {
        OSL_ENSURE( !xFormComponent->getParent().is(), "FmFormPageImpl::setUniqueName: to be called before insertion!" );
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("svx");
    }
#endif
    OUString sName;
    Reference< css::beans::XPropertySet >  xSet(xFormComponent, UNO_QUERY);
    if (xSet.is())
    {
        sName = ::comphelper::getString( xSet->getPropertyValue( FM_PROP_NAME ) );
        Reference< css::container::XNameAccess >  xNameAcc(xControls, UNO_QUERY);

        if (sName.isEmpty() || xNameAcc->hasByName(sName))
        {
            // set a default name via the ClassId
            sal_Int16 nClassId( FormComponentType::CONTROL );
            xSet->getPropertyValue( FM_PROP_CLASSID ) >>= nClassId;

            OUString sDefaultName = FormControlFactory::getDefaultUniqueName_ByComponentType(
                Reference< XNameAccess >( xControls, UNO_QUERY ), xSet );

            // do not overwrite the name of radio buttons that have it!
            if (sName.isEmpty() || nClassId != css::form::FormComponentType::RADIOBUTTON)
            {
                xSet->setPropertyValue(FM_PROP_NAME, Any(sDefaultName));
            }

            sName = sDefaultName;
        }
    }
    return sName;
}


void FmFormPageImpl::formModelAssigned( const FmFormObj& _object )
{
    Reference< XMap > xControlShapeMap( m_aControlShapeMap.get(), UNO_QUERY );
    if ( !xControlShapeMap.is() )
        // our map does not exist -> not interested in this event
        return;

    try
    {
        lcl_removeFormObject_throw( _object,  xControlShapeMap );
        lcl_insertFormObject_throw( _object,  xControlShapeMap );
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("svx");
    }
}


void FmFormPageImpl::formObjectInserted( const FmFormObj& _object )
{
    Reference< XMap > xControlShapeMap( m_aControlShapeMap.get(), UNO_QUERY );
    if ( !xControlShapeMap.is() )
        // our map does not exist -> not interested in this event
        return;

    try
    {
        lcl_insertFormObject_throw( _object,  xControlShapeMap );
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("svx");
    }
}


void FmFormPageImpl::formObjectRemoved( const FmFormObj& _object )
{
    Reference< XMap > xControlShapeMap( m_aControlShapeMap.get(), UNO_QUERY );
    if ( !xControlShapeMap.is() )
        // our map does not exist -> not interested in this event
        return;

    try
    {
        lcl_removeFormObject_throw( _object, xControlShapeMap );
    }
    catch( const Exception& )
    {
        DBG_UNHANDLED_EXCEPTION("svx");
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
