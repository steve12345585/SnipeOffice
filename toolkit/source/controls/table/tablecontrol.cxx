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

#include <controls/table/tablecontrol.hxx>

#include "tablecontrol_impl.hxx"
#include "tabledatawindow.hxx"

#include <com/sun/star/accessibility/AccessibleStateType.hpp>
#include <com/sun/star/accessibility/AccessibleEventId.hpp>

#include <sal/log.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <vcl/settings.hxx>
#include <vcl/vclevent.hxx>

using namespace ::com::sun::star::uno;
using ::com::sun::star::accessibility::XAccessible;
using namespace ::com::sun::star::accessibility;

namespace svt::table
{


    namespace AccessibleEventId = ::com::sun::star::accessibility::AccessibleEventId;


    //= TableControl


    TableControl::TableControl( vcl::Window* _pParent, WinBits _nStyle )
        :Control( _pParent, _nStyle )
        ,m_pImpl( std::make_shared<TableControl_Impl>( *this ) )
    {
        TableDataWindow& rDataWindow = m_pImpl->getDataWindow();
        rDataWindow.SetSelectHdl( LINK( this, TableControl, ImplSelectHdl ) );

        // by default, use the background as determined by the style settings
        const Color aWindowColor( GetSettings().GetStyleSettings().GetFieldColor() );
        SetBackground( Wallpaper( aWindowColor ) );
        GetOutDev()->SetFillColor( aWindowColor );

        SetCompoundControl( true );
    }


    TableControl::~TableControl()
    {
        disposeOnce();
    }

    void TableControl::dispose()
    {
        CallEventListeners( VclEventId::ObjectDying );

        m_pImpl->setModel( PTableModel() );
        m_pImpl->disposeAccessible();
        m_pImpl.reset();
        Control::dispose();
    }


    void TableControl::GetFocus()
    {
        if (m_pImpl)
            m_pImpl->showCursor();

        Control::GetFocus();
    }


    void TableControl::LoseFocus()
    {
        if (m_pImpl)
            m_pImpl->hideCursor();

        Control::LoseFocus();
    }


    void TableControl::KeyInput( const KeyEvent& rKEvt )
    {
        bool bHandled = false;
        if (m_pImpl)
        {
            const vcl::KeyCode& rKeyCode = rKEvt.GetKeyCode();
            sal_uInt16 nKeyCode = rKeyCode.GetCode();

            struct ActionMapEntry
            {
                sal_uInt16 nKeyCode;
                sal_uInt16 nKeyModifier;
                TableControlAction eAction;
            }
            static const aKnownActions[] = {
                      { KEY_DOWN,     0,          TableControlAction::cursorDown },
                      { KEY_UP,       0,          TableControlAction::cursorUp },
                      { KEY_LEFT,     0,          TableControlAction::cursorLeft },
                      { KEY_RIGHT,    0,          TableControlAction::cursorRight },
                      { KEY_HOME,     0,          TableControlAction::cursorToLineStart },
                      { KEY_END,      0,          TableControlAction::cursorToLineEnd },
                      { KEY_PAGEUP,   0,          TableControlAction::cursorPageUp },
                      { KEY_PAGEDOWN, 0,          TableControlAction::cursorPageDown },
                      { KEY_PAGEUP,   KEY_MOD1,   TableControlAction::cursorToFirstLine },
                      { KEY_PAGEDOWN, KEY_MOD1,   TableControlAction::cursorToLastLine },
                      { KEY_HOME,     KEY_MOD1,   TableControlAction::cursorTopLeft },
                      { KEY_END,      KEY_MOD1,   TableControlAction::cursorBottomRight },
                      { KEY_SPACE,    KEY_MOD1,   TableControlAction::cursorSelectRow },
                      { KEY_UP,       KEY_SHIFT,  TableControlAction::cursorSelectRowUp },
                      { KEY_DOWN,     KEY_SHIFT,  TableControlAction::cursorSelectRowDown },
                      { KEY_END,      KEY_SHIFT,  TableControlAction::cursorSelectRowAreaBottom },
                      { KEY_HOME,     KEY_SHIFT,  TableControlAction::cursorSelectRowAreaTop }
                  };
            for (const ActionMapEntry& rAction : aKnownActions)
            {
                if ((rAction.nKeyCode == nKeyCode) && (rAction.nKeyModifier == rKeyCode.GetModifier()))
                {
                    bHandled = m_pImpl->dispatchAction(rAction.eAction);
                    break;
                }
            }
        }

        if (!bHandled)
            Control::KeyInput( rKEvt );
        else
        {
            m_pImpl->commitCellEvent( AccessibleEventId::STATE_CHANGED,
                                      Any( AccessibleStateType::FOCUSED ),
                                      Any()
                                    );
                // Huh? What the heck? Why do we unconditionally notify a STATE_CHANGE/FOCUSED after each and every
                // (handled) key stroke?

            m_pImpl->commitTableEvent( AccessibleEventId::ACTIVE_DESCENDANT_CHANGED,
                                       Any(),
                                       Any()
                                     );
                // ditto: Why do we notify this unconditionally? We should find the right place to notify the
                // ACTIVE_DESCENDANT_CHANGED event.
                // Also, we should check if STATE_CHANGED/FOCUSED is really necessary: finally, the children are
                // transient, aren't they?
        }
    }


    void TableControl::StateChanged( StateChangedType i_nStateChange )
    {
        Control::StateChanged( i_nStateChange );

        // forward certain settings to the data window
        switch ( i_nStateChange )
        {
        case StateChangedType::ControlFocus:
            m_pImpl->invalidateSelectedRows();
            break;

        case StateChangedType::ControlBackground:
            if ( IsControlBackground() )
                getDataWindow().SetControlBackground( GetControlBackground() );
            else
                getDataWindow().SetControlBackground();
            break;

        case StateChangedType::ControlForeground:
            if ( IsControlForeground() )
                getDataWindow().SetControlForeground( GetControlForeground() );
            else
                getDataWindow().SetControlForeground();
            break;

        case StateChangedType::ControlFont:
            if ( IsControlFont() )
                getDataWindow().SetControlFont( GetControlFont() );
            else
                getDataWindow().SetControlFont();
            break;
        default:;
        }
    }


    void TableControl::Resize()
    {
        Control::Resize();
        m_pImpl->onResize();
    }


    void TableControl::SetModel( const PTableModel& _pModel )
    {
        m_pImpl->setModel( _pModel );
    }


    PTableModel TableControl::GetModel() const
    {
        return m_pImpl->getModel();
    }


    sal_Int32 TableControl::GetCurrentRow() const
    {
        return m_pImpl->getCurrentRow();
    }


    sal_Int32 TableControl::GetCurrentColumn() const
    {
        return m_pImpl->getCurrentColumn();
    }


    void TableControl::GoTo( ColPos _nColumn, RowPos _nRow )
    {
        m_pImpl->goTo( _nColumn, _nRow );
    }


    void TableControl::GoToCell(sal_Int32 _nColPos, sal_Int32 _nRowPos)
    {
        m_pImpl->goTo( _nColPos, _nRowPos );
    }


    sal_Int32 TableControl::GetSelectedRowCount() const
    {
        return sal_Int32( m_pImpl->getSelectedRowCount() );
    }


    sal_Int32 TableControl::GetSelectedRowIndex( sal_Int32 const i_selectionIndex ) const
    {
        return m_pImpl->getSelectedRowIndex( i_selectionIndex );
    }


    bool TableControl::IsRowSelected( sal_Int32 const i_rowIndex ) const
    {
        return m_pImpl->isRowSelected( i_rowIndex );
    }


    void TableControl::SelectRow( sal_Int32 const i_rowIndex, bool const i_select )
    {
        ENSURE_OR_RETURN_VOID( ( i_rowIndex >= 0 ) && ( i_rowIndex < m_pImpl->getModel()->getRowCount() ),
            "TableControl::SelectRow: invalid row index!" );

        if ( i_select )
        {
            if ( !m_pImpl->markRowAsSelected( i_rowIndex ) )
                // nothing to do
                return;
        }
        else
        {
            m_pImpl->markRowAsDeselected( i_rowIndex );
        }

        m_pImpl->invalidateRowRange( i_rowIndex, i_rowIndex );
        Select();
    }


    void TableControl::SelectAllRows( bool const i_select )
    {
        if ( i_select )
        {
            if ( !m_pImpl->markAllRowsAsSelected() )
                // nothing to do
                return;
        }
        else
        {
            if ( !m_pImpl->markAllRowsAsDeselected() )
                // nothing to do
                return;
        }


        Invalidate();
            // TODO: can't we do better than this, and invalidate only the rows which changed?
        Select();
    }

    SelectionEngine* TableControl::getSelEngine()
    {
        return m_pImpl->getSelEngine();
    }


    vcl::Window& TableControl::getDataWindow()
    {
        return m_pImpl->getDataWindow();
    }


    Reference< XAccessible > TableControl::CreateAccessible()
    {
        css::uno::Reference<css::accessibility::XAccessible> xParent = GetAccessibleParent();
        return m_pImpl->getAccessible(xParent);
    }

    OUString TableControl::GetAccessibleObjectName( AccessibleTableControlObjType eObjType, sal_Int32 _nRow, sal_Int32 _nCol) const
    {
        OUString aRetText;
        //Window* pWin;
        switch( eObjType )
        {
            case AccessibleTableControlObjType::GRIDCONTROL:
                aRetText = "Grid control";
                break;
            case AccessibleTableControlObjType::TABLE:
                aRetText = "Grid control";
                break;
            case AccessibleTableControlObjType::ROWHEADERBAR:
                aRetText = "RowHeaderBar";
                break;
            case AccessibleTableControlObjType::COLUMNHEADERBAR:
                aRetText = "ColumnHeaderBar";
                break;
            case AccessibleTableControlObjType::TABLECELL:
                //the name of the cell consists of column name and row name if defined
                //if the name is equal to cell content, it'll be read twice
                if(GetModel()->hasColumnHeaders())
                {
                    aRetText = GetColumnName(_nCol) + " , ";
                }
                if(GetModel()->hasRowHeaders())
                {
                    aRetText += GetRowName(_nRow) + " , ";
                }
                //aRetText = GetAccessibleCellText(_nRow, _nCol);
                break;
            case AccessibleTableControlObjType::ROWHEADERCELL:
                aRetText = GetRowName(_nRow);
                break;
            case AccessibleTableControlObjType::COLUMNHEADERCELL:
                aRetText = GetColumnName(_nCol);
                break;
            default:
                OSL_FAIL("GridControl::GetAccessibleName: invalid enum!");
        }
        return aRetText;
    }


    OUString TableControl::GetAccessibleObjectDescription( AccessibleTableControlObjType eObjType ) const
    {
        OUString aRetText;
        switch( eObjType )
        {
            case AccessibleTableControlObjType::GRIDCONTROL:
                aRetText = "Grid control description";
                break;
            case AccessibleTableControlObjType::TABLE:
                    aRetText = "TABLE description";
                break;
            case AccessibleTableControlObjType::ROWHEADERBAR:
                    aRetText = "ROWHEADERBAR description";
                break;
            case AccessibleTableControlObjType::COLUMNHEADERBAR:
                    aRetText = "COLUMNHEADERBAR description";
                break;
            case AccessibleTableControlObjType::TABLECELL:
                // the description of the cell consists of column name and row name if defined
                // if the name is equal to cell content, it'll be read twice
                if ( GetModel()->hasColumnHeaders() )
                {
                    aRetText = GetColumnName( GetCurrentColumn() ) + " , ";
                }
                if ( GetModel()->hasRowHeaders() )
                {
                    aRetText += GetRowName( GetCurrentRow() );
                }
                break;
            case AccessibleTableControlObjType::ROWHEADERCELL:
                    aRetText = "ROWHEADERCELL description";
                break;
            case AccessibleTableControlObjType::COLUMNHEADERCELL:
                    aRetText = "COLUMNHEADERCELL description";
                break;
        }
        return aRetText;
    }


    OUString TableControl::GetRowName( sal_Int32 _nIndex) const
    {
        OUString sRowName;
        GetModel()->getRowHeading( _nIndex ) >>= sRowName;
        return sRowName;
    }


    OUString TableControl::GetColumnName( sal_Int32 _nIndex) const
    {
        return GetModel()->getColumnModel(_nIndex)->getName();
    }


    OUString TableControl::GetAccessibleCellText( sal_Int32 _nRowPos, sal_Int32 _nColPos) const
    {
        return m_pImpl->getCellContentAsString( _nRowPos, _nColPos );
    }


    void TableControl::FillAccessibleStateSet(
            sal_Int64& rStateSet,
            AccessibleTableControlObjType eObjType ) const
    {
        switch( eObjType )
        {
            case AccessibleTableControlObjType::GRIDCONTROL:
            case AccessibleTableControlObjType::TABLE:

                rStateSet |= AccessibleStateType::FOCUSABLE;

                if ( m_pImpl->getSelEngine()->GetSelectionMode() == SelectionMode::Multiple )
                    rStateSet |= AccessibleStateType::MULTI_SELECTABLE;

                if ( HasChildPathFocus() )
                    rStateSet |= AccessibleStateType::FOCUSED;

                if ( IsActive() )
                    rStateSet |= AccessibleStateType::ACTIVE;

                if ( m_pImpl->getDataWindow().IsEnabled() )
                {
                    rStateSet |= AccessibleStateType::ENABLED;
                    rStateSet |= AccessibleStateType::SENSITIVE;
                }

                if ( IsReallyVisible() )
                    rStateSet |= AccessibleStateType::VISIBLE;

                if ( eObjType == AccessibleTableControlObjType::TABLE )
                    rStateSet |= AccessibleStateType::MANAGES_DESCENDANTS;
                break;

            case AccessibleTableControlObjType::COLUMNHEADERBAR:
            case AccessibleTableControlObjType::ROWHEADERBAR:
                rStateSet |= AccessibleStateType::VISIBLE;
                rStateSet |= AccessibleStateType::MANAGES_DESCENDANTS;
                break;

            case AccessibleTableControlObjType::TABLECELL:
                {
                    rStateSet |= AccessibleStateType::FOCUSABLE;
                    if ( HasChildPathFocus() )
                        rStateSet |= AccessibleStateType::FOCUSED;
                    rStateSet |= AccessibleStateType::ACTIVE;
                    rStateSet |= AccessibleStateType::TRANSIENT;
                    rStateSet |= AccessibleStateType::SELECTABLE;
                    rStateSet |= AccessibleStateType::VISIBLE;
                    rStateSet |= AccessibleStateType::SHOWING;
                    if ( IsRowSelected( GetCurrentRow() ) )
                        // Hmm? Wouldn't we expect the affected row to be a parameter to this function?
                        rStateSet |= AccessibleStateType::SELECTED;
                }
                break;

            case AccessibleTableControlObjType::ROWHEADERCELL:
                rStateSet |= AccessibleStateType::VISIBLE;
                rStateSet |= AccessibleStateType::TRANSIENT;
                break;

            case AccessibleTableControlObjType::COLUMNHEADERCELL:
                rStateSet |= AccessibleStateType::VISIBLE;
                break;
        }
    }

    void TableControl::commitCellEvent(sal_Int16 const i_eventID, const Any& i_newValue, const Any& i_oldValue)
    {
        m_pImpl->commitCellEvent( i_eventID, i_newValue, i_oldValue );
    }

    void TableControl::commitTableEvent(sal_Int16 const i_eventID, const Any& i_newValue, const Any& i_oldValue)
    {
        m_pImpl->commitTableEvent( i_eventID, i_newValue, i_oldValue );
    }

    bool TableControl::HasRowHeader()
    {
        return GetModel()->hasRowHeaders();
    }


    bool TableControl::HasColHeader()
    {
        return GetModel()->hasColumnHeaders();
    }


    sal_Int32 TableControl::GetAccessibleControlCount() const
    {
        // TC_TABLE is always defined, no matter whether empty or not
        sal_Int32 count = 1;
        if ( GetModel()->hasRowHeaders() )
            ++count;
        if ( GetModel()->hasColumnHeaders() )
            ++count;
        return count;
    }

    sal_Int32 TableControl::GetRowCount() const
    {
        return GetModel()->getRowCount();
    }


    sal_Int32 TableControl::GetColumnCount() const
    {
        return GetModel()->getColumnCount();
    }


    bool TableControl::ConvertPointToCellAddress( sal_Int32& _rnRow, sal_Int32& _rnColPos, const Point& _rPoint )
    {
        _rnRow = m_pImpl->getRowAtPoint( _rPoint );
        _rnColPos = m_pImpl->getColAtPoint( _rPoint );
        return _rnRow >= 0;
    }


    void TableControl::FillAccessibleStateSetForCell( sal_Int64& _rStateSet, sal_Int32 _nRow, sal_uInt16 ) const
    {
        if ( IsRowSelected( _nRow ) )
            _rStateSet |= AccessibleStateType::SELECTED;
        if ( HasChildPathFocus() )
            _rStateSet |= AccessibleStateType::FOCUSED;
        else // only transient when column is not focused
            _rStateSet |= AccessibleStateType::TRANSIENT;

        _rStateSet |= AccessibleStateType::VISIBLE;
        _rStateSet |= AccessibleStateType::SHOWING;
        _rStateSet |= AccessibleStateType::ENABLED;
        _rStateSet |= AccessibleStateType::SENSITIVE;
        _rStateSet |= AccessibleStateType::ACTIVE;
    }


    tools::Rectangle TableControl::calcHeaderRect(bool _bIsColumnBar )
    {
        return m_pImpl->calcHeaderRect(_bIsColumnBar);
    }


    tools::Rectangle TableControl::calcHeaderCellRect( bool _bIsColumnBar, sal_Int32 nPos )
    {
        return m_pImpl->calcHeaderCellRect( _bIsColumnBar, nPos );
    }


    tools::Rectangle TableControl::calcTableRect()
    {
        return m_pImpl->calcTableRect();
    }


    tools::Rectangle TableControl::calcCellRect( sal_Int32 _nRowPos, sal_Int32 _nColPos )
    {
        return m_pImpl->calcCellRect( _nRowPos, _nColPos );
    }


    IMPL_LINK_NOARG(TableControl, ImplSelectHdl, LinkParamNone*, void)
    {
        Select();
    }


    void TableControl::Select()
    {
        ImplCallEventListenersAndHandler( VclEventId::TableRowSelect, nullptr );
        m_pImpl->commitAccessibleEvent( AccessibleEventId::SELECTION_CHANGED );

        m_pImpl->commitTableEvent( AccessibleEventId::ACTIVE_DESCENDANT_CHANGED, Any(), Any() );
            // TODO: why do we notify this when the *selection* changed? Shouldn't we find a better place for this,
            // actually, when the active descendant, i.e. the current cell, *really* changed?
    }

    TableCell TableControl::hitTest(const Point& rPoint) const
    {
        return m_pImpl->hitTest(rPoint);
    }

    void TableControl::invalidate(const TableArea aArea)
    {
        return m_pImpl->invalidate(aArea);
    }

} // namespace svt::table


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
