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

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <map>
#include <vector>
#include <algorithm>

#include <o3tl/string_view.hxx>
#include <osl/file.h>
#include <osl/file.hxx>
#include <osl/thread.h>
#include <rtl/string.h>
#include <rtl/string.hxx>
#include <rtl/textcvt.h>
#include <rtl/strbuf.hxx>
#include <rtl/ustring.h>
#include <rtl/ustring.hxx>
#include <sal/macros.h>
#include <sal/main.h>
#include <sal/types.h>

#include <po.hxx>

namespace {

OString libraryPathEnvVarOverride;

bool matchList(
    std::u16string_view rUrl, const std::u16string_view* pList, size_t nLength)
{
    for (size_t i = 0; i != nLength; ++i) {
        if (o3tl::ends_with(rUrl, pList[i])) {
            return true;
        }
    }
    return false;
}

bool passesNegativeList(std::u16string_view rUrl) {
    static const std::u16string_view list[] = {
        u"/desktop/test/deployment/passive/help/en/help.tree",
        u"/desktop/test/deployment/passive/help/en/main.xhp",
        u"/dictionaries.xcu",
        u"/dictionaries/da_DK/help/da/help.tree",
        (u"/dictionaries/da_DK/help/da/"
         "org.openoffice.da.hunspell.dictionaries/page1.xhp"),
        (u"/dictionaries/da_DK/help/da/"
         "org.openoffice.da.hunspell.dictionaries/page2.xhp"),
        u"/dictionaries/hu_HU/help/hu/help.tree",
        (u"/dictionaries/hu_HU/help/hu/"
         "org.openoffice.hu.hunspell.dictionaries/page1.xhp"),
        u"/officecfg/registry/data/org/openoffice/Office/Accelerators.xcu"
    };
    return !matchList(rUrl, list, SAL_N_ELEMENTS(list));
}

bool passesPositiveList(std::u16string_view rUrl) {
    static const std::u16string_view list[] = {
        u"/description.xml"
    };
    return matchList(rUrl, list, SAL_N_ELEMENTS(list));
}

void handleCommand(
    std::string_view rInPath, std::string_view rOutPath,
    const std::string& rExecutable)
{
    OStringBuffer buf;
    if (rExecutable == "uiex" || rExecutable == "hrcex")
    {
#if !defined _WIN32
        // For now, this is only needed by some Linux ASan builds, so keep it simply and disable it
        // on  Windows (which doesn't support the relevant shell syntax for (un-)setting environment
        // variables).
        auto const n = libraryPathEnvVarOverride.indexOf('=');
        if (n == -1) {
            buf.append("unset -v " + libraryPathEnvVarOverride + " && ");
        } else {
            buf.append(libraryPathEnvVarOverride + " ");
        }
#endif
        auto const env = getenv("SRC_ROOT");
        assert(env != nullptr);
        buf.append(OString::Concat(env) + "/solenv/bin/");
    }
    else
    {
#if defined MACOSX
        if (auto const env = getenv("DYLD_LIBRARY_PATH")) {
            buf.append(OString::Concat("DYLD_LIBRARY_PATH=") + env + " ");
        }
#endif
        auto const env = getenv("WORKDIR_FOR_BUILD");
        assert(env != nullptr);
        buf.append(OString::Concat(env) + "/LinkTarget/Executable/");
    }
    buf.append(OString::Concat(std::string_view(rExecutable))
        + " -i " + rInPath + " -o " + rOutPath);

    if (system(buf.getStr()) != 0)
    {
        std::cerr << "Error: Failed to execute " << buf.getStr() << '\n';
        throw false; //TODO
    }
}

void InitPoFile(
    std::string_view rProject, std::string_view rInPath,
    std::string_view rPotDir, const OString& rOutPath )
{
    //Create directory for po file
    {
        OUString outDir =
            OStringToOUString(
                rPotDir.substr(0,rPotDir.rfind('/')), RTL_TEXTENCODING_UTF8);
        OUString outDirUrl;
        if (osl::FileBase::getFileURLFromSystemPath(outDir, outDirUrl)
            != osl::FileBase::E_None)
        {
            std::cerr
                << ("Error: Cannot convert pathname to URL in " __FILE__
                    ", in line ")
                << __LINE__ << "\n       outDir: "
                << outDir
                << "\n";
            throw false; //TODO
        }
        osl::Directory::createPath(outDirUrl);
    }

    //Add header to the po file
    PoOfstream aPoOutPut;
    aPoOutPut.open(rOutPath);
    if (!aPoOutPut.isOpen())
    {
        std::cerr
            << "Error: Cannot open po file "
            << rOutPath << "\n";
        throw false; //TODO
    }

    const size_t nProjectInd = rInPath.find(rProject);
    const std::string_view relativPath =
        rInPath.substr(nProjectInd, rInPath.rfind('/')- nProjectInd);

    PoHeader aTmp(relativPath);
    aPoOutPut.writeHeader(aTmp);
    aPoOutPut.close();
}

bool fileExists(const OString& fileName)
{
    FILE *f = fopen(fileName.getStr(), "r");

    if (f != nullptr)
    {
        fclose(f);
        return true;
    }

    return false;
}

OString gDestRoot;

bool handleFile(std::string_view rProject, const OUString& rUrl, std::string_view rPotDir)
{
    struct Command {
        std::u16string_view extension;
        std::string executable;
        bool positive;
    };
    static Command const commands[] = {
        { std::u16string_view(u".hrc"), "hrcex", false },
        { std::u16string_view(u".ulf"), "ulfex", false },
        { std::u16string_view(u".xcu"), "cfgex", false },
        { std::u16string_view(u".xrm"), "xrmex", false },
        { std::u16string_view(u"description.xml"), "xrmex", true },
        { std::u16string_view(u".xhp"), "helpex", false },
        { std::u16string_view(u".properties"), "propex", false },
        { std::u16string_view(u".ui"), "uiex", false },
        { std::u16string_view(u".tree"), "treex", false } };
    for (size_t i = 0; i != std::size(commands); ++i)
    {
        if (rUrl.endsWith(commands[i].extension) &&
            (commands[i].executable != "propex" || rUrl.indexOf("en_US") != -1))
        {
            if (commands[i].positive ? passesPositiveList(rUrl) : passesNegativeList(rUrl))
            {
                //Get input file path
                OString sInPath;
                {
                    OUString sInPathTmp;
                    if (osl::FileBase::getSystemPathFromFileURL(rUrl, sInPathTmp) !=
                        osl::FileBase::E_None)
                    {
                        std::cerr << "osl::FileBase::getSystemPathFromFileURL(" << rUrl << ") failed\n";
                        throw false; //TODO
                    }
                    sInPath = OUStringToOString( sInPathTmp, RTL_TEXTENCODING_UTF8 );
                }
                OString sOutPath;
                bool bCreatedFile = false;
                bool bSimpleModuleCase = commands[i].executable == "uiex" || commands[i].executable == "hrcex";
                if (bSimpleModuleCase)
                    sOutPath = gDestRoot + "/" + rProject + "/messages.pot";
                else
                    sOutPath = OString::Concat(rPotDir) + ".pot";

                if (!fileExists(sOutPath))
                {
                    InitPoFile(rProject, sInPath, rPotDir, sOutPath);
                    bCreatedFile = true;
                }
                handleCommand(sInPath, sOutPath, commands[i].executable);

                {
                    //Delete pot file if it contain only the header
                    PoIfstream aPOStream(sOutPath);
                    PoEntry aPO;
                    aPOStream.readEntry( aPO );
                    bool bDel = aPOStream.eof();
                    aPOStream.close();

                    if (bDel)
                    {
                        if ( system(OString("rm " + sOutPath).getStr()) != 0 )
                        {
                            std::cerr
                                << "Error: Cannot remove entryless pot file: "
                                << sOutPath << "\n";
                            throw false; //TODO
                        }
                    }
                    else if (bCreatedFile && bSimpleModuleCase)
                    {
                        // add one stock Add, Cancel, Close, Help, No, OK, Yes entry to each module.po
                        // and duplicates in .ui files then filtered out by solenv/bin/uiex

                        std::ofstream aOutPut;
                        aOutPut.open(sOutPath.getStr(), std::ios_base::out | std::ios_base::app);

                        aOutPut << "#. wH3TZ\nmsgctxt \"stock\"\nmsgid \"_Add\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. S9dsC\nmsgctxt \"stock\"\nmsgid \"_Apply\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. TMo6G\nmsgctxt \"stock\"\nmsgid \"_Cancel\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. MRCkv\nmsgctxt \"stock\"\nmsgid \"_Close\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. nvx5t\nmsgctxt \"stock\"\nmsgid \"_Delete\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. YspCj\nmsgctxt \"stock\"\nmsgid \"_Edit\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. imQxr\nmsgctxt \"stock\"\nmsgid \"_Help\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. RbjyB\nmsgctxt \"stock\"\nmsgid \"_New\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. dx2yy\nmsgctxt \"stock\"\nmsgid \"_No\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. M9DsL\nmsgctxt \"stock\"\nmsgid \"_OK\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. VtJS9\nmsgctxt \"stock\"\nmsgid \"_Remove\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. C69Fy\nmsgctxt \"stock\"\nmsgid \"_Reset\"\nmsgstr \"\"\n\n";
                        aOutPut << "#. mgpxh\nmsgctxt \"stock\"\nmsgid \"_Yes\"\nmsgstr \"\"\n";

                        aOutPut.close();
                    }
                }


                return true;
            }
            break;
        }
    }
    return false;
}

void handleFilesOfDir(
    std::vector<OUString>& aFiles, std::string_view rProject,
    std::string_view rPotDir )
{
    ///Handle files in lexical order
    std::sort(aFiles.begin(), aFiles.end());

    for (auto const& elem : aFiles)
        handleFile(rProject, elem, rPotDir);
}

bool includeProject(std::string_view rProject) {
    static const char *projects[] = {
        "include",
        "accessibility",
        "avmedia",
        "basctl",
        "basic",
        "chart2",
        "connectivity",
        "cui",
        "dbaccess",
        "desktop",
        "dictionaries",
        "editeng",
        "extensions",
        "extras",
        "filter",
        "forms",
        "formula",
        "fpicker",
        "framework",
        "helpcontent2",
        "instsetoo_native",
        "librelogo",
        "mysqlc",
        "nlpsolver",
        "officecfg",
        "oox",
        "readlicense_oo",
        "reportbuilder",
        "reportdesign",
        "sc",
        "scaddins",
        "sccomp",
        "scp2",
        "sd",
        "sdext",
        "setup_native",
        "sfx2",
        "shell",
        "starmath",
        "svl",
        "svtools",
        "svx",
        "sw",
        "swext",
        "sysui",
        "uui",
        "vcl",
        "wizards",
        "writerperfect",
        "xmlsecurity" };
    for (size_t i = 0; i != SAL_N_ELEMENTS(projects); ++i) {
        if (rProject == projects[i]) {
            return true;
        }
    }
    return false;
}

/// Handle one directory in the hierarchy.
///
/// Ignores symlinks and instead explicitly descends into clone/* or src/*,
/// as the Cygwin symlinks are not supported by osl::Directory on Windows.
///
/// @param rUrl the absolute file URL of this directory
///
/// @param nLevel 0 if this is the root directory (core repository)
/// that contains the individual modules. 1 if it is a toplevel module and
/// larger values for the subdirectories.
///
/// @param rProject the name of the project (empty and ignored if nLevel <= 0)
/// @param rPotDir the path of pot directory
void handleDirectory(
    const OUString& rUrl, int nLevel,
    const OString& rProject, const OString& rPotDir)
{
    osl::Directory dir(rUrl);
    if (dir.open() != osl::FileBase::E_None) {
        std::cerr
            << "Error: Cannot open directory: " << rUrl << '\n';
        throw false; //TODO
    }
    std::vector<OUString> aFileNames;
    std::map<OUString, std::map<OString, OString>> aSubDirs;
    for (;;) {
        osl::DirectoryItem item;
        osl::FileBase::RC e = dir.getNextItem(item);
        if (e == osl::FileBase::E_NOENT) {
            break;
        }
        if (e != osl::FileBase::E_None) {
            std::cerr << "Error: Cannot read directory\n";
            throw false; //TODO
        }
        osl::FileStatus stat(
            osl_FileStatus_Mask_Type | osl_FileStatus_Mask_FileName
            | osl_FileStatus_Mask_FileURL);
        if (item.getFileStatus(stat) != osl::FileBase::E_None) {
            std::cerr << "Error: Cannot get file status\n";
            throw false; //TODO
        }
        const OString sDirName =
            OUStringToOString(stat.getFileName(),RTL_TEXTENCODING_UTF8);
        switch (nLevel)
        {
            case 0: // a root directory
                if (stat.getFileType() == osl::FileStatus::Directory && includeProject(sDirName))
                    aSubDirs[stat.getFileURL()][sDirName] = rPotDir + "/" + sDirName;
                break;
            default:
                if (stat.getFileType() == osl::FileStatus::Directory)
                    aSubDirs[stat.getFileURL()][rProject] = rPotDir + "/" + sDirName;
                else
                    aFileNames.push_back(stat.getFileURL());
                break;
        }
    }

    OString aPotDir(rPotDir);
    if( !aFileNames.empty() )
    {
        OString aProject(rProject);
        if (aProject == "include" && nLevel > 1)
        {
            aProject = aPotDir.copy(aPotDir.lastIndexOf('/') + 1);
            aPotDir = aPotDir.subView(0, aPotDir.lastIndexOf("include")) + aProject + "/messages";
        }
        if (aProject != "include")
        {
            handleFilesOfDir(aFileNames, aProject, aPotDir);
        }
    }

    if (dir.close() != osl::FileBase::E_None) {
        std::cerr << "Error: Cannot close directory\n";
        throw false; //TODO
    }

    for (auto const& elem : aSubDirs)
        handleDirectory(elem.first, nLevel + 1, elem.second.begin()->first,
                        elem.second.begin()->second);

    //Remove empty pot directory
    OUString sPoPath =
        OStringToOUString(
            aPotDir.subView(0,aPotDir.lastIndexOf('/')), RTL_TEXTENCODING_UTF8);
    OUString sPoUrl;
    if (osl::FileBase::getFileURLFromSystemPath(sPoPath, sPoUrl)
        != osl::FileBase::E_None)
    {
        std::cerr
            << ("Error: Cannot convert pathname to URL in " __FILE__
                ", in line ")
            << __LINE__ << "\n"
            << sPoPath
            << "\n";
        throw false; //TODO
    }
    osl::Directory::remove(sPoUrl);
}

void handleProjects(char const * sSourceRoot, char const * sDestRoot)
{
    OUString root16;
    if (!rtl_convertStringToUString(
            &root16.pData, sSourceRoot, rtl_str_getLength(sSourceRoot),
            osl_getThreadTextEncoding(),
            (RTL_TEXTTOUNICODE_FLAGS_UNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_MBUNDEFINED_ERROR
             | RTL_TEXTTOUNICODE_FLAGS_INVALID_ERROR)))
    {
        std::cerr << "Error: Cannot convert pathname to UTF-16\n";
        throw false; //TODO
    }
    OUString rootUrl;
    if (osl::FileBase::getFileURLFromSystemPath(root16, rootUrl)
        != osl::FileBase::E_None)
    {
        std::cerr
            << ("Error: Cannot convert pathname to URL in " __FILE__
                ", in line ")
            << __LINE__ << "\n       root16: "
            << root16
            << "\n";
        throw false; //TODO
    }
    gDestRoot = OString(sDestRoot);
    handleDirectory(rootUrl, 0, OString(), gDestRoot);
}
}

SAL_IMPLEMENT_MAIN_WITH_ARGS(argc, argv)
{
    try
    {
        if (argc != 4)
        {
            std::cerr
                << ("localize (c)2001 by Sun Microsystems\n\n"
                    "As part of the L10N framework, localize extracts en-US\n"
                    "strings for translation out of the toplevel modules defined\n"
                    "in projects array in l10ntools/source/localize.cxx.\n\n"
                    "Syntax: localize <source-root> <outfile> <library-path-env-var-override>\n");
            exit(EXIT_FAILURE);
        }
        libraryPathEnvVarOverride = argv[3];
        handleProjects(argv[1],argv[2]);
    }
    catch (std::exception& e)
    {
        std::cerr << "exception: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (bool) //TODO
    {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
