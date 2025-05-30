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

package ifc.text;

import lib.MultiPropertyTest;

import com.sun.star.sdbc.XConnection;
import com.sun.star.sdbc.XResultSet;
import com.sun.star.uno.UnoRuntime;

public class _MailMerge extends MultiPropertyTest {

    /**
     * Custom tester for properties which contains URLs.
     * Switches between two valid folders
     */
    protected PropertyTester URLTester = new PropertyTester() {
        @Override
        protected Object getNewValue(String propName, Object oldValue) {
            if (oldValue.equals(util.utils.getOfficeTemp(tParam.getMSF())))
                return util.utils.getFullTestURL(""); else
                return util.utils.getOfficeTemp(tParam.getMSF());
        }
    } ;

    /**
     * Custom tester for properties which contains document URLs.
     * Switches between two document URLs.
     */
    protected PropertyTester DocumentURLTester = new PropertyTester() {
        @Override
        protected Object getNewValue(String propName, Object oldValue) {
            if (oldValue.equals(util.utils.getFullTestURL("MailMerge.sxw")))
                return util.utils.getFullTestURL("sForm.sxw"); else
                return util.utils.getFullTestURL("MailMerge.sxw");
        }
    } ;
    /**
     * Tested with custom property tester.
     */
    public void _ResultSet() {
        String propName = "ResultSet";
        try{

            log.println("try to get value from property...");
            UnoRuntime.queryInterface(XResultSet.class,oObj.getPropertyValue(propName));

            log.println("try to get value from object relation...");
            XResultSet newValue = UnoRuntime.queryInterface(XResultSet.class,tEnv.getObjRelation("MailMerge.XResultSet"));

            log.println("set property to a new value...");
            oObj.setPropertyValue(propName, newValue);

            log.println("get the new value...");
            XResultSet getValue = UnoRuntime.queryInterface(XResultSet.class,oObj.getPropertyValue(propName));

            tRes.tested(propName, this.compare(newValue, getValue));
        } catch (com.sun.star.beans.PropertyVetoException e){
            log.println("could not set property '"+ propName +"' to a new value!");
            tRes.tested(propName, false);
        } catch (com.sun.star.lang.IllegalArgumentException e){
            log.println("could not set property '"+ propName +"' to a new value!");
            tRes.tested(propName, false);
        } catch (com.sun.star.beans.UnknownPropertyException e){
            if (this.isOptional(propName)){
                    // skipping optional property test
                    log.println("Property '" + propName
                            + "' is optional and not supported");
                    tRes.tested(propName,true);

            } else {
                log.println("could not get property '"+ propName +"' from XPropertySet!");
                tRes.tested(propName, false);
            }
        } catch (com.sun.star.lang.WrappedTargetException e){
            log.println("could not get property '"+ propName +"' from XPropertySet!");
            tRes.tested(propName, false);
        }
    }

    /**
     * Tested with custom property tester.
     */

    public void _ActiveConnection() {
        String propName = "ActiveConnection";
        try{

            log.println("try to get value from property...");
            UnoRuntime.queryInterface(XConnection.class,oObj.getPropertyValue(propName));

            log.println("try to get value from object relation...");
            XConnection newValue = UnoRuntime.queryInterface(XConnection.class,tEnv.getObjRelation("MailMerge.XConnection"));

            log.println("set property to a new value...");
            oObj.setPropertyValue(propName, newValue);

            log.println("get the new value...");
            XConnection getValue = UnoRuntime.queryInterface(XConnection.class,oObj.getPropertyValue(propName));

            tRes.tested(propName, this.compare(newValue, getValue));
        } catch (com.sun.star.beans.PropertyVetoException e){
            log.println("could not set property '"+ propName +"' to a new value! " + e.toString());
            tRes.tested(propName, false);
        } catch (com.sun.star.lang.IllegalArgumentException e){
            log.println("could not set property '"+ propName +"' to a new value! " + e.toString());
            tRes.tested(propName, false);
        } catch (com.sun.star.beans.UnknownPropertyException e){
            if (this.isOptional(propName)){
                    // skipping optional property test
                    log.println("Property '" + propName
                            + "' is optional and not supported");
                    tRes.tested(propName,true);

            } else {
                log.println("could not get property '"+ propName +"' from XPropertySet!");
                tRes.tested(propName, false);
            }
        } catch (com.sun.star.lang.WrappedTargetException e){
            log.println("could not get property '"+ propName +"' from XPropertySet!");
            tRes.tested(propName, false);
        }
    }

    /**
     * Tested with custom property tester.
     */
    public void _DocumentURL() {
        log.println("Testing with custom Property tester") ;
        testProperty("DocumentURL", DocumentURLTester) ;
    }

    /**
     * Tested with custom property tester.
     */
    public void _OutputURL() {
        log.println("Testing with custom Property tester") ;
        testProperty("OutputURL", URLTester) ;
    }

    /**
    * Forces environment recreation.
    */
    @Override
    protected void after() {
        disposeEnvironment();
    }


} //finish class _MailMerge

