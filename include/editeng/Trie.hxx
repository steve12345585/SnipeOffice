/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_EDITENG_TRIE_HXX
#define INCLUDED_EDITENG_TRIE_HXX

#include <rtl/ustring.hxx>
#include <editeng/editengdllapi.h>
#include <memory>
#include <vector>

namespace editeng
{
struct TrieNode;

class EDITENG_DLLPUBLIC Trie final
{
private:
    std::unique_ptr<TrieNode> mRoot;

public:
    Trie();
    ~Trie();

    void insert(std::u16string_view sInputString) const;
    void findSuggestions(std::u16string_view sWordPart,
                         std::vector<OUString>& rSuggestionList) const;
    size_t size() const;
};
}

#endif // INCLUDED_EDITENG_TRIE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
