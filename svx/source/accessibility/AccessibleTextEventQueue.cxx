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

#include <memory>
#include "AccessibleTextEventQueue.hxx"

#include <editeng/unoedhlp.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdpntv.hxx>

namespace accessibility
{


    // EventQueue implementation


    AccessibleTextEventQueue::AccessibleTextEventQueue()
    {
    }

    AccessibleTextEventQueue::~AccessibleTextEventQueue()
    {
        Clear();
    }

    void AccessibleTextEventQueue::Append( const SdrHint& rHint )
    {
        // only enqueue the events we actually care about in
        // AccessibleTextHelper_Impl::ProcessQueue(), because
        // the cost of some events adds up.
        auto eKind = rHint.GetKind();
        if (eKind == SdrHintKind::BeginEdit
            || eKind == SdrHintKind::EndEdit)
            maEventQueue.push_back( new SdrHint( rHint ) );
    }

    void AccessibleTextEventQueue::Append( const TextHint& rHint )
    {
        maEventQueue.push_back( new TextHint( rHint ) );
    }

    void AccessibleTextEventQueue::Append( const SvxViewChangedHint& rHint )
    {
        maEventQueue.push_back( new SvxViewChangedHint( rHint ) );
    }

    void AccessibleTextEventQueue::Append( const SvxEditSourceHint& rHint )
    {
        maEventQueue.push_back( new SvxEditSourceHint( rHint ) );
    }

    ::std::unique_ptr< SfxHint > AccessibleTextEventQueue::PopFront()
    {
        ::std::unique_ptr< SfxHint > aRes( *(maEventQueue.begin()) );
        maEventQueue.pop_front();
        return aRes;
    }

    bool AccessibleTextEventQueue::IsEmpty() const
    {
        return maEventQueue.empty();
    }

    void AccessibleTextEventQueue::Clear()
    {
        // clear queue
        for( auto p : maEventQueue)
            delete p;
        maEventQueue.clear();
    }

} // end of namespace accessibility


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
