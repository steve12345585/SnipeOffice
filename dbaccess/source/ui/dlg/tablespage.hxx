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

#include "adminpages.hxx"
#include <tabletree.hxx>
#include <com/sun/star/sdbc/XConnection.hpp>

namespace dbaui
{

    // OTableSubscriptionPage
    class OTableSubscriptionDialog;
    class OTableSubscriptionPage final
            :public OGenericAdministrationPage
    {
    private:
        OUString                       m_sCatalogSeparator;
        bool                           m_bCatalogAtStart : 1;

        css::uno::Reference< css::sdbc::XConnection >
                                       m_xCurrentConnection;   /// valid as long as the page is active
        OTableSubscriptionDialog*      m_pTablesDlg;

        std::unique_ptr<weld::Widget>  m_xTables;
        std::unique_ptr<OTableTreeListBox> m_xTablesList;

    public:
        virtual bool            FillItemSet(SfxItemSet* _rCoreAttrs) override;
        virtual DeactivateRC    DeactivatePage(SfxItemSet* _pSet) override;

        OTableSubscriptionPage(weld::Container* pPage, OTableSubscriptionDialog* pController, const SfxItemSet& _rCoreAttrs);
        virtual ~OTableSubscriptionPage() override;

    private:
        virtual void fillControls(std::vector< std::unique_ptr<ISaveValueWrapper> >& _rControlList) override;
        virtual void fillWindows(std::vector< std::unique_ptr<ISaveValueWrapper> >& _rControlList) override;

        DECL_LINK(OnTreeEntryChecked, const weld::TreeView::iter_col&, void);

        /** check the tables in <member>m_aTablesList</member> according to <arg>_rTables</arg>
        */
        void implCheckTables(const css::uno::Sequence< OUString >& _rTables);

        /// returns the next sibling, if not available, the next sibling of the parent, a.s.o.
        std::unique_ptr<weld::TreeIter> implNextSibling(const weld::TreeIter* pEntry) const;

        /** return the current selection in <member>m_aTablesList</member>
        */
        css::uno::Sequence< OUString > collectDetailedSelection() const;

        /// (un)check all entries
        void CheckAll( bool bCheck = true );

        virtual void implInitControls(const SfxItemSet& _rSet, bool _bSaveValue) override;

        // checks the tables according to the filter given
        // in opposite to implCheckTables, this method handles the case of an empty sequence, too ...
        void implCompleteTablesCheck( const css::uno::Sequence< OUString >& _rTableFilter );
    };

}   // namespace dbaui

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
