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

#include <rtl/character.hxx>
#include <rtl/ustring.hxx>
#include <basic/sbxcore.hxx>
#include <basic/basicdllapi.h>
#include <com/sun/star/uno/XInterface.hpp>
#include <com/sun/star/uno/Reference.hxx>

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <memory>


namespace com::sun::star::bridge::oleautomation { struct Decimal; }

class SbxDecimal;
enum class SfxHintId;

struct SbxValues
{
    union {
        sal_uInt8       nByte;
        sal_uInt16      nUShort;
        sal_Unicode     nChar;
        sal_Int16       nInteger;
        sal_uInt32      nULong;
        sal_Int32       nLong;
        unsigned int    nUInt;
        int             nInt;
        sal_uInt64      uInt64;
        sal_Int64       nInt64;

        float           nSingle;
        double          nDouble;

        OUString*       pOUString;
        SbxDecimal*     pDecimal;

        SbxBase*        pObj;

        sal_uInt8*      pByte;
        sal_uInt16*     pUShort;
        sal_Unicode*    pChar;
        sal_Int16*      pInteger;
        sal_uInt32*     pULong;
        sal_Int32*      pLong;
        sal_uInt64*     puInt64;
        sal_Int64*      pnInt64;

        float*          pSingle;
        double*         pDouble;

        void*           pData;
    };
    SbxDataType  eType;

    SbxValues(): pData( nullptr ), eType(SbxEMPTY) {}
    SbxValues( SbxDataType e ): pData( nullptr ), eType(e) {}
    SbxValues( double _nDouble ): nDouble( _nDouble ), eType(SbxDOUBLE) {}

    void clear(SbxDataType type) {
        // A hacky way of zeroing the union value corresponding to the given type (even though the
        // relevant zero value need not be represented by all-zero bits, in general) without evoking
        // GCC 8 -Wclass-memaccess or loplugin:classmemaccess, and without having to turn the
        // anonymous union into a non-anonymous one:
        auto const p = static_cast<void *>(this);
        std::memset(p, 0, offsetof(SbxValues, eType));
        eType = type;
    }
};

class BASIC_DLLPUBLIC SbxValue : public SbxBase
{
    // #55226 Transport additional infos
    BASIC_DLLPRIVATE SbxValue* TheRealValue( bool bObjInObjError ) const;
protected:
    SbxValues aData; // Data
    OUString aPic;  // Picture-String
    OUString aToolString;  // tool string copy

    virtual void Broadcast( SfxHintId );      // Broadcast-Call
    virtual ~SbxValue() override;
    virtual bool LoadData( SvStream&, sal_uInt16 ) override;
    virtual std::pair<bool, sal_uInt32> StoreData( SvStream& ) const override;
public:
    SBX_DECL_PERSIST_NODATA(SBXID_VALUE,1);
    SbxValue();
    SAL_DLLPRIVATE SbxValue( SbxDataType );
    SbxValue( const SbxValue& );
    SbxValue& operator=( const SbxValue& );
    virtual void Clear() override;
    virtual bool IsFixed() const override;

    bool IsInteger()    const { return GetType() == SbxINTEGER   ; }
    bool IsLong()       const { return GetType() == SbxLONG      ; }
    bool IsDouble()     const { return GetType() == SbxDOUBLE    ; }
    bool IsString()     const { return GetType() == SbxSTRING    ; }
    bool IsCurrency()   const { return GetType() == SbxCURRENCY  ; }
    bool IsObject()     const { return GetType() == SbxOBJECT    ; }
    bool IsBool()       const { return GetType() == SbxBOOL      ; }
    bool IsErr()        const { return GetType() == SbxERROR     ; }
    bool IsEmpty()      const { return GetType() == SbxEMPTY     ; }
    bool IsNull()       const { return GetType() == SbxNULL      ; }
    bool IsNumeric() const;
    SAL_DLLPRIVATE bool IsNumericRTL() const;  // #41692 Interface for Basic
    SAL_DLLPRIVATE bool ImpIsNumeric( bool bOnlyIntntl ) const;    // Implementation

    virtual SbxDataType GetType() const override;
    SbxDataType GetFullType() const { return aData.eType;}
    SAL_DLLPRIVATE bool SetType( SbxDataType );

    bool Get( SbxValues& ) const;
    const SbxValues& GetValues_Impl() const { return aData; }
    bool Put( const SbxValues& );

    SbxValues * data() { return &aData; }

    sal_Unicode GetChar() const { return Get(SbxCHAR).nChar; }
    sal_Int16 GetInteger() const { return Get(SbxINTEGER).nInteger; }
    sal_Int32 GetLong() const { return Get(SbxLONG).nLong; }
    sal_Int64 GetInt64() const { return Get(SbxSALINT64).nInt64; }
    sal_uInt64 GetUInt64() const { return Get(SbxSALUINT64).uInt64; }

    sal_Int64 GetCurrency() const { return Get(SbxCURRENCY).nInt64; }
    SbxDecimal* GetDecimal() const { return Get(SbxDECIMAL).pDecimal; }

    float GetSingle() const { return Get(SbxSINGLE).nSingle; }
    double GetDouble() const { return Get(SbxDOUBLE).nDouble; }
    double GetDate() const { return Get(SbxDATE).nDouble; }

    bool GetBool() const { return Get(SbxBOOL).nUShort != 0; }
    SAL_DLLPRIVATE const OUString&   GetCoreString() const;
    OUString    GetOUString() const;

    SbxBase* GetObject() const { return Get(SbxOBJECT).pObj; }
    sal_uInt8 GetByte() const { return Get(SbxBYTE).nByte; }
    sal_uInt16 GetUShort() const { return Get(SbxUSHORT).nUShort; }
    sal_uInt32 GetULong() const { return Get(SbxULONG).nULong; }

    SAL_DLLPRIVATE bool PutInteger( sal_Int16 );
    bool PutLong( sal_Int32 );
    SAL_DLLPRIVATE bool PutSingle( float );
    bool PutDouble( double );
    SAL_DLLPRIVATE void PutDate( double );
    bool PutBool( bool );
    SAL_DLLPRIVATE void PutErr( sal_uInt16 );
    void PutStringExt( const OUString& );     // with extended analysis (International, "sal_True"/"sal_False")
    SAL_DLLPRIVATE bool PutInt64( sal_Int64 );
    SAL_DLLPRIVATE bool PutUInt64( sal_uInt64 );
    bool PutString( const OUString& );
    bool PutChar( sal_Unicode );
    SAL_DLLPRIVATE bool PutByte( sal_uInt8 );
    bool PutUShort( sal_uInt16 );
    bool PutULong( sal_uInt32 );
    bool PutEmpty();
    SAL_DLLPRIVATE void PutNull();

            // Special methods
    SAL_DLLPRIVATE void PutDecimal( css::bridge::oleautomation::Decimal const & rAutomationDec );
    SAL_DLLPRIVATE bool PutDecimal( SbxDecimal* pDecimal ); // This function is needed for Windows build, don't remove
    SAL_DLLPRIVATE void fillAutomationDecimal( css::bridge::oleautomation::Decimal& rAutomationDec ) const;
    SAL_DLLPRIVATE bool PutCurrency( sal_Int64 );
            // Interface for CDbl in Basic
    SAL_DLLPRIVATE static ErrCode ScanNumIntnl( const OUString& rSrc, double& nVal, bool bSingle = false );

    bool PutObject( SbxBase* );

    SAL_DLLPRIVATE bool Convert( SbxDataType );
    bool Compute( SbxOperator, const SbxValue& );
    bool Compare( SbxOperator, const SbxValue& ) const;
    SAL_DLLPRIVATE bool Scan( std::u16string_view, sal_Int32* );
    SAL_DLLPRIVATE void Format( OUString&, const OUString* = nullptr ) const;

    // The following operators are defined for easier handling.
    // TODO: Ensure error conditions (overflow, conversions)
    // are taken into consideration in Compute and Compare

    inline bool operator <=( const SbxValue& ) const;
    inline bool operator >=( const SbxValue& ) const;

    inline SbxValue& operator *=( const SbxValue& );
    inline SbxValue& operator /=( const SbxValue& );
    inline SbxValue& operator +=( const SbxValue& );
    inline SbxValue& operator -=( const SbxValue& );

private:
    SbxValues Get(SbxDataType t) const;
};

inline bool SbxValue::operator<=( const SbxValue& r ) const
{ return Compare( SbxLE, r ); }

inline bool SbxValue::operator>=( const SbxValue& r ) const
{ return Compare( SbxGE, r ); }

inline SbxValue& SbxValue::operator*=( const SbxValue& r )
{ Compute( SbxMUL, r ); return *this; }

inline SbxValue& SbxValue::operator/=( const SbxValue& r )
{ Compute( SbxDIV, r ); return *this; }

inline SbxValue& SbxValue::operator+=( const SbxValue& r )
{ Compute( SbxPLUS, r ); return *this; }

inline SbxValue& SbxValue::operator-=( const SbxValue& r )
{ Compute( SbxMINUS, r ); return *this; }

class SbxArray;
class SbxInfo;

typedef tools::SvRef<SbxArray> SbxArrayRef;

typedef tools::SvRef<SbxInfo> SbxInfoRef;

class SfxBroadcaster;

class SbxVariableImpl;
class StarBASIC;

class BASIC_DLLPUBLIC SbxVariable : public SbxValue
{
    friend class SbMethod;

    OUString         m_aDeclareClassName;
    css::uno::Reference< css::uno::XInterface > m_xComListener;
    StarBASIC*       m_pComListenerParentBasic = nullptr;
    std::unique_ptr<SfxBroadcaster>  mpBroadcaster; // Broadcaster, if needed
    OUString         maName;            // Name, if available
    mutable OUString maNameCI;          // Name, case insensitive - cached for fast comparison
    SbxArrayRef      mpPar;             // Parameter-Array, if set
    sal_uInt16       nHash = 0;         // Hash-ID for search

protected:
    SbxInfoRef  pInfo;              // Probably called information
    sal_uInt32 nUserData= 0;        // User data for Call()
    SbxObject* pParent = nullptr;   // Currently attached object
    SAL_DLLPRIVATE virtual ~SbxVariable() override;
    SAL_DLLPRIVATE virtual bool LoadData( SvStream&, sal_uInt16 ) override;
    SAL_DLLPRIVATE virtual std::pair<bool, sal_uInt32> StoreData( SvStream& ) const override;
public:
    SBX_DECL_PERSIST_NODATA(SBXID_VARIABLE,2);
    SbxVariable();
    SbxVariable( SbxDataType );
    SAL_DLLPRIVATE SbxVariable( const SbxVariable& );
    SbxVariable& operator=( const SbxVariable& );

    SAL_DLLPRIVATE void Dump( SvStream&, bool bDumpAll );

    SAL_DLLPRIVATE void SetName( const OUString& );
    const OUString& GetName( SbxNameType = SbxNameType::NONE ) const;
    sal_uInt16 GetHashCode() const          { return nHash; }
    SAL_DLLPRIVATE static OUString NameToCaseInsensitiveName(const OUString& rName);

    SAL_DLLPRIVATE virtual void SetModified( bool ) override;

    sal_uInt32 GetUserData() const { return nUserData; }
    void SetUserData( sal_uInt32 n ) { nUserData = n; }

    SAL_DLLPRIVATE virtual SbxDataType  GetType()  const override;
    SAL_DLLPRIVATE virtual SbxClassType GetClass() const;

    // Parameter-Interface
    SAL_DLLPRIVATE virtual SbxInfo* GetInfo();
    SAL_DLLPRIVATE void SetInfo( SbxInfo* p );
    void SetParameters( SbxArray* p );
    SbxArray* GetParameters() const;

    // Sfx-Broadcasting-Support:
    // Due to data reduction and better DLL-hierarchy currently via casting
    SfxBroadcaster& GetBroadcaster();
    bool IsBroadcaster() const { return mpBroadcaster != nullptr; }
    virtual void Broadcast( SfxHintId nHintId ) override;

    const SbxObject* GetParent() const { return pParent; }
    SbxObject* GetParent() { return pParent;}
    SAL_DLLPRIVATE virtual void SetParent( SbxObject* );

    SAL_DLLPRIVATE const OUString& GetDeclareClassName() const;
    SAL_DLLPRIVATE void SetDeclareClassName( const OUString& );
    SAL_DLLPRIVATE void SetComListener( const css::uno::Reference< css::uno::XInterface >& xComListener,
                         StarBASIC* pParentBasic );
    SAL_DLLPRIVATE void ClearComListener();

    // Create a simple hashcode: the first six characters are evaluated.
    static constexpr sal_uInt16 MakeHashCode(std::u16string_view aName)
    {
        sal_uInt16 n = 0;
        const auto first6 = aName.substr(0, 6);
        for (const auto& c : first6)
        {
            if (!rtl::isAscii(c))
                continue; // Just skip it to let non-ASCII strings have some hash variance
            n = static_cast<sal_uInt16>((n << 3) + rtl::toAsciiUpperCase(c));
        }
        return n;
    }
};

typedef tools::SvRef<SbxObject> SbxObjectRef;
typedef tools::SvRef<SbxVariable> SbxVariableRef;

//tdf#59222 SbxEnsureParentVariable is a SbxVariable which keeps a reference to
//its parent, ensuring it always exists while this SbxVariable exists
class SbxEnsureParentVariable final : public SbxVariable
{
    SbxObjectRef xParent;
public:
    SbxEnsureParentVariable(const SbxVariable& r);
    virtual void SetParent(SbxObject* p) override;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
