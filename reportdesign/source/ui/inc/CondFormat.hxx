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

#ifndef INCLUDED_REPORTDESIGN_SOURCE_UI_INC_CONDFORMAT_HXX
#define INCLUDED_REPORTDESIGN_SOURCE_UI_INC_CONDFORMAT_HXX

#include <com/sun/star/report/XReportControlModel.hpp>
#include <vcl/weld.hxx>
#include <vector>

namespace rptui
{


    constexpr size_t MAX_CONDITIONS = 3;

    class OReportController;
    class Condition;


    //= IConditionalFormatAction

    class SAL_NO_VTABLE IConditionalFormatAction
    {
    public:
        virtual void            addCondition( size_t _nAddAfterIndex ) = 0;
        virtual void            deleteCondition( size_t _nCondIndex ) = 0;
        virtual void            applyCommand( size_t _nCondIndex, sal_uInt16 _nCommandId, const ::Color& rColor ) = 0;
        virtual void            moveConditionUp( size_t _nCondIndex ) = 0;
        virtual void            moveConditionDown( size_t _nCondIndex ) = 0;
        virtual OUString getDataField() const = 0;

    protected:
        ~IConditionalFormatAction() {}
    };

    /*************************************************************************
    |*
    |* Conditional formatting dialog
    |*
    \************************************************************************/
    class ConditionalFormattingDialog : public weld::GenericDialogController
                                      , public IConditionalFormatAction
    {
        typedef ::std::vector< std::unique_ptr<Condition> >  Conditions;

        ::rptui::OReportController&                         m_rController;
        css::uno::Reference< css::report::XReportControlModel >
                                                            m_xFormatConditions;
        css::uno::Reference< css::report::XReportControlModel >
                                                            m_xCopy;

        bool    m_bConstructed;

        std::unique_ptr<weld::ScrolledWindow> m_xScrollWindow;
        std::unique_ptr<weld::Box> m_xConditionPlayground;
        Conditions                m_aConditions;

    public:
        ConditionalFormattingDialog(
            weld::Window* pParent,
            const css::uno::Reference< css::report::XReportControlModel>& _xHoldAlive,
            ::rptui::OReportController& _rController
        );
        virtual ~ConditionalFormattingDialog() override;
        // Dialog overridables
        virtual short run() override;

        // IConditionalFormatAction overridables
        virtual void addCondition( size_t _nAddAfterIndex ) override;
        virtual void deleteCondition( size_t _nCondIndex ) override;
        virtual void applyCommand( size_t _nCondIndex, sal_uInt16 _nCommandId, const ::Color& rColor ) override;
        virtual void moveConditionUp( size_t _nCondIndex ) override;
        virtual void moveConditionDown( size_t _nCondIndex ) override;
        virtual OUString getDataField() const override;

    private:
        DECL_LINK(OnScroll, weld::ScrolledWindow&, void);

    private:
        /// returns the current number of conditions
        size_t  impl_getConditionCount() const { return m_aConditions.size(); }

        /** adds a condition
            @param _nNewCondIndex
                the index of the to-be-inserted condition
        */
        void    impl_addCondition_nothrow( size_t _nNewCondIndex );

        /// deletes the condition with the given index
        void    impl_deleteCondition_nothrow( size_t _nCondIndex );

        /// moves the condition with the given index one position
        void    impl_moveCondition_nothrow( size_t _nCondIndex, bool _bMoveUp );

        /// does the dialog layouting
        void    impl_layoutAll();

        /// called when the number of conditions has changed in any way
        void    impl_conditionCountChanged();

        /// initializes the conditions from m_xCopy
        void    impl_initializeConditions();

        /// tells all our Condition instances their new index
        void    impl_updateConditionIndicies();

        /// returns the number of the condition which has the (child path) focus
        size_t  impl_getFocusedConditionIndex( sal_Int32 _nFallBackIfNone ) const;

        /// returns the index of the first visible condition
        size_t  impl_getFirstVisibleConditionIndex() const;

        /// returns the index of the last visible condition
        size_t  impl_getLastVisibleConditionIndex() const;

        /// focuses the condition with the given index, making it visible if necessary
        void    impl_focusCondition( size_t _nCondIndex );

        /// scrolls the condition with the given index to the top position
        void    impl_scrollTo( size_t _nTopCondIndex );

        /// ensures the condition with the given index is visible
        void    impl_ensureConditionVisible( size_t _nCondIndex );

        /// set the preferred height of the action_area
        void    impl_setPrefHeight(bool bFirst);
    };


} // namespace rptui


#endif // INCLUDED_REPORTDESIGN_SOURCE_UI_INC_CONDFORMAT_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
