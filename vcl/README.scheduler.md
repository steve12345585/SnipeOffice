# VCL Scheduler

## Introduction

The VCL scheduler handles LOs primary event queue. It is simple by design,
currently just a single-linked list, processed in list-order by priority
using round-robin for reoccurring tasks.

The scheduler has the following behaviour:

B.1. Tasks are scheduled just priority based

B.2. Implicitly cooperative AKA non-preemptive

B.3. It's not "fair" in any way (a consequence of B.2)

B.4. Tasks are handled round-robin (per priority)

B.5. Higher priorities have lower values

B.6. A small set of priorities instead of an flexible value AKA int

There are some consequences due to this design.

C.1. Higher priority tasks starve lower priority tasks
     As long as a higher task is available, lower tasks are never run!
     See Anti-pattern.

C.2. Tasks should be split into sensible blocks
     If this can't really be done, process pending tasks by calling
     `Application::Reschedule()`. Or use a thread.

C.3. This is not an OS scheduler
     There is no real way to "fix" B.2. and B.3.
     If you need to do a preemptive task, use a thread!
     Otherwise make your task suspendable.


## Driving the scheduler AKA the system timer

  1. There is just one system timer, which drives LO event loop
  2. The timer has to run in the main window thread
  3. The scheduler is run with the Solar mutex acquired
  4. The system timer is a single-shot timer
  5. The scheduler system event / message has a low system priority.
     All system events should have a higher priority.

Every time a task is started, the scheduler timer is adjusted. When the timer
fires, it posts an event to the system message queue. If the next most
important task is an Idle (AKA instant, 0ms timeout), the event is pushed to
the back of the queue, so we don't starve system messages, otherwise to the
front.

Every time the scheduler is invoked it searches for the next task to process,
restarts the timer with the timeout for the next event and then invokes the
task. After invoking the task and if the task is still active, it is pushed
to the end of the queue and the timeout is eventually adjusted.


## Locking

The locking is quite primitive: all interaction with internal Scheduler
structures are locked. This includes the `ImplSchedulerContext` and the
`Task::mpSchedulerData`, which is actually a part of the scheduler.
Before invoking the task, we have to release the lock, so others can
Start new Tasks.

The Scheduler just processes its own Tasks in the main thread and needs
the `SolarMutex` for it and for `DeInit` (tested by `DBG_TESTSOLARMUTEX`). All
the other interaction just take the scheduler mutex or don't need locking
at all.

There is a "workaround" for static Task objects, which would crash LO on
destruction, because `Task::~Task` would try to de-register itself in the
Scheduler, while the `SchedulerLock` would be long gone. OTOH this makes
Task handling less error-prone, than doing "special" manual cleanup.
There is also a "= TODOs and ideas =" to get rid if static Tasks.

Actually the scheduler mutex should never be locked when calling into
non-scheduler code, so it was converted to a non-recursive
`std::mutex`.


## Idle processing

Confusingly, there are 2 concepts that are called 'idle':

* Instant (zero timeout) tasks, represented e.g. by the Idle class. This is
a misnomer, as these tasks are processed after returning to the main loop.
This is not necessarily when LO is idle, in fact such tasks may be invoked
while there is input in the OS event queue pending.
(TODO: This case should be fixed by renaming.)

* Low priority tasks, represented by priorities `TaskPriority::HIGH_IDLE` and
lower. In addition to being invoked only when there is no task with a higher
priority, pending input in the OS event queue also takes precedence.


## Lifecycle / thread-safety of Scheduler-based objects

A scheduler object it thread-safe in the way, that it can be associated to
any thread and any thread is free to call any functions on it. The owner must
guarantee that the `Invoke()` function can be called, while the Scheduler object
exists / is not disposed.


## Anti-pattern: Dependencies via (fine grained) priorities

"Idle 1" should run before "Idle 2", therefore give "Idle 1" a higher priority
then "Idle 2". This just works correct for low frequency idles, but otherwise
always breaks!

If you have some longer work - even if it can be split by into schedulable,
smaller blocks - you normally don't want to schedule it with a non-default
priority, as it starves all lower priority tasks. Even if a block was processed
in "Idle 1", it is scheduled with the same (higher) priority again. Changing
the "Idle" to a "Timer" also won't work, as this breaks the dependency.

What is needed is task based dependency handling, so if "Task 1" is done, it
has to start "Task 2" and if "Task 1" is started again, it has to stop
"Task 2". This currently has to be done by the implementor, but this feature
can be added to the scheduler reasonably.


## Implementation details

### General: event priority for `DoYield`

There are three types of events, with different priority:

1. LO user events
2. System events
3. LO Scheduler event

They should be processed according to the following code:

```
bool ImplYield(bool bWait, bool bAllCurrent)
{
    DBG_TESTSOLARMUTEX();
    assert(IsMainThread());

    bool bWasEvent = ProcessUserEvents( bAllCurrent );
    if ( !bAllCurrent && bWasEvent )
        return true;

    SolarMutexReleaser();
    bWasEvent = ProcessSystemEvents( bAllCurrent, &bWasSchedulerEvent ) || bWasEvent;
    if ( !bWasSchedulerEvent && IsSchedulerEvent() )
    {
        ProcessSchedulerEvent()
        bWasEvent = true;
    }
    if ( !bWasEvent && bWait )
    {
        WaitForSystemEvents();
        bWasEvent = true;
    }
    return bWasEvent;
}
```

### General: main thread deferral

In almost all VCL backends, we run main thread deferrals by disabling the
`SolarMutex` using a boolean. In the case of the redirect, this makes
`tryToAcquire` and `doAcquire` return `true` or `1`, while a release is ignored.
Also the `IsCurrentThread()` mutex check function will act accordingly, so all
the `DBG_TESTSOLARMUTEX` won't fail.

Since we just disable the locks when we start running the deferred code in the
main thread, we won't let the main thread run into stuff, where it would
normally wait for the SolarMutex.

Eventually this will move into the `SolarMutex`. KDE / Qt also does main
thread redirects using `Qt::BlockingQueuedConnection`.

### General: processing all current events for `DoYield`

This is easily implemented for all non-priority queue based implementations.
Windows and macOS both have a timestamp attached to their events / messages,
so simply get the current time and just process anything < timestamp.
For the **KDE** backend this is already the default behaviour - single event
processing isn't even supported. The headless backend accomplishes this by
just processing a copy of the list of current events.

Problematic in this regard is the **Gtk+** backend. `g_main_context_iteration`
dispatches "only those highest priority event sources". There is no real way
to tell, when these became ready. I've added a workaround idea to the TODO
list. FWIW: **Qt** runs just a single timer source in the glib main context,
basically the same we're doing with the LO scheduler as a system event.

The gen **X11** backend has some levels of redirection, but needs quite some work
to get this fixed.

### General: non-main thread yield

Yielding from a non-main thread must not wait in the main thread, as this
may block the main thread until some events happen.

Currently we wait on an extra conditional, which is cleared by the main event
loop.

### General: invalidation of elapsed timer event messages

Since the system timer to run the scheduler is single-shot, there should never
be more than one elapsed timer event in system event queue. When stopping or
restarting the timer, we eventually have to remove the now invalid event from
the queue.

But for the Windows and macOS backends this may fail as they have delayed
posting of events, so a consecutive remove after a post will actually yield no
remove. On Windows we even get unwanted processing of events outside of the
main event loop, which may call the Scheduler, as timer management is handled
in critical scheduler code.

To prevent these problems, we don't even try to remove these events, but
invalidate them by versioning the timer events. Timer events with invalid
versions are processed but simply don't run the scheduler.

### General: track time of long running tasks

There is `TaskStopwatch` class. It'll track the time and report a timeout either
when the tasks time slice is finished or some system event did occur.

Eventually it will be merged into the main scheduler, so each invoked task can
easily track it's runtime and eventually this can be used to "blame" / find
other long running tasks, so interactivity can be improved.

There were some questions coming up when implementing it:

#### Why does the scheduler not detect that we only have idle tasks pending, and skip the instant timeout?

You never know how long a task will run. Currently the scheduler simply asks
each task when it'll be ready to run, until two runnable tasks are found.
Normally this is very quick, as LO has a lot of one-shot instant tasks / Idles
and just a very few long term pending Timers.

Especially UNO calls add a lot of Idles to the task list, which just need to
be processed in order.

#### Why not use things like Linux timer wheels?

LO has relatively few timers and a lot one-shot Idles. 99% of time the search
for the next task is quick, because there are just ~5 long term timers per
document (cache invalidation, cursor blinking etc.).

This might become a problem, if you have a lot of open documents, so the long
term timer list increases AKA for highly loaded LOOL instances.

But the Linux timer wheel mainly relies on the facts that the OS timers are
expected to not expire, as they are use to catch "error" timeouts, which rarely
happen, so this definitely not matches LO's usage.

#### Not really usable to find misbehaving tasks

The TaskStopwatch class is just a little time keeper + detecting of input
events. This is not about misbehaving Tasks, but long running tasks, which
have to yield to the Scheduler, so other Tasks and System events can be
processed.

There is the TODO to merge the functionality into the Scheduler itself, at
which point we can think about profiling individual Tasks to improve
interactivity.

### macOS implementation details

Generally the Scheduler is handled as expected, except on resize, which is
handled with different runloop-modes in macOS. In case of a resize, the normal
`runloop` is suspended in `sendEvent`, so we can't call the scheduler via posted
main loop-events. Instead the scheduler uses the timer again.

Like the Windows backend, all Cocoa / GUI handling also has to be run in
the main thread. We're emulating Windows out-of-order PeekMessage processing,
via a `YieldWakeupEvent` and two conditionals. When in a `RUNINMAIN` call, all
the `DBG_TESTSOLARMUTEX` calls are disabled, as we can't release the
`SolarMutex`, but we can prevent running any other `SolarMutex` based code.
Those wakeup events must be ignored to prevent busy-locks. For more info read
the "General: main thread deferral" section.

We can neither rely on macOS `dispatch_sync` code block execution nor the
message handling, as both can't be prioritized or filtered and the first
does also not allow nested execution and is just processed in sequence.

There is also a workaround for a problem for pushing tasks to an empty queue,
as `[NSApp postEvent: ... atStart: NO]` doesn't append the event, if the
message queue is empty.

An additional problem is the filtering of events on Window close. This drops
posted timer events, when a Window is closed resulting in a busy DoYield loop,
so we have to re-post the event, after closing a window.

### Windows implementation details

Posted or sent event messages often trigger processing of WndProc in
`PeekMessage`, `GetMessage` or `DispatchMessage`, independently from the message
to fetch, remove or dispatch ("During this call, the system delivers pending,
nonqueued messages..."). Additionally messages have an inherited priority
based on the function used to generate them. Even if `WM_TIMER` messages should
have the lowest priority, a manually posted `WM_TIMER` is processed with the
priority of a PostMessage message.

So we're giving up on processing all our Scheduler events as a message in the
system message loop. Instead we just indicate a 0ms timer message by setting
the `m_bDirectTimeout` in the timer object. This timer is always processed, if
the system message wasn't already our timer. As a result we can also skip the
polling. All this is one more reason to drop the single message processing
in favour of always processing all pending (system) events.

There is another special case, we have to handle: window updates during move
and resize of windows. These system actions run in their own nested message
loop. So we have to completely switch to timers, even for 0 ms. But these
posted events prevent any event processing, while we're busy. The only viable
solution seems to be to switch to `WM_TIMER` based timers, as these generate
messages with the lowest system priority (but they don't allow 0 ms timeouts).
So processing slows down during resize and move, but we gain working painting,
even when busy.

An additional workaround is implemented for the delayed queuing of posted
messages, where `PeekMessage` in `WinSalTimer::Stop()` won't be able remove the
just posted timer callback message. See "General: invalidation of elapsed
timer event messages" for the details.

To run the required GUI code in the main thread without unlocking the
`SolarMutex`, we "disable" it. For more infos read the "General: main thread
deferral" section.

### KDE implementation details

This implementation also works as intended. But there is a different Yield
handling, because Qts `QAbstractEventDispatcher::processEvents` will always
process all pending events.


## TODOs and ideas

### Task dependencies AKA children

Every task can have a list of children / a child.

 * When a task is stopped, the children are started.
 * When a task is started, the children are stopped.

This should be easy to implement.

### Per priority time-sorted queues

This would result in O(1) scheduler. It was used in the Linux kernel for some
time (search Ingo Molnar's O(1) scheduler). This can be a scheduling
optimization, which would prevent walking longer event list. But probably the
management overhead would be too large, as we have many one-shot events.

To find the next task the scheduler just walks the (constant) list of priority
queues and schedules the first ready event of any queue.

The downside of this approach: Insert / Start / Reschedule(for "auto" tasks)
now need O(log(n)) to find the position in the queue of the priority.

### Always process all (higher priority) pending events

Currently `Application::Reschedule()` processes a single event or "all" events,
with "all" defined as "100 events" in most backends. This already is ignored
by the KDE backend, as Qt defines its `QAbstractEventDispatcher::processEvents`
processing all pending events (there are ways to skip event classes, but no
easy way to process just a single event).

Since the Scheduler is always handled by the system message queue, there is
really no more reasoning to stop after 100 events to prevent LO Scheduler
starvation.

### Drop static inherited or composed Task objects

The sequence of destruction of static objects is not defined. So a static Task
can not be guaranteed to happen before the Scheduler. When dynamic unloading
is involved, this becomes an even worse problem. This way we could drop the
mbStatic workaround from the Task class.

### Run the LO application in its own thread

This would probably get rid of most of the macOS and Windows implementation
details / workarounds, but is quite probably a large amount of work.

Instead of LO running in the main process / thread, we run it in a 2nd thread
and defer al GUI calls to the main thread. This way it'll hopefully not block
and can process system events.

That's just a theory - it definitely needs more analysis before even attending
an implementation.

### Re-evaluate the macOS `ImplNSAppPostEvent`

Probably a solution comparable to the Windows backends delayed PostMessage
workaround using a validation timestamp is better then the current peek,
remove, re-postEvent, which has to run in the main thread.

Originally I didn't evaluate, if the event is actually lost or just delayed.

### Drop `nMaxEvents` from Gtk+ based backends

```
gint last_priority = G_MAXINT;
bool bWasEvent = false;
do {
    gint max_priority;
    g_main_context_acquire( NULL );
    bool bHasPending = g_main_context_prepare( NULL, &max_priority );
    g_main_context_release( NULL );
    if ( bHasPending )
    {
        if ( last_priority > max_priority )
        {
            bHasPending = g_main_context_iteration( NULL, bWait );
	    bWasEvent = bWasEvent || bHasPending;
	}
	else
	    bHasPending = false;
    }
}
while ( bHasPending )
```

The idea is to use `g_main_context_prepare` and keep the `max_priority` as an
indicator. We cannot prevent running newer lower events, but we can prevent
running new higher events, which should be sufficient for most stuff.

This also touches user event processing, which currently runs as a high
priority idle in the event loop.

### Drop nMaxEvents from gen (X11) backend

A few layers of indirection make this code hard to follow. The `SalXLib::Yield`
and `SalX11Display::Yield` architecture makes it impossible to process just the
current events. This really needs a refactoring and rearchitecture step, which
will also affect the Gtk+ and KDE backend for the user event handling.

### Merge TaskStopwatch functionality into the Scheduler

This way it can be easier used to profile Tasks, eventually to improve LO's
interactivity.

## See Also

- [Solar Mutex](https://wiki.openoffice.org/wiki/Terms/Solar_Mutex)
- [SnipeOffice Main Loop](https://wiki.SnipeOffice.org/Development/LHM_LiMux/Main_Loop)
- [AOO Advanced Threading-Architecture (proposal)](https://wiki.openoffice.org/wiki/Architecture/Proposal/Advanced_Threading-Architecture)
- [Revise OOo Multi-Threading Efforts](https://wiki.openoffice.org/wiki/Effort/Revise_OOo_Multi-Threading)
- [Multi-Threading Analysis](https://wiki.openoffice.org/wiki/Analysis/Multi-Threading)
- [AOO Wiki - Category:Multi-Threading](https://wiki.openoffice.org/wiki/Category:Multi-Threading)
