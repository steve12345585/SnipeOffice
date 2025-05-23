/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <oslmemory.h>

#include <malloc.h>

void* osl_aligned_alloc( sal_Size align, sal_Size size )
{
    return _aligned_malloc(size, align);
}

void osl_aligned_free( void* p )
{
    _aligned_free(p);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
