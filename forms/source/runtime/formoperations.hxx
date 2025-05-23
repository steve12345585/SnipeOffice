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

#include <com/sun/star/form/runtime/XFormOperations.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/form/XForm.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/form/XLoadable.hpp>
#include <com/sun/star/sdb/XSingleSelectQueryComposer.hpp>
#include <com/sun/star/util/XModifyListener.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/sdb/SQLFilterOperator.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/compbase.hxx>
#include <connectivity/dbtools.hxx>
#include <tools/long.hxx>
#include <unotools/resmgr.hxx>
#include <utility>

namespace frm
{


    //= FormOperations

    typedef ::cppu::WeakComponentImplHelper    <   css::form::runtime::XFormOperations
                                                ,   css::lang::XInitialization
                                                ,   css::lang::XServiceInfo
                                                ,   css::beans::XPropertyChangeListener
                                                ,   css::util::XModifyListener
                                                ,   css::sdbc::XRowSetListener
                                                >   FormOperations_Base;

    class FormOperations    :public ::cppu::BaseMutex
                            ,public FormOperations_Base
    {
    public:
        class MethodGuard;

    private:
        css::uno::Reference<css::uno::XComponentContext>                      m_xContext;
        css::uno::Reference< css::form::runtime::XFormController >            m_xController;
        css::uno::Reference< css::sdbc::XRowSet >                             m_xCursor;
        css::uno::Reference< css::sdbc::XResultSetUpdate >                    m_xUpdateCursor;
        css::uno::Reference< css::beans::XPropertySet >                       m_xCursorProperties;
        css::uno::Reference< css::form::XLoadable >                           m_xLoadableForm;
        css::uno::Reference< css::form::runtime::XFeatureInvalidation >       m_xFeatureInvalidation;
        mutable css::uno::Reference< css::sdb::XSingleSelectQueryComposer >   m_xParser;

        bool    m_bInitializedParser;
        bool    m_bActiveControlModified;
        bool    m_bConstructed;
    #ifdef DBG_UTIL
        mutable tools::Long
                m_nMethodNestingLevel;
    #endif

    public:
        explicit FormOperations( const css::uno::Reference< css::uno::XComponentContext >& _rxContext );

        struct MethodAccess { friend class MethodGuard; private: MethodAccess() { } };

        void enterMethod( MethodAccess ) const
        {
            m_aMutex.acquire();
            impl_checkDisposed_throw();
        #ifdef DBG_UTIL
            ++m_nMethodNestingLevel;
        #endif
        }

        void leaveMethod( MethodAccess ) const
        {
            m_aMutex.release();
        #ifdef DBG_UTIL
            --m_nMethodNestingLevel;
        #endif
        }

    protected:
        virtual ~FormOperations() override;

        // XInitialization
        virtual void SAL_CALL initialize( const css::uno::Sequence< css::uno::Any >& aArguments ) override;

        // XServiceInfo
        virtual OUString SAL_CALL getImplementationName(  ) override;
        virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
        virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

        // XFormOperations
        virtual css::uno::Reference< css::sdbc::XRowSet > SAL_CALL getCursor() override;
        virtual css::uno::Reference< css::sdbc::XResultSetUpdate > SAL_CALL getUpdateCursor() override;
        virtual css::uno::Reference< css::form::runtime::XFormController > SAL_CALL getController() override;
        virtual css::uno::Reference< css::form::runtime::XFeatureInvalidation > SAL_CALL getFeatureInvalidation() override;
        virtual void SAL_CALL setFeatureInvalidation(const css::uno::Reference< css::form::runtime::XFeatureInvalidation > & the_value) override;
        virtual css::form::runtime::FeatureState SAL_CALL getState(::sal_Int16 Feature) override;
        virtual sal_Bool SAL_CALL isEnabled(::sal_Int16 Feature) override;
        virtual void SAL_CALL execute(::sal_Int16 Feature) override;
        virtual void SAL_CALL executeWithArguments(::sal_Int16 Feature, const css::uno::Sequence< css::beans::NamedValue >& Arguments) override;
        virtual sal_Bool SAL_CALL commitCurrentRecord(sal_Bool & RecordInserted) override;
        virtual sal_Bool SAL_CALL commitCurrentControl() override;
        virtual sal_Bool SAL_CALL isInsertionRow() override;
        virtual sal_Bool SAL_CALL isModifiedRow() override;

        // XRowSetListener
        virtual void SAL_CALL cursorMoved( const css::lang::EventObject& event ) override;
        virtual void SAL_CALL rowChanged( const css::lang::EventObject& event ) override;
        virtual void SAL_CALL rowSetChanged( const css::lang::EventObject& event ) override;

        // XModifyListener
        virtual void SAL_CALL modified( const css::lang::EventObject& _rSource ) override;

        // XPropertyChangeListener
        virtual void SAL_CALL propertyChange( const css::beans::PropertyChangeEvent& evt ) override;

        // XEventListener
        virtual void SAL_CALL disposing( const css::lang::EventObject& Source ) override;

        // XComponent/OComponentHelper
        virtual void SAL_CALL disposing() override;

    private:
        // service constructors
        void    createWithFormController( const css::uno::Reference< css::form::runtime::XFormController >& _rxController );
        void    createWithForm( const css::uno::Reference< css::form::XForm >& _rxForm );

        /** checks whether the instance is already disposed, and throws an exception if so
        */
        void        impl_checkDisposed_throw() const;

        /** initializes the instance after m_xController has been set
            @precond
                m_xController is not <NULL/>
        */
        void        impl_initFromController_throw();

        /** initializes the instance after m_xCursor has been set
            @precond
                m_xCursor is not <NULL/>
        */
        void        impl_initFromForm_throw();

        /// invalidate the full palette of features which we know
        void        impl_invalidateAllSupportedFeatures_nothrow( MethodGuard& _rClearForCallback ) const;

        /** invalidate the features which depend on the "modified" state of the current control
            of our controller
        */
        void        impl_invalidateModifyDependentFeatures_nothrow( MethodGuard& _rClearForCallback ) const;

        /** ensures that our parse is initialized, or at least that we attempted to do so
            @precond
                we're not disposed
        */
        void        impl_ensureInitializedParser_nothrow();

        /// disposes our parser, if we have one
        void        impl_disposeParser_nothrow();

        /** determines whether our cursor can be moved left
            @precond hasCursor()
        */
        bool        impl_canMoveLeft_throw() const;

        /** determines whether our cursor can be moved right
            @precond hasCursor()
        */
        bool        impl_canMoveRight_throw() const;

        /// determines whether we're positioned on the insertion row
        bool        impl_isInsertionRow_throw() const;

        /// retrieves the RowCount property of the form
        sal_Int32   impl_getRowCount_throw() const;

        /// retrieves the RowCountFinal property of the form
        bool        impl_isRowCountFinal_throw() const;

        /// retrieves the IsModified property of the form
        bool        impl_isModifiedRow_throw() const;

        /// determines whether we can parse the query of our form
        bool        impl_isParseable_throw() const;

        /// determines if we have an active filter or order condition
        bool        impl_hasFilterOrOrder_throw() const;

        /// determines whether our form is in "insert-only" mode
        bool        impl_isInsertOnlyForm_throw() const;

        /** retrieves the column to which the current control of our controller is bound
            @precond
                m_xController.is()
        */
        css::uno::Reference< css::beans::XPropertySet >
                    impl_getCurrentBoundField_nothrow( ) const;

        /** returns the control model of the current control

            If the current control is a grid control, then the returned model is the
            model of the current <em>column</em> in the grid.

            @precond
                m_xController.is()
        */
        css::uno::Reference< css::awt::XControlModel >
                    impl_getCurrentControlModel_throw() const;

        /// determines if we have a valid cursor
        bool    impl_hasCursor_nothrow() const { return m_xCursorProperties.is(); }

        /** determines the model position from a grid control column's view position

            A grid control can have columns which are currently hidden, so the index of a
            column in the view is not necessarily the same as its index in the model.
        */
        static sal_Int32   impl_gridView2ModelPos_nothrow( const css::uno::Reference< css::container::XIndexAccess >& _rxColumns, sal_Int16 _nViewPos );

        /** moves our cursor one position to the left, caring for different possible
            cursor states.

            Before the movement is done, the current row is saved, if necessary.

            @precond
                canMoveLeft()
        */
        void        impl_moveLeft_throw() const;

        /** moves our cursor one position to the right, caring for different possible
            cursor states.

            Before the movement is done, the current row is saved, if necessary.

            @precond
                canMoveRight()
        */
        void        impl_moveRight_throw( ) const;

        /** impl-version of commitCurrentRecord, which can be called without caring for
            an output parameter, and within const-contexts

            @precond
                our mutex is locked
        */
        bool        impl_commitCurrentRecord_throw( sal_Bool* _pRecordInserted = nullptr ) const;

        /** impl-version of commitCurrentControl, which can be called in const-contexts

            @precond
                our mutex is locked
        */
        bool        impl_commitCurrentControl_throw() const;

        /// resets all control models in our own form
        void        impl_resetAllControls_nothrow() const;

        /// executes the "auto sort ascending" and "auto sort descending" features
        void        impl_executeAutoSort_throw( bool _bUp ) const;

        /// executes the "auto filter" feature
        void        impl_executeAutoFilter_throw( ) const;

        /// executes the interactive sort resp. filter feature
        void        impl_executeFilterOrSort_throw( bool _bFilter ) const;

    private:
        /** calls a (member) function, catches SQLExceptions, extends them with additional context information,
            and rethrows them

            @param f
                a functionoid with no arguments to do the work
            @param pErrorResourceId
                the id of the resources string to use as error message
        */
        template < typename FunctObj >
        void        impl_doActionInSQLContext_throw( FunctObj f, TranslateId pErrorResourceId ) const;

        // functionoid to call appendOrderByColumn
        class impl_appendOrderByColumn_throw
        {
        public:
            impl_appendOrderByColumn_throw(const FormOperations *pFO,
                                           css::uno::Reference< css::beans::XPropertySet > xField,
                                           bool bUp)
                : m_pFO(pFO)
                , m_xField(std::move(xField))
                , m_bUp(bUp)
            {};

            void operator()() { m_pFO->m_xParser->appendOrderByColumn(m_xField, m_bUp); }
        private:
            const FormOperations *m_pFO;
            css::uno::Reference< css::beans::XPropertySet > m_xField;
            bool m_bUp;
        };

        // functionoid to call appendFilterByColumn
        class impl_appendFilterByColumn_throw
        {
        public:
            impl_appendFilterByColumn_throw(const FormOperations *pFO,
                                            css::uno::Reference< css::sdb::XSingleSelectQueryComposer > xParser,
                                            css::uno::Reference< css::beans::XPropertySet > xField)
                : m_pFO(pFO)
                , m_xParser(std::move(xParser))
                , m_xField(std::move(xField))
            {};

            void operator()() {
                if (dbtools::isAggregateColumn( m_xParser, m_xField ))
                    m_pFO->m_xParser->appendHavingClauseByColumn( m_xField, true, css::sdb::SQLFilterOperator::EQUAL );
                else
                    m_pFO->m_xParser->appendFilterByColumn( m_xField, true, css::sdb::SQLFilterOperator::EQUAL );
            }
        private:
            const FormOperations *m_pFO;
            css::uno::Reference< css::sdb::XSingleSelectQueryComposer > m_xParser;
            css::uno::Reference< css::beans::XPropertySet > m_xField;
        };

    private:
        FormOperations( const FormOperations& ) = delete;
        FormOperations& operator=( const FormOperations& ) = delete;

    public:

        css::uno::Reference<css::awt::XWindow> GetDialogParent() const;

        class MethodGuard
        {
            FormOperations& m_rOwner;
            bool            m_bCleared;

        public:
            explicit MethodGuard( FormOperations& _rOwner )
                :m_rOwner( _rOwner )
                ,m_bCleared( false )
            {
                m_rOwner.enterMethod( FormOperations::MethodAccess() );
            }

            ~MethodGuard()
            {
                clear();
            }

            void clear()
            {
                if ( !m_bCleared )
                    m_rOwner.leaveMethod( FormOperations::MethodAccess() );
                m_bCleared = true;
            }
        };
    };


} // namespace frm


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
