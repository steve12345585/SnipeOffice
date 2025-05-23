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

#pragma once

#include <svl/poolitem.hxx>
#include <com/sun/star/media/ZoomLevel.hpp>
#include <avmedia/avmediadllapi.h>
#include <memory>
#include <string_view>

#include <o3tl/typed_flags_set.hxx>
#include <utility>

namespace com::sun::star::embed { class XStorage; }
namespace com::sun::star::frame { class XModel; }
namespace com::sun::star::io { class XInputStream; }
namespace com::sun::star::io { class XStream; }
namespace com::sun::star::text { struct GraphicCrop; }
class Graphic;

enum class AVMediaSetMask
{
    NONE        = 0x000,
    STATE       = 0x001,
    DURATION    = 0x002,
    TIME        = 0x004,
    LOOP        = 0x008,
    MUTE        = 0x010,
    VOLUMEDB    = 0x020,
    ZOOM        = 0x040,
    URL         = 0x080,
    MIME_TYPE   = 0x100,
    GRAPHIC     = 0x200,
    CROP        = 0x400,
    ALL         = 0x7ff,
};
namespace o3tl
{
    template<> struct typed_flags<AVMediaSetMask> : is_typed_flags<AVMediaSetMask, 0x7ff> {};
}


namespace avmedia
{


enum class MediaState
{
    Stop, Play, Pause
};


class AVMEDIA_DLLPUBLIC MediaItem final : public SfxPoolItem
{
public:
                            static SfxPoolItem* CreateDefault();

    DECLARE_ITEM_TYPE_FUNCTION(MediaItem)
    explicit                MediaItem( sal_uInt16 i_nWhich = 0,
                                       AVMediaSetMask nMaskSet = AVMediaSetMask::NONE );
                            MediaItem( const MediaItem& rMediaItem );
    virtual                 ~MediaItem() override;

    virtual bool            operator==( const SfxPoolItem& ) const override;
    virtual MediaItem*      Clone( SfxItemPool* pPool = nullptr ) const override;
    virtual bool            GetPresentation( SfxItemPresentation ePres,
                                                 MapUnit eCoreUnit,
                                                 MapUnit ePresUnit,
                                                 OUString&  rText,
                                                 const IntlWrapper& rIntl ) const override;
    virtual bool            QueryValue( css::uno::Any& rVal, sal_uInt8 nMemberId = 0 ) const override;
    virtual bool            PutValue( const css::uno::Any& rVal, sal_uInt8 nMemberId ) override;

    bool                    merge(const MediaItem& rMediaItem);

    AVMediaSetMask          getMaskSet() const;

    bool                    setState(MediaState eState);
    MediaState              getState() const;

    bool                    setDuration(double fDuration);
    double                  getDuration() const;

    bool                    setTime(double fTime);
    double                  getTime() const;

    bool                    setLoop(bool bLoop);
    bool                    isLoop() const;

    bool                    setMute(bool bMute);
    bool                    isMute() const;

    bool                    setVolumeDB(sal_Int16 nDB);
    sal_Int16               getVolumeDB() const;

    bool                    setZoom(css::media::ZoomLevel eZoom);
    ::css::media::ZoomLevel getZoom() const;

    bool                    setURL(const OUString& rURL,
                                   const OUString& rTempURL,
                                   const OUString& rReferer);
    const OUString&         getURL() const;

    bool                    setFallbackURL(const OUString& rURL);
    const OUString&         getFallbackURL() const;

    bool                    setMimeType(const OUString& rMimeType);
    const OUString&         getMimeType() const;
    bool setGraphic(const Graphic& rGraphic);
    const Graphic & getGraphic() const;
    bool setCrop(const css::text::GraphicCrop& rCrop);
    const css::text::GraphicCrop& getCrop() const;
    const OUString&         getTempURL() const;

    const OUString&         getReferer() const;

private:

    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};

typedef ::avmedia::MediaItem avmedia_MediaItem;

bool AVMEDIA_DLLPUBLIC EmbedMedia(
        const ::css::uno::Reference< ::css::frame::XModel>& xModel,
        const OUString& rSourceURL,
        OUString & o_rEmbeddedURL,
        ::css::uno::Reference<::css::io::XInputStream> const& xInputStream =
            ::css::uno::Reference<::css::io::XInputStream>());

bool AVMEDIA_DLLPUBLIC CreateMediaTempFile(
        ::css::uno::Reference<::css::io::XInputStream> const& xInStream,
        OUString& o_rTempFileURL,
        std::u16string_view rDesiredExtension);

OUString GetFilename(OUString const& rSourceURL);

::css::uno::Reference< ::css::io::XStream> CreateStream(
    const ::css::uno::Reference< ::css::embed::XStorage>& xStorage, const OUString& rFilename);

struct AVMEDIA_DLLPUBLIC MediaTempFile
{
    OUString const m_TempFileURL;
    MediaTempFile(OUString aURL)
        : m_TempFileURL(std::move(aURL))
    {}
    ~MediaTempFile();
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
