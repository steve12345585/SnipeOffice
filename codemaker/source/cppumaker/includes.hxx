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

#include <rtl/ref.hxx>
#include <rtl/ustring.hxx>
#include <string_view>
#include <vector>
#include "dependencies.hxx"

class FileStream;
class TypeManager;

namespace codemaker::cppumaker {
enum class FileType;

class Includes {
public:
    Includes(
        rtl::Reference< TypeManager > manager,
        Dependencies const & dependencies, FileType eFileType);

    ~Includes();

    void add(OString const & entityName);
    void addCassert() { m_includeCassert = true; }
    void addAny() { m_includeAny = true; }
    void addReference() { m_includeReference = true; }
    void addSequence() { m_includeSequence = true; }
    void addType() { m_includeType = true; }
    void addCppuMacrosHxx() { m_includeCppuMacrosHxx = true; }
    void addCppuUnotypeHxx() { m_includeCppuUnotypeHxx = true; }
    void addOslMutexHxx() { m_includeOslMutexHxx = true; }
    void addRtlStrbufHxx() { m_includeRtlStrbufHxx = true; }
    void addRtlStringH() { m_includeRtlStringH = true; }
    void addRtlTextencH() { m_includeRtlTextencH = true; }
    void addRtlUstrbufHxx() { m_includeRtlUstrbufHxx = true; }
    void addRtlUstringH() { m_includeRtlUstringH = true; }
    void addRtlUstringHxx() { m_includeRtlUstringHxx = true; }
    void addRtlInstanceHxx() { m_includeRtlInstanceHxx = true; }
    void addSalTypesH() { m_includeSalTypesH = true; }
    void addTypelibTypeclassH() { m_includeTypelibTypeclassH = true; }
    void addTypelibTypedescriptionH()
        { m_includeTypelibTypedescriptionH = true; }
    void addCustom(const OUString& s) { m_custom.push_back(s); }
    void dump(
        FileStream & out, OUString const * companionHdl, bool exceptions);

    static void dumpInclude(
        FileStream & out, OString const & entityName, bool hpp);

private:
    Includes(Includes const &) = delete;
    Includes& operator =(const Includes&) = delete;

    bool isInterfaceType(std::string_view entityName) const;

    rtl::Reference< TypeManager > m_manager;
    Dependencies::Map m_map;
    FileType m_filetype;
    bool m_includeCassert;
    bool m_includeAny;
    bool m_includeReference;
    bool m_includeSequence;
    bool m_includeType;
    bool m_includeCppuMacrosHxx;
    bool m_includeCppuUnotypeHxx;
    bool m_includeOslMutexHxx;
    bool m_includeRtlStrbufHxx;
    bool m_includeRtlStringH;
    bool m_includeRtlTextencH;
    bool m_includeRtlUstrbufHxx;
    bool m_includeRtlUstringH;
    bool m_includeRtlUstringHxx;
    bool m_includeRtlInstanceHxx;
    bool m_includeSalTypesH;
    bool m_includeTypelibTypeclassH;
    bool m_includeTypelibTypedescriptionH;
    std::vector<OUString> m_custom;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
