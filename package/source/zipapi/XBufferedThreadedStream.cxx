/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "XBufferedThreadedStream.hxx"

using namespace css::uno;

namespace {

class UnzippingThread: public salhelper::Thread
{
    XBufferedThreadedStream &mxStream;
public:
    explicit UnzippingThread(XBufferedThreadedStream &xStream): Thread("Unzipping"), mxStream(xStream) {}
private:
    virtual void execute() override
    {
        try
        {
            mxStream.produce();
        }
        catch (...)
        {
            mxStream.saveException(std::current_exception());
        }

        mxStream.setTerminateThread();
    }
};

}

XBufferedThreadedStream::XBufferedThreadedStream(
                    const Reference<XInputStream>& xSrcStream,
                    sal_Int64 nStreamSize)
: mxSrcStream( xSrcStream )
, mnPos(0)
, mnStreamSize( nStreamSize )
, mnOffset( 0 )
, mxUnzippingThread( new UnzippingThread(*this) )
, mbTerminateThread( false )
{
    mxUnzippingThread->launch();
}

XBufferedThreadedStream::~XBufferedThreadedStream()
{
    setTerminateThread();
    mxUnzippingThread->join();
}

/**
 * Reads from UnbufferedStream in a separate thread and stores the buffer blocks
 * in maPendingBuffers queue for further use.
 */
void XBufferedThreadedStream::produce()
{
    Buffer pProducedBuffer;
    sal_Int64 nTotalBytesRead(0);
    std::unique_lock<std::mutex> aGuard( maBufferProtector );
    do
    {
        if( !maUsedBuffers.empty() )
        {
            pProducedBuffer = maUsedBuffers.front();
            maUsedBuffers.pop();
        }

        aGuard.unlock();
        nTotalBytesRead += mxSrcStream->readBytes( pProducedBuffer, nBufferSize );

        aGuard.lock();
        maPendingBuffers.push( pProducedBuffer );
        maBufferConsumeResume.notify_one();

        if (!mbTerminateThread)
            maBufferProduceResume.wait( aGuard, [&]{return canProduce(); } );

    } while( !mbTerminateThread && nTotalBytesRead < mnStreamSize );
}

/**
 * Fetches next available block from maPendingBuffers for use in Reading thread.
 */
const Buffer& XBufferedThreadedStream::getNextBlock()
{
    std::unique_lock<std::mutex> aGuard( maBufferProtector );
    const sal_Int32 nBufSize = maInUseBuffer.getLength();
    if( nBufSize <= 0 || mnOffset >= nBufSize )
    {
        if( mnOffset >= nBufSize )
            maUsedBuffers.push( maInUseBuffer );

        maBufferConsumeResume.wait( aGuard, [&]{return canConsume(); } );

        if( maPendingBuffers.empty() )
        {
            maInUseBuffer = Buffer();
            if (maSavedException)
                std::rethrow_exception(maSavedException);
        }
        else
        {
            maInUseBuffer = maPendingBuffers.front();
            maPendingBuffers.pop();
            mnOffset = 0;

            if( maPendingBuffers.size() <= nBufferLowWater )
                maBufferProduceResume.notify_one();
        }
    }

    return maInUseBuffer;
}

void XBufferedThreadedStream::setTerminateThread()
{
    std::scoped_lock<std::mutex> aGuard( maBufferProtector );
    mbTerminateThread = true;
    maBufferProduceResume.notify_one();
    maBufferConsumeResume.notify_one();
}

sal_Int32 SAL_CALL XBufferedThreadedStream::readBytes( Sequence< sal_Int8 >& rData, sal_Int32 nBytesToRead )
{
    if( !hasBytes() )
        return 0;

    const sal_Int32 nAvailableSize = static_cast< sal_Int32 > ( std::min< sal_Int64 >( nBytesToRead, remainingSize() ) );
    rData.realloc( nAvailableSize );
    auto pData = rData.getArray();
    sal_Int32 i = 0, nPendingBytes = nAvailableSize;

    while( nPendingBytes )
    {
        const Buffer &pBuffer = getNextBlock();
        if( !pBuffer.hasElements() )
        {
            rData.realloc( nAvailableSize - nPendingBytes );
            return nAvailableSize - nPendingBytes;
        }
        const sal_Int32 limit = std::min<sal_Int32>( nPendingBytes, pBuffer.getLength() - mnOffset );

        memcpy( &pData[i], &pBuffer[mnOffset], limit );

        nPendingBytes -= limit;
        mnOffset += limit;
        mnPos += limit;
        i += limit;
    }

    return nAvailableSize;
}

sal_Int32 XBufferedThreadedStream::readSomeBytes( sal_Int8* pData, sal_Int32 nBytesToRead )
{
    if( !hasBytes() )
        return 0;

    const sal_Int32 nAvailableSize = static_cast< sal_Int32 > ( std::min< sal_Int64 >( nBytesToRead, remainingSize() ) );
    sal_Int32 i = 0, nPendingBytes = nAvailableSize;

    while( nPendingBytes )
    {
        const Buffer &pBuffer = getNextBlock();
        if( !pBuffer.hasElements() )
            return nAvailableSize - nPendingBytes;
        const sal_Int32 limit = std::min<sal_Int32>( nPendingBytes, pBuffer.getLength() - mnOffset );

        memcpy( &pData[i], &pBuffer[mnOffset], limit );

        nPendingBytes -= limit;
        mnOffset += limit;
        mnPos += limit;
        i += limit;
    }

    return nAvailableSize;
}

sal_Int32 SAL_CALL XBufferedThreadedStream::readSomeBytes( Sequence< sal_Int8 >& aData, sal_Int32 nMaxBytesToRead )
{
    return readBytes( aData, nMaxBytesToRead );
}
void SAL_CALL XBufferedThreadedStream::skipBytes( sal_Int32 nBytesToSkip )
{
    if( nBytesToSkip )
    {
        Sequence < sal_Int8 > aSequence( nBytesToSkip );
        readBytes( aSequence, nBytesToSkip );
    }
}

sal_Int32 SAL_CALL XBufferedThreadedStream::available()
{
    if( !hasBytes() )
        return 0;

    return static_cast< sal_Int32 > ( std::min< sal_Int64 >( SAL_MAX_INT32, remainingSize() ) );
}

void SAL_CALL XBufferedThreadedStream::closeInput()
{
    setTerminateThread();
    mxUnzippingThread->join();
    mxSrcStream->closeInput();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
