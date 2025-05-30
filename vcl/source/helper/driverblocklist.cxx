/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <driverblocklist.hxx>

#include <algorithm>
#include <string_view>

#include <sal/log.hxx>
#include <utility>

#ifdef _WIN32
#if !defined WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace DriverBlocklist
{
static OperatingSystem getOperatingSystem(std::string_view rString)
{
    if (rString == "all")
        return DRIVER_OS_ALL;
    else if (rString == "10")
        return DRIVER_OS_WINDOWS_10;
    else if (rString == "windows")
        return DRIVER_OS_WINDOWS_ALL;
    else if (rString == "linux")
        return DRIVER_OS_LINUX;
    else if (rString == "osx_10_5")
        return DRIVER_OS_OSX_10_5;
    else if (rString == "osx_10_6")
        return DRIVER_OS_OSX_10_6;
    else if (rString == "osx_10_7")
        return DRIVER_OS_OSX_10_7;
    else if (rString == "osx_10_8")
        return DRIVER_OS_OSX_10_8;
    else if (rString == "osx")
        return DRIVER_OS_OSX_ALL;
    else if (rString == "android")
        return DRIVER_OS_ANDROID;
    return DRIVER_OS_UNKNOWN;
}

static VersionComparisonOp getComparison(std::string_view rString)
{
    if (rString == "less")
    {
        return DRIVER_LESS_THAN;
    }
    else if (rString == "less_equal")
    {
        return DRIVER_LESS_THAN_OR_EQUAL;
    }
    else if (rString == "greater")
    {
        return DRIVER_GREATER_THAN;
    }
    else if (rString == "greater_equal")
    {
        return DRIVER_GREATER_THAN_OR_EQUAL;
    }
    else if (rString == "equal")
    {
        return DRIVER_EQUAL;
    }
    else if (rString == "not_equal")
    {
        return DRIVER_NOT_EQUAL;
    }
    else if (rString == "between_exclusive")
    {
        return DRIVER_BETWEEN_EXCLUSIVE;
    }
    else if (rString == "between_inclusive")
    {
        return DRIVER_BETWEEN_INCLUSIVE;
    }
    else if (rString == "between_inclusive_start")
    {
        return DRIVER_BETWEEN_INCLUSIVE_START;
    }

    throw InvalidFileException();
}

static OUString GetVendorId(std::string_view rString)
{
    if (rString == "all")
    {
        return u""_ustr;
    }
    else if (rString == "intel")
    {
        return u"0x8086"_ustr;
    }
    else if (rString == "nvidia")
    {
        return u"0x10de"_ustr;
    }
    else if (rString == "amd")
    {
        return u"0x1002"_ustr;
    }
    else if (rString == "microsoft")
    {
        return u"0x1414"_ustr;
    }
    else
    {
        // Allow having simply the hex number as such there, too.
        return OStringToOUString(rString, RTL_TEXTENCODING_UTF8);
    }
}

OUString GetVendorId(DeviceVendor id)
{
    assert(id >= 0 && id < DeviceVendorMax);

    switch (id)
    {
        case VendorAll:
            return u""_ustr;
        case VendorIntel:
            return u"0x8086"_ustr;
        case VendorNVIDIA:
            return u"0x10de"_ustr;
        case VendorAMD:
            return u"0x1002"_ustr;
        case VendorMicrosoft:
            return u"0x1414"_ustr;
    }
    abort();
}

DeviceVendor GetVendorFromId(uint32_t id)
{
    switch (id)
    {
        case 0x8086:
            return VendorIntel;
        case 0x10de:
            return VendorNVIDIA;
        case 0x1002:
            return VendorAMD;
        case 0x1414:
            return VendorMicrosoft;
        default:
            return VendorAll;
    }
}

std::string_view GetVendorNameFromId(uint32_t id)
{
    switch (id)
    {
        case 0x8086:
            return "Intel";
        case 0x10de:
            return "Nvidia";
        case 0x1002:
            return "AMD";
        case 0x1414:
            return "Microsoft";
        default:
            return "?";
    }
}

Parser::Parser(OUString aURL, std::vector<DriverInfo>& rDriverList, VersionType versionType)
    : meBlockType(BlockType::UNKNOWN)
    , mrDriverList(rDriverList)
    , maURL(std::move(aURL))
    , mVersionType(versionType)
{
}

bool Parser::parse()
{
    try
    {
        xmlreader::XmlReader aReader(maURL);
        handleContent(aReader);
    }
    catch (...)
    {
        mrDriverList.clear();
        return false;
    }
    return true;
}

// This allows us to pad driver version 'substrings' with 0s, this
// effectively allows us to treat the version numbers as 'decimals'. This is
// a little strange but this method seems to do the right thing for all
// different vendor's driver strings. i.e. .98 will become 9800, which is
// larger than .978 which would become 9780.
static void PadDriverDecimal(char* aString)
{
    for (int i = 0; i < 4; i++)
    {
        if (!aString[i])
        {
            for (int c = i; c < 4; c++)
            {
                aString[c] = '0';
            }
            break;
        }
    }
    aString[4] = 0;
}

// All destination string storage needs to have at least 5 bytes available.
static bool SplitDriverVersion(const char* aSource, char* aAStr, char* aBStr, char* aCStr,
                               char* aDStr, VersionType versionType)
{
    // sscanf doesn't do what we want here to we parse this manually.
    int len = strlen(aSource);
    char* dest[4] = { aAStr, aBStr, aCStr, aDStr };
    unsigned destIdx = 0;
    unsigned destPos = 0;

    for (int i = 0; i < len; i++)
    {
        if (destIdx >= SAL_N_ELEMENTS(dest))
        {
            // Invalid format found. Ensure we don't access dest beyond bounds.
            return false;
        }

        if (aSource[i] == '.')
        {
            dest[destIdx++][destPos] = 0;
            destPos = 0;
            continue;
        }

        if (destPos > 3)
        {
            // Ignore more than 4 chars. Ensure we never access dest[destIdx]
            // beyond its bounds.
            continue;
        }

        dest[destIdx][destPos++] = aSource[i];
    }

    // Add last terminator.
    dest[destIdx][destPos] = 0;

    // Vulkan version numbers have only 3 fields.
    if (versionType == VersionType::Vulkan && destIdx == SAL_N_ELEMENTS(dest) - 2)
        dest[++destIdx][0] = '\0';
    if (destIdx != SAL_N_ELEMENTS(dest) - 1)
    {
        return false;
    }
    return true;
}

static bool ParseDriverVersion(std::u16string_view aVersion, uint64_t& rNumericVersion,
                               VersionType versionType)
{
    rNumericVersion = 0;

    int a, b, c, d;
    char aStr[8], bStr[8], cStr[8], dStr[8];
    /* honestly, why do I even bother */
    OString aOVersion = OUStringToOString(aVersion, RTL_TEXTENCODING_UTF8);
    if (!SplitDriverVersion(aOVersion.getStr(), aStr, bStr, cStr, dStr, versionType))
        return false;

    if (versionType == VersionType::OpenGL)
    {
        PadDriverDecimal(bStr);
        PadDriverDecimal(cStr);
        PadDriverDecimal(dStr);
    }

    a = atoi(aStr);
    b = atoi(bStr);
    c = atoi(cStr);
    d = atoi(dStr);

    if (versionType == VersionType::Vulkan)
        assert(d == 0);

    if (a < 0 || a > 0xffff)
        return false;
    if (b < 0 || b > 0xffff)
        return false;
    if (c < 0 || c > 0xffff)
        return false;
    if (d < 0 || d > 0xffff)
        return false;

    rNumericVersion = GFX_DRIVER_VERSION(a, b, c, d);
    return true;
}

uint64_t Parser::getVersion(std::string_view rString)
{
    OUString aString = OStringToOUString(rString, RTL_TEXTENCODING_UTF8);
    uint64_t nVersion;
    bool bResult = ParseDriverVersion(aString, nVersion, mVersionType);

    if (!bResult)
    {
        throw InvalidFileException();
    }

    return nVersion;
}

void Parser::handleDevices(DriverInfo& rDriver, xmlreader::XmlReader& rReader)
{
    int nLevel = 1;
    bool bInMsg = false;

    while (true)
    {
        xmlreader::Span name;
        int nsId;

        xmlreader::XmlReader::Result res
            = rReader.nextItem(xmlreader::XmlReader::Text::Normalized, &name, &nsId);

        if (res == xmlreader::XmlReader::Result::Begin)
        {
            ++nLevel;
            if (nLevel > 2)
                throw InvalidFileException();

            if (name == "msg")
            {
                bInMsg = true;
            }
            else if (name == "device")
            {
                int nsIdDeveice;
                while (rReader.nextAttribute(&nsIdDeveice, &name))
                {
                    if (name == "id")
                    {
                        name = rReader.getAttributeValue(false);
                        OString aDeviceId(name.begin, name.length);
                        rDriver.maDevices.push_back(
                            OStringToOUString(aDeviceId, RTL_TEXTENCODING_UTF8));
                    }
                }
            }
            else
                throw InvalidFileException();
        }
        else if (res == xmlreader::XmlReader::Result::End)
        {
            --nLevel;
            bInMsg = false;
            if (!nLevel)
                break;
        }
        else if (res == xmlreader::XmlReader::Result::Text)
        {
            if (bInMsg)
            {
                OString sMsg(name.begin, name.length);
                rDriver.maMsg = OStringToOUString(sMsg, RTL_TEXTENCODING_UTF8);
            }
        }
    }
}

void Parser::handleEntry(DriverInfo& rDriver, xmlreader::XmlReader& rReader)
{
    if (meBlockType == BlockType::ALLOWLIST)
    {
        rDriver.mbAllowlisted = true;
    }
    else if (meBlockType == BlockType::DENYLIST)
    {
        rDriver.mbAllowlisted = false;
    }
    else if (meBlockType == BlockType::UNKNOWN)
    {
        throw InvalidFileException();
    }

    xmlreader::Span name;
    int nsId;

    while (rReader.nextAttribute(&nsId, &name))
    {
        if (name == "os")
        {
            name = rReader.getAttributeValue(false);
            OString sOS(name.begin, name.length);
            rDriver.meOperatingSystem = getOperatingSystem(sOS);
        }
        else if (name == "vendor")
        {
            name = rReader.getAttributeValue(false);
            OString sVendor(name.begin, name.length);
            rDriver.maAdapterVendor = GetVendorId(sVendor);
        }
        else if (name == "compare")
        {
            name = rReader.getAttributeValue(false);
            OString sCompare(name.begin, name.length);
            rDriver.meComparisonOp = getComparison(sCompare);
        }
        else if (name == "version")
        {
            name = rReader.getAttributeValue(false);
            OString sVersion(name.begin, name.length);
            rDriver.mnDriverVersion = getVersion(sVersion);
        }
        else if (name == "minVersion")
        {
            name = rReader.getAttributeValue(false);
            OString sMinVersion(name.begin, name.length);
            rDriver.mnDriverVersion = getVersion(sMinVersion);
        }
        else if (name == "maxVersion")
        {
            name = rReader.getAttributeValue(false);
            OString sMaxVersion(name.begin, name.length);
            rDriver.mnDriverVersionMax = getVersion(sMaxVersion);
        }
        else
        {
            OString aAttrName(name.begin, name.length);
            SAL_WARN("vcl.driver", "unsupported attribute: " << aAttrName);
        }
    }

    handleDevices(rDriver, rReader);
}

void Parser::handleList(xmlreader::XmlReader& rReader)
{
    xmlreader::Span name;
    int nsId;

    while (true)
    {
        xmlreader::XmlReader::Result res
            = rReader.nextItem(xmlreader::XmlReader::Text::NONE, &name, &nsId);

        if (res == xmlreader::XmlReader::Result::Begin)
        {
            if (name == "entry")
            {
                DriverInfo aDriver;
                handleEntry(aDriver, rReader);
                mrDriverList.push_back(aDriver);
            }
            else if (name == "entryRange")
            {
                DriverInfo aDriver;
                handleEntry(aDriver, rReader);
                mrDriverList.push_back(aDriver);
            }
            else
            {
                throw InvalidFileException();
            }
        }
        else if (res == xmlreader::XmlReader::Result::End)
        {
            break;
        }
    }
}

void Parser::handleContent(xmlreader::XmlReader& rReader)
{
    while (true)
    {
        xmlreader::Span name;
        int nsId;

        xmlreader::XmlReader::Result res
            = rReader.nextItem(xmlreader::XmlReader::Text::NONE, &name, &nsId);

        if (res == xmlreader::XmlReader::Result::Begin)
        {
            if (name == "allowlist")
            {
                meBlockType = BlockType::ALLOWLIST;
                handleList(rReader);
            }
            else if (name == "denylist")
            {
                meBlockType = BlockType::DENYLIST;
                handleList(rReader);
            }
            else if (name == "root")
            {
            }
            else
            {
                throw InvalidFileException();
            }
        }
        else if (res == xmlreader::XmlReader::Result::End)
        {
            if (name == "allowlist" || name == "denylist")
            {
                meBlockType = BlockType::UNKNOWN;
            }
        }
        else if (res == xmlreader::XmlReader::Result::Done)
        {
            break;
        }
    }
}

static OperatingSystem getOperatingSystem()
{
#ifdef _WIN32
    // OS version in 16.16 major/minor form
    // based on http://msdn.microsoft.com/en-us/library/ms724834(VS.85).aspx
    switch (DriverBlocklist::GetWindowsVersion())
    {
        case 0x000A0000: // Major 10 Minor 0
            return DRIVER_OS_WINDOWS_10;
        default:
            return DRIVER_OS_UNKNOWN;
    }
#elif defined LINUX
    return DRIVER_OS_LINUX;
#else
    return DRIVER_OS_UNKNOWN;
#endif
}

namespace
{
struct compareIgnoreAsciiCase
{
    explicit compareIgnoreAsciiCase(OUString aString)
        : maString(std::move(aString))
    {
    }

    bool operator()(std::u16string_view rCompare)
    {
        return maString.equalsIgnoreAsciiCase(rCompare);
    }

private:
    OUString maString;
};
}

const uint64_t allDriverVersions = ~(uint64_t(0));

DriverInfo::DriverInfo()
    : meOperatingSystem(DRIVER_OS_UNKNOWN)
    , maAdapterVendor(GetVendorId(VendorAll))
    , mbAllowlisted(false)
    , meComparisonOp(DRIVER_COMPARISON_IGNORED)
    , mnDriverVersion(0)
    , mnDriverVersionMax(0)
{
}

DriverInfo::DriverInfo(OperatingSystem os, OUString vendor, VersionComparisonOp op,
                       uint64_t driverVersion, bool bAllowlisted,
                       const char* suggestedVersion /* = nullptr */)
    : meOperatingSystem(os)
    , maAdapterVendor(std::move(vendor))
    , mbAllowlisted(bAllowlisted)
    , meComparisonOp(op)
    , mnDriverVersion(driverVersion)
    , mnDriverVersionMax(0)
{
    if (suggestedVersion)
        maSuggestedVersion
            = OStringToOUString(std::string_view(suggestedVersion), RTL_TEXTENCODING_UTF8);
}

bool FindBlocklistedDeviceInList(std::vector<DriverInfo>& aDeviceInfos, VersionType versionType,
                                 std::u16string_view sDriverVersion,
                                 std::u16string_view sAdapterVendorID,
                                 OUString const& sAdapterDeviceID, OperatingSystem system,
                                 const OUString& blocklistURL)
{
    uint64_t driverVersion;
    ParseDriverVersion(sDriverVersion, driverVersion, versionType);

    bool match = false;
    for (const auto& rDeviceInfo : aDeviceInfos)
    {
        bool osMatch = false;
        if (rDeviceInfo.meOperatingSystem == DRIVER_OS_ALL)
            osMatch = true;
        else if (rDeviceInfo.meOperatingSystem == system)
            osMatch = true;
        else if (rDeviceInfo.meOperatingSystem == DRIVER_OS_WINDOWS_ALL
                 && system >= DRIVER_OS_WINDOWS_FIRST && system <= DRIVER_OS_WINDOWS_LAST)
            osMatch = true;
        else if (rDeviceInfo.meOperatingSystem == DRIVER_OS_OSX_ALL && system >= DRIVER_OS_OSX_FIRST
                 && system <= DRIVER_OS_OSX_LAST)
            osMatch = true;
        if (!osMatch)
        {
            continue;
        }

        if (!rDeviceInfo.maAdapterVendor.equalsIgnoreAsciiCase(GetVendorId(VendorAll))
            && !rDeviceInfo.maAdapterVendor.equalsIgnoreAsciiCase(sAdapterVendorID))
        {
            continue;
        }

        if (std::none_of(rDeviceInfo.maDevices.begin(), rDeviceInfo.maDevices.end(),
                         compareIgnoreAsciiCase(u"all"_ustr))
            && std::none_of(rDeviceInfo.maDevices.begin(), rDeviceInfo.maDevices.end(),
                            compareIgnoreAsciiCase(sAdapterDeviceID)))
        {
            continue;
        }

        switch (rDeviceInfo.meComparisonOp)
        {
            case DRIVER_LESS_THAN:
                match = driverVersion < rDeviceInfo.mnDriverVersion;
                break;
            case DRIVER_LESS_THAN_OR_EQUAL:
                match = driverVersion <= rDeviceInfo.mnDriverVersion;
                break;
            case DRIVER_GREATER_THAN:
                match = driverVersion > rDeviceInfo.mnDriverVersion;
                break;
            case DRIVER_GREATER_THAN_OR_EQUAL:
                match = driverVersion >= rDeviceInfo.mnDriverVersion;
                break;
            case DRIVER_EQUAL:
                match = driverVersion == rDeviceInfo.mnDriverVersion;
                break;
            case DRIVER_NOT_EQUAL:
                match = driverVersion != rDeviceInfo.mnDriverVersion;
                break;
            case DRIVER_BETWEEN_EXCLUSIVE:
                match = driverVersion > rDeviceInfo.mnDriverVersion
                        && driverVersion < rDeviceInfo.mnDriverVersionMax;
                break;
            case DRIVER_BETWEEN_INCLUSIVE:
                match = driverVersion >= rDeviceInfo.mnDriverVersion
                        && driverVersion <= rDeviceInfo.mnDriverVersionMax;
                break;
            case DRIVER_BETWEEN_INCLUSIVE_START:
                match = driverVersion >= rDeviceInfo.mnDriverVersion
                        && driverVersion < rDeviceInfo.mnDriverVersionMax;
                break;
            case DRIVER_COMPARISON_IGNORED:
                // We don't have a comparison op, so we match everything.
                match = true;
                break;
            default:
                SAL_WARN("vcl.driver", "Bogus op in " << blocklistURL);
                break;
        }

        if (match || rDeviceInfo.mnDriverVersion == allDriverVersions)
        {
            // white listed drivers
            if (rDeviceInfo.mbAllowlisted)
            {
                SAL_INFO("vcl.driver", "allowlisted driver");
                return false;
            }

            match = true;
            if (!rDeviceInfo.maSuggestedVersion.isEmpty())
            {
                SAL_WARN("vcl.driver", "use : " << rDeviceInfo.maSuggestedVersion);
            }
            break;
        }
    }

    SAL_INFO("vcl.driver", (match ? "denylisted" : "not denylisted") << " in " << blocklistURL);
    return match;
}

bool IsDeviceBlocked(const OUString& blocklistURL, VersionType versionType,
                     std::u16string_view driverVersion, std::u16string_view vendorId,
                     const OUString& deviceId)
{
    std::vector<DriverInfo> driverList;
    Parser parser(blocklistURL, driverList, versionType);
    if (!parser.parse())
    {
        SAL_WARN("vcl.driver", "error parsing denylist " << blocklistURL);
        return false;
    }
    return FindBlocklistedDeviceInList(driverList, versionType, driverVersion, vendorId, deviceId,
                                       getOperatingSystem(), blocklistURL);
}

#ifdef _WIN32
int32_t GetWindowsVersion()
{
    static int32_t winVersion = []() {
        // GetVersion(Ex) and VersionHelpers (based on VerifyVersionInfo) API are
        // subject to manifest-based behavior since Windows 8.1, so give wrong results.
        // Another approach would be to use NetWkstaGetInfo, but that has some small
        // reported delays (some milliseconds), and might get slower in domains with
        // poor network connections.
        // So go with a solution described at https://msdn.microsoft.com/en-us/library/ms724429
        HINSTANCE hLibrary = LoadLibraryW(L"kernel32.dll");
        if (hLibrary != nullptr)
        {
            wchar_t szPath[MAX_PATH];
            DWORD dwCount = GetModuleFileNameW(hLibrary, szPath, SAL_N_ELEMENTS(szPath));
            FreeLibrary(hLibrary);
            if (dwCount != 0 && dwCount < SAL_N_ELEMENTS(szPath))
            {
                dwCount = GetFileVersionInfoSizeW(szPath, nullptr);
                if (dwCount != 0)
                {
                    std::unique_ptr<char[]> ver(new char[dwCount]);
                    if (GetFileVersionInfoW(szPath, 0, dwCount, ver.get()) != FALSE)
                    {
                        void* pBlock = nullptr;
                        UINT dwBlockSz = 0;
                        if (VerQueryValueW(ver.get(), L"\\", &pBlock, &dwBlockSz) != FALSE
                            && dwBlockSz >= sizeof(VS_FIXEDFILEINFO))
                        {
                            VS_FIXEDFILEINFO* vinfo = static_cast<VS_FIXEDFILEINFO*>(pBlock);
                            return int32_t(vinfo->dwProductVersionMS);
                        }
                    }
                }
            }
        }
        return 0;
    }();

    return winVersion;
}
#endif

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
