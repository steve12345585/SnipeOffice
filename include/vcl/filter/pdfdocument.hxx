/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 */

#ifndef INCLUDED_VCL_FILTER_PDFDOCUMENT_HXX
#define INCLUDED_VCL_FILTER_PDFDOCUMENT_HXX

#include <memory>
#include <map>
#include <vector>

#include <tools/stream.hxx>
#include <vcl/dllapi.h>
#include <rtl/strbuf.hxx>

#include <vcl/filter/pdfobjectcontainer.hxx>

namespace tools
{
class Rectangle;
}
namespace svl::crypto
{
class SigningContext;
}

namespace vcl::filter
{
class PDFTrailerElement;
class PDFReferenceElement;
class PDFDocument;
class PDFDictionaryElement;
class PDFArrayElement;
class PDFStreamElement;
class PDFNameElement;
class PDFNumberElement;

/// A byte range in a PDF file.
class VCL_DLLPUBLIC PDFElement
{
    bool m_bVisiting = false;
    bool m_bParsing = false;

public:
    PDFElement() = default;
    virtual bool Read(SvStream& rStream) = 0;
    virtual ~PDFElement() = default;
    void setVisiting(bool bVisiting) { m_bVisiting = bVisiting; }
    bool alreadyVisiting() const { return m_bVisiting; }
    void setParsing(bool bParsing) { m_bParsing = bParsing; }
    bool alreadyParsing() const { return m_bParsing; }

    virtual void writeString(OStringBuffer& rBuffer) = 0;
};

/// Indirect object: something with a unique ID.
class VCL_DLLPUBLIC PDFObjectElement final : public PDFElement
{
    /// The document owning this element.
    PDFDocument& m_rDoc;
    double m_fObjectValue;
    double m_fGenerationValue;
    /// If set, the object contains this number element (outside any dictionary/array).
    PDFNumberElement* m_pNumberElement;
    /// If set, the object contains this name element (outside any dictionary/array).
    PDFNameElement* m_pNameElement;
    /// Position after the '<<' token.
    sal_uInt64 m_nDictionaryOffset;
    /// Length of the dictionary buffer till (before) the '>>' token.
    sal_uInt64 m_nDictionaryLength;
    PDFDictionaryElement* m_pDictionaryElement;
    /// Position after the '[' token, if m_pArrayElement is set.
    sal_uInt64 m_nArrayOffset;
    /// Length of the array buffer till (before) the ']' token.
    sal_uInt64 m_nArrayLength;
    /// The contained direct array, if any.
    PDFArrayElement* m_pArrayElement;
    /// The stream of this object, used when this is an object stream.
    PDFStreamElement* m_pStreamElement;
    /// Objects of an object stream.
    std::vector<std::unique_ptr<PDFObjectElement>> m_aStoredElements;
    /// Elements of an object in an object stream.
    std::vector<std::unique_ptr<PDFElement>> m_aElements;
    /// Uncompressed buffer of an object in an object stream.
    std::unique_ptr<SvMemoryStream> m_pStreamBuffer;
    /// List of all reference elements inside this object's dictionary and
    /// nested dictionaries.
    std::vector<PDFReferenceElement*> m_aDictionaryReferences;

    bool m_bParsed;

    SAL_DLLPRIVATE void parseIfNecessary();

public:
    PDFObjectElement(PDFDocument& rDoc, double fObjectValue, double fGenerationValue);
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    PDFElement* Lookup(const OString& rDictionaryKey);
    PDFObjectElement* LookupObject(const OString& rDictionaryKey);
    SAL_DLLPRIVATE double GetObjectValue() const;
    SAL_DLLPRIVATE void SetDictionaryOffset(sal_uInt64 nDictionaryOffset);
    sal_uInt64 GetDictionaryOffset();
    SAL_DLLPRIVATE void SetDictionaryLength(sal_uInt64 nDictionaryLength);
    sal_uInt64 GetDictionaryLength();
    PDFDictionaryElement* GetDictionary();
    SAL_DLLPRIVATE void SetDictionary(PDFDictionaryElement* pDictionaryElement);
    SAL_DLLPRIVATE void SetNumberElement(PDFNumberElement* pNumberElement);
    PDFNumberElement* GetNumberElement() const;
    SAL_DLLPRIVATE void SetNameElement(PDFNameElement* pNameElement);
    PDFNameElement* GetNameElement() const;
    /// Get access to the parsed key-value items from the object dictionary.
    const std::map<OString, PDFElement*>& GetDictionaryItems();
    SAL_DLLPRIVATE const std::vector<PDFReferenceElement*>& GetDictionaryReferences() const;
    SAL_DLLPRIVATE void AddDictionaryReference(PDFReferenceElement* pReference);
    SAL_DLLPRIVATE void SetArray(PDFArrayElement* pArrayElement);
    SAL_DLLPRIVATE void SetStream(PDFStreamElement* pStreamElement);
    /// Access to the stream of the object, if it has any.
    PDFStreamElement* GetStream() const;
    SAL_DLLPRIVATE void SetArrayOffset(sal_uInt64 nArrayOffset);
    SAL_DLLPRIVATE sal_uInt64 GetArrayOffset() const;
    SAL_DLLPRIVATE void SetArrayLength(sal_uInt64 nArrayLength);
    SAL_DLLPRIVATE sal_uInt64 GetArrayLength() const;
    PDFArrayElement* GetArray();
    /// Parse objects stored in this object stream.
    SAL_DLLPRIVATE void ParseStoredObjects();
    SAL_DLLPRIVATE std::vector<std::unique_ptr<PDFElement>>& GetStoredElements();
    SAL_DLLPRIVATE SvMemoryStream* GetStreamBuffer() const;
    SAL_DLLPRIVATE void SetStreamBuffer(std::unique_ptr<SvMemoryStream>& pStreamBuffer);
    SAL_DLLPRIVATE PDFDocument& GetDocument();

    void writeString(OStringBuffer& /*rBuffer*/) override { assert(false && "not implemented"); }
};

/// Array object: a list.
class VCL_DLLPUBLIC PDFArrayElement final : public PDFElement
{
    std::vector<PDFElement*> m_aElements;
    /// The object that contains this array.
    PDFObjectElement* m_pObject;

public:
    PDFArrayElement(PDFObjectElement* pObject);
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    SAL_DLLPRIVATE void PushBack(PDFElement* pElement);
    const std::vector<PDFElement*>& GetElements() const;
    PDFElement* GetElement(size_t nIndex) const { return m_aElements[nIndex]; }

    void writeString(OStringBuffer& rBuffer) override
    {
        rBuffer.append("[ ");
        for (auto& rElement : m_aElements)
        {
            rElement->writeString(rBuffer);
            rBuffer.append(" ");
        }
        rBuffer.append("]");
    }
};

/// Reference object: something with a unique ID.
class VCL_DLLPUBLIC PDFReferenceElement final : public PDFElement
{
    PDFDocument& m_rDoc;
    int m_fObjectValue;
    int m_fGenerationValue;
    /// Location after the 'R' token.
    sal_uInt64 m_nOffset = 0;
    /// The element providing the object number.
    PDFNumberElement& m_rObject;

public:
    PDFReferenceElement(PDFDocument& rDoc, PDFNumberElement& rObject,
                        PDFNumberElement const& rGeneration);
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    /// Assuming the reference points to a number object, return its value.
    SAL_DLLPRIVATE double LookupNumber(SvStream& rStream) const;
    /// Lookup referenced object, without assuming anything about its contents.
    PDFObjectElement* LookupObject();
    int GetObjectValue() const;
    int GetGenerationValue() const;
    SAL_DLLPRIVATE sal_uInt64 GetOffset() const;
    SAL_DLLPRIVATE PDFNumberElement& GetObjectElement() const;

    void writeString(OStringBuffer& rBuffer) override
    {
        rBuffer.append(sal_Int32(GetObjectValue()));
        rBuffer.append(' ');
        rBuffer.append(sal_Int32(GetGenerationValue()));
        rBuffer.append(" R");
    }
};

/// Stream object: a byte array with a known length.
class VCL_DLLPUBLIC PDFStreamElement final : public PDFElement
{
    size_t m_nLength;
    sal_uInt64 m_nOffset;
    /// The byte array itself.
    SvMemoryStream m_aMemory;

public:
    SAL_DLLPRIVATE explicit PDFStreamElement(size_t nLength);
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    SAL_DLLPRIVATE sal_uInt64 GetOffset() const;
    SvMemoryStream& GetMemory();

    void writeString(OStringBuffer& rBuffer) override
    {
        rBuffer.append("stream\n");
        rBuffer.append(static_cast<const char*>(m_aMemory.GetData()), m_aMemory.GetSize());
        rBuffer.append("\nendstream\n");
    }
};

/// Name object: a key string.
class VCL_DLLPUBLIC PDFNameElement final : public PDFElement
{
    OString m_aValue;
    /// Offset after the '/' token.
    sal_uInt64 m_nLocation = 0;

public:
    PDFNameElement();
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    void SetValue(const OString& rValue) { m_aValue = rValue; }
    const OString& GetValue() const;
    SAL_DLLPRIVATE sal_uInt64 GetLocation() const;
    sal_uInt64 GetLength() const { return m_aValue.getLength(); }

    void writeString(OStringBuffer& rBuffer) override
    {
        rBuffer.append("/");
        rBuffer.append(m_aValue);
    }
};

/// Dictionary object: a set key-value pairs.
class VCL_DLLPUBLIC PDFDictionaryElement final : public PDFElement
{
    /// Key-value pairs when the dictionary is a nested value.
    std::map<OString, PDFElement*> m_aItems;
    /// Offset after the '<<' token.
    sal_uInt64 m_nLocation = 0;
    /// Position after the '/' token.
    std::map<OString, sal_uInt64> m_aDictionaryKeyOffset;
    /// Length of the dictionary key and value, till (before) the next token.
    std::map<OString, sal_uInt64> m_aDictionaryKeyValueLength;

public:
    PDFDictionaryElement();
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;

    SAL_DLLPRIVATE static PDFElement* Lookup(const std::map<OString, PDFElement*>& rDictionary,
                                             const OString& rKey);
    SAL_DLLPRIVATE void SetKeyOffset(const OString& rKey, sal_uInt64 nOffset);
    sal_uInt64 GetKeyOffset(const OString& rKey) const;
    SAL_DLLPRIVATE void SetKeyValueLength(const OString& rKey, sal_uInt64 nLength);
    sal_uInt64 GetKeyValueLength(const OString& rKey) const;
    const std::map<OString, PDFElement*>& GetItems() const;
    /// Looks up an object which is only referenced in this dictionary.
    PDFObjectElement* LookupObject(const OString& rDictionaryKey);
    /// Looks up an element which is contained in this dictionary.
    PDFElement* LookupElement(const OString& rDictionaryKey);
    sal_uInt64 GetLocation() const { return m_nLocation; }
    void insert(OString const& rKey, PDFElement* pPDFElement)
    {
        m_aItems.emplace(rKey, pPDFElement);
    }

    void writeString(OStringBuffer& rBuffer) override
    {
        rBuffer.append("<< ");
        for (auto& rPair : m_aItems)
        {
            rBuffer.append("/");
            rBuffer.append(rPair.first);
            rBuffer.append(" ");
            rPair.second->writeString(rBuffer);
            rBuffer.append(" ");
        }
        rBuffer.append(">>");
    }
};

enum class TokenizeMode
{
    /// Full file.
    END_OF_STREAM,
    /// Till the first %%EOF token.
    EOF_TOKEN,
    /// Till the end of the current object.
    END_OF_OBJECT,
    /// Same as END_OF_OBJECT, but for object streams (no endobj keyword).
    STORED_OBJECT
};

/// The type column of an entry in a cross-reference stream.
enum class XRefEntryType
{
    /// xref "f" or xref stream "0".
    FREE,
    /// xref "n" or xref stream "1".
    NOT_COMPRESSED,
    /// xref stream "2".
    COMPRESSED
};

/// An entry in a cross-reference stream.
class XRefEntry
{
    XRefEntryType m_eType = XRefEntryType::NOT_COMPRESSED;
    /**
     * Non-compressed: The byte offset of the object, starting from the
     * beginning of the file.
     * Compressed: The object number of the object stream in which this object is
     * stored.
     */
    sal_uInt64 m_nOffset = 0;
    /// Are changed as part of an incremental update?.
    bool m_bDirty = false;

public:
    XRefEntry();

    void SetType(XRefEntryType eType) { m_eType = eType; }

    XRefEntryType GetType() const { return m_eType; }

    void SetOffset(sal_uInt64 nOffset) { m_nOffset = nOffset; }

    sal_uInt64 GetOffset() const { return m_nOffset; }

    void SetDirty(bool bDirty) { m_bDirty = bDirty; }

    bool GetDirty() const { return m_bDirty; }
};

/// Hex string: in <AABB> form.
class VCL_DLLPUBLIC PDFHexStringElement final : public PDFElement
{
    OString m_aValue;

public:
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    SAL_DLLPRIVATE const OString& GetValue() const;

    void writeString(OStringBuffer& rBuffer) override
    {
        rBuffer.append("<");
        rBuffer.append(m_aValue);
        rBuffer.append(">");
    }
};

/// Literal string: in (asdf) form.
class VCL_DLLPUBLIC PDFLiteralStringElement final : public PDFElement
{
    OString m_aValue;

public:
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    const OString& GetValue() const;

    void writeString(OStringBuffer& rBuffer) override
    {
        rBuffer.append("(");
        rBuffer.append(m_aValue);
        rBuffer.append(")");
    }
};

/// Numbering object: an integer or a real.
class VCL_DLLPUBLIC PDFNumberElement final : public PDFElement
{
    /// Input file start location.
    sal_uInt64 m_nOffset = 0;
    /// Input file token length.
    sal_uInt64 m_nLength = 0;
    double m_fValue = 0;

public:
    PDFNumberElement();
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    double GetValue() const;
    void SetValue(double fValue) { m_fValue = fValue; }

    SAL_DLLPRIVATE sal_uInt64 GetLocation() const;
    SAL_DLLPRIVATE sal_uInt64 GetLength() const;

    void writeString(OStringBuffer& rBuffer) override { rBuffer.append(m_fValue); }
};

/// A one-liner comment.
class UNLESS_MERGELIBS(VCL_DLLPUBLIC) PDFCommentElement final : public PDFElement
{
    PDFDocument& m_rDoc;
    OString m_aComment;

public:
    SAL_DLLPRIVATE explicit PDFCommentElement(PDFDocument& rDoc);
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    void writeString(OStringBuffer& /*rBuffer*/) override {}
};

/// End of a dictionary: '>>'.
class VCL_DLLPUBLIC PDFEndDictionaryElement final : public PDFElement
{
    /// Offset before the '>>' token.
    sal_uInt64 m_nLocation = 0;

public:
    PDFEndDictionaryElement();
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    SAL_DLLPRIVATE sal_uInt64 GetLocation() const;

    void writeString(OStringBuffer& /*rBuffer*/) override {}
};

/// End of a stream: 'endstream' keyword.
class UNLESS_MERGELIBS(VCL_DLLPUBLIC) PDFEndStreamElement final : public PDFElement
{
public:
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;

    void writeString(OStringBuffer& /*rBuffer*/) override {}
};

/// End of an object: 'endobj' keyword.
class VCL_DLLPUBLIC PDFEndObjectElement final : public PDFElement
{
public:
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;

    void writeString(OStringBuffer& /*rBuffer*/) override {}
};

/// End of an array: ']'.
class VCL_DLLPUBLIC PDFEndArrayElement final : public PDFElement
{
    /// Location before the ']' token.
    sal_uInt64 m_nOffset = 0;

public:
    PDFEndArrayElement();
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;
    SAL_DLLPRIVATE sal_uInt64 GetOffset() const;

    void writeString(OStringBuffer& /*rBuffer*/) override {}
};

/// Boolean object: a 'true' or a 'false'.
class UNLESS_MERGELIBS(VCL_DLLPUBLIC) PDFBooleanElement final : public PDFElement
{
    bool m_aValue;

public:
    explicit PDFBooleanElement(bool bValue)
        : m_aValue(bValue)
    {
    }

    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;

    void writeString(OStringBuffer& rBuffer) override
    {
        rBuffer.append(m_aValue ? "true" : "false");
    }
};

/// Null object: the 'null' singleton.
class UNLESS_MERGELIBS(VCL_DLLPUBLIC) PDFNullElement final : public PDFElement
{
public:
    SAL_DLLPRIVATE bool Read(SvStream& rStream) override;

    void writeString(OStringBuffer& rBuffer) override { rBuffer.append("null"); }
};

/**
 * In-memory representation of an on-disk PDF document.
 *
 * The PDF element list is not meant to be saved back to disk, but some
 * elements remember their source offset / length, and based on that it's
 * possible to modify the input file.
 */
class VCL_DLLPUBLIC PDFDocument final : public PDFObjectContainer
{
    /// This vector owns all elements.
    std::vector<std::unique_ptr<PDFElement>> m_aElements;
    /// Object ID <-> object offset map.
    std::map<size_t, XRefEntry> m_aXRef;
    /// Object offset <-> Object pointer map.
    std::map<size_t, PDFObjectElement*> m_aOffsetObjects;
    /// Object ID <-> Object pointer map.
    std::map<size_t, PDFObjectElement*> m_aIDObjects;
    /// List of xref offsets we know.
    std::vector<size_t> m_aStartXRefs;
    /// Offsets of trailers, from latest to oldest.
    std::vector<size_t> m_aTrailerOffsets;
    /// Trailer offset <-> Trailer pointer map.
    std::map<size_t, PDFTrailerElement*> m_aOffsetTrailers;
    /// List of EOF offsets we know.
    std::vector<size_t> m_aEOFs;
    PDFTrailerElement* m_pTrailer = nullptr;
    /// When m_pTrailer is nullptr, this can still have a dictionary.
    PDFObjectElement* m_pXRefStream = nullptr;
    /// All editing takes place in this buffer, if it happens.
    SvMemoryStream m_aEditBuffer;

    /// Signature line in PDF format, to be consumed by the next Sign() invocation.
    std::vector<sal_Int8> m_aSignatureLine;

    /// 0-based page number where m_aSignatureLine should be placed.
    size_t m_nSignaturePage = 0;

    /// Suggest a minimal, yet free signature ID to use for the next signature.
    SAL_DLLPRIVATE sal_uInt32 GetNextSignature();
    /// Write the signature object as part of signing.
    SAL_DLLPRIVATE sal_Int32 WriteSignatureObject(svl::crypto::SigningContext& rSigningContext,
                                                  const OUString& rDescription, bool bAdES,
                                                  sal_uInt64& rLastByteRangeOffset,
                                                  sal_Int64& rContentOffset);
    /// Write the appearance object as part of signing.
    SAL_DLLPRIVATE sal_Int32 WriteAppearanceObject(tools::Rectangle& rSignatureRectangle);
    /// Write the annot object as part of signing.
    SAL_DLLPRIVATE sal_Int32 WriteAnnotObject(PDFObjectElement const& rFirstPage,
                                              sal_Int32 nSignatureId, sal_Int32 nAppearanceId,
                                              const tools::Rectangle& rSignatureRectangle);
    /// Write the updated Page object as part of signing.
    SAL_DLLPRIVATE bool WritePageObject(PDFObjectElement& rFirstPage, sal_Int32 nAnnotId);
    /// Write the updated Catalog object as part of signing.
    SAL_DLLPRIVATE bool WriteCatalogObject(sal_Int32 nAnnotId, PDFReferenceElement*& pRoot);
    /// Write the updated cross-references as part of signing.
    SAL_DLLPRIVATE void WriteXRef(sal_uInt64 nXRefOffset, PDFReferenceElement const* pRoot);

public:
    PDFDocument();
    virtual ~PDFDocument();
    PDFDocument& operator=(const PDFDocument&) = delete;
    PDFDocument(const PDFDocument&) = delete;
    /// @name Low-level functions, to be used by PDFElement subclasses.
    //@{
    /// Decode a hex dump.
    SAL_DLLPRIVATE static std::vector<unsigned char>
    DecodeHexString(PDFHexStringElement const* pElement);
    static OUString DecodeHexStringUTF16BE(PDFHexStringElement const& rElement);
    SAL_DLLPRIVATE static OString ReadKeyword(SvStream& rStream);
    SAL_DLLPRIVATE static size_t FindStartXRef(SvStream& rStream);
    SAL_DLLPRIVATE void ReadXRef(SvStream& rStream);
    SAL_DLLPRIVATE void ReadXRefStream(SvStream& rStream);
    SAL_DLLPRIVATE static void SkipWhitespace(SvStream& rStream);
    /// Instead of all whitespace, just skip CR and NL characters.
    SAL_DLLPRIVATE static void SkipLineBreaks(SvStream& rStream);
    SAL_DLLPRIVATE size_t GetObjectOffset(size_t nIndex) const;
    const std::vector<std::unique_ptr<PDFElement>>& GetElements() const;
    std::vector<PDFObjectElement*> GetPages();
    PDFObjectElement* GetCatalog();
    /// Remember the end location of an EOF token.
    SAL_DLLPRIVATE void PushBackEOF(size_t nOffset);
    /// Look up object based on object number, possibly by parsing object streams.
    PDFObjectElement* LookupObject(size_t nObjectNumber);
    /// Access to the input document, even after the input stream is gone.
    SAL_DLLPRIVATE SvMemoryStream& GetEditBuffer();
    /// Tokenize elements from current offset.
    SAL_DLLPRIVATE bool Tokenize(SvStream& rStream, TokenizeMode eMode,
                                 std::vector<std::unique_ptr<PDFElement>>& rElements,
                                 PDFObjectElement* pObjectElement);
    /// Register an object (owned directly or indirectly by m_aElements) as a provider for a given ID.
    SAL_DLLPRIVATE void SetIDObject(size_t nID, PDFObjectElement* pObject);
    //@}

    /// @name High-level functions, to be used by others.
    //@{
    /// Read elements from the start of the stream till its end.
    bool Read(SvStream& rStream);
    /// Calls Read() first and if it fails it tries to fixup and then retry.
    SAL_DLLPRIVATE bool ReadWithPossibleFixup(SvStream& rStream);
    void SetSignatureLine(std::vector<sal_Int8>&& rSignatureLine);
    void SetSignaturePage(size_t nPage);
    /// Sign the read document with xCertificate in the edit buffer.
    bool Sign(svl::crypto::SigningContext& rSigningContext, const OUString& rDescription,
              bool bAdES);
    /// Serializes the contents of the edit buffer.
    bool Write(SvStream& rStream);
    /// Get a list of signatures embedded into this document.
    std::vector<PDFObjectElement*> GetSignatureWidgets();
    /// Remove the nth signature from read document in the edit buffer.
    bool RemoveSignature(size_t nPosition);
    //@}

    /// See vcl::PDFObjectContainer::createObject().
    SAL_DLLPRIVATE sal_Int32 createObject() override;
    /// See vcl::PDFObjectContainer::updateObject().
    SAL_DLLPRIVATE bool updateObject(sal_Int32 n) override;
    /// See vcl::PDFObjectContainer::writeBuffer().
    SAL_DLLPRIVATE bool writeBufferBytes(const void* pBuffer, sal_uInt64 nBytes) override;
    void checkAndEnableStreamEncryption(sal_Int32 /*nObject*/) override {}
    void disableStreamEncryption() override {}
};

/// The trailer singleton is at the end of the doc.
class VCL_DLLPUBLIC PDFTrailerElement final : public PDFElement
{
    PDFDocument& m_rDoc;
    PDFDictionaryElement* m_pDictionaryElement;
    /// Location of the end of the trailer token.
    sal_uInt64 m_nOffset = 0;

public:
    explicit PDFTrailerElement(PDFDocument& rDoc);
    bool Read(SvStream& rStream) override;
    PDFElement* Lookup(const OString& rDictionaryKey);
    sal_uInt64 GetLocation() const;

    void SetDictionary(PDFDictionaryElement* pDictionaryElement)
    {
        m_pDictionaryElement = pDictionaryElement;
    }

    PDFDictionaryElement* GetDictionary() { return m_pDictionaryElement; }

    void writeString(OStringBuffer& /*rBuffer*/) override { assert(false && "not implemented"); }
};

class VCL_DLLPUBLIC PDFObjectParser final
{
    const std::vector<std::unique_ptr<PDFElement>>& mrElements;

public:
    PDFObjectParser(std::vector<std::unique_ptr<PDFElement>> const& rElements)
        : mrElements(rElements)
    {
    }

    size_t parse(PDFElement* pParsingElement, size_t nStartIndex = 0, int nCurrentDepth = 0);
};

} // namespace vcl::filter

#endif // INCLUDED_VCL_FILTER_PDFDOCUMENT_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
