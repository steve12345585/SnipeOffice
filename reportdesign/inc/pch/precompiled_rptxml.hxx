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

 Generated on 2021-03-08 13:14:10 using:
 ./bin/update_pch reportdesign rptxml --cutoff=2 --exclude:system --exclude:module --include:local

 If after updating build fails, use the following command to locate conflicting headers:
 ./bin/update_pch_bisect ./reportdesign/inc/pch/precompiled_rptxml.hxx "make reportdesign.build" --find-conflicts
*/

#include <sal/config.h>
#if PCH_LEVEL >= 1
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <new>
#include <string_view>
#include <type_traits>
#include <vector>
#endif // PCH_LEVEL >= 1
#if PCH_LEVEL >= 2
#include <osl/diagnose.h>
#include <osl/endian.h>
#include <rtl/math.hxx>
#include <rtl/ref.hxx>
#include <rtl/strbuf.h>
#include <rtl/strbuf.hxx>
#include <rtl/string.hxx>
#include <rtl/stringconcat.hxx>
#include <rtl/stringutils.hxx>
#include <rtl/textenc.h>
#include <rtl/ustrbuf.h>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.hxx>
#include <sal/log.hxx>
#include <sal/types.h>
#include <vcl/dllapi.h>
#include <vcl/svapp.hxx>
#endif // PCH_LEVEL >= 2
#if PCH_LEVEL >= 3
#include <basegfx/color/bcolor.hxx>
#include <com/sun/star/awt/FontDescriptor.hpp>
#include <com/sun/star/awt/ImageScaleMode.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/chart/XComplexDescriptionAccess.hpp>
#include <com/sun/star/chart2/data/XDatabaseDataProvider.hpp>
#include <com/sun/star/embed/ElementModes.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/reflection/ProxyFactory.hpp>
#include <com/sun/star/report/ForceNewPage.hpp>
#include <com/sun/star/report/GroupOn.hpp>
#include <com/sun/star/report/KeepTogether.hpp>
#include <com/sun/star/report/ReportPrintOption.hpp>
#include <com/sun/star/report/XFixedLine.hpp>
#include <com/sun/star/report/XFixedText.hpp>
#include <com/sun/star/report/XShape.hpp>
#include <com/sun/star/sdb/CommandType.hpp>
#include <com/sun/star/table/BorderLine2.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/util/MeasureUnit.hpp>
#include <comphelper/comphelperdllapi.h>
#include <comphelper/documentconstants.hxx>
#include <comphelper/genericpropertyset.hxx>
#include <comphelper/propertysetinfo.hxx>
#include <comphelper/sequenceashashmap.hxx>
#include <connectivity/dbtools.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <o3tl/typed_flags_set.hxx>
#include <salhelper/simplereferenceobject.hxx>
#include <tools/color.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/link.hxx>
#include <tools/toolsdllapi.h>
#include <unotools/options.hxx>
#include <unotools/saveopt.hxx>
#include <unotools/unotoolsdllapi.h>
#include <xmloff/ProgressBarHelper.hxx>
#include <xmloff/dllapi.h>
#include <xmloff/families.hxx>
#include <xmloff/maptype.hxx>
#include <xmloff/namespacemap.hxx>
#include <xmloff/prstylei.hxx>
#include <xmloff/txtimp.hxx>
#include <xmloff/xmlement.hxx>
#include <xmloff/xmlictxt.hxx>
#include <xmloff/xmlimppr.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmlstyle.hxx>
#include <xmloff/xmltkmap.hxx>
#include <xmloff/xmltoken.hxx>
#include <xmloff/xmluconv.hxx>
#endif // PCH_LEVEL >= 3
#if PCH_LEVEL >= 4
#include <RptDef.hxx>
#include <strings.hxx>
#endif // PCH_LEVEL >= 4

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
