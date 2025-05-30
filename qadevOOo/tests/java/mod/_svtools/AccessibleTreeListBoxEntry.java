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
package mod._svtools;

import com.sun.star.view.XSelectionSupplier;

import java.io.PrintWriter;

import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.AccessibilityTools;
import util.DesktopTools;
import util.SOfficeFactory;

import com.sun.star.accessibility.AccessibleRole;
import com.sun.star.accessibility.XAccessible;
import com.sun.star.awt.XWindow;
import com.sun.star.beans.PropertyValue;
import com.sun.star.frame.XController;
import com.sun.star.frame.XDesktop;
import com.sun.star.frame.XDispatch;
import com.sun.star.frame.XDispatchProvider;
import com.sun.star.frame.XFrame;
import com.sun.star.frame.XModel;
import com.sun.star.lang.XInitialization;
import com.sun.star.text.XTextDocument;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.util.URL;
import com.sun.star.util.XCloseable;


/**
 * Test for object that implements the following interfaces :
 * <ul>
 *  <li><code>::com::sun::star::accessibility::XAccessibleComponent
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleEventBroadcaster
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleContext
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleSelection
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleAction
 *  </code></li>
 *  <li><code>::com::sun::star::accessibility::XAccessibleText
 *  </code></li>
 * </ul> <p>
 *
 * @see com.sun.star.accessibility.XAccessibleEventBroadcaster
 * @see com.sun.star.accessibility.XAccessibleContext
 * @see com.sun.star.accessibility.XAccessibleComponent
 * @see com.sun.star.accessibility.XAccessibleSelection
 * @see com.sun.star.accessibility.XAccessibleAction
 * @see com.sun.star.accessibility.XAccessibleText
 * @see ifc.accessibility._XAccessibleEventBroadcaster
 * @see ifc.accessibility._XAccessibleContext
 * @see ifc.accessibility._XAccessibleComponent
 * @see ifc.accessibility._XAccessibleSelection
 * @see ifc.accessibility._XAccessibleAction
 * @see ifc.accessibility._XAccessibleText
 */
public class AccessibleTreeListBoxEntry extends TestCase {
    static XDesktop xDesktop;
    static XTextDocument xTextDoc;

    /**
     * Creates the Desktop service (<code>com.sun.star.frame.Desktop</code>).
     */
    @Override
    protected void initialize(TestParameters Param, PrintWriter log) throws Exception {
        xDesktop = DesktopTools.createDesktop(Param.getMSF());
    }

    /**
     * Disposes the document, if exists, created in
     * <code>createTestEnvironment</code> method.
     */
    @Override
    protected void cleanup(TestParameters Param, PrintWriter log) {
        log.println("disposing xTextDoc");

        if (xTextDoc != null) {
            closeDoc();
        }
    }

    /**
     * Creates a text document. Opens the DataSource browser.
     * Creates an instance of the service <code>com.sun.star.awt.Toolkit</code>
     * and gets active top window. Then obtains an accessible object with
     * the role <code>AccessibleRole.LABEL</code> and with the name
     * <code>"Bibliography"</code>.
     * Object relations created :
     * <ul>
     *  <li> <code>'EventProducer'</code> for
     *      {@link ifc.accessibility._XAccessibleEventBroadcaster}:
     *   </li>
     *  <li> <code>'XAccessibleSelection.multiSelection'</code>
     *  of type <code>Boolean</code> for
     *  {@link ifc.accessibility._XAccessibleSelection}:
     *   indicates that component supports single selection mode.</li>
     *   <li> <code>'XAccessibleText.Text'</code> for
     *   {@link ifc.accessibility._XAccessibleText}:
     *    the text of the component</li>
     * </ul>
     *
     * @param tParam test parameters
     * @param log writer to log information while testing
     *
     * @see com.sun.star.awt.Toolkit
     * @see com.sun.star.accessibility.AccessibleRole
     * @see ifc.accessibility._XAccessibleEventBroadcaster
     * @see ifc.accessibility._XAccessibleSelection
     * @see ifc.accessibility._XAccessibleText
     * @see com.sun.star.accessibility.XAccessibleEventBroadcaster
     * @see com.sun.star.accessibility.XAccessibleSelection
     * @see com.sun.star.accessibility.XAccessibleText
     */
    @Override
    protected TestEnvironment createTestEnvironment(TestParameters tParam,
                                                    PrintWriter log) throws Exception {
        log.println("creating a test environment");

        if (xTextDoc != null) {
            closeDoc();
        }

        // get a soffice factory object
        SOfficeFactory SOF = SOfficeFactory.getFactory( tParam.getMSF());

        log.println("creating a text document");
        xTextDoc = SOF.createTextDoc(null);

        util.utils.waitForEventIdle(tParam.getMSF());

        XModel aModel1 = UnoRuntime.queryInterface(XModel.class,
                                                            xTextDoc);

        XController secondController = aModel1.getCurrentController();

        XDispatchProvider aProv = UnoRuntime.queryInterface(
                                          XDispatchProvider.class,
                                          secondController);

        XDispatch getting = null;

        log.println("opening DatasourceBrowser");

        URL the_url = new URL();
        the_url.Complete = ".component:DB/DataSourceBrowser";
        getting = aProv.queryDispatch(the_url, "_beamer", 12);

        PropertyValue[] noArgs = new PropertyValue[0];
        getting.dispatch(the_url, noArgs);

        util.utils.waitForEventIdle(tParam.getMSF());

        XFrame the_frame1 = xDesktop.getCurrentFrame();

        if (the_frame1 == null) {
            log.println("Current frame was not found !!!");
        }

        XFrame the_frame2 = the_frame1.findFrame("_beamer", 4);

        the_frame2.setName("DatasourceBrowser");

        XInterface oObj = null;

        UnoRuntime.queryInterface(
                                        XInitialization.class,
                                        the_frame2.getController());

        PropertyValue[] params = new PropertyValue[3];
        PropertyValue param1 = new PropertyValue();
        param1.Name = "DataSourceName";
        param1.Value = "Bibliography";
        params[0] = param1;

        PropertyValue param2 = new PropertyValue();
        param2.Name = "CommandType";
        param2.Value = Integer.valueOf(com.sun.star.sdb.CommandType.QUERY);
        params[1] = param2;

        PropertyValue param3 = new PropertyValue();
        param3.Name = "Command";
        param3.Value = "select * from biblio";
        params[2] = param3;

        XController xCont = the_frame2.getController();

        XSelectionSupplier xSelect = UnoRuntime.queryInterface(
            XSelectionSupplier.class, xCont);

        xSelect.select(params);

        util.utils.waitForEventIdle(tParam.getMSF());

        XWindow xWindow = secondController.getFrame().getContainerWindow();

        XAccessible xRoot = AccessibilityTools.getAccessibleObject(xWindow);

        AccessibilityTools.printAccessibleTree(log, xRoot, tParam.getBool(util.PropertyName.DEBUG_IS_ACTIVE));

        oObj = AccessibilityTools.getAccessibleObjectForRole(xRoot, AccessibleRole.LABEL,
                                             "Bibliography");

        log.println("ImplementationName: " + util.utils.getImplName(oObj));

        TestEnvironment tEnv = new TestEnvironment(oObj);

        tEnv.addObjRelation("EditOnly",
                            "This method isn't supported in this component");

        //adding ObjectRelation for XAccessibleText
        tEnv.addObjRelation("LimitedBounds", "true");


        tEnv.addObjRelation("EventProducer",
                            new ifc.accessibility._XAccessibleEventBroadcaster.EventProducer() {
            public void fireEvent() {
            }
        });

        tEnv.addObjRelation("XAccessibleSelection.multiSelection",
                            Boolean.FALSE);

        tEnv.addObjRelation("XAccessibleText.Text", "Bibliography");

        return tEnv;
    }

    protected void closeDoc() {
        XCloseable closer = UnoRuntime.queryInterface(
                                    XCloseable.class, xTextDoc);

        try {
            closer.close(true);
        } catch (com.sun.star.util.CloseVetoException e) {
            log.println("Couldn't close document " + e.getMessage());
        } catch (com.sun.star.lang.DisposedException e) {
            log.println("Couldn't close document " + e.getMessage());
        }
    }
}