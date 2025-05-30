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
#ifndef INCLUDED_REPORTDESIGN_INC_UNDOENV_HXX
#define INCLUDED_REPORTDESIGN_INC_UNDOENV_HXX

#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <com/sun/star/beans/PropertyChangeEvent.hpp>
#include <com/sun/star/container/XContainerListener.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <com/sun/star/util/XModifyListener.hpp>
#include <com/sun/star/report/XSection.hpp>

#include <memory>
#include <cppuhelper/implbase.hxx>
#include <svl/lstner.hxx>
#include "dllapi.h"
#include "RptPage.hxx"

namespace rptui
{
    class OXUndoEnvironmentImpl;


    class SAL_DLLPUBLIC_RTTI OXUndoEnvironment final
        : public ::cppu::WeakImplHelper<   css::beans::XPropertyChangeListener
                                        ,   css::container::XContainerListener
                                        ,   css::util::XModifyListener
                                        >
        , public SfxListener
    {
        const ::std::unique_ptr<OXUndoEnvironmentImpl> m_pImpl;

        OXUndoEnvironment(const OXUndoEnvironment&) = delete;
        OXUndoEnvironment& operator=(const OXUndoEnvironment&) = delete;

        virtual ~OXUndoEnvironment() override;

        REPORTDESIGN_DLLPUBLIC void SetUndoMode(bool _bUndo);

    public:
        OXUndoEnvironment(OReportModel& _rModel);

        /**
           Create an object ob OUndoEnvLock locks the undo possibility
           As long as in the OUndoEnvLock scope, no undo is possible for manipulated object.
         */
        class OUndoEnvLock
        {
            OXUndoEnvironment& m_rUndoEnv;
        public:
            OUndoEnvLock(OXUndoEnvironment& _rUndoEnv): m_rUndoEnv(_rUndoEnv){m_rUndoEnv.Lock();}
            ~OUndoEnvLock(){ m_rUndoEnv.UnLock(); }
        };

        /**
           This is near the same as OUndoEnvLock but it is also possible to ask for the current mode.
           UndoMode will set if SID_UNDO is called in execute()
         */
        class OUndoMode
        {
            OXUndoEnvironment& m_rUndoEnv;
        public:
            OUndoMode(OXUndoEnvironment& _rUndoEnv)
                :m_rUndoEnv(_rUndoEnv)
            {
                m_rUndoEnv.Lock();
                m_rUndoEnv.SetUndoMode(true);
            }
            ~OUndoMode()
            {
                m_rUndoEnv.SetUndoMode(false);
                m_rUndoEnv.UnLock();
            }
        };

        REPORTDESIGN_DLLPUBLIC void Lock();
        REPORTDESIGN_DLLPUBLIC void UnLock();
        bool IsLocked() const;

        // returns sal_True is we are in UNDO
        bool IsUndoMode() const;

        // access control
        struct Accessor { friend class OReportModel; private: Accessor() { } };
        void Clear(const Accessor& _r);

        REPORTDESIGN_DLLPUBLIC void AddElement(const css::uno::Reference< css::uno::XInterface>& Element);
        REPORTDESIGN_DLLPUBLIC void RemoveElement(const css::uno::Reference< css::uno::XInterface>& Element);

        void AddSection( const css::uno::Reference< css::report::XSection>& _xSection);
        void RemoveSection( const css::uno::Reference< css::report::XSection>& _xSection );
        /** removes the section from the page out of the undo env
        *
        * \param _pPage
        */
        void RemoveSection(OReportPage const * _pPage);

    private:
        // XEventListener
        virtual void SAL_CALL disposing(const css::lang::EventObject& Source) override;

        // XPropertyChangeListener
        virtual void SAL_CALL propertyChange(const css::beans::PropertyChangeEvent& evt) override;

        // XContainerListener
        virtual void SAL_CALL elementInserted(const css::container::ContainerEvent& rEvent) override;
        virtual void SAL_CALL elementReplaced(const css::container::ContainerEvent& rEvent) override;
        virtual void SAL_CALL elementRemoved(const css::container::ContainerEvent& rEvent) override;

        // XModifyListener
        virtual void SAL_CALL modified( const css::lang::EventObject& aEvent ) override;

        void ModeChanged();

        virtual void Notify( SfxBroadcaster& rBC, const SfxHint& rHint ) override;

    private:
        void    implSetModified();

        void    switchListening( const css::uno::Reference< css::container::XIndexAccess >& _rxContainer, bool _bStartListening );
        void    switchListening( const css::uno::Reference< css::uno::XInterface >& _rxObject, bool _bStartListening );

        ::std::vector< css::uno::Reference< css::container::XChild> >::const_iterator
            getSection(const css::uno::Reference< css::container::XChild>& _xContainer) const;
    };

}
#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
