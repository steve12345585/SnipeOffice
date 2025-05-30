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

class BodyNotInBlock
    : public loplugin::FilteringPlugin<BodyNotInBlock>
    {
    public:
        explicit BodyNotInBlock( const InstantiationData& data );
        virtual void run() override;
        bool VisitIfStmt( const IfStmt* stmt );
        bool VisitWhileStmt( const WhileStmt* stmt );
        bool VisitForStmt( const ForStmt* stmt );
        bool VisitCXXForRangeStmt( const CXXForRangeStmt* stmt );
    private:
        void checkBody( const Stmt* body, SourceLocation stmtLocation, int stmtType, bool dontGoUp = false );
    };

} // namespace

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
