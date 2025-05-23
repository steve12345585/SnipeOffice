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

#include <string.h>

#include "scdllapi.h"
#include "global.hxx"
#include "refdata.hxx"
#include "token.hxx"
#include <formula/token.hxx>
#include <formula/grammar.hxx>
#include <rtl/ustrbuf.hxx>
#include <com/sun/star/sheet/ExternalLinkInfo.hpp>
#include <com/sun/star/i18n/ParseResult.hpp>
#include <queue>
#include <vector>
#include <memory>
#include <unordered_set>
#include <set>
#include <com/sun/star/uno/Sequence.hxx>
#include <o3tl/typed_flags_set.hxx>

#include <formula/FormulaCompiler.hxx>

struct ScSheetLimits;

// constants and data types also for external modules (ScInterpreter et al)

#define MAXSTRLEN    1024   /* maximum length of input string of one symbol */

// flag values of CharTable
enum class ScCharFlags : sal_uInt32 {
    NONE            = 0x00000000,
    Illegal         = 0x00000000,
    Char            = 0x00000001,
    CharBool        = 0x00000002,
    CharWord        = 0x00000004,
    CharValue       = 0x00000008,
    CharString      = 0x00000010,
    CharDontCare    = 0x00000020,
    Bool            = 0x00000040,
    Word            = 0x00000080,
    WordSep         = 0x00000100,
    Value           = 0x00000200,
    ValueSep        = 0x00000400,
    ValueExp        = 0x00000800,
    ValueSign       = 0x00001000,
    ValueValue      = 0x00002000,
    StringSep       = 0x00004000,
    NameSep         = 0x00008000,  // there can be only one! '\''
    CharIdent       = 0x00010000,  // identifier (built-in function) or reference start
    Ident           = 0x00020000,  // identifier or reference continuation
    OdfLBracket     = 0x00040000,  // ODF '[' reference bracket
    OdfRBracket     = 0x00080000,  // ODF ']' reference bracket
    OdfLabelOp      = 0x00100000,  // ODF '!!' automatic intersection of labels
    OdfNameMarker   = 0x00200000,  // ODF '$$' marker that starts a defined (range) name
    CharName        = 0x00400000,  // start character of a defined name
    Name            = 0x00800000,  // continuation character of a defined name
    CharErrConst    = 0x01000000,  // start character of an error constant ('#')
};
namespace o3tl {
    template<> struct typed_flags<ScCharFlags> : is_typed_flags<ScCharFlags, 0x01ffffff> {};
}

#define SC_COMPILER_FILE_TAB_SEP      '#'         // 'Doc'#Tab

class ScDocument;
class ScMatrix;
class ScRangeData;
class ScTokenArray;
struct ScInterpreterContext;
class CharClass;

namespace sc {

class CompileFormulaContext;

}

// constants and data types internal to compiler

struct ScRawToken final
{
    friend class ScCompiler;
    // Friends that use a temporary ScRawToken on the stack (and therefore need
    // the private dtor) and know what they're doing...
    friend class ScTokenArray;
    OpCode              eOp;
    formula::StackVar   eType;  // type of data; this determines how the unions are used
public:
    union {
        double       nValue;
        struct {
            sal_uInt8           nCount;
            sal_Unicode         cChar;
        } whitespace;
        struct {
            sal_uInt8           cByte;
            formula::ParamClass eInForceArray;
        } sbyte;
        ScComplexRefData aRef;
        struct {
            sal_uInt16          nFileId;
            ScComplexRefData    aRef;
        } extref;
        struct {
            sal_uInt16  nFileId;
        } extname;
        struct {
            sal_Int16   nSheet;
            sal_uInt16  nIndex;
        } name;
        struct {
            sal_uInt16              nIndex;
            ScTableRefToken::Item   eItem;
        } table;
        struct {
            rtl_uString* mpData;
            rtl_uString* mpDataIgnoreCase;
        } sharedstring;
        ScMatrix*    pMat;
        FormulaError nError;
        short        nJump[ FORMULA_MAXPARAMS + 1 ];     // If/Choose/Let token
    };
    OUString   maExternalName; // depending on the opcode, this is either the external, or the external name, or the external table name

    // coverity[uninit_member] - members deliberately not initialized
    ScRawToken() {}
private:
                ~ScRawToken() {}                //! only delete via Delete()
public:
    formula::StackVar    GetType()   const       { return eType; }
    OpCode      GetOpCode() const       { return eOp; }
    void        NewOpCode( OpCode e )   { eOp = e; }

    // Use these methods only on tokens that are not part of a token array,
    // since the reference count is cleared!
    void SetOpCode( OpCode eCode );
    void SetString( rtl_uString* pData, rtl_uString* pDataIgnoreCase );
    void SetStringName( rtl_uString* pData, rtl_uString* pDataIgnoreCase );
    void SetSingleReference( const ScSingleRefData& rRef );
    void SetDoubleReference( const ScComplexRefData& rRef );
    void SetDouble( double fVal );
    void SetErrorConstant( FormulaError nErr );

    // These methods are ok to use, reference count not cleared.
    void SetName(sal_Int16 nSheet, sal_uInt16 nIndex);
    void SetExternalSingleRef( sal_uInt16 nFileId, const OUString& rTabName, const ScSingleRefData& rRef );
    void SetExternalDoubleRef( sal_uInt16 nFileId, const OUString& rTabName, const ScComplexRefData& rRef );
    void SetExternalName( sal_uInt16 nFileId, const OUString& rName );
    void SetExternal(const OUString& rStr);

    /** If the token is a non-external reference, determine if the reference is
        valid. If the token is an external reference, return true. Else return
        false. Used only in ScCompiler::NextNewToken() to preserve non-existing
        sheet names in otherwise valid references.
     */
    bool IsValidReference(const ScDocument& rDoc) const;

    formula::FormulaToken* CreateToken(ScSheetLimits& rLimits) const;   // create typified token
};

class SAL_DLLPUBLIC_RTTI ScCompiler final : public formula::FormulaCompiler
{
public:

    enum ExtendedErrorDetection
    {
        EXTENDED_ERROR_DETECTION_NONE = 0,      // no error on unknown symbols, default (interpreter handles it)
        EXTENDED_ERROR_DETECTION_NAME_BREAK,    // name error on unknown symbols and break, pCode incomplete
        EXTENDED_ERROR_DETECTION_NAME_NO_BREAK  // name error on unknown symbols, don't break, continue
    };

    struct Convention
    {
        const formula::FormulaGrammar::AddressConvention meConv;

        Convention( formula::FormulaGrammar::AddressConvention eConvP );
        virtual ~Convention();

        virtual void makeRefStr(
            ScSheetLimits& rLimits,
            OUStringBuffer& rBuffer,
            formula::FormulaGrammar::Grammar eGram,
            const ScAddress& rPos,
            const OUString& rErrRef, const std::vector<OUString>& rTabNames,
            const ScComplexRefData& rRef, bool bSingleRef, bool bFromRangeName ) const = 0;

        virtual css::i18n::ParseResult
                    parseAnyToken( const OUString& rFormula,
                                   sal_Int32 nSrcPos,
                                   const CharClass* pCharClass,
                                   bool bGroupSeparator) const = 0;

        /**
         * Parse the symbol string and pick up the file name and the external
         * range name.
         *
         * @return true on successful parse, or false otherwise.
         */
        virtual bool parseExternalName( const OUString& rSymbol, OUString& rFile, OUString& rName,
                const ScDocument& rDoc,
                const css::uno::Sequence< css::sheet::ExternalLinkInfo>* pExternalLinks ) const = 0;

        virtual OUString makeExternalNameStr( sal_uInt16 nFileId, const OUString& rFile,
                const OUString& rName ) const = 0;

        virtual void makeExternalRefStr(
            ScSheetLimits& rLimits,
            OUStringBuffer& rBuffer, const ScAddress& rPos, sal_uInt16 nFileId, const OUString& rFileName,
            const OUString& rTabName, const ScSingleRefData& rRef ) const = 0;

        virtual void makeExternalRefStr(
            ScSheetLimits& rLimits,
            OUStringBuffer& rBuffer, const ScAddress& rPos,
            sal_uInt16 nFileId, const OUString& rFileName, const std::vector<OUString>& rTabNames,
            const OUString& rTabName, const ScComplexRefData& rRef ) const = 0;

        enum SpecialSymbolType
        {
            /**
             * Character between sheet name and address.  In OOO A1 this is
             * '.', while XL A1 and XL R1C1 this is '!'.
             */
            SHEET_SEPARATOR,

            /**
             * In OOO A1, a sheet name may be prefixed with '$' to indicate an
             * absolute sheet position.
             */
            ABS_SHEET_PREFIX
        };
        virtual sal_Unicode getSpecialSymbol( SpecialSymbolType eSymType ) const = 0;

        virtual ScCharFlags getCharTableFlags( sal_Unicode c, sal_Unicode cLast ) const = 0;

    protected:
        const std::array<ScCharFlags, 128>& mrCharTable;
    };
    friend struct Convention;

private:

    static const CharClass      *pCharClassEnglish;     // character classification for en_US locale
    static const CharClass      *pCharClassLocalized;   // character classification for UI locale
    static const Convention     *pConventions[ formula::FormulaGrammar::CONV_LAST ];

    static const struct AddInMap
    {
        const char* pODFF;
        const char* pEnglish;
        const char* pOriginal;              // programmatical name
        const char* pUpper;                 // upper case programmatical name
    } g_aAddInMap[];
    static size_t GetAddInMapCount();

    ScDocument& rDoc;
    ScAddress   aPos;

    ScInterpreterContext& mrInterpreterContext;

    SCTAB       mnCurrentSheetTab;      // indicates current sheet number parsed so far
    sal_Int32   mnCurrentSheetEndPos;   // position after current sheet name if parsed

    // For CONV_XL_OOX, may be set via API by MOOXML filter.
    css::uno::Sequence<css::sheet::ExternalLinkInfo> maExternalLinks;

    sal_Unicode cSymbol[MAXSTRLEN+1];               // current Symbol + 0
    OUString    aFormula;                           // formula source code
    sal_Int32   nSrcPos;                            // tokenizer position (source code)
    ScRawToken maRawToken;

    std::queue<OpCode> maPendingOpCodes; // additional opcodes generated from a single symbol

    const CharClass* pCharClass; // which character classification is used for parseAnyToken and upper/lower
    bool        mbCharClassesDiffer;    // whether pCharClass and current system locale's CharClass differ
    sal_uInt16      mnPredetectedReference;     // reference when reading ODF, 0 (none), 1 (single) or 2 (double)
    sal_Int32   mnRangeOpPosInSymbol;       // if and where a range operator is in symbol
    const Convention *pConv;
    ExtendedErrorDetection  meExtendedErrorDetection;
    bool        mbCloseBrackets;            // whether to close open brackets automatically, default TRUE
    bool        mbRewind;                   // whether symbol is to be rewound to some step during lexical analysis
    bool        mbRefConventionChartOOXML;  // whether to use special ooxml chart syntax in case of OOXML reference convention,
                                            // when parsing a formula string. [0]!GlobalNamedRange, LocalSheet!LocalNamedRange
    std::vector<sal_uInt16> maExternalFiles;

    std::vector<OUString> maTabNames;                /// sheet names mangled for the current grammar for output
    std::vector<OUString> &GetSetupTabNames() const; /// get or setup tab names for the current grammar

    struct TableRefEntry
    {
        boost::intrusive_ptr<ScTableRefToken> mxToken;
        sal_uInt16  mnLevel;
        TableRefEntry( ScTableRefToken* p ) : mxToken(p), mnLevel(0) {}
    };
    std::vector<TableRefEntry> maTableRefs;     /// "stack" of currently active ocTableRef tokens

    // Optimizing implicit intersection is done only at the end of code generation, because the usage context may
    // be important. Store candidate parameters and the operation they are the argument for.
    struct PendingImplicitIntersectionOptimization
    {
        PendingImplicitIntersectionOptimization(formula::FormulaToken** p, formula::FormulaToken* o)
            : parameterLocation( p ), parameter( *p ), operation( o ) {}
        formula::FormulaToken** parameterLocation;
        formula::FormulaTokenRef parameter;
        formula::FormulaTokenRef operation;
    };
    std::vector< PendingImplicitIntersectionOptimization > mPendingImplicitIntersectionOptimizations;
    std::unordered_set<formula::FormulaTokenRef> mUnhandledPossibleImplicitIntersections;
#ifdef DBG_UTIL
    std::set<OpCode> mUnhandledPossibleImplicitIntersectionsOpCodes;
#endif

    bool   NextNewToken(bool bInArray);
    bool ToUpperAsciiOrI18nIsAscii( OUString& rUpper, const OUString& rOrg ) const;
    short  GetPossibleParaCount( std::u16string_view rLambdaFormula ) const;

    virtual void SetError(FormulaError nError) override;

    struct Whitespace final
    {
        sal_Int32   nCount;
        sal_Unicode cChar;

        Whitespace() : nCount(0), cChar(0x20) {}
        void reset( sal_Unicode c ) { nCount = 0; cChar = c; }
    };

    static void addWhitespace( std::vector<ScCompiler::Whitespace> & rvSpaces,
            ScCompiler::Whitespace & rSpace, sal_Unicode c, sal_Int32 n = 1 );

    std::vector<Whitespace> NextSymbol(bool bInArray);

    bool ParseValue( const OUString& );
    bool ParseOpCode( const OUString&, bool bInArray );
    bool ParseOpCode2( std::u16string_view );
    bool ParseString();
    bool ParseReference( const OUString& rSymbol, const OUString* pErrRef = nullptr );
    bool ParseSingleReference( const OUString& rSymbol, const OUString* pErrRef = nullptr );
    bool ParseDoubleReference( const OUString& rSymbol, const OUString* pErrRef = nullptr );
    bool ParsePredetectedReference( const OUString& rSymbol );
    bool ParsePredetectedErrRefReference( const OUString& rName, const OUString* pErrRef );
    bool ParseMacro( const OUString& );
    bool ParseNamedRange( const OUString&, bool onlyCheck = false );
    bool ParseLambdaFuncName( const OUString& );
    bool ParseExternalNamedRange( const OUString& rSymbol, bool& rbInvalidExternalNameRange );
    bool ParseDBRange( const OUString& );
    bool ParseColRowName( const OUString& );
    bool ParseBoolean( const OUString& );
    void AutoCorrectParsedSymbol();
    const ScRangeData* GetRangeData( SCTAB& rSheet, const OUString& rUpperName ) const;

    void AdjustSheetLocalNameRelReferences( SCTAB nDelta );
    void SetRelNameReference();

    /** Obtain range data for ocName token, global or sheet local.

        Prerequisite: rToken is a FormulaIndexToken so IsGlobal() and
        GetIndex() can be called on it. We don't check with RTTI.
     */
    ScRangeData* GetRangeData( const formula::FormulaToken& pToken ) const;

    bool HasPossibleNamedRangeConflict(SCTAB nTab) const;

public:
    static const CharClass* GetCharClassLocalized();
    static const CharClass* GetCharClassEnglish();

public:
    ScCompiler( sc::CompileFormulaContext& rCxt, const ScAddress& rPos,
            bool bComputeII = false, bool bMatrixFlag = false, ScInterpreterContext* pContext = nullptr );

    /** If eGrammar == GRAM_UNSPECIFIED then the grammar of rDocument is used,
     */
    SC_DLLPUBLIC ScCompiler( ScDocument& rDocument, const ScAddress&,
            formula::FormulaGrammar::Grammar eGrammar = formula::FormulaGrammar::GRAM_UNSPECIFIED,
            bool bComputeII = false, bool bMatrixFlag = false, ScInterpreterContext* pContext = nullptr );

    SC_DLLPUBLIC ScCompiler( sc::CompileFormulaContext& rCxt, const ScAddress& rPos, ScTokenArray& rArr,
            bool bComputeII = false, bool bMatrixFlag = false, ScInterpreterContext* pContext = nullptr );

    /** If eGrammar == GRAM_UNSPECIFIED then the grammar of rDocument is used,
     */
    SC_DLLPUBLIC ScCompiler( ScDocument& rDocument, const ScAddress&, ScTokenArray& rArr,
            formula::FormulaGrammar::Grammar eGrammar = formula::FormulaGrammar::GRAM_UNSPECIFIED,
            bool bComputeII = false, bool bMatrixFlag = false, ScInterpreterContext* pContext = nullptr );

    SC_DLLPUBLIC virtual ~ScCompiler() override;

public:
    static void DeInit();               /// all

    // for ScAddress::Format()
    SC_DLLPUBLIC static void CheckTabQuotes( OUString& aTabName,
                                const formula::FormulaGrammar::AddressConvention eConv = formula::FormulaGrammar::CONV_OOO );

    /** Concatenates two sheet names in Excel syntax, i.e. 'Sheet1:Sheet2'
        instead of 'Sheet1':'Sheet2' or 'Sheet1':Sheet2 or Sheet1:'Sheet2'.

        @param  rBuf
                Contains the first sheet name already, in the correct quoted 'Sheet''1' or
                unquoted Sheet1 form if plain name.

        @param  nQuotePos
                Start position, of the first sheet name if unquoted, or its
                opening quote.

        @param  rEndTabName
                Second sheet name to append, in the correct quoted 'Sheet''2'
                or unquoted Sheet2 form if plain name.
     */
    static void FormExcelSheetRange( OUStringBuffer& rBuf, sal_Int32 nQuotePos, const OUString& rEndTabName );

    /** Analyzes a string for a 'Doc'#Tab construct, or 'Do''c'#Tab etc...

        @returns the position of the unquoted # hash mark in 'Doc'#Tab, or
                 -1 if none. */
    static sal_Int32 GetDocTabPos( const OUString& rString );

    // Check if it is a valid english function name
    SC_DLLPUBLIC static bool IsEnglishSymbol( const OUString& rName );

    bool ParseErrorConstant( const OUString& );
    bool ParseTableRefItem( const OUString& );
    bool ParseTableRefColumn( const OUString& );

    /** Calls GetToken() if PeekNextNoSpaces() is of given OpCode. */
    bool GetTokenIfOpCode( OpCode eOp );

    /**
     * When auto correction is set, the jump command reorder must be enabled.
     */
    void SetAutoCorrection( bool bVal );
    void            SetCloseBrackets( bool bVal ) { mbCloseBrackets = bVal; }
    void            SetRefConventionChartOOXML( bool bVal ) { mbRefConventionChartOOXML = bVal; }
    void            SetRefConvention( const Convention *pConvP );
    void            SetRefConvention( const formula::FormulaGrammar::AddressConvention eConv );

    static const Convention* GetRefConvention( formula::FormulaGrammar::AddressConvention eConv );

    /** Overwrite FormulaCompiler::GetOpCodeMap() forwarding to
        GetFinalOpCodeMap().
     */
    OpCodeMapPtr    GetOpCodeMap( const sal_Int32 nLanguage ) const { return GetFinalOpCodeMap(nLanguage); }

    /// Set symbol map if not empty.
    void            SetFormulaLanguage( const OpCodeMapPtr & xMap );

    SC_DLLPUBLIC void SetGrammar( const formula::FormulaGrammar::Grammar eGrammar );

private:
    /** Set grammar and reference convention from within SetFormulaLanguage()
        or SetGrammar().

        @param eNewGrammar
            The new grammar to be set and the associated reference convention.

        @param eOldGrammar
            The previous grammar that was active before SetFormulaLanguage().
     */
    void            SetGrammarAndRefConvention(
                        const formula::FormulaGrammar::Grammar eNewGrammar,
                        const formula::FormulaGrammar::Grammar eOldGrammar );
public:

    /// Set external link info for ScAddress::CONV_XL_OOX.
    void SetExternalLinks(
        const css::uno::Sequence<
            css::sheet::ExternalLinkInfo>& rLinks )
    {
        maExternalLinks = rLinks;
    }

    void            CreateStringFromXMLTokenArray( OUString& rFormula, OUString& rFormulaNmsp );

    void            SetExtendedErrorDetection( ExtendedErrorDetection eVal ) { meExtendedErrorDetection = eVal; }

    bool            IsCorrected() const { return bCorrected; }
    const OUString& GetCorrectedFormula() const { return aCorrectedFormula; }

    /**
     * Tokenize formula expression string into an array of tokens.
     *
     * @param rFormula formula expression to tokenize.
     *
     * @return heap allocated token array object. The caller <i>must</i>
     *         manage the life cycle of this object.
     */
    SC_DLLPUBLIC std::unique_ptr<ScTokenArray> CompileString( const OUString& rFormula );
    std::unique_ptr<ScTokenArray> CompileString( const OUString& rFormula, const OUString& rFormulaNmsp );
    const ScAddress& GetPos() const { return aPos; }

    void MoveRelWrap();
    SC_DLLPUBLIC static void MoveRelWrap( const ScTokenArray& rArr, const ScDocument& rDoc, const ScAddress& rPos,
                             SCCOL nMaxCol, SCROW nMaxRow );

    /** If the character is allowed as tested by nFlags (SC_COMPILER_C_...
        bits) for all known address conventions. If more than one bit is given
        in nFlags, all bits must match. */
    SC_DLLPUBLIC static bool IsCharFlagAllConventions(
        OUString const & rStr, sal_Int32 nPos, ScCharFlags nFlags );

    /** TODO : Move this to somewhere appropriate. */
    static bool DoubleRefToPosSingleRefScalarCase(const ScRange& rRange, ScAddress& rAdr,
                                                  const ScAddress& rFormulaPos);

    bool HasUnhandledPossibleImplicitIntersections() const { return !mUnhandledPossibleImplicitIntersections.empty(); }
#ifdef DBG_UTIL
    const std::set<OpCode>& UnhandledPossibleImplicitIntersectionsOpCodes() { return mUnhandledPossibleImplicitIntersectionsOpCodes; }
#endif

private:
    // FormulaCompiler
    virtual OUString FindAddInFunction( const OUString& rUpperName, bool bLocalFirst ) const override;
    virtual void fillFromAddInCollectionUpperName( const NonConstOpCodeMapPtr& xMap ) const override;
    virtual void fillFromAddInCollectionEnglishName( const NonConstOpCodeMapPtr& xMap ) const override;
    virtual void fillFromAddInCollectionExcelName( const NonConstOpCodeMapPtr& xMap ) const override;
    virtual void fillFromAddInMap( const NonConstOpCodeMapPtr& xMap, formula::FormulaGrammar::Grammar _eGrammar ) const override;
    virtual void fillAddInToken(::std::vector< css::sheet::FormulaOpCodeMapEntry >& _rVec,bool _bIsEnglish) const override;

    virtual bool HandleExternalReference(const formula::FormulaToken& _aToken) override;
    virtual bool HandleStringName() override;
    virtual bool HandleRange() override;
    virtual bool HandleColRowName() override;
    virtual bool HandleDbData() override;
    virtual bool HandleTableRef() override;

    virtual formula::FormulaTokenRef ExtendRangeReference( formula::FormulaToken & rTok1, formula::FormulaToken & rTok2 ) override;
    virtual void CreateStringFromExternal( OUStringBuffer& rBuffer, const formula::FormulaToken* pToken ) const override;
    virtual void CreateStringFromSingleRef( OUStringBuffer& rBuffer, const formula::FormulaToken* pToken ) const override;
    virtual void CreateStringFromDoubleRef( OUStringBuffer& rBuffer, const formula::FormulaToken* pToken ) const override;
    virtual void CreateStringFromMatrix( OUStringBuffer& rBuffer, const formula::FormulaToken* pToken ) const override;
    virtual void CreateStringFromIndex( OUStringBuffer& rBuffer, const formula::FormulaToken* pToken ) const override;
    virtual void LocalizeString( OUString& rName ) const override;   // modify rName - input: exact name
    virtual bool GetExcelName( OUString& rName ) const override;    // modify rName - input: exact name

    virtual formula::ParamClass GetForceArrayParameter( const formula::FormulaToken* pToken, sal_uInt16 nParam ) const override;

    /// Access the CharTable flags
    ScCharFlags GetCharTableFlags( sal_Unicode c, sal_Unicode cLast )
        { return c < 128 ? pConv->getCharTableFlags(c, cLast) : ScCharFlags::NONE; }

    virtual void HandleIIOpCode(formula::FormulaToken* token, formula::FormulaToken*** pppToken, sal_uInt8 nNumParams) override;
    bool HandleIIOpCodeInternal(formula::FormulaToken* token, formula::FormulaToken*** pppToken, sal_uInt8 nNumParams);
    bool SkipImplicitIntersectionOptimization(const formula::FormulaToken* token) const;
    virtual void PostProcessCode() override;
    virtual void AnnotateOperands() override;
    static bool ParameterMayBeImplicitIntersection(const formula::FormulaToken* token, int parameter);
    void ReplaceDoubleRefII(formula::FormulaToken** ppDoubleRefTok);
    bool AdjustSumRangeShape(const ScComplexRefData& rBaseRange, ScComplexRefData& rSumRange);
    void CorrectSumRange(const ScComplexRefData& rBaseRange, ScComplexRefData& rSumRange, formula::FormulaToken** ppSumRangeToken);
    void AnnotateTrimOnDoubleRefs();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
