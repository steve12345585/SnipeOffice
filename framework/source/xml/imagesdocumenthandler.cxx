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

#include <xml/imagesdocumenthandler.hxx>

#include <com/sun/star/xml/sax/XExtendedDocumentHandler.hpp>
#include <com/sun/star/xml/sax/SAXException.hpp>

#include <rtl/ref.hxx>
#include <rtl/ustrbuf.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::xml::sax;

#define ELEMENT_IMAGECONTAINER      "imagescontainer"
#define ELEMENT_IMAGES              "images"
#define ELEMENT_ENTRY               "entry"
#define ELEMENT_EXTERNALIMAGES      "externalimages"
#define ELEMENT_EXTERNALENTRY       "externalentry"

constexpr OUString ELEMENT_NS_IMAGESCONTAINER = u"image:imagescontainer"_ustr;
constexpr OUString ELEMENT_NS_IMAGES = u"image:images"_ustr;
constexpr OUString ELEMENT_NS_ENTRY = u"image:entry"_ustr;

#define ATTRIBUTE_HREF                  "href"
#define ATTRIBUTE_MASKCOLOR             "maskcolor"
#define ATTRIBUTE_COMMAND               "command"
#define ATTRIBUTE_BITMAPINDEX           "bitmap-index"
#define ATTRIBUTE_MASKURL               "maskurl"
#define ATTRIBUTE_MASKMODE              "maskmode"
#define ATTRIBUTE_HIGHCONTRASTURL       "highcontrasturl"
#define ATTRIBUTE_HIGHCONTRASTMASKURL   "highcontrastmaskurl"

constexpr OUStringLiteral ATTRIBUTE_XMLNS_IMAGE = u"xmlns:image";
constexpr OUStringLiteral ATTRIBUTE_XMLNS_XLINK = u"xmlns:xlink";

constexpr OUString ATTRIBUTE_XLINK_HREF = u"xlink:href"_ustr;
constexpr OUStringLiteral ATTRIBUTE_XLINK_TYPE = u"xlink:type";
constexpr OUStringLiteral ATTRIBUTE_XLINK_TYPE_VALUE = u"simple";

constexpr OUString XMLNS_IMAGE = u"http://openoffice.org/2001/image"_ustr;
constexpr OUString XMLNS_XLINK = u"http://www.w3.org/1999/xlink"_ustr;
constexpr OUStringLiteral XMLNS_IMAGE_PREFIX = u"image:";

constexpr OUStringLiteral XMLNS_FILTER_SEPARATOR = u"^";

constexpr OUStringLiteral IMAGES_DOCTYPE = u"<!DOCTYPE image:imagecontainer PUBLIC \"-//OpenOffice.org//DTD OfficeDocument 1.0//EN\" \"image.dtd\">";

namespace framework
{

namespace {

struct ImageXMLEntryProperty
{
    OReadImagesDocumentHandler::Image_XML_Namespace nNamespace;
    char                                            aEntryName[20];
};

}

ImageXMLEntryProperty const ImagesEntries[OReadImagesDocumentHandler::IMG_XML_ENTRY_COUNT] =
{
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ELEMENT_IMAGECONTAINER          },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ELEMENT_IMAGES                  },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ELEMENT_ENTRY                   },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ELEMENT_EXTERNALIMAGES          },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ELEMENT_EXTERNALENTRY           },
    { OReadImagesDocumentHandler::IMG_NS_XLINK, ATTRIBUTE_HREF                  },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ATTRIBUTE_MASKCOLOR             },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ATTRIBUTE_COMMAND               },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ATTRIBUTE_BITMAPINDEX           },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ATTRIBUTE_MASKURL               },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ATTRIBUTE_MASKMODE              },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ATTRIBUTE_HIGHCONTRASTURL       },
    { OReadImagesDocumentHandler::IMG_NS_IMAGE, ATTRIBUTE_HIGHCONTRASTMASKURL   }
};

OReadImagesDocumentHandler::OReadImagesDocumentHandler( ImageItemDescriptorList& rItems ) :
    m_rImageList( rItems )
{
    // create hash map to speed up lookup
    for ( int i = 0; i < IMG_XML_ENTRY_COUNT; i++ )
    {
        OUStringBuffer temp( 20 );

        if ( ImagesEntries[i].nNamespace == IMG_NS_IMAGE )
            temp.append( XMLNS_IMAGE );
        else
            temp.append( XMLNS_XLINK );

        temp.append( XMLNS_FILTER_SEPARATOR );
        temp.appendAscii( ImagesEntries[i].aEntryName );
        m_aImageMap.emplace( temp.makeStringAndClear(), static_cast<Image_XML_Entry>(i) );
    }

    // reset states
    m_bImageContainerStartFound     = false;
    m_bImageContainerEndFound       = false;
    m_bImagesStartFound             = false;
}

OReadImagesDocumentHandler::~OReadImagesDocumentHandler()
{
}

// XDocumentHandler
void SAL_CALL OReadImagesDocumentHandler::startDocument()
{
}

void SAL_CALL OReadImagesDocumentHandler::endDocument()
{
    if (m_bImageContainerStartFound != m_bImageContainerEndFound)
    {
        OUString aErrorMessage = getErrorLineString() + "No matching start or end element 'image:imagecontainer' found!";
        throw SAXException( aErrorMessage, Reference< XInterface >(), Any() );
    }
}

void SAL_CALL OReadImagesDocumentHandler::startElement(
    const OUString& aName, const Reference< XAttributeList > &xAttribs )
{
    ImageHashMap::const_iterator pImageEntry = m_aImageMap.find( aName );
    if ( pImageEntry == m_aImageMap.end() )
        return;

    switch ( pImageEntry->second )
    {
        case IMG_ELEMENT_IMAGECONTAINER:
        {
            // image:imagecontainer element (container element for all further image elements)
            if ( m_bImageContainerStartFound )
            {
                OUString aErrorMessage = getErrorLineString() + "Element 'image:imagecontainer' cannot be embedded into 'image:imagecontainer'!";
                throw SAXException( aErrorMessage, Reference< XInterface >(), Any() );
            }

            m_bImageContainerStartFound = true;
        }
        break;

        case IMG_ELEMENT_IMAGES:
        {
            if ( !m_bImageContainerStartFound )
            {
                OUString aErrorMessage = getErrorLineString() + "Element 'image:images' must be embedded into element 'image:imagecontainer'!";
                throw SAXException( aErrorMessage, Reference< XInterface >(), Any() );
            }

            if ( m_bImagesStartFound )
            {
                OUString aErrorMessage = getErrorLineString() + "Element 'image:images' cannot be embedded into 'image:images'!";
                throw SAXException( aErrorMessage, Reference< XInterface >(), Any() );
            }

            m_bImagesStartFound = true;
        }
        break;

        case IMG_ELEMENT_ENTRY:
        {
            // Check that image:entry is embedded into image:images!
            if ( !m_bImagesStartFound )
            {
                OUString aErrorMessage = getErrorLineString() + "Element 'image:entry' must be embedded into element 'image:images'!";
                throw SAXException( aErrorMessage, Reference< XInterface >(), Any() );
            }

            // Create new image item descriptor
            ImageItemDescriptor aItem;

            // Read attributes for this image definition
            for ( sal_Int16 n = 0; n < xAttribs->getLength(); n++ )
            {
                pImageEntry = m_aImageMap.find( xAttribs->getNameByIndex( n ) );
                if ( pImageEntry != m_aImageMap.end() )
                {
                    switch ( pImageEntry->second )
                    {
                        case IMG_ATTRIBUTE_COMMAND:
                        {
                            aItem.aCommandURL  = xAttribs->getValueByIndex( n );
                        }
                        break;

                        default:
                            break;
                    }
                }
            }

            // Check required attribute "command"
            if ( aItem.aCommandURL.isEmpty() )
            {
                OUString aErrorMessage = getErrorLineString() + "Required attribute 'image:command' must have a value!";
                throw SAXException( aErrorMessage, Reference< XInterface >(), Any() );
            }

            m_rImageList.aImageItemDescriptors.push_back(aItem);
        }
        break;

        default:
        break;
    }
}

void SAL_CALL OReadImagesDocumentHandler::endElement(const OUString& aName)
{
    ImageHashMap::const_iterator pImageEntry = m_aImageMap.find( aName );
    if ( pImageEntry == m_aImageMap.end() )
        return;

    switch ( pImageEntry->second )
    {
        case IMG_ELEMENT_IMAGECONTAINER:
        {
            m_bImageContainerEndFound = true;
        }
        break;

        case IMG_ELEMENT_IMAGES:
        {
            m_bImagesStartFound = false;
        }
        break;

        default: break;
    }
}

void SAL_CALL OReadImagesDocumentHandler::characters(const OUString&)
{
}

void SAL_CALL OReadImagesDocumentHandler::ignorableWhitespace(const OUString&)
{
}

void SAL_CALL OReadImagesDocumentHandler::processingInstruction(
    const OUString& /*aTarget*/, const OUString& /*aData*/ )
{
}

void SAL_CALL OReadImagesDocumentHandler::setDocumentLocator(
    const Reference< XLocator > &xLocator)
{
    m_xLocator = xLocator;
}

OUString OReadImagesDocumentHandler::getErrorLineString()
{
    if ( m_xLocator.is() )
    {
        return "Line: " +
            OUString::number(m_xLocator->getLineNumber()) +
            " - ";
    }
    else
        return OUString();
}

//  OWriteImagesDocumentHandler

OWriteImagesDocumentHandler::OWriteImagesDocumentHandler(
    const ImageItemDescriptorList& rItems,
    Reference< XDocumentHandler > const & rWriteDocumentHandler ) :
    m_rImageItemList( rItems ),
    m_xWriteDocumentHandler( rWriteDocumentHandler )
{
    m_aXMLImageNS           = XMLNS_IMAGE_PREFIX;
    m_aAttributeXlinkType   = ATTRIBUTE_XLINK_TYPE;
    m_aAttributeValueSimple = ATTRIBUTE_XLINK_TYPE_VALUE;
}

OWriteImagesDocumentHandler::~OWriteImagesDocumentHandler()
{
}

void OWriteImagesDocumentHandler::WriteImagesDocument()
{
    m_xWriteDocumentHandler->startDocument();

    // write DOCTYPE line!
    Reference< XExtendedDocumentHandler > xExtendedDocHandler( m_xWriteDocumentHandler, UNO_QUERY );
    if ( xExtendedDocHandler.is() )
    {
        xExtendedDocHandler->unknown( IMAGES_DOCTYPE );
        m_xWriteDocumentHandler->ignorableWhitespace( OUString() );
    }

    rtl::Reference<::comphelper::AttributeList> pList = new ::comphelper::AttributeList;

    pList->AddAttribute( ATTRIBUTE_XMLNS_IMAGE,
                         XMLNS_IMAGE );

    pList->AddAttribute( ATTRIBUTE_XMLNS_XLINK,
                         XMLNS_XLINK );

    m_xWriteDocumentHandler->startElement( ELEMENT_NS_IMAGESCONTAINER, pList );
    m_xWriteDocumentHandler->ignorableWhitespace( OUString() );

    WriteImageList( &m_rImageItemList );

    m_xWriteDocumentHandler->ignorableWhitespace( OUString() );
    m_xWriteDocumentHandler->endElement( ELEMENT_NS_IMAGESCONTAINER );
    m_xWriteDocumentHandler->ignorableWhitespace( OUString() );
    m_xWriteDocumentHandler->endDocument();
}

//  protected member functions

void OWriteImagesDocumentHandler::WriteImageList( const ImageItemDescriptorList* pImageList )
{
    rtl::Reference<::comphelper::AttributeList> pList = new ::comphelper::AttributeList;

    // save required attributes
    pList->AddAttribute( m_aAttributeXlinkType,
                         m_aAttributeValueSimple );

    pList->AddAttribute(ATTRIBUTE_XLINK_HREF, pImageList->aURL);

    m_xWriteDocumentHandler->startElement( ELEMENT_NS_IMAGES, pList );
    m_xWriteDocumentHandler->ignorableWhitespace( OUString() );

    for (const ImageItemDescriptor & i : pImageList->aImageItemDescriptors)
        WriteImage( &i );

    m_xWriteDocumentHandler->endElement( ELEMENT_NS_IMAGES );
    m_xWriteDocumentHandler->ignorableWhitespace( OUString() );
}

void OWriteImagesDocumentHandler::WriteImage( const ImageItemDescriptor* pImage )
{
    rtl::Reference<::comphelper::AttributeList> pList = new ::comphelper::AttributeList;

    pList->AddAttribute(m_aXMLImageNS + ATTRIBUTE_BITMAPINDEX, OUString::number(pImage->nIndex));

    pList->AddAttribute( m_aXMLImageNS + ATTRIBUTE_COMMAND,
                         pImage->aCommandURL );

    m_xWriteDocumentHandler->startElement( ELEMENT_NS_ENTRY, pList );
    m_xWriteDocumentHandler->ignorableWhitespace( OUString() );

    m_xWriteDocumentHandler->endElement( ELEMENT_NS_ENTRY );
    m_xWriteDocumentHandler->ignorableWhitespace( OUString() );
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
