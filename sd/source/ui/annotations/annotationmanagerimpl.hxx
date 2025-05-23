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

#include <com/sun/star/document/XEventListener.hpp>

#include <rtl/ustring.hxx>

#include <comphelper/compbase.hxx>

namespace com::sun::star::drawing { class XDrawView; }
namespace com::sun::star::office { class XAnnotationAccess; }
namespace com::sun::star::office { class XAnnotation; }

class SfxRequest;
class SdrObject;
class SdPage;
class SdDrawDocument;
struct ImplSVEvent;

namespace sdr::annotation { class Annotation; }

namespace sd
{
class Annotation;
class ViewShellBase;
class View;
class DrawController;

namespace tools { class EventMultiplexerEvent; }

typedef comphelper::WeakComponentImplHelper <
    css::document::XEventListener
    > AnnotationManagerImplBase;

class AnnotationManagerImpl : public AnnotationManagerImplBase
{
public:
    explicit AnnotationManagerImpl( ViewShellBase& rViewShellBase );

    void init();

    // WeakComponentImplHelper
    virtual void disposing (std::unique_lock<std::mutex>&) override;

    // XEventListener
    virtual void SAL_CALL notifyEvent( const css::document::EventObject& Event ) override;
    virtual void SAL_CALL disposing( const css::lang::EventObject& Source ) override;

    void ExecuteAnnotation (SfxRequest const & rRequest);
    void GetAnnotationState (SfxItemSet& rItemSet);

    void ExecuteInsertAnnotation(SfxRequest const & rReq);
    void ExecuteDeleteAnnotation(SfxRequest const & rReq);
    void ExecuteEditAnnotation(SfxRequest const & rReq);
    void ExecuteReplyToAnnotation(SfxRequest const & rReq);

    void SelectNextAnnotation(bool bForward);

    void SelectAnnotation(rtl::Reference<sdr::annotation::Annotation> const& xAnnotation, bool bEdit = false);
    void GetSelectedAnnotation(rtl::Reference<sdr::annotation::Annotation>& xAnnotation);

    void InsertAnnotation(const OUString& rText);
    void DeleteAnnotation(rtl::Reference<sdr::annotation::Annotation> const& xAnnotation);
    void DeleteAnnotationsByAuthor( std::u16string_view sAuthor );
    void DeleteAllAnnotations();

    static Color GetColorDark(sal_uInt16 aAuthorIndex);
    static Color GetColorLight(sal_uInt16 aAuthorIndex);
    static Color GetColor(sal_uInt16 aAuthorIndex);

    void onSelectionChanged();

    void addListener();
    void removeListener();

    void invalidateSlots();

    DECL_LINK(EventMultiplexerListener, tools::EventMultiplexerEvent&, void);
    DECL_LINK(UpdateTagsHdl, void *, void);

    void UpdateTags(bool bSynchron = false);
    void SyncAnnotationObjects();

    SdPage* GetNextPage( SdPage const * pPage, bool bForward );

    SdPage* GetCurrentPage();

    void ShowAnnotations(bool bShow);

private:
    ViewShellBase& mrBase;
    SdDrawDocument* mpDoc;

    rtl::Reference< ::sd::DrawController > mxView;
    rtl::Reference<SdPage> mxCurrentPage;
    rtl::Reference<sdr::annotation::Annotation> mxSelectedAnnotation;

    bool mbShowAnnotations;
    ImplSVEvent * mnUpdateTagsEvent;

    rtl::Reference<sdr::annotation::Annotation> GetAnnotationById(sal_uInt32 nAnnotationId);
};

OUString getAnnotationDateTimeString( const css::uno::Reference< css::office::XAnnotation >& xAnnotation );

SfxItemPool* GetAnnotationPool();

css::util::DateTime getCurrentDateTime();

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
