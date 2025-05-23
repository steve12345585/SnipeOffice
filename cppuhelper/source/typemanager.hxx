/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>

#include <cstddef>

#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/container/XSet.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/reflection/TypeDescriptionSearchDepth.hpp>
#include <com/sun/star/reflection/XTypeDescriptionEnumerationAccess.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <compbase2.hxx>
#include <rtl/ref.hxx>
#include <sal/types.h>

namespace com::sun::star {
    namespace reflection { class XTypeDescription; }
}
namespace unoidl {
    class ConstantGroupEntity;
    class Entity;
    class EnumTypeEntity;
    class Manager;
}

namespace cppuhelper {

typedef WeakComponentImplHelper2<
    css::lang::XServiceInfo, css::container::XHierarchicalNameAccess,
    css::container::XSet, css::reflection::XTypeDescriptionEnumerationAccess >
TypeManager_Base;

class TypeManager: public TypeManager_Base {
public:
    TypeManager();

    using TypeManager_Base::acquire;
    using TypeManager_Base::release;

    void init(std::u16string_view rdbUris);

    css::uno::Any find(OUString const & name);

    css::uno::Reference< css::reflection::XTypeDescription > resolve(
        OUString const & name);

private:
    virtual ~TypeManager() noexcept override;

    virtual OUString SAL_CALL getImplementationName() override;

    virtual sal_Bool SAL_CALL supportsService(OUString const & ServiceName) override;

    virtual css::uno::Sequence< OUString > SAL_CALL
    getSupportedServiceNames() override;

    virtual css::uno::Any SAL_CALL getByHierarchicalName(
        OUString const & aName) override;

    virtual sal_Bool SAL_CALL hasByHierarchicalName(OUString const & aName) override;

    virtual css::uno::Type SAL_CALL getElementType() override;

    virtual sal_Bool SAL_CALL hasElements() override;

    virtual css::uno::Reference< css::container::XEnumeration > SAL_CALL
    createEnumeration() override;

    virtual sal_Bool SAL_CALL has(css::uno::Any const & aElement) override;

    virtual void SAL_CALL insert(css::uno::Any const & aElement) override;

    virtual void SAL_CALL remove(css::uno::Any const & aElement) override;

    virtual css::uno::Reference< css::reflection::XTypeDescriptionEnumeration >
    SAL_CALL createTypeDescriptionEnumeration(
        OUString const & moduleName,
        css::uno::Sequence< css::uno::TypeClass > const & types,
        css::reflection::TypeDescriptionSearchDepth depth) override;

    void readRdbDirectory(std::u16string_view uri, bool optional);

    void readRdbFile(std::u16string_view uri, bool optional);

    css::uno::Any getSequenceType(OUString const & name);

    css::uno::Any getInstantiatedStruct(
        OUString const & name, sal_Int32 separator);

    css::uno::Any getInterfaceMember(
        std::u16string_view name, std::size_t separator);

    css::uno::Any getNamed(
        OUString const & name,
        rtl::Reference< unoidl::Entity > const & entity);

    static css::uno::Any getEnumMember(
        rtl::Reference< unoidl::EnumTypeEntity > const & entity,
        std::u16string_view member);

    static css::uno::Any getConstant(
        std::u16string_view constantGroupName,
        rtl::Reference< unoidl::ConstantGroupEntity > const & entity,
        std::u16string_view member);

    rtl::Reference< unoidl::Entity > findEntity(OUString const & name);

    rtl::Reference< unoidl::Manager > manager_;
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
