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


#include <comphelper/diagnose_ex.hxx>
#include <sal/log.hxx>

#include <event.hxx>
#include <eventqueue.hxx>
#include <slideshowexceptions.hxx>

#include <limits>
#include <memory>
#include <utility>


using namespace ::com::sun::star;

namespace slideshow::internal
{
        bool EventQueue::EventEntry::operator<( const EventEntry& rEvent ) const
        {
            // negate comparison, we want priority queue to be sorted
            // in increasing order of activation times
            return nTime > rEvent.nTime;
        }


        EventQueue::EventQueue(
            std::shared_ptr<canvas::tools::ElapsedTime> pPresTimer )
            : maMutex(),
              maEvents(),
              maNextEvents(),
              maNextNextEvents(),
              mpTimer(std::move( pPresTimer ))
        {
        }

        EventQueue::~EventQueue()
        {
            // add in all that have been added explicitly for this round:
            for ( const auto& rEvent : maNextEvents )
            {
                maEvents.push(rEvent);
            }
            EventEntryVector().swap( maNextEvents );

            // dispose event queue
            while( !maEvents.empty() )
            {
                try
                {
                    maEvents.top().pEvent->dispose();
                }
                catch (const uno::Exception&)
                {
                    TOOLS_WARN_EXCEPTION("slideshow", "");
                }
                maEvents.pop();
            }
        }

        bool EventQueue::addEvent( const EventSharedPtr& rEvent )
        {
            std::unique_lock aGuard( maMutex );

            SAL_INFO("slideshow.eventqueue", "adding event \"" << rEvent->GetDescription()
                << "\" [" << rEvent.get()
                << "] at " << mpTimer->getElapsedTime()
                << " with delay " << rEvent->getActivationTime(0.0)
                );
            ENSURE_OR_RETURN_FALSE( rEvent,
                               "EventQueue::addEvent: event ptr NULL" );

            // prepare entry

            // A seemingly obvious optimization cannot be used here,
            // because it breaks assumed order of notification: zero
            // timeout events could be fired() immediately, but that
            // would not unwind the stack and furthermore changes
            // order of notification

            // add entry
            maEvents.push( EventEntry( rEvent, rEvent->getActivationTime(
                                           mpTimer->getElapsedTime()) ) );
            return true;
        }

        bool EventQueue::addEventForNextRound( EventSharedPtr const& rEvent )
        {
            std::unique_lock aGuard( maMutex );

            SAL_INFO("slideshow.eventqueue", "adding event \"" << rEvent->GetDescription()
                << "\" [" << rEvent.get()
                << "] for the next round at " << mpTimer->getElapsedTime()
                << " with delay " << rEvent->getActivationTime(0.0)
                );

            ENSURE_OR_RETURN_FALSE( rEvent,
                               "EventQueue::addEvent: event ptr NULL" );
            maNextEvents.emplace_back( rEvent, rEvent->getActivationTime(
                                mpTimer->getElapsedTime()) );
            return true;
        }

        bool EventQueue::addEventWhenQueueIsEmpty (const EventSharedPtr& rpEvent)
        {
            std::unique_lock aGuard( maMutex );

            SAL_INFO("slideshow.eventqueue", "adding event \"" << rpEvent->GetDescription()
                << "\" [" << rpEvent.get()
                << "] for execution when the queue is empty at " << mpTimer->getElapsedTime()
                << " with delay " << rpEvent->getActivationTime(0.0)
                );

            ENSURE_OR_RETURN_FALSE( rpEvent, "EventQueue::addEvent: event ptr NULL");

            maNextNextEvents.push(
                EventEntry(
                    rpEvent,
                    rpEvent->getActivationTime(mpTimer->getElapsedTime())));

            return true;
        }

        void EventQueue::forceEmpty()
        {
            process_(true);
        }

        void EventQueue::process()
        {
            process_(false);
        }

        void EventQueue::process_( bool bFireAllEvents )
        {
            std::unique_lock aGuard( maMutex );

            SAL_INFO("slideshow.verbose", "EventQueue: heartbeat" );

            // add in all that have been added explicitly for this round:
            for ( const auto& rEvent : maNextEvents ) {
                maEvents.push(rEvent);
            }
            EventEntryVector().swap( maNextEvents );

            // perform topmost, ready-to-execute event
            // =======================================

            const double nCurrTime( mpTimer->getElapsedTime() );

            // When maEvents does not contain any events that are due now
            // then process one event from maNextNextEvents.
            if (!maNextNextEvents.empty()
                && !bFireAllEvents
                && (maEvents.empty() || maEvents.top().nTime > nCurrTime))
            {
                const EventEntry aEvent (maNextNextEvents.top());
                maNextNextEvents.pop();
                maEvents.push(aEvent);
            }

            // process ready/elapsed events. Note that the 'perceived'
            // current time remains constant for this loop, thus we're
            // processing only those events which where ready when we
            // entered this method.
            while( !maEvents.empty() &&
                   (bFireAllEvents || maEvents.top().nTime <= nCurrTime) )
            {
                EventEntry event( maEvents.top() );
                maEvents.pop();

                // only process event, if it is still 'charged',
                // i.e. the fire() call effects something. This is
                // used when e.g. having events registered at multiple
                // places, which should fire only once: after the
                // initial fire() call, those events become inactive
                // and return false on isCharged. This frees us from
                // the need to prune queues of those inactive shells.
                if( event.pEvent->isCharged() )
                {
                    aGuard.unlock();
                    try
                    {
                        SAL_INFO("slideshow.eventqueue", "firing event \""
                                << event.pEvent->GetDescription()
                                << "\" [" << event.pEvent.get()
                                << "] at " << mpTimer->getElapsedTime()
                                << " with delay " << event.pEvent->getActivationTime(0.0)
                                );
                        event.pEvent->fire();
                        SAL_INFO("slideshow.eventqueue", "event \""
                                << event.pEvent->GetDescription()
                                << "\" [" << event.pEvent.get() << "] fired"
                                );
                    }
                    catch( uno::RuntimeException& )
                    {
                        throw;
                    }
                    catch( uno::Exception& )
                    {
                        // catch anything here, we don't want
                        // to leave this scope under _any_
                        // circumstance. Although, do _not_
                        // reinsert an activity that threw
                        // once.

                        // NOTE: we explicitly don't catch(...) here,
                        // since this will also capture segmentation
                        // violations and the like. In such a case, we
                        // still better let our clients now...
                        TOOLS_WARN_EXCEPTION( "slideshow", "" );
                    }
                    catch( SlideShowException& )
                    {
                        // catch anything here, we don't want
                        // to leave this scope under _any_
                        // circumstance. Although, do _not_
                        // reinsert an activity that threw
                        // once.

                        // NOTE: we explicitly don't catch(...) here,
                        // since this will also capture segmentation
                        // violations and the like. In such a case, we
                        // still better let our clients now...
                        SAL_WARN("slideshow.eventqueue", "::presentation::internal::EventQueue: Event threw a SlideShowException, action might not have been fully performed" );
                    }
                    aGuard.lock();
                }
                else
                {
                    SAL_INFO(
                        "slideshow.eventqueue",
                        "Ignoring discharged event: unknown ("
                            << event.pEvent.get() << "), timeout was: "
                            << event.pEvent->getActivationTime(0.0));
                }
            }
        }

        bool EventQueue::isEmpty() const
        {
            std::unique_lock aGuard( maMutex );

            return maEvents.empty() && maNextEvents.empty() && maNextNextEvents.empty();
        }

        double EventQueue::nextTimeout() const
        {
            std::unique_lock aGuard( maMutex );

            // return time for next entry (if any)
            double nTimeout (::std::numeric_limits<double>::max());
            const double nCurrentTime (mpTimer->getElapsedTime());
            if ( ! maEvents.empty())
                nTimeout = maEvents.top().nTime - nCurrentTime;
            if ( ! maNextEvents.empty())
                nTimeout = ::std::min(nTimeout, maNextEvents.front().nTime - nCurrentTime);
            if ( ! maNextNextEvents.empty())
                nTimeout = ::std::min(nTimeout, maNextNextEvents.top().nTime - nCurrentTime);

            return nTimeout;
        }

        void EventQueue::clear()
        {
            std::unique_lock aGuard( maMutex );

            // TODO(P1): Maybe a plain vector and vector.swap will
            // be faster here. Profile.
            maEvents = ImplQueueType();

            maNextEvents.clear();
            maNextNextEvents = ImplQueueType();
        }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
