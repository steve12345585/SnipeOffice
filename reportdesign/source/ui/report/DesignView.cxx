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

#include <DesignView.hxx>
#include <ReportController.hxx>
#include <svtools/acceleratorexecute.hxx>
#include <unotools/viewoptions.hxx>
#include <RptDef.hxx>
#include <UITools.hxx>
#include <RptObject.hxx>
#include <propbrw.hxx>
#include <helpids.h>
#include <SectionView.hxx>
#include <ReportSection.hxx>
#include <rptui_slotid.hrc>
#include <AddField.hxx>
#include <ScrollHelper.hxx>
#include <Navigator.hxx>
#include <SectionWindow.hxx>

#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>

#include <algorithm>

namespace rptui
{
using namespace ::dbaui;
using namespace ::com::sun::star;
using namespace uno;
using namespace beans;

#define START_SIZE_TASKPANE 30
#define COLSET_ID           1
#define REPORT_ID           2
#define TASKPANE_ID         3

namespace {

class OTaskWindow : public vcl::Window
{
    VclPtr<PropBrw> m_pPropWin;
public:
    explicit OTaskWindow(vcl::Window* _pParent) : Window(_pParent),m_pPropWin(nullptr){}
    virtual ~OTaskWindow() override { disposeOnce(); }
    virtual void dispose() override { m_pPropWin.clear(); vcl::Window::dispose(); }

    void setPropertyBrowser(PropBrw* _pPropWin)
    {
        m_pPropWin = _pPropWin;
    }

    virtual void Resize() override
    {
        const Size aSize = GetOutputSizePixel();
        if ( m_pPropWin && aSize.Height() && aSize.Width() )
            m_pPropWin->SetSizePixel(aSize);
    }
};

}



ODesignView::ODesignView(   vcl::Window* pParent,
                            const Reference< XComponentContext >& _rxOrb,
                            OReportController& _rController) :
    ODataView( pParent, _rController, _rxOrb, WB_DIALOGCONTROL )
    ,m_aSplitWin(VclPtr<SplitWindow>::Create(this))
    ,m_rReportController( _rController )
    ,m_aScrollWindow(VclPtr<rptui::OScrollWindowHelper>::Create(this))
    ,m_pPropWin(nullptr)
    ,m_pCurrentView(nullptr)
    ,m_aMarkIdle("reportdesign ODesignView Mark Idle")
    ,m_eMode( DlgEdMode::Select )
    ,m_eActObj( SdrObjKind::NONE )
    ,m_aGridSizeCoarse( 1000, 1000 )    // #i93595# 100TH_MM changed to grid using coarse 1 cm grid
    ,m_aGridSizeFine( 250, 250 )        // and a 0,25 cm subdivision for better visualisation
    ,m_bDeleted( false )
{
    SetHelpId(u"" UID_RPT_RPT_APP_VIEW ""_ustr);
    ImplInitSettings();

    SetMapMode( MapMode( MapUnit::Map100thMM ) );

    // now create the task pane on the right side :-)
    m_pTaskPane = VclPtr<OTaskWindow>::Create(this);

    m_aSplitWin->InsertItem( COLSET_ID,100,SPLITWINDOW_APPEND, 0, SplitWindowItemFlags::PercentSize | SplitWindowItemFlags::ColSet );
    m_aSplitWin->InsertItem( REPORT_ID, m_aScrollWindow.get(), 100, SPLITWINDOW_APPEND, COLSET_ID, SplitWindowItemFlags::PercentSize);

    // set up splitter
    m_aSplitWin->SetSplitHdl(LINK(this, ODesignView,SplitHdl));
    m_aSplitWin->SetAlign(WindowAlign::Left);
    m_aSplitWin->Show();

    m_aMarkIdle.SetInvokeHandler( LINK( this, ODesignView, MarkTimeout ) );
}


ODesignView::~ODesignView()
{
    disposeOnce();
}

void ODesignView::dispose()
{
    m_bDeleted = true;
    Hide();
    m_aScrollWindow->Hide();
    m_aMarkIdle.Stop();
    if ( m_pPropWin )
    {
        notifySystemWindow(this,m_pPropWin,::comphelper::mem_fun(&TaskPaneList::RemoveWindow));
        m_pPropWin.disposeAndClear();
    }
    if ( m_xAddField )
    {
        SvtViewOptions aDlgOpt( EViewType::Window, u"" UID_RPT_RPT_APP_VIEW ""_ustr );
        aDlgOpt.SetWindowState(m_xAddField->getDialog()->get_window_state(vcl::WindowDataMask::All));

        if (m_xAddField->getDialog()->get_visible())
            m_xAddField->response(RET_CANCEL);

        m_xAddField.reset();
    }
    if ( m_xReportExplorer )
    {
        SvtViewOptions aDlgOpt(EViewType::Window, m_xReportExplorer->get_help_id());
        aDlgOpt.SetWindowState(m_xReportExplorer->getDialog()->get_window_state(vcl::WindowDataMask::All));

        if (m_xReportExplorer->getDialog()->get_visible())
            m_xReportExplorer->response(RET_CANCEL);

        m_xReportExplorer.reset();
    }

    m_pTaskPane.disposeAndClear();
    m_aScrollWindow.disposeAndClear();
    m_aSplitWin.disposeAndClear();
    dbaui::ODataView::dispose();
}

void ODesignView::initialize()
{
    SetMapMode( MapMode( MapUnit::Map100thMM ) );
    m_aScrollWindow->initialize();
    m_aScrollWindow->Show();
}

void ODesignView::DataChanged( const DataChangedEvent& rDCEvt )
{
    ODataView::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DataChangedEventType::SETTINGS) &&
         (rDCEvt.GetFlags() & AllSettingsFlags::STYLE) )
    {
        ImplInitSettings();
        Invalidate();
    }
}

bool ODesignView::PreNotify( NotifyEvent& rNEvt )
{
    bool bRet = ODataView::PreNotify(rNEvt); // 1 := has to be handled here
    switch(rNEvt.GetType())
    {
        case NotifyEventType::KEYINPUT:
        {
            if ( m_pPropWin && m_pPropWin->HasChildPathFocus() )
                return false;
            if (m_xAddField && m_xAddField->getDialog()->has_toplevel_focus())
                return false;
            if ( m_xReportExplorer && m_xReportExplorer->getDialog()->has_toplevel_focus())
                return false;
            const KeyEvent* pKeyEvent = rNEvt.GetKeyEvent();
            if ( handleKeyEvent(*pKeyEvent) )
                bRet = true;
            else if ( bRet && m_pAccel )
            {
                const vcl::KeyCode& rCode = pKeyEvent->GetKeyCode();
                util::URL aUrl;
                aUrl.Complete = m_pAccel->findCommand(svt::AcceleratorExecute::st_VCLKey2AWTKey(rCode));
                if ( aUrl.Complete.isEmpty() || !m_xController->isCommandEnabled( aUrl.Complete ) )
                    bRet = false;
            }
            break;
        }
        default:
            break;
    }

    return bRet;
}

void ODesignView::resizeDocumentView(tools::Rectangle& _rPlayground)
{
    if ( !_rPlayground.IsEmpty() )
    {
        const Size aPlaygroundSize( _rPlayground.GetSize() );

        // calc the split pos, and forward it to the controller
        sal_Int32 nSplitPos = getController().getSplitPos();
        if ( 0 != aPlaygroundSize.Width() )
        {
            if  (   ( -1 == nSplitPos )
                ||  ( nSplitPos >= aPlaygroundSize.Width() )
                )
            {
                tools::Long nMinWidth = static_cast<tools::Long>(0.1*aPlaygroundSize.Width());
                if ( m_pPropWin && m_pPropWin->IsVisible() )
                    nMinWidth = m_pPropWin->GetMinOutputSizePixel().Width();
                nSplitPos = static_cast<sal_Int32>(_rPlayground.Right() - nMinWidth);
                getController().setSplitPos(nSplitPos);
            }
        }

        if ( m_aSplitWin->IsItemValid(TASKPANE_ID) )
        {
            // normalize the split pos
            const tools::Long nSplitterWidth = StyleSettings::GetSplitSize();
            Point aTaskPanePos(nSplitPos + nSplitterWidth, _rPlayground.Top());
            if (m_pTaskPane && m_pTaskPane->IsVisible() && m_pPropWin)
            {
                aTaskPanePos.setX( aPlaygroundSize.Width() - m_pTaskPane->GetSizePixel().Width() );
                sal_Int32 nMinWidth = m_pPropWin->getMinimumSize().Width();
                if ( nMinWidth > (aPlaygroundSize.Width() - aTaskPanePos.X()) )
                {
                    aTaskPanePos.setX( aPlaygroundSize.Width() - nMinWidth );
                }
                nSplitPos = aTaskPanePos.X() - nSplitterWidth;
                getController().setSplitPos(nSplitPos);

                if (const auto nWidth = aPlaygroundSize.Width())
                {
                    const tools::Long nTaskPaneSize = static_cast<tools::Long>((aPlaygroundSize.Width() - aTaskPanePos.X())*100/nWidth);
                    if ( m_aSplitWin->GetItemSize( TASKPANE_ID ) != nTaskPaneSize )
                    {
                        m_aSplitWin->SetItemSize( REPORT_ID, 99 - nTaskPaneSize );
                        m_aSplitWin->SetItemSize( TASKPANE_ID, nTaskPaneSize );
                    }
                }
            }
        }
        // set the size of the report window
        m_aSplitWin->SetPosSizePixel( _rPlayground.TopLeft(),aPlaygroundSize );
    }
        // just for completeness: there is no space left, we occupied it all ...
    _rPlayground.SetPos( _rPlayground.BottomRight() );
    _rPlayground.SetSize( Size( 0, 0 ) );

}

IMPL_LINK_NOARG(ODesignView, MarkTimeout, Timer *, void)
{
    if ( m_pPropWin && m_pPropWin->IsVisible() )
    {
        m_pPropWin->Update(m_pCurrentView);
        uno::Reference<beans::XPropertySet> xProp(m_xReportComponent,uno::UNO_QUERY);
        if ( xProp.is() )
        {
            m_pPropWin->Update(xProp);
            static_cast<OTaskWindow*>(m_pTaskPane.get())->Resize();
        }
        Resize();
    }
}


void ODesignView::SetMode( DlgEdMode _eNewMode )
{
    m_eMode = _eNewMode;
    if ( m_eMode == DlgEdMode::Select )
        m_eActObj = SdrObjKind::NONE;

    m_aScrollWindow->SetMode(_eNewMode);
}

void ODesignView::SetInsertObj( SdrObjKind eObj,const OUString& _sShapeType )
{
    m_eActObj = eObj;
    m_aScrollWindow->SetInsertObj( eObj,_sShapeType );
}

OUString const & ODesignView::GetInsertObjString() const
{
    return m_aScrollWindow->GetInsertObjString();
}


void ODesignView::Cut()
{
    Copy();
    Delete();
}


void ODesignView::Copy()
{
    m_aScrollWindow->Copy();
}


void ODesignView::Paste()
{
    m_aScrollWindow->Paste();
}

void ODesignView::Delete()
{
    m_aScrollWindow->Delete();
}

bool ODesignView::HasSelection() const
{
    return m_aScrollWindow->HasSelection();
}


bool ODesignView::IsPasteAllowed() const
{
    return m_aScrollWindow->IsPasteAllowed();
}


void ODesignView::UpdatePropertyBrowserDelayed(OSectionView& _rView)
{
    if ( m_pCurrentView != &_rView )
    {
        if ( m_pCurrentView )
            m_aScrollWindow->setMarked(m_pCurrentView,false);
        m_pCurrentView = &_rView;
        m_aScrollWindow->setMarked(m_pCurrentView, true);
        m_xReportComponent.clear();
        DlgEdHint aHint( RPTUI_HINT_SELECTIONCHANGED );
        Broadcast( aHint );
    }
    m_aMarkIdle.Start();
}


void ODesignView::toggleGrid(bool _bGridVisible)
{
     m_aScrollWindow->toggleGrid(_bGridVisible);
}

sal_uInt16 ODesignView::getSectionCount() const
{
    return m_aScrollWindow->getSectionCount();
}

void ODesignView::showRuler(bool _bShow)
{
     m_aScrollWindow->showRuler(_bShow);
}

void ODesignView::removeSection(sal_uInt16 _nPosition)
{
     m_aScrollWindow->removeSection(_nPosition);
}

void ODesignView::addSection(const uno::Reference< report::XSection >& _xSection,const OUString& _sColorEntry,sal_uInt16 _nPosition)
{
     m_aScrollWindow->addSection(_xSection,_sColorEntry,_nPosition);
}

void ODesignView::GetFocus()
{
    Window::GetFocus();

    if ( !m_bDeleted )
    {
        OSectionWindow* pSectionWindow = m_aScrollWindow->getMarkedSection();
        if ( pSectionWindow )
            pSectionWindow->GrabFocus();
    }
}

void ODesignView::ImplInitSettings()
{
    SetBackground( Wallpaper( Application::GetSettings().GetStyleSettings().GetFaceColor() ));
    GetOutDev()->SetFillColor( Application::GetSettings().GetStyleSettings().GetFaceColor() );
    SetTextFillColor( Application::GetSettings().GetStyleSettings().GetFaceColor() );
}

IMPL_LINK_NOARG( ODesignView, SplitHdl, SplitWindow*, void )
{
    const Size aOutputSize = GetOutputSizePixel();
    const tools::Long nTest = aOutputSize.Width() * m_aSplitWin->GetItemSize(TASKPANE_ID) / 100;
    tools::Long nMinWidth = static_cast<tools::Long>(0.1*aOutputSize.Width());
    if ( m_pPropWin && m_pPropWin->IsVisible() )
        nMinWidth = m_pPropWin->GetMinOutputSizePixel().Width();

    if ( (aOutputSize.Width() - nTest) >= nMinWidth && nTest > m_aScrollWindow->getMaxMarkerWidth() )
    {
        getController().setSplitPos(nTest);
    }
}

void ODesignView::SelectAll(const SdrObjKind _nObjectType)
{
     m_aScrollWindow->SelectAll(_nObjectType);
}

void ODesignView::unmarkAllObjects()
{
    m_aScrollWindow->unmarkAllObjects();
}

void ODesignView::togglePropertyBrowser(bool _bToggleOn)
{
    if ( !m_pPropWin && _bToggleOn )
    {
        m_pPropWin = VclPtr<PropBrw>::Create(getController().getORB(), m_pTaskPane,this);
        m_pPropWin->Invalidate();
        static_cast<OTaskWindow*>(m_pTaskPane.get())->setPropertyBrowser(m_pPropWin);
        notifySystemWindow(this,m_pPropWin,::comphelper::mem_fun(&TaskPaneList::AddWindow));
    }
    if ( !(m_pPropWin && _bToggleOn != m_pPropWin->IsVisible()) )
        return;

    if ( !m_pCurrentView && !m_xReportComponent.is() )
        m_xReportComponent = getController().getReportDefinition();

    const bool bWillBeVisible = _bToggleOn;
    m_pPropWin->Show(bWillBeVisible);
    m_pTaskPane->Show(bWillBeVisible);
    m_pTaskPane->Invalidate();

    if ( bWillBeVisible )
        m_aSplitWin->InsertItem( TASKPANE_ID, m_pTaskPane,START_SIZE_TASKPANE, SPLITWINDOW_APPEND, COLSET_ID, SplitWindowItemFlags::PercentSize);
    else
        m_aSplitWin->RemoveItem(TASKPANE_ID);

    if ( bWillBeVisible )
        m_aMarkIdle.Start();
}

void ODesignView::showProperties(const uno::Reference< uno::XInterface>& _xReportComponent)
{
    if ( m_xReportComponent != _xReportComponent )
    {
        m_xReportComponent = _xReportComponent;
        if ( m_pCurrentView )
            m_aScrollWindow->setMarked(m_pCurrentView,false);
        m_pCurrentView = nullptr;
        m_aMarkIdle.Start();
    }
}

bool ODesignView::isReportExplorerVisible() const
{
    return m_xReportExplorer && m_xReportExplorer->getDialog()->get_visible();
}

void ODesignView::toggleReportExplorer()
{
    if ( !m_xReportExplorer )
    {
        OReportController& rReportController = getController();
        m_xReportExplorer = std::make_shared<ONavigator>(GetFrameWeld(), rReportController);
        SvtViewOptions aDlgOpt(EViewType::Window, m_xReportExplorer->get_help_id());
        if ( aDlgOpt.Exists() )
            m_xReportExplorer->getDialog()->set_window_state(aDlgOpt.GetWindowState());
    }

    if (!m_xReportExplorer->getDialog()->get_visible())
        weld::DialogController::runAsync(m_xReportExplorer, [this](sal_Int32 /*nResult*/) { m_xReportExplorer.reset(); });
    else
        m_xReportExplorer->response(RET_CANCEL);
}

bool ODesignView::isAddFieldVisible() const
{
    return m_xAddField && m_xAddField->getDialog()->get_visible();
}

void ODesignView::toggleAddField()
{
    if (!m_xAddField)
    {
        uno::Reference< report::XReportDefinition > xReport(m_xReportComponent,uno::UNO_QUERY);
        uno::Reference< report::XReportComponent > xReportComponent(m_xReportComponent,uno::UNO_QUERY);
        OReportController& rReportController = getController();
        if ( !m_pCurrentView && !xReport.is() )
        {
            if ( xReportComponent.is() )
                xReport = xReportComponent->getSection()->getReportDefinition();
            else
                xReport = rReportController.getReportDefinition().get();
        }
        else if ( m_pCurrentView )
        {
            uno::Reference< report::XSection > xSection = m_pCurrentView->getReportSection()->getSection();
            xReport = xSection->getReportDefinition();
        }
        uno::Reference < beans::XPropertySet > xSet(rReportController.getRowSet(),uno::UNO_QUERY);
        m_xAddField = std::make_shared<OAddFieldWindow>(GetFrameWeld(), xSet);
        m_xAddField->SetCreateHdl(LINK( &rReportController, OReportController, OnCreateHdl ) );
        SvtViewOptions aDlgOpt( EViewType::Window, u"" UID_RPT_RPT_APP_VIEW ""_ustr );
        if ( aDlgOpt.Exists() )
            m_xAddField->getDialog()->set_window_state(aDlgOpt.GetWindowState());
        m_xAddField->Update();
    }
    if (!m_xAddField->getDialog()->get_visible())
        weld::DialogController::runAsync(m_xAddField, [this](sal_Int32 /*nResult*/) { m_xAddField.reset(); });
    else
        m_xAddField->response(RET_CANCEL);
}

uno::Reference< report::XSection > ODesignView::getCurrentSection() const
{
    uno::Reference< report::XSection > xSection;
    if ( m_pCurrentView )
        xSection = m_pCurrentView->getReportSection()->getSection();

    return xSection;
}

uno::Reference< report::XReportComponent > ODesignView::getCurrentControlModel() const
{
    uno::Reference< report::XReportComponent > xModel;
    if ( m_pCurrentView )
    {
        xModel = m_pCurrentView->getReportSection()->getCurrentControlModel();
    }
    return xModel;
}

OSectionWindow* ODesignView::getMarkedSection(NearSectionAccess nsa) const
{
    return  m_aScrollWindow->getMarkedSection(nsa);
}

OSectionWindow* ODesignView::getSectionWindow(const css::uno::Reference< css::report::XSection>& _xSection) const
{
    return  m_aScrollWindow->getSectionWindow(_xSection);
}

void ODesignView::markSection(const sal_uInt16 _nPos)
{
    m_aScrollWindow->markSection(_nPos);
}

void ODesignView::fillCollapsedSections(::std::vector<sal_uInt16>& _rCollapsedPositions) const
{
    m_aScrollWindow->fillCollapsedSections(_rCollapsedPositions);
}

void ODesignView::collapseSections(const uno::Sequence< beans::PropertyValue>& _aCollapsedSections)
{
    m_aScrollWindow->collapseSections(_aCollapsedSections);
}

OUString ODesignView::getCurrentPage() const
{
    return m_pPropWin ? m_pPropWin->getCurrentPage() : OUString();
}

void ODesignView::setCurrentPage(const OUString& _sLastActivePage)
{
    if ( m_pPropWin )
        m_pPropWin->setCurrentPage(_sLastActivePage);
}

void ODesignView::alignMarkedObjects(ControlModification _nControlModification,bool _bAlignAtSection)
{
    m_aScrollWindow->alignMarkedObjects(_nControlModification, _bAlignAtSection);
}

bool ODesignView::handleKeyEvent(const KeyEvent& _rEvent)
{
    if ( m_pPropWin && m_pPropWin->HasChildPathFocus() )
        return false;
    if (m_xAddField && m_xAddField->getDialog()->has_toplevel_focus())
        return false;
    if (m_xReportExplorer && m_xReportExplorer->getDialog()->has_toplevel_focus())
        return false;
    return m_aScrollWindow->handleKeyEvent(_rEvent);
}

void ODesignView::setMarked(const uno::Reference< report::XSection>& _xSection,bool _bMark)
{
    m_aScrollWindow->setMarked(_xSection,_bMark);
    if ( _bMark )
        UpdatePropertyBrowserDelayed(getMarkedSection()->getReportSection().getSectionView());
    else
        m_pCurrentView = nullptr;
}

void ODesignView::setMarked(const uno::Sequence< uno::Reference< report::XReportComponent> >& _aShapes,bool _bMark)
{
    m_aScrollWindow->setMarked(_aShapes,_bMark);
    if ( _aShapes.hasElements() && _bMark )
        showProperties(_aShapes[0]);
    else
        m_xReportComponent.clear();
}

void ODesignView::MouseButtonDown( const MouseEvent& rMEvt )
{
    if ( rMEvt.IsLeft() )
    {
        const uno::Sequence< beans::PropertyValue> aArgs;
        getController().executeChecked(SID_SELECT_REPORT,aArgs);
    }
    ODataView::MouseButtonDown(rMEvt);
}

uno::Any ODesignView::getCurrentlyShownProperty() const
{
    uno::Any aRet;
    OSectionWindow* pSectionWindow = getMarkedSection();
    if ( pSectionWindow )
    {
        ::std::vector< uno::Reference< uno::XInterface > > aSelection;
        pSectionWindow->getReportSection().fillControlModelSelection(aSelection);
        if ( !aSelection.empty() )
        {
            uno::Sequence< uno::Reference< report::XReportComponent > > aSeq(aSelection.size());
            std::transform(
                aSelection.begin(), aSelection.end(), aSeq.getArray(),
                [](const auto& rxInterface)
                { return uno::Reference<report::XReportComponent>(rxInterface, uno::UNO_QUERY); });
            aRet <<= aSeq;
        }
    }
    return aRet;
}

void ODesignView::fillControlModelSelection(::std::vector< uno::Reference< uno::XInterface > >& _rSelection) const
{
    m_aScrollWindow->fillControlModelSelection(_rSelection);
}

void ODesignView::setGridSnap(bool bOn)
{
    m_aScrollWindow->setGridSnap(bOn);

}

void ODesignView::setDragStripes(bool bOn)
{
    m_aScrollWindow->setDragStripes(bOn);
}

bool ODesignView::isHandleEvent() const
{
    return m_pPropWin && m_pPropWin->HasChildPathFocus();
}

sal_uInt32 ODesignView::getMarkedObjectCount() const
{
    return m_aScrollWindow->getMarkedObjectCount();
}

void ODesignView::zoom(const Fraction& _aZoom)
{
    m_aScrollWindow->zoom(_aZoom);
}

sal_uInt16 ODesignView::getZoomFactor(SvxZoomType _eType) const
{
    return m_aScrollWindow->getZoomFactor(_eType);
}

} // rptui


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
