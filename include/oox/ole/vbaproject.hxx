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

#ifndef INCLUDED_OOX_OLE_VBAPROJECT_HXX
#define INCLUDED_OOX_OLE_VBAPROJECT_HXX

#include <functional>
#include <map>
#include <string_view>

#include <com/sun/star/uno/Reference.hxx>
#include <oox/dllapi.h>
#include <oox/helper/refmap.hxx>
#include <oox/helper/refvector.hxx>
#include <rtl/ustring.hxx>
#include <sal/types.h>

namespace com::sun::star {
    namespace container { class XNameContainer; }
    namespace frame { class XModel; }
    namespace script { class XLibraryContainer; }
    namespace script::vba { class XVBAMacroResolver; }
    namespace uno { class XComponentContext; }
    namespace uno { class XInterface; }
    namespace io { class XInputStream; }
}

namespace oox {
    class GraphicHelper;
    class StorageBase;
}

namespace oox::ole {

class VbaModule;

class VbaFilterConfig
{
public:
    explicit            VbaFilterConfig(
                            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
                            std::u16string_view rConfigCompName );
                        ~VbaFilterConfig();

    /** Returns true, if the VBA source code and forms should be imported. */
    bool                isImportVba() const;
    /** Returns true, if the VBA source code should be imported executable. */
    bool                isImportVbaExecutable() const;
    /** Returns true, if the VBA source code and forms should be exported. */
    bool                isExportVba() const;

private:
    css::uno::Reference< css::uno::XInterface >
                        mxConfigAccess;
};


/** Base class for objects that attach a macro to a specific action.

    Purpose is to collect objects that need to attach a VBA macro to an action.
    The VBA project will be loaded at a very late point of the document import
    process, because it depends on an initialized core document model (e.g.
    spreadsheet codenames). Some objects that want to attach a VBA macro to an
    action (e.g. mouse click action for drawing shapes) are loaded long before
    the VBA project. The drawback is that in most cases macros are specified
    without module name, or the VBA project name is part of the macro name.
    In the former case, all code modules have to be scanned for the macro to be
    able to create a valid script URL.

    The import code will register these requests to attach a VBA macro with an
    instance of a class derived from this base class. The derived class will
    store all information needed to finally attach the macro to the action,
    once the VBA project has been imported.
 */
class OOX_DLLPUBLIC VbaMacroAttacherBase
{
public:
    explicit            VbaMacroAttacherBase( OUString aMacroName );
    virtual             ~VbaMacroAttacherBase();

    /** Resolves the internal macro name to the related macro URL, and attaches
        the macro to the object. */
    void                resolveAndAttachMacro(
                            const css::uno::Reference< css::script::vba::XVBAMacroResolver >& rxResolver );

private:
    /** Called after the VBA project has been imported. Derived classes will
        attach the passed script to the object represented by this instance. */
    virtual void        attachMacro( const OUString& rScriptUrl ) = 0;

private:
    OUString     maMacroName;
};

typedef std::shared_ptr< VbaMacroAttacherBase > VbaMacroAttacherRef;


class OOX_DLLPUBLIC VbaProject : public VbaFilterConfig
{
public:
    explicit            VbaProject(
                            const css::uno::Reference< css::uno::XComponentContext >& rxContext,
                            const css::uno::Reference< css::frame::XModel >& rxDocModel,
                            std::u16string_view rConfigCompName );
    virtual             ~VbaProject();

    /** Imports the entire VBA project from the passed storage.

        @param rVbaPrjStrg  The root storage of the entire VBA project.
     */
    void                importVbaProject(
                            StorageBase& rVbaPrjStrg,
                            const GraphicHelper& rGraphicHelper );

    bool                importVbaProject(
                            StorageBase& rVbaPrjStrg );

    /// Imports VBA data for a VBA project, e.g. word/vbaData.xml.
    void                importVbaData(const css::uno::Reference<css::io::XInputStream>& xInputStream);

    /** Reads vba module related information from the project streams */
    void                readVbaModules( StorageBase& rVbaPrjStrg );
    /** Imports (and creates) vba modules and user forms from the vba project records previously read.
      Note: ( expects that readVbaModules was already called ) */
    void                importModulesAndForms( StorageBase& rVbaPrjStrg, const GraphicHelper& rGraphicHelper );
    /** Registers a macro attacher object. For details, see description of the
        VbaMacroAttacherBase class. */
    void                registerMacroAttacher( const VbaMacroAttacherRef& rxAttacher );

    /** Attaches VBA macros to objects registered via registerMacroAttacher(). */
    void                attachMacros();

    void                setOleOverridesSink( css::uno::Reference< css::container::XNameContainer > const & rxOleOverridesSink ){ mxOleOverridesSink = rxOleOverridesSink; }

protected:
    /** Registers a dummy module that will be created when the VBA project is
        imported. */
    void                addDummyModule( const OUString& rName, sal_Int32 nType );

    /** Called when the import process of the VBA project has been started. */
    virtual void        prepareImport();

private:
                        VbaProject( const VbaProject& ) = delete;
    VbaProject&         operator=( const VbaProject& ) = delete;

    /** Returns the Basic or dialog library container. */
    css::uno::Reference< css::script::XLibraryContainer >
                        getLibraryContainer( sal_Int32 nPropId );
    /** Opens a Basic or dialog library, creates missing if not found. */
    css::uno::Reference< css::container::XNameContainer >
                        openLibrary( sal_Int32 nPropId );
    /** Creates and returns the Basic library of the document used for import. */
    css::uno::Reference< css::container::XNameContainer > const &
                        createBasicLibrary();
    /** Creates and returns the dialog library of the document used for import. */
    css::uno::Reference< css::container::XNameContainer > const &
                        createDialogLibrary();

    /** Imports the VBA code modules and forms. */
    void                importVba(
                            StorageBase& rVbaPrjStrg,
                            const GraphicHelper& rGraphicHelper );

    /** Copies the entire VBA project storage to the passed document model. */
    void                copyStorage( StorageBase& rVbaPrjStrg );

private:
    typedef RefVector< VbaMacroAttacherBase >           MacroAttacherVector;
    typedef ::std::map< OUString, sal_Int32 >    DummyModuleMap;

    css::uno::Reference< css::uno::XComponentContext >
                        mxContext;          ///< Component context with service manager.
    css::uno::Reference< css::frame::XModel >
                        mxDocModel;         ///< Document model used to import/export the VBA project.
    css::uno::Reference< css::container::XNameContainer >
                        mxBasicLib;         ///< The Basic library of the document used for import.
    css::uno::Reference< css::container::XNameContainer >
                        mxDialogLib;        ///< The dialog library of the document used for import.
    MacroAttacherVector maMacroAttachers;   ///< Objects that want to attach a VBA macro to an action.
    DummyModuleMap      maDummyModules;     ///< Additional empty modules created on import.
    OUString            maPrjName;          ///< Name of the VBA project.
    css::uno::Reference< css::container::XNameContainer >
                        mxOleOverridesSink;
    typedef RefMap< OUString, VbaModule > VbaModuleMap;
    VbaModuleMap        maModules;
    VbaModuleMap        maModulesByStrm;
};


} // namespace oox::ole

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
