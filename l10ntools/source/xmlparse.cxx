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
#include <stdio.h>
#include <string_view>

#include <helper.hxx>
#include <utility>
#include <xmlparse.hxx>
#include <fstream>
#include <iostream>
#include <osl/file.hxx>
#include <osl/process.h>
#include <o3tl/string_view.hxx>
#include <rtl/ustring.hxx>
#include <rtl/strbuf.hxx>
#include <unicode/regex.h>

using namespace osl;

constexpr OString XML_LANG = "xml-lang"_ostr;




XMLChildNode::XMLChildNode( XMLParentNode *pPar )
    : m_pParent( pPar )
{
    if ( m_pParent )
        m_pParent->AddChild( this );
}


XMLChildNode::XMLChildNode( const XMLChildNode& rObj)
    : XMLNode(rObj),
      m_pParent(rObj.m_pParent)
{
}

XMLChildNode& XMLChildNode::operator=(const XMLChildNode& rObj)
{
    if(this != &rObj)
    {
        m_pParent=rObj.m_pParent;
    }
    return *this;
}




XMLParentNode::~XMLParentNode()
{
    if( m_pChildList )
    {
        RemoveAndDeleteAllChildren();
    }
}

XMLParentNode::XMLParentNode( const XMLParentNode& rObj)
: XMLChildNode( rObj )
{
    if( !rObj.m_pChildList )
        return;

    m_pChildList.reset( new XMLChildNodeList );
    for (XMLChildNode* pChild : *rObj.m_pChildList)
    {
        if( pChild != nullptr)
        {
            switch(pChild->GetNodeType())
            {
                case XMLNodeType::ELEMENT:
                    AddChild( new XMLElement( *static_cast<XMLElement* >(pChild) ) ); break;
                case XMLNodeType::DATA:
                    AddChild( new XMLData   ( *static_cast<XMLData* >   (pChild) ) ); break;
                case XMLNodeType::COMMENT:
                    AddChild( new XMLComment( *static_cast<XMLComment* >(pChild) ) ); break;
                case XMLNodeType::DEFAULT:
                    AddChild( new XMLDefault( *static_cast<XMLDefault* >(pChild) ) ); break;
                default:    fprintf(stdout,"XMLParentNode::XMLParentNode( const XMLParentNode& rObj) strange obj");
            }
        }
    }
}

XMLParentNode& XMLParentNode::operator=(const XMLParentNode& rObj)
{
    if(this!=&rObj)
    {
        XMLChildNode::operator=(rObj);
        if( m_pChildList )
        {
            RemoveAndDeleteAllChildren();
        }
        if( rObj.m_pChildList )
        {
            m_pChildList.reset( new XMLChildNodeList );
            for (XMLChildNode* pChild : *rObj.m_pChildList)
                AddChild(pChild);
        }
        else
            m_pChildList.reset();

    }
    return *this;
}
void XMLParentNode::AddChild( XMLChildNode *pChild )
{
    if ( !m_pChildList )
        m_pChildList.reset( new XMLChildNodeList );
    m_pChildList->push_back( pChild );
}

void XMLParentNode::RemoveAndDeleteAllChildren()
{
    if ( m_pChildList )
    {
        for (const XMLChildNode* pChild : *m_pChildList)
            delete pChild;
        m_pChildList->clear();
    }
}




void XMLFile::Write( OString const &aFilename )
{
    std::ofstream s(
        aFilename.getStr(), std::ios_base::out | std::ios_base::trunc);
    if (!s.is_open())
    {
        std::cerr
            << "Error: helpex cannot create file " << aFilename
            << '\n';
        std::exit(EXIT_FAILURE);
    }
    Write(s);
    s.close();
}

void XMLFile::Write( std::ofstream &rStream , XMLNode *pCur )
{
    if ( !pCur )
        Write( rStream, this );
    else {
        switch( pCur->GetNodeType())
        {
            case XMLNodeType::XFILE:
            {
                if( GetChildList())
                    for (XMLChildNode* pChild : *GetChildList())
                        Write( rStream, pChild);
            }
            break;
            case XMLNodeType::ELEMENT:
            {
                XMLElement *pElement = static_cast<XMLElement*>(pCur);
                rStream  << "<";
                rStream << pElement->GetName();
                if ( pElement->GetAttributeList())
                    for (const XMLAttribute* pAttribute : *pElement->GetAttributeList())
                    {
                        rStream << " ";
                        OString sData(pAttribute->GetName());
                        rStream << XMLUtil::QuotHTML( sData );
                        rStream << "=\"";
                        sData = pAttribute->GetValue();
                        rStream << XMLUtil::QuotHTML( sData );
                        rStream << "\"";
                    }
                if ( !pElement->GetChildList())
                    rStream << "/>";
                else
                {
                    rStream << ">";
                    for (XMLChildNode* pChild : *pElement->GetChildList())
                        Write(rStream, pChild);
                    rStream << "</";
                    rStream << pElement->GetName();
                    rStream << ">";
                }
            }
            break;
            case XMLNodeType::DATA:
            {
                OString sData( static_cast<const XMLData*>(pCur)->GetData());
                rStream << XMLUtil::QuotHTML( sData );
            }
            break;
            case XMLNodeType::COMMENT:
            {
                const XMLComment *pComment = static_cast<const XMLComment*>(pCur);
                rStream << "<!--";
                rStream <<  pComment->GetComment();
                rStream << "-->";
            }
            break;
            case XMLNodeType::DEFAULT:
            {
                const XMLDefault *pDefault = static_cast<const XMLDefault*>(pCur);
                rStream <<  pDefault->GetDefault();
            }
            break;
        }
    }
}

void XMLFile::Print( XMLNode *pCur, sal_uInt16 nLevel )
{
    if ( !pCur )
        Print( this );
    else
    {
        switch( pCur->GetNodeType())
        {
            case XMLNodeType::XFILE:
            {
                if( GetChildList())
                    for (XMLChildNode* pChild : *GetChildList())
                        Print(pChild);
            }
            break;
            case XMLNodeType::ELEMENT:
            {
                XMLElement *pElement = static_cast<XMLElement*>(pCur);

                fprintf( stdout, "<%s", pElement->GetName().getStr());
                if ( pElement->GetAttributeList())
                {
                    for (const XMLAttribute* pAttribute : *pElement->GetAttributeList())
                    {
                        const OString aAttrName(pAttribute->GetName());
                        if (aAttrName != XML_LANG)
                        {
                            fprintf( stdout, " %s=\"%s\"",
                                aAttrName.getStr(),
                                pAttribute->GetValue().getStr());
                        }
                    }
                }
                if ( !pElement->GetChildList())
                    fprintf( stdout, "/>" );
                else
                {
                    fprintf( stdout, ">" );
                    for (XMLChildNode* pChild : *pElement->GetChildList())
                        Print(pChild, nLevel + 1);
                    fprintf( stdout, "</%s>", pElement->GetName().getStr());
                }
            }
            break;
            case XMLNodeType::DATA:
            {
                const XMLData *pData = static_cast<const XMLData*>(pCur);
                fprintf( stdout, "%s", pData->GetData().getStr());
            }
            break;
            case XMLNodeType::COMMENT:
            {
                const XMLComment *pComment = static_cast<const XMLComment*>(pCur);
                fprintf( stdout, "<!--%s-->", pComment->GetComment().getStr());
            }
            break;
            case XMLNodeType::DEFAULT:
            {
                const XMLDefault *pDefault = static_cast<const XMLDefault*>(pCur);
                fprintf( stdout, "%s", pDefault->GetDefault().getStr());
            }
            break;
        }
    }
}
XMLFile::~XMLFile()
{
    if( m_pXMLStrings )
    {
        for (auto const& pos : *m_pXMLStrings)
        {
            delete pos.second;             // Check and delete content also ?
        }
    }
}

XMLFile::XMLFile( OString _sFileName ) // the file name, empty if created from memory stream
    : XMLParentNode( nullptr )
    , m_sFileName(std::move( _sFileName ))
{
    m_aNodes_localize.emplace( "bookmark"_ostr , true );
    m_aNodes_localize.emplace( "variable"_ostr , true );
    m_aNodes_localize.emplace( "paragraph"_ostr , true );
    m_aNodes_localize.emplace( "h1"_ostr , true );
    m_aNodes_localize.emplace( "h2"_ostr , true );
    m_aNodes_localize.emplace( "h3"_ostr , true );
    m_aNodes_localize.emplace( "h4"_ostr , true );
    m_aNodes_localize.emplace( "h5"_ostr , true );
    m_aNodes_localize.emplace( "h6"_ostr , true );
    m_aNodes_localize.emplace( "note"_ostr , true );
    m_aNodes_localize.emplace( "tip"_ostr , true );
    m_aNodes_localize.emplace( "warning"_ostr , true );
    m_aNodes_localize.emplace( "alt"_ostr , true );
    m_aNodes_localize.emplace( "caption"_ostr , true );
    m_aNodes_localize.emplace( "title"_ostr , true );
    m_aNodes_localize.emplace( "link"_ostr , true );
}

void XMLFile::Extract()
{
    m_pXMLStrings.reset( new XMLHashMap );
    SearchL10NElements( this );
}

void XMLFile::InsertL10NElement( XMLElement* pElement )
{
    OString sId, sLanguage("en-US"_ostr);
    LangHashMap* pElem;

    if( pElement->GetAttributeList() != nullptr )
    {
        for (const XMLAttribute* pAttribute : *pElement->GetAttributeList())
        {
            const OString sTempStr(pAttribute->GetName());
            // Get the "id" Attribute
            if (sTempStr == "id")
            {
                sId = pAttribute->GetValue();
            }
            // Get the "xml-lang" Attribute
            if (sTempStr == XML_LANG)
            {
                sLanguage = pAttribute->GetValue();
            }
        }
    }
    else
    {
        fprintf(stdout,"XMLFile::InsertL10NElement: No AttributeList found");
        fprintf(stdout,"++++++++++++++++++++++++++++++++++++++++++++++++++");
        Print( pElement );
        fprintf(stdout,"++++++++++++++++++++++++++++++++++++++++++++++++++");
    }

    XMLHashMap::iterator pos = m_pXMLStrings->find( sId );
    if( pos == m_pXMLStrings->end() ) // No instance, create new one
    {
        pElem = new LangHashMap;
        (*pElem)[ sLanguage ]=pElement;
        m_pXMLStrings->emplace( sId , pElem );
        m_vOrder.push_back( sId );
    }
    else        // Already there
    {
        pElem=pos->second;
        if ( pElem->count(sLanguage) )
        {
            fprintf(stdout,"Error: Duplicated entry. ID = %s  LANG = %s in File %s\n", sId.getStr(), sLanguage.getStr(), m_sFileName.getStr() );
            exit( -1 );
        }
        (*pElem)[ sLanguage ]=pElement;
    }
}

XMLFile::XMLFile( const XMLFile& rObj )
    : XMLParentNode( rObj )
    , m_sFileName( rObj.m_sFileName )
{
    if( this != &rObj )
    {
        m_aNodes_localize = rObj.m_aNodes_localize;
        m_vOrder = rObj.m_vOrder;
    }
}

XMLFile& XMLFile::operator=(const XMLFile& rObj)
{
    if( this == &rObj )
        return *this;

    XMLParentNode::operator=(rObj);

    m_aNodes_localize = rObj.m_aNodes_localize;
    m_vOrder = rObj.m_vOrder;

    m_pXMLStrings.reset();

    if( rObj.m_pXMLStrings )
    {
        m_pXMLStrings.reset( new XMLHashMap );
        for (auto const& pos : *rObj.m_pXMLStrings)
        {
            LangHashMap* pElem=pos.second;
            LangHashMap* pNewelem = new LangHashMap;
            for (auto const& pos2 : *pElem)
            {
                (*pNewelem)[ pos2.first ] = new XMLElement( *pos2.second );
            }
            (*m_pXMLStrings)[ pos.first ] = pNewelem;
        }
    }
    return *this;
}

void XMLFile::SearchL10NElements( XMLChildNode *pCur )
{
    if ( !pCur )
        SearchL10NElements( this  );
    else
    {
        switch( pCur->GetNodeType())
        {
            case XMLNodeType::XFILE:
            {
                if( GetChildList())
                {
                    for (XMLChildNode* pElement : *GetChildList())
                    {
                        if( pElement->GetNodeType() ==  XMLNodeType::ELEMENT )
                            SearchL10NElements(pElement);
                    }
                }
            }
            break;
            case XMLNodeType::ELEMENT:
            {
                bool bInsert = true;
                XMLElement *pElement = static_cast<XMLElement*>(pCur);
                const OString sName(pElement->GetName().toAsciiLowerCase());
                if ( pElement->GetAttributeList())
                {
                    for (const XMLAttribute* pAttribute : *pElement->GetAttributeList())
                    {
                        if (pAttribute->GetName() == "localize")
                        {
                            bInsert=false;
                            break;
                        }
                    }
                }

                if ( bInsert && ( m_aNodes_localize.find( sName ) != m_aNodes_localize.end() ) )
                    InsertL10NElement(pElement);
                else if ( bInsert && pElement->GetChildList() )
                {
                    for (XMLChildNode* pChild : *pElement->GetChildList())
                        SearchL10NElements(pChild);
                }
            }
            break;
            default:
            break;
        }
    }
}

bool XMLFile::CheckExportStatus( XMLChildNode *pCur )
{
    static bool bStatusExport = true;

    if ( !pCur )
        CheckExportStatus( this );
    else {
        switch( pCur->GetNodeType())
        {
            case XMLNodeType::XFILE:
            {
                if( GetChildList())
                {
                    for (XMLChildNode* pElement : *GetChildList())
                        if( pElement->GetNodeType() ==  XMLNodeType::ELEMENT ) CheckExportStatus( pElement );//, i);
                }
            }
            break;
            case XMLNodeType::ELEMENT:
            {
                XMLElement *pElement = static_cast<XMLElement*>(pCur);
                if (pElement->GetName().equalsIgnoreAsciiCase("TOPIC"))
                {
                    if ( pElement->GetAttributeList())
                    {
                        for (const XMLAttribute* pAttribute : *pElement->GetAttributeList())
                        {
                            const OString tmpStr(pAttribute->GetName());
                            if (tmpStr.equalsIgnoreAsciiCase("STATUS"))
                            {
                                const OString tmpStrVal(pAttribute->GetValue());
                                if (!tmpStrVal.equalsIgnoreAsciiCase("PUBLISH") &&
                                    !tmpStrVal.equalsIgnoreAsciiCase("DEPRECATED"))
                                {
                                    bStatusExport = false;
                                }
                            }
                        }
                    }
                }
                else if ( pElement->GetChildList() )
                {
                    for (XMLChildNode* pChild : *pElement->GetChildList())
                        CheckExportStatus(pChild);
                }
            }
            break;
            default:
            break;
        }
    }
    return bStatusExport;
}

XMLElement::XMLElement(
    OString _sName,    // the element name
    XMLParentNode *pParent   // parent node of this element
)
    : XMLParentNode( pParent )
    , m_sElementName(std::move( _sName ))
{
}

XMLElement::XMLElement(const XMLElement& rObj)
    : XMLParentNode( rObj )
    , m_sElementName( rObj.m_sElementName )
{
    if ( rObj.m_pAttributes )
    {
        m_pAttributes.reset( new XMLAttributeList );
        for (const XMLAttribute* pAttribute : *rObj.m_pAttributes)
            AddAttribute(pAttribute->GetName(), pAttribute->GetValue());
    }
}

XMLElement& XMLElement::operator=(const XMLElement& rObj)
{
    if( this !=& rObj )
    {
        XMLParentNode::operator=(rObj);
        m_sElementName = rObj.m_sElementName;

        if ( m_pAttributes )
        {
            for (const XMLAttribute* pAttribute : *m_pAttributes)
                delete pAttribute;
            m_pAttributes.reset();
        }
        if ( rObj.m_pAttributes )
        {
            m_pAttributes.reset( new XMLAttributeList );
            for (const XMLAttribute* pAttribute : *rObj.m_pAttributes)
                AddAttribute(pAttribute->GetName(), pAttribute->GetValue());
        }
    }
    return *this;
}

void XMLElement::AddAttribute( const OString &rAttribute, const OString &rValue )
{
    if ( !m_pAttributes )
        m_pAttributes.reset( new XMLAttributeList );
    m_pAttributes->push_back( new XMLAttribute( rAttribute, rValue ) );
}

void XMLElement::ChangeLanguageTag( const OString &rValue )
{
    if ( m_pAttributes )
    {
        bool bWasSet = false;
        for (XMLAttribute* pAttribute : *m_pAttributes)
        {
            if (pAttribute->GetName() == XML_LANG)
            {
                pAttribute->setValue(rValue);
                bWasSet = true;
            }
        }

        if (!bWasSet)
            AddAttribute(XML_LANG, rValue);
    }
    XMLChildNodeList* pCList = GetChildList();

    if( !pCList )
        return;

    for (XMLChildNode* pChild : *pCList)
    {
        if( pChild && pChild->GetNodeType() == XMLNodeType::ELEMENT )
        {
            XMLElement* pElem = static_cast< XMLElement* >(pChild);
            pElem->ChangeLanguageTag( rValue );
            pElem  = nullptr;
            pChild  = nullptr;
        }
    }
    pCList = nullptr;
}

XMLElement::~XMLElement()
{
    if ( m_pAttributes )
    {
        for (const XMLAttribute* pAttribute : *m_pAttributes)
            delete pAttribute;
    }
}

OString XMLElement::ToOString()
{
    OStringBuffer sBuffer;
    Print(this,sBuffer,true);
    return sBuffer.makeStringAndClear();
}

void XMLElement::Print(XMLNode *pCur, OStringBuffer& rBuffer, bool bRootelement ) const
{
    if( pCur )
    {
        if( bRootelement )
        {
            XMLElement *pElement = static_cast<XMLElement*>(pCur);
            if ( pElement->GetAttributeList())
            {
                if ( pElement->GetChildList())
                {
                    for (XMLChildNode* pChild : *pElement->GetChildList())
                        Print(pChild, rBuffer, false);
                }
            }
        }
        else
        {
            switch( pCur->GetNodeType())
            {
                case XMLNodeType::ELEMENT:
                {
                    XMLElement *pElement = static_cast<XMLElement*>(pCur);

                    if( !pElement->GetName().equalsIgnoreAsciiCase("comment") )
                    {
                        rBuffer.append( "<" );
                        rBuffer.append( pElement->GetName() );
                        if ( pElement->GetAttributeList())
                        {
                            for (const XMLAttribute* pAttribute : *pElement->GetAttributeList())
                            {
                                const OString aAttrName(pAttribute->GetName());
                                if (aAttrName != XML_LANG)
                                {
                                    rBuffer.append(
                                        " " + aAttrName + "=\"" +
                                        pAttribute->GetValue() + "\"" );
                                }
                            }
                        }
                        if ( !pElement->GetChildList())
                            rBuffer.append( "/>" );
                        else
                        {
                            rBuffer.append( ">" );
                            for (XMLChildNode* pChild : *pElement->GetChildList())
                                Print(pChild, rBuffer, false);
                            rBuffer.append( "</" + pElement->GetName() + ">" );
                        }
                    }
                }
                break;
                case XMLNodeType::DATA:
                {
                    const XMLData *pData = static_cast<const XMLData*>(pCur);
                    rBuffer.append( pData->GetData() );
                }
                break;
                case XMLNodeType::COMMENT:
                {
                    const XMLComment *pComment = static_cast<const XMLComment*>(pCur);
                    rBuffer.append( "<!--" + pComment->GetComment() + "-->" );
                }
                break;
                case XMLNodeType::DEFAULT:
                {
                    const XMLDefault *pDefault = static_cast<const XMLDefault*>(pCur);
                    rBuffer.append( pDefault->GetDefault() );
                }
                break;
                default:
                break;
            }
        }
    }
    else
    {
        fprintf(stdout,"\n#+------Error: NULL Pointer in XMLELement::Print------+#\n");
        return;
    }
}




namespace
{

OUString lcl_pathnameToAbsoluteUrl(std::string_view rPathname)
{
    OUString sPath = OStringToOUString(rPathname, RTL_TEXTENCODING_UTF8 );
    OUString sUrl;
    if (osl::FileBase::getFileURLFromSystemPath(sPath, sUrl)
        != osl::FileBase::E_None)
    {
        std::cerr << "Error: Cannot convert input pathname to URL\n";
        std::exit(EXIT_FAILURE);
    }
    OUString sCwd;
    if (osl_getProcessWorkingDir(&sCwd.pData) != osl_Process_E_None)
    {
        std::cerr << "Error: Cannot determine cwd\n";
        std::exit(EXIT_FAILURE);
    }
    if (osl::FileBase::getAbsoluteFileURL(sCwd, sUrl, sUrl)
        != osl::FileBase::E_None)
    {
        std::cerr << "Error: Cannot convert input URL to absolute URL\n";
        std::exit(EXIT_FAILURE);
    }
    return sUrl;
}
}


SimpleXMLParser::SimpleXMLParser()
    : m_pCurNode(nullptr)
    , m_pCurData(nullptr)
{
    m_aParser = XML_ParserCreate( nullptr );
    XML_SetUserData( m_aParser, this );
    XML_SetElementHandler( m_aParser, StartElementHandler, EndElementHandler );
    XML_SetCharacterDataHandler( m_aParser, CharacterDataHandler );
    XML_SetCommentHandler( m_aParser, CommentHandler );
    XML_SetDefaultHandler( m_aParser, DefaultHandler );
}

SimpleXMLParser::~SimpleXMLParser()
{
    XML_ParserFree( m_aParser );
}

void SimpleXMLParser::StartElementHandler(
    void *userData, const XML_Char *name, const XML_Char **atts )
{
    static_cast<SimpleXMLParser *>(userData)->StartElement( name, atts );
}

void SimpleXMLParser::EndElementHandler(
    void *userData, const XML_Char * /*name*/ )
{
    static_cast<SimpleXMLParser *>(userData)->EndElement();
}

void SimpleXMLParser::CharacterDataHandler(
    void *userData, const XML_Char *s, int len )
{
    static_cast<SimpleXMLParser *>(userData)->CharacterData( s, len );
}

void SimpleXMLParser::CommentHandler(
    void *userData, const XML_Char *data )
{
    static_cast<SimpleXMLParser *>(userData)->Comment( data );
}

void SimpleXMLParser::DefaultHandler(
    void *userData, const XML_Char *s, int len )
{
    static_cast<SimpleXMLParser *>(userData)->Default( s, len );
}

void SimpleXMLParser::StartElement(
    const XML_Char *name, const XML_Char **atts )
{
    XMLElement *pElement = new XMLElement( OString(name), m_pCurNode );
    m_pCurNode = pElement;
    m_pCurData = nullptr;

    int i = 0;
    while( atts[i] )
    {
        pElement->AddAttribute( atts[ i ], atts[ i + 1 ] );
        i += 2;
    }
}

void SimpleXMLParser::EndElement()
{
    m_pCurNode = m_pCurNode->GetParent();
    m_pCurData = nullptr;
}

void SimpleXMLParser::CharacterData( const XML_Char *s, int len )
{
    if ( !m_pCurData )
    {
        OString x( s, len );
        m_pCurData = new XMLData( helper::UnQuotHTML(x) , m_pCurNode );
    }
    else
    {
        OString x( s, len );
        m_pCurData->AddData( helper::UnQuotHTML(x) );

    }
}

void SimpleXMLParser::Comment( const XML_Char *data )
{
    m_pCurData = nullptr;
    new XMLComment( OString( data ), m_pCurNode );
}

void SimpleXMLParser::Default( const XML_Char *s, int len )
{
    m_pCurData = nullptr;
    new XMLDefault(OString( s, len ), m_pCurNode );
}

bool SimpleXMLParser::Execute( const OString &rFileName, XMLFile* pXMLFile )
{
    m_aErrorInformation.m_eCode = XML_ERROR_NONE;
    m_aErrorInformation.m_nLine = 0;
    m_aErrorInformation.m_nColumn = 0;
    m_aErrorInformation.m_sMessage = "ERROR: Unable to open file "_ostr;
    m_aErrorInformation.m_sMessage += rFileName;

    OUString aFileURL(lcl_pathnameToAbsoluteUrl(rFileName));

    oslFileHandle h;
    if (osl_openFile(aFileURL.pData, &h, osl_File_OpenFlag_Read)
        != osl_File_E_None)
    {
        return false;
    }

    sal_uInt64 s;
    oslFileError e = osl_getFileSize(h, &s);
    void * p = nullptr;
    if (e == osl_File_E_None)
    {
        e = osl_mapFile(h, &p, s, 0, 0);
    }
    if (e != osl_File_E_None)
    {
        osl_closeFile(h);
        return false;
    }

    pXMLFile->SetName( rFileName );

    m_pCurNode = pXMLFile;
    m_pCurData = nullptr;

    m_aErrorInformation.m_eCode = XML_ERROR_NONE;
    m_aErrorInformation.m_nLine = 0;
    m_aErrorInformation.m_nColumn = 0;
    if ( !pXMLFile->GetName().isEmpty())
    {
        m_aErrorInformation.m_sMessage = "File " + pXMLFile->GetName() + " parsed successfully";
    }
    else
        m_aErrorInformation.m_sMessage = "XML-File parsed successfully"_ostr;

    bool result = XML_Parse(m_aParser, static_cast< char * >(p), s, true);
    if (!result)
    {
        m_aErrorInformation.m_eCode = XML_GetErrorCode( m_aParser );
        m_aErrorInformation.m_nLine = XML_GetErrorLineNumber( m_aParser );
        m_aErrorInformation.m_nColumn = XML_GetErrorColumnNumber( m_aParser );

        m_aErrorInformation.m_sMessage = "ERROR: "_ostr;
        if ( !pXMLFile->GetName().isEmpty())
            m_aErrorInformation.m_sMessage += pXMLFile->GetName();
        else
            m_aErrorInformation.m_sMessage += "XML-File (";

        m_aErrorInformation.m_sMessage +=
            OString::number(sal::static_int_cast< sal_Int64 >(m_aErrorInformation.m_nLine)) + "," +
            OString::number(sal::static_int_cast< sal_Int64 >(m_aErrorInformation.m_nColumn)) + "): ";

        switch (m_aErrorInformation.m_eCode)
        {
        case XML_ERROR_NO_MEMORY:
            m_aErrorInformation.m_sMessage += "No memory";
            break;
        case XML_ERROR_SYNTAX:
            m_aErrorInformation.m_sMessage += "Syntax";
            break;
        case XML_ERROR_NO_ELEMENTS:
            m_aErrorInformation.m_sMessage += "No elements";
            break;
        case XML_ERROR_INVALID_TOKEN:
            m_aErrorInformation.m_sMessage += "Invalid token";
            break;
        case XML_ERROR_UNCLOSED_TOKEN:
            m_aErrorInformation.m_sMessage += "Unclosed token";
            break;
        case XML_ERROR_PARTIAL_CHAR:
            m_aErrorInformation.m_sMessage += "Partial char";
            break;
        case XML_ERROR_TAG_MISMATCH:
            m_aErrorInformation.m_sMessage += "Tag mismatch";
            break;
        case XML_ERROR_DUPLICATE_ATTRIBUTE:
            m_aErrorInformation.m_sMessage += "Duplicated attribute";
            break;
        case XML_ERROR_JUNK_AFTER_DOC_ELEMENT:
            m_aErrorInformation.m_sMessage += "Junk after doc element";
            break;
        case XML_ERROR_PARAM_ENTITY_REF:
            m_aErrorInformation.m_sMessage += "Param entity ref";
            break;
        case XML_ERROR_UNDEFINED_ENTITY:
            m_aErrorInformation.m_sMessage += "Undefined entity";
            break;
        case XML_ERROR_RECURSIVE_ENTITY_REF:
            m_aErrorInformation.m_sMessage += "Recursive entity ref";
            break;
        case XML_ERROR_ASYNC_ENTITY:
            m_aErrorInformation.m_sMessage += "Async_entity";
            break;
        case XML_ERROR_BAD_CHAR_REF:
            m_aErrorInformation.m_sMessage += "Bad char ref";
            break;
        case XML_ERROR_BINARY_ENTITY_REF:
            m_aErrorInformation.m_sMessage += "Binary entity";
            break;
        case XML_ERROR_ATTRIBUTE_EXTERNAL_ENTITY_REF:
            m_aErrorInformation.m_sMessage += "Attribute external entity ref";
            break;
        case XML_ERROR_MISPLACED_XML_PI:
            m_aErrorInformation.m_sMessage += "Misplaced xml pi";
            break;
        case XML_ERROR_UNKNOWN_ENCODING:
            m_aErrorInformation.m_sMessage += "Unknown encoding";
            break;
        case XML_ERROR_INCORRECT_ENCODING:
            m_aErrorInformation.m_sMessage += "Incorrect encoding";
            break;
        case XML_ERROR_UNCLOSED_CDATA_SECTION:
            m_aErrorInformation.m_sMessage += "Unclosed cdata section";
            break;
        case XML_ERROR_EXTERNAL_ENTITY_HANDLING:
            m_aErrorInformation.m_sMessage += "External entity handling";
            break;
        case XML_ERROR_NOT_STANDALONE:
            m_aErrorInformation.m_sMessage += "Not standalone";
            break;
        case XML_ERROR_NONE:
            break;
        default:
            break;
        }
    }

    osl_unmapMappedFile(h, p, s);
    osl_closeFile(h);

    return result;
}

namespace
{

icu::UnicodeString lcl_QuotRange(
    const icu::UnicodeString& rString, const sal_Int32 nStart,
    const sal_Int32 nEnd, bool bInsideTag = false )
{
    icu::UnicodeString sReturn;
    assert( nStart < nEnd );
    assert( nStart >= 0 );
    assert( nEnd <= rString.length() );
    for (sal_Int32 i = nStart; i < nEnd; ++i)
    {
        switch (rString[i])
        {
            case '<':
                sReturn.append("&lt;");
                break;
            case '>':
                sReturn.append("&gt;");
                break;
            case '"':
                if( !bInsideTag )
                    sReturn.append("&quot;");
                else
                    sReturn.append(rString[i]);
                break;
            case '&':
                if (rString.startsWith("&amp;", i, 5))
                    sReturn.append('&');
                else
                    sReturn.append("&amp;");
                break;
            default:
                sReturn.append(rString[i]);
                break;
        }
    }
    return sReturn;
}

bool lcl_isTag( const icu::UnicodeString& rString )
{
    static const int nSize = 20;
    static const icu::UnicodeString vTags[nSize] = {
        "ahelp", "link", "item", "emph", "defaultinline",
        "switchinline", "caseinline", "variable",
        "bookmark_value", "image", "object",
        "embedvar", "alt", "sup", "sub",
        "menuitem", "keycode", "input", "literal", "widget"
    };

    for( int nIndex = 0; nIndex < nSize; ++nIndex )
    {
        if( rString.startsWith("<" + vTags[nIndex]) ||
             rString == "</" + vTags[nIndex] + ">" )
            return true;
    }

    return rString == "<br/>" || rString =="<help-id-missing/>";
}

} /// anonymous namespace

OString XMLUtil::QuotHTML( const OString &rString )
{
    if( o3tl::trim(rString).empty() )
        return rString;
    UErrorCode nIcuErr = U_ZERO_ERROR;
    static const sal_uInt32 nSearchFlags =
        UREGEX_DOTALL | UREGEX_CASE_INSENSITIVE;
    static const icu::UnicodeString sSearchPat( "<[/]\?\?[a-z_-]+?(?:| +[a-z]+?=\".*?\") *[/]\?\?>" );

    const OUString sOUSource = OStringToOUString(rString, RTL_TEXTENCODING_UTF8);
    icu::UnicodeString sSource(
        reinterpret_cast<const UChar*>(
            sOUSource.getStr()), sOUSource.getLength() );

    icu::RegexMatcher aRegexMatcher( sSearchPat, nSearchFlags, nIcuErr );
    aRegexMatcher.reset( sSource );

    icu::UnicodeString sReturn;
    int32_t nEndPos = 0;
    int32_t nStartPos = 0;
    while( aRegexMatcher.find(nStartPos, nIcuErr) && U_SUCCESS(nIcuErr) )
    {
        nStartPos = aRegexMatcher.start(nIcuErr);
        if ( nEndPos < nStartPos )
            sReturn.append(lcl_QuotRange(sSource, nEndPos, nStartPos));
        nEndPos = aRegexMatcher.end(nIcuErr);
        icu::UnicodeString sMatch = aRegexMatcher.group(nIcuErr);
        if( lcl_isTag(sMatch) )
        {
            sReturn.append("<");
            sReturn.append(lcl_QuotRange(sSource, nStartPos+1, nEndPos-1, true));
            sReturn.append(">");
        }
        else
            sReturn.append(lcl_QuotRange(sSource, nStartPos, nEndPos));
        nStartPos = nEndPos;
    }
    if( nEndPos < sSource.length() )
        sReturn.append(lcl_QuotRange(sSource, nEndPos, sSource.length()));
    sReturn.append('\0');
    return
        OUStringToOString(
            reinterpret_cast<const sal_Unicode*>(sReturn.getBuffer()),
            RTL_TEXTENCODING_UTF8);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
