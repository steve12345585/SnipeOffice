/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "exceldetect.hxx"

#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <cppuhelper/supportsservice.hxx>

#include <sfx2/docfile.hxx>
#include <unotools/mediadescriptor.hxx>
#include <sot/storage.hxx>
#include <comphelper/diagnose_ex.hxx>

using namespace com::sun::star;
using utl::MediaDescriptor;

ScExcelBiffDetect::ScExcelBiffDetect() {}
ScExcelBiffDetect::~ScExcelBiffDetect() {}

OUString ScExcelBiffDetect::getImplementationName()
{
    return u"com.sun.star.comp.calc.ExcelBiffFormatDetector"_ustr;
}

sal_Bool ScExcelBiffDetect::supportsService( const OUString& aName )
{
    return cppu::supportsService(this, aName);
}

uno::Sequence<OUString> ScExcelBiffDetect::getSupportedServiceNames()
{
    return { u"com.sun.star.frame.ExtendedTypeDetection"_ustr };
}

namespace {

bool hasStream(const uno::Reference<io::XInputStream>& xInStream, const OUString& rName)
{
    SfxMedium aMedium;
    aMedium.UseInteractionHandler(false);
    aMedium.setStreamToLoadFrom(xInStream, true);
    SvStream* pStream = aMedium.GetInStream();
    if (!pStream)
        return false;

    sal_uInt64 const nSize = pStream->TellEnd();
    pStream->Seek(0);

    if (!nSize)
    {
        // 0-size stream.  Failed.
        return false;
    }

    try
    {
        rtl::Reference<SotStorage> xStorage = new SotStorage(pStream, false);
        if (!xStorage.is() || xStorage->GetError())
            return false;
        return xStorage->IsStream(rName);
    }
    catch (const css::ucb::ContentCreationException &)
    {
        TOOLS_WARN_EXCEPTION("sc", "hasStream");
    }

    return false;
}

/**
 * We detect BIFF 2, 3 and 4 file types together since the only thing that
 * set them apart is the BOF ID.
 */
bool isExcel40(const uno::Reference<io::XInputStream>& xInStream)
{
    SfxMedium aMedium;
    aMedium.UseInteractionHandler(false);
    aMedium.setStreamToLoadFrom(xInStream, true);
    SvStream* pStream = aMedium.GetInStream();
    if (!pStream)
        return false;

    sal_uInt64 const nSize = pStream->TellEnd();
    pStream->Seek(0);

    if (nSize < 4)
        return false;

    sal_uInt16 nBofId, nBofSize;
    pStream->ReadUInt16( nBofId ).ReadUInt16( nBofSize );

    switch (nBofId)
    {
        case 0x0009: // Excel 2.1 worksheet (BIFF 2)
        case 0x0209: // Excel 3.0 worksheet (BIFF 3)
        case 0x0409: // Excel 4.0 worksheet (BIFF 4)
        case 0x0809: // Excel 5.0 worksheet (BIFF 5), some apps create such files (fdo#70100)
            break;
        default:
            return false;
    }

    if (nBofSize < 4 || 16 < nBofSize)
        // BOF record must be sized between 4 and 16 for BIFF 2, 3 and 4.
        return false;

    sal_uInt64 const nPos = pStream->Tell();
    if (nSize - nPos < nBofSize)
        // BOF record doesn't have required bytes.
        return false;

    return true;
}

bool isTemplate(std::u16string_view rType)
{
    return rType.find(u"_VorlageTemplate") != std::u16string_view::npos;
}

}

OUString ScExcelBiffDetect::detect( uno::Sequence<beans::PropertyValue>& lDescriptor )
{
    MediaDescriptor aMediaDesc(lDescriptor);
    OUString aType;
    aMediaDesc[MediaDescriptor::PROP_TYPENAME] >>= aType;
    if (aType.isEmpty())
        // Type is not given.  We can't proceed.
        return OUString();

    aMediaDesc.addInputStream();
    uno::Reference<io::XInputStream> xInStream(aMediaDesc[MediaDescriptor::PROP_INPUTSTREAM], uno::UNO_QUERY);
    if (!xInStream.is())
        // No input stream.
        return OUString();

    if (aType == "calc_MS_Excel_97" || aType == "calc_MS_Excel_97_VorlageTemplate")
    {
        // See if this stream is an Excel 97/XP/2003 (BIFF8) stream.
        if (!hasStream(xInStream, u"Workbook"_ustr))
            // BIFF8 is expected to contain a stream named "Workbook".
            return OUString();

        aMediaDesc[MediaDescriptor::PROP_FILTERNAME] <<= isTemplate(aType) ? u"MS Excel 97 Vorlage/Template"_ustr : u"MS Excel 97"_ustr;
    }

    else if (aType == "calc_MS_Excel_95" || aType == "calc_MS_Excel_95_VorlageTemplate")
    {
        // See if this stream is an Excel 95 (BIFF5) stream.
        if (!hasStream(xInStream, u"Book"_ustr))
            return OUString();

        aMediaDesc[MediaDescriptor::PROP_FILTERNAME] <<= isTemplate(aType) ? u"MS Excel 95 Vorlage/Template"_ustr : u"MS Excel 95"_ustr;
    }

    else if (aType == "calc_MS_Excel_5095" || aType == "calc_MS_Excel_5095_VorlageTemplate")
    {
        // See if this stream is an Excel 5.0/95 stream.
        if (!hasStream(xInStream, u"Book"_ustr))
            return OUString();

        aMediaDesc[MediaDescriptor::PROP_FILTERNAME] <<= isTemplate(aType) ? u"MS Excel 5.0/95 Vorlage/Template"_ustr : u"MS Excel 5.0/95"_ustr;
    }

    else if (aType == "calc_MS_Excel_40" || aType == "calc_MS_Excel_40_VorlageTemplate")
    {
        // See if this stream is an Excel 4.0 stream.
        if (!isExcel40(xInStream))
            return OUString();

        aMediaDesc[MediaDescriptor::PROP_FILTERNAME] <<= isTemplate(aType) ? u"MS Excel 4.0 Vorlage/Template"_ustr : u"MS Excel 4.0"_ustr;
    }

    else
        // Nothing to detect.
        return OUString();

    aMediaDesc >> lDescriptor;
    return aType;
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_calc_ExcelBiffFormatDetector_get_implementation(css::uno::XComponentContext* /*context*/,
                                                                  css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new ScExcelBiffDetect);
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
