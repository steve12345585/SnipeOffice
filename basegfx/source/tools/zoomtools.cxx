/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <basegfx/utils/zoomtools.hxx>

namespace basegfx::zoomtools
{

/** 2^(1/6) as the default step

    This ensures (unless the rounding is used) that 6 steps lead
    to double / half zoom level.
*/
const double ZOOM_FACTOR = 1.12246205;

/**
* Round a value against a specified multiple. Values below half
* of the multiple are rounded down and all others are rounded up.
*
* @param nCurrent current value
* @param nMultiple multiple against which the current value is rounded
*/
static sal_uInt16 roundMultiple(sal_uInt16 nCurrent, int nMultiple)
{
    // round zoom to a multiple of nMultiple
    return (( nCurrent + nMultiple / 2 ) - ( nCurrent + nMultiple / 2 ) % nMultiple);
}

/**
* Convert geometric progression results into more common values by
* rounding them against certain multiples depending on the size.
* Beginning with 50 the multiple is 5, with 100, 10, and so on.
*
* @param nCurrent current zoom factor
*/
static sal_uInt16 roundZoom(double nCurrent)
{
    // convert nCurrent properly to int
    sal_uInt16 nNew = nCurrent + 0.5;

    // round to more common numbers above 50
    if (nNew > 1000) {
        nNew = roundMultiple(nNew, 100);
    } else if ( nNew > 500 ) {
        nNew = roundMultiple(nNew, 50);
    } else if ( nNew > 100 ) {
        nNew = roundMultiple(nNew, 10);
    } else if ( nNew > 50 ) {
        nNew = roundMultiple(nNew, 5);
    }

    return nNew;
}

/**
* Make sure that a certain step isn't skipped during the zooming
* progress.
*
* @param nCurrent current zoom factor
* @param nPrevious previous zoom factor
* @param nStep step which shouldn't be skipped
*/
static sal_uInt16 enforceStep(sal_uInt16 nCurrent, sal_uInt16 nPrevious, unsigned int nStep)
{
    if ((( nCurrent > nStep ) && ( nPrevious < nStep ))
    || (( nCurrent < nStep ) && ( nPrevious > nStep )))
        return nStep;
    else
        return nCurrent;
}

/**
* Increasing the zoom level.
*
* @param nCurrent current zoom factor
*/
sal_uInt16 zoomIn(sal_uInt16 nCurrent)
{
    sal_uInt16 nNew = roundZoom( nCurrent * ZOOM_FACTOR );
    // make sure some values are not skipped
    nNew = enforceStep(nNew, nCurrent, 200);
    nNew = enforceStep(nNew, nCurrent, 100);
    nNew = enforceStep(nNew, nCurrent, 75);
    nNew = enforceStep(nNew, nCurrent, 50);
    nNew = enforceStep(nNew, nCurrent, 25);
    return nNew;
}

/**
* Decreasing the zoom level.
*
* @param nCurrent current zoom factor
*/
sal_uInt16 zoomOut(sal_uInt16 nCurrent)
{
    sal_uInt16 nNew = roundZoom( nCurrent / ZOOM_FACTOR );
    // make sure some values are not skipped
    nNew = enforceStep(nNew, nCurrent, 200);
    nNew = enforceStep(nNew, nCurrent, 100);
    nNew = enforceStep(nNew, nCurrent, 75);
    nNew = enforceStep(nNew, nCurrent, 50);
    nNew = enforceStep(nNew, nCurrent, 25);
    return nNew;
}
} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
