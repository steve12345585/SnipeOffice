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

#include <fstream>

#include <sal/log.hxx>

#include <stdio.h>
#include <stdlib.h>
#include <helpmerge.hxx>
#include <utility>
#include <vector>

#ifdef _WIN32
#if !defined WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#undef CopyFile
#include <direct.h>
#endif

#include <export.hxx>
#include <common.hxx>
#include <helper.hxx>
#include <po.hxx>

#if OSL_DEBUG_LEVEL > 2
void HelpParser::Dump(XMLHashMap* rElem_in)
{
    for (auto const& pos : *rElem_in)
    {
        Dump(pos.second,pos.first);
    }
}

void HelpParser::Dump(LangHashMap* rElem_in,const OString & sKey_in)
{
    OString x;
    OString y;
    fprintf(stdout,"+------------%s-----------+\n",sKey_in.getStr() );
    for (auto const& posn : *rElem_in)
    {
        x=posn.first;
        y=posn.second->ToOString();
        fprintf(stdout,"key=%s value=%s\n",x.getStr(),y.getStr());
    }
    fprintf(stdout,"+--------------------------+\n");
}
#endif

HelpParser::HelpParser( OString _sHelpFile )
        : sHelpFile(std::move( _sHelpFile ))
          {};

/*****************************************************************************/
bool HelpParser::CreatePO(
/*****************************************************************************/
    const OString &rPOFile_in, const OString &sHelpFile,
    XMLFile* pXmlFile, std::string_view rGsi1){
    SimpleXMLParser aParser;
    //TODO: explicit BOM handling?

    if (!aParser.Execute( sHelpFile, pXmlFile ))
    {
        printf(
            "%s: %s\n",
            sHelpFile.getStr(),
            aParser.GetError().m_sMessage.getStr());
        exit(-1);
    }
    pXmlFile->Extract();
    if( !pXmlFile->CheckExportStatus() ){
        return true;
    }

    PoOfstream aPoOutput( rPOFile_in, PoOfstream::APP );

    if (!aPoOutput.isOpen()) {
        fprintf(stdout,"Can't open file %s\n",rPOFile_in.getStr());
        return false;
    }

    XMLHashMap* aXMLStrHM = pXmlFile->GetStrings();

    std::vector<OString> order = pXmlFile->getOrder();

    for (auto const& pos : order)
    {
        auto posm = aXMLStrHM->find(pos);
        LangHashMap* pElem = posm->second;

        XMLElement* pXMLElement = (*pElem)[ "en-US"_ostr ];

        if( pXMLElement != nullptr )
        {
            OString data(
                pXMLElement->ToOString().
                    replaceAll("\n"_ostr,OString()).
                    replaceAll("\t"_ostr,OString()).trim());

            common::writePoEntry(
                "Helpex"_ostr, aPoOutput, sHelpFile, rGsi1,
                posm->first, OString(), OString(), data);

            pXMLElement=nullptr;
        }
        else
        {
            // If this is something totally unexpected, wouldn't an assert() be in order?
            // On the other hand, if this is expected, why the printf?
            fprintf(stdout,"\nDBG: NullPointer in HelpParser::CreatePO, File %s\n", sHelpFile.getStr());
        }
    }
    aPoOutput.close();

    return true;
}

bool HelpParser::Merge( const OString &rDestinationFile,
    const OString& rLanguage , MergeDataFile* pMergeDataFile )
{
    SimpleXMLParser aParser;

    //TODO: explicit BOM handling?

    XMLFile xmlfile( OString('0') );
    if (!aParser.Execute( sHelpFile, &xmlfile))
    {
        SAL_WARN("l10ntools", "could not parse " << sHelpFile);
        return false;
    }
    MergeSingleFile( &xmlfile , pMergeDataFile , rLanguage , rDestinationFile );
    return true;
}

void HelpParser::MergeSingleFile( XMLFile* file , MergeDataFile* pMergeDataFile , const OString& sLanguage ,
                                  OString const & sPath )
{
    file->Extract();

    XMLHashMap* aXMLStrHM = file->GetStrings();
    static ResData s_ResData(""_ostr,""_ostr);
    s_ResData.sResTyp   = "help"_ostr;

    std::vector<OString> order = file->getOrder();

    for (auto const& pos : order) // Merge every l10n related string in the same order as export
    {
        auto posm = aXMLStrHM->find(pos);
        LangHashMap*  aLangHM = posm->second;
#if OSL_DEBUG_LEVEL > 2
        printf("*********************DUMPING HASHMAP***************************************");
        Dump(aXMLStrHM);
        printf("DBG: sHelpFile = %s\n",sHelpFile.getStr() );
#endif

        s_ResData.sGId      =  posm->first;
        s_ResData.sFilename  =  sHelpFile;

        ProcessHelp( aLangHM , sLanguage, &s_ResData , pMergeDataFile );
    }

    file->Write(sPath);
}

/* ProcessHelp method: search for en-US entry and replace it with the current language*/
void HelpParser::ProcessHelp( LangHashMap* aLangHM , const OString& sCur , ResData *pResData , MergeDataFile* pMergeDataFile ){

    XMLElement*   pXMLElement = nullptr;

    if( sCur.equalsIgnoreAsciiCase("en-US") )
        return;

    pXMLElement = (*aLangHM)[ "en-US"_ostr ];
    if( pXMLElement == nullptr )
    {
        printf("Error: Can't find en-US entry\n");
        return;
    }

    OString sNewText;
    OString sNewdata;
    OString sSourceText(
    pXMLElement->ToOString().
        replaceAll(
            "\n"_ostr,
            OString()).
        replaceAll(
            "\t"_ostr,
            OString()));
    // re-add spaces to the beginning of translated string,
    // important for indentation of Basic code examples
    sal_Int32 nPreSpaces = 0;
    sal_Int32 nLen = sSourceText.getLength();
    while ( (nPreSpaces < nLen) && (sSourceText[nPreSpaces] == ' ') )
        nPreSpaces++;
    if( sCur == "qtz" )
    {
        sNewText = MergeEntrys::GetQTZText(*pResData, sSourceText);
        sNewdata = sNewText;
    }
    else if( pMergeDataFile )
    {
        MergeEntrys *pEntrys = pMergeDataFile->GetMergeEntrys( pResData );
        if( pEntrys != nullptr)
        {
            pEntrys->GetText( sNewText, sCur, true );
            if (helper::isWellFormedXML(XMLUtil::QuotHTML(sNewText)))
            {
                sNewdata = sSourceText.subView(0,nPreSpaces) + sNewText;
            }
        }
    }
    if (!sNewdata.isEmpty())
    {
        XMLData *data = new XMLData( sNewdata , nullptr ); // Add new one
        pXMLElement->RemoveAndDeleteAllChildren();
        pXMLElement->AddChild( data );
        aLangHM->erase( sCur );
    }
    else
    {
        SAL_WARN(
            "l10ntools",
            "Can't find GID=" << pResData->sGId << " TYP=" << pResData->sResTyp);
    }
    pXMLElement->ChangeLanguageTag(sCur);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
