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

#include <sal/config.h>

#include <cstddef>
#include <memory>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <rtl/string.hxx>
#include <rtl/ustring.hxx>
#include <tools/solar.h>
#include <vcl/dllapi.h>

#define PRINTER_PPDDIR "driver"

namespace psp {

enum class orientation;

class PPDCache;
class PPDTranslator;

enum class PPDValueType
{
    Invocation,
    Quoted,
    Symbol,
    String,
    No
};

struct VCL_DLLPUBLIC PPDValue
{
    PPDValueType     m_eType;
    //CustomOption stuff for fdo#43049
    //see http://www.cups.org/documentation.php/spec-ppd.html#OPTIONS
    //for full specs, only the basics are implemented here
    bool             m_bCustomOption;
    mutable bool     m_bCustomOptionSetViaApp;
    mutable OUString m_aCustomOption;
    OUString         m_aOption;
    OUString         m_aValue;
};


/*
 * PPDKey - a container for the available options (=values) of a PPD keyword
 */

class PPDKey
{
    friend class PPDParser;
    friend class CPDManager;

    typedef std::unordered_map< OUString, PPDValue > hash_type;

    OUString            m_aKey;
    hash_type           m_aValues;
    std::vector<PPDValue*> m_aOrderedValues;
    const PPDValue*     m_pDefaultValue;
    bool                m_bQueryValue;
    OUString            m_aGroup;

private:

    bool                m_bUIOption;
    int                 m_nOrderDependency;

    void eraseValue( const OUString& rOption );
public:
    PPDKey( OUString aKey );
    ~PPDKey();

    PPDValue*           insertValue(const OUString& rOption, PPDValueType eType, bool bCustomOption = false);
    int                 countValues() const
    { return m_aValues.size(); }
    // neither getValue will return the query option
    const PPDValue*     getValue( int n ) const;
    const PPDValue*     getValue( const OUString& rOption ) const;
    const PPDValue*     getValueCaseInsensitive( const OUString& rOption ) const;
    const PPDValue*     getDefaultValue() const { return m_pDefaultValue; }
    const OUString&     getGroup() const { return m_aGroup; }

    const OUString&     getKey() const { return m_aKey; }
    bool                isUIKey() const { return m_bUIOption; }
    int                 getOrderDependency() const { return m_nOrderDependency; }
};

// define a hash for PPDKey
struct PPDKeyhash
{
    size_t operator()( const PPDKey * pKey) const
        { return reinterpret_cast<size_t>(pKey); }
};

/*
 * PPDParser - parses a PPD file and contains all available keys from it
 */

class PPDParser
{
    friend class CUPSManager;
    friend class CPDManager;
    friend class PPDCache;

    typedef std::unordered_map< OUString, std::unique_ptr<PPDKey> > hash_type;

    void insertKey( std::unique_ptr<PPDKey> pKey );
public:
    struct PPDConstraint
    {
        const PPDKey*       m_pKey1;
        const PPDValue*     m_pOption1;
        const PPDKey*       m_pKey2;
        const PPDValue*     m_pOption2;

        PPDConstraint() : m_pKey1( nullptr ), m_pOption1( nullptr ), m_pKey2( nullptr ), m_pOption2( nullptr ) {}
    };
private:
    hash_type                                   m_aKeys;
    std::vector<PPDKey*>                        m_aOrderedKeys;
    ::std::vector< PPDConstraint >              m_aConstraints;

    // the full path of the PPD file
    OUString                                    m_aFile;
    // some basic attributes
    rtl_TextEncoding                            m_aFileEncoding;


    // shortcuts to important keys and their default values
    // imageable area
    const PPDKey*                               m_pImageableAreas;
    // paper dimensions
    const PPDValue*                             m_pDefaultPaperDimension;
    const PPDKey*                               m_pPaperDimensions;
    // paper trays
    const PPDValue*                             m_pDefaultInputSlot;
    // resolutions
    const PPDValue*                             m_pDefaultResolution;

    // translations
    std::unique_ptr<PPDTranslator>              m_pTranslator;

    PPDParser( OUString aFile );
    PPDParser(OUString aFile, const std::vector<PPDKey*>& keys);

    void parseOrderDependency(const OString& rLine);
    void parseOpenUI(const OString& rLine, std::string_view rPPDGroup);
    void parseConstraint(const OString& rLine);
    void parse( std::vector< OString >& rLines );

    OUString handleTranslation(const OString& i_rString, bool i_bIsGlobalized);

    static void scanPPDDir( const OUString& rDir );
    static void initPPDFiles(PPDCache &rPPDCache);
    static OUString getPPDFile( const OUString& rFile );

    OUString        matchPaperImpl(int nWidth, int nHeight, bool bSwapped, psp::orientation* pOrientation = nullptr) const;

public:
    ~PPDParser();
    static const PPDParser* getParser( const OUString& rFile );

    const PPDKey*   getKey( int n ) const;
    const PPDKey*   getKey( const OUString& rKey ) const;
    int             getKeys() const { return m_aKeys.size(); }
    bool            hasKey( const PPDKey* ) const;

    const ::std::vector< PPDConstraint >& getConstraints() const { return m_aConstraints; }

    const OUString & getDefaultPaperDimension() const;
    void            getDefaultPaperDimension( int& rWidth, int& rHeight ) const
    { getPaperDimension( getDefaultPaperDimension(), rWidth, rHeight ); }
    bool getPaperDimension( std::u16string_view rPaperName,
                            int& rWidth, int& rHeight ) const;
    // width and height in pt
    // returns false if paper not found

    // match the best paper for width and height
    OUString        matchPaper( int nWidth, int nHeight, psp::orientation* pOrientation = nullptr ) const;

    bool getMargins( std::u16string_view rPaperName,
                     int &rLeft, int& rRight,
                     int &rUpper, int& rLower ) const;
    // values in pt
    // returns true if paper found

    // values int pt

    const OUString & getDefaultInputSlot() const;

    void            getDefaultResolution( int& rXRes, int& rYRes ) const;
    // values in dpi
    static void     getResolutionFromString( std::u16string_view, int&, int& );
    // helper function

    OUString   translateKey( const OUString& i_rKey ) const;
    OUString   translateOption( std::u16string_view i_rKey,
                                const OUString& i_rOption ) const;
};


/*
 * PPDContext - a class to manage user definable states based on the
 * contents of a PPDParser.
 */

class PPDContext
{
    typedef std::unordered_map< const PPDKey*, const PPDValue*, PPDKeyhash > hash_type;
    hash_type m_aCurrentValues;
    const PPDParser*                                    m_pParser;

    // returns false: check failed, new value is constrained
    //         true:  check succeeded, new value can be set
    bool checkConstraints( const PPDKey*, const PPDValue*, bool bDoReset );
    bool resetValue( const PPDKey*, bool bDefaultable = false );
public:
    PPDContext();
    PPDContext( const PPDContext& rContext ) { operator=( rContext ); }
    PPDContext& operator=( const PPDContext& rContext ) = default;
    PPDContext& operator=( PPDContext&& rContext );

    void setParser( const PPDParser* );
    const PPDParser* getParser() const { return m_pParser; }

    const PPDValue* getValue( const PPDKey* ) const;
    const PPDValue* setValue( const PPDKey*, const PPDValue*, bool bDontCareForConstraints = false );

    std::size_t countValuesModified() const { return m_aCurrentValues.size(); }
    const PPDKey* getModifiedKey( std::size_t n ) const;

    // public wrapper for the private method
    bool checkConstraints( const PPDKey*, const PPDValue* );

    // for printer setup
    char*   getStreamableBuffer( sal_uLong& rBytes ) const;
    void    rebuildFromStreamBuffer(const std::vector<char> &rBuffer);

    // convenience
    int getRenderResolution() const;

    // width, height in points, paper will contain the name of the selected
    // paper after the call
    void getPageSize( OUString& rPaper, int& rWidth, int& rHeight ) const;
};

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
