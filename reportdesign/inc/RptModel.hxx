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

#ifndef INCLUDED_REPORTDESIGN_INC_RPTMODEL_HXX
#define INCLUDED_REPORTDESIGN_INC_RPTMODEL_HXX

#include "dllapi.h"
#include <svx/svdmodel.hxx>
#include <com/sun/star/report/XReportDefinition.hpp>

namespace dbaui
{
    class DBSubComponentController;
}
namespace reportdesign
{
    class OReportDefinition;
}
namespace rptui
{
class OReportPage;
class OXUndoEnvironment;

class UNLESS_MERGELIBS_MORE(REPORTDESIGN_DLLPUBLIC) OReportModel final : public SdrModel
{
    friend class OReportPage;

private:
    rtl::Reference<OXUndoEnvironment>   m_xUndoEnv;
    ::dbaui::DBSubComponentController*  m_pController;
    ::reportdesign::OReportDefinition*  m_pReportDefinition;

    virtual css::uno::Reference< css::frame::XModel > createUnoModel() override;

    OReportModel( const OReportModel& ) = delete;
    void operator=(const OReportModel& rSrcModel) = delete;

public:

    OReportModel(::reportdesign::OReportDefinition* _pReportDefinition);
    virtual ~OReportModel() override;

    virtual void        SetChanged(bool bFlg = true) override;
    virtual rtl::Reference<SdrPage> AllocPage(bool bMasterPage) override;
    virtual rtl::Reference<SdrPage> RemovePage(sal_uInt16 nPgNum) override;
    /** @returns the numbering type that is used to format page fields in drawing shapes */
    virtual SvxNumType  GetPageNumType() const override;

    OXUndoEnvironment&  GetUndoEnv() { return *m_xUndoEnv;}
    void                SetModified(bool _bModified);

    dbaui::DBSubComponentController* getController() const { return m_pController; }
    void attachController( dbaui::DBSubComponentController& _rController ) { m_pController = &_rController; }
    void detachController();

    OReportPage* createNewPage(const css::uno::Reference< css::report::XSection >& _xSection);

    /** returns the page which belongs to a section
    *
    * @param _xSection
    * @return The page or <NULL/> when no page could be found.
    */
    OReportPage* getPage(const css::uno::Reference< css::report::XSection >& _xSection);

    /// returns the XReportDefinition which the OReportModel belongs to
    css::uno::Reference< css::report::XReportDefinition >
                getReportDefinition() const;

    css::uno::Reference< css::uno::XInterface > createShape(const OUString& aServiceSpecifier,css::uno::Reference< css::drawing::XShape >& _rShape,sal_Int32 nOrientation = -1);
};
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
