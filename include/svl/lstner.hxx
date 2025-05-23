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
#ifndef INCLUDED_SVL_LSTNER_HXX
#define INCLUDED_SVL_LSTNER_HXX

#include <svl/svldllapi.h>
#include <map>
#include <memory>
#include <vector>
#ifdef DBG_UTIL
#include <sal/backtrace.hxx>
#endif

class SfxBroadcaster;
class SfxHint;

//StartListening duplicate handling options
//Prevent only adds the listener if it's not already added
//Allow allows duplicate listeners
//Unexpected, the default, is for the usual case where the
//listener should only be added once and duplicates are
//unexpected. In dbgutil mode this tracks where the original
//listener was added from and reports the duplicate addition
//as an error
enum class DuplicateHandling { Unexpected, Prevent, Allow };

class SVL_DLLPUBLIC SfxListener
{
    std::vector<SfxBroadcaster*> maBCs;
#ifdef DBG_UTIL
    std::map<SfxBroadcaster*, std::unique_ptr<sal::BacktraceState>>
        maCallStacks;
#endif

private:
    const SfxListener&  operator=(const SfxListener &) = delete;

public:

                        SfxListener() {}
                        SfxListener( const SfxListener &rCopy );
    virtual             ~SfxListener() COVERITY_NOEXCEPT_FALSE;

    void                StartListening(SfxBroadcaster& rBroadcaster, DuplicateHandling eDuplicateHanding = DuplicateHandling::Unexpected);
    void                EndListening( SfxBroadcaster& rBroadcaster, bool bRemoveAllDuplicates = false );
    void                EndListeningAll();
    bool                IsListening( SfxBroadcaster& rBroadcaster ) const;

    sal_uInt16          GetBroadcasterCount() const;
    SfxBroadcaster*     GetBroadcasterJOE( sal_uInt16 nNo ) const;

    virtual void        Notify( SfxBroadcaster& rBC, const SfxHint& rHint );

    void                RemoveBroadcaster_Impl( SfxBroadcaster& rBC );

    /// Used to avoid cost of dynamic_cast in SdrViewIter::ImpFindView
    virtual bool        IsSdrView() const { return false; }
    /// Used to avoid cost of dynamic_cast in SwXStyleFamily::FindStyle
    virtual bool        IsSwXStyle() const { return false; }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
