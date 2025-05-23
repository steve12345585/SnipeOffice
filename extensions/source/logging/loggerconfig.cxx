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


#include "loggerconfig.hxx"
#include <stdio.h>
#include <string_view>

#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/logging/LogLevel.hpp>
#include <com/sun/star/lang/NullPointerException.hpp>
#include <com/sun/star/lang/ServiceNotRegisteredException.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/logging/XLogHandler.hpp>
#include <com/sun/star/logging/XLogFormatter.hpp>

#include <comphelper/diagnose_ex.hxx>
#include <osl/process.h>

#include <cppuhelper/component_context.hxx>


namespace logging
{


    using ::com::sun::star::uno::Reference;
    using ::com::sun::star::logging::XLogger;
    using ::com::sun::star::lang::XMultiServiceFactory;
    using ::com::sun::star::uno::Sequence;
    using ::com::sun::star::uno::Any;
    using ::com::sun::star::container::XNameContainer;
    using ::com::sun::star::uno::UNO_QUERY_THROW;
    using ::com::sun::star::lang::XSingleServiceFactory;
    using ::com::sun::star::uno::XInterface;
    using ::com::sun::star::util::XChangesBatch;
    using ::com::sun::star::lang::NullPointerException;
    using ::com::sun::star::uno::Exception;
    using ::com::sun::star::lang::ServiceNotRegisteredException;
    using ::com::sun::star::beans::NamedValue;
    using ::com::sun::star::logging::XLogHandler;
    using ::com::sun::star::logging::XLogFormatter;
    using ::com::sun::star::container::XNameAccess;
    using ::com::sun::star::uno::XComponentContext;

    namespace LogLevel = ::com::sun::star::logging::LogLevel;

    namespace
    {

        typedef void (*SettingTranslation)( const Reference< XLogger >&, const OUString&, Any& );


        void    lcl_substituteFileHandlerURLVariables_nothrow( const Reference< XLogger >& _rxLogger, OUString& _inout_rFileURL )
        {
            struct Variable
            {
                std::u16string_view pVariablePattern;
                OUString sVariableValue;
            };

            OUString sLoggerName;
            try { sLoggerName = _rxLogger->getName(); }
            catch( const Exception& ) { DBG_UNHANDLED_EXCEPTION("extensions.logging"); }

            TimeValue aTimeValue;
            oslDateTime aDateTime;
            OSL_VERIFY( osl_getSystemTime( &aTimeValue ) );
            OSL_VERIFY( osl_getDateTimeFromTimeValue( &aTimeValue, &aDateTime ) );

            char buffer[ 30 ];
            const size_t buffer_size = sizeof( buffer );

            snprintf( buffer, buffer_size, "%04i-%02i-%02i",
                      static_cast<int>(aDateTime.Year),
                      static_cast<int>(aDateTime.Month),
                      static_cast<int>(aDateTime.Day) );
            OUString sDate = OUString::createFromAscii( buffer );

            snprintf( buffer, buffer_size, "%02i-%02i-%02i.%03i",
                static_cast<int>(aDateTime.Hours),
                static_cast<int>(aDateTime.Minutes),
                static_cast<int>(aDateTime.Seconds),
                ::sal::static_int_cast< sal_Int16 >( aDateTime.NanoSeconds / 10000000 ) );
            OUString sTime = OUString::createFromAscii( buffer );

            oslProcessIdentifier aProcessId = 0;
            oslProcessInfo info;
            info.Size = sizeof (oslProcessInfo);
            if ( osl_getProcessInfo ( nullptr, osl_Process_IDENTIFIER, &info ) == osl_Process_E_None)
                aProcessId = info.Ident;

            Variable const aVariables[] =
            {
                {std::u16string_view(u"$(loggername)"), sLoggerName},
                {std::u16string_view(u"$(date)"), sDate},
                {std::u16string_view(u"$(time)"), sTime},
                {std::u16string_view(u"$(datetime)"), sDate + "." + sTime },
                {std::u16string_view(u"$(pid)"), OUString::number(aProcessId)}
            };

            for (Variable const & aVariable : aVariables)
            {
                sal_Int32 nVariableIndex = _inout_rFileURL.indexOf( aVariable.pVariablePattern );
                if  (nVariableIndex >= 0)
                {
                    _inout_rFileURL = _inout_rFileURL.replaceAt( nVariableIndex, aVariable.pVariablePattern.size(), aVariable.sVariableValue );
                }
            }
        }


        void    lcl_transformFileHandlerSettings_nothrow( const Reference< XLogger >& _rxLogger, const OUString& _rSettingName, Any& _inout_rSettingValue )
        {
            if ( _rSettingName != "FileURL" )
                // not interested in this setting
                return;

            OUString sURL;
            OSL_VERIFY( _inout_rSettingValue >>= sURL );
            lcl_substituteFileHandlerURLVariables_nothrow( _rxLogger, sURL );
            _inout_rSettingValue <<= sURL;
        }


        Reference< XInterface > lcl_createInstanceFromSetting_throw(
                const Reference<XComponentContext>& _rContext,
                const Reference< XLogger >& _rxLogger,
                const Reference< XNameAccess >& _rxLoggerSettings,
                const char* _pServiceNameAsciiNodeName,
                const char* _pServiceSettingsAsciiNodeName,
                SettingTranslation _pSettingTranslation = nullptr
            )
        {
            Reference< XInterface > xInstance;

            // read the settings for the to-be-created service
            Reference< XNameAccess > xServiceSettingsNode( _rxLoggerSettings->getByName(
                OUString::createFromAscii( _pServiceSettingsAsciiNodeName ) ), UNO_QUERY_THROW );

            Sequence< OUString > aSettingNames( xServiceSettingsNode->getElementNames() );
            size_t nServiceSettingCount( aSettingNames.getLength() );
            Sequence< NamedValue > aSettings( nServiceSettingCount );
            if ( nServiceSettingCount )
            {
                const OUString* pSettingNames = aSettingNames.getConstArray();
                const OUString* pSettingNamesEnd = aSettingNames.getConstArray() + aSettingNames.getLength();
                NamedValue* pSetting = aSettings.getArray();

                for (   ;
                        pSettingNames != pSettingNamesEnd;
                        ++pSettingNames, ++pSetting
                    )
                {
                    pSetting->Name = *pSettingNames;
                    pSetting->Value = xServiceSettingsNode->getByName( *pSettingNames );

                    if ( _pSettingTranslation )
                        _pSettingTranslation( _rxLogger, pSetting->Name, pSetting->Value );
                }
            }

            OUString sServiceName;
            _rxLoggerSettings->getByName( OUString::createFromAscii( _pServiceNameAsciiNodeName ) ) >>= sServiceName;
            if ( !sServiceName.isEmpty() )
            {
                bool bSuccess = false;
                if ( aSettings.hasElements() )
                {
                    Sequence< Any > aConstructionArgs{ Any(aSettings) };
                    xInstance = _rContext->getServiceManager()->createInstanceWithArgumentsAndContext(sServiceName, aConstructionArgs, _rContext);
                    bSuccess = xInstance.is();
                }
                else
                {
                    xInstance = _rContext->getServiceManager()->createInstanceWithContext(sServiceName, _rContext);
                    bSuccess = xInstance.is();
                }

                if ( !bSuccess )
                    throw ServiceNotRegisteredException( sServiceName );
            }

            return xInstance;
        }
    }


    void initializeLoggerFromConfiguration( const Reference<XComponentContext>& _rContext, const Reference< XLogger >& _rxLogger )
    {
        try
        {
            if ( !_rxLogger.is() )
                throw NullPointerException();

            Reference< XMultiServiceFactory > xConfigProvider(
                css::configuration::theDefaultProvider::get(_rContext));

            // write access to the "Settings" node (which includes settings for all loggers)
            Sequence<Any> aArguments{ Any(NamedValue(
                u"nodepath"_ustr, Any(u"/org.openoffice.Office.Logging/Settings"_ustr))) };
            Reference< XNameContainer > xAllSettings( xConfigProvider->createInstanceWithArguments(
                u"com.sun.star.configuration.ConfigurationUpdateAccess"_ustr,
                aArguments
            ), UNO_QUERY_THROW );

            OUString sLoggerName( _rxLogger->getName() );
            if ( !xAllSettings->hasByName( sLoggerName ) )
            {
                // no node yet for this logger. Create default settings.
                Reference< XSingleServiceFactory > xNodeFactory( xAllSettings, UNO_QUERY_THROW );
                Reference< XInterface > xLoggerSettings( xNodeFactory->createInstance(), css::uno::UNO_SET_THROW );
                xAllSettings->insertByName( sLoggerName, Any( xLoggerSettings ) );
                Reference< XChangesBatch > xChanges( xAllSettings, UNO_QUERY_THROW );
                xChanges->commitChanges();
            }

            // actually read and forward the settings
            Reference< XNameAccess > xLoggerSettings( xAllSettings->getByName( sLoggerName ), UNO_QUERY_THROW );

            // the log level
            sal_Int32 nLogLevel( LogLevel::OFF );
            OSL_VERIFY( xLoggerSettings->getByName(u"LogLevel"_ustr) >>= nLogLevel );
            _rxLogger->setLevel( nLogLevel );

            // the default handler, if any
            Reference< XInterface > xUntyped( lcl_createInstanceFromSetting_throw( _rContext, _rxLogger, xLoggerSettings, "DefaultHandler", "HandlerSettings", &lcl_transformFileHandlerSettings_nothrow ) );
            if ( !xUntyped.is() )
                // no handler -> we're done
                return;
            Reference< XLogHandler > xHandler( xUntyped, UNO_QUERY_THROW );
            _rxLogger->addLogHandler( xHandler );

            // The newly created handler might have an own (default) level. Ensure that it uses
            // the same level as the logger.
            xHandler->setLevel( nLogLevel );

            // the default formatter for the handler
            xUntyped = lcl_createInstanceFromSetting_throw( _rContext, _rxLogger, xLoggerSettings, "DefaultFormatter", "FormatterSettings" );
            if ( !xUntyped.is() )
                // no formatter -> we're done
                return;
            Reference< XLogFormatter > xFormatter( xUntyped, UNO_QUERY_THROW );
            xHandler->setFormatter( xFormatter );

            // TODO: we could first create the formatter, then the handler. This would allow
            // passing the formatter as value in the component context, so the handler would
            // not create an own default formatter
        }
        catch( const Exception& )
        {
            DBG_UNHANDLED_EXCEPTION("extensions.logging");
        }
    }


} // namespace logging


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
