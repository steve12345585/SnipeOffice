/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sal/config.h>
#include <sal/log.hxx>

#include <map>
#include <utility>
#include <vector>

#include <osl/file.h>
#include <osl/file.hxx>
#include <rtl/character.hxx>
#include <rtl/ref.hxx>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>
#include <unoidl/unoidl.hxx>

#include "sourceprovider-scanner.hxx"
#include "sourcetreeprovider.hxx"

#if defined MACOSX
#include <dirent.h>
#include <osl/thread.h>
#endif

namespace unoidl::detail {

namespace {

//TODO: Bad hack to work around osl::FileStatus::getFileName not determining the
// original spelling of a file name (not even with
// osl_FileStatus_Mask_Validate):
OUString getFileName(OUString const & uri, osl::FileStatus const & status) {
#if defined MACOSX
    sal_Int32 i = uri.lastIndexOf('/') + 1;
    OUString path;
    if (osl::FileBase::getSystemPathFromFileURL(uri.copy(0, i), path)
        != osl::FileBase::E_None)
    {
        SAL_WARN(
            "unoidl",
            "cannot getSystemPathFromFileURL(" << uri.copy(0, i) << ")");
        return status.getFileName();
    }
    OString dir(OUStringToOString(path, osl_getThreadTextEncoding()));
    OString name(OUStringToOString(uri.subView(i), osl_getThreadTextEncoding()));
    DIR * d = opendir(dir.getStr());
    if (d == nullptr) {
        SAL_WARN("unoidl", "cannot opendir(" << dir << ")");
        return status.getFileName();
    }
    for (;;) {
        dirent ent;
        dirent * p;
        int e = readdir_r(d, &ent, &p);
        if (e != 0) {
            SAL_WARN("unoidl", "cannot readdir_r");
            closedir(d);
            return status.getFileName();
        }
        if (p == nullptr) {
            SAL_WARN(
                "unoidl", "cannot find " << name << " via readdir of " << dir);
            closedir(d);
            return status.getFileName();
        }
        if (name.equalsIgnoreAsciiCase(p->d_name)) {
            closedir(d);
            return OUString(
                p->d_name, std::strlen(p->d_name), osl_getThreadTextEncoding());
        }
    }
#else
    (void) uri;
    return status.getFileName();
#endif
}

bool exists(OUString const & uri, bool directory) {
    osl::DirectoryItem item;
    osl::FileStatus status(
        osl_FileStatus_Mask_Type | osl_FileStatus_Mask_FileName);
    return osl::DirectoryItem::get(uri, item) == osl::FileBase::E_None
        && item.getFileStatus(status) == osl::FileBase::E_None
        && (status.getFileType() == osl::FileStatus::Directory) == directory
        && getFileName(uri, status) == uri.subView(uri.lastIndexOf('/') + 1);
}

class Cursor: public MapCursor {
public:
    Cursor(Manager& manager, OUString const & uri): manager_(manager), directory_(uri) {
        auto const rc = directory_.open();
        SAL_WARN_IF(
            rc != osl::FileBase::E_None, "unoidl", "open(" << uri << ") failed with " << +rc);
    }

private:
    virtual ~Cursor() noexcept override {}

    virtual rtl::Reference<Entity> getNext(OUString *) override;

    Manager& manager_;
    osl::Directory directory_;
};

class SourceModuleEntity: public ModuleEntity {
public:
    SourceModuleEntity(Manager& manager, OUString uri): manager_(manager), uri_(std::move(uri)) {}

private:
    virtual ~SourceModuleEntity() noexcept override {}

    virtual std::vector<OUString> getMemberNames() const override
    { return std::vector<OUString>(); } //TODO

    virtual rtl::Reference< MapCursor > createCursor() const override
    { return new Cursor(manager_, uri_); }

    Manager& manager_;
    OUString uri_;
};

bool isValidFileName(std::u16string_view name, bool directory) {
    for (size_t i = 0;; ++i) {
        if (i == name.size()) {
            if (i == 0) {
                return false;
            }
            return directory;
        }
        auto const c = name[i];
        if (c == '.') {
            if (i == 0 || name[i - 1] == '_') {
                return false;
            }
            return !directory && name.substr(i + 1) == u"idl";
        } else if (c == '_') {
            //TODO: Ignore case of name[0] only for case-insensitive file systems:
            if (i == 0 || name[i - 1] == '_') {
                return false;
            }
        } else if (rtl::isAsciiDigit(c)) {
            if (i == 0) {
                return false;
            }
        } else if (!rtl::isAsciiAlpha(c)) {
            return false;
        }
    }
}

}

rtl::Reference<Entity> Cursor::getNext(OUString * name) {
    assert(name != nullptr);
    for (;;) {
        osl::DirectoryItem i;
        auto rc = directory_.getNextItem(i);
        switch (rc) {
        case osl::FileBase::E_None:
            {
                osl::FileStatus stat(
                    osl_FileStatus_Mask_Type | osl_FileStatus_Mask_FileName |
                    osl_FileStatus_Mask_FileURL);
                rc = i.getFileStatus(stat);
                if (rc != osl::FileBase::E_None) {
                    SAL_WARN(
                        "unoidl",
                        "getFileStatus in <" << directory_.getURL() << "> failed with " << +rc);
                    continue;
                }
                auto const dir = stat.getFileType() == osl::FileStatus::Directory;
                if (!isValidFileName(stat.getFileName(), dir)) {
                    continue;
                }
                if (dir) {
                    //TODO: Using osl::FileStatus::getFileName can likely cause issues on case-
                    // insensitive/preserving file systems, see the free getFileName function above
                    // (which likely goes unnoticed if module identifiers follow the convention of
                    // being all-lowercase):
                    *name = stat.getFileName();
                    return new SourceModuleEntity(manager_, stat.getFileURL());
                } else {
                    SourceProviderScannerData data(&manager_);
                    if (!parse(stat.getFileURL(), &data)) {
                        SAL_WARN("unoidl", "cannot parse <" << stat.getFileURL() << ">");
                        continue;
                    }
                    auto ent = data.entities.end();
                    for (auto j = data.entities.begin(); j != data.entities.end(); ++j) {
                        if (j->second.kind != SourceProviderEntity::KIND_LOCAL)
                        {
                            continue;
                        }
                        if (ent != data.entities.end()) {
                            throw FileFormatException(
                                stat.getFileURL(), u"source file defines more than one entity"_ustr);
                        }
                        ent = j;
                    }
                    if (ent == data.entities.end()) {
                        SAL_INFO(
                            "unoidl",
                            "source file <" << stat.getFileURL() << "> defines no entity");
                        continue;
                    }
                    //TODO: Check that the entity's name matches the suffix of stat.getFileURL():
                    *name = ent->first.copy(ent->first.lastIndexOf('.') + 1);
                    return ent->second.entity;
                }
            }
        default:
            SAL_WARN( "unoidl", "getNext from <" << directory_.getURL() << "> failed with " << +rc);
            [[fallthrough]];
        case osl::FileBase::E_NOENT:
            return {};
        }
    }
}

SourceTreeProvider::SourceTreeProvider(Manager & manager, OUString const & uri):
    manager_(manager), uri_(uri.endsWith("/") ? uri : uri + "/")
{}

rtl::Reference<MapCursor> SourceTreeProvider::createRootCursor() const {
    return new Cursor(manager_, uri_);
}

rtl::Reference<Entity> SourceTreeProvider::findEntity(OUString const & name)
    const
{
    std::map< OUString, rtl::Reference<Entity> >::iterator ci(
        cache_.find(name));
    if (ci != cache_.end()) {
        return ci->second;
    }
    // Match name against
    //   name ::= identifier ("." identifier)*
    //   identifier ::= upper-blocks | lower-block
    //   upper-blocks ::= upper ("_"? alnum)*
    //   lower-block :== lower alnum*
    //   alnum ::= digit | upper | lower
    //   digit ::= "0"--"9"
    //   upper ::= "A"--"Z"
    //   lower ::= "a"--"z"
    OUStringBuffer buf(name);
    sal_Int32 start = 0;
    sal_Int32 i = 0;
    for (; i != name.getLength(); ++i) {
        sal_Unicode c = name[i];
        if (c == '.') {
            assert(i == start || i != 0);
            if (i == start || name[i - 1] == '_') {
                throw FileFormatException( //TODO
                    u""_ustr, "Illegal UNOIDL identifier \"" + name + "\"");
            }
            buf[i] = '/';
            start = i + 1;
        } else if (c == '_') {
            assert(i == start || i != 0);
            if (i == start || name[i - 1] == '_'
                || !rtl::isAsciiUpperCase(name[start]))
            {
                throw FileFormatException( //TODO
                    u""_ustr, "Illegal UNOIDL identifier \"" + name + "\"");
            }
        } else if (rtl::isAsciiDigit(c)) {
            if (i == start) {
                throw FileFormatException( //TODO
                    u""_ustr, "Illegal UNOIDL identifier \"" + name + "\"");
            }
        } else if (!rtl::isAsciiAlpha(c)) {
            throw FileFormatException( //TODO
                u""_ustr, "Illegal UNOIDL identifier \"" + name + "\"");
        }
    }
    if (i == start) {
        throw FileFormatException( //TODO
            u""_ustr, "Illegal UNOIDL identifier \"" + name + "\"");
    }
    OUString uri(uri_ + buf);
    rtl::Reference<Entity> ent;
    // Prevent conflicts between foo/ and Foo.idl on case-preserving file
    // systems:
    if (exists(uri, true) && !exists(uri + ".idl", false)) {
        ent = new SourceModuleEntity(manager_, uri);
    } else {
        uri += ".idl";
        SourceProviderScannerData data(&manager_);
        if (parse(uri, &data)) {
            std::map<OUString, SourceProviderEntity>::const_iterator j(
                data.entities.find(name));
            if (j != data.entities.end()) {
                ent = j->second.entity;
            }
            SAL_WARN_IF(
                !ent.is(), "unoidl",
                "<" << uri << "> does not define entity " << name);
        }
    }
    cache_.emplace(name, ent);
    return ent;
}

SourceTreeProvider::~SourceTreeProvider() noexcept {}

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
