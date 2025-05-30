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

#ifndef INCLUDED_EDITENG_UNONRULE_HXX
#define INCLUDED_EDITENG_UNONRULE_HXX

#include <com/sun/star/container/XIndexReplace.hpp>
#include <com/sun/star/ucb/XAnyCompare.hpp>
#include <editeng/editengdllapi.h>
#include <cppuhelper/implbase.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/util/XCloneable.hpp>
#include <editeng/numitem.hxx>

namespace com::sun::star::beans { struct PropertyValue; }

EDITENG_DLLPUBLIC css::uno::Reference< css::container::XIndexReplace > SvxCreateNumRule(const SvxNumRule& rRule);
css::uno::Reference< css::container::XIndexReplace > SvxCreateNumRule();
/// @throws css::lang::IllegalArgumentException
const SvxNumRule& SvxGetNumRule( css::uno::Reference< css::container::XIndexReplace > const & xRule );
EDITENG_DLLPUBLIC css::uno::Reference< css::ucb::XAnyCompare > SvxCreateNumRuleCompare() noexcept;

class SvxUnoNumberingRules final : public ::cppu::WeakImplHelper< css::container::XIndexReplace, css::ucb::XAnyCompare,
    css::util::XCloneable, css::lang::XServiceInfo >
{
private:
    SvxNumRule maRule;
public:
    SvxUnoNumberingRules(SvxNumRule aRule);
    virtual ~SvxUnoNumberingRules() noexcept override;

    //XIndexReplace
    virtual void SAL_CALL replaceByIndex( sal_Int32 Index, const css::uno::Any& Element ) override;

    //XIndexAccess
    virtual sal_Int32 SAL_CALL getCount() override ;
    virtual css::uno::Any SAL_CALL getByIndex( sal_Int32 Index ) override;

    //XElementAccess
    virtual css::uno::Type SAL_CALL getElementType() override;
    virtual sal_Bool SAL_CALL hasElements() override;

    // XAnyCompare
    virtual sal_Int16 SAL_CALL compare( const css::uno::Any& Any1, const css::uno::Any& Any2 ) override;

    // XCloneable
    virtual css::uno::Reference< css::util::XCloneable > SAL_CALL createClone(  ) override;

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName(  ) override;
    virtual sal_Bool SAL_CALL supportsService( const OUString& ServiceName ) override;
    virtual css::uno::Sequence< OUString > SAL_CALL getSupportedServiceNames(  ) override;

    // internal
    /// @throws css::uno::RuntimeException
    css::uno::Sequence<css::beans::PropertyValue> getNumberingRuleByIndex( sal_Int32 nIndex) const;
    /// @throws css::uno::RuntimeException
    /// @throws css::lang::IllegalArgumentException
    void setNumberingRuleByIndex(const css::uno::Sequence<css::beans::PropertyValue>& rProperties, sal_Int32 nIndex);

    static sal_Int16 Compare( const css::uno::Any& rAny1, const css::uno::Any& rAny2 );

    const SvxNumRule& getNumRule() const { return maRule; }
};


#endif


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
