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

 Generated on 2022-08-13 18:01:14 using:
 ./bin/update_pch sc sc --cutoff=12 --exclude:system --include:module --include:local

 If after updating build fails, use the following command to locate conflicting headers:
 ./bin/update_pch_bisect ./sc/inc/pch/precompiled_sc.hxx "make sc.build" --find-conflicts
*/

#include <sal/config.h>
#if PCH_LEVEL >= 1
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <float.h>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iterator>
#include <limits.h>
#include <limits>
#include <map>
#include <math.h>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <ostream>
#include <set>
#include <span>
#include <sstream>
#include <stddef.h>
#include <stdexcept>
#include <string.h>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <boost/functional/hash.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ptree_fwd.hpp>
#endif // PCH_LEVEL >= 1
#if PCH_LEVEL >= 2
#include <osl/conditn.hxx>
#include <osl/diagnose.h>
#include <osl/diagnose.hxx>
#include <osl/doublecheckedlocking.h>
#include <osl/endian.h>
#include <osl/file.hxx>
#include <osl/getglobalmutex.hxx>
#include <osl/interlck.h>
#include <osl/module.hxx>
#include <osl/mutex.h>
#include <osl/mutex.hxx>
#include <osl/process.h>
#include <osl/security.hxx>
#include <osl/thread.h>
#include <rtl/alloc.h>
#include <rtl/bootstrap.hxx>
#include <rtl/character.hxx>
#include <rtl/crc.h>
#include <rtl/digest.h>
#include <rtl/instance.hxx>
#include <rtl/math.h>
#include <rtl/math.hxx>
#include <rtl/ref.hxx>
#include <rtl/strbuf.h>
#include <rtl/strbuf.hxx>
#include <rtl/string.h>
#include <rtl/string.hxx>
#include <rtl/stringconcat.hxx>
#include <rtl/stringutils.hxx>
#include <rtl/tencinfo.h>
#include <rtl/textcvt.h>
#include <rtl/textenc.h>
#include <rtl/ustrbuf.h>
#include <rtl/ustrbuf.hxx>
#include <rtl/ustring.h>
#include <rtl/ustring.hxx>
#include <sal/backtrace.hxx>
#include <sal/detail/log.h>
#include <sal/log.hxx>
#include <sal/macros.h>
#include <sal/mathconf.h>
#include <sal/saldllapi.h>
#include <sal/types.h>
#include <sal/typesizes.h>
#include <vcl/BinaryDataContainer.hxx>
#include <vcl/EnumContext.hxx>
#include <vcl/GraphicAttributes.hxx>
#include <vcl/GraphicExternalLink.hxx>
#include <vcl/GraphicObject.hxx>
#include <vcl/IDialogRenderable.hxx>
#include <vcl/InterimItemWindow.hxx>
#include <vcl/Scanline.hxx>
#include <vcl/WindowPosSize.hxx>
#include <vcl/alpha.hxx>
#include <vcl/animate/Animation.hxx>
#include <vcl/animate/AnimationFrame.hxx>
#include <vcl/bitmap.hxx>
#include <vcl/bitmap/BitmapTypes.hxx>
#include <vcl/bitmapex.hxx>
#include <vcl/checksum.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/ctrl.hxx>
#include <vcl/customweld.hxx>
#include <vcl/dllapi.h>
#include <comphelper/errcode.hxx>
#include <vcl/event.hxx>
#include <vcl/fntstyle.hxx>
#include <vcl/font.hxx>
#include <vcl/gfxlink.hxx>
#include <vcl/graph.hxx>
#include <vcl/idle.hxx>
#include <vcl/image.hxx>
#include <vcl/keycod.hxx>
#include <vcl/keycodes.hxx>
#include <vcl/mapmod.hxx>
#include <vcl/outdev.hxx>
#include <vcl/region.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <vcl/syswin.hxx>
#include <vcl/task.hxx>
#include <vcl/timer.hxx>
#include <vcl/toolboxid.hxx>
#include <vcl/transfer.hxx>
#include <vcl/uitest/factory.hxx>
#include <vcl/vclenum.hxx>
#include <vcl/vclptr.hxx>
#include <vcl/vectorgraphicdata.hxx>
#include <vcl/virdev.hxx>
#include <vcl/weld.hxx>
#include <vcl/window.hxx>
#include <vcl/windowstate.hxx>
#include <vcl/wintypes.hxx>
#endif // PCH_LEVEL >= 2
#if PCH_LEVEL >= 3
#include <basegfx/basegfxdllapi.h>
#include <basegfx/color/bcolor.hxx>
#include <basegfx/matrix/b2dhommatrix.hxx>
#include <basegfx/numeric/ftools.hxx>
#include <basegfx/point/b2dpoint.hxx>
#include <basegfx/point/b2ipoint.hxx>
#include <basegfx/polygon/b2dpolygon.hxx>
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <basegfx/range/Range2D.hxx>
#include <basegfx/range/b2drange.hxx>
#include <basegfx/range/b2drectangle.hxx>
#include <basegfx/range/basicrange.hxx>
#include <basegfx/tuple/Tuple2D.hxx>
#include <basegfx/tuple/Tuple3D.hxx>
#include <basegfx/tuple/b2dtuple.hxx>
#include <basegfx/tuple/b2ituple.hxx>
#include <basegfx/tuple/b3dtuple.hxx>
#include <basegfx/utils/common.hxx>
#include <basegfx/vector/b2dsize.hxx>
#include <basegfx/vector/b2dvector.hxx>
#include <basegfx/vector/b2enums.hxx>
#include <basegfx/vector/b2ivector.hxx>
#include <basic/basicdllapi.h>
#include <basic/sbxdef.hxx>
#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/XAccessible.hpp>
#include <com/sun/star/accessibility/XAccessibleComponent.hpp>
#include <com/sun/star/accessibility/XAccessibleContext.hpp>
#include <com/sun/star/accessibility/XAccessibleEventBroadcaster.hpp>
#include <com/sun/star/accessibility/XAccessibleSelection.hpp>
#include <com/sun/star/awt/Gradient.hpp>
#include <com/sun/star/awt/GradientStyle.hpp>
#include <com/sun/star/awt/Key.hpp>
#include <com/sun/star/awt/KeyGroup.hpp>
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/PropertyState.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XFastPropertySet.hpp>
#include <com/sun/star/beans/XMultiPropertySet.hpp>
#include <com/sun/star/beans/XPropertiesChangeListener.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/beans/XPropertySetInfo.hpp>
#include <com/sun/star/beans/XPropertySetOption.hpp>
#include <com/sun/star/beans/XPropertyState.hpp>
#include <com/sun/star/beans/XVetoableChangeListener.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/container/XNameContainer.hpp>
#include <com/sun/star/datatransfer/DataFlavor.hpp>
#include <com/sun/star/datatransfer/XTransferable2.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardOwner.hpp>
#include <com/sun/star/datatransfer/dnd/DNDConstants.hpp>
#include <com/sun/star/datatransfer/dnd/DropTargetDragEvent.hpp>
#include <com/sun/star/datatransfer/dnd/DropTargetDropEvent.hpp>
#include <com/sun/star/datatransfer/dnd/XDragGestureListener.hpp>
#include <com/sun/star/datatransfer/dnd/XDragSourceListener.hpp>
#include <com/sun/star/datatransfer/dnd/XDropTargetListener.hpp>
#include <com/sun/star/drawing/DashStyle.hpp>
#include <com/sun/star/drawing/HatchStyle.hpp>
#include <com/sun/star/drawing/TextFitToSizeType.hpp>
#include <com/sun/star/embed/Aspects.hpp>
#include <com/sun/star/embed/XEmbeddedObject.hpp>
#include <com/sun/star/embed/XStorage.hpp>
#include <com/sun/star/frame/XFrame.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XStatusListener.hpp>
#include <com/sun/star/frame/XTerminateListener.hpp>
#include <com/sun/star/frame/XToolbarController.hpp>
#include <com/sun/star/graphic/XPrimitive2D.hpp>
#include <com/sun/star/i18n/ForbiddenCharacters.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/lang/EventObject.hpp>
#include <com/sun/star/lang/IndexOutOfBoundsException.hpp>
#include <com/sun/star/lang/Locale.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XTypeProvider.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/sheet/DataPilotFieldOrientation.hpp>
#include <com/sun/star/style/NumberingType.hpp>
#include <com/sun/star/style/XStyle.hpp>
#include <com/sun/star/uno/Any.h>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.h>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/uno/Sequence.h>
#include <com/sun/star/uno/Sequence.hxx>
#include <com/sun/star/uno/Type.h>
#include <com/sun/star/uno/Type.hxx>
#include <com/sun/star/uno/TypeClass.hdl>
#include <com/sun/star/uno/XAggregation.hpp>
#include <com/sun/star/uno/XInterface.hpp>
#include <com/sun/star/uno/XWeak.hpp>
#include <com/sun/star/uno/genfunc.h>
#include <com/sun/star/uno/genfunc.hxx>
#include <com/sun/star/util/Date.hpp>
#include <com/sun/star/util/DateTime.hpp>
#include <com/sun/star/util/Time.hpp>
#include <com/sun/star/util/XAccounting.hpp>
#include <com/sun/star/util/XUpdatable.hpp>
#include <com/sun/star/xml/sax/XFastContextHandler.hpp>
#include <comphelper/broadcasthelper.hxx>
#include <comphelper/compbase.hxx>
#include <comphelper/comphelperdllapi.h>
#include <comphelper/interfacecontainer2.hxx>
#include <comphelper/interfacecontainer4.hxx>
#include <comphelper/lok.hxx>
#include <comphelper/multicontainer2.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/propagg.hxx>
#include <comphelper/proparrhlp.hxx>
#include <comphelper/propertycontainer.hxx>
#include <comphelper/propertycontainerhelper.hxx>
#include <comphelper/propertysequence.hxx>
#include <comphelper/propertysetinfo.hxx>
#include <comphelper/propstate.hxx>
#include <comphelper/sequence.hxx>
#include <comphelper/servicehelper.hxx>
#include <comphelper/string.hxx>
#include <comphelper/uno3.hxx>
#include <cppu/cppudllapi.h>
#include <cppu/unotype.hxx>
#include <cppuhelper/basemutex.hxx>
#include <cppuhelper/compbase5.hxx>
#include <cppuhelper/compbase_ex.hxx>
#include <cppuhelper/cppuhelperdllapi.h>
#include <cppuhelper/implbase.hxx>
#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/implbase5.hxx>
#include <cppuhelper/implbase_ex.hxx>
#include <cppuhelper/implbase_ex_post.hxx>
#include <cppuhelper/implbase_ex_pre.hxx>
#include <cppuhelper/interfacecontainer.h>
#include <cppuhelper/propshlp.hxx>
#include <cppuhelper/queryinterface.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>
#include <cppuhelper/weakagg.hxx>
#include <cppuhelper/weakref.hxx>
#include <drawinglayer/drawinglayerdllapi.h>
#include <drawinglayer/geometry/viewinformation2d.hxx>
#include <drawinglayer/primitive2d/CommonTypes.hxx>
#include <drawinglayer/primitive2d/Primitive2DContainer.hxx>
#include <drawinglayer/primitive2d/Primitive2DVisitor.hxx>
#include <drawinglayer/primitive2d/baseprimitive2d.hxx>
#include <editeng/adjustitem.hxx>
#include <editeng/borderline.hxx>
#include <editeng/boxitem.hxx>
#include <editeng/brushitem.hxx>
#include <editeng/colritem.hxx>
#include <editeng/editdata.hxx>
#include <editeng/editeng.hxx>
#include <editeng/editengdllapi.h>
#include <editeng/editobj.hxx>
#include <editeng/editstat.hxx>
#include <editeng/editview.hxx>
#include <editeng/eedata.hxx>
#include <editeng/eeitem.hxx>
#include <editeng/fhgtitem.hxx>
#include <editeng/flditem.hxx>
#include <editeng/fontitem.hxx>
#include <editeng/forbiddencharacterstable.hxx>
#include <editeng/justifyitem.hxx>
#include <editeng/langitem.hxx>
#include <editeng/lineitem.hxx>
#include <editeng/outliner.hxx>
#include <editeng/outlobj.hxx>
#include <editeng/overflowingtxt.hxx>
#include <editeng/paragraphdata.hxx>
#include <editeng/postitem.hxx>
#include <editeng/svxenum.hxx>
#include <editeng/svxfont.hxx>
#include <editeng/wghtitem.hxx>
#include <i18nlangtag/lang.h>
#include <o3tl/cow_wrapper.hxx>
#include <o3tl/deleter.hxx>
#include <o3tl/enumarray.hxx>
#include <o3tl/safeint.hxx>
#include <o3tl/sorted_vector.hxx>
#include <o3tl/string_view.hxx>
#include <o3tl/strong_int.hxx>
#include <o3tl/typed_flags_set.hxx>
#include <o3tl/underlyingenumvalue.hxx>
#include <o3tl/unit_conversion.hxx>
#include <officecfg/Office/Common.hxx>
#include <salhelper/salhelperdllapi.h>
#include <salhelper/simplereferenceobject.hxx>
#include <salhelper/thread.hxx>
#include <sax/tools/converter.hxx>
#include <sfx2/app.hxx>
#include <sfx2/basedlgs.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/chalign.hxx>
#include <sfx2/childwin.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/dllapi.h>
#include <sfx2/docfile.hxx>
#include <sfx2/linkmgr.hxx>
#include <sfx2/lokhelper.hxx>
#include <sfx2/objface.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/printer.hxx>
#include <sfx2/request.hxx>
#include <sfx2/shell.hxx>
#include <sfx2/viewfrm.hxx>
#include <sot/exchange.hxx>
#include <sot/formats.hxx>
#include <sot/sotdllapi.h>
#include <svl/SfxBroadcaster.hxx>
#include <svl/cenumitm.hxx>
#include <svl/eitem.hxx>
#include <svl/hint.hxx>
#include <svl/intitem.hxx>
#include <svl/itempool.hxx>
#include <svl/itemprop.hxx>
#include <svl/itemset.hxx>
#include <svl/languageoptions.hxx>
#include <svl/lstner.hxx>
#include <svl/numformat.hxx>
#include <svl/poolitem.hxx>
#include <svl/sharedstring.hxx>
#include <svl/sharedstringpool.hxx>
#include <svl/stritem.hxx>
#include <svl/style.hxx>
#include <svl/stylesheetuser.hxx>
#include <svl/svldllapi.h>
#include <svl/typedwhich.hxx>
#include <svl/undo.hxx>
#include <svl/whichranges.hxx>
#include <svl/whiter.hxx>
#include <svl/zforlist.hxx>
#include <svl/zformat.hxx>
#include <svtools/colorcfg.hxx>
#include <svtools/svtdllapi.h>
#include <svtools/toolboxcontroller.hxx>
#include <svx/XPropertyEntry.hxx>
#include <svx/algitem.hxx>
#include <svx/itextprovider.hxx>
#include <svx/sdr/animation/scheduler.hxx>
#include <svx/sdr/overlay/overlayobject.hxx>
#include <svx/sdr/overlay/overlayobjectlist.hxx>
#include <svx/sdr/properties/defaultproperties.hxx>
#include <svx/sdr/properties/properties.hxx>
#include <svx/sdrobjectuser.hxx>
#include <svx/sdtaditm.hxx>
#include <svx/sdtaitm.hxx>
#include <svx/sdtakitm.hxx>
#include <svx/svddef.hxx>
#include <svx/svddrag.hxx>
#include <svx/svdglue.hxx>
#include <svx/svdhdl.hxx>
#include <svx/svdhlpln.hxx>
#include <svx/svditer.hxx>
#include <svx/svdlayer.hxx>
#include <svx/svdmark.hxx>
#include <svx/svdmodel.hxx>
#include <svx/svdmrkv.hxx>
#include <svx/svdoattr.hxx>
#include <svx/svdobj.hxx>
#include <svx/svdocapt.hxx>
#include <svx/svdoedge.hxx>
#include <svx/svdoole2.hxx>
#include <svx/svdotext.hxx>
#include <svx/svdouno.hxx>
#include <svx/svdpage.hxx>
#include <svx/svdpagv.hxx>
#include <svx/svdpntv.hxx>
#include <svx/svdsnpv.hxx>
#include <svx/svdsob.hxx>
#include <svx/svdtext.hxx>
#include <svx/svdtrans.hxx>
#include <svx/svdtypes.hxx>
#include <svx/svdundo.hxx>
#include <svx/svxdlg.hxx>
#include <svx/svxdllapi.h>
#include <svx/xdash.hxx>
#include <svx/xdef.hxx>
#include <svx/xhatch.hxx>
#include <svx/xit.hxx>
#include <svx/xpoly.hxx>
#include <svx/xtable.hxx>
#include <tools/color.hxx>
#include <tools/date.hxx>
#include <tools/datetime.hxx>
#include <tools/degree.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <tools/fldunit.hxx>
#include <tools/fontenum.hxx>
#include <tools/fract.hxx>
#include <tools/gen.hxx>
#include <tools/globname.hxx>
#include <tools/helpers.hxx>
#include <tools/lineend.hxx>
#include <tools/link.hxx>
#include <tools/long.hxx>
#include <tools/mapunit.hxx>
#include <tools/poly.hxx>
#include <tools/ref.hxx>
#include <tools/solar.h>
#include <tools/stream.hxx>
#include <tools/time.hxx>
#include <tools/toolsdllapi.h>
#include <tools/urlobj.hxx>
#include <tools/weakbase.h>
#include <tools/weakbase.hxx>
#include <typelib/typeclass.h>
#include <typelib/typedescription.h>
#include <typelib/uik.h>
#include <uno/any2.h>
#include <uno/data.h>
#include <uno/sequence2.h>
#include <unotools/charclass.hxx>
#include <unotools/collatorwrapper.hxx>
#include <unotools/configitem.hxx>
#include <unotools/configmgr.hxx>
#include <unotools/options.hxx>
#include <unotools/resmgr.hxx>
#include <unotools/syslocale.hxx>
#include <unotools/transliterationwrapper.hxx>
#include <unotools/unotoolsdllapi.h>
#include <xmloff/dllapi.h>
#include <xmloff/namespacemap.hxx>
#include <xmloff/xmlictxt.hxx>
#include <xmloff/xmlnamespace.hxx>
#include <xmloff/xmltoken.hxx>
#endif // PCH_LEVEL >= 3
#if PCH_LEVEL >= 4
#include <AccessibleContextBase.hxx>
#include <IAnyRefDialog.hxx>
#include <Sparkline.hxx>
#include <SparklineGroup.hxx>
#include <TableFillingAndNavigationTools.hxx>
#include <address.hxx>
#include <anyrefdg.hxx>
#include <appoptio.hxx>
#include <attrib.hxx>
#include <brdcst.hxx>
#include <calcmacros.hxx>
#include <cellform.hxx>
#include <cellsuno.hxx>
#include <cellvalue.hxx>
#include <chartlis.hxx>
#include <chgtrack.hxx>
#include <clipparam.hxx>
#include <colorscale.hxx>
#include <column.hxx>
#include <columnspanset.hxx>
#include <compiler.hxx>
#include <conditio.hxx>
#include <convuno.hxx>
#include <datamapper.hxx>
#include <dbdata.hxx>
#include <dbdocfun.hxx>
#include <dbfunc.hxx>
#include <docfunc.hxx>
#include <dociter.hxx>
#include <docoptio.hxx>
#include <docpool.hxx>
#include <docsh.hxx>
#include <document.hxx>
#include <documentlinkmgr.hxx>
#include <docuno.hxx>
#include <dpobject.hxx>
#include <dpsave.hxx>
#include <dpshttab.hxx>
#include <dputil.hxx>
#include <drawview.hxx>
#include <drwlayer.hxx>
#include <editable.hxx>
#include <editsrc.hxx>
#include <editutil.hxx>
#include <externalrefmgr.hxx>
#include <fillinfo.hxx>
#include <formula/IControlReferenceHandler.hxx>
#include <formula/compiler.hxx>
#include <formula/errorcodes.hxx>
#include <formula/formuladllapi.h>
#include <formula/funcutl.hxx>
#include <formula/opcode.hxx>
#include <formula/token.hxx>
#include <formula/vectortoken.hxx>
#include <formulacell.hxx>
#include <funcdesc.hxx>
#include <global.hxx>
#include <globalnames.hxx>
#include <gridwin.hxx>
#include <hints.hxx>
#include <inputhdl.hxx>
#include <inputopt.hxx>
#include <inputwin.hxx>
#include <interpre.hxx>
#include <listenercontext.hxx>
#include <markdata.hxx>
#include <miscuno.hxx>
#include <mtvelements.hxx>
#include <olinetab.hxx>
#include <output.hxx>
#include <patattr.hxx>
#include <postit.hxx>
#include <prevwsh.hxx>
#include <printfun.hxx>
#include <queryentry.hxx>
#include <queryparam.hxx>
#include <rangelst.hxx>
#include <rangenam.hxx>
#include <rangeutl.hxx>
#include <refdata.hxx>
#include <reffact.hxx>
#include <refundo.hxx>
#include <refupdat.hxx>
#include <refupdatecontext.hxx>
#include <rowheightcontext.hxx>
#include <scabstdlg.hxx>
#include <scdllapi.h>
#include <scitems.hxx>
#include <scmatrix.hxx>
#include <scmod.hxx>
#include <scopetools.hxx>
#include <scresid.hxx>
#include <segmenttree.hxx>
#include <sheetdata.hxx>
#include <sheetevents.hxx>
#include <shellids.hxx>
#include <stlpool.hxx>
#include <stlsheet.hxx>
#include <stringutil.hxx>
#include <table.hxx>
#include <tablink.hxx>
#include <tabprotection.hxx>
#include <tabvwsh.hxx>
#include <tokenarray.hxx>
#include <tokenstringcontext.hxx>
#include <transobj.hxx>
#include <types.hxx>
#include <uiitems.hxx>
#include <undobase.hxx>
#include <undoblk.hxx>
#include <unonames.hxx>
#include <userdat.hxx>
#include <validat.hxx>
#include <viewdata.hxx>
#include <xlconst.hxx>
#include <xlroot.hxx>
#endif // PCH_LEVEL >= 4

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
