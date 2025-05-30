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

#include <sal/config.h>

#include <string_view>

#include <jobs/configaccess.hxx>
#include <jobs/jobdata.hxx>
#include <classes/converter.hxx>

#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XMultiHierarchicalPropertySet.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>

#include <tools/wldcrd.hxx>
#include <unotools/configpaths.hxx>
#include <utility>
#include <vcl/svapp.hxx>

namespace framework{

/**
    @short      standard ctor
    @descr      It initialize this new instance.
                But for real working it's necessary to call setAlias() or setService() later.
                Because we need the job data ...

    @param      rxContext
                    reference to the uno service manager
*/
JobData::JobData( css::uno::Reference< css::uno::XComponentContext > xContext )
    : m_xContext    (std::move(xContext                    ))
{
    // share code for member initialization with defaults!
    impl_reset();
}

/**
    @short  copy ctor
    @descr  Sometimes such job data container must be moved from one using place
            to another one. Then a copy ctor and copy operator must be available.

    @param  rCopy
                the original instance, from which we must copy all data
*/
JobData::JobData( const JobData& rCopy )
{
    // use the copy operator to share the same code
    *this = rCopy;
}

/**
    @short  operator for copying JobData instances
    @descr  Sometimes such job data container must be moved from one using place
            to another one. Then a copy ctor and copy operator must be available.

    @param  rCopy
                the original instance, from which we must copy all data
*/
JobData& JobData::operator=( const JobData& rCopy )
{
    // Please don't copy the uno service manager reference.
    // That can change the uno context, which isn't a good idea!
    m_eMode                = rCopy.m_eMode;
    m_eEnvironment         = rCopy.m_eEnvironment;
    m_sAlias               = rCopy.m_sAlias;
    m_sService             = rCopy.m_sService;
    m_sContext             = rCopy.m_sContext;
    m_sEvent               = rCopy.m_sEvent;
    m_lArguments           = rCopy.m_lArguments;
    return *this;
}

/**
    @short  let this instance die
    @descr  There is no chance any longer to work. We have to
            release all used resources and free used memory.
*/
JobData::~JobData()
{
    impl_reset();
}

/**
    @short      initialize this instance as a job with configuration
    @descr      They given alias can be used to address some configuration data.
                We read it and fill our internal structures. Of course old information
                will be lost doing so.

    @param      sAlias
                    the alias name of this job, used to locate job properties inside cfg
*/
void JobData::setAlias( const OUString& sAlias )
{
    // delete all old information! Otherwise we mix it with the new one ...
    impl_reset();

    // take over the new information
    m_sAlias   = sAlias;
    m_eMode    = E_ALIAS;

    // try to open the configuration set of this job directly and get a property access to it
    // We open it readonly here
    ConfigAccess aConfig(
        m_xContext,
        ("/org.openoffice.Office.Jobs/Jobs/"
         + utl::wrapConfigurationElementName(m_sAlias)));
    aConfig.open(ConfigAccess::E_READONLY);
    if (aConfig.getMode()==ConfigAccess::E_CLOSED)
    {
        impl_reset();
        return;
    }

    css::uno::Reference< css::beans::XPropertySet > xJobProperties(aConfig.cfg(), css::uno::UNO_QUERY);
    if (xJobProperties.is())
    {
        css::uno::Any aValue;

        // read uno implementation name
        aValue   = xJobProperties->getPropertyValue(u"Service"_ustr);
        aValue >>= m_sService;

        // read module context list
        aValue   = xJobProperties->getPropertyValue(u"Context"_ustr);
        aValue >>= m_sContext;

        // read whole argument list
        aValue = xJobProperties->getPropertyValue(u"Arguments"_ustr);
        css::uno::Reference< css::container::XNameAccess > xArgumentList;
        if (
            (aValue >>= xArgumentList)  &&
            (xArgumentList.is()      )
           )
        {
            css::uno::Sequence< OUString > lArgumentNames = xArgumentList->getElementNames();
            sal_Int32                             nCount         = lArgumentNames.getLength();
            m_lArguments.resize(nCount);
            for (sal_Int32 i=0; i<nCount; ++i)
            {
                m_lArguments[i].Name  = lArgumentNames[i];
                m_lArguments[i].Value = xArgumentList->getByName(m_lArguments[i].Name);
            }
        }
    }

    aConfig.close();
}

/**
    @short      initialize this instance as a job without configuration
    @descr      This job has no configuration data. We have to forget all old information
                and set only some of them new, so this instance can work.

    @param      sService
                    the uno service name of this "non configured" job
*/
void JobData::setService( const OUString& sService )
{
    // delete all old information! Otherwise we mix it with the new one ...
    impl_reset();
    // take over the new information
    m_sService = sService;
    m_eMode    = E_SERVICE;
}

/**
    @short      initialize this instance with new job values.
    @descr      It reads automatically all properties of the specified
                job (using it's alias name) and "register it" for the
                given event. This registration will not be validated against
                the underlying configuration! (That must be done from outside.
                Because the caller must have the configuration already open to
                get the values for sEvent and sAlias! And doing so it can perform
                only, if the time stamp values are read outside too.
                Further it makes no sense to initialize and start a disabled job.
                So this initialization method will be called for enabled jobs only.)

    @param      sEvent
                    the triggered event, for which this job should be started

    @param      sAlias
                    mark the required job inside event registration list
*/
void JobData::setEvent( const OUString& sEvent ,
                        const OUString& sAlias )
{
    // share code to read all job properties!
    setAlias(sAlias);

    // take over the new information - which differ against set one of method setAlias()!
    m_sEvent = sEvent;
    m_eMode  = E_EVENT;
}

/**
    @short      set the new job specific arguments
    @descr      If a job finish his work, it can give us a new list of arguments (which
                will not interpreted by us). We write it back to the configuration only
                (if this job has its own configuration!).
                So a job can have persistent data without implementing anything
                or define own config areas for that.

    @param      lArguments
                    list of arguments, which should be set for this job
 */
void JobData::setJobConfig( std::vector< css::beans::NamedValue >&& lArguments )
{
    // update member
    m_lArguments = std::move(lArguments);

    // update the configuration ... if possible!
    if (m_eMode!=E_ALIAS)
        return;

    // It doesn't matter if this config object was already opened before.
    // It doesn nothing here then ... or it change the mode automatically, if
    // it was opened using another one before.
    ConfigAccess aConfig(
        m_xContext,
        ("/org.openoffice.Office.Jobs/Jobs/"
         + utl::wrapConfigurationElementName(m_sAlias)));
    aConfig.open(ConfigAccess::E_READWRITE);
    if (aConfig.getMode()==ConfigAccess::E_CLOSED)
        return;

    css::uno::Reference< css::beans::XMultiHierarchicalPropertySet > xArgumentList(aConfig.cfg(), css::uno::UNO_QUERY);
    if (xArgumentList.is())
    {
        sal_Int32                             nCount = m_lArguments.size();
        css::uno::Sequence< OUString > lNames (nCount);
        auto lNamesRange = asNonConstRange(lNames);
        css::uno::Sequence< css::uno::Any >   lValues(nCount);
        auto lValuesRange = asNonConstRange(lValues);

        for (sal_Int32 i=0; i<nCount; ++i)
        {
            lNamesRange [i] = m_lArguments[i].Name;
            lValuesRange[i] = m_lArguments[i].Value;
        }

        xArgumentList->setHierarchicalPropertyValues(lNames, lValues);
    }
    aConfig.close();
}

/**
    @short  set a new environment descriptor for this job
    @descr  It must(!) be done every time this container is initialized
            with new job data e.g.: setAlias()/setEvent()/setService() ...
            Otherwise the environment will be unknown!
 */
void JobData::setEnvironment( EEnvironment eEnvironment )
{
    m_eEnvironment = eEnvironment;
}

/**
    @short      these functions provides access to our internal members
    @descr      These member represent any information about the job
                and can be used from outside to e.g. start a job.
 */
JobData::EMode JobData::getMode() const
{
    return m_eMode;
}

JobData::EEnvironment JobData::getEnvironment() const
{
    return m_eEnvironment;
}

OUString JobData::getEnvironmentDescriptor() const
{
    OUString sDescriptor;
    switch(m_eEnvironment)
    {
        case E_EXECUTION :
            sDescriptor = "EXECUTOR";
            break;

        case E_DISPATCH :
            sDescriptor = "DISPATCH";
            break;

        case E_DOCUMENTEVENT :
            sDescriptor = "DOCUMENTEVENT";
            break;
        default:
            break;
    }
    return sDescriptor;
}

const OUString & JobData::getService() const
{
    return m_sService;
}

const OUString & JobData::getEvent() const
{
    return m_sEvent;
}

const std::vector< css::beans::NamedValue > & JobData::getJobConfig() const
{
    return m_lArguments;
}

css::uno::Sequence< css::beans::NamedValue > JobData::getConfig() const
{
    css::uno::Sequence< css::beans::NamedValue > lConfig;
    if (m_eMode==E_ALIAS)
    {
        lConfig = { { u"Alias"_ustr, css::uno::Any(m_sAlias) },
                    { u"Service"_ustr, css::uno::Any(m_sService) },
                    { u"Context"_ustr, css::uno::Any(m_sContext) } };
    }
    return lConfig;
}

/**
    @short  return information, if this job is part of the global configuration package
            org.openoffice.Office.Jobs
    @descr  Because jobs can be executed by the dispatch framework using a uno service name
            directly - an executed job must not have any configuration really. Such jobs
            must provide the right interfaces only! But after finishing jobs can return
            some information (e.g. for updating her configuration ...). We must know
            if such request is valid or not then.

    @return sal_True if the represented job is part of the underlying configuration package.
 */
bool JobData::hasConfig() const
{
    return (m_eMode==E_ALIAS || m_eMode==E_EVENT);
}

/**
    @short      mark a job as non startable for further requests
    @descr      We don't remove the configuration entry! We set a timestamp value only.
                And there exist two of them: one for an administrator... and one for the
                current user. We change it for the user layer only. So this JobDispatch can't be
                started any more... till the administrator change his timestamp.
                That can be useful for post setup scenarios, which must run one time only.

                Note: This method don't do anything, if this represented job doesn't have a configuration!
 */
void JobData::disableJob()
{
    // No configuration - not used from EXECUTOR and not triggered from an event => no chance!
    if (m_eMode!=E_EVENT)
        return;

    // update the configuration
    // It doesn't matter if this config object was already opened before.
    // It doesn nothing here then ... or it change the mode automatically, if
    // it was opened using another one before.
    ConfigAccess aConfig(
        m_xContext,
        ("/org.openoffice.Office.Jobs/Events/"
         + utl::wrapConfigurationElementName(m_sEvent) + "/JobList/"
         + utl::wrapConfigurationElementName(m_sAlias)));
    aConfig.open(ConfigAccess::E_READWRITE);
    if (aConfig.getMode()==ConfigAccess::E_CLOSED)
        return;

    css::uno::Reference< css::beans::XPropertySet > xPropSet(aConfig.cfg(), css::uno::UNO_QUERY);
    if (xPropSet.is())
    {
        // Convert and write the user timestamp to the configuration.
        css::uno::Any aValue;
        aValue <<= Converter::convert_DateTime2ISO8601(DateTime( DateTime::SYSTEM));
        xPropSet->setPropertyValue(u"UserTime"_ustr, aValue);
    }

    aConfig.close();
}

static bool isEnabled( std::u16string_view sAdminTime ,
                    std::u16string_view sUserTime  )
{
    /*Attention!
        To prevent interpreting of TriGraphs inside next const string value,
        we have to encode all '?' signs. Otherwise e.g. "??-" will be translated
        to "~" ...
     */
    WildCard aISOPattern(u"\?\?\?\?-\?\?-\?\?*");

    bool bValidAdmin = aISOPattern.Matches(sAdminTime);
    bool bValidUser  = aISOPattern.Matches(sUserTime );

    // We check for "isEnabled()" here only.
    // Note further: ISO8601 formatted strings can be compared as strings directly!
    //               FIXME: this is not true! "T1215" is the same time as "T12:15" or "T121500"
    return (
            (!bValidAdmin && !bValidUser                         ) ||
            ( bValidAdmin &&  bValidUser && sAdminTime>=sUserTime)
           );
}

void JobData::appendEnabledJobsForEvent( const css::uno::Reference< css::uno::XComponentContext >&              rxContext,
                                         const OUString&                                                 sEvent ,
                                               ::std::vector< JobData::TJob2DocEventBinding >& lJobs  )
{
    std::vector< OUString > lAdditionalJobs = JobData::getEnabledJobsForEvent(rxContext, sEvent);
    sal_Int32                c               = lAdditionalJobs.size();
    sal_Int32                i               = 0;

    for (i=0; i<c; ++i)
    {
        JobData::TJob2DocEventBinding aBinding(lAdditionalJobs[i], sEvent);
        lJobs.push_back(aBinding);
    }
}

bool JobData::hasCorrectContext(std::u16string_view rModuleIdent) const
{
    sal_Int32 nContextLen  = m_sContext.getLength();
    sal_Int32 nModuleIdLen = rModuleIdent.size();

    if ( nContextLen == 0 )
        return true;

    if ( nModuleIdLen > 0 )
    {
        sal_Int32 nIndex = m_sContext.indexOf( rModuleIdent );
        if ( nIndex >= 0 && ( nIndex+nModuleIdLen <= nContextLen ))
        {
            std::u16string_view sContextModule = m_sContext.subView( nIndex, nModuleIdLen );
            return sContextModule == rModuleIdent;
        }
    }

    return false;
}

std::vector< OUString > JobData::getEnabledJobsForEvent( const css::uno::Reference< css::uno::XComponentContext >& rxContext,
                                                                       std::u16string_view                                sEvent )
{
    // create a config access to "/org.openoffice.Office.Jobs/Events"
    ConfigAccess aConfig(rxContext, u"/org.openoffice.Office.Jobs/Events"_ustr);
    aConfig.open(ConfigAccess::E_READONLY);
    if (aConfig.getMode()==ConfigAccess::E_CLOSED)
        return std::vector< OUString >();

    css::uno::Reference< css::container::XHierarchicalNameAccess > xEventRegistry(aConfig.cfg(), css::uno::UNO_QUERY);
    if (!xEventRegistry.is())
        return std::vector< OUString >();

    // check if the given event exist inside list of registered ones
    OUString sPath(OUString::Concat(sEvent) + "/JobList");
    if (!xEventRegistry->hasByHierarchicalName(sPath))
        return std::vector< OUString >();

    // step to the job list, which is a child of the event node inside cfg
    // e.g. "/org.openoffice.Office.Jobs/Events/<event name>/JobList"
    css::uno::Any aJobList = xEventRegistry->getByHierarchicalName(sPath);
    css::uno::Reference< css::container::XNameAccess > xJobList;
    if (!(aJobList >>= xJobList) || !xJobList.is())
        return std::vector< OUString >();

    // get all alias names of jobs, which are part of this job list
    // But Some of them can be disabled by its timestamp values.
    // We create an additional job name list with the same size, then the original list...
    // step over all job entries... check her timestamps... and put only job names to the
    // destination list, which represent an enabled job.
    const css::uno::Sequence< OUString > lAllJobs = xJobList->getElementNames();
    sal_Int32 c = lAllJobs.getLength();

    std::vector< OUString > lEnabledJobs(c);
    sal_Int32 d = 0;

    for (OUString const & jobName : lAllJobs)
    {
        css::uno::Reference< css::beans::XPropertySet > xJob;
        if (
            !(xJobList->getByName(jobName) >>= xJob) ||
            !(xJob.is()     )
           )
        {
           continue;
        }

        OUString sAdminTime;
        xJob->getPropertyValue(u"AdminTime"_ustr) >>= sAdminTime;

        OUString sUserTime;
        xJob->getPropertyValue(u"UserTime"_ustr) >>= sUserTime;

        if (!isEnabled(sAdminTime, sUserTime))
            continue;

        lEnabledJobs[d] = jobName;
        ++d;
    }
    lEnabledJobs.resize(d);

    aConfig.close();

    return lEnabledJobs;
}

/**
    @short      reset all internal structures
    @descr      If someone recycles this instance, he can switch from one
                using mode to another one. But then we have to reset all currently
                used information. Otherwise we mix it and they can make trouble.

                But note: that does not set defaults for internal used members, which
                does not relate to any job property! e.g. the reference to the global
                uno service manager. Such information is used for internal processes only
                and are necessary for our work.
 */
void JobData::impl_reset()
{
    m_eMode        = E_UNKNOWN_MODE;
    m_eEnvironment = E_UNKNOWN_ENVIRONMENT;
    m_sAlias.clear();
    m_sService.clear();
    m_sContext.clear();
    m_sEvent.clear();
    m_lArguments.clear();
}

} // namespace framework

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
