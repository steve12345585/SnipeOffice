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
#include <osl/file.hxx>

#include <svtools/filechangedchecker.hxx>
#include <utility>
#include <vcl/timer.hxx>

FileChangedChecker::FileChangedChecker(OUString aFilename,
        ::std::function<void ()> aCallback)
    : mTimer("SVTools FileChangedChecker Timer")
    , mFileName(std::move(aFilename))
    , mLastModTime()
    , mpCallback(std::move(aCallback))
{
    // Get the current last file modified Status
    getCurrentModTime(mLastModTime);

    // associate the callback function for the Timer
    mTimer.SetInvokeHandler(LINK(this, FileChangedChecker, TimerHandler));

    // set timer interval
    mTimer.SetTimeout(100);

    // start the timer
    resetTimer();
}

FileChangedChecker::FileChangedChecker(OUString aFilename)
    : mTimer("")
    , mFileName(std::move(aFilename))
    , mLastModTime()
    , mpCallback(nullptr)
{
    // Get the current last file modified Status
    getCurrentModTime(mLastModTime);
}

void FileChangedChecker::resetTimer()
{
    if (mpCallback == nullptr)
        return;

    // Start the Idle if it's not active
    if (!mTimer.IsActive())
        mTimer.Start();

    // Set lowest Priority
    mTimer.SetPriority(TaskPriority::LOWEST);
}

bool FileChangedChecker::getCurrentModTime(TimeValue& o_rValue) const
{
    // Need a Directory item to fetch file status
    osl::DirectoryItem aItem;
    if (osl::FileBase::E_None != osl::DirectoryItem::get(mFileName, aItem))
        return false;

    // Retrieve the status - we are only interested in last File
    // Modified time
    osl::FileStatus aStatus( osl_FileStatus_Mask_ModifyTime );
    if (osl::FileBase::E_None != aItem.getFileStatus(aStatus))
        return false;

    o_rValue = aStatus.getModifyTime();
    return true;
}

bool FileChangedChecker::hasFileChanged(bool bUpdate)
{
    // Get the current file Status
    TimeValue newTime={0,0};
    if( !getCurrentModTime(newTime) )
        return true; // well. hard to answer correctly here ...

    // Check if the seconds time stamp has any difference
    // If so, then our file has changed meanwhile
    if( newTime.Seconds != mLastModTime.Seconds ||
        newTime.Nanosec != mLastModTime.Nanosec )
    {
        // Since the file has changed, set the new status as the file status and
        // return True
        if(bUpdate)
            mLastModTime = newTime ;

        return true;
    }
    else
        return false;
}

IMPL_LINK_NOARG(FileChangedChecker, TimerHandler, Timer *, void)
{
    // If the file has changed, then update the graphic in the doc
    SAL_INFO("svtools", "Timeout Called");
    if(hasFileChanged())
    {
        SAL_INFO("svtools", "File modified");
        mpCallback();
    }

    // Reset the Timer in any case
    resetTimer();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
