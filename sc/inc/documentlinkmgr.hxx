/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <memory>
#include <string_view>

class SfxObjectShell;
namespace weld { class Window; }

namespace sfx2 {

class LinkManager;

}

namespace sc {

class DataStream;
struct DocumentLinkManagerImpl;

class DocumentLinkManager
{
    std::unique_ptr<DocumentLinkManagerImpl> mpImpl;

public:
    DocumentLinkManager( SfxObjectShell* pShell );
    DocumentLinkManager(const DocumentLinkManager&) = delete;
    const DocumentLinkManager& operator=(const DocumentLinkManager&) = delete;
    ~DocumentLinkManager();

    void setDataStream( DataStream* p );
    DataStream* getDataStream();
    const DataStream* getDataStream() const;

    /**
     * @param bCreate if true, create a new link manager instance in case one
     *                does not exist.
     *
     * @return link manager instance.
     */
    sfx2::LinkManager* getLinkManager( bool bCreate = true );

    const sfx2::LinkManager* getExistingLinkManager() const;

    bool idleCheckLinks();

    bool hasDdeLinks() const;
    bool hasDdeOrOleOrWebServiceLinks() const;
    bool hasExternalRefLinks() const;

    bool updateDdeOrOleOrWebServiceLinks(weld::Window* pWin);

    void updateDdeLink( std::u16string_view rAppl, std::u16string_view rTopic, std::u16string_view rItem );

    size_t getDdeLinkCount() const;

private:
    bool hasDdeOrOleOrWebServiceLinks(bool bDde, bool bOle, bool bWebService) const;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
