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

#ifndef INCLUDED_OOX_OLE_VBAMODULE_HXX
#define INCLUDED_OOX_OLE_VBAMODULE_HXX

#include <com/sun/star/uno/Reference.hxx>
#include <rtl/textenc.h>
#include <rtl/ustring.hxx>
#include <sal/types.h>
#include <vector>

namespace com::sun::star {
    namespace container { class XNameAccess; }
    namespace container { class XNameContainer; }
    namespace frame { class XModel; }
    namespace uno { class XComponentContext; }
}

namespace oox {
    class BinaryInputStream;
    class StorageBase;
}

namespace oox::ole {

/** Stores, which key shortcut maps to which VBA macro method. */
struct VbaMacroKeyAndMethodBinding
{
    // This describes a key combination in "raw" VBA Macro form, that
    // still needs translated to a key event that can be used in
    // LibreOffice.
    OUString msApiKey;
    // The name of the macro method
    OUString msMethodName;
};

class VbaModule
{
public:
    explicit            VbaModule(
                            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
                            const css::uno::Reference< css::frame::XModel >& rxDocModel,
                            OUString aName,
                            rtl_TextEncoding eTextEnc,
                            bool bExecutable );

    /** Returns the module type (com.sun.star.script.ModuleType constant). */
    sal_Int32    getType() const { return mnType; }
    /** Sets the passed module type. */
    void         setType( sal_Int32 nType ) { mnType = nType; }

    /** Returns the name of the module. */
    const OUString& getName() const { return maName; }
    /** Returns the stream name of the module. */
    const OUString& getStreamName() const { return maStreamName; }

    /** Imports all records for this module until the MODULEEND record. */
    void                importDirRecords( BinaryInputStream& rDirStrm );

    /** Imports the VBA source code into the passed Basic library. */
    void                createAndImportModule(
                            StorageBase& rVbaStrg,
                            const css::uno::Reference< css::container::XNameContainer >& rxBasicLib,
                            const css::uno::Reference< css::container::XNameAccess >& rxDocObjectNA );
    /** Creates an empty Basic module in the passed Basic library. */
    void                createEmptyModule(
                            const css::uno::Reference< css::container::XNameContainer >& rxBasicLib,
                            const css::uno::Reference< css::container::XNameAccess >& rxDocObjectNA ) const;

    void registerShortcutKeys();

private:
    /** Reads and returns the VBA source code from the passed storage. */
    OUString     readSourceCode( StorageBase& rVbaStrg );

    /** Creates a new Basic module and inserts it into the passed Basic library. */
    void                createModule(
                            std::u16string_view rVBASourceCode,
                            const css::uno::Reference< css::container::XNameContainer >& rxBasicLib,
                            const css::uno::Reference< css::container::XNameAccess >& rxDocObjectNA ) const;

private:
    css::uno::Reference< css::uno::XComponentContext >
                        mxContext;          ///< Component context with service manager.
    css::uno::Reference< css::frame::XModel >
                        mxDocModel;         ///< Document model used to import/export the VBA project.
    OUString            maName;
    OUString            maStreamName;
    OUString            maDocString;
    rtl_TextEncoding    meTextEnc;
    sal_Int32           mnType;
    sal_uInt32          mnOffset;
    bool                mbReadOnly;
    bool                mbPrivate;
    bool                mbExecutable;

    /** Keys and VBA macro method bindings */
    std::vector<VbaMacroKeyAndMethodBinding> maKeyBindings;
};


} // namespace oox::ole

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
