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

package mod._sw;

import java.io.PrintWriter;

import lib.TestCase;
import lib.TestEnvironment;
import lib.TestParameters;
import util.SOfficeFactory;
import util.XMLTools;

import com.sun.star.document.XExporter;
import com.sun.star.lang.XMultiServiceFactory;
import com.sun.star.text.XSimpleText;
import com.sun.star.text.XTextCursor;
import com.sun.star.text.XTextDocument;
import com.sun.star.uno.Any;
import com.sun.star.uno.Type;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XInterface;
import com.sun.star.xml.sax.XDocumentHandler;

/**
 * Test for object which is represented by service
 * <code>com.sun.star.comp.Calc.XMLContentExporter</code>. <p>
 * Object implements the following interfaces :
 * <ul>
 *  <li><code>com::sun::star::lang::XInitialization</code></li>
 *  <li><code>com::sun::star::document::ExportFilter</code></li>
 *  <li><code>com::sun::star::document::XFilter</code></li>
 *  <li><code>com::sun::star::document::XExporter</code></li>
 *  <li><code>com::sun::star::beans::XPropertySet</code></li>
 * </ul>
 * @see com.sun.star.lang.XInitialization
 * @see com.sun.star.document.ExportFilter
 * @see com.sun.star.document.XFilter
 * @see com.sun.star.document.XExporter
 * @see com.sun.star.beans.XPropertySet
 * @see ifc.lang._XInitialization
 * @see ifc.document._ExportFilter
 * @see ifc.document._XFilter
 * @see ifc.document._XExporter
 * @see ifc.beans._XPropertySet
 */
public class XMLContentExporter extends TestCase {
    XTextDocument xTextDoc;
    ContentFilterChecker Filter;

    /**
     * New text document created.
     */
    @Override
    protected void initialize( TestParameters tParam, PrintWriter log ) throws Exception {
        SOfficeFactory SOF = SOfficeFactory.getFactory( tParam.getMSF() );

      log.println( "creating a textdocument" );
      xTextDoc = SOF.createTextDoc( null );
    }

    /**
     * Document disposed here.
     */
    @Override
    protected void cleanup( TestParameters tParam, PrintWriter log ) {
        log.println( "    disposing xTextDoc " );
        util.DesktopTools.closeDoc(xTextDoc);
    }

    /**
    * Creating a TestEnvironment for the interfaces to be tested.
    * Creates an instance of the service
    * <code>com.sun.star.comp.Calc.XMLContentExporter</code> with
    * argument which is an implementation of <code>XDocumentHandler</code>
    * and which can check if required tags and character data is
    * exported. <p>
    * The text document is set as a source document for exporter
    * created. A string inserted into document. This made
    * for checking if this string is successfully exported within
    * the document content.
    *     Object relations created :
    * <ul>
    *  <li> <code>'MediaDescriptor'</code> for
    *      {@link ifc.document._XFilter} interface </li>
    *  <li> <code>'XFilter.Checker'</code> for
    *      {@link ifc.document._XFilter} interface </li>
    *  <li> <code>'SourceDocument'</code> for
    *      {@link ifc.document._XExporter} interface </li>
    * </ul>
    */
    @Override
    public TestEnvironment createTestEnvironment
            ( TestParameters tParam, PrintWriter log ) throws Exception {

        final String CONTENT = "XMLContentExporter";
        XMultiServiceFactory xMSF = tParam.getMSF() ;
        XInterface oObj = null;

        Filter = new ContentFilterChecker(log);
        Any arg = new Any(new Type(XDocumentHandler.class), Filter);

        oObj = (XInterface) xMSF.createInstanceWithArguments(
            "com.sun.star.comp.Writer.XMLContentExporter",
            new Object[] {arg});
        XExporter xEx = UnoRuntime.queryInterface
            (XExporter.class,oObj);
        xEx.setSourceDocument(xTextDoc);

        // text added to the document
        XSimpleText aText = xTextDoc.getText();
        XTextCursor curs = aText.createTextCursor();
        aText.insertString(curs, CONTENT, false);

        // adding tags which must be contained in XML output
        Filter.addTag("office:document-content") ;
        Filter.addTagEnclosed("office:body", "office:document-content") ;
        Filter.addTagEnclosed("office:script", "office:document-content") ;
        Filter.addTagEnclosed("office:body", "office:document-content") ;
        Filter.addCharactersEnclosed(CONTENT, "text:p") ;

        // create testobject here
        log.println( "creating a new environment" );
        TestEnvironment tEnv = new TestEnvironment( oObj );
        tEnv.addObjRelation("MediaDescriptor", XMLTools.createMediaDescriptor(
                new String[] {"FilterName"},
            new Object[] {"swriter: StarOffice XML (Writer)"}));
        tEnv.addObjRelation("SourceDocument",xTextDoc);
        tEnv.addObjRelation("XFilter.Checker", Filter) ;
        return tEnv;
    }

    /**
     * This class checks the XML for tags and data required and returns
     * checking result to <code>XFilter</code> interface test. All
     * the information about errors occurred in XML data is written
     * to log specified.
     * @see ifc.document._XFilter
     */
    private class ContentFilterChecker extends XMLTools.XMLTagsChecker
        implements ifc.document._XFilter.FilterChecker {

        /**
         * Creates a class which will write information
         * into log specified.
         */
        private ContentFilterChecker(PrintWriter log) {
            super(log) ;
        }

        /**
         * <code>_XFilter.FilterChecker</code> interface method
         * which returns the result of XML checking.
         * @return <code>true</code> if the XML data exported was
         * valid (i.e. all necessary tags and character data exists),
         * <code>false</code> if some errors occurred.
         */
        public boolean checkFilter() {
          return checkTags();
        }
    }
}

