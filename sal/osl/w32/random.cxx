/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define _CRT_RAND_S

#include <stdlib.h>
#include <memory.h>

#include <oslrandom.h>

bool osl_get_system_random_data(char* buffer, size_t desired_len)
{
    unsigned int val;

    /* if unaligned fill to alignment */
    if (reinterpret_cast<uintptr_t>(buffer) & 3)
    {
        size_t len = 4 - (reinterpret_cast<size_t>(buffer) & 3);

        if (len > desired_len)
        {
            len = desired_len;
        }
        if (rand_s(&val))
        {
            return false;
        }
        memcpy(buffer, &val, len);
        buffer += len;
        desired_len -= len;
    }
    /* fill directly into the buffer as long as we can */
    while (desired_len >= 4)
    {
        if (rand_s(reinterpret_cast<unsigned int*>(buffer)))
        {
            return false;
        }
        else
        {
            buffer += 4;
            desired_len -= 4;
        }
    }
    /* deal with the partial int reminder to fill */
    if (desired_len)
    {
        if (rand_s(&val))
        {
            return false;
        }
        memcpy(buffer, &val, desired_len);
    }
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
