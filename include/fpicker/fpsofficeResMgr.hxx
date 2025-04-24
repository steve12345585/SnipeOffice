/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <unotools/resmgr.hxx>

inline std::locale FpsResLocale() { return Translate::Create("fps"); }
inline OUString FpsResId(TranslateId aId) { return Translate::get(aId, FpsResLocale()); };

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
