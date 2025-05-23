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

#ifndef INCLUDED_JVMACCESS_CLASSPATH_HXX
#define INCLUDED_JVMACCESS_CLASSPATH_HXX

#include <sal/config.h>

#if defined __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-attributes"
#endif
#include <jni.h>
#if defined __clang__
#pragma clang diagnostic pop
#endif

#include <jvmaccess/jvmaccessdllapi.h>

#include <string_view>

namespace com::sun::star::uno
{
class XComponentContext;
}

namespace com::sun::star::uno
{
template <typename> class Reference;
}

namespace jvmaccess
{
/**
   Helper functions for class path handling.
 */
namespace ClassPath
{
/**
   translates a class path into a java.net.URL[] instance.

   @param context
   a component context; must not be null.

   @param environment
   a JNI environment; must not be null.

   @param classPath
   a list of zero or more internal (see the
   com.sun.star.uri.ExternalUriReferenceTranslator service) URI references,
   where any space characters (U+0020) are ignored (and, in particular, separate
   adjacent URI references).  Any vnd.sun.star.expand URL references in the list
   are expanded using the com.sun.star.util.theMacroExpander singleton of the
   given context.

   @returns
   a local reference to a java.net.URL[] instance containing the external (see
   the com.sun.star.uri.ExternalUriReferenceTranslator service) equivalents of
   all the URI references in the given classPath.  If null, a (still pending)
   JNI exception occurred.

   @throws com::sun::star::uno::RuntimeException
 */
JVMACCESS_DLLPUBLIC jobjectArray
translateToUrls(css::uno::Reference<css::uno::XComponentContext> const& context,
                JNIEnv* environment, std::u16string_view classPath);
}
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
