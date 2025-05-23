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

#include <set>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/xml/sax/XFastContextHandler.hpp>
#include <oox/mathml/importutils.hxx>
#include <rtl/ref.hxx>
#include "OOXMLParserState.hxx"
#include "OOXMLPropertySet.hxx"
#include "ShadowContext.hxx"

namespace writerfilter::ooxml
{
class OOXMLDocumentImpl;

class OOXMLFastContextHandler: public ::cppu::WeakImplHelper<css::xml::sax::XFastContextHandler>
{
public:
    typedef tools::SvRef<OOXMLFastContextHandler> Pointer_t;

    enum ResourceEnum_t { UNKNOWN, STREAM, PROPERTIES, TABLE, SHAPE };

    explicit OOXMLFastContextHandler(css::uno::Reference< css::uno::XComponentContext > const & context);

    explicit OOXMLFastContextHandler(OOXMLFastContextHandler * pContext);

    OOXMLFastContextHandler(OOXMLFastContextHandler const &) = default;

    virtual ~OOXMLFastContextHandler() override;

    // css::xml::sax::XFastContextHandler:
    virtual void SAL_CALL startFastElement (sal_Int32 Element, const css::uno::Reference< css::xml::sax::XFastAttributeList >& Attribs) override final;

    virtual void SAL_CALL startUnknownElement(const OUString & Namespace, const OUString & Name, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void SAL_CALL endFastElement(sal_Int32 Element) override;

    virtual void SAL_CALL endUnknownElement(const OUString & Namespace, const OUString & Name) override;

    virtual css::uno::Reference<css::xml::sax::XFastContextHandler> SAL_CALL createFastChildContext(sal_Int32 Element,
        const css::uno::Reference<css::xml::sax::XFastAttributeList>& Attribs) override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createUnknownChildContext(const OUString & Namespace, const OUString & Name,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void SAL_CALL characters(const OUString & aChars) override;

    // local

    void setStream(Stream * pStream);

    /**
       Return value of this context(element).

       @return  the value
     */
    virtual OOXMLValue getValue() const;

    /**
       Returns a string describing the type of the context.

       This is the name of the define normally.

       @return type string
     */
    virtual std::string getType() const { return "??"; }

    virtual ResourceEnum_t getResource() const { return STREAM; }

    /// @throws css::uno::RuntimeException
    /// @throws css::xml::sax::SAXException
    virtual void attributes(const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs);

    virtual void newProperty(Id aId, const OOXMLValue& pVal);
    virtual void setPropertySet(const OOXMLPropertySet::Pointer_t& pPropertySet);
    virtual OOXMLPropertySet::Pointer_t getPropertySet() const;

    virtual void setToken(Token_t nToken);
    virtual Token_t getToken() const;

    void resolveFootnote(const sal_Int32 nId);
    void resolveEndnote(const sal_Int32 nId);
    void resolveComment(const sal_Int32 nId);
    void resolvePicture(const OUString & rId);
    void resolveHeader(const sal_Int32 type,
                                const OUString & rId);
    void resolveFooter(const sal_Int32 type,
                                const OUString & rId);
    void resolveData(const OUString & rId);

    OUString getTargetForId(const OUString & rId);

    void setDocument(OOXMLDocumentImpl* pDocument);
    OOXMLDocumentImpl* getDocument();
    void setXNoteId(const OOXMLValue& pValue);
    void setXNoteId(const sal_Int32 nId);
    sal_Int32 getXNoteId() const;
    void setForwardEvents(bool bForwardEvents);
    bool isForwardEvents() const;
    virtual void setId(Id nId);
    virtual Id getId() const;

    void setDefine(Id nDefine);
    Id getDefine() const { return mnDefine;}

    const OOXMLParserState::Pointer_t& getParserState() const { return mpParserState;}

    void sendTableDepth() const;
    void setHandle();

    void startSectionGroup();
    void setLastParagraphInSection();
    void setLastSectionGroup();
    void endSectionGroup();
    void startParagraphGroup();
    void endParagraphGroup();
    void startCharacterGroup();
    void endCharacterGroup();
    virtual void pushBiDiEmbedLevel();
    virtual void popBiDiEmbedLevel();
    void startSdt();
    void endSdt();
    void startSdtRun();
    void endSdtRun();

    void startField();
    void fieldSeparator();
    void endField();
    void lockField();
    void ftnednref();
    void ftnedncont();
    void ftnednsep();
    void pgNum();
    void tab();
    void symbol();
    void cr();
    void noBreakHyphen();
    void softHyphen();
    void handleLastParagraphInSection();
    void endOfParagraph();
    void text(const OUString & sText);
    void positionOffset(const OUString & sText);
    static void ignore();
    void alignH(const OUString & sText);
    void alignV(const OUString & sText);
    void positivePercentage(const OUString& rText);
    void startGlossaryEntry();
    void endGlossaryEntry();
    void startTxbxContent();
    void endTxbxContent();
    void propagateCharacterProperties();
    void propagateTableProperties();
    void propagateRowProperties();
    void propagateCellProperties();
    void sendPropertiesWithId(Id nId);
    void sendPropertiesToParent();
    void sendCellProperties();
    void sendRowProperties();
    void sendTableProperties();
    void clearTableProps();
    void clearProps();

    virtual void setDefaultBooleanValue();
    virtual void setDefaultIntegerValue();
    virtual void setDefaultHexValue();
    virtual void setDefaultStringValue();

    void sendPropertyToParent();
    OOXMLFastContextHandler* getParent() const { return mpParent; }
    void setGridAfter(const OOXMLValue& pGridAfter) { maGridAfter = pGridAfter; }

protected:
    OOXMLFastContextHandler * mpParent;
    Id mId;
    Id mnDefine;
    Token_t mnToken;

    // the formula insertion mode: inline/newline(left, center, right)
    sal_Int8 mnMathJcVal;
    bool mbIsMathPara;
    enum eMathParaJc
    {
        INLINE, //The equation is anchored as inline to the text
        CENTER, //The equation is center aligned
        LEFT,   //The equation is left aligned
        RIGHT  //The equation is right aligned
    };
    // the stream to send the stream events to.
    Stream * mpStream;

    // the current global parser state
    OOXMLParserState::Pointer_t mpParserState;

    // the table depth of this context
    unsigned int mnTableDepth;

    /// @throws css::uno::RuntimeException
    /// @throws css::xml::sax::SAXException
    virtual void lcl_startFastElement(Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs);

    /// @throws css::uno::RuntimeException
    /// @throws css::xml::sax::SAXException
    virtual void lcl_endFastElement(Token_t Element);

    /// @throws css::uno::RuntimeException
    /// @throws css::xml::sax::SAXException
    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > lcl_createFastChildContext(Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs);

    /// @throws css::uno::RuntimeException
    /// @throws css::xml::sax::SAXException
    virtual void lcl_characters(const OUString & aChars);

    void startAction();
    void endAction();

    bool m_inPositionV;
    bool mbAllowInCell; // o:allowincell
    bool mbIsVMLfound;
    OOXMLValue maGridAfter;

private:
    void operator =(OOXMLFastContextHandler const &) = delete;
    /// Handles AlternateContent. Returns true, if children of the current element should be ignored.
    bool prepareMceContext(Token_t nElement, const css::uno::Reference<css::xml::sax::XFastAttributeList>& Attribs);

    // 2.10 of XML 1.0 specification
    bool IsPreserveSpace() const;

    css::uno::Reference< css::uno::XComponentContext > m_xContext;
    bool m_bDiscardChildren;
    bool m_bTookChoice; ///< Did we take the Choice or want Fallback instead?
    bool mbPreserveSpace = false;
    bool mbPreserveSpaceSet = false;

};

class OOXMLFastContextHandlerStream : public OOXMLFastContextHandler
{
public:
    explicit OOXMLFastContextHandlerStream(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerStream() override;

    virtual ResourceEnum_t getResource() const override { return STREAM; }

    const OOXMLPropertySet::Pointer_t& getPropertySetAttrs() const { return mpPropertySetAttrs;}

    virtual void newProperty(Id aId, const OOXMLValue& pVal) override;
    void sendProperty(Id nId);
    virtual OOXMLPropertySet::Pointer_t getPropertySet() const override;

    void handleHyperlink();

private:
    mutable OOXMLPropertySet::Pointer_t mpPropertySetAttrs;
};

class OOXMLFastContextHandlerProperties : public OOXMLFastContextHandler
{
public:
    explicit OOXMLFastContextHandlerProperties(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerProperties() override;

    virtual OOXMLValue getValue() const override;
    virtual ResourceEnum_t getResource() const override { return PROPERTIES; }

    virtual void newProperty(Id nId, const OOXMLValue& pVal) override;

    void handleXNotes();
    void handleHdrFtr();
    void handleComment();
    void handlePicture();
    void handleBreak();
    void handleOutOfOrderBreak();
    void handleOLE();
    void handleFontRel();
    void handleHyperlinkURL();
    void handleAltChunk();

    virtual void setPropertySet(const OOXMLPropertySet::Pointer_t& pPropertySet) override;
    virtual OOXMLPropertySet::Pointer_t getPropertySet() const override;

protected:
    /// the properties
    OOXMLPropertySet::Pointer_t mpPropertySet;

    virtual void lcl_endFastElement(Token_t Element) override;

private:

    bool mbResolve;
};

class OOXMLFastContextHandlerPropertyTable :
    public OOXMLFastContextHandlerProperties
{
public:
    explicit OOXMLFastContextHandlerPropertyTable(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerPropertyTable() override;

private:
    OOXMLTable mTable;

    virtual void lcl_endFastElement(Token_t Element) override;
};

class OOXMLFastContextHandlerValue :
    public OOXMLFastContextHandler
{
public:
    explicit OOXMLFastContextHandlerValue(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerValue() override;

    void setValue(const OOXMLValue& pValue);
    virtual OOXMLValue getValue() const override;

    virtual void lcl_endFastElement(Token_t Element) override;

    virtual std::string getType() const override { return "Value"; }

    virtual void setDefaultBooleanValue() override;
    virtual void setDefaultIntegerValue() override;
    virtual void setDefaultHexValue() override;
    virtual void setDefaultStringValue() override;

    virtual void pushBiDiEmbedLevel() override;
    virtual void popBiDiEmbedLevel() override;

    void handleGridAfter();

private:
    OOXMLValue maValue;
};

class OOXMLFastContextHandlerTable : public OOXMLFastContextHandler
{
public:
    explicit OOXMLFastContextHandlerTable(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerTable() override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createFastChildContext (sal_Int32 Element,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

private:
    OOXMLTable mTable;

    css::uno::Reference<css::xml::sax::XFastContextHandler> mCurrentChild;

    virtual void lcl_endFastElement(Token_t Element) override;

    virtual ResourceEnum_t getResource() const override { return TABLE; }

    virtual std::string getType() const override { return "Table"; }

    void addCurrentChild();
};

class OOXMLFastContextHandlerXNote : public OOXMLFastContextHandlerProperties
{
public:
    explicit OOXMLFastContextHandlerXNote(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerXNote() override;

    void checkId(const OOXMLValue& pValue);

    void checkType(const OOXMLValue& pValue);

    virtual std::string getType() const override { return "XNote"; }

private:
    bool mbForwardEventsSaved;
    sal_Int32 mnMyXNoteId;
    sal_Int32 mnMyXNoteType;

    virtual void lcl_startFastElement(Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void lcl_endFastElement(Token_t Element) override;

    virtual ResourceEnum_t getResource() const override { return STREAM; }
};

class OOXMLFastContextHandlerTextTableCell : public OOXMLFastContextHandler
{
public:
    explicit OOXMLFastContextHandlerTextTableCell(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerTextTableCell() override;

    virtual std::string getType() const override { return "TextTableCell"; }

    void startCell();
    void endCell();
};

class OOXMLFastContextHandlerTextTableRow : public OOXMLFastContextHandler
{
public:
    explicit OOXMLFastContextHandlerTextTableRow(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerTextTableRow() override;

    virtual std::string getType() const override { return "TextTableRow"; }

    static void startRow();
    void endRow();
    void handleGridBefore( const OOXMLValue& val );
};

class OOXMLFastContextHandlerTextTable : public OOXMLFastContextHandler
{
public:
    explicit OOXMLFastContextHandlerTextTable(OOXMLFastContextHandler * pContext);

    virtual ~OOXMLFastContextHandlerTextTable() override;

    virtual std::string getType() const override { return "TextTable"; }

    // tdf#111550
    // when <w:tbl> appears as direct child of <w:p>, we need to rearrange this paragraph
    // to merge with the table's first paragraph (that's what Word does in this case)
    void start_P_Tbl();
protected:
    virtual void lcl_startFastElement(Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void lcl_endFastElement(Token_t Element) override;
};

class OOXMLFastContextHandlerShape: public OOXMLFastContextHandlerProperties
{
    bool m_bShapeSent;
    bool m_bShapeStarted;
    /// Is it necessary to pop the stack in the dtor?
    bool m_bShapeContextPushed;
    rtl::Reference<oox::shape::ShapeContextHandler> mrShapeContext;

public:
    explicit OOXMLFastContextHandlerShape(OOXMLFastContextHandler * pContext);
    virtual ~OOXMLFastContextHandlerShape() override;

    virtual std::string getType() const override { return "Shape"; }

    // css::xml::sax::XFastContextHandler:
    virtual void SAL_CALL startUnknownElement (const OUString & Namespace, const OUString & Name, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void SAL_CALL endUnknownElement(const OUString & Namespace, const OUString & Name) override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createUnknownChildContext(const OUString & Namespace, const OUString & Name,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void setToken(Token_t nToken) override;

    virtual ResourceEnum_t getResource() const override { return SHAPE; }

    void sendShape( Token_t Element );
    bool isShapeSent( ) const { return m_bShapeSent; }
    bool isDMLGroupShape() const;

protected:
    virtual void lcl_startFastElement(Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void lcl_endFastElement(Token_t Element) override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > lcl_createFastChildContext (Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void lcl_characters(const OUString & aChars) override;

};

/**
   OOXMLFastContextHandlerWrapper wraps an OOXMLFastContextHandler.

   The method calls for the interface css::xml::sax::XFastContextHandler are
   forwarded to the wrapped OOXMLFastContextHandler.
 */
class OOXMLFastContextHandlerWrapper : public OOXMLFastContextHandler
{
public:
    OOXMLFastContextHandlerWrapper(OOXMLFastContextHandler * pParent,
                                   css::uno::Reference<css::xml::sax::XFastContextHandler> const & xContext,
            rtl::Reference<OOXMLFastContextHandlerShape> const & xShapeHandler);
    OOXMLFastContextHandlerWrapper(OOXMLFastContextHandler * pParent,
                                   rtl::Reference<ShadowContext> const & xContext,
                                   css::uno::Reference<css::xml::sax::XFastContextHandler> const & xParentContext,
            rtl::Reference<OOXMLFastContextHandlerShape> const & xShapeHandler);
    virtual ~OOXMLFastContextHandlerWrapper() override;

    // css::xml::sax::XFastContextHandler:
    virtual void SAL_CALL endFastElement( ::sal_Int32 Element ) override;
    virtual void SAL_CALL startUnknownElement(const OUString & Namespace, const OUString & Name, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void SAL_CALL endUnknownElement(const OUString & Namespace, const OUString & Name) override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > SAL_CALL createUnknownChildContext (const OUString & Namespace, const OUString & Name,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void attributes(const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual ResourceEnum_t getResource() const override;

    void addNamespace(Id nId);
    void addToken( Token_t Element );

    virtual void newProperty(Id nId, const OOXMLValue& pVal) override;
    virtual void setPropertySet(const OOXMLPropertySet::Pointer_t& pPropertySet) override;
    virtual OOXMLPropertySet::Pointer_t getPropertySet() const override;

    virtual std::string getType() const override;

protected:
    virtual void lcl_startFastElement(Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void lcl_endFastElement(Token_t Element) override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > lcl_createFastChildContext(Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void lcl_characters(const OUString & aChars) override;

    virtual void setId(Id nId) override;
    virtual Id getId() const override;

    virtual void setToken(Token_t nToken) override;
    virtual Token_t getToken() const override;

private:
    css::uno::Reference<css::xml::sax::XFastContextHandler> mxWrappedContext;
    rtl::Reference<OOXMLFastContextHandlerShape> mxShapeHandler;
    std::set<Id> mMyNamespaces;
    std::set<Token_t> mMyTokens;
    OOXMLPropertySet::Pointer_t mpPropertySet;
    rtl::Reference<ShadowContext> const mxShadowContext;
    css::uno::Reference<css::xml::sax::XFastContextHandler> mxReplayParentContext;
    bool mbIsWriterFrameDetected;
    bool mbIsReplayTextBox;

    OOXMLFastContextHandler * getFastContextHandler() const;
};

/**
 A class that converts from XFastParser/XFastContextHandler usage to a liner XML stream of data.

 The purpose of this class is to convert the rather complex XFastContextHandler-based XML
 processing that requires context subclasses, callbacks, etc. into a linear stream of XML tokens
 that can be handled simply by reading the tokens one by one and directly processing them.
 See the oox::formulaimport::XmlStream class documentation for more information.

 Usage: Create a subclass of OOXMLFastContextHandlerLinear, reimplemented getType() to provide
 type of the subclass and process() to actually process the XML stream. Also make sure to
 add a line like the following to model.xml (for class OOXMLFastContextHandlerMath):

 <resource name="CT_OMath" resource="Math"/>

 @since 3.5
*/
class OOXMLFastContextHandlerLinear: public OOXMLFastContextHandlerProperties
{
public:
    explicit OOXMLFastContextHandlerLinear(OOXMLFastContextHandler * pContext);
    /**
     Return the type of the class, as written in model.xml .
     */
    virtual std::string getType() const override = 0;

protected:
    /**
     Called when the tokens for the element, its content and sub-elements have been linearized
     and should be processed. The data member @ref buffer contains the converted data.
    */
    virtual void process() = 0;

    virtual void lcl_startFastElement(Token_t Element, const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void lcl_endFastElement(Token_t Element) override;

    virtual css::uno::Reference< css::xml::sax::XFastContextHandler > lcl_createFastChildContext(Token_t Element,
        const css::uno::Reference< css::xml::sax::XFastAttributeList > & Attribs) override;

    virtual void lcl_characters(const OUString & aChars) override;

    // should be private, but not much point in making deep copies of it
    oox::formulaimport::XmlStreamBuilder m_buffer;

private:
    int m_depthCount;
};

class OOXMLFastContextHandlerMath: public OOXMLFastContextHandlerLinear
{
public:
    explicit OOXMLFastContextHandlerMath(OOXMLFastContextHandler * pContext);
    virtual std::string getType() const override { return "Math"; }
protected:
    virtual void process() override;
};

/**
 A class that reads individual w15:commentEx elements from commentsExtended stream [MS-DOCX].

 It is used to pre-populate the extended comment properties in domain mapper. The stream
 contains information about resolved state of the comments ("done" attribute) and the parent
 comment (the one that this comment answers to).

 Note that the data is linked to paraId identifiers (also introduced in [MS-DOCX]), which
 correspond to paragraphs, not directly to comment ids.

 @since 7.2
*/
class OOXMLFastContextHandlerCommentEx : public OOXMLFastContextHandler
{
public:
    explicit OOXMLFastContextHandlerCommentEx(OOXMLFastContextHandler* pContext);

    virtual std::string getType() const override { return "CommentEx"; }
    virtual void lcl_endFastElement(Token_t Element) override;

    void att_paraId(const OOXMLValue& pValue);
    void att_done(const OOXMLValue& pValue);
    void att_paraIdParent(const OOXMLValue& pValue);

private:
    OUString m_sParaId;
    bool m_bDone = false;
    OUString m_sParentId {};
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
