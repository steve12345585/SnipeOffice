/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SAL_INC_INTERNAL_OSLRANDOM_H
#define INCLUDED_SAL_INC_INTERNAL_OSLRANDOM_H

#include <cstddef>

#if defined __cplusplus
extern "C" {
#endif

bool osl_get_system_random_data(char* buffer, size_t desired_len);

#if defined __cplusplus
}
#endif

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
