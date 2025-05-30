/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <iostream>
#include <cassert>
#include <cstring>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>

#include <export.hxx>
#include <helper.hxx>
#include <common.hxx>
#include <po.hxx>
#include <treemerge.hxx>
#include <utility>


namespace
{
    // Extract strings from nodes on all level recursively
    void lcl_ExtractLevel(
        const xmlDocPtr pSource, const xmlNodePtr pRoot,
        const xmlChar* pNodeName, PoOfstream& rPOStream )
    {
        if( !pRoot->children )
        {
            return;
        }
        for( xmlNodePtr pCurrent = pRoot->children->next;
            pCurrent; pCurrent = pCurrent->next)
        {
            if (!xmlStrcmp(pCurrent->name, pNodeName))
            {
                xmlChar* pID = xmlGetProp(pCurrent, reinterpret_cast<const xmlChar*>("id"));
                xmlChar* pText =
                    xmlGetProp(pCurrent, reinterpret_cast<const xmlChar*>("title"));

                common::writePoEntry(
                    "Treex"_ostr, rPOStream, pSource->name, helper::xmlStrToOString( pNodeName ),
                    helper::xmlStrToOString( pID ), OString(), OString(), helper::xmlStrToOString( pText ));

                xmlFree( pID );
                xmlFree( pText );

                lcl_ExtractLevel(
                    pSource, pCurrent, reinterpret_cast<const xmlChar *>("node"),
                    rPOStream );
            }
        }
    }

    // Update id and content of the topic
    xmlNodePtr lcl_UpdateTopic(
        const xmlNodePtr pCurrent, std::string_view rXhpRoot )
    {
        xmlNodePtr pReturn = pCurrent;
        xmlChar* pID = xmlGetProp(pReturn, reinterpret_cast<const xmlChar*>("id"));
        const OString sID =
            helper::xmlStrToOString( pID );
        xmlFree( pID );

        const std::string_view::size_type nFirstSlash = sID.indexOf('/');
        const auto nAfterSlash = (nFirstSlash != std::string_view::npos) ? (nFirstSlash + 1) : 0;
        // Update id attribute of topic
        {
            std::u16string_view::size_type nXhpSlash = rXhpRoot.rfind('/');
            const auto nAfterXhpSlash = (nXhpSlash != std::u16string_view::npos) ? (nXhpSlash + 1) : 0;

            OString sNewID =
                OString::Concat(sID.subView( 0, nAfterSlash )) +
                rXhpRoot.substr( nAfterXhpSlash ) +
                sID.subView( sID.indexOf( '/', nAfterSlash ) );
            xmlSetProp(
                pReturn, reinterpret_cast<const xmlChar*>("id"),
                reinterpret_cast<const xmlChar*>(sNewID.getStr()));
        }

        const OString sXhpPath =
            OString::Concat(rXhpRoot) +
            sID.subView(sID.indexOf('/', nAfterSlash));
        xmlDocPtr pXhpFile = xmlParseFile( sXhpPath.getStr() );
        // if xhpfile is missing than put this topic into comment
        if ( !pXhpFile )
        {
            xmlNodePtr pTemp = pReturn;
            xmlChar* sNewID =
                xmlGetProp(pReturn, reinterpret_cast<const xmlChar*>("id"));
            xmlChar* sComment =
                xmlStrcat( xmlCharStrdup("removed "), sNewID );
            pReturn = xmlNewComment( sComment );
            xmlReplaceNode( pTemp, pReturn );
            xmlFree( pTemp );
            xmlFree( sNewID );
            xmlFree( sComment );
        }
        // update topic's content on the basis of xhpfile's title
        else
        {
            xmlNodePtr pXhpNode = xmlDocGetRootElement( pXhpFile );
            for( pXhpNode = pXhpNode->children;
                pXhpNode; pXhpNode = pXhpNode->children )
            {
                while( pXhpNode->type != XML_ELEMENT_NODE )
                {
                    pXhpNode = pXhpNode->next;
                }
                if(!xmlStrcmp(pXhpNode->name, reinterpret_cast<const xmlChar *>("title")))
                {
                    xmlChar* sTitle =
                        xmlNodeListGetString(pXhpFile, pXhpNode->children, 1);
                    OString sNewTitle =
                        helper::xmlStrToOString( sTitle ).
                            replaceAll("$[officename]"_ostr,"%PRODUCTNAME"_ostr).
                                replaceAll("$[officeversion]"_ostr,"%PRODUCTVERSION"_ostr);
                    xmlChar *xmlString = xmlEncodeSpecialChars(nullptr,
                        reinterpret_cast<const xmlChar*>( sNewTitle.getStr() ));
                    xmlNodeSetContent( pReturn, xmlString);
                    xmlFree( xmlString );
                    xmlFree( sTitle );
                    break;
                }
            }
            if( !pXhpNode )
            {
                std::cerr
                    << "Treex error: Cannot find title in "
                    << sXhpPath << std::endl;
                pReturn = nullptr;
            }
            xmlFreeDoc( pXhpFile );
            xmlCleanupParser();
        }
        return pReturn;
    }
    // Localize title attribute of help_section and node tags
    void lcl_MergeLevel(
        xmlDocPtr io_pSource, const xmlNodePtr pRoot,
        const xmlChar * pNodeName, MergeDataFile* pMergeDataFile,
        const OString& rLang, const OString& rXhpRoot )
    {
        if( !pRoot->children )
        {
            return;
        }
        for( xmlNodePtr pCurrent = pRoot->children;
            pCurrent; pCurrent = pCurrent->next)
        {
            if( !xmlStrcmp(pCurrent->name, pNodeName) )
            {
                if( rLang != "en-US" )
                {
                    OString sNewText;
                    xmlChar* pID = xmlGetProp(pCurrent, reinterpret_cast<const xmlChar*>("id"));
                    ResData  aResData(
                        helper::xmlStrToOString( pID ),
                        static_cast<OString>(io_pSource->name) );
                    xmlFree( pID );
                    aResData.sResTyp = helper::xmlStrToOString( pNodeName );
                    if( pMergeDataFile )
                    {
                        MergeEntrys* pEntrys =
                            pMergeDataFile->GetMergeEntrys( &aResData );
                        if( pEntrys )
                        {
                            pEntrys->GetText( sNewText, rLang );
                        }
                    }
                    else if( rLang == "qtz" )
                    {
                        xmlChar* pText = xmlGetProp(pCurrent, reinterpret_cast<const xmlChar*>("title"));
                        const OString sOriginText = helper::xmlStrToOString(pText);
                        xmlFree( pText );
                        sNewText = MergeEntrys::GetQTZText(aResData, sOriginText);
                    }
                    if( !sNewText.isEmpty() )
                    {
                        xmlSetProp(
                            pCurrent, reinterpret_cast<const xmlChar*>("title"),
                            reinterpret_cast<const xmlChar*>(sNewText.getStr()));
                    }
                }

                lcl_MergeLevel(
                    io_pSource, pCurrent, reinterpret_cast<const xmlChar *>("node"),
                    pMergeDataFile, rLang, rXhpRoot );
            }
            else if( !xmlStrcmp(pCurrent->name, reinterpret_cast<const xmlChar *>("topic")) )
            {
                pCurrent = lcl_UpdateTopic( pCurrent, rXhpRoot );
            }
        }
    }
}

TreeParser::TreeParser(
    const OString& rInputFile, OString _sLang )
    : m_pSource( nullptr )
    , m_sLang(std::move( _sLang ))
    , m_bIsInitialized( false )
{
    m_pSource = xmlParseFile( rInputFile.getStr() );
    if ( !m_pSource ) {
        std::cerr
            << "Treex error: Cannot open source file: "
            << rInputFile << std::endl;
        return;
    }
    if( !m_pSource->name )
    {
        m_pSource->name = static_cast<char *>(xmlMalloc(strlen(rInputFile.getStr())+1));
        strcpy( m_pSource->name, rInputFile.getStr() );
    }
    m_bIsInitialized = true;
}

TreeParser::~TreeParser()
{
    // be sure m_pSource is freed
    if (m_bIsInitialized)
        xmlFreeDoc( m_pSource );
}

void TreeParser::Extract( const OString& rPOFile )
{
    assert( m_bIsInitialized );
    PoOfstream aPOStream( rPOFile, PoOfstream::APP );
    if( !aPOStream.isOpen() )
    {
        std::cerr
            << "Treex error: Cannot open po file for extract: "
            << rPOFile << std::endl;
        return;
    }

    xmlNodePtr pRootNode = xmlDocGetRootElement( m_pSource );
    lcl_ExtractLevel(
        m_pSource, pRootNode, reinterpret_cast<const xmlChar *>("help_section"),
        aPOStream );

    xmlFreeDoc( m_pSource );
    xmlCleanupParser();
    aPOStream.close();
    m_bIsInitialized = false;
}

void TreeParser::Merge(
    const OString &rMergeSrc, const OString &rDestinationFile,
    const OString &rXhpRoot )
{
    assert( m_bIsInitialized );

    const xmlNodePtr pRootNode = xmlDocGetRootElement( m_pSource );
    std::unique_ptr<MergeDataFile> pMergeDataFile;
    if( m_sLang != "qtz" && m_sLang != "en-US" )
    {
        pMergeDataFile.reset(new MergeDataFile(
            rMergeSrc, static_cast<OString>( m_pSource->name ), false, false ));
        const std::vector<OString> vLanguages = pMergeDataFile->GetLanguages();
        if( !vLanguages.empty() && vLanguages[0] != m_sLang )
        {
            std::cerr
                << ("Treex error: given language conflicts with language of"
                    " Mergedata file: ")
                << m_sLang << " - "
                << vLanguages[0] << std::endl;
            return;
        }
    }
    lcl_MergeLevel(
        m_pSource, pRootNode, reinterpret_cast<const xmlChar *>("help_section"),
        pMergeDataFile.get(), m_sLang, rXhpRoot );

    pMergeDataFile.reset();
    xmlSaveFile( rDestinationFile.getStr(), m_pSource );
    xmlFreeDoc( m_pSource );
    xmlCleanupParser();
    m_bIsInitialized = false;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
