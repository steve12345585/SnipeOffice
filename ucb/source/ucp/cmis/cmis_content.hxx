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

#include <string_view>

#include "cmis_url.hxx"
#include "children_provider.hxx"

#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/ucb/CheckinArgument.hpp>
#include <com/sun/star/ucb/CommandFailedException.hpp>
#include <com/sun/star/ucb/ContentCreationException.hpp>
#include <com/sun/star/ucb/OpenCommandArgument2.hpp>
#include <com/sun/star/ucb/TransferInfo.hpp>
#include <com/sun/star/ucb/XContentCreator.hpp>
#include <com/sun/star/document/CmisVersion.hpp>
#include <ucbhelper/contenthelper.hxx>

#if defined __GNUC__ && !defined __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated"
#pragma GCC diagnostic ignored "-Wunused-but-set-parameter"
#endif
#include <libcmis/libcmis.hxx>
#if defined __GNUC__ && !defined __clang__
#pragma GCC diagnostic pop
#endif

namespace com::sun::star {
    namespace beans {
        struct Property;
        struct PropertyValue;
    }
    namespace sdbc {
        class XRow;
    }
}
namespace ucbhelper
{
    class Content;
}


namespace cmis
{

inline constexpr OUString CMIS_FILE_TYPE = u"application/vnd.libreoffice.cmis-file"_ustr;
inline constexpr OUString CMIS_FOLDER_TYPE = u"application/vnd.libreoffice.cmis-folder"_ustr;

class ContentProvider;
class Content : public ::ucbhelper::ContentImplHelper,
                public css::ucb::XContentCreator,
                public ChildrenProvider
{
private:
    ContentProvider*       m_pProvider;
    libcmis::Session*      m_pSession;
    libcmis::ObjectPtr     m_pObject;
    OUString          m_sObjectPath;
    OUString          m_sObjectId;
    OUString          m_sURL;
    cmis::URL         m_aURL;

    // Members to be set for non-persistent content
    bool                   m_bTransient;
    bool                   m_bIsFolder;
    libcmis::ObjectTypePtr m_pObjectType;
    std::map< std::string, libcmis::PropertyPtr > m_pObjectProps;

    bool isFolder( const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );
    void setCmisProperty(const std::string& rName, const std::string& rValue,
            const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    css::uno::Any getBadArgExcept();

    css::uno::Reference< css::sdbc::XRow >
        getPropertyValues(
            const css::uno::Sequence< css::beans::Property >& rProperties,
            const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    libcmis::Session* getSession( const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );
    libcmis::ObjectTypePtr const & getObjectType( const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

private:
    typedef rtl::Reference< Content > ContentRef;
    typedef std::vector< ContentRef > ContentRefList;

    /// @throws css::uno::Exception
    /// @throws libcmis::Exception
    css::uno::Any open(const css::ucb::OpenCommandArgument2 & rArg,
        const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    /// @throws css::uno::Exception
    void transfer( const css::ucb::TransferInfo& rTransferInfo,
        const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    /// @throws css::uno::Exception
    void insert( const css::uno::Reference< css::io::XInputStream > & xInputStream,
        bool bReplaceExisting, std::u16string_view rMimeType,
        const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    /// @throws css::uno::Exception
    OUString checkIn( const css::ucb::CheckinArgument& rArg,
        const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    /// @throws css::uno::Exception
    OUString checkOut( const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    /// @throws css::uno::Exception
    OUString cancelCheckOut( const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    static void copyData( const css::uno::Reference< css::io::XInputStream >& xIn,
        const css::uno::Reference< css::io::XOutputStream >& xOut );

    css::uno::Sequence< css::uno::Any >
        setPropertyValues( const css::uno::Sequence< css::beans::PropertyValue >& rValues,
            const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    /// @throws css::uno::Exception
    css::uno::Sequence< css::document::CmisVersion >
        getAllVersions( const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv );

    bool feedSink( const css::uno::Reference< css::uno::XInterface>& aSink,
        const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

public:
    /// @throws css::ucb::ContentCreationException
    Content( const css::uno::Reference< css::uno::XComponentContext >& rxContext,
        ContentProvider *pProvider,
        const css::uno::Reference< css::ucb::XContentIdentifier >& Identifier,
        libcmis::ObjectPtr pObject = libcmis::ObjectPtr( ) );

    /// @throws css::ucb::ContentCreationException
    Content( const css::uno::Reference< css::uno::XComponentContext >& rxContext,
        ContentProvider *pProvider,
        const css::uno::Reference< css::ucb::XContentIdentifier >& Identifier,
        bool bIsFolder);

    virtual ~Content() override;

    virtual css::uno::Sequence< css::beans::Property >
        getProperties( const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv ) override;

    libcmis::ObjectPtr updateProperties(
            const css::uno::Any& iCmisProps,
            const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv);

    virtual css::uno::Sequence< css::ucb::CommandInfo >
        getCommands( const css::uno::Reference< css::ucb::XCommandEnvironment > & xEnv ) override;

    virtual OUString getParentURL() override;

    // XInterface
    virtual css::uno::Any SAL_CALL queryInterface( const css::uno::Type & rType ) override;
    virtual void SAL_CALL acquire()
        noexcept override;
    virtual void SAL_CALL release()
        noexcept override;

    virtual css::uno::Sequence< sal_Int8 > SAL_CALL getImplementationId() override;
    virtual css::uno::Sequence< css::uno::Type > SAL_CALL getTypes() override;

    virtual OUString SAL_CALL
    getImplementationName() override;

    virtual css::uno::Sequence< OUString > SAL_CALL
    getSupportedServiceNames() override;

    virtual OUString SAL_CALL
    getContentType() override;

    virtual css::uno::Any SAL_CALL
        execute( const css::ucb::Command& aCommand,
        sal_Int32 CommandId,
        const css::uno::Reference< css::ucb::XCommandEnvironment >& Environment ) override;

    virtual void SAL_CALL abort( sal_Int32 CommandId ) override;

    virtual css::uno::Sequence< css::ucb::ContentInfo >
        SAL_CALL queryCreatableContentsInfo() override;

    virtual css::uno::Reference< css::ucb::XContent >
        SAL_CALL createNewContent( const css::ucb::ContentInfo& Info ) override;

    /// @throws css::uno::RuntimeException
    css::uno::Sequence< css::ucb::ContentInfo >
        queryCreatableContentsInfo( const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );

    virtual std::vector< css::uno::Reference< css::ucb::XContent > > getChildren( ) override;

    /// @throws css::uno::RuntimeException
    /// @throws css::ucb::CommandFailedException
    /// @throws libcmis::Exception
    libcmis::ObjectPtr const & getObject( const css::uno::Reference< css::ucb::XCommandEnvironment >& xEnv );
};

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
