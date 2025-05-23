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

#include "TabPageNotifiable.hxx"
#include <vcl/weld.hxx>
#include <memory>

class BuilderPage;

namespace chart
{
class ChartModel;
class ChartTypeTemplateProvider;
class DataSourceTabPage;
class DialogModel;
class RangeChooserTabPage;

class DataSourceDialog final :
        public weld::GenericDialogController,
        public TabPageNotifiable
{
public:
    explicit DataSourceDialog(
        weld::Window * pParent,
        const rtl::Reference<::chart::ChartModel> & xChartDocument );
    virtual ~DataSourceDialog() override;

    // TabPageNotifiable
    virtual void setInvalidPage( BuilderPage * pTabPage ) override;
    virtual void setValidPage( BuilderPage * pTabPage ) override;

private:
    DECL_LINK(ActivatePageHdl, const OUString&, void);
    DECL_LINK(DeactivatePageHdl, const OUString&, bool);
    DECL_LINK(OkHdl, weld::Button&, void);
    void commitPages();

    std::unique_ptr< ChartTypeTemplateProvider >  m_apDocTemplateProvider;
    std::unique_ptr< DialogModel >                m_apDialogModel;

    std::unique_ptr<RangeChooserTabPage> m_xRangeChooserTabPage;
    std::unique_ptr<DataSourceTabPage> m_xDataSourceTabPage;
    bool                  m_bRangeChooserTabIsValid;
    bool                  m_bDataSourceTabIsValid;
    bool                  m_bTogglingEnabled;

    std::unique_ptr<weld::Notebook> m_xTabControl;
    std::unique_ptr<weld::Button> m_xBtnOK;

    static sal_uInt16         m_nLastPageId;
};

} //  namespace chart

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
