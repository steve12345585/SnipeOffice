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


#include <sfx2/linksrc.hxx>
#include <sfx2/lnkbase.hxx>
#include <com/sun/star/uno/Any.hxx>

#include <utility>
#include <vcl/timer.hxx>
#include <memory>
#include <vector>
#include <algorithm>


using namespace ::com::sun::star::uno;

namespace sfx2
{

namespace {

class SvLinkSourceTimer : public Timer
{
    SvLinkSource *  pOwner;
    virtual void    Invoke() override;
public:
    explicit SvLinkSourceTimer( SvLinkSource * pOwn );
};

}

SvLinkSourceTimer::SvLinkSourceTimer( SvLinkSource * pOwn )
    : Timer("sfx2 SvLinkSourceTimer"), pOwner( pOwn )
{
}

void SvLinkSourceTimer::Invoke()
{
    // Secure against being destroyed in Handler
    SvLinkSourceRef xHoldAlive( pOwner );
    pOwner->SendDataChanged();
}

static void StartTimer( std::unique_ptr<SvLinkSourceTimer>& pTimer, SvLinkSource * pOwner,
                        sal_uInt64 nTimeout )
{
    if( !pTimer )
    {
        pTimer.reset( new SvLinkSourceTimer( pOwner ) );
        pTimer->SetTimeout( nTimeout );
        pTimer->Start();
    }
}

namespace {

struct SvLinkSource_Entry_Impl
{
    tools::SvRef<SvBaseLink>  xSink;
    OUString                  aDataMimeType;
    sal_uInt16                nAdviseModes;
    bool                      bIsDataSink;

    SvLinkSource_Entry_Impl( SvBaseLink* pLink, OUString aMimeType,
                                sal_uInt16 nAdvMode )
        : xSink( pLink ), aDataMimeType(std::move( aMimeType )),
            nAdviseModes( nAdvMode ), bIsDataSink( true )
    {}

    explicit SvLinkSource_Entry_Impl( SvBaseLink* pLink )
        : xSink( pLink ), nAdviseModes( 0 ), bIsDataSink( false )
    {}
};

class SvLinkSource_Array_Impl
{
friend class SvLinkSource_EntryIter_Impl;
private:
    std::vector<std::unique_ptr<SvLinkSource_Entry_Impl>> mvData;

public:
    SvLinkSource_Array_Impl() {}

    size_t size() const { return mvData.size(); }
    SvLinkSource_Entry_Impl *operator[](size_t idx) const { return mvData[idx].get(); }
    void push_back(SvLinkSource_Entry_Impl* rData) { mvData.emplace_back(rData); }

    void DeleteAndDestroy(SvLinkSource_Entry_Impl const * p)
    {
        auto it = std::find_if(mvData.begin(), mvData.end(),
            [&p](const std::unique_ptr<SvLinkSource_Entry_Impl>& rxData) { return rxData.get() == p; });
        if (it != mvData.end())
            mvData.erase(it);
    }
};

class SvLinkSource_EntryIter_Impl
{
    std::vector<SvLinkSource_Entry_Impl*> aArr;
    const SvLinkSource_Array_Impl& rOrigArr;
    sal_uInt16 nPos;
public:
    explicit SvLinkSource_EntryIter_Impl( const SvLinkSource_Array_Impl& rArr );
    SvLinkSource_Entry_Impl* Curr()
                            { return nPos < aArr.size() ? aArr[ nPos ] : nullptr; }
    SvLinkSource_Entry_Impl* Next();
    bool IsValidCurrValue( SvLinkSource_Entry_Impl const * pEntry );
};

}

SvLinkSource_EntryIter_Impl::SvLinkSource_EntryIter_Impl(
        const SvLinkSource_Array_Impl& rArr )
    : rOrigArr( rArr ), nPos( 0 )
{
    for (auto const & i : rArr.mvData)
        aArr.push_back(i.get());
}

bool SvLinkSource_EntryIter_Impl::IsValidCurrValue( SvLinkSource_Entry_Impl const * pEntry )
{
    if ( nPos >= aArr.size() )
        return false;
    if (aArr[nPos] != pEntry)
        return false;
    for (auto const & i : rOrigArr.mvData)
        if (i.get() == pEntry)
            return true;
    return false;
}

SvLinkSource_Entry_Impl* SvLinkSource_EntryIter_Impl::Next()
{
    SvLinkSource_Entry_Impl* pRet = nullptr;
    if( nPos + 1 < static_cast<sal_uInt16>(aArr.size()) )
    {
        ++nPos;
        if( rOrigArr.size() == aArr.size() &&
            rOrigArr[ nPos ] == aArr[ nPos ] )
            pRet = aArr[ nPos ];
        else
        {
            // then we must search the current (or the next) in the orig
            do {
                pRet = aArr[ nPos ];
                for (auto const & i : rOrigArr.mvData)
                    if (i.get() == pRet)
                        return pRet;
                pRet = nullptr;
                ++nPos;
            } while( nPos < aArr.size() );

            if( nPos >= aArr.size() )
                pRet = nullptr;
        }
    }
    return pRet;
}

struct SvLinkSource_Impl
{
    SvLinkSource_Array_Impl aArr;
    OUString                aDataMimeType;
    std::unique_ptr<SvLinkSourceTimer>
                            pTimer;
    sal_uInt64              nTimeout;
    css::uno::Reference<css::io::XInputStream>
                            m_xInputStreamToLoadFrom;
    bool                    m_bIsReadOnly;

    SvLinkSource_Impl()
        : nTimeout(3000)
        , m_bIsReadOnly(false)
    {
    }
};

SvLinkSource::SvLinkSource()
     : pImpl( new SvLinkSource_Impl )
{
}

SvLinkSource::~SvLinkSource()
{
}


SvLinkSource::StreamToLoadFrom SvLinkSource::getStreamToLoadFrom()
{
    return StreamToLoadFrom(
        pImpl->m_xInputStreamToLoadFrom,
        pImpl->m_bIsReadOnly);
}

void SvLinkSource::setStreamToLoadFrom(const css::uno::Reference<css::io::XInputStream>& xInputStream, bool bIsReadOnly )
{
    pImpl->m_xInputStreamToLoadFrom = xInputStream;
    pImpl->m_bIsReadOnly = bIsReadOnly;
}

// #i88291#
void SvLinkSource::clearStreamToLoadFrom()
{
    pImpl->m_xInputStreamToLoadFrom.clear();
}

void  SvLinkSource::Closed()
{
    SvLinkSource_EntryIter_Impl aIter( pImpl->aArr );
    for( SvLinkSource_Entry_Impl* p = aIter.Curr(); p; p = aIter.Next() )
        if( !p->bIsDataSink )
            p->xSink->Closed();
}

sal_uInt64 SvLinkSource::GetUpdateTimeout() const
{
    return pImpl->nTimeout;
}

void SvLinkSource::SetUpdateTimeout( sal_uInt64 nTimeout )
{
    pImpl->nTimeout = nTimeout;
    if( pImpl->pTimer )
        pImpl->pTimer->SetTimeout( nTimeout );
}

void SvLinkSource::SendDataChanged()
{
    SvLinkSource_EntryIter_Impl aIter( pImpl->aArr );
    for( SvLinkSource_Entry_Impl* p = aIter.Curr(); p; p = aIter.Next() )
    {
        if( p->bIsDataSink )
        {
            OUString sDataMimeType( pImpl->aDataMimeType );
            if( sDataMimeType.isEmpty() )
                sDataMimeType = p->aDataMimeType;

            Any aVal;
            if( ( p->nAdviseModes & ADVISEMODE_NODATA ) ||
                GetData( aVal, sDataMimeType, true ) )
            {
                p->xSink->DataChanged( sDataMimeType, aVal );

                if ( !aIter.IsValidCurrValue( p ) )
                    continue;

                if( p->nAdviseModes & ADVISEMODE_ONLYONCE )
                {
                    pImpl->aArr.DeleteAndDestroy( p );
                }

            }
        }
    }
    pImpl->pTimer.reset();
    pImpl->aDataMimeType.clear();
}

void SvLinkSource::NotifyDataChanged()
{
    if( pImpl->nTimeout )
        StartTimer( pImpl->pTimer, this, pImpl->nTimeout ); // New timeout
    else
    {
        SvLinkSource_EntryIter_Impl aIter( pImpl->aArr );
        for( SvLinkSource_Entry_Impl* p = aIter.Curr(); p; p = aIter.Next() )
            if( p->bIsDataSink )
            {
                Any aVal;
                if( ( p->nAdviseModes & ADVISEMODE_NODATA ) ||
                    GetData( aVal, p->aDataMimeType, true ) )
                {
                    tools::SvRef<sfx2::SvBaseLink> xLink(p->xSink);
                    xLink->DataChanged( p->aDataMimeType, aVal );

                    if ( !aIter.IsValidCurrValue( p ) )
                        continue;

                    if( p->nAdviseModes & ADVISEMODE_ONLYONCE )
                    {
                        pImpl->aArr.DeleteAndDestroy( p );
                    }
                }
            }

        pImpl->pTimer.reset();
    }
}

// notify the sink, the mime type is not
// a selection criterion
void SvLinkSource::DataChanged( const OUString & rMimeType,
                                const css::uno::Any & rVal )
{
    if( pImpl->nTimeout && !rVal.hasValue() )
    {   // only when no data was included
        // fire all data to the sink, independent of the requested format
        pImpl->aDataMimeType = rMimeType;
        StartTimer( pImpl->pTimer, this, pImpl->nTimeout ); // New timeout
    }
    else
    {
        SvLinkSource_EntryIter_Impl aIter( pImpl->aArr );
        for( SvLinkSource_Entry_Impl* p = aIter.Curr(); p; p = aIter.Next() )
        {
            if( p->bIsDataSink )
            {
                p->xSink->DataChanged( rMimeType, rVal );

                if ( !aIter.IsValidCurrValue( p ) )
                    continue;

                if( p->nAdviseModes & ADVISEMODE_ONLYONCE )
                {
                    pImpl->aArr.DeleteAndDestroy( p );
                }
            }
        }

        pImpl->pTimer.reset();
    }
}


// only one link is correct
void SvLinkSource::AddDataAdvise( SvBaseLink * pLink, const OUString& rMimeType,
                                    sal_uInt16 nAdviseModes )
{
    SvLinkSource_Entry_Impl* pNew = new SvLinkSource_Entry_Impl(
                    pLink, rMimeType, nAdviseModes );
    pImpl->aArr.push_back( pNew );
}

void SvLinkSource::RemoveAllDataAdvise( SvBaseLink const * pLink )
{
    SvLinkSource_EntryIter_Impl aIter( pImpl->aArr );
    for( SvLinkSource_Entry_Impl* p = aIter.Curr(); p; p = aIter.Next() )
        if( p->bIsDataSink && p->xSink.get() == pLink )
        {
            pImpl->aArr.DeleteAndDestroy( p );
        }
}

// only one link is correct
void SvLinkSource::AddConnectAdvise( SvBaseLink * pLink )
{
    SvLinkSource_Entry_Impl* pNew = new SvLinkSource_Entry_Impl( pLink );
    pImpl->aArr.push_back( pNew );
}

void SvLinkSource::RemoveConnectAdvise( SvBaseLink const * pLink )
{
    SvLinkSource_EntryIter_Impl aIter( pImpl->aArr );
    for( SvLinkSource_Entry_Impl* p = aIter.Curr(); p; p = aIter.Next() )
        if( !p->bIsDataSink && p->xSink.get() == pLink )
        {
            pImpl->aArr.DeleteAndDestroy( p );
        }
}

bool SvLinkSource::HasDataLinks() const
{
    bool bRet = false;
    for( sal_uInt16 n = 0, nEnd = pImpl->aArr.size(); n < nEnd; ++n )
        if( pImpl->aArr[ n ]->bIsDataSink )
        {
            bRet = true;
            break;
        }
    return bRet;
}

// sal_True => waitinmg for data
bool SvLinkSource::IsPending() const
{
    return false;
}

// sal_True => data complete loaded
bool SvLinkSource::IsDataComplete() const
{
    return true;
}

bool SvLinkSource::Connect( SvBaseLink* )
{
    return true;
}

bool SvLinkSource::GetData( css::uno::Any &, const OUString &, bool )
{
    return false;
}

void SvLinkSource::Edit(weld::Window *, SvBaseLink *, const Link<const OUString&, void>&)
{
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
