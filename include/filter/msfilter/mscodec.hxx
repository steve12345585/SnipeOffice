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

#ifndef INCLUDED_FILTER_MSFILTER_MSCODEC_HXX
#define INCLUDED_FILTER_MSFILTER_MSCODEC_HXX

#include <com/sun/star/uno/Sequence.hxx>
#include <filter/msfilter/msfilterdllapi.h>
#include <rtl/cipher.h>
#include <rtl/digest.h>
#include <sal/types.h>
#include <comphelper/hash.hxx>
#include <vector>

namespace com::sun::star {
    namespace beans { struct NamedValue; }
}

namespace msfilter {


/** Encodes and decodes data from protected MSO 95- documents.
 */
class MSFILTER_DLLPUBLIC MSCodec_Xor95
{
public:
    explicit            MSCodec_Xor95(int nRotateDistance);
    virtual            ~MSCodec_Xor95();

    /** Initializes the algorithm with the specified password.

        @param pPassData
            Character array containing the password. Must be zero terminated,
            which results in a maximum length of 15 characters.
     */
    void                InitKey( const sal_uInt8 pnPassData[ 16 ] );

    /** Initializes the algorithm with the encryption data.

        @param aData
            The sequence contains the necessary data to initialize
            the codec.
     */
    bool                InitCodec( const css::uno::Sequence< css::beans::NamedValue >& aData );

    /** Retrieves the encryption data

        @return
            The sequence contains the necessary data to initialize
            the codec.
     */
    css::uno::Sequence< css::beans::NamedValue > GetEncryptionData();


    /** Verifies the validity of the password using the passed key and hash.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param nKey
            Password key value read from the file.
        @param nHash
            Password hash value read from the file.

        @return
            true = Test was successful.
     */
    bool                VerifyKey( sal_uInt16 nKey, sal_uInt16 nHash ) const;

    /** Reinitializes the codec to start a new memory block.

        Resets the internal key offset to 0.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.
     */
    void                InitCipher();

    /** Decodes a block of memory inplace.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param pnData
            Encrypted data block. Will contain the decrypted data afterwards.
        @param nBytes
            Size of the passed data block.
    */
    virtual void                Decode( sal_uInt8* pnData, std::size_t nBytes )=0;

    /** Lets the cipher skip a specific amount of bytes.

        This function sets the cipher to the same state as if the specified
        amount of data has been decoded with one or more calls of Decode().

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param nBytes
            Number of bytes to be skipped (cipher "seeks" forward).
     */
    void                Skip( std::size_t nBytes );

protected:
    sal_uInt8           mpnKey[ 16 ] = {}; /// Encryption key.
    std::size_t         mnOffset;       /// Key offset.

private:
                        MSCodec_Xor95( const MSCodec_Xor95& ) = delete;
    MSCodec_Xor95&      operator=( const MSCodec_Xor95& ) = delete;

    sal_uInt16          mnKey;          /// Base key from password.
    sal_uInt16          mnHash;         /// Hash value from password.
    int                 mnRotateDistance;
};

/** Encodes and decodes data from protected MSO XLS 95- documents.
 */
class MSFILTER_DLLPUBLIC MSCodec_XorXLS95 final : public MSCodec_Xor95
{
public:
    explicit            MSCodec_XorXLS95() : MSCodec_Xor95(2) {}

    /** Decodes a block of memory inplace.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param pnData
            Encrypted data block. Will contain the decrypted data afterwards.
        @param nBytes
            Size of the passed data block.
    */
    virtual void                Decode( sal_uInt8* pnData, std::size_t nBytes ) override;
};

/** Encodes and decodes data from protected MSO Word 95- documents.
 */
class MSFILTER_DLLPUBLIC MSCodec_XorWord95 final : public MSCodec_Xor95
{
public:
    explicit            MSCodec_XorWord95() : MSCodec_Xor95(7) {}

    /** Decodes a block of memory inplace.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param pnData
            Encrypted data block. Will contain the decrypted data afterwards.
        @param nBytes
            Size of the passed data block.
    */
    virtual void                Decode( sal_uInt8* pnData, std::size_t nBytes ) override;
};

class MSFILTER_DLLPUBLIC MSCodec97
{
public:
    MSCodec97(size_t nHashLen, OUString aEncKeyName);
    virtual ~MSCodec97();

    /** Initializes the algorithm with the encryption data.

        @param aData
            The sequence contains the necessary data to initialize
            the codec.
     */
    bool InitCodec(const css::uno::Sequence< css::beans::NamedValue >& aData);

    /** Retrieves the encryption data

        @return
            The sequence contains the necessary data to initialize
            the codec.
     */
    virtual css::uno::Sequence<css::beans::NamedValue> GetEncryptionData();

    /** Initializes the algorithm with the specified password and document ID.

        @param pPassData
            Wide character array containing the password. Must be zero
            terminated, which results in a maximum length of 15 characters.
        @param pDocId
            Unique document identifier read from or written to the file.
     */
    virtual void InitKey(const sal_uInt16 pPassData[16],
                         const sal_uInt8 pDocId[16]) = 0;


    /** Verifies the validity of the password using the passed salt data.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param pSaltData
            Salt data block read from the file.
        @param pSaltDigest
            Salt digest read from the file.

        @return
            true = Test was successful.
     */
    bool VerifyKey(const sal_uInt8* pSaltData, const sal_uInt8* pSaltDigest);

    virtual void GetDigestFromSalt(const sal_uInt8* pSaltData, sal_uInt8* pDigest) = 0;

    /** Rekeys the codec using the specified counter.

        After reading a specific amount of data the cipher algorithm needs to
        be rekeyed using a counter that counts the data blocks.

        The block size is for example 512 Bytes for Word files and 1024 Bytes
        for Excel files.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param nCounter
            Block counter used to rekey the cipher.
     */
    virtual bool                InitCipher(sal_uInt32 nCounter) = 0;

    /** Encodes a block of memory.

        @see rtl_cipher_encode()

        @precond
            The codec must be initialized with InitKey() before this function
            can be used. The destination buffer must be able to take all
            unencoded data from the source buffer (usually this means it must be
            as long as or longer than the source buffer).

        @param pData
            Unencrypted source data block.
        @param nDatLen
            Size of the passed source data block.
        @param pBuffer
            Destination buffer for the encrypted data.
        @param nBufLen
            Size of the destination buffer.

        @return
            true = Encoding was successful (no error occurred).
    */
    bool                Encode(const void* pData, std::size_t nDatLen,
                               sal_uInt8* pBuffer, std::size_t nBufLen);

    /** Decodes a block of memory.

        @see rtl_cipher_decode()

        @precond
            The codec must be initialized with InitKey() before this function
            can be used. The destination buffer must be able to take all
            encoded data from the source buffer (usually this means it must be
            as long as or longer than the source buffer).

        @param pData
            Encrypted source data block.
        @param nDatLen
            Size of the passed source data block.
        @param pBuffer
            Destination buffer for the decrypted data.
        @param nBufLen
            Size of the destination buffer.

        @return
            true = Decoding was successful (no error occurred).
    */
    bool                Decode(const void* pData, std::size_t nDatLen,
                               sal_uInt8* pBuffer, std::size_t nBufLen);

    /** Lets the cipher skip a specific amount of bytes.

        This function sets the cipher to the same state as if the specified
        amount of data has been decoded with one or more calls of Decode().

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param nDatLen
            Number of bytes to be skipped (cipher "seeks" forward).
     */
    bool                Skip(std::size_t nDatLen);

    /* allows to get the unique document id from the codec
     */
    void                GetDocId( sal_uInt8 pDocId[16] );

private:
                        MSCodec97(const MSCodec97&) = delete;
    MSCodec97&          operator=(const MSCodec97&) = delete;

protected:
    OUString            m_sEncKeyName;
    size_t              m_nHashLen;
    rtlCipher           m_hCipher;
    std::vector<sal_uInt8> m_aDocId;
    std::vector<sal_uInt8> m_aDigestValue;
};

/** Encodes and decodes data from protected MSO 97+ documents.

    This is a wrapper class around low level cryptographic functions from RTL.
    Implementation is based on the wvDecrypt package by Caolan McNamara:
    http://www.csn.ul.ie/~caolan/docs/wvDecrypt.html
 */
class MSFILTER_DLLPUBLIC MSCodec_Std97 final : public MSCodec97
{
public:
    MSCodec_Std97();
    virtual ~MSCodec_Std97() override;

    /** Initializes the algorithm with the specified password and document ID.

        @param pPassData
            Wide character array containing the password. Must be zero
            terminated, which results in a maximum length of 15 characters.
        @param pDocId
            Unique document identifier read from or written to the file.
     */
    virtual void InitKey(const sal_uInt16 pPassData[16],
                         const sal_uInt8 pDocId[16]) override;

    /** Rekeys the codec using the specified counter.

        After reading a specific amount of data the cipher algorithm needs to
        be rekeyed using a counter that counts the data blocks.

        The block size is for example 512 Bytes for Word files and 1024 Bytes
        for Excel files.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param nCounter
            Block counter used to rekey the cipher.
     */
    virtual bool InitCipher(sal_uInt32 nCounter) override;

    /** Creates an MD5 digest of salt digest. */
    void               CreateSaltDigest(
                            const sal_uInt8 nSaltData[16], sal_uInt8 nSaltDigest[16] );

    /** Gets salt data and salt digest.

        @precond
            The codec must be initialized with InitKey() before this function
            can be used.

        @param pSalt
            Salt, a random number.
        @param pSaltData
            Salt data block generated from the salt.
        @param pSaltDigest
            Salt digest generated from the salt.
     */
    void                GetEncryptKey (
                            const sal_uInt8 pSalt[16],
                            sal_uInt8 pSaltData[16],
                            sal_uInt8 pSaltDigest[16]);

    virtual void        GetDigestFromSalt(const sal_uInt8* pSaltData, sal_uInt8* pDigest) override;

private:
                        MSCodec_Std97( const MSCodec_Std97& ) = delete;
    MSCodec_Std97&      operator=( const MSCodec_Std97& ) = delete;

    rtlDigest           m_hDigest;
};

class MSFILTER_DLLPUBLIC MSCodec_CryptoAPI final : public MSCodec97
{
private:
    css::uno::Sequence<sal_Int8> m_aStd97Key;
public:
    MSCodec_CryptoAPI();

    virtual void InitKey(const sal_uInt16 pPassData[16],
                         const sal_uInt8 pDocId[16]) override;
    virtual bool InitCipher(sal_uInt32 nCounter) override;
    virtual void GetDigestFromSalt(const sal_uInt8* pSaltData, sal_uInt8* pDigest) override;
    virtual css::uno::Sequence<css::beans::NamedValue> GetEncryptionData() override;
};

const sal_uInt32 ENCRYPTINFO_CRYPTOAPI      = 0x00000004;
const sal_uInt32 ENCRYPTINFO_DOCPROPS       = 0x00000008;
const sal_uInt32 ENCRYPTINFO_EXTERNAL       = 0x00000010;
const sal_uInt32 ENCRYPTINFO_AES            = 0x00000020;

const sal_uInt32 ENCRYPT_ALGO_AES128        = 0x0000660E;
const sal_uInt32 ENCRYPT_ALGO_AES192        = 0x0000660F;
const sal_uInt32 ENCRYPT_ALGO_AES256        = 0x00006610;
const sal_uInt32 ENCRYPT_ALGO_RC4           = 0x00006801;

const sal_uInt32 ENCRYPT_HASH_SHA1          = 0x00008004;

const sal_uInt32 ENCRYPT_KEY_SIZE_AES_128   = 0x00000080;
const sal_uInt32 ENCRYPT_KEY_SIZE_AES_192   = 0x000000C0;
const sal_uInt32 ENCRYPT_KEY_SIZE_AES_256   = 0x00000100;

const sal_uInt32 ENCRYPT_PROVIDER_TYPE_AES  = 0x00000018;
const sal_uInt32 ENCRYPT_PROVIDER_TYPE_RC4  = 0x00000001;

// version of encryption info used in MS Office 1997 (major = 1, minor = 1)
const sal_uInt32 VERSION_INFO_1997_FORMAT       = 0x00010001;
// version of encryption info used in MS Office 2007 (major = 3, minor = 2)
const sal_uInt32 VERSION_INFO_2007_FORMAT       = 0x00020003;
// version of encryption info used in MS Office 2007 SP2 and older (major = 4, minor = 2)
const sal_uInt32 VERSION_INFO_2007_FORMAT_SP2   = 0x00020004;

// version of encryption info - agile (major = 4, minor = 4)
const sal_uInt32 VERSION_INFO_AGILE         = 0x00040004;

const sal_uInt32 AGILE_ENCRYPTION_RESERVED  = 0x00000040;

const sal_uInt32 SALT_LENGTH                    = 16;
const sal_uInt32 ENCRYPTED_VERIFIER_LENGTH      = 16;

struct MSFILTER_DLLPUBLIC EncryptionStandardHeader
{
    sal_uInt32 flags;
    sal_uInt32 sizeExtra;       // 0
    sal_uInt32 algId;           // if flag AES && CRYPTOAPI this defaults to 128-bit AES
    sal_uInt32 algIdHash;       // 0: determine by flags - defaults to SHA-1 if not external
    sal_uInt32 keyBits;         // key size in bits: 0 (determine by flags), 128, 192, 256
    sal_uInt32 providedType;    // AES or RC4
    sal_uInt32 reserved1;       // 0
    sal_uInt32 reserved2;       // 0

    EncryptionStandardHeader();
};

struct MSFILTER_DLLPUBLIC EncryptionVerifierAES
{
    sal_uInt32 saltSize;                                                // must be 0x00000010
    sal_uInt8  salt[SALT_LENGTH] = {};                                  // random generated salt value
    sal_uInt8  encryptedVerifier[ENCRYPTED_VERIFIER_LENGTH] = {};       // randomly generated verifier value
    sal_uInt32 encryptedVerifierHashSize;                               // actually written hash size - depends on algorithm
    sal_uInt8  encryptedVerifierHash[comphelper::SHA256_HASH_LENGTH] = {};          // verifier value hash - itself also encrypted

    EncryptionVerifierAES();
};

struct MSFILTER_DLLPUBLIC EncryptionVerifierRC4
{
    sal_uInt32 saltSize;                                                // must be 0x00000010
    sal_uInt8  salt[SALT_LENGTH] = {};                                  // random generated salt value
    sal_uInt8  encryptedVerifier[ENCRYPTED_VERIFIER_LENGTH] = {};       // randomly generated verifier value
    sal_uInt32 encryptedVerifierHashSize;                               // actually written hash size - depends on algorithm
    sal_uInt8  encryptedVerifierHash[comphelper::SHA1_HASH_LENGTH] = {};            // verifier value hash - itself also encrypted

    EncryptionVerifierRC4();
};

struct MSFILTER_DLLPUBLIC StandardEncryptionInfo
{
    EncryptionStandardHeader header;
    EncryptionVerifierAES    verifier;
};

struct MSFILTER_DLLPUBLIC RC4EncryptionInfo
{
    EncryptionStandardHeader header;
    EncryptionVerifierRC4 verifier;
};

} // namespace msfilter

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
