/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "lwptaboverride.hxx"

LwpTabOverride::LwpTabOverride(LwpTabOverride const& rOther)
    : LwpOverride(rOther)
    , m_aTabRackID(rOther.m_aTabRackID)
{
}

LwpTabOverride* LwpTabOverride::clone() const { return new LwpTabOverride(*this); }

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
