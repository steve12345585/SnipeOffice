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

#include <com/sun/star/document/XDocumentPropertiesSupplier.hpp>
#include <com/sun/star/document/XDocumentProperties.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <osl/diagnose.h>

#include <doc.hxx>
#include <IDocumentContentOperations.hxx>
#include <IDocumentUndoRedo.hxx>
#include <IDocumentFieldsAccess.hxx>
#include <shellio.hxx>
#include <pam.hxx>
#include <swundo.hxx>
#include <acorrect.hxx>
#include <crsrsh.hxx>
#include <docsh.hxx>

using namespace ::com::sun::star;

void SwDoc::ReplaceUserDefinedDocumentProperties(
        const uno::Reference<document::XDocumentProperties>& xSourceDocProps)
{
    OSL_ENSURE(xSourceDocProps.is(), "null reference");

    SwDocShell* pShell = GetDocShell();
    if (!pShell)
        return;

    uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
        pShell->GetModel(), uno::UNO_QUERY_THROW);
    uno::Reference<document::XDocumentProperties> xDocProps(
        xDPS->getDocumentProperties() );
    OSL_ENSURE(xDocProps.is(), "null reference");

    uno::Reference<beans::XPropertySet> xSourceUDSet(
        xSourceDocProps->getUserDefinedProperties(), uno::UNO_QUERY_THROW);
    uno::Reference<beans::XPropertyContainer> xTargetUD(
        xDocProps->getUserDefinedProperties());
    uno::Reference<beans::XPropertySet> xTargetUDSet(xTargetUD,
        uno::UNO_QUERY_THROW);
    const uno::Sequence<beans::Property> tgtprops
        = xTargetUDSet->getPropertySetInfo()->getProperties();

    for (const auto& rTgtProp : tgtprops) {
        try {
            xTargetUD->removeProperty(rTgtProp.Name);
        } catch (uno::Exception &) {
            // ignore
        }
    }

    uno::Reference<beans::XPropertySetInfo> xSetInfo
        = xSourceUDSet->getPropertySetInfo();
    const uno::Sequence<beans::Property> srcprops = xSetInfo->getProperties();

    for (const auto& rSrcProp : srcprops) {
        try {
            OUString name = rSrcProp.Name;
            xTargetUD->addProperty(name, rSrcProp.Attributes,
                xSourceUDSet->getPropertyValue(name));
        } catch (uno::Exception &) {
            // ignore
        }
    }
}

void SwDoc::ReplaceDocumentProperties(const SwDoc& rSource, bool mailMerge)
{
    SwDocShell* pShell = GetDocShell();
    const SwDocShell* pSourceShell = rSource.GetDocShell();
    if (!pShell || !pSourceShell)
        return;
    uno::Reference<document::XDocumentPropertiesSupplier> xSourceDPS(
        pSourceShell->GetModel(), uno::UNO_QUERY_THROW);
    uno::Reference<document::XDocumentProperties> xSourceDocProps(
        xSourceDPS->getDocumentProperties() );
    OSL_ENSURE(xSourceDocProps.is(), "null reference");

    uno::Reference<document::XDocumentPropertiesSupplier> xDPS(
        pShell->GetModel(), uno::UNO_QUERY_THROW);
    uno::Reference<document::XDocumentProperties> xDocProps(
        xDPS->getDocumentProperties() );
    OSL_ENSURE(xDocProps.is(), "null reference");

    xDocProps->setAuthor(xSourceDocProps->getAuthor());
    xDocProps->setGenerator(xSourceDocProps->getGenerator());
    xDocProps->setCreationDate(xSourceDocProps->getCreationDate());
    xDocProps->setTitle(xSourceDocProps->getTitle());
    xDocProps->setSubject(xSourceDocProps->getSubject());
    xDocProps->setDescription(xSourceDocProps->getDescription());
    xDocProps->setKeywords(xSourceDocProps->getKeywords());
    xDocProps->setLanguage(xSourceDocProps->getLanguage());
    // Note: These below originally weren't copied for mailmerge, but I don't see why not.
    xDocProps->setModifiedBy(xSourceDocProps->getModifiedBy());
    xDocProps->setModificationDate(xSourceDocProps->getModificationDate());
    xDocProps->setPrintedBy(xSourceDocProps->getPrintedBy());
    xDocProps->setPrintDate(xSourceDocProps->getPrintDate());
    xDocProps->setTemplateName(xSourceDocProps->getTemplateName());
    xDocProps->setTemplateURL(xSourceDocProps->getTemplateURL());
    xDocProps->setTemplateDate(xSourceDocProps->getTemplateDate());
    xDocProps->setAutoloadURL(xSourceDocProps->getAutoloadURL());
    xDocProps->setAutoloadSecs(xSourceDocProps->getAutoloadSecs());
    xDocProps->setDefaultTarget(xSourceDocProps->getDefaultTarget());
    xDocProps->setDocumentStatistics(xSourceDocProps->getDocumentStatistics());
    xDocProps->setEditingCycles(xSourceDocProps->getEditingCycles());
    xDocProps->setEditingDuration(xSourceDocProps->getEditingDuration());

    if( mailMerge ) // Note: Not sure this is needed.
    {
        // Manually set the creation date, otherwise author field isn't filled
        // during MM, as it's set when saving the document the first time.
        xDocProps->setCreationDate( xSourceDocProps->getModificationDate() );
    }

    ReplaceUserDefinedDocumentProperties( xSourceDocProps );
}

/// inserts an AutoText block
bool SwDoc::InsertGlossary( SwTextBlocks& rBlock, const OUString& rEntry,
                            SwPaM& rPaM, SwCursorShell* pShell )
{
    bool bRet = false;
    const sal_uInt16 nIdx = rBlock.GetIndex( rEntry );
    if( USHRT_MAX != nIdx )
    {
        bool bSav_IsInsGlossary = mbInsOnlyTextGlssry;
        mbInsOnlyTextGlssry = rBlock.IsOnlyTextBlock( nIdx );

        if( rBlock.BeginGetDoc( nIdx ) )
        {
            SwDoc* pGDoc = rBlock.GetDoc();

            // tdf#53023 - remove the last empty paragraph (check SwXMLTextBlockParContext dtor)
            if (mbInsOnlyTextGlssry)
            {
                SwPaM aPaM(*pGDoc->GetNodes()[pGDoc->GetNodes().GetEndOfContent().GetIndex() - 1]);
                pGDoc->getIDocumentContentOperations().DelFullPara(aPaM);
            }

            // Update all fixed fields, with the right DocInfo.
            // FIXME: UGLY: Because we cannot limit the range in which to do
            // field updates, we must update the fixed fields at the glossary
            // entry document.
            // To be able to do this, we copy the document properties of the
            // target document to the glossary document
            // OSL_ENSURE(GetDocShell(), "no SwDocShell"); // may be clipboard!
            OSL_ENSURE(pGDoc->GetDocShell(), "no SwDocShell at glossary");
            if (GetDocShell() && pGDoc->GetDocShell())
                pGDoc->ReplaceDocumentProperties( *this );
            pGDoc->getIDocumentFieldsAccess().SetFixFields(nullptr);

            // StartAllAction();
            getIDocumentFieldsAccess().LockExpFields();

            SwNodeIndex aStt( pGDoc->GetNodes().GetEndOfExtras(), 1 );
            SwContentNode* pContentNd = SwNodes::GoNext(&aStt);
            const SwTableNode* pTableNd = pContentNd->FindTableNode();
            SwPaM aCpyPam( pTableNd ? *const_cast<SwNode*>(static_cast<SwNode const *>(pTableNd)) : *static_cast<SwNode*>(pContentNd) );
            aCpyPam.SetMark();

            // till the nodes array's end
            aCpyPam.GetPoint()->Assign( pGDoc->GetNodes().GetEndOfContent().GetIndex()-SwNodeOffset(1) );
            pContentNd = aCpyPam.GetPointContentNode();
            if (pContentNd)
                aCpyPam.GetPoint()->SetContent( pContentNd->Len() );

            GetIDocumentUndoRedo().StartUndo( SwUndoId::INSGLOSSARY, nullptr );
            SwPaM *_pStartCursor = &rPaM, *_pStartCursor2 = _pStartCursor;
            do {

                SwPosition& rInsPos = *_pStartCursor->GetPoint();
                SwStartNode* pBoxSttNd = const_cast<SwStartNode*>(rInsPos.GetNode().
                                            FindTableBoxStartNode());

                if( pBoxSttNd && SwNodeOffset(2) == pBoxSttNd->EndOfSectionIndex() -
                                      pBoxSttNd->GetIndex() &&
                    aCpyPam.GetPoint()->GetNode() != aCpyPam.GetMark()->GetNode() )
                {
                    // We copy more than one Node to the current Box.
                    // However, we have to remove the BoxAttributes then.
                    ClearBoxNumAttrs( rInsPos.GetNode() );
                }

                SwDontExpandItem aACD;
                aACD.SaveDontExpandItems( rInsPos );

                pGDoc->getIDocumentContentOperations().CopyRange(aCpyPam, rInsPos, SwCopyFlags::CheckPosInFly);

                aACD.RestoreDontExpandItems( rInsPos );
                if( pShell )
                    pShell->SaveTableBoxContent( &rInsPos );
            } while( (_pStartCursor = _pStartCursor->GetNext()) !=
                        _pStartCursor2 );
            GetIDocumentUndoRedo().EndUndo( SwUndoId::INSGLOSSARY, nullptr );

            getIDocumentFieldsAccess().UnlockExpFields();
            if( !getIDocumentFieldsAccess().IsExpFieldsLocked() )
                getIDocumentFieldsAccess().UpdateExpFields(nullptr, true);
            bRet = true;
        }
        mbInsOnlyTextGlssry = bSav_IsInsGlossary;
    }
    rBlock.EndGetDoc();
    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
