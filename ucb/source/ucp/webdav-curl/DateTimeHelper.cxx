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

#include <osl/time.h>
#include <com/sun/star/util/DateTime.hpp>
#include "DateTimeHelper.hxx"

using namespace com::sun::star::util;

using namespace http_dav_ucp;

bool DateTimeHelper::ISO8601_To_DateTime (std::u16string_view s,
    DateTime& dateTime)
{
    OString aDT = OUStringToOString(s, RTL_TEXTENCODING_ASCII_US);

    int year, month, day, hours, minutes, off_hours, off_minutes, fix;
    double seconds;

    // 2001-01-01T12:30:00Z
    int n = sscanf( aDT.getStr(), "%04d-%02d-%02dT%02d:%02d:%lfZ",
                    &year, &month, &day, &hours, &minutes, &seconds );
    if ( n == 6 )
    {
        fix = 0;
    }
    else
    {
        // 2001-01-01T12:30:00+03:30
        n = sscanf( aDT.getStr(), "%04d-%02d-%02dT%02d:%02d:%lf+%02d:%02d",
                    &year, &month, &day, &hours, &minutes, &seconds,
                    &off_hours, &off_minutes );
        if ( n == 8 )
        {
            fix = - off_hours * 3600 - off_minutes * 60;
        }
        else
        {
            // 2001-01-01T12:30:00-03:30
            n = sscanf( aDT.getStr(), "%04d-%02d-%02dT%02d:%02d:%lf-%02d:%02d",
                        &year, &month, &day, &hours, &minutes, &seconds,
                        &off_hours, &off_minutes );
            if ( n == 8 )
            {
                fix = off_hours * 3600 + off_minutes * 60;
            }
            else
            {
                return false;
            }
        }
    }

    // Convert to local time...

    oslDateTime aDateTime;
    aDateTime.NanoSeconds = 0;
    aDateTime.Seconds     = sal::static_int_cast< sal_uInt16 >(seconds); // 0-59
    aDateTime.Minutes     = sal::static_int_cast< sal_uInt16 >(minutes); // 0-59
    aDateTime.Hours       = sal::static_int_cast< sal_uInt16 >(hours); // 0-23
    aDateTime.Day         = sal::static_int_cast< sal_uInt16 >(day); // 1-31
    aDateTime.DayOfWeek   = 0;          // 0-6, 0 = Sunday
    aDateTime.Month       = sal::static_int_cast< sal_uInt16 >(month); // 1-12
    aDateTime.Year        = sal::static_int_cast< sal_Int16  >(year);

    TimeValue aTimeValue;
    if ( osl_getTimeValueFromDateTime( &aDateTime, &aTimeValue ) )
    {
        aTimeValue.Seconds += fix;

        if ( osl_getLocalTimeFromSystemTime( &aTimeValue, &aTimeValue ) )
        {
            if ( osl_getDateTimeFromTimeValue( &aTimeValue, &aDateTime ) )
            {
                dateTime.Year    = aDateTime.Year;
                dateTime.Month   = aDateTime.Month;
                dateTime.Day     = aDateTime.Day;
                dateTime.Hours   = aDateTime.Hours;
                dateTime.Minutes = aDateTime.Minutes;
                dateTime.Seconds = aDateTime.Seconds;

                return true;
            }
        }
    }

    return false;
}

/*
sal_Int32 DateTimeHelper::convertDayToInt (const OUString& day)
{
    if (day.equalsAscii("Sun"))
        return 0;
    else if (day.equalsAscii("Mon"))
        return 1;
    else if (day.equalsAscii("Tue"))
        return 2;
    else if (day.equalsAscii("Wed"))
        return 3;
    else if (day.equalsAscii("Thu"))
        return 4;
    else if (day.equalsAscii("Fri"))
        return 5;
    else if (day.equalsAscii("Sat"))
        return 6;
    else
        return -1;
}
*/

sal_Int32 DateTimeHelper::convertMonthToInt(std::u16string_view month)
{
    if (month == u"Jan")
        return 1;
    else if (month == u"Feb")
        return 2;
    else if (month == u"Mar")
        return 3;
    else if (month == u"Apr")
        return 4;
    else if (month == u"May")
        return 5;
    else if (month == u"Jun")
        return 6;
    else if (month == u"Jul")
        return 7;
    else if (month == u"Aug")
        return 8;
    else if (month == u"Sep")
        return 9;
    else if (month == u"Oct")
        return 10;
    else if (month == u"Nov")
        return 11;
    else if (month == u"Dec")
        return 12;
    else
        return 0;
}

bool DateTimeHelper::RFC2068_To_DateTime (std::u16string_view s,
    DateTime& dateTime)
{
    int year;
    int day;
    int hours;
    int minutes;
    int seconds;
    char string_month[3 + 1];
    char string_day[3 + 1];

    bool res = false;
    if (s.find(',') != std::u16string_view::npos)
    {
        OString aDT = OUStringToOString(s, RTL_TEXTENCODING_ASCII_US);

        // RFC 1123
        int found = sscanf(aDT.getStr(), "%3s, %2d %3s %4d %2d:%2d:%2d GMT",
                           string_day, &day, string_month, &year, &hours, &minutes, &seconds);
        if (found != 7)
        {
            // RFC 1036
            found = sscanf (aDT.getStr(), "%3s, %2d-%3s-%2d %2d:%2d:%2d GMT",
                            string_day, &day, string_month, &year, &hours, &minutes, &seconds);
        }
        res = found == 7;
    }
    else
    {
        OString aDT = OUStringToOString(s, RTL_TEXTENCODING_ASCII_US);

        // ANSI C's asctime () format
        int found = sscanf(aDT.getStr(), "%3s %3s %d %2d:%2d:%2d %4d",
                           string_day, string_month,
                           &day, &hours, &minutes, &seconds, &year);
        res = found == 7;
    }

    if (res)
    {
        res = false;

        int month = DateTimeHelper::convertMonthToInt (
                            OUString::createFromAscii (string_month));
        if (month)
        {
            // Convert to local time...

            oslDateTime aDateTime;
            aDateTime.NanoSeconds = 0;
            aDateTime.Seconds     = sal::static_int_cast< sal_uInt16 >(seconds);
                // 0-59
            aDateTime.Minutes     = sal::static_int_cast< sal_uInt16 >(minutes);
                // 0-59
            aDateTime.Hours       = sal::static_int_cast< sal_uInt16 >(hours);
                // 0-23
            aDateTime.Day         = sal::static_int_cast< sal_uInt16 >(day);
                // 1-31
            aDateTime.DayOfWeek   = 0; //dayofweek;  // 0-6, 0 = Sunday
            aDateTime.Month       = sal::static_int_cast< sal_uInt16 >(month);
                // 1-12
            aDateTime.Year        = sal::static_int_cast< sal_Int16  >(year);

            TimeValue aTimeValue;
            if ( osl_getTimeValueFromDateTime( &aDateTime,
                                                &aTimeValue ) )
            {
                if ( osl_getLocalTimeFromSystemTime( &aTimeValue,
                                                        &aTimeValue ) )
                {
                    if ( osl_getDateTimeFromTimeValue( &aTimeValue,
                                                        &aDateTime ) )
                    {
                        dateTime.Year    = aDateTime.Year;
                        dateTime.Month   = aDateTime.Month;
                        dateTime.Day     = aDateTime.Day;
                        dateTime.Hours   = aDateTime.Hours;
                        dateTime.Minutes = aDateTime.Minutes;
                        dateTime.Seconds = aDateTime.Seconds;

                        res = true;
                    }
                }
            }
        }
    }

    return res;
}

bool DateTimeHelper::convert (std::u16string_view s, DateTime& dateTime)
{
    if (ISO8601_To_DateTime (s, dateTime))
        return true;
    else if (RFC2068_To_DateTime (s, dateTime))
        return true;
    else
        return false;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
