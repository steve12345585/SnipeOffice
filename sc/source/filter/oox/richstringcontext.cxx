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

#include <richstringcontext.hxx>

#include <stylesfragment.hxx>
#include <oox/token/namespaces.hxx>

namespace oox::xls {

using ::oox::core::ContextHandlerRef;

ContextHandlerRef RichStringContext::onCreateContext( sal_Int32 nElement, const AttributeList& rAttribs )
{
    if( isRootElement() )
    {
        switch( nElement )
        {
            case XLS_TOKEN( t ):
                mnPortionIdx = mxString->importText(rAttribs);
                return this;    // collect text in onCharacters()
            case XLS_TOKEN( r ):
                mnPortionIdx = mxString->importRun();
                return this;
            case XLS_TOKEN( rPh ):
                mxPhonetic = mxString->importPhoneticRun( rAttribs );
                return this;
            case XLS_TOKEN( phoneticPr ):
                mxString->importPhoneticPr( rAttribs, *this );
            break;
        }
    }
    else switch( getCurrentElement() )
    {
        case XLS_TOKEN( r ):
            switch( nElement )
            {
                case XLS_TOKEN( rPr ):
                    if( mnPortionIdx != -1 )
                        return new FontContext( *this, mxString->getPortion(mnPortionIdx).createFont(*this) );
                break;

                case XLS_TOKEN( t ):
                    mxString->setAttributes(rAttribs);
                    return this;    // collect portion text in onCharacters()
            }
        break;

        case XLS_TOKEN( rPh ):
            switch( nElement )
            {
                case XLS_TOKEN( t ):
                    mxString->setAttributes(rAttribs);
                    return this;    // collect phonetic text in onCharacters()
            }
        break;
    }
    return nullptr;
}

void RichStringContext::onCharacters( const OUString& rChars )
{
    if( isCurrentElement( XLS_TOKEN( t ) ) ) switch( getParentElement() )
    {
        case XLS_TOKEN( rPh ):
            if( mxPhonetic )
                mxPhonetic->setText( rChars );
        break;
        default:
            if( mnPortionIdx != -1 )
                mxString->getPortion(mnPortionIdx).setText( rChars );
    }
}

} // namespace oox::xls

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
