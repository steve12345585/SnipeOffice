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

#pragma once

#include <codemaker/global.hxx>
#include <unordered_map>
#include <utility>

typedef std::unordered_map
<
    ::rtl::OString,
    ::rtl::OString,
    OStringHash
> OptionMap;

class IllegalArgument
{
public:
    IllegalArgument(::rtl::OString msg)
        : m_message(std::move(msg)) {}

    ::rtl::OString  m_message;
};

class Options
{
public:
    Options();
    virtual ~Options();

    /// @throws IllegalArgument
    virtual bool initOptions(int ac, char* av[], bool bCmdFile=false) = 0;

    virtual ::rtl::OString  prepareHelp() = 0;

    const ::rtl::OString&   getProgramName() const { return m_program;}
    bool                isValid(const ::rtl::OString& option) const;
    /// @throws IllegalArgument
    const OString&      getOption(const ::rtl::OString& option) const;

    const StringVector& getInputFiles() const { return m_inputFiles;}

    const StringVector& getExtraInputFiles() const
        { return m_extra_input_files; }
protected:
    ::rtl::OString  m_program;
    StringVector    m_inputFiles;
    StringVector    m_extra_input_files;
    OptionMap       m_options;
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
