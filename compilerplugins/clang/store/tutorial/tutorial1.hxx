/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * Based on LLVM/Clang.
 *
 * This file is distributed under the University of Illinois Open Source
 * License. See LICENSE.TXT for details.
 *
 */

#pragma once

#include "plugin.hxx"

namespace loplugin
{

// The class implementing the plugin action.
class Tutorial1
    // Inherits from the Clang class that will allow examining the Clang AST tree (i.e. syntax tree).
    : public FilteringPlugin< Tutorial1 >
    {
    public:
        // Ctor, nothing special.
        Tutorial1( const InstantiationData& data );
        // The function that will be called to perform the actual action.
        virtual void run() override;
        // Function from Clang, it will be called for every return statement in the source.
        bool VisitReturnStmt( const ReturnStmt* returnstmt );
    };

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
