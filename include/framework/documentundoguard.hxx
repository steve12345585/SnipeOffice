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

#ifndef INCLUDED_FRAMEWORK_DOCUMENTUNDOGUARD_HXX
#define INCLUDED_FRAMEWORK_DOCUMENTUNDOGUARD_HXX

#include <framework/fwkdllapi.h>
#include <com/sun/star/uno/Reference.hxx>
#include <rtl/ref.hxx>

namespace com::sun::star::uno { class XInterface; }
namespace com::sun::star::document { class XUndoManager; }

namespace framework
{
    class UndoManagerContextListener;

    //= DocumentUndoGuard

    /** a helper class guarding the Undo manager of a document

        This class guards, within a given scope, the Undo Manager of a document (or another component supporting
        the XUndoManagerSupplier interface). When entering the scope (i.e. when the <code>DocumentUndoGuard</code>
        instances is constructed), the current state of the undo contexts of the undo manager is examined.
        Upon leaving the scope (i.e. when the <code>DocumentUndoGuard</code> is destructed), the guard will execute
        as many calls to <member scope="css::document">XUndoManager::leaveUndoContext</member> as are
        necessary to restore the manager's initial state.
    */
    class FWK_DLLPUBLIC DocumentUndoGuard
    {
    public:
        DocumentUndoGuard( const css::uno::Reference< css::uno::XInterface >& i_undoSupplierComponent );
        ~DocumentUndoGuard();

    private:
        css::uno::Reference< css::document::XUndoManager > mxUndoManager;
        ::rtl::Reference< UndoManagerContextListener >  mxContextListener;
    };


} // namespace framework


#endif // INCLUDED_FRAMEWORK_DOCUMENTUNDOGUARD_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
