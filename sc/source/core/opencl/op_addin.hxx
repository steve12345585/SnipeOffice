/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "opbase.hxx"
#include "utils.hxx"

namespace sc::opencl {

class OpBesselj: public Normal
{
public:
    virtual void GenSlidingWindowFunction(outputstream &ss,
        const std::string &sSymName, SubArguments &vSubArguments) override;
    virtual std::string BinFuncName() const override { return "Besselj"; }
};
class OpGestep: public Normal
{
public:
    virtual void GenSlidingWindowFunction(outputstream &ss,
        const std::string &sSymName, SubArguments &vSubArguments) override;
    virtual std::string BinFuncName() const override { return "Gestep"; }
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
