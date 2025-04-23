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

#include "connpoolconfig.hxx"
#include "connpoolsettings.hxx"

#include <svl/itemset.hxx>
#include <svx/databaseregistrationui.hxx>
#include <unotools/confignode.hxx>
#include <svl/eitem.hxx>
#include <comphelper/processfactory.hxx>
#include "sdbcdriverenum.hxx"

#include <com/sun/star/uno/Sequence.hxx>

namespace offapp
{


    using namespace ::utl;
    using namespace ::com::sun::star::uno;

    constexpr OUString CONNECTIONPOOL_NODENAME = u"org.openoffice.Office.DataAccess/ConnectionPool"_ustr;
    constexpr OUString ENABLE_POOLING = u"EnablePooling"_ustr;
    constexpr OUString DRIVER_SETTINGS = u"DriverSettings"_ustr;
    constexpr OUString DRIVER_NAME = u"DriverName"_ustr;
    constexpr OUString ENABLE = u"Enable"_ustr;
    constexpr OUString TIMEOUT = u"Timeout"_ustr;

    void ConnectionPoolConfig::GetOptions(SfxItemSet& _rFillItems)
    {
        // the config node where all pooling relevant info are stored under
        OConfigurationTreeRoot aConnectionPoolRoot = OConfigurationTreeRoot::createWithComponentContext(
            ::comphelper::getProcessComponentContext(), CONNECTIONPOOL_NODENAME, -1, OConfigurationTreeRoot::CM_READONLY);

        // the global "enabled" flag
        Any aEnabled = aConnectionPoolRoot.getNodeValue(ENABLE_POOLING);
        bool bEnabled = true;
        aEnabled >>= bEnabled;
        _rFillItems.Put(SfxBoolItem(SID_SB_POOLING_ENABLED, bEnabled));

        // the settings for the single drivers
        DriverPoolingSettings aSettings;
        // first get all the drivers register at the driver manager
        ODriverEnumeration aEnumDrivers;
        for (auto const& elem : aEnumDrivers)
        {
            aSettings.push_back(DriverPooling(elem));
        }

        // then look for which of them settings are stored in the configuration
        OConfigurationNode aDriverSettings = aConnectionPoolRoot.openNode(DRIVER_SETTINGS);

        for (auto& driverKey : aDriverSettings.getNodeNames())
        {
            // the name of the driver in this round
            OConfigurationNode aThisDriverSettings = aDriverSettings.openNode(driverKey);
            OUString sThisDriverName;
            aThisDriverSettings.getNodeValue(DRIVER_NAME) >>= sThisDriverName;

            // look if we (resp. the driver manager) know this driver
            // doing O(n) search here, which is expensive, but this doesn't matter in this small case ...
            DriverPoolingSettings::iterator aLookup;
            for    (   aLookup = aSettings.begin();
                    aLookup != aSettings.end();
                    ++aLookup
                )
                if (sThisDriverName == aLookup->sName)
                    break;

            if (aLookup == aSettings.end())
            {   // do not know the driver - add it
                aSettings.push_back(DriverPooling(sThisDriverName));

                // and the position of the new entry
                aLookup = aSettings.end();
                --aLookup;
            }

            // now fill this entry with the settings from the configuration
            aThisDriverSettings.getNodeValue(ENABLE) >>= aLookup->bEnabled;
            aThisDriverSettings.getNodeValue(TIMEOUT) >>= aLookup->nTimeoutSeconds;
        }

        _rFillItems.Put(DriverPoolingSettingsItem(SID_SB_DRIVER_TIMEOUTS, std::move(aSettings)));
    }


    void ConnectionPoolConfig::SetOptions(const SfxItemSet& _rSourceItems)
    {
        // the config node where all pooling relevant info are stored under
        OConfigurationTreeRoot aConnectionPoolRoot = OConfigurationTreeRoot::createWithComponentContext(
            ::comphelper::getProcessComponentContext(), CONNECTIONPOOL_NODENAME);

        if (!aConnectionPoolRoot.isValid())
            // already asserted by the OConfigurationTreeRoot
            return;

        bool bNeedCommit = false;

        // the global "enabled" flag
        const SfxBoolItem* pEnabled = _rSourceItems.GetItem<SfxBoolItem>(SID_SB_POOLING_ENABLED);
        if (pEnabled)
        {
            bool bEnabled = pEnabled->GetValue();
            aConnectionPoolRoot.setNodeValue(ENABLE_POOLING, Any(bEnabled));
            bNeedCommit = true;
        }

        // the settings for the single drivers
        const DriverPoolingSettingsItem* pDriverSettings = _rSourceItems.GetItem<DriverPoolingSettingsItem>(SID_SB_DRIVER_TIMEOUTS);
        if (pDriverSettings)
        {
            OConfigurationNode aDriverSettings = aConnectionPoolRoot.openNode(DRIVER_SETTINGS);
            if (!aDriverSettings.isValid())
                return;

            OUString sThisDriverName;
            OConfigurationNode aThisDriverSettings;

            const DriverPoolingSettings& rNewSettings = pDriverSettings->getSettings();
            for (auto const& newSetting : rNewSettings)
            {
                // need the name as OUString
                sThisDriverName = newSetting.sName;

                // the sub-node for this driver
                if (aDriverSettings.hasByName(newSetting.sName))
                    aThisDriverSettings = aDriverSettings.openNode(newSetting.sName);
                else
                    aThisDriverSettings = aDriverSettings.createNode(newSetting.sName);

                // set the values
                aThisDriverSettings.setNodeValue(DRIVER_NAME, Any(sThisDriverName));
                aThisDriverSettings.setNodeValue(ENABLE, Any(newSetting.bEnabled));
                aThisDriverSettings.setNodeValue(TIMEOUT, Any(newSetting.nTimeoutSeconds));
            }
            bNeedCommit = true;
        }
        if (bNeedCommit)
            aConnectionPoolRoot.commit();
    }


}   // namespace offapp


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
