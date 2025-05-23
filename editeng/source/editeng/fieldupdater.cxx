/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <memory>
#include <editeng/fieldupdater.hxx>
#include <editeng/flditem.hxx>
#include "editobj2.hxx"

#include <com/sun/star/text/textfield/Type.hpp>

using namespace com::sun::star;

namespace editeng {

SvxFieldItemUpdater::~SvxFieldItemUpdater() {}

namespace {
class SvxFieldItemUpdaterImpl : public SvxFieldItemUpdater
{
    SfxPoolItemHolder& mrItemHolder;
public:
    SvxFieldItemUpdaterImpl(SfxPoolItemHolder& rHolder) : mrItemHolder(rHolder) {}

    virtual void SetItem(const SvxFieldItem& rNewItem)
    {
        mrItemHolder = SfxPoolItemHolder(mrItemHolder.getPool(), &rNewItem, false);
    }
};
}

class FieldUpdaterImpl
{
    EditTextObjectImpl& mrObj;
public:
    explicit FieldUpdaterImpl(EditTextObject& rObj) : mrObj(toImpl(rObj)) {}

    void updateTableFields(int nTab)
    {
        SfxItemPool* pPool = mrObj.GetPool();
        EditTextObjectImpl::ContentInfosType& rContents = mrObj.GetContents();
        for (std::unique_ptr<ContentInfo> & i : rContents)
        {
            ContentInfo& rContent = *i;
            for (XEditAttribute & rAttr : rContent.GetCharAttribs())
            {
                const SfxPoolItem* pItem = rAttr.GetItem();
                if (pItem->Which() != EE_FEATURE_FIELD)
                    // This is not a field item.
                    continue;

                const SvxFieldItem* pFI = static_cast<const SvxFieldItem*>(pItem);
                const SvxFieldData* pData = pFI->GetField();
                if (pData->GetClassId() != text::textfield::Type::TABLE)
                    // This is not a table field.
                    continue;

                // Create a new table field with the new ID, and set it to the
                // attribute object.
                SvxFieldItem aNewItem(SvxTableField(nTab), EE_FEATURE_FIELD);
                rAttr.SetItem(*pPool, aNewItem);
            }
        }
    }

    void UpdatePageRelativeURLs(const std::function<void(const SvxFieldItem & rFieldItem, SvxFieldItemUpdater& rFieldItemUpdater)>& rItemCallback)
    {
        EditTextObjectImpl::ContentInfosType& rContents = mrObj.GetContents();
        for (std::unique_ptr<ContentInfo> & i : rContents)
        {
            ContentInfo& rContent = *i;
            for (XEditAttribute & rAttr : rContent.GetCharAttribs())
            {
                const SfxPoolItem* pItem = rAttr.GetItem();
                if (pItem->Which() != EE_FEATURE_FIELD)
                    // This is not a field item.
                    continue;
                SvxFieldItemUpdaterImpl aUpdater(rAttr.GetItemHolder());
                rItemCallback(static_cast<const SvxFieldItem&>(*pItem), aUpdater);
            }
        }
    }
};

FieldUpdater::FieldUpdater(EditTextObject& rObj) : mpImpl(new FieldUpdaterImpl(rObj)) {}
FieldUpdater::FieldUpdater(const FieldUpdater& r) : mpImpl(new FieldUpdaterImpl(*r.mpImpl)) {}

FieldUpdater::~FieldUpdater()
{
}

void FieldUpdater::updateTableFields(int nTab)
{
    mpImpl->updateTableFields(nTab);
}

void FieldUpdater::UpdatePageRelativeURLs(const std::function<void(const SvxFieldItem & rFieldItem, SvxFieldItemUpdater& rFieldItemUpdater)>& rItemCallback)
{
    mpImpl->UpdatePageRelativeURLs(rItemCallback);
}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
