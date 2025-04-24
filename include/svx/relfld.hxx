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
#ifndef INCLUDED_SVX_RELFLD_HXX
#define INCLUDED_SVX_RELFLD_HXX

#include <tools/fldunit.hxx>
#include <svtools/unitconv.hxx>
#include <vcl/weld.hxx>
#include <svx/svxdllapi.h>

class SVX_DLLPUBLIC SvxRelativeField
{
private:
    std::unique_ptr<weld::MetricSpinButton> m_xSpinButton;

    FieldUnit eAbsoluteFieldUnit = FieldUnit::NONE;
    FieldUnit eFontRelativeFieldUnit = FieldUnit::NONE;
    sal_uInt16          nRelMin;
    sal_uInt16          nRelMax;
    bool                bRelativeMode;
    bool                bRelative;
    bool                bNegativeEnabled;
    bool bFontRelativeMode;

    DECL_DLLPRIVATE_LINK(ModifyHdl, weld::Entry&, void);

public:
    SvxRelativeField(std::unique_ptr<weld::MetricSpinButton> pControl);

    void            EnableRelativeMode( sal_uInt16 nMin, sal_uInt16 nMax );
    void EnableFontRelativeMode();
    void            SetRelative( bool bRelative );
    void SetFontRelative(FieldUnit eNewRelativeUnit);
    bool            IsRelative() const { return bRelative; }
    void            EnableNegativeMode() {bNegativeEnabled = true;}
    FieldUnit GetCurrentUnit() const { return eFontRelativeFieldUnit; }

    void set_sensitive(bool sensitive) { m_xSpinButton->set_sensitive(sensitive); }
    bool get_sensitive() const { return m_xSpinButton->get_sensitive(); }
    void set_value(sal_Int64 nValue, FieldUnit eValueUnit) { m_xSpinButton->set_value(nValue, eValueUnit); }
    sal_Int64 get_value(FieldUnit eDestUnit) const { return m_xSpinButton->get_value(eDestUnit); }
    sal_Int64 get_min(FieldUnit eValueUnit) const { return m_xSpinButton->get_min(eValueUnit); }
    void set_min(sal_Int64 min, FieldUnit eValueUnit) { m_xSpinButton->set_min(min, eValueUnit); }
    void set_max(sal_Int64 max, FieldUnit eValueUnit) { m_xSpinButton->set_max(max, eValueUnit); }
    sal_Int64 normalize(sal_Int64 nValue) const { return m_xSpinButton->normalize(nValue); }
    sal_Int64 denormalize(sal_Int64 nValue) const { return m_xSpinButton->denormalize(nValue); }
    void connect_value_changed(const Link<weld::MetricSpinButton&, void>& rLink) { m_xSpinButton->connect_value_changed(rLink); }
    OUString get_text() const { return m_xSpinButton->get_text(); }
    void set_text(const OUString& rText) { m_xSpinButton->set_text(rText); }
    void save_value() { m_xSpinButton->save_value(); }
    bool get_value_changed_from_saved() const { return m_xSpinButton->get_value_changed_from_saved(); }
    weld::SpinButton& get_widget() { return m_xSpinButton->get_widget(); }

    sal_Int64 GetCoreValue(MapUnit eUnit) const { return ::GetCoreValue(*m_xSpinButton, eUnit); }
    void SetFieldUnit(FieldUnit eUnit, bool bAll = false)
    {
        ::SetFieldUnit(*m_xSpinButton, eUnit, bAll);
        eAbsoluteFieldUnit = m_xSpinButton->get_unit();
    }
    void SetMetricValue(sal_Int64 lCoreValue, MapUnit eUnit) { ::SetMetricValue(*m_xSpinButton, lCoreValue, eUnit); }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
