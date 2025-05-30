/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
* This file is Part of the SnipeOffice project.
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include <memory>
#include <svdobjuserdatalist.hxx>

SdrObjUserDataList::SdrObjUserDataList() {}
SdrObjUserDataList::~SdrObjUserDataList() {}

size_t SdrObjUserDataList::GetUserDataCount() const
{
    return maList.size();
}

SdrObjUserData& SdrObjUserDataList::GetUserData(size_t nNum)
{
    return *maList.at(nNum);
}

void SdrObjUserDataList::AppendUserData(std::unique_ptr<SdrObjUserData> pData)
{
    maList.push_back(std::move(pData));
}

void SdrObjUserDataList::DeleteUserData(size_t nNum)
{
    maList.erase(maList.begin()+nNum);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
