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


#include <symbol.hxx>
#include <smmod.hxx>
#include "tmpdevice.hxx"
#include <utility>
#include <visitors.hxx>
#include <tools/UnitConversion.hxx>
#include <vcl/metric.hxx>
#include <o3tl/safeint.hxx>
#include <osl/diagnose.h>
#include <basegfx/numeric/ftools.hxx>
#include <unicode/uchar.h>
#include <unicode/uscript.h>

namespace {

template<typename F>
void ForEachNonNull(SmNode *pNode, F && f)
{
    size_t nSize = pNode->GetNumSubNodes();
    for (size_t i = 0; i < nSize; ++i)
    {
        SmNode *pSubNode = pNode->GetSubNode(i);
        if (pSubNode != nullptr)
            f(pSubNode);
    }
}

}

SmNode::SmNode(SmNodeType eNodeType, SmToken aNodeToken)
    : maNodeToken(std::move( aNodeToken ))
    , meType( eNodeType )
    , meScaleMode( SmScaleMode::None )
    , meRectHorAlign( RectHorAlign::Left )
    , mnFlags( FontChangeMask::None )
    , mnAttributes( FontAttribute::None )
    , mbIsPhantom( false )
    , mbIsSelected( false )
    , mnAccIndex( -1 )
    , mpParentNode( nullptr )
{
}

SmNode::~SmNode()
{
}

const SmNode * SmNode::GetLeftMost() const
    //  returns leftmost node of current subtree.
    //! (this assumes the one with index 0 is always the leftmost subnode
    //! for the current node).
{
    const SmNode *pNode = GetNumSubNodes() > 0 ?
                        GetSubNode(0) : nullptr;

    return pNode ? pNode->GetLeftMost() : this;
}


void SmNode::SetPhantom(bool bIsPhantomP)
{
    if (! (Flags() & FontChangeMask::Phantom))
        mbIsPhantom = bIsPhantomP;

    bool b = mbIsPhantom;
    ForEachNonNull(this, [b](SmNode *pNode){pNode->SetPhantom(b);});
}


void SmNode::SetColor(const Color& rColor)
{
    if (! (Flags() & FontChangeMask::Color))
        GetFont().SetColor(rColor);

    ForEachNonNull(this, [&rColor](SmNode *pNode){pNode->SetColor(rColor);});
}


void SmNode::SetAttribute(FontAttribute nAttrib)
{
    if (
        (nAttrib == FontAttribute::Bold && !(Flags() & FontChangeMask::Bold)) ||
        (nAttrib == FontAttribute::Italic && !(Flags() & FontChangeMask::Italic))
       )
    {
        mnAttributes |= nAttrib;
    }

    ForEachNonNull(this, [nAttrib](SmNode *pNode){pNode->SetAttribute(nAttrib);});
}


void SmNode::ClearAttribute(FontAttribute nAttrib)
{
    if (
        (nAttrib == FontAttribute::Bold && !(Flags() & FontChangeMask::Bold)) ||
        (nAttrib == FontAttribute::Italic && !(Flags() & FontChangeMask::Italic))
       )
    {
        mnAttributes &= ~nAttrib;
    }

    ForEachNonNull(this, [nAttrib](SmNode *pNode){pNode->ClearAttribute(nAttrib);});
}


void SmNode::SetFont(const SmFace &rFace)
{
    if (!(Flags() & FontChangeMask::Face))
        GetFont() = rFace;
    ForEachNonNull(this, [&rFace](SmNode *pNode){pNode->SetFont(rFace);});
}


void SmNode::SetFontSize(const Fraction &rSize, FontSizeType nType)
    //! 'rSize' is in units of pts
{
    Size  aFntSize;

    if (!(Flags() & FontChangeMask::Size))
    {
        Fraction aVal(conversionFract(o3tl::Length::pt, SmO3tlLengthUnit()) * rSize);
        tools::Long      nHeight = static_cast<tools::Long>(aVal);

        aFntSize = GetFont().GetFontSize();
        aFntSize.setWidth( 0 );
        switch(nType)
        {
            case FontSizeType::ABSOLUT:
                aFntSize.setHeight( nHeight );
                break;

            case FontSizeType::PLUS:
                aFntSize.AdjustHeight(nHeight );
                break;

            case FontSizeType::MINUS:
                aFntSize.AdjustHeight( -nHeight );
                break;

            case FontSizeType::MULTIPLY:
                aFntSize.setHeight( static_cast<tools::Long>(Fraction(aFntSize.Height()) * rSize) );
                break;

            case FontSizeType::DIVIDE:
                if (rSize != Fraction(0))
                    aFntSize.setHeight( static_cast<tools::Long>(Fraction(aFntSize.Height()) / rSize) );
                break;
            default:
                break;
        }

        // check the requested size against maximum value
        const int nMaxVal = o3tl::convert(128, o3tl::Length::pt, SmO3tlLengthUnit());
        if (aFntSize.Height() > nMaxVal)
            aFntSize.setHeight( nMaxVal );

        GetFont().SetSize(aFntSize);
    }

    ForEachNonNull(this, [&rSize, &nType](SmNode *pNode){pNode->SetFontSize(rSize, nType);});
}


void SmNode::SetSize(const Fraction &rSize)
{
    GetFont() *= rSize;

    ForEachNonNull(this, [&rSize](SmNode *pNode){pNode->SetSize(rSize);});
}


void SmNode::SetRectHorAlign(RectHorAlign eHorAlign, bool bApplyToSubTree )
{
    meRectHorAlign = eHorAlign;

    if (bApplyToSubTree)
        ForEachNonNull(this, [eHorAlign](SmNode *pNode){pNode->SetRectHorAlign(eHorAlign);});
}


void SmNode::PrepareAttributes()
{
    GetFont().SetWeight((Attributes() & FontAttribute::Bold)   ? WEIGHT_BOLD   : WEIGHT_NORMAL);
    GetFont().SetItalic((Attributes() & FontAttribute::Italic) ? ITALIC_NORMAL : ITALIC_NONE);
}


void SmNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    if (nDepth > 1024)
        throw std::range_error("parser depth limit");

    mbIsPhantom  = false;
    mnFlags      = FontChangeMask::None;
    mnAttributes = FontAttribute::None;

    switch (rFormat.GetHorAlign())
    {   case SmHorAlign::Left:     meRectHorAlign = RectHorAlign::Left;   break;
        case SmHorAlign::Center:   meRectHorAlign = RectHorAlign::Center; break;
        case SmHorAlign::Right:    meRectHorAlign = RectHorAlign::Right;  break;
    }

    GetFont() = rFormat.GetFont(FNT_MATH);
    OSL_ENSURE( GetFont().GetCharSet() == RTL_TEXTENCODING_UNICODE,
            "unexpected CharSet" );
    GetFont().SetWeight(WEIGHT_NORMAL);
    GetFont().SetItalic(ITALIC_NONE);

    ForEachNonNull(this, [&rFormat, &rDocShell, nDepth](SmNode *pNode){pNode->Prepare(rFormat, rDocShell, nDepth + 1);});
}

void SmNode::Move(const Point& rVector)
{
    if (rVector.X() == 0  &&  rVector.Y() == 0)
        return;

    SmRect::Move(rVector);

    ForEachNonNull(this, [&rVector](SmNode *pNode){pNode->Move(rVector);});
}

void SmNode::AdaptToX(OutputDevice &/*rDev*/, tools::Long /*nWidth*/)
{
}


void SmNode::AdaptToY(OutputDevice &/*rDev*/, tools::Long /*nHeight*/)
{
}


const SmNode * SmNode::FindTokenAt(sal_uInt16 nRow, sal_uInt16 nCol) const
    // returns (first) ** visible ** (sub)node with the tokens text at
    // position 'nRow', 'nCol'.
    //! (there should be exactly one such node if any)
{
    if (    IsVisible()
        &&  nRow == GetSelection().start.nPara
        &&  nCol >= GetSelection().start.nIndex  &&  nCol <= GetSelection().end.nIndex )
        return this;
    else
    {
        size_t nNumSubNodes = GetNumSubNodes();
        for (size_t i = 0;  i < nNumSubNodes; ++i)
        {
            const SmNode *pNode = GetSubNode(i);

            if (!pNode)
                continue;

            const SmNode *pResult = pNode->FindTokenAt(nRow, nCol);
            if (pResult)
                return pResult;
        }
    }

    return nullptr;
}


const SmNode * SmNode::FindRectClosestTo(const Point &rPoint) const
{
    tools::Long          nDist   = LONG_MAX;
    const SmNode *pResult = nullptr;

    if (IsVisible())
        pResult = this;
    else
    {
        size_t nNumSubNodes = GetNumSubNodes();
        for (size_t i = 0;  i < nNumSubNodes; ++i)
        {
            const SmNode *pNode = GetSubNode(i);

            if (!pNode)
                continue;

            const SmNode *pFound = pNode->FindRectClosestTo(rPoint);
            if (pFound)
            {
                tools::Long nTmp = pFound->OrientedDist(rPoint);
                if (nTmp < nDist)
                {
                    nDist   = nTmp;
                    pResult = pFound;

                    // quit immediately if 'rPoint' is inside the *should not
                    // overlap with other rectangles* part.
                    // This (partly) serves for getting the attributes in eg
                    // "bar overstrike a".
                    // ('nDist < 0' is used as *quick shot* to avoid evaluation of
                    // the following expression, where the result is already determined)
                    if (nDist < 0  &&  pFound->IsInsideRect(rPoint))
                        break;
                }
            }
        }
    }

    return pResult;
}

const SmNode * SmNode::FindNodeWithAccessibleIndex(sal_Int32 nAccIdx) const
{
    const SmNode *pResult = nullptr;

    sal_Int32 nIdx = GetAccessibleIndex();
    OUStringBuffer aTxt;
    if (nIdx >= 0)
        GetAccessibleText( aTxt );  // get text if used in following 'if' statement

    if (nIdx >= 0
        &&  nIdx <= nAccIdx  &&  nAccIdx < nIdx + aTxt.getLength())
        pResult = this;
    else
    {
        size_t nNumSubNodes = GetNumSubNodes();
        for (size_t i = 0; i < nNumSubNodes; ++i)
        {
            const SmNode *pNode = GetSubNode(i);
            if (!pNode)
                continue;

            pResult = pNode->FindNodeWithAccessibleIndex(nAccIdx);
            if (pResult)
                return pResult;
        }
    }

    return pResult;
}


SmStructureNode::~SmStructureNode()
{
    ForEachNonNull(this, std::default_delete<SmNode>());
}


void SmStructureNode::ClearSubNodes()
{
    maSubNodes.clear();
}

void SmStructureNode::SetSubNodes(std::unique_ptr<SmNode> pFirst, std::unique_ptr<SmNode> pSecond, std::unique_ptr<SmNode> pThird)
{
    size_t nSize = pThird ? 3 : (pSecond ? 2 : (pFirst ? 1 : 0));
    maSubNodes.resize( nSize );
    if (pFirst)
        maSubNodes[0] = pFirst.release();
    if (pSecond)
        maSubNodes[1] = pSecond.release();
    if (pThird)
        maSubNodes[2] = pThird.release();

    ClaimPaternity();
}

void SmStructureNode::SetSubNodes(SmNode* pFirst, SmNode* pSecond, SmNode* pThird)
{
    size_t nSize = pThird ? 3 : (pSecond ? 2 : (pFirst ? 1 : 0));
    maSubNodes.resize( nSize );
    if (pFirst)
        maSubNodes[0] = pFirst;
    if (pSecond)
        maSubNodes[1] = pSecond;
    if (pThird)
        maSubNodes[2] = pThird;

    ClaimPaternity();
}

void SmStructureNode::SetSubNodesBinMo(std::unique_ptr<SmNode> pFirst, std::unique_ptr<SmNode> pSecond, std::unique_ptr<SmNode> pThird)
{
    if(GetType()==SmNodeType::BinDiagonal)
    {
        size_t nSize = pSecond ? 3 : (pThird ? 2 : (pFirst ? 1 : 0));
        maSubNodes.resize( nSize );
        if (pFirst)
            maSubNodes[0] = pFirst.release();
        if (pSecond)
            maSubNodes[2] = pSecond.release();
        if (pThird)
            maSubNodes[1] = pThird.release();
    }
    else
    {
        size_t nSize = pThird ? 3 : (pSecond ? 2 : (pFirst ? 1 : 0));
        maSubNodes.resize( nSize );
        if (pFirst)
            maSubNodes[0] = pFirst.release();
        if (pSecond)
            maSubNodes[1] = pSecond.release();
        if (pThird)
            maSubNodes[2] = pThird.release();
    }
    ClaimPaternity();
}

void SmStructureNode::SetSubNodes(SmNodeArray&& rNodeArray)
{
    maSubNodes = std::move(rNodeArray);
    ClaimPaternity();
}

bool SmStructureNode::IsVisible() const
{
    return false;
}

size_t SmStructureNode::GetNumSubNodes() const
{
    return maSubNodes.size();
}

SmNode* SmStructureNode::GetSubNode(size_t nIndex)
{
    return maSubNodes[nIndex];
}

SmNode* SmStructureNode::GetSubNodeBinMo(size_t nIndex) const
{
    if(GetType()==SmNodeType::BinDiagonal)
    {
        if (nIndex==1)
            nIndex = 2;
        else if (nIndex==2)
            nIndex = 1;
    }
    return maSubNodes[nIndex];
}

void SmStructureNode::GetAccessibleText( OUStringBuffer &rText ) const
{
    ForEachNonNull(const_cast<SmStructureNode *>(this),
                   [&rText](SmNode *pNode)
        {
            if (pNode->IsVisible())
                pNode->SetAccessibleIndex(rText.getLength());
            pNode->GetAccessibleText( rText );
        });
}

void SmStructureNode::ClaimPaternity()
{
    ForEachNonNull(this, [this](SmNode *pNode){pNode->SetParent(this);});
}

int SmStructureNode::IndexOfSubNode(SmNode const * pSubNode)
{
    size_t nSize = GetNumSubNodes();
    for (size_t i = 0; i < nSize; i++)
        if (pSubNode == GetSubNode(i))
            return i;
    return -1;
}

void SmStructureNode::SetSubNode(size_t nIndex, SmNode* pNode)
{
    size_t size = maSubNodes.size();
    if (size <= nIndex)
    {
        //Resize subnodes array
        maSubNodes.resize(nIndex + 1);
        //Set new slots to NULL except at nIndex
        for (size_t i = size; i < nIndex; i++)
            maSubNodes[i] = nullptr;
    }
    maSubNodes[nIndex] = pNode;
    if (pNode)
        pNode->SetParent(this);
}

bool SmVisibleNode::IsVisible() const
{
    return true;
}

size_t SmVisibleNode::GetNumSubNodes() const
{
    return 0;
}

SmNode * SmVisibleNode::GetSubNode(size_t /*nIndex*/)
{
    return nullptr;
}

void SmGraphicNode::GetAccessibleText( OUStringBuffer &rText ) const
{
    rText.append(GetToken().aText);
}

void SmTableNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
    // arranges all subnodes in one column
{
    SmNode *pNode;
    size_t nSize = GetNumSubNodes();

    // make distance depend on font size
    tools::Long  nDist = +(rFormat.GetDistance(DIS_VERTICAL)
                    * GetFont().GetFontSize().Height()) / 100;

    if (nSize < 1)
        return;

    // arrange subnodes and get maximum width of them
    tools::Long  nMaxWidth = 0,
          nTmp;
    for (size_t i = 0; i < nSize; ++i)
    {
        if (nullptr != (pNode = GetSubNode(i)))
        {   pNode->Arrange(rDev, rFormat);
            if ((nTmp = pNode->GetItalicWidth()) > nMaxWidth)
                nMaxWidth = nTmp;
        }
    }

    Point  aPos;
    SmRect::operator = (SmRect(nMaxWidth, 1));
    for (size_t i = 0; i < nSize; ++i)
    {
        if (nullptr != (pNode = GetSubNode(i)))
        {   const SmRect &rNodeRect = pNode->GetRect();
            const SmNode *pCoNode   = pNode->GetLeftMost();
            RectHorAlign  eHorAlign = pCoNode->GetRectHorAlign();

            aPos = rNodeRect.AlignTo(*this, RectPos::Bottom,
                        eHorAlign, RectVerAlign::Baseline);
            if (i)
                aPos.AdjustY(nDist );
            pNode->MoveTo(aPos);
            ExtendBy(rNodeRect, nSize > 1 ? RectCopyMBL::None : RectCopyMBL::Arg);
        }
    }
    // #i972#
    if (HasBaseline())
        mnFormulaBaseline = GetBaseline();
    else
    {
        SmTmpDevice aTmpDev (rDev, true);
        aTmpDev.SetFont(GetFont());

        SmRect aRect(aTmpDev, &rFormat, u"a"_ustr, GetFont().GetBorderWidth());
        mnFormulaBaseline = GetAlignM();
        // move from middle position by constant - distance
        // between middle and baseline for single letter
        mnFormulaBaseline += aRect.GetBaseline() - aRect.GetAlignM();
    }
}

const SmNode * SmTableNode::GetLeftMost() const
{
    return this;
}


tools::Long SmTableNode::GetFormulaBaseline() const
{
    return mnFormulaBaseline;
}


/**************************************************************************/


void SmLineNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    SmNode::Prepare(rFormat, rDocShell, nDepth);

    // Here we use the 'FNT_VARIABLE' font since it's ascent and descent in general fit better
    // to the rest of the formula compared to the 'FNT_MATH' font.
    GetFont() = rFormat.GetFont(FNT_VARIABLE);
    Flags() |= FontChangeMask::Face;
}


/**************************************************************************/


void SmLineNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
    // arranges all subnodes in one row with some extra space between
{
    SmNode *pNode;
    size_t nSize = GetNumSubNodes();
    for (size_t i = 0; i < nSize; ++i)
    {
        if (nullptr != (pNode = GetSubNode(i)))
            pNode->Arrange(rDev, rFormat);
    }

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    if (nSize < 1)
    {
        // provide an empty rectangle with alignment parameters for the "current"
        // font (in order to make "a^1 {}_2^3 a_4" work correct, that is, have the
        // same sub-/supscript positions.)
        //! be sure to use a character that has explicitly defined HiAttribut
        //! line in rect.cxx such as 'a' in order to make 'vec a' look same to
        //! 'vec {a}'.
        SmRect::operator = (SmRect(aTmpDev, &rFormat, u"a"_ustr,
                            GetFont().GetBorderWidth()));
        // make sure that the rectangle occupies (almost) no space
        SetWidth(1);
        SetItalicSpaces(0, 0);
        return;
    }

    // make distance depend on font size
    tools::Long nDist = (rFormat.GetDistance(DIS_HORIZONTAL) * GetFont().GetFontSize().Height()) / 100;
    if (!IsUseExtraSpaces())
        nDist = 0;

    Point   aPos;
    // copy the first node into LineNode and extend by the others
    if (nullptr != (pNode = GetSubNode(0)))
        SmRect::operator = (pNode->GetRect());

    for (size_t i = 1;  i < nSize; ++i)
    {
        if (nullptr != (pNode = GetSubNode(i)))
        {
            aPos = pNode->AlignTo(*this, RectPos::Right, RectHorAlign::Center, RectVerAlign::Baseline);

            // add horizontal space to the left for each but the first sub node
            aPos.AdjustX(nDist );

            pNode->MoveTo(aPos);
            ExtendBy( *pNode, RectCopyMBL::Xor );
        }
    }
}


/**************************************************************************/


void SmExpressionNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
    // as 'SmLineNode::Arrange' but keeps alignment of leftmost subnode
{
    SmLineNode::Arrange(rDev, rFormat);

    //  copy alignment of leftmost subnode if any
    const SmNode *pNode = GetLeftMost();
    if (pNode)
        SetRectHorAlign(pNode->GetRectHorAlign(), false);
}


/**************************************************************************/


void SmUnHorNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    bool  bIsPostfix = GetToken().eType == TFACT;

    SmNode *pNode0 = GetSubNode(0),
           *pNode1 = GetSubNode(1);
    SmNode *pOper = bIsPostfix ? pNode1 : pNode0,
           *pBody = bIsPostfix ? pNode0 : pNode1;
    assert(pOper);
    assert(pBody);

    pOper->SetSize(Fraction (rFormat.GetRelSize(SIZ_OPERATOR), 100));
    pOper->Arrange(rDev, rFormat);
    pBody->Arrange(rDev, rFormat);

    tools::Long nDist = (pOper->GetRect().GetWidth() * rFormat.GetDistance(DIS_HORIZONTAL)) / 100;

    SmRect::operator = (*pNode0);

    Point aPos = pNode1->AlignTo(*this, RectPos::Right, RectHorAlign::Center, RectVerAlign::Baseline);
    aPos.AdjustX(nDist );
    pNode1->MoveTo(aPos);
    ExtendBy(*pNode1, RectCopyMBL::Xor);
}


/**************************************************************************/

namespace {

void lcl_GetHeightVerOffset(const SmRect &rRect,
                                    tools::Long &rHeight, tools::Long &rVerOffset)
    // calculate height and vertical offset of root sign suitable for 'rRect'
{
    rVerOffset = (rRect.GetBottom() - rRect.GetAlignB()) / 2;
    rHeight    = rRect.GetHeight() - rVerOffset;

    OSL_ENSURE(rHeight    >= 0, "Sm : Ooops...");
    OSL_ENSURE(rVerOffset >= 0, "Sm : Ooops...");
}


Point lcl_GetExtraPos(const SmRect &rRootSymbol,
                              const SmRect &rExtra)
{
    const Size &rSymSize = rRootSymbol.GetSize();

    Point  aPos = rRootSymbol.GetTopLeft()
            + Point((rSymSize.Width()  * 70) / 100,
                    (rSymSize.Height() * 52) / 100);

    // from this calculate topleft edge of 'rExtra'
    aPos.AdjustX( -(rExtra.GetWidth() + rExtra.GetItalicRightSpace()) );
    aPos.AdjustY( -(rExtra.GetHeight()) );
    // if there's enough space move a bit less to the right
    // examples: "nroot i a", "nroot j a"
    // (it looks better if we don't use italic-spaces here)
    tools::Long  nX = rRootSymbol.GetLeft() + (rSymSize.Width() * 30) / 100;
    if (aPos.X() > nX)
        aPos.setX( nX );

    return aPos;
}

}

void SmRootNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    //! pExtra needs to have the smaller index than pRootSym in order to
    //! not to get the root symbol but the pExtra when clicking on it in the
    //! GraphicWindow. (That is because of the simplicity of the algorithm
    //! that finds the node corresponding to a mouseclick in the window.)
    SmNode *pExtra   = GetSubNode(0),
           *pRootSym = GetSubNode(1),
           *pBody    = GetSubNode(2);
    assert(pRootSym);
    assert(pBody);

    pBody->Arrange(rDev, rFormat);

    tools::Long  nHeight,
          nVerOffset;
    lcl_GetHeightVerOffset(*pBody, nHeight, nVerOffset);
    nHeight += rFormat.GetDistance(DIS_ROOT)
               * GetFont().GetFontSize().Height() / 100;

    if (nHeight < 0)
    {
        SAL_WARN("starmath", "negative height");
        nHeight = 0;
    }

    // font specialist advised to change the width first
    pRootSym->AdaptToY(rDev, nHeight);
    pRootSym->AdaptToX(rDev, pBody->GetItalicWidth());

    pRootSym->Arrange(rDev, rFormat);

    // Set the top and bottom of the root symbol to the top and bottom of its glyph bounding rect,
    // to get accurate position of the root symbol.
    SmRect rRootSymRect = pRootSym->AsGlyphRect();
    pRootSym->SetTop(rRootSymRect.GetTop());
    pRootSym->SetBottom(rRootSymRect.GetBottom());

    Point  aPos = pRootSym->AlignTo(*pBody, RectPos::Left, RectHorAlign::Center, RectVerAlign::Baseline);
    //! override calculated vertical position
    aPos.setY( pRootSym->GetTop() + pBody->GetBottom() - pRootSym->GetBottom() );
    aPos.AdjustY( -nVerOffset );
    pRootSym->MoveTo(aPos);

    if (pExtra)
    {   pExtra->SetSize(Fraction(rFormat.GetRelSize(SIZ_INDEX), 100));
        pExtra->Arrange(rDev, rFormat);

        aPos = lcl_GetExtraPos(*pRootSym, *pExtra);
        pExtra->MoveTo(aPos);
    }

    SmRect::operator = (*pBody);
    ExtendBy(*pRootSym, RectCopyMBL::This);
    if (pExtra)
        ExtendBy(*pExtra, RectCopyMBL::This, true);
}

/**************************************************************************/


void SmBinHorNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmNode *pLeft  = LeftOperand(),
           *pOper  = Symbol(),
           *pRight = RightOperand();
    assert(pLeft);
    assert(pOper);
    assert(pRight);

    pOper->SetSize(Fraction (rFormat.GetRelSize(SIZ_OPERATOR), 100));

    pLeft ->Arrange(rDev, rFormat);
    pOper ->Arrange(rDev, rFormat);
    pRight->Arrange(rDev, rFormat);

    const SmRect &rOpRect = pOper->GetRect();

    tools::Long nMul;
    if (o3tl::checked_multiply<tools::Long>(rOpRect.GetWidth(), rFormat.GetDistance(DIS_HORIZONTAL), nMul))
    {
        SAL_WARN("starmath", "integer overflow");
        return;
    }

    tools::Long nDist = nMul / 100;

    SmRect::operator = (*pLeft);

    Point aPos;
    aPos = pOper->AlignTo(*this, RectPos::Right, RectHorAlign::Center, RectVerAlign::Baseline);
    aPos.AdjustX(nDist );
    pOper->MoveTo(aPos);
    ExtendBy(*pOper, RectCopyMBL::Xor);

    aPos = pRight->AlignTo(*this, RectPos::Right, RectHorAlign::Center, RectVerAlign::Baseline);
    aPos.AdjustX(nDist );

    pRight->MoveTo(aPos);
    ExtendBy(*pRight, RectCopyMBL::Xor);
}


/**************************************************************************/


void SmBinVerNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmNode *pNum   = GetSubNode(0),
           *pLine  = GetSubNode(1),
           *pDenom = GetSubNode(2);
    assert(pNum);
    assert(pLine);
    assert(pDenom);

    bool  bIsTextmode = rFormat.IsTextmode();
    if (bIsTextmode)
    {
        Fraction  aFraction(rFormat.GetRelSize(SIZ_INDEX), 100);
        pNum  ->SetSize(aFraction);
        pLine ->SetSize(aFraction);
        pDenom->SetSize(aFraction);
    }

    pNum  ->Arrange(rDev, rFormat);
    pDenom->Arrange(rDev, rFormat);

    tools::Long  nFontHeight = GetFont().GetFontSize().Height(),
          nExtLen     = nFontHeight * rFormat.GetDistance(DIS_FRACTION) / 100,
          nThick      = nFontHeight * rFormat.GetDistance(DIS_STROKEWIDTH) / 100,
          nWidth      = std::max(pNum->GetItalicWidth(), pDenom->GetItalicWidth()),
          nNumDist    = bIsTextmode ? 0 :
                            nFontHeight * rFormat.GetDistance(DIS_NUMERATOR)   / 100,
          nDenomDist  = bIsTextmode ? 0 :
                            nFontHeight * rFormat.GetDistance(DIS_DENOMINATOR) / 100;

    // font specialist advised to change the width first
    pLine->AdaptToY(rDev, nThick);
    pLine->AdaptToX(rDev, nWidth + 2 * nExtLen);
    pLine->Arrange(rDev, rFormat);

    // get horizontal alignment for numerator
    const SmNode *pLM       = pNum->GetLeftMost();
    RectHorAlign  eHorAlign = pLM->GetRectHorAlign();

    // move numerator to its position
    Point  aPos = pNum->AlignTo(*pLine, RectPos::Top, eHorAlign, RectVerAlign::Baseline);
    aPos.AdjustY( -nNumDist );
    pNum->MoveTo(aPos);

    // get horizontal alignment for denominator
    pLM       = pDenom->GetLeftMost();
    eHorAlign = pLM->GetRectHorAlign();

    // move denominator to its position
    aPos = pDenom->AlignTo(*pLine, RectPos::Bottom, eHorAlign, RectVerAlign::Baseline);
    aPos.AdjustY(nDenomDist );
    pDenom->MoveTo(aPos);

    SmRect::operator = (*pNum);
    ExtendBy(*pDenom, RectCopyMBL::None).ExtendBy(*pLine, RectCopyMBL::None, pLine->GetCenterY());
}

const SmNode * SmBinVerNode::GetLeftMost() const
{
    return this;
}


namespace {

/// @return value of the determinant formed by the two points
double Det(const Point &rHeading1, const Point &rHeading2)
{
    return rHeading1.X() * rHeading2.Y() - rHeading1.Y() * rHeading2.X();
}


/// Is true iff the point 'rPoint1' belongs to the straight line through 'rPoint2'
/// and has the direction vector 'rHeading2'
bool IsPointInLine(const Point &rPoint1,
                   const Point &rPoint2, const Point &rHeading2)
{
    assert(rHeading2 != Point());

    bool bRes = false;
    static const double eps = 5.0 * DBL_EPSILON;

    double fLambda;
    if (std::abs(rHeading2.X()) > std::abs(rHeading2.Y()))
    {
        fLambda = (rPoint1.X() - rPoint2.X()) / static_cast<double>(rHeading2.X());
        bRes = fabs(rPoint1.Y() - (rPoint2.Y() + fLambda * rHeading2.Y())) < eps;
    }
    else
    {
        fLambda = (rPoint1.Y() - rPoint2.Y()) / static_cast<double>(rHeading2.Y());
        bRes = fabs(rPoint1.X() - (rPoint2.X() + fLambda * rHeading2.X())) < eps;
    }

    return bRes;
}


sal_uInt16 GetLineIntersectionPoint(Point &rResult,
                                const Point& rPoint1, const Point &rHeading1,
                                const Point& rPoint2, const Point &rHeading2)
{
    assert(rHeading1 != Point());
    assert(rHeading2 != Point());

    sal_uInt16 nRes = 1;
    static const double eps = 5.0 * DBL_EPSILON;

    // are the direction vectors linearly dependent?
    double  fDet = Det(rHeading1, rHeading2);
    if (fabs(fDet) < eps)
    {
        nRes    = IsPointInLine(rPoint1, rPoint2, rHeading2) ? USHRT_MAX : 0;
        rResult = nRes ? rPoint1 : Point();
    }
    else
    {
        // here we do not pay attention to the computational accuracy
        // (that would be more complicated and is not really worth it in this case)
        double fLambda = (    (rPoint1.Y() - rPoint2.Y()) * rHeading2.X()
                            - (rPoint1.X() - rPoint2.X()) * rHeading2.Y())
                         / fDet;
        rResult = Point(rPoint1.X() + static_cast<tools::Long>(fLambda * rHeading1.X()),
                        rPoint1.Y() + static_cast<tools::Long>(fLambda * rHeading1.Y()));
    }

    return nRes;
}

}


/// @return position and size of the diagonal line
/// premise: SmRect of the node defines the limitation(!) consequently it has to be known upfront
void SmBinDiagonalNode::GetOperPosSize(Point &rPos, Size &rSize,
                        const Point &rDiagPoint, double fAngleDeg) const

{
    double  fAngleRad   = basegfx::deg2rad(fAngleDeg);
    tools::Long    nRectLeft   = GetItalicLeft(),
            nRectRight  = GetItalicRight(),
            nRectTop    = GetTop(),
            nRectBottom = GetBottom();
    Point   aRightHdg     (100, 0),
            aDownHdg      (0, 100),
            aDiagHdg      ( static_cast<tools::Long>(100.0 * cos(fAngleRad)),
                            static_cast<tools::Long>(-100.0 * sin(fAngleRad)) );

    tools::Long  nLeft, nRight, nTop, nBottom;     // margins of the rectangle for the diagonal
    Point aPoint;
    if (IsAscending())
    {
        // determine top right corner
        GetLineIntersectionPoint(aPoint,
            Point(nRectLeft, nRectTop), aRightHdg,
            rDiagPoint, aDiagHdg);
        // is there a point of intersection with the top border?
        if (aPoint.X() <= nRectRight)
        {
            nRight = aPoint.X();
            nTop   = nRectTop;
        }
        else
        {
            // there has to be a point of intersection with the right border!
            GetLineIntersectionPoint(aPoint,
                Point(nRectRight, nRectTop), aDownHdg,
                rDiagPoint, aDiagHdg);

            nRight = nRectRight;
            nTop   = aPoint.Y();
        }

        // determine bottom left corner
        GetLineIntersectionPoint(aPoint,
            Point(nRectLeft, nRectBottom), aRightHdg,
            rDiagPoint, aDiagHdg);
        // is there a point of intersection with the bottom border?
        if (aPoint.X() >= nRectLeft)
        {
            nLeft   = aPoint.X();
            nBottom = nRectBottom;
        }
        else
        {
            // there has to be a point of intersection with the left border!
            GetLineIntersectionPoint(aPoint,
                Point(nRectLeft, nRectTop), aDownHdg,
                rDiagPoint, aDiagHdg);

            nLeft   = nRectLeft;
            nBottom = aPoint.Y();
        }
    }
    else
    {
        // determine top left corner
        GetLineIntersectionPoint(aPoint,
            Point(nRectLeft, nRectTop), aRightHdg,
            rDiagPoint, aDiagHdg);
        // is there a point of intersection with the top border?
        if (aPoint.X() >= nRectLeft)
        {
            nLeft = aPoint.X();
            nTop  = nRectTop;
        }
        else
        {
            // there has to be a point of intersection with the left border!
            GetLineIntersectionPoint(aPoint,
                Point(nRectLeft, nRectTop), aDownHdg,
                rDiagPoint, aDiagHdg);

            nLeft = nRectLeft;
            nTop  = aPoint.Y();
        }

        // determine bottom right corner
        GetLineIntersectionPoint(aPoint,
            Point(nRectLeft, nRectBottom), aRightHdg,
            rDiagPoint, aDiagHdg);
        // is there a point of intersection with the bottom border?
        if (aPoint.X() <= nRectRight)
        {
            nRight  = aPoint.X();
            nBottom = nRectBottom;
        }
        else
        {
            // there has to be a point of intersection with the right border!
            GetLineIntersectionPoint(aPoint,
                Point(nRectRight, nRectTop), aDownHdg,
                rDiagPoint, aDiagHdg);

            nRight  = nRectRight;
            nBottom = aPoint.Y();
        }
    }

    rSize = Size(nRight - nLeft + 1, nBottom - nTop + 1);
    rPos.setX( nLeft );
    rPos.setY( nTop );
}


void SmBinDiagonalNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    // Both arguments have to get into the SubNodes before the Operator so that clicking
    // within the GraphicWindow sets the FormulaCursor correctly (cf. SmRootNode)
    SmNode *pLeft  = GetSubNode(0),
           *pRight = GetSubNode(1),
           *pLine  = GetSubNode(2);
    assert(pLeft);
    assert(pRight);
    assert(pLine && pLine->GetType() == SmNodeType::PolyLine);

    SmPolyLineNode *pOper = static_cast<SmPolyLineNode *>(pLine);
    assert(pOper);

    //! some routines being called extract some info from the OutputDevice's
    //! font (eg the space to be used for borders OR the font name(!!)).
    //! Thus the font should reflect the needs and has to be set!
    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    pLeft->Arrange(aTmpDev, rFormat);
    pRight->Arrange(aTmpDev, rFormat);

    // determine implicitly the values (incl. the margin) of the diagonal line
    pOper->Arrange(aTmpDev, rFormat);

    tools::Long nDelta = pOper->GetWidth() * 8 / 10;

    // determine TopLeft position from the right argument
    Point aPos;
    aPos.setX( pLeft->GetItalicRight() + nDelta + pRight->GetItalicLeftSpace() );
    if (IsAscending())
        aPos.setY( pLeft->GetBottom() + nDelta );
    else
        aPos.setY( pLeft->GetTop() - nDelta - pRight->GetHeight() );

    pRight->MoveTo(aPos);

    // determine new baseline
    tools::Long nTmpBaseline = IsAscending() ? (pLeft->GetBottom() + pRight->GetTop()) / 2
                        : (pLeft->GetTop() + pRight->GetBottom()) / 2;
    Point  aLogCenter ((pLeft->GetItalicRight() + pRight->GetItalicLeft()) / 2,
                       nTmpBaseline);

    SmRect::operator = (*pLeft);
    ExtendBy(*pRight, RectCopyMBL::None);


    // determine position and size of diagonal line
    Size  aTmpSize;
    GetOperPosSize(aPos, aTmpSize, aLogCenter, IsAscending() ? 60.0 : -60.0);

    // font specialist advised to change the width first
    pOper->AdaptToY(aTmpDev, aTmpSize.Height());
    pOper->AdaptToX(aTmpDev, aTmpSize.Width());
    // and make it active
    pOper->Arrange(aTmpDev, rFormat);

    pOper->MoveTo(aPos);

    ExtendBy(*pOper, RectCopyMBL::None, nTmpBaseline);
}


/**************************************************************************/


void SmSubSupNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    OSL_ENSURE(GetNumSubNodes() == 1 + SUBSUP_NUM_ENTRIES,
               "Sm: wrong number of subnodes");

    SmNode *pBody = GetBody();
    assert(pBody);

    tools::Long  nOrigHeight = pBody->GetFont().GetFontSize().Height();

    pBody->Arrange(rDev, rFormat);

    const SmRect &rBodyRect = pBody->GetRect();
    SmRect::operator = (rBodyRect);

    // line that separates sub- and supscript rectangles
    tools::Long  nDelimLine = SmFromTo(GetAlignB(), GetAlignT(), 0.4);

    Point  aPos;
    tools::Long   nDelta, nDist;

    // iterate over all possible sub-/supscripts
    SmRect  aTmpRect (rBodyRect);
    for (int i = 0;  i < SUBSUP_NUM_ENTRIES;  i++)
    {
        SmSubSup eSubSup = static_cast<SmSubSup>(i);
        SmNode *pSubSup = GetSubSup(eSubSup);

        if (!pSubSup)
            continue;

        // switch position of limits if we are in textmode
        if (rFormat.IsTextmode()  &&  (GetToken().nGroup & TG::Limit))
            switch (eSubSup)
            {   case CSUB:  eSubSup = RSUB;     break;
                case CSUP:  eSubSup = RSUP;     break;
                default:
                    break;
            }

        // prevent sub-/supscripts from diminishing in size
        // (as would be in "a_{1_{2_{3_4}}}")
        if (GetFont().GetFontSize().Height() > rFormat.GetBaseSize().Height() / 3)
        {
            sal_uInt16 nIndex = (eSubSup == CSUB  ||  eSubSup == CSUP) ?
                                    SIZ_LIMITS : SIZ_INDEX;
            Fraction  aFraction ( rFormat.GetRelSize(nIndex), 100 );
            pSubSup->SetSize(aFraction);
        }

        pSubSup->Arrange(rDev, rFormat);

        bool  bIsTextmode = rFormat.IsTextmode();
        nDist = 0;

        //! be sure that CSUB, CSUP are handled before the other cases!
        switch (eSubSup)
        {   case RSUB :
            case LSUB :
                if (!bIsTextmode)
                    nDist = nOrigHeight
                            * rFormat.GetDistance(DIS_SUBSCRIPT) / 100;
                aPos  = pSubSup->GetRect().AlignTo(aTmpRect,
                                eSubSup == LSUB ? RectPos::Left : RectPos::Right,
                                RectHorAlign::Center, RectVerAlign::Bottom);
                aPos.AdjustY(nDist );
                nDelta = nDelimLine - aPos.Y();
                if (nDelta > 0)
                    aPos.AdjustY(nDelta );
                break;
            case RSUP :
            case LSUP :
                if (!bIsTextmode)
                    nDist = nOrigHeight
                            * rFormat.GetDistance(DIS_SUPERSCRIPT) / 100;
                aPos  = pSubSup->GetRect().AlignTo(aTmpRect,
                                eSubSup == LSUP ? RectPos::Left : RectPos::Right,
                                RectHorAlign::Center, RectVerAlign::Top);
                aPos.AdjustY( -nDist );
                nDelta = aPos.Y() + pSubSup->GetHeight() - nDelimLine;
                if (nDelta > 0)
                    aPos.AdjustY( -nDelta );
                break;
            case CSUB :
                if (!bIsTextmode)
                    nDist = nOrigHeight
                            * rFormat.GetDistance(DIS_LOWERLIMIT) / 100;
                aPos = pSubSup->GetRect().AlignTo(rBodyRect, RectPos::Bottom,
                                RectHorAlign::Center, RectVerAlign::Baseline);
                aPos.AdjustY(nDist );
                break;
            case CSUP :
                if (!bIsTextmode)
                    nDist = nOrigHeight
                            * rFormat.GetDistance(DIS_UPPERLIMIT) / 100;
                aPos = pSubSup->GetRect().AlignTo(rBodyRect, RectPos::Top,
                                RectHorAlign::Center, RectVerAlign::Baseline);
                aPos.AdjustY( -nDist );
                break;
        }

        pSubSup->MoveTo(aPos);
        ExtendBy(*pSubSup, RectCopyMBL::This, true);

        // update rectangle to which  RSUB, RSUP, LSUB, LSUP
        // will be aligned to
        if (eSubSup == CSUB  ||  eSubSup == CSUP)
            aTmpRect = *this;
    }
}

/**************************************************************************/

void SmBraceNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmNode *pLeft  = OpeningBrace(),
           *pBody  = Body(),
           *pRight = ClosingBrace();
    assert(pLeft);
    assert(pBody);
    assert(pRight);

    pBody->Arrange(rDev, rFormat);

    bool  bIsScaleNormal = rFormat.IsScaleNormalBrackets(),
          bScale         = pBody->GetHeight() > 0  &&
                           (GetScaleMode() == SmScaleMode::Height  ||  bIsScaleNormal),
          bIsABS         = GetToken().eType == TABS;

    tools::Long  nFaceHeight = GetFont().GetFontSize().Height();

    // determine oversize in %
    sal_uInt16  nPerc = 0;
    if (!bIsABS && bScale)
    {   // in case of oversize braces...
        sal_uInt16 nIndex = GetScaleMode() == SmScaleMode::Height ?
                            DIS_BRACKETSIZE : DIS_NORMALBRACKETSIZE;
        nPerc = rFormat.GetDistance(nIndex);
    }

    // determine the height for the braces
    tools::Long  nBraceHeight;
    if (bScale)
    {
        nBraceHeight = pBody->GetType() == SmNodeType::Bracebody ?
                              static_cast<SmBracebodyNode *>(pBody)->GetBodyHeight()
                            : pBody->GetHeight();
        nBraceHeight += 2 * (nBraceHeight * nPerc / 100);
    }
    else
        nBraceHeight = nFaceHeight;

    // distance to the argument
    nPerc = bIsABS ? 0 : rFormat.GetDistance(DIS_BRACKETSPACE);
    tools::Long  nDist = nFaceHeight * nPerc / 100;

    // if wanted, scale the braces to the wanted size
    if (bScale)
    {
        Size  aTmpSize (pLeft->GetFont().GetFontSize());
        OSL_ENSURE(pRight->GetFont().GetFontSize() == aTmpSize,
                    "Sm : different font sizes");
        aTmpSize.setWidth( std::min(nBraceHeight * 60 / 100,
                            rFormat.GetBaseSize().Height() * 3 / 2) );
        // correction factor since change from StarMath to OpenSymbol font
        // because of the different font width in the FontMetric
        aTmpSize.setWidth( aTmpSize.Width() * 182 );
        aTmpSize.setWidth( aTmpSize.Width() / 267 );

        sal_Unicode cChar = pLeft->GetToken().cMathChar[0];
        if (cChar != MS_LINE  &&  cChar != MS_DLINE &&
            cChar != MS_VERTLINE  &&  cChar != MS_DVERTLINE)
            pLeft ->GetFont().SetSize(aTmpSize);

        cChar = pRight->GetToken().cMathChar[0];
        if (cChar != MS_LINE  &&  cChar != MS_DLINE &&
            cChar != MS_VERTLINE  &&  cChar != MS_DVERTLINE)
            pRight->GetFont().SetSize(aTmpSize);

        pLeft ->AdaptToY(rDev, nBraceHeight);
        pRight->AdaptToY(rDev, nBraceHeight);
    }

    pLeft ->Arrange(rDev, rFormat);
    pRight->Arrange(rDev, rFormat);

    // required in order to make "\(a\) - (a) - left ( a right )" look alright
    RectVerAlign  eVerAlign = bScale ? RectVerAlign::CenterY : RectVerAlign::Baseline;

    Point         aPos;
    aPos = pLeft->AlignTo(*pBody, RectPos::Left, RectHorAlign::Center, eVerAlign);
    aPos.AdjustX( -nDist );
    pLeft->MoveTo(aPos);

    aPos = pRight->AlignTo(*pBody, RectPos::Right, RectHorAlign::Center, eVerAlign);
    aPos.AdjustX(nDist );
    pRight->MoveTo(aPos);

    SmRect::operator = (*pBody);
    ExtendBy(*pLeft, RectCopyMBL::This).ExtendBy(*pRight, RectCopyMBL::This);
}


/**************************************************************************/


void SmBracebodyNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    size_t nNumSubNodes = GetNumSubNodes();
    if (nNumSubNodes == 0)
        return;

    // arrange arguments
    for (size_t i = 0;  i < nNumSubNodes; i += 2)
        GetSubNode(i)->Arrange(rDev, rFormat);

    // build reference rectangle with necessary info for vertical alignment
    SmRect  aRefRect (*GetSubNode(0));
    for (size_t i = 0;  i < nNumSubNodes; i += 2)
    {
        SmRect aTmpRect (*GetSubNode(i));
        Point  aPos = aTmpRect.AlignTo(aRefRect, RectPos::Right, RectHorAlign::Center, RectVerAlign::Baseline);
        aTmpRect.MoveTo(aPos);
        aRefRect.ExtendBy(aTmpRect, RectCopyMBL::Xor);
    }

    mnBodyHeight = aRefRect.GetHeight();

    // scale separators to required height and arrange them
    bool bScale  = GetScaleMode() == SmScaleMode::Height  ||  rFormat.IsScaleNormalBrackets();
    tools::Long nHeight = bScale ? aRefRect.GetHeight() : GetFont().GetFontSize().Height();
    sal_uInt16 nIndex  = GetScaleMode() == SmScaleMode::Height ?
                        DIS_BRACKETSIZE : DIS_NORMALBRACKETSIZE;
    sal_uInt16 nPerc   = rFormat.GetDistance(nIndex);
    if (bScale)
        nHeight += 2 * (nHeight * nPerc / 100);
    for (size_t i = 1; i < nNumSubNodes; i += 2)
    {
        SmNode *pNode = GetSubNode(i);
        pNode->AdaptToY(rDev, nHeight);
        pNode->Arrange(rDev, rFormat);
    }

    // horizontal distance between argument and brackets or separators
    tools::Long  nDist = GetFont().GetFontSize().Height()
                  * rFormat.GetDistance(DIS_BRACKETSPACE) / 100;

    SmNode *pLeft = GetSubNode(0);
    SmRect::operator = (*pLeft);
    for (size_t i = 1; i < nNumSubNodes; ++i)
    {
        bool          bIsSeparator = i % 2 != 0;
        RectVerAlign  eVerAlign    = bIsSeparator ? RectVerAlign::CenterY : RectVerAlign::Baseline;

        SmNode *pRight = GetSubNode(i);
        Point  aPosX = pRight->AlignTo(*pLeft,   RectPos::Right, RectHorAlign::Center, eVerAlign),
               aPosY = pRight->AlignTo(aRefRect, RectPos::Right, RectHorAlign::Center, eVerAlign);
        aPosX.AdjustX(nDist );

        pRight->MoveTo(Point(aPosX.X(), aPosY.Y()));
        ExtendBy(*pRight, bIsSeparator ? RectCopyMBL::This : RectCopyMBL::Xor);

        pLeft = pRight;
    }
}


/**************************************************************************/


void SmVerticalBraceNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmNode *pBody   = Body(),
           *pBrace  = Brace(),
           *pScript = Script();
    assert(pBody);
    assert(pBrace);
    assert(pScript);

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    pBody->Arrange(aTmpDev, rFormat);

    // size is the same as for limits for this part
    pScript->SetSize( Fraction( rFormat.GetRelSize(SIZ_LIMITS), 100 ) );
    // braces are a bit taller than usually
    pBrace ->SetSize( Fraction(3, 2) );

    tools::Long  nItalicWidth = pBody->GetItalicWidth();
    if (nItalicWidth > 0)
        pBrace->AdaptToX(aTmpDev, nItalicWidth);

    pBrace ->Arrange(aTmpDev, rFormat);
    pScript->Arrange(aTmpDev, rFormat);

    // determine the relative position and the distances between each other
    RectPos  eRectPos;
    tools::Long nFontHeight = pBody->GetFont().GetFontSize().Height();
    tools::Long nDistBody   = nFontHeight * rFormat.GetDistance(DIS_ORNAMENTSIZE),
         nDistScript = nFontHeight;
    if (GetToken().eType == TOVERBRACE)
    {
        eRectPos = RectPos::Top;
        nDistBody    = - nDistBody;
        nDistScript *= - rFormat.GetDistance(DIS_UPPERLIMIT);
    }
    else // TUNDERBRACE
    {
        eRectPos = RectPos::Bottom;
        nDistScript *= + rFormat.GetDistance(DIS_LOWERLIMIT);
    }
    nDistBody   /= 100;
    nDistScript /= 100;

    Point  aPos = pBrace->AlignTo(*pBody, eRectPos, RectHorAlign::Center, RectVerAlign::Baseline);
    aPos.AdjustY(nDistBody );
    pBrace->MoveTo(aPos);

    aPos = pScript->AlignTo(*pBrace, eRectPos, RectHorAlign::Center, RectVerAlign::Baseline);
    aPos.AdjustY(nDistScript );
    pScript->MoveTo(aPos);

    SmRect::operator = (*pBody);
    ExtendBy(*pBrace, RectCopyMBL::This).ExtendBy(*pScript, RectCopyMBL::This);
}


/**************************************************************************/


SmNode * SmOperNode::GetSymbol()
{
    SmNode *pNode = GetSubNode(0);
    assert(pNode);

    if (pNode->GetType() == SmNodeType::SubSup)
        pNode = static_cast<SmSubSupNode *>(pNode)->GetBody();

    OSL_ENSURE(pNode, "Sm: NULL pointer!");
    return pNode;
}


tools::Long SmOperNode::CalcSymbolHeight(const SmNode &rSymbol,
                                  const SmFormat &rFormat) const
    // returns the font height to be used for operator-symbol
{
    tools::Long  nHeight = GetFont().GetFontSize().Height();

    SmTokenType  eTmpType = GetToken().eType;
    if (eTmpType == TLIM  ||  eTmpType == TLIMINF  ||  eTmpType == TLIMSUP)
        return nHeight;

    if (!rFormat.IsTextmode())
    {
        // set minimum size ()
        nHeight += (nHeight * 20) / 100;

        nHeight += nHeight
                   * rFormat.GetDistance(DIS_OPERATORSIZE) / 100;
        nHeight = nHeight * 686 / 845;
    }

    // correct user-defined symbols to match height of sum from used font
    if (rSymbol.GetToken().eType == TSPECIAL)
        nHeight = nHeight * 845 / 686;

    return nHeight;
}


void SmOperNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmNode *pOper = GetSubNode(0);
    SmNode *pBody = GetSubNode(1);

    assert(pOper);
    assert(pBody);

    SmNode *pSymbol = GetSymbol();
    pSymbol->SetSize(Fraction(CalcSymbolHeight(*pSymbol, rFormat),
                              pSymbol->GetFont().GetFontSize().Height()));

    pBody->Arrange(rDev, rFormat);
    bool bDynamicallySized = false;
    if (pSymbol->GetToken().eType == TINTD)
    {
        tools::Long nBodyHeight = pBody->GetHeight();
        tools::Long nFontHeight = pSymbol->GetFont().GetFontSize().Height();
        if (nFontHeight < nBodyHeight)
        {
            pSymbol->SetSize(Fraction(nBodyHeight, nFontHeight));
            bDynamicallySized = true;
        }
    }
    pOper->Arrange(rDev, rFormat);

    tools::Long  nOrigHeight = GetFont().GetFontSize().Height(),
          nDist = nOrigHeight
                  * rFormat.GetDistance(DIS_OPERATORSPACE) / 100;

    Point aPos = pOper->AlignTo(*pBody, RectPos::Left, RectHorAlign::Center, bDynamicallySized ? RectVerAlign::CenterY : RectVerAlign::Mid);
    aPos.AdjustX( -nDist );
    pOper->MoveTo(aPos);

    SmRect::operator = (*pBody);
    ExtendBy(*pOper, RectCopyMBL::This);
}


/**************************************************************************/


void SmAlignNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
    // set alignment within the entire subtree (including current node)
{
    assert(GetNumSubNodes() == 1);

    SmNode  *pNode = GetSubNode(0);
    assert(pNode);

    RectHorAlign  eHorAlign = RectHorAlign::Center;
    switch (GetToken().eType)
    {
        case TALIGNL:   eHorAlign = RectHorAlign::Left;   break;
        case TALIGNC:   eHorAlign = RectHorAlign::Center; break;
        case TALIGNR:   eHorAlign = RectHorAlign::Right;  break;
        default:
            break;
    }
    SetRectHorAlign(eHorAlign);

    pNode->Arrange(rDev, rFormat);

    SmRect::operator = (pNode->GetRect());
}


/**************************************************************************/


void SmAttributeNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmNode *pAttr = Attribute(),
           *pBody = Body();
    assert(pBody);
    assert(pAttr);

    pBody->Arrange(rDev, rFormat);

    if (GetScaleMode() == SmScaleMode::Width)
        pAttr->AdaptToX(rDev, pBody->GetItalicWidth());
    pAttr->Arrange(rDev, rFormat);

    // get relative position of attribute
    RectVerAlign  eVerAlign;
    tools::Long          nDist = 0;
    switch (GetToken().eType)
    {   case TUNDERLINE :
            eVerAlign = RectVerAlign::AttributeLo;
            break;
        case TOVERSTRIKE :
            eVerAlign = RectVerAlign::AttributeMid;
            break;
        default :
            eVerAlign = RectVerAlign::AttributeHi;
            if (pBody->GetType() == SmNodeType::Attribute)
                nDist = GetFont().GetFontSize().Height()
                        * rFormat.GetDistance(DIS_ORNAMENTSPACE) / 100;
    }
    Point  aPos = pAttr->AlignTo(*pBody, RectPos::Attribute, RectHorAlign::Center, eVerAlign);
    aPos.AdjustY( -nDist );
    pAttr->MoveTo(aPos);

    SmRect::operator = (*pBody);
    ExtendBy(*pAttr, RectCopyMBL::This, true);
}

void SmFontNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    //! prepare subnodes first
    SmNode::Prepare(rFormat, rDocShell, nDepth);

    int  nFnt = -1;
    switch (GetToken().eType)
    {
        case TFIXED:    nFnt = FNT_FIXED;   break;
        case TSANS:     nFnt = FNT_SANS;    break;
        case TSERIF:    nFnt = FNT_SERIF;   break;
        default:
            break;
    }
    if (nFnt != -1)
    {   GetFont() = rFormat.GetFont( sal::static_int_cast< sal_uInt16 >(nFnt) );
        SetFont(GetFont());
    }

    //! prevent overwrites of this font by 'Arrange' or 'SetFont' calls of
    //! other font nodes (those with lower depth in the tree)
    Flags() |= FontChangeMask::Face;
}

void SmFontNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmNode *pNode = GetSubNode(1);
    assert(pNode);
    sal_uInt32 nc;

    switch (GetToken().eType)
    {   case TSIZE :
            pNode->SetFontSize(maFontSize, meSizeType);
            break;
        case TSANS :
        case TSERIF :
        case TFIXED :
            pNode->SetFont(GetFont());
            break;
        case TUNKNOWN : break;  // no assertion on "font <?> <?>"

        case TPHANTOM : SetPhantom(true);               break;
        case TBOLD :    SetAttribute(FontAttribute::Bold);     break;
        case TITALIC :  SetAttribute(FontAttribute::Italic);   break;
        case TNBOLD :   ClearAttribute(FontAttribute::Bold);   break;
        case TNITALIC : ClearAttribute(FontAttribute::Italic); break;

        // Using HTML CSS Level 1 standard
        case TRGB :
        case TRGBA :
        case THTMLCOL :
        case TMATHMLCOL :
        case TDVIPSNAMESCOL:
        case TICONICCOL :
        case THEX :
            nc = GetToken().cMathChar.toUInt32(16);
            SetColor(Color(ColorTransparency, nc));
            break;

        default:
            SAL_WARN("starmath", "unknown case");
    }

    pNode->Arrange(rDev, rFormat);

    SmRect::operator = (pNode->GetRect());
}

/**************************************************************************/


SmPolyLineNode::SmPolyLineNode(const SmToken &rNodeToken)
    : SmGraphicNode(SmNodeType::PolyLine, rNodeToken)
    , maPoly(2)
    , mnWidth(0)
{
}


void SmPolyLineNode::AdaptToX(OutputDevice &/*rDev*/, tools::Long nNewWidth)
{
    maToSize.setWidth( nNewWidth );
}


void SmPolyLineNode::AdaptToY(OutputDevice &/*rDev*/, tools::Long nNewHeight)
{
    GetFont().FreezeBorderWidth();
    maToSize.setHeight( nNewHeight );
}


void SmPolyLineNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    //! some routines being called extract some info from the OutputDevice's
    //! font (eg the space to be used for borders OR the font name(!!)).
    //! Thus the font should reflect the needs and has to be set!
    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    tools::Long  nBorderwidth = GetFont().GetBorderWidth();

    // create polygon using both endpoints
    assert(maPoly.GetSize() == 2);
    Point  aPointA, aPointB;
    if (GetToken().eType == TWIDESLASH)
    {
        aPointA.setX( nBorderwidth );
        aPointA.setY( maToSize.Height() - nBorderwidth );
        aPointB.setX( maToSize.Width() - nBorderwidth );
        aPointB.setY( nBorderwidth );
    }
    else
    {
        OSL_ENSURE(GetToken().eType == TWIDEBACKSLASH, "Sm : unexpected token");
        aPointA.setX( nBorderwidth );
        aPointA.setY( nBorderwidth );
        aPointB.setX( maToSize.Width() - nBorderwidth );
        aPointB.setY( maToSize.Height() - nBorderwidth );
    }
    maPoly.SetPoint(aPointA, 0);
    maPoly.SetPoint(aPointB, 1);

    tools::Long  nThick       = GetFont().GetFontSize().Height()
                            * rFormat.GetDistance(DIS_STROKEWIDTH) / 100;
    mnWidth = nThick + 2 * nBorderwidth;

    SmRect::operator = (SmRect(maToSize.Width(), maToSize.Height()));
}


/**************************************************************************/

void SmRootSymbolNode::AdaptToX(OutputDevice &/*rDev*/, tools::Long nWidth)
{
    mnBodyWidth = nWidth;
}


void SmRootSymbolNode::AdaptToY(OutputDevice &rDev, tools::Long nHeight)
{
    // some additional length so that the horizontal
    // bar will be positioned above the argument
    SmMathSymbolNode::AdaptToY(rDev, nHeight + nHeight / 10);
}


/**************************************************************************/


void SmRectangleNode::AdaptToX(OutputDevice &/*rDev*/, tools::Long nWidth)
{
    maToSize.setWidth( nWidth );
}


void SmRectangleNode::AdaptToY(OutputDevice &/*rDev*/, tools::Long nHeight)
{
    GetFont().FreezeBorderWidth();
    maToSize.setHeight( nHeight );
}


void SmRectangleNode::Arrange(OutputDevice &rDev, const SmFormat &/*rFormat*/)
{
    tools::Long  nFontHeight = GetFont().GetFontSize().Height();
    tools::Long  nWidth  = maToSize.Width(),
          nHeight = maToSize.Height();
    if (nHeight == 0)
        nHeight = nFontHeight / 30;
    if (nWidth == 0)
        nWidth  = nFontHeight / 3;

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    // add some borderspace
    tools::Long  nTmpBorderWidth = GetFont().GetBorderWidth();
    nHeight += 2 * nTmpBorderWidth;

    //! use this method in order to have 'SmRect::HasAlignInfo() == true'
    //! and thus having the attribute-fences updated in 'SmRect::ExtendBy'
    SmRect::operator = (SmRect(nWidth, nHeight));
}


/**************************************************************************/


SmTextNode::SmTextNode( SmNodeType eNodeType, const SmToken &rNodeToken, sal_uInt16 nFontDescP )
    : SmVisibleNode(eNodeType, rNodeToken)
    , mnFontDesc(nFontDescP)
    , mnSelectionStart(0)
    , mnSelectionEnd(0)
{
}

SmTextNode::SmTextNode( const SmToken &rNodeToken, sal_uInt16 nFontDescP )
    : SmVisibleNode(SmNodeType::Text, rNodeToken)
    , mnFontDesc(nFontDescP)
    , mnSelectionStart(0)
    , mnSelectionEnd(0)
{
}

void SmTextNode::ChangeText(const OUString &rText) {
    maText = rText;
    GetToken().aText = rText;
    AdjustFontDesc();
}

void SmTextNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    SmNode::Prepare(rFormat, rDocShell, nDepth);

    // default setting for horizontal alignment of nodes with TTEXT
    // content is as alignl (cannot be done in Arrange since it would
    // override the settings made by an SmAlignNode before)
    if (TTEXT == GetToken().eType)
        SetRectHorAlign( RectHorAlign::Left );

    maText = GetToken().aText;
    GetFont() = rFormat.GetFont(GetFontDesc());

    if (IsItalic( GetFont() ))
        Attributes() |= FontAttribute::Italic;
    if (IsBold( GetFont() ))
        Attributes() |= FontAttribute::Bold;

    // special handling for ':' where it is a token on its own and is likely
    // to be used for mathematical notations. (E.g. a:b = 2:3)
    // In that case it should not be displayed in italic.
    if (maText.getLength() == 1 && GetToken().aText[0] == ':')
        Attributes() &= ~FontAttribute::Italic;

    // Arabic text should not be italic, so we check for any character in Arabic script and
    // remove italic attribute.
    if (!maText.isEmpty())
    {
        sal_Int32 nIndex = 0;
        while (nIndex < maText.getLength())
        {
            sal_uInt32 cChar = maText.iterateCodePoints(&nIndex);
            if (u_getIntPropertyValue(cChar, UCHAR_SCRIPT) == USCRIPT_ARABIC)
            {
                Attributes() &= ~FontAttribute::Italic;
                break;
            }
        }
    }
};


void SmTextNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    PrepareAttributes();

    sal_uInt16  nSizeDesc = GetFontDesc() == FNT_FUNCTION ?
                            SIZ_FUNCTION : SIZ_TEXT;
    GetFont() *= Fraction (rFormat.GetRelSize(nSizeDesc), 100);

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    SmRect::operator = (SmRect(aTmpDev, &rFormat, maText, GetFont().GetBorderWidth()));
}

void SmTextNode::GetAccessibleText( OUStringBuffer &rText ) const
{
    rText.append(maText);
}

void SmTextNode::AdjustFontDesc()
{
    if (GetToken().nGroup == TG::Function) mnFontDesc = FNT_FUNCTION;
    else if (GetToken().eType == TTEXT) mnFontDesc = FNT_TEXT;
    else {
        sal_Unicode firstChar = maText[0];
        if( ('0' <= firstChar && firstChar <= '9') || firstChar == '.' || firstChar == ',')
            mnFontDesc = FNT_NUMBER;
        else mnFontDesc = FNT_VARIABLE;
    }
}

sal_Unicode SmTextNode::ConvertSymbolToUnicode(sal_Unicode nIn)
{
    //Find the best match in accepted unicode for our private area symbols
    static const sal_Unicode aStarMathPrivateToUnicode[] =
    {
        0x2030, 0xF613, 0xF612, 0x002B, 0x003C, 0x003E, 0xE425, 0xE421, 0xE088, 0x2208,
        0x0192, 0x2026, 0x2192, 0x221A, 0x221A, 0x221A, 0xE090, 0x005E, 0x02C7, 0x02D8,
        0x00B4, 0x0060, 0x02DC, 0x00AF, 0x0362, 0xE099, 0xE09A, 0x20DB, 0xE09C, 0xE09D,
        0x0028, 0x0029, 0x2220, 0x22AF, 0xE0A2, 0xE0A3, 0xE0A4, 0xE0A5, 0xE0A6, 0xE0A7,
        0x002F, 0x005C, 0x274F, 0xE0AB, 0x0393, 0x0394, 0x0398, 0x039b, 0x039e, 0x03A0,
        0x03a3, 0x03a5, 0x03a6, 0x03a8, 0x03A9, 0x03B1, 0x03B2, 0x03b3, 0x03b4, 0x03b5,
        0x03b6, 0x03b7, 0x03b8, 0x03b9, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03be, 0x03bf,
        0x03c0, 0x03c1, 0x03c3, 0x03c4, 0x03c5, 0x03c6, 0x03c7, 0x03c8, 0x03c9, 0x03b5,
        0x03d1, 0x03d6, 0xE0D2, 0x03db, 0x2118, 0x2202, 0x2129, 0xE0D7, 0xE0D8, 0x22A4,
        0xE0DA, 0x2190, 0x2191, 0x2193
    };
    if ((nIn >= 0xE080) && (nIn <= 0xE0DD))
        nIn = aStarMathPrivateToUnicode[nIn-0xE080];

    //For whatever unicode glyph that equation editor doesn't ship with that
    //we have a possible match we can munge it to.
    switch (nIn)
    {
        case 0x2223:
            nIn = '|';
            break;
        default:
            break;
    }

    return nIn;
}

/**************************************************************************/

void SmMatrixNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmNode *pNode;

    // initialize array that is to hold the maximum widths of all
    // elements (subnodes) in that column.
    std::vector<tools::Long> aColWidth(mnNumCols);

    // arrange subnodes and calculate the above arrays contents
    size_t nNodes = GetNumSubNodes();
    for (size_t i = 0; i < nNodes; ++i)
    {
        size_t nIdx = nNodes - 1 - i;
        if (nullptr != (pNode = GetSubNode(nIdx)))
        {
            pNode->Arrange(rDev, rFormat);
            int  nCol = nIdx % mnNumCols;
            aColWidth[nCol] = std::max(aColWidth[nCol], pNode->GetItalicWidth());
        }
    }

    // norm distance from which the following two are calculated
    const tools::Long  nNormDist = 3 * GetFont().GetFontSize().Height();

    // define horizontal and vertical minimal distances that separate
    // the elements
    tools::Long  nHorDist = nNormDist * rFormat.GetDistance(DIS_MATRIXCOL) / 100,
          nVerDist = nNormDist * rFormat.GetDistance(DIS_MATRIXROW) / 100;

    // build array that holds the leftmost position for each column
    std::vector<tools::Long> aColLeft(mnNumCols);
    tools::Long  nX = 0;
    for (size_t j = 0; j < mnNumCols; ++j)
    {
        aColLeft[j] = nX;
        nX += aColWidth[j] + nHorDist;
    }

    SmRect::operator = (SmRect());
    for (size_t i = 0;  i < mnNumRows; ++i)
    {
        Point aPos;
        SmRect aLineRect;
        for (size_t j = 0;  j < mnNumCols; ++j)
        {
            SmNode *pTmpNode = GetSubNode(i * mnNumCols + j);
            assert(pTmpNode);

            const SmRect &rNodeRect = pTmpNode->GetRect();

            // align all baselines in that row if possible
            aPos = rNodeRect.AlignTo(aLineRect, RectPos::Right, RectHorAlign::Center, RectVerAlign::Baseline);

            // get horizontal alignment
            const SmNode *pCoNode   = pTmpNode->GetLeftMost();
            RectHorAlign  eHorAlign = pCoNode->GetRectHorAlign();

            // calculate horizontal position of element depending on column
            // and horizontal alignment
            switch (eHorAlign)
            {   case RectHorAlign::Left:
                    aPos.setX( aColLeft[j] );
                    break;
                case RectHorAlign::Center:
                    aPos.setX( rNodeRect.GetLeft() + aColLeft[j]
                               + aColWidth[j] / 2
                               - rNodeRect.GetItalicCenterX() );
                    break;
                case RectHorAlign::Right:
                    aPos.setX( aColLeft[j]
                               + aColWidth[j] - rNodeRect.GetItalicWidth() );
                    break;
                default:
                    assert(false);
            }

            pTmpNode->MoveTo(aPos);
            aLineRect.ExtendBy(rNodeRect, RectCopyMBL::Xor);
        }

        aPos = aLineRect.AlignTo(*this, RectPos::Bottom, RectHorAlign::Center, RectVerAlign::Baseline);
        if (i > 0)
            aPos.AdjustY(nVerDist );

        // move 'aLineRect' and rectangles in that line to final position
        Point aDelta(0, // since horizontal alignment is already done
                     aPos.Y() - aLineRect.GetTop());
        aLineRect.Move(aDelta);
        for (size_t j = 0;  j < mnNumCols; ++j)
        {
            if (nullptr != (pNode = GetSubNode(i * mnNumCols + j)))
                pNode->Move(aDelta);
        }

        ExtendBy(aLineRect, RectCopyMBL::None);
    }
}

const SmNode * SmMatrixNode::GetLeftMost() const
{
    return this;
}


/**************************************************************************/


SmMathSymbolNode::SmMathSymbolNode(const SmToken &rNodeToken)
:   SmSpecialNode(SmNodeType::Math, rNodeToken, FNT_MATH)
{
    SetText(GetToken().cMathChar);
}

void SmMathSymbolNode::AdaptToX(OutputDevice &rDev, tools::Long nWidth)
{
    // Since there is no function to do this, we try to approximate it:
    Size  aFntSize (GetFont().GetFontSize());

    //! however the result is a bit better with 'nWidth' as initial font width
    aFntSize.setWidth( nWidth );
    GetFont().SetSize(aFntSize);

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    // get denominator of error factor for width
    tools::Long nTmpBorderWidth = GetFont().GetBorderWidth();
    tools::Long nDenom = SmRect(aTmpDev, nullptr, GetText(), nTmpBorderWidth).GetItalicWidth();

    // scale fontwidth with this error factor
    aFntSize.setWidth( aFntSize.Width() * nWidth );
    aFntSize.setWidth( aFntSize.Width() / ( nDenom ? nDenom : 1) );

    GetFont().SetSize(aFntSize);
}

void SmMathSymbolNode::AdaptToY(OutputDevice &rDev, tools::Long nHeight)
{
    GetFont().FreezeBorderWidth();
    Size  aFntSize (GetFont().GetFontSize());

    // Since we only want to scale the height, we might have
    // to determine the font width in order to keep it
    if (aFntSize.Width() == 0)
    {
        rDev.Push(vcl::PushFlags::FONT | vcl::PushFlags::MAPMODE);
        rDev.SetFont(GetFont());
        aFntSize.setWidth( rDev.GetFontMetric().GetFontSize().Width() );
        rDev.Pop();
    }
    OSL_ENSURE(aFntSize.Width() != 0, "Sm: ");

    //! however the result is a bit better with 'nHeight' as initial
    //! font height
    aFntSize.setHeight( nHeight );
    GetFont().SetSize(aFntSize);

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    // get denominator of error factor for height
    tools::Long nTmpBorderWidth = GetFont().GetBorderWidth();
    tools::Long nDenom = 0;
    if (!GetText().isEmpty())
        nDenom = SmRect(aTmpDev, nullptr, GetText(), nTmpBorderWidth).GetHeight();

    // scale fontwidth with this error factor
    aFntSize.setHeight( aFntSize.Height() * nHeight );
    aFntSize.setHeight( aFntSize.Height() / ( nDenom ? nDenom : 1) );

    GetFont().SetSize(aFntSize);
}


void SmMathSymbolNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    SmNode::Prepare(rFormat, rDocShell, nDepth);

    GetFont() = rFormat.GetFont(GetFontDesc());
    // use same font size as is used for variables
    GetFont().SetSize( rFormat.GetFont( FNT_VARIABLE ).GetFontSize() );

    OSL_ENSURE(GetFont().GetCharSet() == RTL_TEXTENCODING_SYMBOL  ||
               GetFont().GetCharSet() == RTL_TEXTENCODING_UNICODE,
        "wrong charset for character from StarMath/OpenSymbol font");

    Flags() |= FontChangeMask::Face | FontChangeMask::Italic;
};


void SmMathSymbolNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    const OUString &rText = GetText();

    if (rText.isEmpty() || rText[0] == '\0')
    {   SmRect::operator = (SmRect());
        return;
    }

    PrepareAttributes();

    GetFont() *= Fraction (rFormat.GetRelSize(SIZ_TEXT), 100);

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    SmRect::operator = (SmRect(aTmpDev, &rFormat, rText, GetFont().GetBorderWidth()));
}

/**************************************************************************/

SmSpecialNode::SmSpecialNode(SmNodeType eNodeType, const SmToken &rNodeToken, sal_uInt16 _nFontDesc)
    : SmTextNode(eNodeType, rNodeToken, _nFontDesc)
{
}


SmSpecialNode::SmSpecialNode(const SmToken &rNodeToken)
    : SmTextNode(SmNodeType::Special, rNodeToken, FNT_VARIABLE)  // default Font isn't always correct!
{
}


void SmSpecialNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    SmNode::Prepare(rFormat, rDocShell, nDepth);

    const SmSym* pSym
        = GetToken().aText.isEmpty()
              ? nullptr
              : SmModule::get()->GetSymbolManager().GetSymbolByName(GetToken().aText.subView(1));

    bool bIsGreekSymbol = false;
    bool bIsSpecialSymbol = false;
    bool bIsArabic = false;

    if (pSym)
    {
        sal_UCS4 cChar = pSym->GetCharacter();
        OUString aTmp( &cChar, 1 );
        SetText( aTmp );
        GetFont() = SmFace(pSym->GetFace(&rFormat));

        OUString aSymbolSetName = SmLocalizedSymbolData::GetExportSymbolSetName(pSym->GetSymbolSetName());
        if (aSymbolSetName == "Greek")
            bIsGreekSymbol = true;
        else if (aSymbolSetName == "Special")
            bIsSpecialSymbol = true;
        else if (aSymbolSetName == "Arabic")
            bIsArabic = true;
    }
    else
    {
        SetText( GetToken().aText );
        GetFont() = rFormat.GetFont(FNT_VARIABLE);
    }
    // use same font size as is used for variables
    GetFont().SetSize( rFormat.GetFont( FNT_VARIABLE ).GetFontSize() );

    // Actually only WEIGHT_NORMAL and WEIGHT_BOLD should occur... However, the sms-file also
    // contains e.g. 'WEIGHT_ULTRALIGHT'. Consequently, compare here with '>' instead of '!='.
    // (In the long term the necessity for 'PrepareAttribut' and thus also for this here should be dropped)

    //! see also SmFontStyles::GetStyleName
    if (IsItalic( GetFont() ))
        SetAttribute(FontAttribute::Italic);
    if (IsBold( GetFont() ))
        SetAttribute(FontAttribute::Bold);

    Flags() |= FontChangeMask::Face;

    sal_uInt32 cChar = 0;
    if (!GetText().isEmpty())
    {
        sal_Int32 nIndex = 0;
        cChar = GetText().iterateCodePoints(&nIndex);
        if (!bIsArabic)
            bIsArabic = u_getIntPropertyValue(cChar, UCHAR_SCRIPT) == USCRIPT_ARABIC;
    }

    if (!bIsGreekSymbol && !bIsSpecialSymbol && !bIsArabic)
        return;

    // Arabic and special symbols should not be italic,
    // Greek is italic only in some cases.
    bool bItalic = false;
    if (bIsGreekSymbol)
    {
        sal_Int16 nStyle = rFormat.GetGreekCharStyle();
        OSL_ENSURE( nStyle >= 0 && nStyle <= 2, "unexpected value for GreekCharStyle" );
        if (nStyle == 1)
            bItalic = true;
        else if (nStyle == 2)
        {
            static const sal_Unicode cUppercaseAlpha = 0x0391;
            static const sal_Unicode cUppercaseOmega = 0x03A9;
            // uppercase letters should be straight and lowercase letters italic
            bItalic = cUppercaseAlpha > cChar || cChar > cUppercaseOmega;
        }
    }

    if (bItalic)
        Attributes() |= FontAttribute::Italic;
    else
        Attributes() &= ~FontAttribute::Italic;
};


void SmSpecialNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    PrepareAttributes();

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    SmRect::operator = (SmRect(aTmpDev, &rFormat, GetText(), GetFont().GetBorderWidth()));
}

/**************************************************************************/


void SmGlyphSpecialNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    PrepareAttributes();

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    SmRect::operator = (SmRect(aTmpDev, &rFormat, GetText(),
                               GetFont().GetBorderWidth()).AsGlyphRect());
}


/**************************************************************************/


void SmPlaceNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    SmNode::Prepare(rFormat, rDocShell, nDepth);

    GetFont().SetColor(COL_GRAY);
    Flags() |= FontChangeMask::Color | FontChangeMask::Face | FontChangeMask::Italic;
};


void SmPlaceNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    PrepareAttributes();

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    SmRect::operator = (SmRect(aTmpDev, &rFormat, GetText(), GetFont().GetBorderWidth()));
}


/**************************************************************************/


void SmErrorNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    SmNode::Prepare(rFormat, rDocShell, nDepth);

    GetFont().SetColor(COL_RED);
    Flags() |= FontChangeMask::Phantom | FontChangeMask::Bold | FontChangeMask::Italic
               | FontChangeMask::Color | FontChangeMask::Face | FontChangeMask::Size;
}


void SmErrorNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    PrepareAttributes();

    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    const OUString &rText = GetText();
    SmRect::operator = (SmRect(aTmpDev, &rFormat, rText, GetFont().GetBorderWidth()));
}

/**************************************************************************/

void SmBlankNode::IncreaseBy(const SmToken &rToken, sal_uInt32 nMultiplyBy)
{
    switch(rToken.eType)
    {
        case TBLANK:  mnNum += (4 * nMultiplyBy); break;
        case TSBLANK: mnNum += (1 * nMultiplyBy); break;
        default:
            break;
    }
}

void SmBlankNode::Prepare(const SmFormat &rFormat, const SmDocShell &rDocShell, int nDepth)
{
    SmNode::Prepare(rFormat, rDocShell, nDepth);

    // Here it need/should not be the StarMath font, so that for the character
    // used in Arrange a normal (non-clipped) rectangle is generated
    GetFont() = rFormat.GetFont(FNT_VARIABLE);

    Flags() |= FontChangeMask::Face | FontChangeMask::Bold | FontChangeMask::Italic;
}


void SmBlankNode::Arrange(OutputDevice &rDev, const SmFormat &rFormat)
{
    SmTmpDevice aTmpDev (rDev, true);
    aTmpDev.SetFont(GetFont());

    // make distance depend on the font height
    // (so that it increases when scaling (e.g. size *2 {a ~ b})
    tools::Long  nDist  = GetFont().GetFontSize().Height() / 10,
          nSpace = mnNum * nDist;

    // get a SmRect with Baseline and all the bells and whistles
    SmRect::operator = (SmRect(aTmpDev, &rFormat, OUString(' '),
                               GetFont().GetBorderWidth()));

    // and resize it to the requested size
    SetItalicSpaces(0, 0);
    SetWidth(nSpace);
}

/**************************************************************************/
//Implementation of all accept methods for SmVisitor

void SmTableNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmBraceNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmBracebodyNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmOperNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmAlignNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmAttributeNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmFontNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmUnHorNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmBinHorNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmBinVerNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmBinDiagonalNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmSubSupNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmMatrixNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmPlaceNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmTextNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmSpecialNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmGlyphSpecialNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmMathSymbolNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmBlankNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmErrorNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmLineNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmExpressionNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmPolyLineNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmRootNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmRootSymbolNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmRectangleNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

void SmVerticalBraceNode::Accept(SmVisitor* pVisitor) {
    pVisitor->Visit(this);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
