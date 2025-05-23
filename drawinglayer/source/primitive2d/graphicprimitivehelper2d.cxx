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

#include <sal/config.h>

#include <algorithm>

#include <drawinglayer/primitive2d/graphicprimitivehelper2d.hxx>
#include <drawinglayer/animation/animationtiming.hxx>
#include <drawinglayer/primitive2d/bitmapprimitive2d.hxx>
#include <drawinglayer/primitive2d/BitmapAlphaPrimitive2D.hxx>
#include <drawinglayer/primitive2d/unifiedtransparenceprimitive2d.hxx>
#include <drawinglayer/primitive2d/animatedprimitive2d.hxx>
#include <drawinglayer/primitive2d/metafileprimitive2d.hxx>
#include <drawinglayer/primitive2d/transformprimitive2d.hxx>
#include <drawinglayer/primitive2d/maskprimitive2d.hxx>
#include <drawinglayer/primitive2d/modifiedcolorprimitive2d.hxx>
#include <drawinglayer/primitive2d/fillgraphicprimitive2d.hxx>
#include <drawinglayer/primitive2d/drawinglayer_primitivetypes2d.hxx>
#include <drawinglayer/geometry/viewinformation2d.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolygontools.hxx>
#include <basegfx/numeric/ftools.hxx>

// helper class for animated graphics

#include <utility>
#include <vcl/animate/Animation.hxx>
#include <vcl/graph.hxx>
#include <vcl/virdev.hxx>
#include <vcl/svapp.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <vcl/skia/SkiaHelper.hxx>

namespace drawinglayer::primitive2d
{
        namespace {

        class AnimatedGraphicPrimitive2D : public AnimatedSwitchPrimitive2D
        {
        private:
            /// the geometric definition
            basegfx::B2DHomMatrix                       maTransform;

            /** the Graphic with all its content possibilities, here only
                animated is allowed and gets checked by isValidData().
                an instance of Graphic is used here since it's ref-counted
                and thus a safe copy for now
             */
            Graphic                                     maGraphic;

            /** defines parameters for tiling if this AnimatedGraphicPrimitive2D
                is to be used for a FillGraphicPrimitive2D. In that case,
                maFillGraphicAttribute.isDefault() will be false
             */
            drawinglayer::attribute::FillGraphicAttribute maFillGraphicAttribute;

            /// local animation processing data, excerpt from maGraphic
            ::Animation                                 maAnimation;

            /// the transparency in range [0.0 .. 1.0]
            double mfTransparency;

            /// the on-demand created VirtualDevices for frame creation
            ScopedVclPtrInstance< VirtualDevice >       maVirtualDevice;
            ScopedVclPtrInstance< VirtualDevice >       maVirtualDeviceMask;

            // index of the next frame that would be regularly prepared
            sal_uInt32                                  mnNextFrameToPrepare;

            /// buffering of 1st frame (always active)
            Primitive2DReference                        maBufferedFirstFrame;

            /// buffering of all frames
            std::vector<Primitive2DReference>           maBufferedPrimitives;
            bool                                        mbBufferingAllowed;

            /// set if the animation is huge so that just always the next frame
            /// is used instead of using timing
            bool                                        mbHugeSize;

            /// helper methods
            bool isValidData() const
            {
                return (GraphicType::Bitmap == maGraphic.GetType()
                    && maGraphic.IsAnimated()
                    && maAnimation.Count()
                    && !basegfx::fTools::equal(getTransparency(), 1.0));
            }

            void ensureVirtualDeviceSizeAndState()
            {
                if (!isValidData())
                    return;

                const Size aCurrent(maVirtualDevice->GetOutputSizePixel());
                const Size aTarget(maAnimation.GetDisplaySizePixel());

                if (aCurrent != aTarget)
                {
                    maVirtualDevice->EnableMapMode(false);
                    maVirtualDeviceMask->EnableMapMode(false);
                    maVirtualDevice->SetOutputSizePixel(aTarget);
                    maVirtualDeviceMask->SetOutputSizePixel(aTarget);

                    // tdf#156630 make erase calls fill with transparency
                    maVirtualDevice->SetBackground(COL_BLACK);
                    maVirtualDeviceMask->SetBackground(COL_ALPHA_TRANSPARENT);
                }

                maVirtualDevice->Erase();
                maVirtualDeviceMask->Erase();
                const ::tools::Rectangle aRect(Point(0, 0), aTarget);
                maVirtualDeviceMask->SetFillColor(COL_BLACK);
                maVirtualDeviceMask->SetLineColor();
                maVirtualDeviceMask->DrawRect(aRect);
            }

            sal_uInt32 generateStepTime(sal_uInt32 nIndex) const
            {
                const AnimationFrame& rAnimationFrame = maAnimation.Get(sal_uInt16(nIndex));
                sal_uInt32 nWaitTime(rAnimationFrame.mnWait * 10);

                // Take care of special value for MultiPage TIFFs. ATM these shall just
                // show their first page. Later we will offer some switching when object
                // is selected.
                if (ANIMATION_TIMEOUT_ON_CLICK == rAnimationFrame.mnWait)
                {
                    // ATM the huge value would block the timer, so
                    // use a long time to show first page (whole day)
                    nWaitTime = 100 * 60 * 60 * 24;
                }

                // Bad trap: There are animated gifs with no set WaitTime (!).
                // In that case use a default value.
                if (0 == nWaitTime)
                {
                    nWaitTime = 100;
                }

                return nWaitTime;
            }

            void createAndSetAnimationTiming()
            {
                if (!isValidData())
                    return;

                animation::AnimationEntryLoop aAnimationLoop(maAnimation.GetLoopCount() ? maAnimation.GetLoopCount() : 0xffff);
                const sal_uInt32 nCount(maAnimation.Count());

                for (sal_uInt32 a(0); a < nCount; a++)
                {
                    const sal_uInt32 aStepTime(generateStepTime(a));
                    const animation::AnimationEntryFixed aTime(static_cast<double>(aStepTime), static_cast<double>(a) / static_cast<double>(nCount));

                    aAnimationLoop.append(aTime);
                }

                animation::AnimationEntryList aAnimationEntryList;
                aAnimationEntryList.append(aAnimationLoop);

                setAnimationEntry(aAnimationEntryList);
            }

            Primitive2DReference createFromBuffer() const
            {
                // create BitmapEx by extracting from VirtualDevices
                const Bitmap aMainBitmap(maVirtualDevice->GetBitmap(Point(), maVirtualDevice->GetOutputSizePixel()));
                bool useAlphaMask = false;
#if defined(MACOSX) || defined(IOS)
                useAlphaMask = true;
#else
                // GetBitmap()-> AlphaMask is optimized with SkiaSalBitmap::InterpretAs8Bit(), 1bpp mask is not.
                if( SkiaHelper::isVCLSkiaEnabled())
                    useAlphaMask = true;
#endif
                BitmapEx bitmap;
                if( useAlphaMask )
                {
                    const AlphaMask aMaskBitmap(maVirtualDeviceMask->GetBitmap(Point(), maVirtualDeviceMask->GetOutputSizePixel()));
                    bitmap = BitmapEx(aMainBitmap, aMaskBitmap);
                }
                else
                {
                    Bitmap aMaskBitmap(maVirtualDeviceMask->GetBitmap(Point(), maVirtualDeviceMask->GetOutputSizePixel()));
                    // tdf#156630 invert the alpha mask
                    aMaskBitmap.Invert(); // convert from transparency to alpha
                    bitmap = BitmapEx(aMainBitmap, aMaskBitmap);
                }

                if (!maFillGraphicAttribute.isDefault())
                {
                    // need to create FillGraphicPrimitive2D
                    const drawinglayer::attribute::FillGraphicAttribute aAttribute(
                        Graphic(bitmap),
                        maFillGraphicAttribute.getGraphicRange(),
                        maFillGraphicAttribute.getTiling(),
                        maFillGraphicAttribute.getOffsetX(),
                        maFillGraphicAttribute.getOffsetY());

                    return new FillGraphicPrimitive2D(
                        getTransform(),
                        aAttribute,
                        getTransparency());
                }

                // need to create BitmapAlphaPrimitive2D/BitmapPrimitive2D
                if (basegfx::fTools::equal(getTransparency(), 0.0))
                    return new BitmapPrimitive2D(bitmap, getTransform());

                return new BitmapAlphaPrimitive2D(bitmap, getTransform(), getTransparency());
            }

            void checkSafeToBuffer(sal_uInt32 nIndex)
            {
                if (mbBufferingAllowed)
                {
                    // all frames buffered
                    if (!maBufferedPrimitives.empty() && nIndex < maBufferedPrimitives.size())
                    {
                        if (!maBufferedPrimitives[nIndex].is())
                        {
                            maBufferedPrimitives[nIndex] = createFromBuffer();

                            // check if buffering is complete
                            bool bBufferingComplete(true);

                            for (auto const & a: maBufferedPrimitives)
                            {
                                if (!a.is())
                                {
                                    bBufferingComplete = false;
                                    break;
                                }
                            }

                            if (bBufferingComplete)
                            {
                                maVirtualDevice.disposeAndClear();
                                maVirtualDeviceMask.disposeAndClear();
                            }
                        }
                    }
                }
                else
                {
                    // always buffer first frame
                    if (0 == nIndex && !maBufferedFirstFrame.is())
                    {
                        maBufferedFirstFrame = createFromBuffer();
                    }
                }
            }

            void createFrame(sal_uInt32 nTarget)
            {
                // mnNextFrameToPrepare is the target frame to create next (which implies that
                // mnNextFrameToPrepare-1 *is* currently in the VirtualDevice when
                // 0 != mnNextFrameToPrepare. nTarget is the target frame.
                if (!isValidData())
                    return;

                if (mnNextFrameToPrepare > nTarget)
                {
                    // we are ahead request, reset mechanism to start at frame zero
                    ensureVirtualDeviceSizeAndState();
                    mnNextFrameToPrepare = 0;
                }

                while (mnNextFrameToPrepare <= nTarget)
                {
                    // prepare step
                    const AnimationFrame& rAnimationFrame = maAnimation.Get(sal_uInt16(mnNextFrameToPrepare));

                    bool bSourceBlending = rAnimationFrame.meBlend == Blend::Source;

                    if (bSourceBlending)
                    {
                        tools::Rectangle aArea(rAnimationFrame.maPositionPixel, rAnimationFrame.maBitmapEx.GetSizePixel());
                        maVirtualDevice->Erase(aArea);
                        maVirtualDeviceMask->Erase(aArea);
                    }

                    switch (rAnimationFrame.meDisposal)
                    {
                        case Disposal::Not:
                        {
                            maVirtualDevice->DrawBitmapEx(rAnimationFrame.maPositionPixel, rAnimationFrame.maBitmapEx);
                            AlphaMask aAlphaMask = rAnimationFrame.maBitmapEx.GetAlphaMask();

                            if (aAlphaMask.IsEmpty())
                            {
                                const Point aEmpty;
                                const ::tools::Rectangle aRect(aEmpty, maVirtualDeviceMask->GetOutputSizePixel());
                                const Wallpaper aWallpaper(COL_BLACK);
                                maVirtualDeviceMask->DrawWallpaper(aRect, aWallpaper);
                            }
                            else
                            {
                                BitmapEx aExpandVisibilityMask(aAlphaMask.GetBitmap(), aAlphaMask);
                                maVirtualDeviceMask->DrawBitmapEx(rAnimationFrame.maPositionPixel, aExpandVisibilityMask);
                            }

                            break;
                        }
                        case Disposal::Back:
                        {
                            // #i70772# react on no mask, for primitives, too.
                            const AlphaMask & rMask(rAnimationFrame.maBitmapEx.GetAlphaMask());

                            maVirtualDeviceMask->Erase();
                            maVirtualDevice->DrawBitmapEx(rAnimationFrame.maPositionPixel, rAnimationFrame.maBitmapEx);

                            if (rMask.IsEmpty())
                            {
                                const ::tools::Rectangle aRect(rAnimationFrame.maPositionPixel, rAnimationFrame.maBitmapEx.GetSizePixel());
                                maVirtualDeviceMask->SetFillColor(COL_BLACK);
                                maVirtualDeviceMask->SetLineColor();
                                maVirtualDeviceMask->DrawRect(aRect);
                            }
                            else
                            {
                                BitmapEx aExpandVisibilityMask(rMask.GetBitmap(), rMask);
                                maVirtualDeviceMask->DrawBitmapEx(rAnimationFrame.maPositionPixel, aExpandVisibilityMask);
                            }

                            break;
                        }
                        case Disposal::Previous:
                        {
                            maVirtualDevice->DrawBitmapEx(rAnimationFrame.maPositionPixel, rAnimationFrame.maBitmapEx);
                            BitmapEx aExpandVisibilityMask(rAnimationFrame.maBitmapEx.GetAlphaMask().GetBitmap(), rAnimationFrame.maBitmapEx.GetAlphaMask());
                            maVirtualDeviceMask->DrawBitmapEx(rAnimationFrame.maPositionPixel, aExpandVisibilityMask);
                            break;
                        }
                    }

                    // to not waste created data, check adding to buffers
                    checkSafeToBuffer(mnNextFrameToPrepare);

                    mnNextFrameToPrepare++;
                }
            }

            Primitive2DReference tryTogetFromBuffer(sal_uInt32 nIndex) const
            {
                if (mbBufferingAllowed)
                {
                    // all frames buffered, check if available
                    if (!maBufferedPrimitives.empty() && nIndex < maBufferedPrimitives.size())
                    {
                        if (maBufferedPrimitives[nIndex].is())
                        {
                            return maBufferedPrimitives[nIndex];
                        }
                    }
                }
                else
                {
                    // always buffer first frame, it's sometimes requested out-of-order
                    if (0 == nIndex && maBufferedFirstFrame.is())
                    {
                        return maBufferedFirstFrame;
                    }
                }

                return Primitive2DReference();
            }

        public:
            /// constructor
            AnimatedGraphicPrimitive2D(
                const Graphic& rGraphic,
                const drawinglayer::attribute::FillGraphicAttribute* pFillGraphicAttribute,
                basegfx::B2DHomMatrix aTransform,
                double fTransparency = 0.0);
            virtual ~AnimatedGraphicPrimitive2D();

            /// data read access
            const basegfx::B2DHomMatrix& getTransform() const { return maTransform; }
            double getTransparency() const { return mfTransparency; }

            /// provide unique ID
            virtual sal_uInt32 getPrimitive2DID() const override { return PRIMITIVE2D_ID_ANIMATEDGRAPHICPRIMITIVE2D; }

            /// compare operator
            virtual bool operator==(const BasePrimitive2D& rPrimitive) const override;

            /// override to deliver the correct expected frame dependent of timing
            virtual void get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const override;

            /// get range
            virtual basegfx::B2DRange getB2DRange(const geometry::ViewInformation2D& rViewInformation) const override;
        };

        }

        AnimatedGraphicPrimitive2D::AnimatedGraphicPrimitive2D(
            const Graphic& rGraphic,
            const drawinglayer::attribute::FillGraphicAttribute* pFillGraphicAttribute,
            basegfx::B2DHomMatrix aTransform,
            double fTransparency)
        :   AnimatedSwitchPrimitive2D(
                animation::AnimationEntryList(),
                Primitive2DContainer(),
                false),
            maTransform(std::move(aTransform)),
            maGraphic(rGraphic),
            maFillGraphicAttribute(),
            maAnimation(rGraphic.GetAnimation()),
            mfTransparency(std::max(0.0, std::min(1.0, fTransparency))),
            maVirtualDevice(*Application::GetDefaultDevice()),
            maVirtualDeviceMask(*Application::GetDefaultDevice()),
            mnNextFrameToPrepare(SAL_MAX_UINT32),
            mbBufferingAllowed(false),
            mbHugeSize(false)
        {
            // if FillGraphicAttribute copy it -> FillGraphicPrimitive2D is intended
            if (nullptr != pFillGraphicAttribute)
                maFillGraphicAttribute = *pFillGraphicAttribute;

            // initialize AnimationTiming, needed to detect which frame is requested
            // in get2DDecomposition
            createAndSetAnimationTiming();

            // check if we allow buffering
            if (isValidData())
            {
                // allow buffering up to a size of:
                // - 64 frames
                // - sizes of 256x256 pixels
                // This may be offered in option values if needed
                static const sal_uInt64 nAllowedSize(64 * 256 * 256);
                static const sal_uInt64 nHugeSize(10000000);
                const Size aTarget(maAnimation.GetDisplaySizePixel());
                const sal_uInt64 nUsedSize(static_cast<sal_uInt64>(maAnimation.Count()) * aTarget.Width() * aTarget.Height());

                if (nUsedSize < nAllowedSize)
                {
                    mbBufferingAllowed = true;
                }

                if (nUsedSize > nHugeSize)
                {
                    mbHugeSize = true;
                }
            }

            // prepare buffer space
            if (mbBufferingAllowed && isValidData())
            {
                maBufferedPrimitives.resize(maAnimation.Count());
            }
        }

        AnimatedGraphicPrimitive2D::~AnimatedGraphicPrimitive2D()
        {
            // Related: tdf#158807 mutex must be locked when disposing a VirtualDevice
            // If the following .ppt document is opened in a debug build
            // and the document is left open for a minute or two without
            // changing any content, this destructor will be called on a
            // non-main thread with the mutex unlocked:
            //   https://bugs.SnipeOffice.org/attachment.cgi?id=46801
            // This hits an assert in VirtualDevice::ReleaseGraphics() so
            // explicitly lock the mutex and explicitly dispose and clear
            // the VirtualDevice instances variables.
            const SolarMutexGuard aSolarGuard;

            maVirtualDevice.disposeAndClear();
            maVirtualDeviceMask.disposeAndClear();
            maAnimation.Clear();
            maGraphic.Clear();
        }

        bool AnimatedGraphicPrimitive2D::operator==(const BasePrimitive2D& rPrimitive) const
        {
            // do not use 'GroupPrimitive2D::operator==' here, that would compare
            // the children. Also do not use 'BasePrimitive2D::operator==', that would
            // check the ID-Type. Since we are a simple derivation without own ID,
            // use the dynamic_cast RTTI directly
            const AnimatedGraphicPrimitive2D* pCompare = dynamic_cast<const AnimatedGraphicPrimitive2D*>(&rPrimitive);

            // use operator== of Graphic - if that is equal, the basic definition is equal
            return (nullptr != pCompare
                && getTransform() == pCompare->getTransform()
                && maGraphic == pCompare->maGraphic);
        }

        void AnimatedGraphicPrimitive2D::get2DDecomposition(Primitive2DDecompositionVisitor& rVisitor, const geometry::ViewInformation2D& rViewInformation) const
        {
            if (!isValidData())
                return;

            Primitive2DReference aRetval;
            const double fState(getAnimationEntry().getStateAtTime(rViewInformation.getViewTime()));
            const sal_uInt32 nLen(maAnimation.Count());
            sal_uInt32 nIndex(basegfx::fround(fState * static_cast<double>(nLen)));

            // nIndex is the requested frame - it is in range [0..nLen[
            // create frame representation in VirtualDevices
            if (nIndex >= nLen)
            {
                nIndex = nLen - 1;
            }

            // check buffering shortcuts, may already be created
            aRetval = tryTogetFromBuffer(nIndex);

            if (aRetval.is())
            {
                rVisitor.visit(aRetval);
                return;
            }

            // if huge size (and not the buffered 1st frame) simply
            // create next frame
            if (mbHugeSize && 0 != nIndex && mnNextFrameToPrepare <= nIndex)
            {
                nIndex = mnNextFrameToPrepare % nLen;
            }

            // frame not (yet) buffered or no buffering allowed, create it
            const_cast<AnimatedGraphicPrimitive2D*>(this)->createFrame(nIndex);

            // try to get from buffer again, may have been added from createFrame
            aRetval = tryTogetFromBuffer(nIndex);

            if (aRetval.is())
            {
                rVisitor.visit(aRetval);
                return;
            }

            // did not work (not buffered and not 1st frame), create from buffer
            aRetval = createFromBuffer();

            rVisitor.visit(aRetval);
        }

        basegfx::B2DRange AnimatedGraphicPrimitive2D::getB2DRange(const geometry::ViewInformation2D& rViewInformation) const
        {
            // get object's range
            basegfx::B2DRange aUnitRange(0.0, 0.0, 1.0, 1.0);
            aUnitRange.transform(getTransform());

            // intersect with visible part
            aUnitRange.intersect(rViewInformation.getViewport());

            return aUnitRange;
        }

} // end of namespace

namespace drawinglayer::primitive2d
{
        Primitive2DReference createFillGraphicPrimitive2D(
            const basegfx::B2DHomMatrix& rTransform,
            const drawinglayer::attribute::FillGraphicAttribute& rFillGraphicAttribute,
            double fTransparency)
        {
            if (basegfx::fTools::equal(fTransparency, 1.0))
            {
                // completely transparent, done
                return nullptr;
            }

            const Graphic& rGraphic(rFillGraphicAttribute.getGraphic());
            const GraphicType aType(rGraphic.GetType());

            if (GraphicType::Bitmap == aType && rGraphic.IsAnimated())
            {
                return new AnimatedGraphicPrimitive2D(
                    rGraphic,
                    &rFillGraphicAttribute,
                    rTransform,
                    fTransparency);
            }

            return new FillGraphicPrimitive2D(
                rTransform,
                rFillGraphicAttribute,
                fTransparency);
        }

        void create2DDecompositionOfGraphic(
            Primitive2DContainer& rContainer,
            const Graphic& rGraphic,
            const basegfx::B2DHomMatrix& rTransform,
            double fTransparency)
        {
            if (basegfx::fTools::equal(fTransparency, 1.0))
            {
                // completely transparent, done
                return;
            }

            switch(rGraphic.GetType())
            {
                case GraphicType::Bitmap :
                {
                    if(rGraphic.IsAnimated())
                    {
                        // prepare specialized AnimatedGraphicPrimitive2D, now with
                        // support for alpha
                        rContainer.append(new AnimatedGraphicPrimitive2D(
                            rGraphic,
                            nullptr,
                            rTransform,
                            fTransparency));
                    }
                    else if(rGraphic.getVectorGraphicData())
                    {
                        // embedded Vector Graphic Data fill, create embed transform
                        const basegfx::B2DRange& rSvgRange(rGraphic.getVectorGraphicData()->getRange());

                        if(rSvgRange.getWidth() > 0.0 && rSvgRange.getHeight() > 0.0)
                        {
                            // translate back to origin, scale to unit coordinates
                            basegfx::B2DHomMatrix aEmbedVectorGraphic(
                                basegfx::utils::createTranslateB2DHomMatrix(
                                    -rSvgRange.getMinX(),
                                    -rSvgRange.getMinY()));

                            aEmbedVectorGraphic.scale(
                                1.0 / rSvgRange.getWidth(),
                                1.0 / rSvgRange.getHeight());

                            // apply created object transformation
                            aEmbedVectorGraphic = rTransform * aEmbedVectorGraphic;

                            // add Vector Graphic Data primitives embedded
                            rtl::Reference<BasePrimitive2D> aPrimitive(
                                new TransformPrimitive2D(
                                    aEmbedVectorGraphic,
                                    Primitive2DContainer(rGraphic.getVectorGraphicData()->getPrimitive2DSequence())));

                            // if needed embed to UnifiedTransparencePrimitive2D
                            if (!basegfx::fTools::equalZero(fTransparency, 0.0))
                                aPrimitive = new UnifiedTransparencePrimitive2D(
                                    Primitive2DContainer { aPrimitive }, fTransparency);

                            rContainer.append(aPrimitive);
                        }
                    }
                    else
                    {
                        // dependent of transparency used create the needed bitmap primitive
                        if (basegfx::fTools::equalZero(fTransparency))
                        {
                            rContainer.append(
                                new BitmapPrimitive2D(
                                    rGraphic.GetBitmapEx(),
                                    rTransform));
                        }
                        else
                        {
                            rContainer.append(
                                new BitmapAlphaPrimitive2D(
                                    rGraphic.GetBitmapEx(),
                                    rTransform,
                                    fTransparency));
                        }
                    }

                    break;
                }

                case GraphicType::GdiMetafile :
                {
                    // create MetafilePrimitive2D
                    const GDIMetaFile& rMetafile = rGraphic.GetGDIMetaFile();

                    rtl::Reference<BasePrimitive2D> aPrimitive(
                        new MetafilePrimitive2D(
                            rTransform,
                            rMetafile));

                    // #i100357# find out if clipping is needed for this primitive. Unfortunately,
                    // there exist Metafiles who's content is bigger than the proposed PrefSize set
                    // at them. This is an error, but we need to work around this
                    const Size aMetaFilePrefSize(rMetafile.GetPrefSize());
                    const Size aMetaFileRealSize(
                        rMetafile.GetBoundRect(
                            *Application::GetDefaultDevice()).GetSize());

                    if(aMetaFileRealSize.getWidth() > aMetaFilePrefSize.getWidth()
                        || aMetaFileRealSize.getHeight() > aMetaFilePrefSize.getHeight())
                    {
                        // clipping needed. Embed to MaskPrimitive2D. Create children and mask polygon
                        basegfx::B2DPolygon aMaskPolygon(basegfx::utils::createUnitPolygon());
                        aMaskPolygon.transform(rTransform);

                        aPrimitive = new MaskPrimitive2D(
                            basegfx::B2DPolyPolygon(aMaskPolygon),
                            Primitive2DContainer { aPrimitive });
                    }

                    // if needed embed to UnifiedTransparencePrimitive2D
                    if (!basegfx::fTools::equalZero(fTransparency, 0.0))
                        aPrimitive = new UnifiedTransparencePrimitive2D(
                            Primitive2DContainer { aPrimitive }, fTransparency);

                    rContainer.append(aPrimitive);
                    break;
                }

                default:
                {
                    // nothing to create
                    break;
                }
            }
        }

        Primitive2DContainer create2DColorModifierEmbeddingsAsNeeded(
            Primitive2DContainer&& rChildren,
            GraphicDrawMode aGraphicDrawMode,
            double fLuminance,
            double fContrast,
            double fRed,
            double fGreen,
            double fBlue,
            double fGamma,
            bool bInvert)
        {
            Primitive2DContainer aRetval;

            if(rChildren.empty())
            {
                // no child content, done
                return aRetval;
            }

            // set child content as retval; that is what will be used as child content in all
            // embeddings from here
            aRetval = std::move(rChildren);

            if(GraphicDrawMode::Watermark == aGraphicDrawMode)
            {
                // this is solved by applying fixed values additionally to luminance
                // and contrast, do it here and reset DrawMode to GraphicDrawMode::Standard
                // original in svtools uses:
                // #define WATERMARK_LUM_OFFSET        50
                // #define WATERMARK_CON_OFFSET        -70
                fLuminance = std::clamp(fLuminance + 0.5, -1.0, 1.0);
                fContrast = std::clamp(fContrast - 0.7, -1.0, 1.0);
                aGraphicDrawMode = GraphicDrawMode::Standard;
            }

            // DrawMode (GraphicDrawMode::Watermark already handled)
            switch(aGraphicDrawMode)
            {
                case GraphicDrawMode::Greys:
                {
                    // convert to grey
                    const Primitive2DReference aPrimitiveGrey(
                        new ModifiedColorPrimitive2D(
                            std::move(aRetval),
                            std::make_shared<basegfx::BColorModifier_gray>()));

                    aRetval = Primitive2DContainer { aPrimitiveGrey };
                    break;
                }
                case GraphicDrawMode::Mono:
                {
                    // convert to mono (black/white with threshold 0.5)
                    const Primitive2DReference aPrimitiveBlackAndWhite(
                        new ModifiedColorPrimitive2D(
                            std::move(aRetval),
                            std::make_shared<basegfx::BColorModifier_black_and_white>(0.5)));

                    aRetval = Primitive2DContainer { aPrimitiveBlackAndWhite };
                    break;
                }
                default: // case GraphicDrawMode::Standard:
                {
                    assert(
                        aGraphicDrawMode != GraphicDrawMode::Watermark
                        && "OOps, GraphicDrawMode::Watermark should already be handled (see above)");
                    // nothing to do
                    break;
                }
            }

            // mnContPercent, mnLumPercent, mnRPercent, mnGPercent, mnBPercent
            // handled in a single call
            if(!basegfx::fTools::equalZero(fLuminance)
                || !basegfx::fTools::equalZero(fContrast)
                || !basegfx::fTools::equalZero(fRed)
                || !basegfx::fTools::equalZero(fGreen)
                || !basegfx::fTools::equalZero(fBlue))
            {
                const Primitive2DReference aPrimitiveRGBLuminannceContrast(
                    new ModifiedColorPrimitive2D(
                        std::move(aRetval),
                        std::make_shared<basegfx::BColorModifier_RGBLuminanceContrast>(
                                fRed,
                                fGreen,
                                fBlue,
                                fLuminance,
                                fContrast)));

                aRetval = Primitive2DContainer { aPrimitiveRGBLuminannceContrast };
            }

            // gamma (boolean)
            if(!basegfx::fTools::equal(fGamma, 1.0))
            {
                const Primitive2DReference aPrimitiveGamma(
                    new ModifiedColorPrimitive2D(
                        std::move(aRetval),
                        std::make_shared<basegfx::BColorModifier_gamma>(
                                fGamma)));

                aRetval = Primitive2DContainer { aPrimitiveGamma };
            }

            // invert (boolean)
            if(bInvert)
            {
                const Primitive2DReference aPrimitiveInvert(
                    new ModifiedColorPrimitive2D(
                        std::move(aRetval),
                        std::make_shared<basegfx::BColorModifier_invert>()));

                aRetval = Primitive2DContainer { aPrimitiveInvert };
            }

            return aRetval;
        }

} // end of namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
