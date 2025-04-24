/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; fill-column: 100 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

// Our mathml
#include "element.hxx"

// XML tools
#include <utility>
#include <comphelper/errcode.hxx>
#include <xmloff/xmlimp.hxx>

// Extras

class SfxMedium;
class SmDocShell;
class SmMLImport;
class SmModel;

class SmMLImportWrapper
{
    rtl::Reference<SmModel> m_xModel;
    SmDocShell* m_pDocShell;
    SmMLImport* m_pMlImport;

private:
    // Use customized entities

public:
    /** Get the element tree when parsed from text
    */
    SmMlElement* getElementTree();

public:
    /** Constructor
     */
    explicit SmMLImportWrapper(rtl::Reference<SmModel> xRef)
        : m_xModel(std::move(xRef))
        , m_pDocShell(nullptr)
        , m_pMlImport(nullptr)
    {
    }

    /** Imports the mathml
    */
    ErrCode Import(SfxMedium& rMedium);

    /** Imports the mathml
    */
    ErrCode Import(std::u16string_view aSource);

    /** read a component from input stream
     */
    ErrCode
    ReadThroughComponentIS(const css::uno::Reference<css::io::XInputStream>& xInputStream,
                           const css::uno::Reference<css::lang::XComponent>& xModelComponent,
                           css::uno::Reference<css::uno::XComponentContext> const& rxContext,
                           css::uno::Reference<css::beans::XPropertySet> const& rPropSet,
                           const char16_t* pFilterName, bool bEncrypted,
                           int_fast16_t nSyntaxVersion);

    /** read a component from storage
     */
    ErrCode ReadThroughComponentS(const css::uno::Reference<css::embed::XStorage>& xStorage,
                                  const css::uno::Reference<css::lang::XComponent>& xModelComponent,
                                  const char16_t* pStreamName,
                                  css::uno::Reference<css::uno::XComponentContext> const& rxContext,
                                  css::uno::Reference<css::beans::XPropertySet> const& rPropSet,
                                  const char16_t* pFilterName, int_fast16_t nSyntaxVersion);

    /** read a component from text
     */
    ErrCode
    ReadThroughComponentMS(std::u16string_view aText,
                           const css::uno::Reference<css::lang::XComponent>& xModelComponent,
                           css::uno::Reference<css::uno::XComponentContext> const& rxContext,
                           css::uno::Reference<css::beans::XPropertySet> const& rPropSet);
};

class SmMLImport final : public SvXMLImport
{
private:
    SmMlElement* m_pElementTree = new SmMlElement(SmMlElementType::NMlEmpty);
    bool m_bSuccess;
    size_t m_nSmSyntaxVersion;

public:
    /** Gets parsed element tree
    */
    SmMlElement* getElementTree() { return m_pElementTree; }

    /** Checks out if parse was a success
     */
    bool getSuccess() const { return m_bSuccess; }

public:
    /** Handles an error on the mathml structure
     */
    void declareMlError();

public:
    /** Constructor
    */
    SmMLImport(const css::uno::Reference<css::uno::XComponentContext>& rContext,
               OUString const& implementationName, SvXMLImportFlags nImportFlags);

    /** Destructor
    */
    virtual ~SmMLImport() noexcept override { cleanup(); };

public:
    /** End the document
    */
    void SAL_CALL endDocument() override;

    /** Create a fast context
    */
    SvXMLImportContext* CreateFastContext(
        sal_Int32 nElement,
        const css::uno::Reference<css::xml::sax::XFastAttributeList>& xAttrList) override;

    /** Imports view settings formula
    */
    virtual void
    SetViewSettings(const css::uno::Sequence<css::beans::PropertyValue>& aViewProps) override;

    /** Imports configurations settings formula
    */
    virtual void SetConfigurationSettings(
        const css::uno::Sequence<css::beans::PropertyValue>& aViewProps) override;

    /** Set syntax version
    */
    void SetSmSyntaxVersion(sal_Int16 nSmSyntaxVersion) { m_nSmSyntaxVersion = nSmSyntaxVersion; }

    /** Get syntax version
    */
    sal_Int16 GetSmSyntaxVersion() const { return m_nSmSyntaxVersion; }
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab cinoptions=b1,g0,N-s cinkeys+=0=break: */
