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

#include "ImageButton.hxx"
#include <tools/debug.hxx>
#include <tools/urlobj.hxx>
#include <vcl/svapp.hxx>
#include <osl/mutex.hxx>
#include <comphelper/basicio.hxx>
#include <com/sun/star/awt/MouseButton.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/form/FormComponentType.hpp>
#include <property.hxx>
#include <services.hxx>

namespace frm
{

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::form;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::util;

// OImageButtonModel
OImageButtonModel::OImageButtonModel(const Reference<XComponentContext>& _rxFactory)
                    :OClickableImageBaseModel( _rxFactory, VCL_CONTROLMODEL_IMAGEBUTTON, FRM_SUN_CONTROL_IMAGEBUTTON )
                    // use the old control name for compatibility reasons
{
    m_nClassId = FormComponentType::IMAGEBUTTON;
}

OImageButtonModel::OImageButtonModel( const OImageButtonModel* _pOriginal, const Reference<XComponentContext>& _rxFactory)
    :OClickableImageBaseModel( _pOriginal, _rxFactory )
{
    implInitializeImageURL();
}

css::uno::Reference< css::util::XCloneable > SAL_CALL OImageButtonModel::createClone()
{
    rtl::Reference<OImageButtonModel> pClone = new OImageButtonModel(this, getContext());
    pClone->clonedFrom(this);
    return pClone;
}

OImageButtonModel::~OImageButtonModel()
{
}

// XServiceInfo
css::uno::Sequence<OUString>  OImageButtonModel::getSupportedServiceNames()
{
    css::uno::Sequence<OUString> aSupported = OClickableImageBaseModel::getSupportedServiceNames();
    aSupported.realloc(aSupported.getLength() + 2);

    OUString*pArray = aSupported.getArray();
    pArray[aSupported.getLength()-2] = FRM_SUN_COMPONENT_IMAGEBUTTON;
    pArray[aSupported.getLength()-1] = FRM_COMPONENT_IMAGEBUTTON;
    return aSupported;
}

void OImageButtonModel::describeFixedProperties( Sequence< Property >& _rProps ) const
{
    OClickableImageBaseModel::describeFixedProperties( _rProps );
    sal_Int32 nOldCount = _rProps.getLength();
    _rProps.realloc( nOldCount + 5);
    css::beans::Property* pProperties = _rProps.getArray() + nOldCount;
    *pProperties++ = css::beans::Property(PROPERTY_BUTTONTYPE, PROPERTY_ID_BUTTONTYPE, cppu::UnoType<FormButtonType>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_DISPATCHURLINTERNAL, PROPERTY_ID_DISPATCHURLINTERNAL, cppu::UnoType<sal_Bool>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_TARGET_URL, PROPERTY_ID_TARGET_URL, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_TARGET_FRAME, PROPERTY_ID_TARGET_FRAME, cppu::UnoType<OUString>::get(), css::beans::PropertyAttribute::BOUND);
    *pProperties++ = css::beans::Property(PROPERTY_TABINDEX, PROPERTY_ID_TABINDEX, cppu::UnoType<sal_Int16>::get(), css::beans::PropertyAttribute::BOUND);
    DBG_ASSERT( pProperties == _rProps.getArray() + _rProps.getLength(), "<...>::describeFixedProperties/getInfoHelper: forgot to adjust the count ?");
}

OUString OImageButtonModel::getServiceName()
{
    return FRM_COMPONENT_IMAGEBUTTON;   // old (non-sun) name for compatibility !
}

void OImageButtonModel::write(const Reference<XObjectOutputStream>& _rxOutStream)
{
    OControlModel::write(_rxOutStream);

    // Version
    _rxOutStream->writeShort(0x0003);
    _rxOutStream->writeShort(static_cast<sal_uInt16>(m_eButtonType));

    OUString sTmp(INetURLObject::decode( m_sTargetURL, INetURLObject::DecodeMechanism::Unambiguous));
    _rxOutStream << sTmp;
    _rxOutStream << m_sTargetFrame;
    writeHelpTextCompatibly(_rxOutStream);
}

void OImageButtonModel::read(const Reference<XObjectInputStream>& _rxInStream)
{
    OControlModel::read(_rxInStream);

    // Version
    sal_uInt16 nVersion = _rxInStream->readShort();

    switch (nVersion)
    {
        case 0x0001:
        {
            m_eButtonType = static_cast<FormButtonType>(_rxInStream->readShort());
        }
        break;
        case 0x0002:
        {
            m_eButtonType = static_cast<FormButtonType>(_rxInStream->readShort());
            _rxInStream >> m_sTargetURL;
            _rxInStream >> m_sTargetFrame;
        }
        break;
        case 0x0003:
        {
            m_eButtonType = static_cast<FormButtonType>(_rxInStream->readShort());
            _rxInStream >> m_sTargetURL;
            _rxInStream >> m_sTargetFrame;
            readHelpTextCompatibly(_rxInStream);
        }
        break;

        default :
            OSL_FAIL("OImageButtonModel::read : unknown version !");
            m_eButtonType = FormButtonType_PUSH;
            m_sTargetURL.clear();
            m_sTargetFrame.clear();
            break;
    }
}

// OImageButtonControl
Sequence<Type> OImageButtonControl::_getTypes()
{
    static Sequence<Type> const aTypes =
        concatSequences(OClickableImageBaseControl::_getTypes(), OImageButtonControl_BASE::getTypes());
    return aTypes;
}

css::uno::Sequence<OUString>  OImageButtonControl::getSupportedServiceNames()
{
    css::uno::Sequence<OUString> aSupported = OClickableImageBaseControl::getSupportedServiceNames();
    aSupported.realloc(aSupported.getLength() + 2);

    OUString*pArray = aSupported.getArray();
    pArray[aSupported.getLength()-2] = FRM_SUN_CONTROL_IMAGEBUTTON;
    pArray[aSupported.getLength()-1] = STARDIV_ONE_FORM_CONTROL_IMAGEBUTTON;
    return aSupported;
}

OImageButtonControl::OImageButtonControl(const Reference<XComponentContext>& _rxFactory)
            :OClickableImageBaseControl(_rxFactory, VCL_CONTROL_IMAGEBUTTON)
{
    osl_atomic_increment(&m_refCount);
    {
        // Register as MouseListener
        if (auto xComp = query_aggregation<awt::XWindow>(m_xAggregate))
            xComp->addMouseListener( static_cast< awt::XMouseListener* >( this ) );
    }
    osl_atomic_decrement(&m_refCount);
}

// UNO Binding
Any SAL_CALL OImageButtonControl::queryAggregation(const Type& _rType)
{
    Any aReturn = OClickableImageBaseControl::queryAggregation(_rType);
    if (!aReturn.hasValue())
        aReturn = OImageButtonControl_BASE::queryInterface(_rType);

    return aReturn;
}

void OImageButtonControl::mousePressed(const awt::MouseEvent& e)
{
    SolarMutexGuard aSolarGuard;

    if (e.Buttons != awt::MouseButton::LEFT)
        return;

    ::osl::ClearableMutexGuard aGuard( m_aMutex );
    if( m_aApproveActionListeners.getLength() )
    {
        // if there are listeners, start the action in an own thread, to not allow
        // them to block us here (we're in the application's main thread)
        getImageProducerThread()->OComponentEventThread::addEvent( std::make_unique<awt::MouseEvent>(e) );
    }
    else
    {
        // Or else don't; we must not notify the listeners in that case.
        // Even not if it's added later on.
        aGuard.clear();
        actionPerformed_Impl( false, e );
    }
}

void SAL_CALL OImageButtonControl::mouseReleased(const awt::MouseEvent& /*e*/)
{
}

void SAL_CALL OImageButtonControl::mouseEntered(const awt::MouseEvent& /*e*/)
{
}

void SAL_CALL OImageButtonControl::mouseExited(const awt::MouseEvent& /*e*/)
{
}

}   // namespace frm

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_form_OImageButtonModel_get_implementation(css::uno::XComponentContext* component,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new frm::OImageButtonModel(component));
}

extern "C" SAL_DLLPUBLIC_EXPORT css::uno::XInterface*
com_sun_star_form_OImageButtonControl_get_implementation(css::uno::XComponentContext* component,
        css::uno::Sequence<css::uno::Any> const &)
{
    return cppu::acquire(new frm::OImageButtonControl(component));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
