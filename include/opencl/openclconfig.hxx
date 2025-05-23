/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_OPENCL_OPENCLCONFIG_HXX
#define INCLUDED_OPENCL_OPENCLCONFIG_HXX

#include <ostream>
#include <set>

#include <opencl/opencldllapi.h>
#include <rtl/ustring.hxx>
#include <utility>

struct OpenCLDeviceInfo;
struct OpenCLPlatformInfo;

struct OPENCL_DLLPUBLIC OpenCLConfig
{
    struct ImplMatcher
    {
        OUString maOS;
        OUString maOSVersion;
        OUString maPlatformVendor;
        OUString maDevice;
        OUString maDriverVersion;

        ImplMatcher()
        {
        }

        ImplMatcher(OUString aOS,
                    OUString aOSVersion,
                    OUString aPlatformVendor,
                    OUString aDevice,
                    OUString aDriverVersion)
            : maOS(std::move(aOS)),
              maOSVersion(std::move(aOSVersion)),
              maPlatformVendor(std::move(aPlatformVendor)),
              maDevice(std::move(aDevice)),
              maDriverVersion(std::move(aDriverVersion))
        {
        }

        bool operator==(const ImplMatcher& r) const
        {
            return maOS == r.maOS &&
                   maOSVersion == r.maOSVersion &&
                   maPlatformVendor == r.maPlatformVendor &&
                   maDevice == r.maDevice &&
                   maDriverVersion == r.maDriverVersion;
        }
        bool operator!=(const ImplMatcher& r) const
        {
            return !operator==(r);
        }
        bool operator<(const ImplMatcher& r) const
        {
            return (maOS < r.maOS ||
                    (maOS == r.maOS &&
                     (maOSVersion < r.maOSVersion ||
                      (maOSVersion == r.maOSVersion &&
                       (maPlatformVendor < r.maPlatformVendor ||
                        (maPlatformVendor == r.maPlatformVendor &&
                         (maDevice < r.maDevice ||
                          (maDevice == r.maDevice &&
                           (maDriverVersion < r.maDriverVersion)))))))));
        }
    };

    bool mbUseOpenCL;

    typedef std::set<ImplMatcher> ImplMatcherSet;

    ImplMatcherSet maDenyList;
    ImplMatcherSet maAllowList;

    OpenCLConfig();

    bool operator== (const OpenCLConfig& r) const;
    bool operator!= (const OpenCLConfig& r) const;

    static OpenCLConfig get();

    void set();

    bool checkImplementation(const OpenCLPlatformInfo& rPlatform, const OpenCLDeviceInfo& rDevice) const;
};

OPENCL_DLLPUBLIC std::ostream& operator<<(std::ostream& rStream, const OpenCLConfig& rConfig);
OPENCL_DLLPUBLIC std::ostream& operator<<(std::ostream& rStream, const OpenCLConfig::ImplMatcher& rImpl);
OPENCL_DLLPUBLIC std::ostream& operator<<(std::ostream& rStream, const OpenCLConfig::ImplMatcherSet& rSet);

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
