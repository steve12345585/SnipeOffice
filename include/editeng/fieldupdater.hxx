/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_EDITENG_FIELDUPDATER_HXX
#define INCLUDED_EDITENG_FIELDUPDATER_HXX

#include <editeng/editengdllapi.h>
#include <editeng/flditem.hxx>
#include <svl/itempool.hxx>
#include <memory>

class EditTextObject;

namespace editeng
{
class FieldUpdaterImpl;
class SvxFieldItemUpdater;

/**
 * Wrapper for EditTextObject to handle updating of fields without exposing
 * the internals of EditTextObject structure.
 */
class EDITENG_DLLPUBLIC FieldUpdater
{
    std::unique_ptr<FieldUpdaterImpl> mpImpl;

public:
    FieldUpdater(EditTextObject& rObj);
    FieldUpdater(const FieldUpdater& r);
    ~FieldUpdater();

    /**
     * Set a new table ID to all table fields.
     *
     * @param nTab new table ID
     */
    void updateTableFields(int nTab);

    void UpdatePageRelativeURLs(
        const std::function<void(const SvxFieldItem& rFieldItem,
                                 SvxFieldItemUpdater& rFieldItemUpdater)>& rItemCallback);
};

// helper for updating the items we find via UpdatePageRelativeURLs
class EDITENG_DLLPUBLIC SvxFieldItemUpdater
{
public:
    virtual ~SvxFieldItemUpdater();

    // write-access when SvxFieldItem needs to be modified
    virtual void SetItem(const SvxFieldItem&) = 0;
};
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
