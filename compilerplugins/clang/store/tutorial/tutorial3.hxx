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

// Similar like for Tutorial2, but this time the base class is RewritePlugin.
class Tutorial3
    : public loplugin::FilteringRewritePlugin< Tutorial3 >
    {
    public:
        // One more argument for ctor.
        Tutorial3( const InstantiationData& data );
        virtual void run() override;
        // Will be called for every if statement.
        bool VisitIfStmt( const IfStmt* ifstmt );
    private:
        // Helper function to check if the statement is 'return false;' and
        // modify it if yes.
        void modifyReturnFalse( const Stmt* stmt );
    };

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
