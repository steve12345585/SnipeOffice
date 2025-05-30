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


#include <FormattedFieldBeautifier.hxx>

#include <com/sun/star/report/XFormattedField.hpp>
#include <com/sun/star/awt/XVclWindowPeer.hpp>

#include <RptObject.hxx>
#include <RptModel.hxx>
#include <RptPage.hxx>
#include <ReportSection.hxx>
#include <ReportController.hxx>
#include <strings.hxx>
#include <reportformula.hxx>

#include <svtools/extcolorcfg.hxx>

// DBG_UNHANDLED_EXCEPTION
#include <comphelper/diagnose_ex.hxx>

namespace rptui
{
    using namespace ::com::sun::star;


    FormattedFieldBeautifier::FormattedFieldBeautifier(const OReportController& _aController)
        :m_rReportController(_aController)
        ,m_nTextColor(COL_AUTO)
    {
    }


    Color FormattedFieldBeautifier::getTextColor()
    {
        if (m_nTextColor == COL_AUTO)
        {
            svtools::ExtendedColorConfig aConfig;
            m_nTextColor = aConfig.GetColorValue(CFG_REPORTDESIGNER, DBTEXTBOXBOUNDCONTENT).getColor();
        }
        return m_nTextColor;
    }


    FormattedFieldBeautifier::~FormattedFieldBeautifier()
    {
    }


    void FormattedFieldBeautifier::setPlaceholderText( const uno::Reference< uno::XInterface >& _rxComponent )
    {
        try
        {
            uno::Reference< report::XFormattedField > xControlModel( _rxComponent, uno::UNO_QUERY );
            if ( xControlModel.is() )
            {
                OUString sDataField = xControlModel->getDataField();

                if ( !sDataField.isEmpty() )
                {
                    ReportFormula aFormula( sDataField );
                    bool bSet = true;
                    if ( aFormula.getType() == ReportFormula::Field )
                    {
                        const OUString& sColumnName = aFormula.getFieldName();
                        OUString sLabel = m_rReportController.getColumnLabel_throw(sColumnName);
                        if ( !sLabel.isEmpty() )
                        {
                            sDataField = "=" + sLabel;
                            bSet = false;
                        }
                    }
                    if ( bSet )
                        sDataField = aFormula.getEqualUndecoratedContent();
                }

                setPlaceholderText( getVclWindowPeer( xControlModel ), sDataField );
            }
        }
        catch (const uno::Exception &)
        {
            DBG_UNHANDLED_EXCEPTION("reportdesign");
        }
    }


    void FormattedFieldBeautifier::setPlaceholderText( const uno::Reference< awt::XVclWindowPeer >& _xVclWindowPeer, const OUString& _rText )
    {
        OSL_ENSURE( _xVclWindowPeer.is(), "FormattedFieldBeautifier::setPlaceholderText: invalid peer!" );
        if ( !_xVclWindowPeer.is() )
            throw uno::RuntimeException();

        // the actual text
        _xVclWindowPeer->setProperty(PROPERTY_TEXT, uno::Any(_rText));
        // the text color
        _xVclWindowPeer->setProperty(PROPERTY_TEXTCOLOR, uno::Any(getTextColor()));
        // font->italic
        uno::Any aFontDescriptor = _xVclWindowPeer->getProperty(PROPERTY_FONTDESCRIPTOR);
        awt::FontDescriptor aFontDescriptorStructure;
        aFontDescriptor >>= aFontDescriptorStructure;
        aFontDescriptorStructure.Slant = css::awt::FontSlant_ITALIC;
        _xVclWindowPeer->setProperty(PROPERTY_FONTDESCRIPTOR, uno::Any(aFontDescriptorStructure));
    }


    void FormattedFieldBeautifier::notifyPropertyChange( const beans::PropertyChangeEvent& _rEvent )
    {
        if  ( _rEvent.PropertyName != "DataField" )
            // not interested in
            return;

        setPlaceholderText( _rEvent.Source );
    }


    void FormattedFieldBeautifier::handle( const uno::Reference< uno::XInterface >& _rxElement )
    {
        setPlaceholderText( _rxElement );
    }


    void FormattedFieldBeautifier::notifyElementInserted( const uno::Reference< uno::XInterface >& _rxElement )
    {
        handle( _rxElement );
    }


    uno::Reference<awt::XVclWindowPeer> FormattedFieldBeautifier::getVclWindowPeer(const uno::Reference< report::XReportComponent >& _xComponent)
    {
        uno::Reference<awt::XVclWindowPeer> xVclWindowPeer;

        std::shared_ptr<OReportModel> pModel = m_rReportController.getSdrModel();

        uno::Reference<report::XSection> xSection(_xComponent->getSection());
        if ( xSection.is() )
        {
            OReportPage *pPage = pModel->getPage(xSection);
            const size_t nIndex = pPage->getIndexOf(_xComponent);
            if (nIndex < pPage->GetObjCount() )
            {
                SdrObject *pObject = pPage->GetObj(nIndex);
                OUnoObject* pUnoObj = dynamic_cast<OUnoObject*>(pObject);
                if ( pUnoObj ) // this doesn't need to be done for shapes
                {
                    OSectionWindow* pSectionWindow = m_rReportController.getSectionWindow(xSection);
                    if (pSectionWindow != nullptr)
                    {
                        OReportSection& aOutputDevice = pSectionWindow->getReportSection(); // OutputDevice
                        OSectionView& aSdrView = aOutputDevice.getSectionView();            // SdrView
                        uno::Reference<awt::XControl> xControl = pUnoObj->GetUnoControl(aSdrView, *aOutputDevice.GetOutDev());
                        xVclWindowPeer.set( xControl->getPeer(), uno::UNO_QUERY);
                    }
                }
            }
        }
        return xVclWindowPeer;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
