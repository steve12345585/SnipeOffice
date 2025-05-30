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

#include "nssrenam.h"
#include <secder.h>

#include <cert.h>
#include <pk11pub.h>
#include <hasht.h>

#include <comphelper/sequence.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustrbuf.hxx>
#include <sal/log.hxx>
#include "x509certificate_nssimpl.hxx"

#include <biginteger.hxx>
#include <certificateextension_xmlsecimpl.hxx>

#include "sanextension_nssimpl.hxx"
#include <tools/time.hxx>
#include <svl/sigstruct.hxx>

using ::css::util::DateTime;

X509Certificate_NssImpl::X509Certificate_NssImpl() :
    m_pCert(nullptr)
{
}

X509Certificate_NssImpl::~X509Certificate_NssImpl() {
    if( m_pCert != nullptr ) {
        CERT_DestroyCertificate( m_pCert ) ;
    }
}

//Methods from XCertificate
sal_Int16 SAL_CALL X509Certificate_NssImpl::getVersion() {
    if( m_pCert != nullptr ) {
        if( m_pCert->version.len > 0 ) {
            return static_cast<char>(*( m_pCert->version.data )) ;
        } else
            return 0 ;
    } else {
        return -1 ;
    }
}

css::uno::Sequence< sal_Int8 > SAL_CALL X509Certificate_NssImpl::getSerialNumber() {
    if( m_pCert != nullptr && m_pCert->serialNumber.len > 0 ) {
        return comphelper::arrayToSequence<sal_Int8>(m_pCert->serialNumber.data,
                                                     m_pCert->serialNumber.len) ;
    } else {
        return css::uno::Sequence< sal_Int8 >();
    }
}

OUString SAL_CALL X509Certificate_NssImpl::getIssuerName() {
    if( m_pCert != nullptr ) {
        return OUString(m_pCert->issuerName , PL_strlen(m_pCert->issuerName) , RTL_TEXTENCODING_UTF8) ;
    } else {
        return OUString() ;
    }
}

OUString SAL_CALL X509Certificate_NssImpl::getSubjectName() {
    if( m_pCert != nullptr ) {
        return OUString(m_pCert->subjectName , PL_strlen(m_pCert->subjectName) , RTL_TEXTENCODING_UTF8);
    } else {
        return OUString() ;
    }
}

css::util::DateTime SAL_CALL X509Certificate_NssImpl::getNotValidBefore() {
    if( m_pCert != nullptr ) {
        SECStatus rv ;
        PRTime notBefore ;
        PRExplodedTime explTime ;
        DateTime dateTime ;

        rv = DER_DecodeTimeChoice( &notBefore, &m_pCert->validity.notBefore ) ;
        if( rv != SECStatus::SECSuccess ) {
            return DateTime() ;
        }

        //Convert the time to readable local time
        PR_ExplodeTime( notBefore, PR_LocalTimeParameters, &explTime ) ;

        dateTime.NanoSeconds = static_cast< sal_Int32 >( explTime.tm_usec * ::tools::Time::nanoPerMicro  );
        dateTime.Seconds = static_cast< sal_Int16 >( explTime.tm_sec  );
        dateTime.Minutes = static_cast< sal_Int16 >( explTime.tm_min  );
        dateTime.Hours = static_cast< sal_Int16 >( explTime.tm_hour  );
        dateTime.Day = static_cast< sal_Int16 >( explTime.tm_mday  );
        dateTime.Month = static_cast< sal_Int16 >( explTime.tm_month+1  );
        dateTime.Year = static_cast< sal_Int16 >( explTime.tm_year  );

        return dateTime ;
    } else {
        return DateTime() ;
    }
}

css::util::DateTime SAL_CALL X509Certificate_NssImpl::getNotValidAfter() {
    if( m_pCert != nullptr ) {
        SECStatus rv ;
        PRTime notAfter ;
        PRExplodedTime explTime ;
        DateTime dateTime ;

        rv = DER_DecodeTimeChoice( &notAfter, &m_pCert->validity.notAfter ) ;
        if( rv != SECStatus::SECSuccess ) {
            return DateTime() ;
        }

        //Convert the time to readable local time
        PR_ExplodeTime( notAfter, PR_LocalTimeParameters, &explTime ) ;

        dateTime.NanoSeconds = static_cast< sal_Int16 >( explTime.tm_usec * ::tools::Time::nanoPerMicro );
        dateTime.Seconds = static_cast< sal_Int16 >( explTime.tm_sec  );
        dateTime.Minutes = static_cast< sal_Int16 >( explTime.tm_min  );
        dateTime.Hours = static_cast< sal_Int16 >( explTime.tm_hour  );
        dateTime.Day = static_cast< sal_Int16 >( explTime.tm_mday  );
        dateTime.Month = static_cast< sal_Int16 >( explTime.tm_month+1  );
        dateTime.Year = static_cast< sal_Int16 >( explTime.tm_year  );

        return dateTime ;
    } else {
        return DateTime() ;
    }
}

css::uno::Sequence< sal_Int8 > SAL_CALL X509Certificate_NssImpl::getIssuerUniqueID() {
    if( m_pCert != nullptr && m_pCert->issuerID.len > 0 ) {
        return comphelper::arrayToSequence<sal_Int8>(m_pCert->issuerID.data, m_pCert->issuerID.len) ;
    } else {
        return css::uno::Sequence< sal_Int8 >();
    }
}

css::uno::Sequence< sal_Int8 > SAL_CALL X509Certificate_NssImpl::getSubjectUniqueID() {
    if( m_pCert != nullptr && m_pCert->subjectID.len > 0 ) {
        return comphelper::arrayToSequence<sal_Int8>(m_pCert->subjectID.data,
                                                     m_pCert->subjectID.len) ;
    } else {
        return css::uno::Sequence< sal_Int8 >();
    }
}

css::uno::Sequence< css::uno::Reference< css::security::XCertificateExtension > > SAL_CALL X509Certificate_NssImpl::getExtensions() {
    if( m_pCert != nullptr && m_pCert->extensions != nullptr ) {
        CERTCertExtension** extns ;
        int len ;

        for( len = 0, extns = m_pCert->extensions; *extns != nullptr; len ++, extns ++ ) ;
        css::uno::Sequence< css::uno::Reference< css::security::XCertificateExtension > > xExtns( len ) ;
        auto xExtnsRange = asNonConstRange(xExtns);

        for( extns = m_pCert->extensions, len = 0; *extns != nullptr; extns ++, len ++ ) {
            const SECItem id = (*extns)->id;
            OString oidString(CERT_GetOidString(&id));

            bool crit;
            if( (*extns)->critical.data == nullptr )
                crit = false ;
            else
                crit = (*extns)->critical.data[0] == 0xFF;

            // remove "OID." prefix if existing
            OString objID;
            static constexpr std::string_view oid("OID.");
            if (oidString.match(oid))
                objID = oidString.copy(oid.size());
            else
                objID = oidString;

            unsigned char* value = (*extns)->value.data;
            unsigned int vlen = (*extns)->value.len;
            unsigned char* objid = reinterpret_cast<unsigned char *>(const_cast<char *>(objID.getStr()));
            unsigned int objidlen = objID.getLength();

            if (objID == "2.5.29.17")
            {
                rtl::Reference<SanExtensionImpl> pExtn = new SanExtensionImpl;
                pExtn->setCertExtn(value, vlen, objid, objidlen, crit);
                xExtnsRange[len] = pExtn ;
            }
            else
            {
                rtl::Reference<CertificateExtension_XmlSecImpl> pExtn = new CertificateExtension_XmlSecImpl;
                pExtn->setCertExtn(value, vlen, objid, objidlen, crit);
                xExtnsRange[len] = pExtn;
            }
        }

        return xExtns ;
    } else {
        return css::uno::Sequence< css::uno::Reference< css::security::XCertificateExtension > > ();
    }
}

css::uno::Reference< css::security::XCertificateExtension > SAL_CALL X509Certificate_NssImpl::findCertificateExtension( const css::uno::Sequence< sal_Int8 >& oid ) {
    if( m_pCert != nullptr && m_pCert->extensions != nullptr ) {
        CERTCertExtension** extns ;
        SECItem idItem ;

        idItem.data = reinterpret_cast<unsigned char *>(const_cast<sal_Int8 *>(oid.getConstArray()));
        idItem.len = oid.getLength() ;

        css::uno::Reference<css::security::XCertificateExtension> xExtn;
        for( extns = m_pCert->extensions; *extns != nullptr; extns ++ ) {
            if( SECITEM_CompareItem( &idItem, &(*extns)->id ) == SECEqual ) {
                const SECItem id = (*extns)->id;
                OString objId(CERT_GetOidString(&id));

                bool crit;
                if( (*extns)->critical.data == nullptr )
                    crit = false ;
                else
                    crit = (*extns)->critical.data[0] == 0xFF;

                unsigned char* value = (*extns)->value.data;
                unsigned int vlen = (*extns)->value.len;
                unsigned char* objid = (*extns)->id.data;
                unsigned int objidlen = (*extns)->id.len;

                if ( objId == "OID.2.5.29.17" )
                {
                    rtl::Reference<SanExtensionImpl> xSanImpl(
                        new SanExtensionImpl);
                    xSanImpl->setCertExtn(value, vlen, objid, objidlen, crit);
                    xExtn = xSanImpl.get();
                }
                else
                {
                    rtl::Reference<CertificateExtension_XmlSecImpl> xSecImpl(
                        new CertificateExtension_XmlSecImpl);
                    xSecImpl->setCertExtn(value, vlen, objid, objidlen, crit);
                    xExtn = xSecImpl.get();
                }
                break;
            }
        }

        return xExtn;
    } else {
        return nullptr ;
    }
}


css::uno::Sequence< sal_Int8 > SAL_CALL X509Certificate_NssImpl::getEncoded() {
    if( m_pCert != nullptr && m_pCert->derCert.len > 0 ) {
        return comphelper::arrayToSequence<sal_Int8>(m_pCert->derCert.data, m_pCert->derCert.len) ;
    } else {
        return css::uno::Sequence< sal_Int8 >();
    }
}

//Helper methods
void X509Certificate_NssImpl::setCert( CERTCertificate* cert ) {
    if( m_pCert != nullptr ) {
        CERT_DestroyCertificate( m_pCert ) ;
        m_pCert = nullptr ;
    }

    if( cert != nullptr ) {
        m_pCert = CERT_DupCertificate( cert ) ;
    }
}

const CERTCertificate* X509Certificate_NssImpl::getNssCert() const {
    if( m_pCert != nullptr ) {
        return m_pCert ;
    } else {
        return nullptr ;
    }
}

void X509Certificate_NssImpl::setRawCert( const css::uno::Sequence< sal_Int8 >& rawCert ) {
    CERTCertificate* cert ;
    SECItem certItem ;

    certItem.data = reinterpret_cast<unsigned char *>(const_cast<sal_Int8 *>(rawCert.getConstArray()));
    certItem.len = rawCert.getLength() ;

    cert = CERT_DecodeDERCertificate( &certItem, PR_TRUE, nullptr ) ;
    if( cert == nullptr )
        throw css::uno::RuntimeException() ;

    if( m_pCert != nullptr ) {
        CERT_DestroyCertificate( m_pCert ) ;
        m_pCert = nullptr ;
    }

    m_pCert = cert ;
}

SECKEYPrivateKey* X509Certificate_NssImpl::getPrivateKey()
{
    if (m_pCert && m_pCert->slot)
    {
        SECKEYPrivateKey* pPrivateKey = PK11_FindPrivateKeyFromCert(m_pCert->slot, m_pCert, nullptr);
        if (pPrivateKey)
            return pPrivateKey;
        pPrivateKey = PK11_FindKeyByDERCert(m_pCert->slot, m_pCert, nullptr);
        if (pPrivateKey)
        {
            SAL_INFO("xmlsecurity.xmlsec", "fallback from PK11_FindPrivateKeyFromCert to PK11_FindKeyByDERCert needed");
            return pPrivateKey;
        }
        SAL_WARN("xmlsecurity.xmlsec", "X509Certificate_NssImpl::getPrivateKey() cannot find private key");
    }
    return nullptr;
}

static OUString getAlgorithmDescription(SECAlgorithmID const *aid)
{
    SECOidTag tag;
    tag = SECOID_GetAlgorithmTag(aid);

    const char *pDesc = SECOID_FindOIDTagDescription(tag);

    return OUString::createFromAscii( pDesc ) ;
}

static css::uno::Sequence< sal_Int8 > getThumbprint(CERTCertificate const *pCert, SECOidTag id)
{
    if( pCert != nullptr )
    {
        SECStatus rv;
        unsigned char fingerprint[32];
        int length = 0;
        switch (id)
        {
        case SEC_OID_MD5:
            length = MD5_LENGTH;
            break;
        case SEC_OID_SHA1:
            length = SHA1_LENGTH;
            break;
        case SEC_OID_SHA256:
            length = SHA256_LENGTH;
            break;
        default:
            break;
        }

        memset(fingerprint, 0, sizeof fingerprint);
        rv = PK11_HashBuf(id, fingerprint, pCert->derCert.data, pCert->derCert.len);
        if(rv == SECStatus::SECSuccess)
        {
            return comphelper::arrayToSequence<sal_Int8>(fingerprint, length);
        }
    }
    return css::uno::Sequence< sal_Int8 >();
}

OUString SAL_CALL X509Certificate_NssImpl::getSubjectPublicKeyAlgorithm()
{
    if( m_pCert != nullptr )
    {
        return getAlgorithmDescription(&(m_pCert->subjectPublicKeyInfo.algorithm));
    }
    else
    {
        return OUString() ;
    }
}

css::uno::Sequence< sal_Int8 > SAL_CALL X509Certificate_NssImpl::getSubjectPublicKeyValue()
{
    if( m_pCert != nullptr )
    {
        SECItem spk = m_pCert->subjectPublicKeyInfo.subjectPublicKey;
        DER_ConvertBitString(&spk);

        if ( spk.len>0)
        {
            return comphelper::arrayToSequence<sal_Int8>(spk.data, spk.len) ;
        }
    }

    return css::uno::Sequence< sal_Int8 >();
}

OUString SAL_CALL X509Certificate_NssImpl::getSignatureAlgorithm()
{
    if( m_pCert != nullptr )
    {
        return getAlgorithmDescription(&(m_pCert->signature));
    }
    else
    {
        return OUString() ;
    }
}

svl::crypto::SignatureMethodAlgorithm X509Certificate_NssImpl::getSignatureMethodAlgorithm()
{
    svl::crypto::SignatureMethodAlgorithm nRet = svl::crypto::SignatureMethodAlgorithm::RSA;

    if (!m_pCert)
        return nRet;

    SECOidTag eTag = SECOID_GetAlgorithmTag(&m_pCert->subjectPublicKeyInfo.algorithm);
    if (eTag == SEC_OID_ANSIX962_EC_PUBLIC_KEY)
        nRet = svl::crypto::SignatureMethodAlgorithm::ECDSA;

    return nRet;
}

css::uno::Sequence< sal_Int8 > SAL_CALL X509Certificate_NssImpl::getSHA1Thumbprint()
{
    return getThumbprint(m_pCert, SEC_OID_SHA1);
}

css::uno::Sequence<sal_Int8> X509Certificate_NssImpl::getSHA256Thumbprint()
{
    return getThumbprint(m_pCert, SEC_OID_SHA256);
}

css::uno::Sequence< sal_Int8 > SAL_CALL X509Certificate_NssImpl::getMD5Thumbprint()
{
    return getThumbprint(m_pCert, SEC_OID_MD5);
}

css::security::CertificateKind SAL_CALL X509Certificate_NssImpl::getCertificateKind()
{
    return css::security::CertificateKind_X509;
}

sal_Int32 SAL_CALL X509Certificate_NssImpl::getCertificateUsage(  )
{
    SECStatus rv;
    SECItem tmpitem;
    sal_Int32 usage;

    rv = CERT_FindKeyUsageExtension(m_pCert, &tmpitem);
    if ( rv == SECStatus::SECSuccess )
    {
        usage = tmpitem.data[0];
        PORT_Free(tmpitem.data);
        tmpitem.data = nullptr;
    }
    else
    {
        usage = KU_ALL;
    }

    /*
     * to make the nss implementation compatible with MSCrypto,
     * the following usage is ignored
     *
     *
    if ( CERT_GovtApprovedBitSet(m_pCert) )
    {
        usage |= KU_NS_GOVT_APPROVED;
    }
    */

    return usage;
}

/* XServiceInfo */
OUString SAL_CALL X509Certificate_NssImpl::getImplementationName()
{
    return u"com.sun.star.xml.security.gpg.XCertificate_NssImpl"_ustr;
}

/* XServiceInfo */
sal_Bool SAL_CALL X509Certificate_NssImpl::supportsService(const OUString& serviceName)
{
    return cppu::supportsService(this, serviceName);
}

/* XServiceInfo */
css::uno::Sequence<OUString> SAL_CALL X509Certificate_NssImpl::getSupportedServiceNames() { return { OUString() }; }

namespace xmlsecurity {

// based on some guesswork and:
// https://datatracker.ietf.org/doc/html/rfc1485
// https://docs.microsoft.com/en-us/windows/win32/api/wincrypt/nf-wincrypt-certnametostra#CERT_X500_NAME_STR
// the main problem appears to be that in values " is escaped as "" vs. \"
static OUString CompatDNCryptoAPI(std::u16string_view rDN)
{
    OUStringBuffer buf(rDN.size());
    enum { DEFAULT, INVALUE, INQUOTE } state(DEFAULT);
    for (size_t i = 0; i < rDN.size(); ++i)
    {
        if (state == DEFAULT)
        {
            buf.append(rDN[i]);
            if (rDN[i] == '=')
            {
                if (rDN.size() == i+1)
                {
                    break; // invalid?
                }
                else if (rDN[i+1] == '"')
                {
                    buf.append(rDN[i+1]);
                    ++i;
                    state = INQUOTE;
                }
                else
                {
                    state = INVALUE;
                }
            }
        }
        else if (state == INVALUE)
        {
            if (rDN[i] == '+' || rDN[i] == ',' || rDN[i] == ';')
            {
                state = DEFAULT;
            }
            buf.append(rDN[i]);
        }
        else
        {
            assert(state == INQUOTE);
            if (rDN[i] == '"')
            {
                if (rDN.size() != i+1 && rDN[i+1] == '"')
                {
                    buf.append(OUString::Concat("\\") + OUStringChar(rDN[i+1]));
                    ++i;
                }
                else
                {
                    buf.append(rDN[i]);
                    state = DEFAULT;
                }
            }
            else
            {
                buf.append(rDN[i]);
            }
        }
    }
    return buf.makeStringAndClear();
}

bool EqualDistinguishedNames(
        std::u16string_view const rName1, std::u16string_view const rName2,
        EqualMode const eMode)
{
    if (eMode == COMPAT_BOTH && !rName1.empty() && rName1 == rName2)
    {   // handle case where both need to be converted
        return true;
    }
    CERTName *const pName1(CERT_AsciiToName(OUStringToOString(rName1, RTL_TEXTENCODING_UTF8).getStr()));
    if (pName1 == nullptr)
    {
        return false;
    }
    CERTName *const pName2(CERT_AsciiToName(OUStringToOString(rName2, RTL_TEXTENCODING_UTF8).getStr()));
    bool ret(false);
    if (pName2)
    {
        ret = (CERT_CompareName(pName1, pName2) == SECEqual);
        CERT_DestroyName(pName2);
    }
    if (!ret && eMode == COMPAT_2ND)
    {
        CERTName *const pName2Compat(CERT_AsciiToName(OUStringToOString(
            CompatDNCryptoAPI(rName2), RTL_TEXTENCODING_UTF8).getStr()));
        if (pName2Compat == nullptr)
        {
            CERT_DestroyName(pName1);
            return false;
        }
        ret = CERT_CompareName(pName1, pName2Compat) == SECEqual;
        CERT_DestroyName(pName2Compat);
    }
    CERT_DestroyName(pName1);
    return ret;
}

} // namespace xmlsecurity

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
