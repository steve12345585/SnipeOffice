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

#ifndef INCLUDED_SFX2_DOCINSERT_HXX
#define INCLUDED_SFX2_DOCINSERT_HXX

#include <rtl/ustring.hxx>
#include <sfx2/dllapi.h>
#include <comphelper/errcode.hxx>
#include <tools/link.hxx>
#include <memory>
#include <vector>

namespace sfx2 { class FileDialogHelper; }
namespace weld { class Window; }
class SfxItemSet;
class SfxMedium;
enum class FileDialogFlags;

typedef ::std::vector< std::unique_ptr<SfxMedium> > SfxMediumList;

namespace sfx2 {

class SFX2_DLLPUBLIC DocumentInserter
{
private:
    weld::Window*           m_pParent;
    OUString                m_sDocFactory;
    OUString                m_sFilter;
    Link<sfx2::FileDialogHelper*,void> m_aDialogClosedLink;

    FileDialogFlags const m_nDlgFlags;
    ErrCode                 m_nError;

    std::unique_ptr<sfx2::FileDialogHelper>
                            m_pFileDlg;
    std::shared_ptr<SfxItemSet> m_xItemSet;
    std::vector<OUString>   m_pURLList;

    DECL_DLLPRIVATE_LINK(DialogClosedHdl, sfx2::FileDialogHelper*, void);

public:
    enum class Mode {
        Insert,
        InsertMulti,
        Compare,
        Merge
    };
    DocumentInserter(weld::Window* pParent, OUString aFactory, const Mode mode = Mode::Insert);
    ~DocumentInserter();

    void                    StartExecuteModal( const Link<sfx2::FileDialogHelper*,void>& _rDialogClosedLink );
    std::unique_ptr<SfxMedium> CreateMedium(char const* pFallbackHack = nullptr);
    SfxMediumList CreateMediumList();
};

} // namespace sfx2

#endif // INCLUDED_SFX2_DOCINSERT_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
