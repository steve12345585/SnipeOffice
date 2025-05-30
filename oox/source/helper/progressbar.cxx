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

#include <oox/helper/progressbar.hxx>

#include <com/sun/star/task/XStatusIndicator.hpp>
#include <oox/helper/helper.hxx>

#include <sal/log.hxx>

namespace oox {

using namespace ::com::sun::star::task;
using namespace ::com::sun::star::uno;

namespace {

const sal_Int32 PROGRESS_RANGE      = 1000000;

} // namespace

IProgressBar::~IProgressBar()
{
}

ISegmentProgressBar::~ISegmentProgressBar()
{
}

ProgressBar::ProgressBar( const Reference< XStatusIndicator >& rxIndicator, const OUString& rText ) :
    mxIndicator( rxIndicator ),
    mfPosition( 0 )
{
    if( mxIndicator.is() )
        mxIndicator->start( rText, PROGRESS_RANGE );
}

ProgressBar::~ProgressBar()
{
    if( mxIndicator.is() )
        mxIndicator->end();
}

double ProgressBar::getPosition() const
{
    return mfPosition;
}

void ProgressBar::setPosition( double fPosition )
{
    SAL_WARN_IF( (mfPosition > fPosition) || (fPosition > 1.0), "oox", "ProgressBar::setPosition - invalid position" );
    mfPosition = getLimitedValue< double >( fPosition, mfPosition, 1.0 );
    if( mxIndicator.is() )
        mxIndicator->setValue( static_cast< sal_Int32 >( mfPosition * PROGRESS_RANGE ) );
}

namespace prv {

namespace {

class SubSegment : public ISegmentProgressBar
{
public:
    explicit            SubSegment( IProgressBar& rParentProgress, double fStartPos, double fLength );

    virtual double      getPosition() const override;
    virtual void        setPosition( double fPosition ) override;

    virtual double      getFreeLength() const override;
    virtual ISegmentProgressBarRef createSegment( double fLength ) override;

private:
    IProgressBar&       mrParentProgress;
    double              mfStartPos;
    double              mfLength;
    double              mfPosition;
    double              mfFreeStart;
};

}

SubSegment::SubSegment( IProgressBar& rParentProgress, double fStartPos, double fLength ) :
    mrParentProgress( rParentProgress ),
    mfStartPos( fStartPos ),
    mfLength( fLength ),
    mfPosition( 0.0 ),
    mfFreeStart( 0.0 )
{
}

double SubSegment::getPosition() const
{
    return mfPosition;
}

void SubSegment::setPosition( double fPosition )
{
    SAL_WARN_IF( (mfPosition > fPosition) || (fPosition > 1.0), "oox", "SubSegment::setPosition - invalid position" );
    mfPosition = getLimitedValue< double >( fPosition, mfPosition, 1.0 );
    mrParentProgress.setPosition( mfStartPos + mfPosition * mfLength );
}

double SubSegment::getFreeLength() const
{
    return 1.0 - mfFreeStart;
}

ISegmentProgressBarRef SubSegment::createSegment( double fLength )
{
    SAL_WARN_IF( (0.0 >= fLength) || (fLength > getFreeLength()), "oox", "SubSegment::createSegment - invalid length" );
    fLength = getLimitedValue< double >( fLength, 0.0, getFreeLength() );
    ISegmentProgressBarRef xSegment = std::make_shared<prv::SubSegment>( *this, mfFreeStart, fLength );
    mfFreeStart += fLength;
    return xSegment;
}

} // namespace oox::prv

SegmentProgressBar::SegmentProgressBar( const Reference< XStatusIndicator >& rxIndicator, const OUString& rText ) :
    maProgress( rxIndicator, rText ),
    mfFreeStart( 0.0 )
{
}

double SegmentProgressBar::getPosition() const
{
    return maProgress.getPosition();
}

void SegmentProgressBar::setPosition( double fPosition )
{
    maProgress.setPosition( fPosition );
}

double SegmentProgressBar::getFreeLength() const
{
    return 1.0 - mfFreeStart;
}

ISegmentProgressBarRef SegmentProgressBar::createSegment( double fLength )
{
    SAL_WARN_IF( (0.0 >= fLength) || (fLength > getFreeLength()), "oox", "SegmentProgressBar::createSegment - invalid length" );
    fLength = getLimitedValue< double >( fLength, 0.0, getFreeLength() );
    ISegmentProgressBarRef xSegment = std::make_shared<prv::SubSegment>( maProgress, mfFreeStart, fLength );
    mfFreeStart += fLength;
    return xSegment;
}

} // namespace oox

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
