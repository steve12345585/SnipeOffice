/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "rtfskipdestination.hxx"
#include <sal/log.hxx>
#include "rtflistener.hxx"

namespace writerfilter::rtftok
{
RTFSkipDestination::RTFSkipDestination(RTFListener& rImport)
    : m_rImport(rImport)
    , m_bParsed(true)
    , m_bReset(true)
{
}

RTFSkipDestination::~RTFSkipDestination()
{
    if (m_rImport.getSkipUnknown() && m_bReset)
    {
        if (!m_bParsed)
        {
            SAL_INFO("writerfilter", __func__ << ": skipping destination");
            m_rImport.setDestination(Destination::SKIP);
        }
        m_rImport.setSkipUnknown(false);
    }
}

void RTFSkipDestination::setParsed(bool bParsed) { m_bParsed = bParsed; }

void RTFSkipDestination::setReset(bool bReset) { m_bReset = bReset; }

} // namespace writerfilter::rtftok

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
