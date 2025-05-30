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
package com.sun.star.wizards.form;

import com.sun.star.beans.PropertyValue;
import com.sun.star.uno.Type;
import com.sun.star.wizards.common.Properties;
import com.sun.star.wizards.common.PropertyNames;

/** This class capsulates the class, that implements the minimal component, a
 * factory for creating the service (<CODE>__getServiceFactory</CODE>).
 */
public class CallFormWizard
{

    /** Gives a factory for creating the service.
     * This method is called by the <code>JavaLoader</code>
     * <p>
     * @return Returns a <code>XSingleServiceFactory</code> for creating the component.
     * @see com.sun.star.comp.loader.JavaLoader
     * @param stringImplementationName The implementation name of the component.
     * @param xMSF The service manager, who gives access to every known service.
     * @param xregistrykey Makes structural information (except regarding tree structures) of a single
     *   registry key accessible.
     */
    public static com.sun.star.lang.XSingleServiceFactory __getServiceFactory(String stringImplementationName, com.sun.star.lang.XMultiServiceFactory xMSF, com.sun.star.registry.XRegistryKey xregistrykey)
    {
        com.sun.star.lang.XSingleServiceFactory xsingleservicefactory = null;
        if (stringImplementationName.equals(FormWizardImplementation.class.getName()))
        {
            xsingleservicefactory = com.sun.star.comp.loader.FactoryHelper.getServiceFactory(FormWizardImplementation.class, FormWizardImplementation.__serviceName, xMSF, xregistrykey);
        }
        return xsingleservicefactory;
    }

    /** This class implements the component. At least the interfaces XServiceInfo,
     * XTypeProvider, and XInitialization should be provided by the service.
     */
    public static class FormWizardImplementation extends com.sun.star.lib.uno.helper.PropertySet implements com.sun.star.lang.XInitialization, com.sun.star.lang.XServiceInfo, com.sun.star.task.XJobExecutor
    {
        private PropertyValue[] m_wizardContext;

        /** The constructor of the inner class has a XMultiServiceFactory parameter.
         * @param xmultiservicefactoryInitialization A special service factory
         * could be introduced while initializing.
         */
        public FormWizardImplementation(com.sun.star.lang.XMultiServiceFactory xmultiservicefactoryInitialization)
        {
            super();
            m_serviceFactory = xmultiservicefactoryInitialization;
        }

        public void trigger(String sEvent)
        {
            try
            {
                if (sEvent.equals(PropertyNames.START))
                {
                    FormWizard CurFormWizard = new FormWizard( m_serviceFactory, m_wizardContext );
                    CurFormWizard.start();
                }
            }
            catch (Exception exception)
            {
                System.err.println(exception);
            }
            System.gc();
        }
        /** The service name, that must be used to get an instance of this service.
         */
        private static final String __serviceName = "com.sun.star.wizards.form.CallFormWizard";
        /** The service manager, that gives access to all registered services.
         */
        private final com.sun.star.lang.XMultiServiceFactory m_serviceFactory;

        /** This method is a member of the interface for initializing an object
         * directly after its creation.
         * @param object This array of arbitrary objects will be passed to the
         * component after its creation.
         * @throws com.sun.star.uno.Exception Every exception will not be handled, but will be
         * passed to the caller.
         */
        public void initialize(Object[] object) throws com.sun.star.uno.Exception
        {
            m_wizardContext = Properties.convertToPropertyValueArray(object);
        }

        /** This method returns an array of all supported service names.
         * @return Array of supported service names.
         */
        public java.lang.String[] getSupportedServiceNames()
        {
            String[] stringSupportedServiceNames = new String[] { __serviceName };

            return stringSupportedServiceNames;
        }

        /** This method returns true, if the given service will be
         * supported by the component.
         * @param stringService Service name.
         * @return True, if the given service name will be supported.
         */
        public boolean supportsService(String stringService)
        {
            boolean booleanSupportsService = false;

            if (stringService.equals(__serviceName))
            {
                booleanSupportsService = true;
            }
            return booleanSupportsService;
        }

        @Override
        public byte[] getImplementationId()
        {
            return new byte[0];
        }

        /** Return the class name of the component.
         * @return Class name of the component.
         */
        public java.lang.String getImplementationName()
        {
            return FormWizardImplementation.class.getName();
        }

        /** Provides a sequence of all types (usually interface types)
         * provided by the object.
         * @return Sequence of all types (usually interface types) provided by the
         * service.
         */
        @Override
        public Type[] getTypes()
        {
            Type[] typeReturn =
            {
            };

            try
            {
                typeReturn = new Type[]
                        {
                            new Type(com.sun.star.task.XJobExecutor.class), new Type(com.sun.star.lang.XTypeProvider.class), new Type(com.sun.star.lang.XServiceInfo.class), new Type(com.sun.star.beans.XPropertySet.class), new Type(com.sun.star.beans.XFastPropertySet.class), new Type(com.sun.star.beans.XMultiPropertySet.class), new Type(com.sun.star.lang.XInitialization.class)
                        };
            }
            catch (Exception exception)
            {
                System.err.println(exception);
            }

            return typeReturn;
        }
    }
}
