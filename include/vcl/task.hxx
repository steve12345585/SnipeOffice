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

#ifndef INCLUDED_VCL_TASK_HXX
#define INCLUDED_VCL_TASK_HXX

#include <vcl/dllapi.h>

struct ImplSchedulerData;

enum class TaskPriority
{
    HIGHEST,       ///< These events should run very fast!
    DEFAULT,       ///< Default priority used, e.g. the default timer priority
    // Input from the OS event queue is processed before HIGH_IDLE tasks.
    HIGH_IDLE,     ///< Important idle events to be run before processing drawing events
    RESIZE,        ///< Resize runs before repaint, so we won't paint twice
    REPAINT,       ///< All repaint events should go in here
    SKIA_FLUSH,    ///< tdf#165277 Skia needs to flush immediately before POST_PAINT tasks on macOS
    POST_PAINT,    ///< Everything running directly after painting
    DEFAULT_IDLE,  ///< Default idle priority
    LOWEST,        ///< Low, very idle cleanup tasks
    TOOLKIT_DEBUG  ///< Do not use. Solely for IdleTask::waitUntilIdleDispatched
};

#define PRIO_COUNT (static_cast<int>(TaskPriority::TOOLKIT_DEBUG) + 1)

class VCL_DLLPUBLIC Task
{
    friend class Scheduler;
    friend struct ImplSchedulerData;

    ImplSchedulerData *mpSchedulerData; ///< Pointer to the element in scheduler list
    const char        *mpDebugName;     ///< Useful for debugging
    TaskPriority       mePriority;      ///< Task priority
    bool               mbActive;        ///< Currently in the scheduler
    bool               mbStatic;        ///< Is a static object

protected:
    static void StartTimer( sal_uInt64 nMS );

    const ImplSchedulerData* GetSchedulerData() const { return mpSchedulerData; }

    virtual void SetDeletionFlags();

    /**
     * How long (in MS) until the Task is ready to be dispatched?
     *
     * Simply return Scheduler::ImmediateTimeoutMs if you're ready, like an
     * Idle. If you have to return Scheduler::InfiniteTimeoutMs, you probably
     * need another mechanism to wake up the Scheduler or rely on other
     * Tasks to be scheduled, or simply use a polling Timer.
     *
     * @param nTimeNow the current time
     * @return the sleep time of the Task to become ready
     */
    virtual sal_uInt64 UpdateMinPeriod( sal_uInt64 nTimeNow ) const = 0;

public:
    Task( const char *pDebugName );
    Task( const Task& rTask );
    virtual ~Task() COVERITY_NOEXCEPT_FALSE;
    Task& operator=( const Task& rTask );

    void            SetPriority(TaskPriority ePriority);
    TaskPriority    GetPriority() const { return mePriority; }

    const char     *GetDebugName() const { return mpDebugName; }

    virtual bool DecideTransferredExecution();

    // Call handler
    virtual void    Invoke() = 0;

    /**
     * Schedules the task for execution
     *
     * If the timer is already active, it's reset!
     * Check with Task::IsActive() to prevent reset.
     *
     * If you unset bStartTimer, the Task must call Task::StartTimer(...) to be correctly scheduled!
     * Otherwise it might just be picked up when the Scheduler runs the next time.
     *
     * @param bStartTimer if false, don't schedule the Task by calling Task::StartTimer(0).
     */
    virtual void Start(bool bStartTimer = true);
    void            Stop();

    bool            IsActive() const { return mbActive; }

    /**
     * This function must be called for static tasks, so the Task destructor
     * ignores the scheduler mutex, as it may not be available anymore.
     * The cleanup is still correct, as it has already happened in
     * DeInitScheduler call well before the static destructor calls.
     */
    void            SetStatic() { mbStatic = true; }
    bool            IsStatic() const { return mbStatic; }
};

#endif // INCLUDED_VCL_TASK_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
