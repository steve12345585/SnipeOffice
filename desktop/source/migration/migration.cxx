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

#include <algorithm>
#include <iterator>
#include <map>
#include <set>

#include <migration.hxx>
#include "migration_impl.hxx"

#include <sal/log.hxx>
#include <unotools/textsearch.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <unotools/bootstrap.hxx>
#include <rtl/uri.hxx>
#include <i18nlangtag/lang.h>
#include <comphelper/diagnose_ex.hxx>
#include <tools/urlobj.hxx>
#include <officecfg/Office/UI.hxx>
#include <osl/file.hxx>
#include <osl/security.hxx>
#include <unotools/configmgr.hxx>

#include <com/sun/star/configuration/Update.hpp>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/task/XJob.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/util/XRefreshable.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/embed/FileSystemStorageFactory.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/ui/theModuleUIConfigurationManagerSupplier.hpp>
#include <com/sun/star/ui/UIConfigurationManager.hpp>
#include <com/sun/star/ui/XUIConfigurationPersistence.hpp>
#include <vcl/commandinfoprovider.hxx>

using namespace osl;
using namespace com::sun::star::task;
using namespace com::sun::star::lang;
using namespace com::sun::star::beans;
using namespace com::sun::star::util;
using namespace com::sun::star::container;
using com::sun::star::uno::Exception;
using namespace com::sun::star;


namespace desktop
{

constexpr OUString ITEM_DESCRIPTOR_COMMANDURL = u"CommandURL"_ustr;
constexpr OUString ITEM_DESCRIPTOR_CONTAINER = u"ItemDescriptorContainer"_ustr;
constexpr OUString ITEM_DESCRIPTOR_LABEL = u"Label"_ustr;

static OUString mapModuleShortNameToIdentifier(std::u16string_view sShortName)
{
    OUString sIdentifier;

    if ( sShortName == u"StartModule" )
        sIdentifier = "com.sun.star.frame.StartModule";

    else if ( sShortName == u"swriter" )
        sIdentifier = "com.sun.star.text.TextDocument";

    else if ( sShortName == u"scalc" )
        sIdentifier = "com.sun.star.sheet.SpreadsheetDocument";

    else if ( sShortName == u"sdraw" )
        sIdentifier = "com.sun.star.drawing.DrawingDocument";

    else if ( sShortName == u"simpress" )
        sIdentifier = "com.sun.star.presentation.PresentationDocument";

    else if ( sShortName == u"smath" )
        sIdentifier = "com.sun.star.formula.FormulaProperties";

    else if ( sShortName == u"schart" )
        sIdentifier = "com.sun.star.chart2.ChartDocument";

    else if ( sShortName == u"BasicIDE" )
        sIdentifier = "com.sun.star.script.BasicIDE";

    else if ( sShortName == u"dbapp" )
        sIdentifier = "com.sun.star.sdb.OfficeDatabaseDocument";

    else if ( sShortName == u"sglobal" )
        sIdentifier = "com.sun.star.text.GlobalDocument";

    else if ( sShortName == u"sweb" )
        sIdentifier = "com.sun.star.text.WebDocument";

    else if ( sShortName == u"swxform" )
        sIdentifier = "com.sun.star.xforms.XMLFormDocument";

    else if ( sShortName == u"sbibliography" )
        sIdentifier = "com.sun.star.frame.Bibliography";

    return sIdentifier;
}

bool MigrationImpl::alreadyMigrated()
{
    OUString aStr = m_aInfo.userdata + "/MIGRATED4";
    File aFile(aStr);
    // create migration stamp, and/or check its existence
    bool bRet = aFile.open (osl_File_OpenFlag_Write | osl_File_OpenFlag_Create | osl_File_OpenFlag_NoLock) == FileBase::E_EXIST;
    SAL_INFO( "desktop.migration", "File '" << aStr << "' exists? " << bRet );
    return bRet;
}

bool MigrationImpl::initializeMigration()
{
    bool bRet = false;

    if (!checkMigrationCompleted()) {
        readAvailableMigrations(m_vMigrationsAvailable);
        sal_Int32 nIndex = findPreferredMigrationProcess(m_vMigrationsAvailable);
        // m_aInfo is now set to the preferred migration source
        if ( nIndex >= 0 ) {
            if (alreadyMigrated())
                return false;
            m_vrMigrations = readMigrationSteps(m_vMigrationsAvailable[nIndex].name);
        }

        bRet = !m_aInfo.userdata.isEmpty();
    }

    SAL_INFO( "desktop.migration", "Migration " << ( bRet ? "needed" : "not required" ) );

    return bRet;
}

void Migration::migrateSettingsIfNecessary()
{
    MigrationImpl aImpl;

    if (! aImpl.initializeMigration() )
        return;

    bool bResult = false;
    try {
        bResult = aImpl.doMigration();
    } catch (const Exception&) {
        TOOLS_WARN_EXCEPTION( "desktop", "doMigration()");
    }
    OSL_ENSURE(bResult, "Migration has not been successful");
}

MigrationImpl::MigrationImpl()
{
}

MigrationImpl::~MigrationImpl()
{
}

// The main entry point for migrating settings
bool MigrationImpl::doMigration()
{
    // compile file list for migration
    m_vrFileList = compileFileList();

    bool result = false;
    try {
        NewVersionUIInfo aNewVersionUIInfo;
        std::vector< MigrationModuleInfo > vModulesInfo = detectUIChangesForAllModules();
        aNewVersionUIInfo.init(vModulesInfo);

        copyFiles();

        static constexpr OUString sMenubarResourceURL(u"private:resource/menubar/menubar"_ustr);
        static constexpr OUStringLiteral sToolbarResourcePre(u"private:resource/toolbar/");
        for (MigrationModuleInfo & i : vModulesInfo) {
            OUString sModuleIdentifier = mapModuleShortNameToIdentifier(i.sModuleShortName);
            if (sModuleIdentifier.isEmpty())
                continue;


            OUString aOldCfgDataPath = m_aInfo.userdata + "/user/config/soffice.cfg/modules/" + i.sModuleShortName;
            uno::Sequence< uno::Any > lArgs {uno::Any(aOldCfgDataPath), uno::Any(embed::ElementModes::READ)};

            const uno::Reference< uno::XComponentContext >& xContext(comphelper::getProcessComponentContext());
            uno::Reference< lang::XSingleServiceFactory > xStorageFactory(embed::FileSystemStorageFactory::create(xContext));
            uno::Reference< embed::XStorage >             xModules(xStorageFactory->createInstanceWithArguments(lArgs), uno::UNO_QUERY);
            uno::Reference< ui::XUIConfigurationManager2 > xOldCfgManager = ui::UIConfigurationManager::create(xContext);

            if ( xModules.is() ) {
                xOldCfgManager->setStorage( xModules );
                xOldCfgManager->reload();
            }

            uno::Reference< ui::XUIConfigurationManager > xCfgManager = aNewVersionUIInfo.getConfigManager(i.sModuleShortName);

            if (i.bHasMenubar) {
                uno::Reference< container::XIndexContainer > xOldVersionMenuSettings(xOldCfgManager->getSettings(sMenubarResourceURL, true), uno::UNO_QUERY);
                uno::Reference< container::XIndexContainer > xNewVersionMenuSettings = aNewVersionUIInfo.getNewMenubarSettings(i.sModuleShortName);
                compareOldAndNewConfig(OUString(), xOldVersionMenuSettings, xNewVersionMenuSettings, sMenubarResourceURL);
                mergeOldToNewVersion(xCfgManager, xNewVersionMenuSettings, sModuleIdentifier, sMenubarResourceURL);
            }

            sal_Int32 nToolbars = i.m_vToolbars.size();
            if (nToolbars >0) {
                for (sal_Int32 j=0; j<nToolbars; ++j) {
                    OUString sToolbarName = i.m_vToolbars[j];
                    OUString sToolbarResourceURL = sToolbarResourcePre + sToolbarName;

                    uno::Reference< container::XIndexContainer > xOldVersionToolbarSettings(xOldCfgManager->getSettings(sToolbarResourceURL, true), uno::UNO_QUERY);
                    uno::Reference< container::XIndexContainer > xNewVersionToolbarSettings = aNewVersionUIInfo.getNewToolbarSettings(i.sModuleShortName, sToolbarName);
                    compareOldAndNewConfig(OUString(), xOldVersionToolbarSettings, xNewVersionToolbarSettings, sToolbarResourceURL);
                    mergeOldToNewVersion(xCfgManager, xNewVersionToolbarSettings, sModuleIdentifier, sToolbarResourceURL);
                }
            }

            m_aOldVersionItemsHashMap.clear();
        }

        // execute the migration items from Setup.xcu
        copyConfig();

        // execute custom migration services from Setup.xcu
        // and refresh the cache
        runServices();
        uno::Reference< XRefreshable >(
            configuration::theDefaultProvider::get(comphelper::getProcessComponentContext()),
            uno::UNO_QUERY_THROW)->refresh();

        result = true;
    } catch (const css::uno::Exception &) {
        TOOLS_WARN_EXCEPTION(
            "desktop.migration",
            "ignored Exception while migrating from version \"" << m_aInfo.productname
            << "\" data \"" << m_aInfo.userdata << "\"");
    }

    // prevent running the migration multiple times
    setMigrationCompleted();
    return result;
}

void MigrationImpl::setMigrationCompleted()
{
    try {
        uno::Reference< XPropertySet > aPropertySet(getConfigAccess("org.openoffice.Setup/Office", true), uno::UNO_QUERY_THROW);
        aPropertySet->setPropertyValue(u"MigrationCompleted"_ustr, uno::Any(true));
        uno::Reference< XChangesBatch >(aPropertySet, uno::UNO_QUERY_THROW)->commitChanges();
    } catch (...) {
        // fail silently
    }
}

bool MigrationImpl::checkMigrationCompleted()
{
    bool bMigrationCompleted = false;
    try {
        uno::Reference< XPropertySet > aPropertySet(
            getConfigAccess("org.openoffice.Setup/Office"), uno::UNO_QUERY_THROW);
        aPropertySet->getPropertyValue(u"MigrationCompleted"_ustr) >>= bMigrationCompleted;

        if( !bMigrationCompleted && getenv("SAL_DISABLE_USERMIGRATION" ) ) {
            // migration prevented - fake its success
            setMigrationCompleted();
            bMigrationCompleted = true;
        }
    } catch (const Exception&) {
        // just return false...
    }
    SAL_INFO( "desktop.migration", "Migration " << ( bMigrationCompleted ? "already completed" : "not done" ) );

    return bMigrationCompleted;
}

static void insertSorted(migrations_available& rAvailableMigrations, supported_migration const & aSupportedMigration)
{
    migrations_available::iterator pIter = std::find_if(rAvailableMigrations.begin(), rAvailableMigrations.end(),
        [&aSupportedMigration](const supported_migration& rMigration) { return rMigration.nPriority < aSupportedMigration.nPriority; });
    if (pIter != rAvailableMigrations.end())
        rAvailableMigrations.insert(pIter, aSupportedMigration );
    else
        rAvailableMigrations.push_back( aSupportedMigration );
}

void MigrationImpl::readAvailableMigrations(migrations_available& rAvailableMigrations)
{
    // get supported version names
    uno::Reference< XNameAccess > aMigrationAccess(getConfigAccess("org.openoffice.Setup/Migration/SupportedVersions"), uno::UNO_SET_THROW);
    const uno::Sequence< OUString > seqSupportedVersions = aMigrationAccess->getElementNames();

    static constexpr OUStringLiteral aVersionIdentifiers( u"VersionIdentifiers" );
    static constexpr OUStringLiteral aPriorityIdentifier( u"Priority" );

    for (OUString const & supportedVersion :seqSupportedVersions) {
        sal_Int32                 nPriority( 0 );
        uno::Sequence< OUString > seqVersions;
        uno::Reference< XNameAccess > xMigrationData( aMigrationAccess->getByName(supportedVersion), uno::UNO_QUERY_THROW );
        xMigrationData->getByName( aVersionIdentifiers ) >>= seqVersions;
        xMigrationData->getByName( aPriorityIdentifier ) >>= nPriority;

        supported_migration aSupportedMigration;
        aSupportedMigration.name      = supportedVersion;
        aSupportedMigration.nPriority = nPriority;
        for (OUString const& s : seqVersions)
            aSupportedMigration.supported_versions.push_back(s.trim());
        insertSorted( rAvailableMigrations, aSupportedMigration );
        SAL_INFO( "desktop.migration", " available migration '" << aSupportedMigration.name << "'" );
    }
}

migrations_vr MigrationImpl::readMigrationSteps(const OUString& rMigrationName)
{
    // get migration access
    uno::Reference< XNameAccess > aMigrationAccess(getConfigAccess("org.openoffice.Setup/Migration/SupportedVersions"), uno::UNO_SET_THROW);
    uno::Reference< XNameAccess > xMigrationData( aMigrationAccess->getByName(rMigrationName), uno::UNO_QUERY_THROW );

    // get migration description from org.openoffice.Setup/Migration
    // and build vector of migration steps
    uno::Reference< XNameAccess > theNameAccess(xMigrationData->getByName(u"MigrationSteps"_ustr), uno::UNO_QUERY_THROW);
    uno::Reference< XNameAccess > tmpAccess;
    uno::Sequence< OUString > tmpSeq;
    migrations_vr vrMigrations(new migrations_v);
    const css::uno::Sequence<OUString> aMigrationSteps = theNameAccess->getElementNames();
    for (const OUString& rMigrationStep : aMigrationSteps) {
        // get current migration step
        theNameAccess->getByName(rMigrationStep) >>= tmpAccess;
        migration_step tmpStep;

        // read included files from current step description
        if (tmpAccess->getByName(u"IncludedFiles"_ustr) >>= tmpSeq) {
            tmpStep.includeFiles.insert(tmpStep.includeFiles.end(), tmpSeq.begin(), tmpSeq.end());
        }

        // excluded files...
        if (tmpAccess->getByName(u"ExcludedFiles"_ustr) >>= tmpSeq) {
            tmpStep.excludeFiles.insert(tmpStep.excludeFiles.end(), tmpSeq.begin(), tmpSeq.end());
        }

        // included nodes...
        if (tmpAccess->getByName(u"IncludedNodes"_ustr) >>= tmpSeq) {
            tmpStep.includeConfig.insert(tmpStep.includeConfig.end(), tmpSeq.begin(), tmpSeq.end());
        }

        // excluded nodes...
        if (tmpAccess->getByName(u"ExcludedNodes"_ustr) >>= tmpSeq) {
            tmpStep.excludeConfig.insert(tmpStep.excludeConfig.end(), tmpSeq.begin(), tmpSeq.end());
        }

        // excluded extensions...
        if (tmpAccess->getByName(u"ExcludedExtensions"_ustr) >>= tmpSeq) {
            tmpStep.excludeExtensions.insert(tmpStep.excludeExtensions.end(), tmpSeq.begin(), tmpSeq.end());
        }

        // generic service
        tmpAccess->getByName(u"MigrationService"_ustr) >>= tmpStep.service;

        vrMigrations->push_back(tmpStep);
    }
    return vrMigrations;
}

static FileBase::RC _checkAndCreateDirectory(INetURLObject const & dirURL)
{
    FileBase::RC result = Directory::create(dirURL.GetMainURL(INetURLObject::DecodeMechanism::ToIUri));
    if (result == FileBase::E_NOENT) {
        INetURLObject baseURL(dirURL);
        baseURL.removeSegment();
        _checkAndCreateDirectory(baseURL);
        return Directory::create(dirURL.GetMainURL(INetURLObject::DecodeMechanism::ToIUri));
    } else
        return result;
}

#if defined UNX && ! defined MACOSX

const char XDG_CONFIG_PART[] = "/.config/";

OUString MigrationImpl::preXDGConfigDir(const OUString& rConfigDir)
{
    OUString aPreXDGConfigPath;
    const char* pXDGCfgHome = getenv("XDG_CONFIG_HOME");

    // cater for XDG_CONFIG_HOME change
    // If XDG_CONFIG_HOME is set then we;
    // assume the user knows what they are doing ( room for improvement here, we could
    // of course search the default config dir etc. also  - but this is more complex,
    // we would need to weigh results from the current config dir against matches in
    // the 'old' config dir etc. ) - currently we just use the returned config dir.
    // If XDG_CONFIG_HOME is NOT set;
    // assume then we should now using the default $HOME/.config config location for
    // our user profiles, however *all* previous libreoffice and openoffice.org
    // configurations will be in the 'old' config directory and that's where we need
    // to search - we convert the returned config dir to the 'old' dir
    if ( !pXDGCfgHome && rConfigDir.endsWith( XDG_CONFIG_PART )  )
        // remove trailing '.config/' but leave the terminating '/'
        aPreXDGConfigPath = rConfigDir.copy( 0, rConfigDir.getLength() - sizeof(  XDG_CONFIG_PART ) + 2 );
    else
        aPreXDGConfigPath = rConfigDir;

    // the application-specific config dir is no longer prefixed by '.' because it is hidden under ".config"
    // we have to add the '.' for the pre-XDG directory names
    aPreXDGConfigPath += ".";

    return aPreXDGConfigPath;
}
#endif

void MigrationImpl::setInstallInfoIfExist(
    install_info& aInfo,
    std::u16string_view rConfigDir,
    const OUString& rVersion)
{
    OUString url(INetURLObject(rConfigDir).GetMainURL(INetURLObject::DecodeMechanism::NONE));
    osl::DirectoryItem item;
    osl::FileStatus stat(osl_FileStatus_Mask_Type);

    if (osl::DirectoryItem::get(url, item) == osl::FileBase::E_None
        && item.getFileStatus(stat) == osl::FileBase::E_None
        && stat.getFileType() == osl::FileStatus::Directory) {
        aInfo.userdata = url;
        aInfo.productname = rVersion;
    }
}

install_info MigrationImpl::findInstallation(const strings_v& rVersions)
{

    OUString aTopConfigDir;
    osl::Security().getConfigDir( aTopConfigDir );
    if ( !aTopConfigDir.isEmpty() && aTopConfigDir[ aTopConfigDir.getLength()-1 ] != '/' )
        aTopConfigDir += "/";

#if defined UNX && ! defined MACOSX
    OUString aPreXDGTopConfigDir = preXDGConfigDir(aTopConfigDir);
#endif

    install_info aInfo;
    for (auto const& elem : rVersions)
    {
        OUString aVersion, aProfileName;
        sal_Int32 nSeparatorIndex = elem.indexOf('=');
        if ( nSeparatorIndex != -1 ) {
            aVersion = elem.copy( 0, nSeparatorIndex );
            aProfileName = elem.copy( nSeparatorIndex+1 );
        }

        if ( !aVersion.isEmpty() && !aProfileName.isEmpty() &&
             ( aInfo.userdata.isEmpty() ||
               aProfileName.equalsIgnoreAsciiCase(
                   utl::ConfigManager::getProductName() ) ) ) {
            setInstallInfoIfExist(aInfo, Concat2View(aTopConfigDir + aProfileName), aVersion);
#if defined UNX && ! defined MACOSX
            //try preXDG path if the new one does not exist
            if ( aInfo.userdata.isEmpty())
                setInstallInfoIfExist(aInfo, Concat2View(aPreXDGTopConfigDir + aProfileName), aVersion);
#endif
        }
    }

    return aInfo;
}

sal_Int32 MigrationImpl::findPreferredMigrationProcess(const migrations_available& rAvailableMigrations)
{
    sal_Int32    nIndex( -1 );
    sal_Int32    i( 0 );

    for (auto const& availableMigration : rAvailableMigrations)
    {
        install_info aInstallInfo = findInstallation(availableMigration.supported_versions);
        if (!aInstallInfo.productname.isEmpty() ) {
            m_aInfo = std::move(aInstallInfo);
            nIndex  = i;
            break;
        }
        ++i;
    }

    SAL_INFO( "desktop.migration", " preferred migration is from product '" << m_aInfo.productname << "'");
    SAL_INFO( "desktop.migration", " and settings directory '" << m_aInfo.userdata << "'");

    return nIndex;
}

strings_vr MigrationImpl::applyPatterns(const strings_v& vSet, const strings_v& vPatterns)
{
    using namespace utl;
    strings_vr vrResult(new strings_v);
    for (auto const& pattern : vPatterns)
    {
        // find matches for this pattern in input set
        // and copy them to the result
        SearchParam param(pattern, SearchParam::SearchType::Regexp);
        TextSearch ts(param, LANGUAGE_DONTKNOW);
        sal_Int32 start = 0;
        sal_Int32 end = 0;
        for (auto const& elem : vSet)
        {
            end = elem.getLength();
            if (ts.SearchForward(elem, &start, &end))
                vrResult->push_back(elem);
        }
    }
    return vrResult;
}

strings_vr MigrationImpl::getAllFiles(const OUString& baseURL) const
{
    strings_vr vrResult(new strings_v);

    // get sub dirs
    Directory dir(baseURL);
    if (dir.open() == FileBase::E_None) {
        strings_v vSubDirs;
        strings_vr vrSubResult;

        // work through directory contents...
        DirectoryItem item;
        FileStatus fs(osl_FileStatus_Mask_Type | osl_FileStatus_Mask_FileURL);
        while (dir.getNextItem(item) == FileBase::E_None) {
            if (item.getFileStatus(fs) == FileBase::E_None) {
                if (fs.getFileType() == FileStatus::Directory)
                    vSubDirs.push_back(fs.getFileURL());
                else
                    vrResult->push_back(fs.getFileURL());
            }
        }

        // recurse subfolders
        for (auto const& subDir : vSubDirs)
        {
            vrSubResult = getAllFiles(subDir);
            vrResult->insert(vrResult->end(), vrSubResult->begin(), vrSubResult->end());
        }
    }
    return vrResult;
}

namespace
{

// removes elements of vector 2 in vector 1
strings_v subtract(strings_v && a, strings_v && b)
{
    std::sort(a.begin(), a.end());
    strings_v::iterator ae(std::unique(a.begin(), a.end()));
    std::sort(b.begin(), b.end());
    strings_v::iterator be(std::unique(b.begin(), b.end()));
    strings_v c;
    std::set_difference(a.begin(), ae, b.begin(), be, std::back_inserter(c));
    return c;
}

}

strings_vr MigrationImpl::compileFileList()
{

    strings_vr vrResult(new strings_v);

    // get a list of all files:
    strings_vr vrFiles = getAllFiles(m_aInfo.userdata);

    // get a file list result for each migration step
    for (auto const& rMigration : *m_vrMigrations)
    {
        strings_vr vrInclude = applyPatterns(*vrFiles, rMigration.includeFiles);
        strings_vr vrExclude = applyPatterns(*vrFiles, rMigration.excludeFiles);
        strings_v sub(subtract(std::move(*vrInclude), std::move(*vrExclude)));
        vrResult->insert(vrResult->end(), sub.begin(), sub.end());
    }
    return vrResult;
}

namespace
{

struct componentParts {
    std::set< OUString > includedPaths;
    std::set< OUString > excludedPaths;
};

typedef std::map< OUString, componentParts > Components;

bool getComponent(OUString const & path, OUString * component)
{
    OSL_ASSERT(component != nullptr);
    if (path.isEmpty() || path[0] != '/') {
        SAL_INFO( "desktop.migration", "configuration migration in/exclude path " << path << " ignored (does not start with slash)" );
        return false;
    }
    sal_Int32 i = path.indexOf('/', 1);
    *component = i < 0 ? path.copy(1) : path.copy(1, i - 1);
    return true;
}

void renameMigratedSetElementTo(
    css::uno::Reference<css::container::XNameContainer> const & set, OUString const & currentName,
    OUString const & migratedName)
{
    // To avoid unexpected data loss, the code is careful to only rename from currentName to
    // migratedName in the expected case where the currentName element exists and the migratedName
    // element doesn't exist:
    bool const hasCurrent = set->hasByName(currentName);
    bool const hasMigrated = set->hasByName(migratedName);
    if (hasCurrent && !hasMigrated) {
        auto const elem = set->getByName(currentName);
        set->removeByName(currentName);
        set->insertByName(migratedName, elem);
    } else {
        SAL_INFO_IF(!hasCurrent, "desktop.migration", "unexpectedly missing " << currentName);
        SAL_INFO_IF(hasMigrated, "desktop.migration", "unexpectedly present " << migratedName);
    }
}

void renameMigratedSetElementBack(
    css::uno::Reference<css::container::XNameContainer> const & set, OUString const & currentName,
    OUString const & migratedName)
{
    // To avoid unexpected data loss, the code is careful to ensure that in the end a currentName
    // element exists, creating it from a template if the migratedName element had unexpectedly gone
    // missing:
    bool const hasMigrated = set->hasByName(migratedName);
    css::uno::Any elem;
    if (hasMigrated) {
        elem = set->getByName(migratedName);
        set->removeByName(migratedName);
    } else {
        SAL_INFO("desktop.migration", "unexpected loss of " << migratedName);
        elem <<= css::uno::Reference<css::lang::XSingleServiceFactory>(
            set, css::uno::UNO_QUERY_THROW)->createInstance();
    }
    if (set->hasByName(currentName)) {
        SAL_INFO("desktop.migration", "unexpected reappearance of " << currentName);
        if (hasMigrated) {
            SAL_INFO(
                "desktop.migration",
                "reappeared " << currentName << " overwritten with " << migratedName);
            set->replaceByName(currentName, elem);
        }
    } else {
        set->insertByName(currentName, elem);
    }
}

}

void MigrationImpl::copyConfig()
{
    Components comps;
    for (auto const& rMigrationStep : *m_vrMigrations) {
        for (const OUString& rIncludePath : rMigrationStep.includeConfig) {
            OUString comp;
            if (getComponent(rIncludePath, &comp)) {
                comps[comp].includedPaths.insert(rIncludePath);
            }
        }
        for (const OUString& rExcludePath : rMigrationStep.excludeConfig) {
            OUString comp;
            if (getComponent(rExcludePath, &comp)) {
                comps[comp].excludedPaths.insert(rExcludePath);
            }
        }
    }

    // check if the shared registrymodifications.xcu file exists
    bool bRegistryModificationsXcuExists = false;
    OUString regFilePath = m_aInfo.userdata + "/user/registrymodifications.xcu";
    File regFile(regFilePath);
    ::osl::FileBase::RC nError = regFile.open(osl_File_OpenFlag_Read);
    if ( nError == ::osl::FileBase::E_None ) {
        bRegistryModificationsXcuExists = true;
        regFile.close();
    }

    // If the to-be-migrated data contains modifications of
    // /org.openoffice.Office.UI/ColorScheme/ColorSchemes set elements named after the migrated
    // product name, those modifications must instead be made to the corresponding set elements
    // named after the current product name.  However, if the current configuration data does not
    // contain those old-named set elements at all, their modification data would silently be
    // ignored by css.configuration.XUpdate::insertModificationXcuFile.  So temporarily rename any
    // new-named set elements to their old-named counterparts here, and rename them back again down
    // below after importing the migrated data:
    OUString sProductName = utl::ConfigManager::getProductName();
    OUString sProductNameDark = sProductName + " Dark";
    OUString sMigratedProductName = m_aInfo.productname;
    // remove version number from the end of product name if there’s one
    if (isdigit(sMigratedProductName[sMigratedProductName.getLength() - 1]))
        sMigratedProductName = (sMigratedProductName.copy(0, m_aInfo.productname.getLength() - 1)).trim();
    OUString sMigratedProductNameDark = sMigratedProductName + " Dark";
    auto const tempRename = sMigratedProductName != sProductName;
    if (tempRename) {
        auto const batch = comphelper::ConfigurationChanges::create();
        auto const schemes = officecfg::Office::UI::ColorScheme::ColorSchemes::get(batch);
        renameMigratedSetElementTo(schemes, sProductName, sMigratedProductName);
        renameMigratedSetElementTo(schemes, sProductNameDark, sMigratedProductNameDark);
        batch->commit();
    }

    for (auto const& comp : comps)
    {
        if (!comp.second.includedPaths.empty()) {
            if (!bRegistryModificationsXcuExists) {
                // shared registrymodifications.xcu does not exists
                // the configuration is split in many registry files
                // determine the file names from the first element in included paths
                OUStringBuffer buf(m_aInfo.userdata
                    + "/user/registry/data");
                sal_Int32 n = 0;
                do {
                    OUString seg(comp.first.getToken(0, '.', n));
                    OUString enc(
                        rtl::Uri::encode(
                            seg, rtl_UriCharClassPchar, rtl_UriEncodeStrict,
                            RTL_TEXTENCODING_UTF8));
                    if (enc.isEmpty() && !seg.isEmpty()) {
                        SAL_INFO( "desktop.migration", "configuration migration component " << comp.first << " ignored (cannot be encoded as file path)" );
                        goto next;
                    }
                    buf.append("/" + enc);
                } while (n >= 0);
                buf.append(".xcu");
                regFilePath = buf.makeStringAndClear();
            }
            configuration::Update::get(
                comphelper::getProcessComponentContext())->
            insertModificationXcuFile(
                regFilePath,
                comphelper::containerToSequence(comp.second.includedPaths),
                comphelper::containerToSequence(comp.second.excludedPaths));

        } else {
            SAL_INFO( "desktop.migration", "configuration migration component " << comp.first << " ignored (only excludes, no includes)" );
        }
next:
        ;
    }
    if (tempRename) {
        auto const batch = comphelper::ConfigurationChanges::create();
        auto const schemes = officecfg::Office::UI::ColorScheme::ColorSchemes::get(batch);
        renameMigratedSetElementBack(schemes, sProductName, sMigratedProductName);
        renameMigratedSetElementBack(schemes, sProductNameDark, sMigratedProductNameDark);
        batch->commit();
    }
    // checking the migrated (product name related) color scheme name, and replace it to the current version scheme name
    try
    {
        OUString sMigratedColorScheme;
        uno::Reference<XPropertySet> aPropertySet(
            getConfigAccess("org.openoffice.Office.UI/ColorScheme", true), uno::UNO_QUERY_THROW);
        if (aPropertySet->getPropertyValue(u"CurrentColorScheme"_ustr) >>= sMigratedColorScheme)
        {
            if (sMigratedColorScheme.equals(sMigratedProductName))
            {
                aPropertySet->setPropertyValue(u"CurrentColorScheme"_ustr,
                                               uno::Any(sProductName));
                uno::Reference<XChangesBatch>(aPropertySet, uno::UNO_QUERY_THROW)->commitChanges();
            }
            else if (sMigratedColorScheme.equals(sMigratedProductNameDark))
            {
                aPropertySet->setPropertyValue(u"CurrentColorScheme"_ustr,
                                               uno::Any(sProductNameDark));
                uno::Reference<XChangesBatch>(aPropertySet, uno::UNO_QUERY_THROW)->commitChanges();
            }
        }
    } catch (const Exception&) {
        // fail silently...
    }
}

uno::Reference< XNameAccess > MigrationImpl::getConfigAccess(const char* pPath, bool bUpdate)
{
    uno::Reference< XNameAccess > xNameAccess;
    try {
        OUString sAccessSrvc;
        if (bUpdate)
            sAccessSrvc = "com.sun.star.configuration.ConfigurationUpdateAccess";
        else
            sAccessSrvc = "com.sun.star.configuration.ConfigurationAccess";

        OUString sConfigURL = OUString::createFromAscii(pPath);

        uno::Reference< XMultiServiceFactory > theConfigProvider(
            configuration::theDefaultProvider::get(
                comphelper::getProcessComponentContext()));

        // access the provider
        uno::Sequence< uno::Any > theArgs {uno::Any(sConfigURL)};
        xNameAccess.set(
            theConfigProvider->createInstanceWithArguments(
                sAccessSrvc, theArgs ), uno::UNO_QUERY_THROW );
    } catch (const css::uno::Exception&) {
        TOOLS_WARN_EXCEPTION("desktop.migration", "ignoring");
    }
    return xNameAccess;
}

void MigrationImpl::copyFiles()
{
    OUString localName;
    OUString destName;
    OUString userInstall;
    utl::Bootstrap::PathStatus aStatus;
    aStatus = utl::Bootstrap::locateUserInstallation(userInstall);
    if (aStatus == utl::Bootstrap::PATH_EXISTS) {
        for (auto const& rFile : *m_vrFileList)
        {
            // remove installation prefix from file
            localName = rFile.copy(m_aInfo.userdata.getLength());
            if (localName.endsWith( "/autocorr/acor_.dat")) {
                // Previous versions used an empty language tag for
                // LANGUAGE_DONTKNOW with the "[All]" autocorrection entry.
                // As of LibreOffice 4.0 it is 'und' for LANGUAGE_UNDETERMINED
                // so the file name is "acor_und.dat".
                localName = OUString::Concat(localName.subView( 0, localName.getLength() - 4)) + "und.dat";
            }
            destName = userInstall + localName;
            INetURLObject aURL(destName);
            // check whether destination directory exists
            aURL.removeSegment();
            _checkAndCreateDirectory(aURL);
            FileBase::RC copyResult = File::copy(rFile, destName);
            if (copyResult != FileBase::E_None) {
                SAL_WARN( "desktop", "Cannot copy " << rFile <<  " to " << destName);
            }
        }
    } else {
        OSL_FAIL("copyFiles: UserInstall does not exist");
    }
}

void MigrationImpl::runServices()
{
    // Build argument array
    uno::Sequence< uno::Any > seqArguments(3);
    auto pseqArguments = seqArguments.getArray();
    pseqArguments[0] <<= NamedValue(u"Productname"_ustr,
                                   uno::Any(m_aInfo.productname));
    pseqArguments[1] <<= NamedValue(u"UserData"_ustr,
                                   uno::Any(m_aInfo.userdata));


    // create an instance of every migration service
    // and execute the migration job
    uno::Reference< XJob > xMigrationJob;

    const uno::Reference< uno::XComponentContext >& xContext(comphelper::getProcessComponentContext());
    for (auto const& rMigration : *m_vrMigrations)
    {
        if( !rMigration.service.isEmpty()) {

            try {
                // set black list for extension migration
                uno::Sequence< OUString > seqExtDenyList;
                sal_uInt32 nSize = rMigration.excludeExtensions.size();
                if ( nSize > 0 )
                    seqExtDenyList = comphelper::arrayToSequence< OUString >(
                                          rMigration.excludeExtensions.data(), nSize );
                pseqArguments[2] <<= NamedValue(u"ExtensionDenyList"_ustr,
                                               uno::Any( seqExtDenyList ));

                xMigrationJob.set(
                    xContext->getServiceManager()->createInstanceWithArgumentsAndContext(rMigration.service, seqArguments, xContext),
                    uno::UNO_QUERY_THROW);

                xMigrationJob->execute(uno::Sequence< NamedValue >());


            } catch (const Exception&) {
                TOOLS_WARN_EXCEPTION( "desktop", "Execution of migration service failed. Service: "
                            << rMigration.service);
            } catch (...) {
                SAL_WARN( "desktop", "Execution of migration service failed (Exception caught).\nService: "
                            << rMigration.service << "\nNo message available");
            }

        }
    }
}

std::vector< MigrationModuleInfo > MigrationImpl::detectUIChangesForAllModules() const
{
    std::vector< MigrationModuleInfo > vModulesInfo;
    static constexpr OUStringLiteral MENUBAR(u"menubar");
    static constexpr OUStringLiteral TOOLBAR(u"toolbar");

    uno::Sequence< uno::Any > lArgs {uno::Any(m_aInfo.userdata + "/user/config/soffice.cfg/modules"),
                                     uno::Any(embed::ElementModes::READ)};

    uno::Reference< lang::XSingleServiceFactory > xStorageFactory(
        embed::FileSystemStorageFactory::create(comphelper::getProcessComponentContext()));
    uno::Reference< embed::XStorage >             xModules;

    xModules.set(xStorageFactory->createInstanceWithArguments(lArgs), uno::UNO_QUERY);
    if (!xModules.is())
        return vModulesInfo;

    uno::Sequence< OUString > lNames = xModules->getElementNames();
    sal_Int32 nLength = lNames.getLength();
    for (sal_Int32 i=0; i<nLength; ++i) {
        const OUString& sModuleShortName = lNames[i];
        uno::Reference< embed::XStorage > xModule = xModules->openStorageElement(sModuleShortName, embed::ElementModes::READ);
        if (xModule.is()) {
            MigrationModuleInfo aModuleInfo;

            uno::Reference< embed::XStorage > xMenubar = xModule->openStorageElement(MENUBAR, embed::ElementModes::READ);
            if (xMenubar.is()) {
                if (xMenubar->getElementNames().hasElements()) {
                    aModuleInfo.sModuleShortName = sModuleShortName;
                    aModuleInfo.bHasMenubar = true;
                }
            }

            uno::Reference< embed::XStorage > xToolbar = xModule->openStorageElement(TOOLBAR, embed::ElementModes::READ);
            if (xToolbar.is()) {
                const ::uno::Sequence< OUString > lToolbars = xToolbar->getElementNames();
                for (OUString const & sToolbarName : lToolbars) {
                    if (sToolbarName.startsWith("custom_"))
                        continue;

                    aModuleInfo.sModuleShortName = sModuleShortName;
                    sal_Int32 nIndex = sToolbarName.lastIndexOf('.');
                    if (nIndex > 0) {
                        std::u16string_view sExtension(sToolbarName.subView(nIndex));
                        OUString sToolbarResourceName(sToolbarName.copy(0, nIndex));
                        if (!sToolbarResourceName.isEmpty() && sExtension == u".xml")
                            aModuleInfo.m_vToolbars.push_back(sToolbarResourceName);
                    }
                }
            }

            if (!aModuleInfo.sModuleShortName.isEmpty())
                vModulesInfo.push_back(aModuleInfo);
        }
    }

    return vModulesInfo;
}

void MigrationImpl::compareOldAndNewConfig(const OUString& sParent,
        const uno::Reference< container::XIndexContainer >& xIndexOld,
        const uno::Reference< container::XIndexContainer >& xIndexNew,
        const OUString& sResourceURL)
{
    static constexpr OUStringLiteral MENU_SEPARATOR(u" | ");

    std::vector< MigrationItem > vOldItems;
    std::vector< MigrationItem > vNewItems;
    uno::Sequence< beans::PropertyValue > aProps;
    sal_Int32 nOldCount = xIndexOld->getCount();
    sal_Int32 nNewCount = xIndexNew->getCount();

    for (int n=0; n<nOldCount; ++n) {
        MigrationItem aMigrationItem;
        if (xIndexOld->getByIndex(n) >>= aProps) {
            for(beans::PropertyValue const & prop : aProps) {
                if ( prop.Name == ITEM_DESCRIPTOR_COMMANDURL )
                    prop.Value >>= aMigrationItem.m_sCommandURL;
                else if ( prop.Name == ITEM_DESCRIPTOR_CONTAINER )
                    prop.Value >>= aMigrationItem.m_xPopupMenu;
            }

            if (!aMigrationItem.m_sCommandURL.isEmpty())
                vOldItems.push_back(aMigrationItem);
        }
    }

    for (int n=0; n<nNewCount; ++n) {
        MigrationItem aMigrationItem;
        if (xIndexNew->getByIndex(n) >>= aProps) {
            for(beans::PropertyValue const & prop : aProps) {
                if ( prop.Name == ITEM_DESCRIPTOR_COMMANDURL )
                    prop.Value >>= aMigrationItem.m_sCommandURL;
                else if ( prop.Name == ITEM_DESCRIPTOR_CONTAINER )
                    prop.Value >>= aMigrationItem.m_xPopupMenu;
            }

            if (!aMigrationItem.m_sCommandURL.isEmpty())
                vNewItems.push_back(aMigrationItem);
        }
    }

    OUString sSibling;
    for (auto const& oldItem : vOldItems)
    {
        std::vector< MigrationItem >::iterator pFound = std::find(vNewItems.begin(), vNewItems.end(), oldItem);
        if (pFound != vNewItems.end() && oldItem.m_xPopupMenu.is()) {
            OUString sName;
            if (!sParent.isEmpty())
                sName = sParent + MENU_SEPARATOR + oldItem.m_sCommandURL;
            else
                sName = oldItem.m_sCommandURL;
            compareOldAndNewConfig(sName, oldItem.m_xPopupMenu, pFound->m_xPopupMenu, sResourceURL);
        } else if (pFound == vNewItems.end()) {
            MigrationItem aMigrationItem(sParent, sSibling, oldItem.m_sCommandURL, oldItem.m_xPopupMenu);
            if (m_aOldVersionItemsHashMap.find(sResourceURL)==m_aOldVersionItemsHashMap.end()) {
                std::vector< MigrationItem > vMigrationItems;
                m_aOldVersionItemsHashMap.emplace(sResourceURL, vMigrationItems);
                m_aOldVersionItemsHashMap[sResourceURL].push_back(aMigrationItem);
            } else {
                if (std::find(m_aOldVersionItemsHashMap[sResourceURL].begin(), m_aOldVersionItemsHashMap[sResourceURL].end(), aMigrationItem)==m_aOldVersionItemsHashMap[sResourceURL].end())
                    m_aOldVersionItemsHashMap[sResourceURL].push_back(aMigrationItem);
            }
        }

        sSibling = oldItem.m_sCommandURL;
    }
}

void MigrationImpl::mergeOldToNewVersion(const uno::Reference< ui::XUIConfigurationManager >& xCfgManager,
        const uno::Reference< container::XIndexContainer>& xIndexContainer,
        const OUString& sModuleIdentifier,
        const OUString& sResourceURL)
{
    MigrationHashMap::iterator pFound = m_aOldVersionItemsHashMap.find(sResourceURL);
    if (pFound==m_aOldVersionItemsHashMap.end())
        return;

    for (auto const& elem : pFound->second)
    {
        uno::Reference< container::XIndexContainer > xTemp = xIndexContainer;

        OUString sParentNodeName = elem.m_sParentNodeName;
        sal_Int32 nIndex = 0;
        do {
            std::u16string_view sToken( o3tl::trim(o3tl::getToken(sParentNodeName, 0, '|', nIndex)) );
            if (sToken.empty())
                break;

            sal_Int32 nCount = xTemp->getCount();
            for (sal_Int32 i=0; i<nCount; ++i) {
                OUString sCommandURL;
                OUString sLabel;
                uno::Reference< container::XIndexContainer > xChild;

                uno::Sequence< beans::PropertyValue > aPropSeq;
                xTemp->getByIndex(i) >>= aPropSeq;
                for (beans::PropertyValue const & prop : aPropSeq) {
                    OUString sPropName = prop.Name;
                    if ( sPropName == ITEM_DESCRIPTOR_COMMANDURL )
                        prop.Value >>= sCommandURL;
                    else if ( sPropName == ITEM_DESCRIPTOR_LABEL )
                        prop.Value >>= sLabel;
                    else if ( sPropName == ITEM_DESCRIPTOR_CONTAINER )
                        prop.Value >>= xChild;
                }

                if (sCommandURL == sToken) {
                    xTemp = std::move(xChild);
                    break;
                }
            }

        } while (nIndex >= 0);

        if (nIndex == -1) {
            auto aProperties = vcl::CommandInfoProvider::GetCommandProperties(elem.m_sCommandURL, sModuleIdentifier);
            uno::Sequence< beans::PropertyValue > aPropSeq {
                beans::PropertyValue(ITEM_DESCRIPTOR_COMMANDURL, 0, uno::Any(elem.m_sCommandURL), beans::PropertyState_DIRECT_VALUE),
                beans::PropertyValue(ITEM_DESCRIPTOR_LABEL, 0, uno::Any(vcl::CommandInfoProvider::GetLabelForCommand(aProperties)), beans::PropertyState_DIRECT_VALUE),
                beans::PropertyValue(ITEM_DESCRIPTOR_CONTAINER, 0, uno::Any(elem.m_xPopupMenu), beans::PropertyState_DIRECT_VALUE)
            };

            if (elem.m_sPrevSibling.isEmpty())
                xTemp->insertByIndex(0, uno::Any(aPropSeq));
            else {
                sal_Int32 nCount = xTemp->getCount();
                sal_Int32 i = 0;
                for (; i<nCount; ++i) {
                    OUString sCmd;
                    uno::Sequence< beans::PropertyValue > aTempPropSeq;
                    xTemp->getByIndex(i) >>= aTempPropSeq;
                    for (beans::PropertyValue const & prop : aTempPropSeq) {
                        if ( prop.Name == ITEM_DESCRIPTOR_COMMANDURL ) {
                            prop.Value >>= sCmd;
                            break;
                        }
                    }

                    if (sCmd == elem.m_sPrevSibling)
                        break;
                }

                xTemp->insertByIndex(i+1, uno::Any(aPropSeq));
            }
        }
    }

    if (xIndexContainer.is())
        xCfgManager->replaceSettings(sResourceURL, xIndexContainer);

    uno::Reference< ui::XUIConfigurationPersistence > xUIConfigurationPersistence(xCfgManager, uno::UNO_QUERY);
    if (xUIConfigurationPersistence.is())
        xUIConfigurationPersistence->store();
}

uno::Reference< ui::XUIConfigurationManager > NewVersionUIInfo::getConfigManager(std::u16string_view sModuleShortName) const
{
    uno::Reference< ui::XUIConfigurationManager > xCfgManager;

    for ( const css::beans::PropertyValue& rProp : m_lCfgManagerSeq) {
        if (rProp.Name == sModuleShortName) {
            rProp.Value >>= xCfgManager;
            break;
        }
    }

    return xCfgManager;
}

uno::Reference< container::XIndexContainer > NewVersionUIInfo::getNewMenubarSettings(std::u16string_view sModuleShortName) const
{
    uno::Reference< container::XIndexContainer > xNewMenuSettings;

    for (auto const & prop : m_lNewVersionMenubarSettingsSeq) {
        if (prop.Name == sModuleShortName) {
            prop.Value >>= xNewMenuSettings;
            break;
        }
    }

    return xNewMenuSettings;
}

uno::Reference< container::XIndexContainer > NewVersionUIInfo::getNewToolbarSettings(std::u16string_view sModuleShortName, std::u16string_view sToolbarName) const
{
    uno::Reference< container::XIndexContainer > xNewToolbarSettings;

    for (auto const & newProp : m_lNewVersionToolbarSettingsSeq) {
        if (newProp.Name == sModuleShortName) {
            uno::Sequence< beans::PropertyValue > lToolbarSettingsSeq;
            newProp.Value >>= lToolbarSettingsSeq;
            for (auto const & prop : lToolbarSettingsSeq) {
                if (prop.Name == sToolbarName) {
                    prop.Value >>= xNewToolbarSettings;
                    break;
                }
            }

            break;
        }
    }

    return xNewToolbarSettings;
}

void NewVersionUIInfo::init(const std::vector< MigrationModuleInfo >& vModulesInfo)
{
    m_lCfgManagerSeq.resize(vModulesInfo.size());
    m_lNewVersionMenubarSettingsSeq.realloc(vModulesInfo.size());
    auto p_lNewVersionMenubarSettingsSeq = m_lNewVersionMenubarSettingsSeq.getArray();
    m_lNewVersionToolbarSettingsSeq.realloc(vModulesInfo.size());
    auto p_lNewVersionToolbarSettingsSeq = m_lNewVersionToolbarSettingsSeq.getArray();

    static constexpr OUStringLiteral sMenubarResourceURL(u"private:resource/menubar/menubar");
    static constexpr OUStringLiteral sToolbarResourcePre(u"private:resource/toolbar/");

    uno::Reference< ui::XModuleUIConfigurationManagerSupplier > xModuleCfgSupplier = ui::theModuleUIConfigurationManagerSupplier::get( ::comphelper::getProcessComponentContext() );

    for (size_t i=0; i<vModulesInfo.size(); ++i) {
        OUString sModuleIdentifier = mapModuleShortNameToIdentifier(vModulesInfo[i].sModuleShortName);
        if (!sModuleIdentifier.isEmpty()) {
            uno::Reference< ui::XUIConfigurationManager > xCfgManager = xModuleCfgSupplier->getUIConfigurationManager(sModuleIdentifier);
            m_lCfgManagerSeq[i].Name = vModulesInfo[i].sModuleShortName;
            m_lCfgManagerSeq[i].Value <<= xCfgManager;

            if (vModulesInfo[i].bHasMenubar) {
                p_lNewVersionMenubarSettingsSeq[i].Name = vModulesInfo[i].sModuleShortName;
                p_lNewVersionMenubarSettingsSeq[i].Value <<= xCfgManager->getSettings(sMenubarResourceURL, true);
            }

            sal_Int32 nToolbars = vModulesInfo[i].m_vToolbars.size();
            if (nToolbars > 0) {
                uno::Sequence< beans::PropertyValue > lPropSeq(nToolbars);
                auto plPropSeq = lPropSeq.getArray();
                for (sal_Int32 j=0; j<nToolbars; ++j) {
                    OUString sToolbarName = vModulesInfo[i].m_vToolbars[j];
                    OUString sToolbarResourceURL = sToolbarResourcePre + sToolbarName;

                    plPropSeq[j].Name = sToolbarName;
                    plPropSeq[j].Value <<= xCfgManager->getSettings(sToolbarResourceURL, true);
                }

                p_lNewVersionToolbarSettingsSeq[i].Name = vModulesInfo[i].sModuleShortName;
                p_lNewVersionToolbarSettingsSeq[i].Value <<= lPropSeq;
            }
        }
    }
}

} // namespace desktop

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
