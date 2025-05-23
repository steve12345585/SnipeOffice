/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_COMPHELPER_CONFIGURATIONLISTENER_HXX
#define INCLUDED_COMPHELPER_CONFIGURATIONLISTENER_HXX

#include <vector>
#include <comphelper/comphelperdllapi.h>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertyChangeListener.hpp>
#include <rtl/ref.hxx>
#include <cppuhelper/implbase.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/configurationhelper.hxx>

namespace com::sun::star::uno { class XComponentContext; }

namespace comphelper {

class ConfigurationListener;

class COMPHELPER_DLLPUBLIC ConfigurationListenerPropertyBase {
public:
    OUString maName;
    rtl::Reference<ConfigurationListener> mxListener;

    virtual ~ConfigurationListenerPropertyBase() {}
    virtual void setProperty(const css::uno::Any &aProperty) = 0;
    void dispose() { mxListener.clear(); }
};

/// Access to this class must be protected by the SolarMutex
template< typename uno_type > class ConfigurationListenerProperty : public ConfigurationListenerPropertyBase
{
    uno_type maValue;
protected:
    virtual void setProperty(const css::uno::Any &aProperty) override
    {
        aProperty >>= maValue;
    }
public:
    /**
     * Provide a mirror of the configmgr's version of this property
     * for the lifecycle of this property. The property value tracks
     * the same value in the configuration.
     */
    inline ConfigurationListenerProperty(const rtl::Reference< ConfigurationListener > &xListener,
                                             const OUString &rProp );

    virtual inline ~ConfigurationListenerProperty() override;

    uno_type get() const { return maValue; }
};

// workaround for incremental linking bugs in MSVC2019
class SAL_DLLPUBLIC_TEMPLATE ConfigurationListener_Base : public cppu::WeakImplHelper< css::beans::XPropertyChangeListener > {};
class COMPHELPER_DLLPUBLIC ConfigurationListener final : public ConfigurationListener_Base
{
    css::uno::Reference< css::beans::XPropertySet > mxConfig;
    std::vector< ConfigurationListenerPropertyBase * > maListeners;
    bool mbDisposed;
public:
    /// Public health warning, you -must- dispose this if you use it.
    ConfigurationListener(const OUString &rPath,
                          css::uno::Reference< css::uno::XComponentContext >
                          const & xContext = comphelper::getProcessComponentContext())
        : mxConfig( ConfigurationHelper::openConfig( xContext, rPath, EConfigurationModes::ReadOnly ),
                    css::uno::UNO_QUERY_THROW )
        , mbDisposed(false)
    { }

    virtual ~ConfigurationListener() override
    {
        dispose();
    }

    /// Listen for the specific property denoted by the listener
    void addListener(ConfigurationListenerPropertyBase *pListener);

    /// Stop listening.
    void removeListener(ConfigurationListenerPropertyBase *pListener);

    /// Release various circular references
    void dispose();

    // XPropertyChangeListener implementation
    virtual void SAL_CALL disposing(css::lang::EventObject const &) override;

    /// Notify of the property change
    virtual void SAL_CALL propertyChange(
        css::beans::PropertyChangeEvent const &rEvt ) override;

    bool isDisposed() const { return mbDisposed; }
};

template< typename uno_type > ConfigurationListenerProperty< uno_type >::ConfigurationListenerProperty(const rtl::Reference< ConfigurationListener > &xListener, const OUString &rProp )
    : maValue()
{
    maName = rProp;
    mxListener = xListener;
    mxListener->addListener(this);
}

template< typename uno_type > ConfigurationListenerProperty< uno_type >::~ConfigurationListenerProperty()
{
    if (mxListener.is())
        mxListener->removeListener(this);
}

} // namespace comphelper

#endif // INCLUDED_COMPHELPER_CONFIGURATIONLISTENER_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
