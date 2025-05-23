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

#include "FormattedFieldWrapper.hxx"
#include "Edit.hxx"
#include "FormattedField.hxx"
#include <services.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <connectivity/dbtools.hxx>
#include <tools/debug.hxx>
#include <vcl/svapp.hxx>
#include <vcl/settings.hxx>
#include <i18nlangtag/languagetag.hxx>
#include <com/sun/star/io/XMarkableStream.hpp>

using namespace comphelper;
using namespace frm;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::util;

OFormattedFieldWrapper::OFormattedFieldWrapper(const Reference<XComponentContext>& _rxFactory,
                                               OUString const & implementationName)
    :m_xContext(_rxFactory)
    ,m_implementationName(implementationName)
{
}

css::uno::Reference<css::uno::XInterface> OFormattedFieldWrapper::createFormattedFieldWrapper(const css::uno::Reference< css::uno::XComponentContext>& _rxFactory, bool bActAsFormatted, OUString const & implementationName)
{
    rtl::Reference<OFormattedFieldWrapper> pRef = new OFormattedFieldWrapper(_rxFactory,
                                                                             implementationName);

    if (bActAsFormatted)
    {
        // instantiate a FormattedModel
        // (instantiate it directly ..., as the OFormattedModel isn't
        // registered for any service names anymore)
        rtl::Reference<OFormattedModel> pModel = new OFormattedModel(pRef->m_xContext);

        pRef->m_xAggregate = pModel;
        OSL_ENSURE(pRef->m_xAggregate.is(), "the OFormattedModel didn't have an XAggregation interface !");

        // _before_ setting the delegator, give it to the member references
        pRef->m_xFormattedPart = pModel;
        pRef->m_pEditPart.set(new OEditModel(pRef->m_xContext));
    }

    if (pRef->m_xAggregate.is())
    {   // has to be in its own block because of the temporary variable created by *this
        pRef->m_xAggregate->setDelegator(static_cast<XWeak*>(pRef.get()));
    }

    css::uno::Reference<css::uno::XInterface> xRef(*pRef);

    return xRef;
}

Reference< XCloneable > SAL_CALL OFormattedFieldWrapper::createClone()
{
    ensureAggregate();

    rtl::Reference< OFormattedFieldWrapper > xRef(new OFormattedFieldWrapper(m_xContext,
                                                                             m_implementationName));

    auto xCloneAccess = query_aggregation<XCloneable>(m_xAggregate);

    // clone the aggregate
    if ( m_xAggregate.is() )
    {
        xRef->m_xAggregate.set(static_cast<OEditBaseModel*>(m_xAggregate->createClone().get()));
        OSL_ENSURE(xRef->m_xAggregate.is(), "invalid aggregate cloned !");

        xRef->m_xFormattedPart = xRef->m_xAggregate;

        if ( m_pEditPart.is() )
        {
            xRef->m_pEditPart.set( new OEditModel(m_pEditPart.get(), m_xContext) );
        }
    }
    else
    {   // the clone source does not yet have an aggregate -> we don't yet need one, too
    }

    if ( xRef->m_xAggregate.is() )
    {   // has to be in its own block because of the temporary variable created by *this
        xRef->m_xAggregate->setDelegator(static_cast< XWeak* >(xRef.get()));
    }

    return xRef;
}

OFormattedFieldWrapper::~OFormattedFieldWrapper()
{
    // release the aggregated object (if any)
    if (m_xAggregate.is())
        m_xAggregate->setDelegator(css::uno::Reference<css::uno::XInterface> ());

}

Any SAL_CALL OFormattedFieldWrapper::queryAggregation(const Type& _rType)
{
    Any aReturn;

    if (_rType.equals( cppu::UnoType<XTypeProvider>::get() ) )
    {   // a XTypeProvider interface needs a working aggregate - we don't want to give the type provider
        // of our base class (OFormattedFieldWrapper_Base) to the caller as it supplies nearly nothing
        ensureAggregate();
        if (m_xAggregate.is())
            aReturn = m_xAggregate->queryAggregation(_rType);
    }

    if (!aReturn.hasValue())
    {
        aReturn = OFormattedFieldWrapper_Base::queryAggregation(_rType);

        if ((_rType.equals( cppu::UnoType<XServiceInfo>::get() ) ) && aReturn.hasValue())
        {   // somebody requested an XServiceInfo interface and our base class provided it
            // check our aggregate if it has one, too
            ensureAggregate();
        }

        if (!aReturn.hasValue())
        {
            aReturn = ::cppu::queryInterface( _rType,
                static_cast< XPersistObject* >( this ),
                static_cast< XCloneable* >( this )
            );

            if (!aReturn.hasValue())
            {
                // somebody requests an interface other than the basics (XInterface) and other than
                // the two we can supply without an aggregate. So ensure
                // the aggregate exists.
                ensureAggregate();
                if (m_xAggregate.is())
                    aReturn = m_xAggregate->queryAggregation(_rType);
            }
        }
    }

    return aReturn;
}

OUString SAL_CALL OFormattedFieldWrapper::getServiceName()
{
    // return the old compatibility name for an EditModel
    return FRM_COMPONENT_EDIT;
}

OUString SAL_CALL OFormattedFieldWrapper::getImplementationName(  )
{
    return m_implementationName;
}

sal_Bool SAL_CALL OFormattedFieldWrapper::supportsService( const OUString& _rServiceName )
{
    return cppu::supportsService(this, _rServiceName);
}

Sequence< OUString > SAL_CALL OFormattedFieldWrapper::getSupportedServiceNames(  )
{
    DBG_ASSERT(m_xAggregate.is(), "OFormattedFieldWrapper::getSupportedServiceNames: should never have made it 'til here without an aggregate!");
    Reference< XServiceInfo > xSI;
    m_xAggregate->queryAggregation(cppu::UnoType<XServiceInfo>::get()) >>= xSI;
    return xSI->getSupportedServiceNames();
}

void SAL_CALL OFormattedFieldWrapper::write(const Reference<XObjectOutputStream>& _rxOutStream)
{
    // can't write myself
    ensureAggregate();

    // if we act as real edit field, we can simple forward this write request
    if (!m_xFormattedPart.is())
    {
        auto xAggregatePersistence = query_aggregation<XPersistObject>(m_xAggregate);
        DBG_ASSERT(xAggregatePersistence.is(), "OFormattedFieldWrapper::write : don't know how to handle this : can't write !");
            // oops ... We gave an XPersistObject interface to the caller but now we aren't an XPersistObject ...
        if (xAggregatePersistence.is())
            xAggregatePersistence->write(_rxOutStream);
        return;
    }

    // else we have to write an edit part first
    OSL_ENSURE(m_pEditPart.is(), "OFormattedFieldWrapper::write : formatted part without edit part ?");
    if ( !m_pEditPart.is() )
        throw RuntimeException( OUString(), *this );

    // for this we transfer the current props of the formatted part to the edit part

    Locale aAppLanguage = Application::GetSettings().GetUILanguageTag().getLocale();
    dbtools::TransferFormComponentProperties(m_xFormattedPart, m_pEditPart, aAppLanguage);

    // then write the edit part, after switching to "fake mode"
    m_pEditPart->enableFormattedWriteFake();
    m_pEditPart->write(_rxOutStream);
    m_pEditPart->disableFormattedWriteFake();

    // and finally write the formatted part we're really interested in
    m_xFormattedPart->write(_rxOutStream);
}

void SAL_CALL OFormattedFieldWrapper::read(const Reference<XObjectInputStream>& _rxInStream)
{
    SolarMutexGuard g;
    if (m_xAggregate.is())
    {   //  we already made a decision if we're an EditModel or a FormattedModel

        // if we act as formatted, we have to read the edit part first
        if (m_xFormattedPart.is())
        {
            // two possible cases:
            // a) the stuff was written by a version which didn't work with an Edit header (all intermediate
            //      versions >5.1 && <=568)
            // b) it was written by a version using edit headers
            // as we can distinguish a) from b) only after we have read the edit part, we need to remember the
            // position
            Reference<XMarkableStream>  xInMarkable(_rxInStream, UNO_QUERY);
            DBG_ASSERT(xInMarkable.is(), "OFormattedFieldWrapper::read : can only work with markable streams !");
            sal_Int32 nBeforeEditPart = xInMarkable->createMark();

            m_pEditPart->read(_rxInStream);
            // this only works because an edit model can read the stuff written by a formatted model
            // (maybe with some assertions) , but not vice versa
            if (!m_pEditPart->lastReadWasFormattedFake())
            {   // case a), written with a version without the edit part fake, so seek to the start position, again
                xInMarkable->jumpToMark(nBeforeEditPart);
            }
            xInMarkable->deleteMark(nBeforeEditPart);
        }

        auto xAggregatePersistence = query_aggregation<XPersistObject>(m_xAggregate);
        DBG_ASSERT(xAggregatePersistence.is(), "OFormattedFieldWrapper::read : don't know how to handle this : can't read !");
            // oops ... We gave an XPersistObject interface to the caller but now we aren't an XPersistObject ...

        if (xAggregatePersistence.is())
            xAggregatePersistence->read(_rxInStream);
        return;
    }

    // we have to decide from the data within the stream whether we should
    // be an EditModel or a FormattedModel

    {
        // let an OEditModel do the reading
        rtl::Reference< OEditModel > pBasicReader(new OEditModel(m_xContext));
        pBasicReader->read(_rxInStream);

        // was it really an edit model ?
        if (!pBasicReader->lastReadWasFormattedFake())
        {
            // yes -> all fine
            m_xAggregate = std::move(pBasicReader);
        }
        else
        {   // no -> substitute it with a formatted model
            // let the formatted model do the reading
            m_xFormattedPart.set(new OFormattedModel(m_xContext));
            m_xFormattedPart->read(_rxInStream);
            m_pEditPart = std::move(pBasicReader);
            m_xAggregate = m_xFormattedPart;
        }
    }

    // do the aggregation
    osl_atomic_increment(&m_refCount);
    if (m_xAggregate.is())
    {   // has to be in its own block because of the temporary variable created by *this
        m_xAggregate->setDelegator(static_cast<XWeak*>(this));
    }
    osl_atomic_decrement(&m_refCount);
}

void OFormattedFieldWrapper::ensureAggregate()
{
    if (m_xAggregate.is())
        return;

    {
        // instantiate an EditModel (the only place where we are allowed to decide that we're a FormattedModel
        // is in ::read)
        rtl::Reference<OEditModel> xEditModel = new OEditModel(m_xContext);
        m_xAggregate = xEditModel;
        DBG_ASSERT(m_xAggregate.is(), "OFormattedFieldWrapper::ensureAggregate : the OEditModel didn't have an XAggregation interface !");
    }

    osl_atomic_increment(&m_refCount);
    if (m_xAggregate.is())
    {   // has to be in its own block because of the temporary variable created by *this
        m_xAggregate->setDelegator(static_cast<XWeak*>(this));
    }
    osl_atomic_decrement(&m_refCount);
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_form_OFormattedFieldWrapper_get_implementation(css::uno::XComponentContext* component,
        css::uno::Sequence<css::uno::Any> const &)
{
    css::uno::Reference<css::uno::XInterface> inst(
        OFormattedFieldWrapper::createFormattedFieldWrapper(
            component, false, u"com.sun.star.form.OFormattedFieldWrapper"_ustr));
    inst->acquire();
    return inst.get();
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_comp_forms_OFormattedFieldWrapper_ForcedFormatted_get_implementation(css::uno::XComponentContext* component,
        css::uno::Sequence<css::uno::Any> const &)
{
    css::uno::Reference<css::uno::XInterface> inst(
        OFormattedFieldWrapper::createFormattedFieldWrapper(
            component, true, u"com.sun.star.comp.forms.OFormattedFieldWrapper_ForcedFormatted"_ustr));
    inst->acquire();
    return inst.get();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
