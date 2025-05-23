/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is Part of the SnipeOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/*
 This file has been autogenerated by update_pch.sh. It is possible to edit it
 manually (such as when an include file has been moved/renamed/removed). All such
 manual changes will be rewritten by the next run of update_pch.sh (which presumably
 also fixes all possible problems, so it's usually better to use it).

 Generated on 2021-04-11 19:48:19 using:
 ./bin/update_pch sot sot --cutoff=5 --exclude:system --exclude:module --include:local

 If after updating build fails, use the following command to locate conflicting headers:
 ./bin/update_pch_bisect ./sot/inc/pch/precompiled_sot.hxx "make sot.build" --find-conflicts
*/

#include <sal/config.h>
#if PCH_LEVEL >= 1
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <ostream>
#include <string_view>
#include <type_traits>
#include <utility>
#endif // PCH_LEVEL >= 1
#if PCH_LEVEL >= 2
#include <osl/endian.h>
#include <osl/file.hxx>
#include <osl/mutex.hxx>
#include <rtl/alloc.h>
#include <rtl/ref.hxx>
#include <rtl/string.hxx>
#include <rtl/stringconcat.hxx>
#include <rtl/stringutils.hxx>
#include <rtl/textenc.h>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.h>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <sal/types.h>
#include <vcl/dllapi.h>
#include <comphelper/errcode.hxx>
#endif // PCH_LEVEL >= 2
#if PCH_LEVEL >= 3
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/Type.h>
#include <o3tl/typed_flags_set.hxx>
#include <tools/ref.hxx>
#include <tools/toolsdllapi.h>
#include <unotools/unotoolsdllapi.h>
#endif // PCH_LEVEL >= 3
#if PCH_LEVEL >= 4
#include <sot/exchange.hxx>
#include <sot/stg.hxx>
#include <sot/storinfo.hxx>
#endif // PCH_LEVEL >= 4

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
