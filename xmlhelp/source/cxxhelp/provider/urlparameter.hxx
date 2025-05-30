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

#include <rtl/ustring.hxx>
#include <rtl/string.hxx>
#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XOutputStream.hpp>

namespace chelp {


    class Databases;


    class DbtToStringConverter
    {
    public:

        explicit DbtToStringConverter( const char* ptr )
            : m_ptr( ptr )
        {
        }

        OUString getHash() const
        {
            if( m_ptr )
            {
                sal_Int32 sizeOfFile = static_cast<sal_Int32>(m_ptr[0]);
                OUString Hash( m_ptr+1,sizeOfFile,RTL_TEXTENCODING_UTF8 );
                sal_Int32 idx;
                if( ( idx = Hash.indexOf( u'#' ) ) != -1 )
                    return Hash.copy( 1+idx );
            }
            return OUString();
        }


        OUString getFile() const
        {
            if( ! m_ptr )
                return OUString();

            sal_Int32 sizeOfFile = static_cast<sal_Int32>(m_ptr[0]);
            OUString File( m_ptr+1,sizeOfFile,RTL_TEXTENCODING_UTF8 );
            sal_Int32 idx;
            if( ( idx = File.indexOf( u'#' ) ) != -1 )
                return File.copy( 0,idx );
            else
                return File;
        }


        OUString getDatabase() const
        {
            if( ! m_ptr )
                return OUString();

            sal_Int32 sizeOfDatabase = static_cast<int>(m_ptr[ 1+ static_cast<sal_Int32>(m_ptr[0]) ]);
            return OUString( m_ptr + 2 + static_cast<sal_Int32>(m_ptr[0]),sizeOfDatabase,RTL_TEXTENCODING_UTF8 );
        }


        OUString getTitle() const
        {
            if( ! m_ptr )
                return OUString();

            //fdo#82025 - use strlen instead of stored length byte to determine string len
            //There is a one byte length field at m_ptr[2 + m_ptr[0] +  m_ptr[1
            //+ m_ptr[0]]] but by default char is signed so anything larger
            //than 127 defaults to a negative value, casting it would allow up
            //to 255 but instead make use of the null termination to avoid
            //running into a later problem with strings >= 255
            const char* pTitle = m_ptr + 3 + m_ptr[0] +  static_cast<sal_Int32>(m_ptr[ 1+ static_cast<sal_Int32>(m_ptr[0]) ]);

            return OStringToOUString(pTitle, RTL_TEXTENCODING_UTF8);
        }


    private:

        const char* m_ptr;

    };


    class URLParameter
    {
    public:
        /// @throws css::ucb::IllegalIdentifierException
        URLParameter( const OUString& aURL,
                      Databases* pDatabases );

        bool isActive() const { return !m_aActive.isEmpty() && m_aActive == "true"; }
        bool isQuery() const { return m_aId.isEmpty() && !m_aQuery.isEmpty(); }
        bool isFile() const { return !m_aId.isEmpty(); }
        bool isModule() const { return m_aId.isEmpty() && !m_aModule.isEmpty(); }
        bool isRoot() const { return m_aModule.isEmpty(); }
        bool isErrorDocument();

        OUString const & get_id();

        const OUString & get_tag();

        //  Not called for a directory

        OUString const & get_path();

        const OUString& get_eid() const   { return m_aEid; }

        const OUString & get_title();

        OUString get_jar();

        const OUString& get_ExtensionRegistryPath() const { return m_aExtensionRegistryPath; }

        const OUString& get_module() const { return m_aModule; }

        OUString const & get_dbpar() const
        {
            if( !m_aDbPar.isEmpty() )
                return m_aDbPar;
            else
                return m_aModule;
        }

        OUString const & get_language() const;

        OUString const & get_program();

        const OUString& get_query() const { return m_aQuery; }

        const OUString& get_scope() const { return m_aScope; }

        const OUString& get_system() const { return m_aSystem; }

        sal_Int32     get_hitCount() const { return m_nHitCount; }

        OString getByName( const char* par );

        void open( const css::uno::Reference< css::io::XActiveDataSink >& xDataSink );

        void open( const css::uno::Reference< css::io::XOutputStream >& xDataSink );

    private:

        Databases* m_pDatabases;

        bool m_bHelpDataFileRead;
        bool m_bUseDB;

        OUString  m_aURL;

        OUString  m_aTag;
        OUString  m_aId;
        OUString  m_aPath;
        OUString  m_aModule;
        OUString  m_aTitle;
        OUString  m_aJar;
        OUString  m_aExtensionRegistryPath;
        OUString  m_aEid;
        OUString  m_aDbPar;

        OUString  m_aLanguage;

        OUString  m_aPrefix;
        OUString  m_aProgram;
        OUString  m_aSystem;
        OUString  m_aActive;

        OUString  m_aQuery;
        OUString  m_aScope;

        OUString m_aExpr;

        sal_Int32      m_nHitCount;                // The default maximum hitcount


        // private methods

        void init();

        const OUString & get_the_tag();

        const OUString & get_the_title();

        void readHelpDataFile();

        /// @throws css::ucb::IllegalIdentifierException
        void parse();

        bool scheme();

        bool module();

        bool name( bool modulePresent );

        bool query();

    };   // end class URLParameter


}  // end namespace chelp

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
