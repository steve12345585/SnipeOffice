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

#include <com/sun/star/i18n/XCalendar4.hpp>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <utility>
#include <vector>

namespace com::sun::star::uno { class XComponentContext; }



namespace i18npool {

class CalendarImpl : public cppu::WeakImplHelper
<
    css::i18n::XCalendar4,
    css::lang::XServiceInfo
>
{
public:

    // Constructors
    CalendarImpl();
    CalendarImpl(const css::uno::Reference < css::uno::XComponentContext >& rxContext);

    /**
    * Destructor
    */
    virtual ~CalendarImpl() override;


    // Methods
    virtual void SAL_CALL loadDefaultCalendar(const css::lang::Locale& rLocale) override;
    virtual void SAL_CALL loadCalendar(const OUString& uniqueID, const css::lang::Locale& rLocale) override;
    virtual css::i18n::Calendar SAL_CALL getLoadedCalendar() override;
    virtual css::uno::Sequence < OUString > SAL_CALL getAllCalendars(const css::lang::Locale& rLocale) override;
    virtual OUString SAL_CALL getUniqueID() override;
    virtual void SAL_CALL setDateTime(double fTimeInDays) override;
    virtual double SAL_CALL getDateTime() override;
    virtual void SAL_CALL setValue( sal_Int16 nFieldIndex, sal_Int16 nValue ) override;
    virtual sal_Int16 SAL_CALL getValue(sal_Int16 nFieldIndex) override;
    virtual sal_Bool SAL_CALL isValid() override;
    virtual void SAL_CALL addValue(sal_Int16 nFieldIndex, sal_Int32 nAmount) override;
    virtual sal_Int16 SAL_CALL getFirstDayOfWeek() override;
    virtual void SAL_CALL setFirstDayOfWeek(sal_Int16 nDay) override;
    virtual void SAL_CALL setMinimumNumberOfDaysForFirstWeek(sal_Int16 nDays) override;
    virtual sal_Int16 SAL_CALL getMinimumNumberOfDaysForFirstWeek() override;
    virtual sal_Int16 SAL_CALL getNumberOfMonthsInYear() override;
    virtual sal_Int16 SAL_CALL getNumberOfDaysInWeek() override;
    virtual css::uno::Sequence < css::i18n::CalendarItem > SAL_CALL getMonths() override;
    virtual css::uno::Sequence < css::i18n::CalendarItem > SAL_CALL getDays() override;
    virtual OUString SAL_CALL getDisplayName(sal_Int16 nCalendarDisplayIndex, sal_Int16 nIdx, sal_Int16 nNameType) override;

    // Methods in XExtendedCalendar
    virtual OUString SAL_CALL getDisplayString( sal_Int32 nCalendarDisplayCode, sal_Int16 nNativeNumberMode ) override;

    // XCalendar3
    virtual css::i18n::Calendar2 SAL_CALL getLoadedCalendar2() override;
    virtual css::uno::Sequence < css::i18n::CalendarItem2 > SAL_CALL getDays2() override;
    virtual css::uno::Sequence < css::i18n::CalendarItem2 > SAL_CALL getMonths2() override;
    virtual css::uno::Sequence < css::i18n::CalendarItem2 > SAL_CALL getGenitiveMonths2() override;
    virtual css::uno::Sequence < css::i18n::CalendarItem2 > SAL_CALL getPartitiveMonths2() override;

    // XCalendar4
    virtual void SAL_CALL setLocalDateTime(double TimeInDays) override;
    virtual double SAL_CALL getLocalDateTime() override;
    virtual void SAL_CALL loadDefaultCalendarTZ(const css::lang::Locale& rLocale, const OUString& rTimeZone) override;
    virtual void SAL_CALL loadCalendarTZ(const OUString& uniqueID, const css::lang::Locale& rLocale, const OUString& rTimeZone) override;

    //XServiceInfo
    virtual OUString SAL_CALL getImplementationName() override;
    virtual sal_Bool SAL_CALL supportsService(const OUString& ServiceName) override;
    virtual css::uno::Sequence < OUString > SAL_CALL getSupportedServiceNames() override;

private:
    struct lookupTableItem {
        lookupTableItem(OUString aCacheID, css::uno::Reference < css::i18n::XCalendar4 > _xCalendar)
            : m_aCacheID(std::move(aCacheID)), xCalendar(std::move(_xCalendar)) {}
        OUString                                      m_aCacheID;
        css::uno::Reference < css::i18n::XCalendar4 > xCalendar;
    };
    std::vector<lookupTableItem>                        lookupTable;
    css::uno::Reference < css::uno::XComponentContext > m_xContext;
    css::uno::Reference < css::i18n::XCalendar4 >       xCalendar;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
