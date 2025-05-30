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

#include <algorithm>

#include <formula/formulahelper.hxx>
#include <formula/IFunctionDescription.hxx>
#include <unotools/charclass.hxx>
#include <unotools/syslocale.hxx>

namespace formula
{

    namespace
    {

        class OEmptyFunctionDescription : public IFunctionDescription
        {
        public:
            OEmptyFunctionDescription(){}
            virtual ~OEmptyFunctionDescription(){}

            virtual OUString getFunctionName() const override { return OUString(); }
            virtual const IFunctionCategory* getCategory() const override { return nullptr; }
            virtual OUString getDescription() const override { return OUString(); }
            virtual sal_Int32 getSuppressedArgumentCount() const override { return 0; }
            virtual OUString getFormula(const ::std::vector< OUString >& ) const override { return OUString(); }
            virtual void fillVisibleArgumentMapping(::std::vector<sal_uInt16>& ) const override {}
            virtual void initArgumentInfo()  const override {}
            virtual OUString getSignature() const override { return OUString(); }
            virtual OUString getHelpId() const override { return u""_ustr; }
            virtual bool isHidden() const override { return false; }
            virtual sal_uInt32 getParameterCount() const override { return 0; }
            virtual sal_uInt32 getVarArgsStart() const override { return 0; }
            virtual sal_uInt32 getVarArgsLimit() const override { return 0; }
            virtual OUString getParameterName(sal_uInt32 ) const override { return OUString(); }
            virtual OUString getParameterDescription(sal_uInt32 ) const override { return OUString(); }
            virtual bool isParameterOptional(sal_uInt32 ) const override { return false; }
        };
    }

//  class FormulaHelper - static Method


#define FUNC_NOTFOUND -1

FormulaHelper::FormulaHelper(const IFunctionManager* _pFunctionManager)
    :m_rCharClass(m_aSysLocale.GetCharClass())
    ,m_pFunctionManager(_pFunctionManager)
    ,open(_pFunctionManager->getSingleToken(IFunctionManager::eOk))
    ,close(_pFunctionManager->getSingleToken(IFunctionManager::eClose))
    ,sep(_pFunctionManager->getSingleToken(IFunctionManager::eSep))
    ,arrayOpen(_pFunctionManager->getSingleToken(IFunctionManager::eArrayOpen))
    ,arrayClose(_pFunctionManager->getSingleToken(IFunctionManager::eArrayClose))
    ,tableRefOpen(_pFunctionManager->getSingleToken(IFunctionManager::eTableRefOpen))
    ,tableRefClose(_pFunctionManager->getSingleToken(IFunctionManager::eTableRefClose))
{
}

sal_Int32 FormulaHelper::GetCategoryCount() const
{
    return m_pFunctionManager->getCount();
}

bool FormulaHelper::GetNextFunc( const OUString&  rFormula,
                                     bool             bBack,
                                     sal_Int32&       rFStart,   // Input and output
                                     sal_Int32*       pFEnd,     // = NULL
                                     const IFunctionDescription**   ppFDesc,   // = NULL
                                     ::std::vector< OUString>*   pArgs )  const // = NULL
{
    sal_Int32  nOldStart = rFStart;
    OUString   aFname;

    rFStart = GetFunctionStart( rFormula, rFStart, bBack, ppFDesc ? &aFname : nullptr );
    bool bFound  = ( rFStart != FUNC_NOTFOUND );

    if ( bFound )
    {
        if ( pFEnd )
            *pFEnd = GetFunctionEnd( rFormula, rFStart );

        if ( ppFDesc )
        {
            *ppFDesc = nullptr;
            const sal_uInt32 nCategoryCount = m_pFunctionManager->getCount();
            for(sal_uInt32 j= 0; j < nCategoryCount && !*ppFDesc; ++j)
            {
                const IFunctionCategory* pCategory = m_pFunctionManager->getCategory(j);
                const sal_uInt32 nCount = pCategory->getCount();
                for(sal_uInt32 i = 0 ; i < nCount; ++i)
                {
                    const IFunctionDescription* pCurrent = pCategory->getFunction(i);
                    if ( pCurrent->getFunctionName().equalsIgnoreAsciiCase(aFname) )
                    {
                        *ppFDesc = pCurrent;
                        break;
                    }
                }// for(sal_uInt32 i = 0 ; i < nCount; ++i)
            }
            if ( *ppFDesc && pArgs )
            {
                GetArgStrings( *pArgs,rFormula, rFStart, static_cast<sal_uInt16>((*ppFDesc)->getParameterCount() ));
            }
            else
            {
                static OEmptyFunctionDescription s_aFunctionDescription;
                *ppFDesc = &s_aFunctionDescription;
            }
        }
    }
    else
        rFStart = nOldStart;

    return bFound;
}


void FormulaHelper::FillArgStrings( std::u16string_view rFormula,
                                    sal_Int32         nFuncPos,
                                    sal_uInt16        nArgs,
                                    ::std::vector< OUString >& _rArgs ) const
{
    sal_Int32       nStart  = 0;
    sal_Int32       nEnd    = 0;
    sal_uInt16      i;
    bool        bLast   = false;

    for ( i=0; i<nArgs && !bLast; i++ )
    {
        nStart = GetArgStart( rFormula, nFuncPos, i );

        if ( i+1<nArgs ) // last argument?
        {
            nEnd = GetArgStart( rFormula, nFuncPos, i+1 );

            if ( nEnd != nStart )
                _rArgs.emplace_back(rFormula.substr( nStart, nEnd-1-nStart ));
            else
            {
                _rArgs.emplace_back();
                bLast = true;
            }
        }
        else
        {
            nEnd = GetFunctionEnd( rFormula, nFuncPos )-1;
            if ( nStart < nEnd )
                _rArgs.emplace_back(rFormula.substr( nStart, nEnd-nStart ));
            else
                _rArgs.emplace_back();
        }
    }

    if ( bLast )
        for ( ; i<nArgs; i++ )
            _rArgs.emplace_back();
}


void FormulaHelper::GetArgStrings( ::std::vector< OUString >& _rArgs,
                                   std::u16string_view rFormula,
                                   sal_Int32 nFuncPos,
                                   sal_uInt16 nArgs ) const
{
    if (nArgs)
    {
        FillArgStrings( rFormula, nFuncPos, nArgs, _rArgs );
    }
}


static bool IsFormulaText(const CharClass& rCharClass, const OUString& rStr, sal_Int32 nPos)
{
    if( rCharClass.isLetterNumeric( rStr, nPos ) )
        return true;
    else
    {   // In internationalized versions function names may contain a dot
        //  and in every version also an underscore... ;-)
        sal_Unicode c = rStr[nPos];
        return c == '.' || c == '_';
    }

}

sal_Int32 FormulaHelper::GetFunctionStart( const OUString&   rFormula,
                                            sal_Int32         nStart,
                                            bool              bBack,
                                            OUString*         pFuncName ) const
{
    sal_Int32 nStrLen = rFormula.getLength();

    if ( nStrLen < nStart )
        return nStart;

    sal_Int32  nFStart = FUNC_NOTFOUND;
    sal_Int32  nParPos = bBack ? ::std::min( nStart, nStrLen - 1) : nStart;

    bool bRepeat;
    do
    {
        bool bFound  = false;
        bRepeat = false;

        if ( bBack )
        {
            while ( !bFound && (nParPos > 0) )
            {
                if ( rFormula[nParPos] == '"' )
                {
                    nParPos--;
                    while ( (nParPos > 0) && rFormula[nParPos] != '"' )
                        nParPos--;
                    if (nParPos > 0)
                        nParPos--;
                }
                else
                {
                    bFound = rFormula[nParPos] == '(';
                    if ( !bFound )
                        nParPos--;
                }
            }
        }
        else
        {
            while ( !bFound && (0 <= nParPos && nParPos < nStrLen) )
            {
                if ( rFormula[nParPos] == '"' )
                {
                    nParPos++;
                    while ( (nParPos < nStrLen) && rFormula[nParPos] != '"' )
                        nParPos++;
                    nParPos++;
                }
                else
                {
                    bFound = rFormula[nParPos] == '(';
                    if ( !bFound )
                        nParPos++;
                }
            }
        }

        if ( bFound && (nParPos > 0) )
        {
            nFStart = nParPos-1;

            while ( (nFStart > 0) && IsFormulaText(m_rCharClass, rFormula, nFStart ))
                nFStart--;
        }

        nFStart++;

        if ( bFound )
        {
            if ( IsFormulaText( m_rCharClass, rFormula, nFStart ) )
            {
                                    //  Function found
                if ( pFuncName )
                    *pFuncName = rFormula.copy( nFStart, nParPos-nFStart );
            }
            else                    // Brackets without function -> keep searching
            {
                bRepeat = true;
                if ( !bBack )
                    nParPos++;
                else if (nParPos > 0)
                    nParPos--;
                else
                    bRepeat = false;
            }
        }
        else                        // No brackets found
        {
            nFStart = FUNC_NOTFOUND;
            if ( pFuncName )
                pFuncName->clear();
        }
    }
    while(bRepeat);

    return nFStart;
}


sal_Int32  FormulaHelper::GetFunctionEnd( std::u16string_view rStr, sal_Int32 nStart ) const
{
    sal_Int32 nStrLen = rStr.size();

    if ( nStrLen < nStart )
        return nStart;

    short   nParCount = 0;
    short   nTableRefCount = 0;
    bool    bInArray = false;
    bool    bFound = false;
    bool    bTickEscaped = false;

    while ( !bFound && (nStart < nStrLen) )
    {
        sal_Unicode c = rStr[nStart];

        if (nTableRefCount > 0)
        {
            // Column names may contain anything, skip. Also skip separator
            // between item specifier and column name or whatever in a
            // TableRef [[...]; [...]]
            // But keep track of apostrophe ' tick escaped brackets.
            if (c == '\'')
                bTickEscaped = !bTickEscaped;
            else
            {
                if (c == tableRefOpen && !bTickEscaped)
                    ++nTableRefCount;
                else if (c == tableRefClose && !bTickEscaped)
                    --nTableRefCount;
                bTickEscaped = false;
            }
        }
        else if (c == tableRefOpen)
        {
            ++nTableRefCount;
        }
        else if ( c == '"' )
        {
            nStart++;
            while ( (nStart < nStrLen) && rStr[nStart] != '"' )
                nStart++;
        }
        else if ( c == open )
            nParCount++;
        else if ( c == close )
        {
            nParCount--;
            if ( nParCount == 0 )
                bFound = true;
            else if ( nParCount < 0 )
            {
                bFound = true;
                nStart--;   // read one too far
            }
        }
        else if ( c == arrayOpen )
        {
            bInArray = true;
        }
        else if ( c == arrayClose )
        {
            bInArray = false;
        }
        else if ( c == sep )
        {
            if ( !bInArray && nParCount == 0 )
            {
                bFound = true;
                nStart--;   // read one too far
            }
        }
        nStart++; // Set behind found position
    }

    // nStart > nStrLen can happen if there was an unclosed quote; instead of
    // checking that in every loop iteration check it once here.
    return std::min(nStart, nStrLen);
}


sal_Int32 FormulaHelper::GetArgStart( std::u16string_view rStr, sal_Int32 nStart, sal_uInt16 nArg ) const
{
    sal_Int32 nStrLen = rStr.size();

    if ( nStrLen < nStart )
        return nStart;

    short   nParCount   = 0;
    short   nTableRefCount = 0;
    bool    bInArray    = false;
    bool    bFound      = false;
    bool    bTickEscaped = false;

    while ( !bFound && (nStart < nStrLen) )
    {
        sal_Unicode c = rStr[nStart];

        if (nTableRefCount > 0)
        {
            // Column names may contain anything, skip. Also skip separator
            // between item specifier and column name or whatever in a
            // TableRef [[...]; [...]]
            // But keep track of apostrophe ' tick escaped brackets.
            if (c == '\'')
                bTickEscaped = !bTickEscaped;
            else
            {
                if (c == tableRefOpen && !bTickEscaped)
                    ++nTableRefCount;
                else if (c == tableRefClose && !bTickEscaped)
                    --nTableRefCount;
                bTickEscaped = false;
            }
        }
        else if (c == tableRefOpen)
        {
            ++nTableRefCount;
        }
        else if ( c == '"' )
        {
            nStart++;
            while ( (nStart < nStrLen) && rStr[nStart] != '"' )
                nStart++;
        }
        else if ( c == open )
        {
            bFound = ( nArg == 0 );
            nParCount++;
        }
        else if ( c == close )
        {
            nParCount--;
            bFound = ( nParCount == 0 );
        }
        else if ( c == arrayOpen )
        {
            bInArray = true;
        }
        else if ( c == arrayClose )
        {
            bInArray = false;
        }
        else if ( c == sep )
        {
            if ( !bInArray && nParCount == 1 )
            {
                nArg--;
                bFound = ( nArg == 0  );
            }
        }
        nStart++;
    }

    return nStart;
}

} // formula


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
