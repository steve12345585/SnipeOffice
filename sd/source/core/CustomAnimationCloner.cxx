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

#include <com/sun/star/animations/XAnimate.hpp>
#include <com/sun/star/animations/XAnimationNode.hpp>
#include <com/sun/star/animations/Event.hpp>
#include <com/sun/star/animations/XCommand.hpp>
#include <com/sun/star/animations/XIterateContainer.hpp>
#include <com/sun/star/animations/XAudio.hpp>
#include <com/sun/star/animations/AnimationNodeType.hpp>
#include <com/sun/star/animations/ValuePair.hpp>
#include <com/sun/star/util/XCloneable.hpp>
#include <com/sun/star/presentation/ParagraphTarget.hpp>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/beans/NamedValue.hpp>

#include <map>

#include <tools/debug.hxx>
#include <comphelper/diagnose_ex.hxx>
#include <animations/animationnodehelper.hxx>

#include <svx/svditer.hxx>

#include <CustomAnimationCloner.hxx>
#include <sdpage.hxx>

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::animations;
using namespace ::com::sun::star::presentation;
using namespace ::com::sun::star::container;

using ::com::sun::star::drawing::XShape;
using ::com::sun::star::beans::NamedValue;

namespace sd
{
    namespace {

    class CustomAnimationClonerImpl
    {
    public:
        CustomAnimationClonerImpl();
        Reference< XAnimationNode > Clone( const Reference< XAnimationNode >& xSourceNode, const SdPage* pSource, const SdPage* pTarget );

    private:
        void transformNode( const Reference< XAnimationNode >& xNode );
        Any transformValue( const Any& rValue );

        Reference< XShape > getClonedShape( const Reference< XShape >& xSource ) const;
        Reference< XAnimationNode > getClonedNode( const Reference< XAnimationNode >& xSource ) const;

        mutable ::std::map< Reference< XShape >, Reference< XShape > > maShapeMap;
        std::vector< Reference< XAnimationNode > > maSourceNodeVector;
        std::vector< Reference< XAnimationNode > > maCloneNodeVector;
    };

    }

    CustomAnimationClonerImpl::CustomAnimationClonerImpl()
    {
    }

    Reference< XAnimationNode > Clone( const Reference< XAnimationNode >& xSourceNode, const SdPage* pSource, const SdPage* pTarget )
    {
        CustomAnimationClonerImpl aCloner;
        return aCloner.Clone( xSourceNode, pSource, pTarget );
    }

    Reference< XAnimationNode > CustomAnimationClonerImpl::Clone( const Reference< XAnimationNode >& xSourceNode, const SdPage* pSourcePage, const SdPage* pTargetPage )
    {
        try
        {
            // clone animation hierarchy
            Reference< css::util::XCloneable > xClonable( xSourceNode, UNO_QUERY_THROW );
            Reference< XAnimationNode > xCloneNode( xClonable->createClone(), UNO_QUERY_THROW );

            // create a dictionary to map source to cloned shapes
            if( pSourcePage && pTargetPage )
            {
                SdrObjListIter aSourceIter( pSourcePage, SdrIterMode::DeepWithGroups );
                SdrObjListIter aTargetIter( pTargetPage, SdrIterMode::DeepWithGroups );

                while( aSourceIter.IsMore() && aTargetIter.IsMore() )
                {
                    SdrObject* pSource = aSourceIter.Next();
                    SdrObject* pTarget = aTargetIter.Next();

                    if( pSource && pTarget)
                    {
                        Reference< XShape > xSource( pSource->getUnoShape(), UNO_QUERY );
                        Reference< XShape > xTarget( pTarget->getUnoShape(), UNO_QUERY );
                        if( xSource.is() && xTarget.is() )
                        {
                            maShapeMap[xSource] = std::move(xTarget);
                        }
                    }
                }
            }

            // create a dictionary to map source to cloned nodes
            ::anim::create_deep_vector( xSourceNode, maSourceNodeVector );
            ::anim::create_deep_vector( xCloneNode, maCloneNodeVector );

            transformNode( xCloneNode );

            return xCloneNode;
        }
        catch( Exception& )
        {
            TOOLS_WARN_EXCEPTION( "sd", "sd::CustomAnimationClonerImpl::Clone()" );
            Reference< XAnimationNode > xEmpty;
            return xEmpty;
        }
    }

    void CustomAnimationClonerImpl::transformNode( const Reference< XAnimationNode >& xNode )
    {
        try
        {
            xNode->setBegin( transformValue( xNode->getBegin() ) );
            xNode->setEnd( transformValue( xNode->getEnd() ) );

            sal_Int16 nNodeType( xNode->getType() );
            switch( nNodeType )
            {
            case AnimationNodeType::ITERATE:
            {
                Reference< XIterateContainer > xIter( xNode, UNO_QUERY_THROW );
                xIter->setTarget( transformValue( xIter->getTarget() ) );
                [[fallthrough]];
            }
            case AnimationNodeType::PAR:
            case AnimationNodeType::SEQ:
            {
                Reference< XEnumerationAccess > xEnumerationAccess( xNode, UNO_QUERY_THROW );
                Reference< XEnumeration > xEnumeration( xEnumerationAccess->createEnumeration(), UNO_SET_THROW );
                while( xEnumeration->hasMoreElements() )
                {
                    Reference< XAnimationNode > xChildNode( xEnumeration->nextElement(), UNO_QUERY_THROW );
                    transformNode( xChildNode );
                }
            }
            break;

            case AnimationNodeType::ANIMATE:
            case AnimationNodeType::SET:
            case AnimationNodeType::ANIMATEMOTION:
            case AnimationNodeType::ANIMATEPHYSICS:
            case AnimationNodeType::ANIMATECOLOR:
            case AnimationNodeType::ANIMATETRANSFORM:
            case AnimationNodeType::TRANSITIONFILTER:
            {
                Reference< XAnimate > xAnimate( xNode, UNO_QUERY_THROW );
                xAnimate->setTarget( transformValue( xAnimate->getTarget() ) );
            }
            break;

            case AnimationNodeType::COMMAND:
            {
                Reference< XCommand > xCommand( xNode, UNO_QUERY_THROW );
                xCommand->setTarget( transformValue( xCommand->getTarget() ) );
            }
            break;

            case AnimationNodeType::AUDIO:
            {
                Reference< XAudio > xAudio( xNode, UNO_QUERY_THROW );
                xAudio->setSource( transformValue( xAudio->getSource() ) );
            }
            break;
            }

            Sequence< NamedValue > aUserData( xNode->getUserData() );
            if( aUserData.hasElements() )
            {
                for( NamedValue & namedValue : asNonConstRange(aUserData) )
                {
                    namedValue.Value = transformValue( namedValue.Value );
                }

                xNode->setUserData( aUserData );
            }
        }
        catch( Exception& )
        {
            TOOLS_WARN_EXCEPTION( "sd", "sd::CustomAnimationClonerImpl::transformNode()" );
        }
    }

    Any CustomAnimationClonerImpl::transformValue( const Any& rValue )
    {
        if( rValue.hasValue() ) try
        {
            if( rValue.getValueType() == cppu::UnoType<ValuePair>::get() )
            {
                ValuePair aValuePair;
                rValue >>= aValuePair;

                aValuePair.First = transformValue( aValuePair.First );
                aValuePair.Second = transformValue( aValuePair.Second );

                return Any( aValuePair );
            }
            else if( rValue.getValueType() == cppu::UnoType< Sequence<Any> >::get() )
            {
                Sequence<Any> aSequence;
                rValue >>= aSequence;

                for( Any& rAny : asNonConstRange(aSequence) )
                    rAny = transformValue( rAny );

                return Any( aSequence );
            }
            else if( rValue.getValueTypeClass() == TypeClass_INTERFACE )
            {
                Reference< XShape > xShape;
                rValue >>= xShape;
                if( xShape.is() )
                {
                    return Any( getClonedShape( xShape ) );
                }
                else
                {
                    Reference< XAnimationNode > xNode;
                    rValue >>= xNode;
                    if( xNode.is() )
                        return Any( getClonedNode( xNode ) );
                }
            }
            else if( rValue.getValueType() == cppu::UnoType<ParagraphTarget>::get() )
            {
                ParagraphTarget aParaTarget;
                rValue >>= aParaTarget;

                aParaTarget.Shape = getClonedShape( aParaTarget.Shape );

                return Any( aParaTarget );
            }
            else if( rValue.getValueType() == cppu::UnoType<Event>::get() )
            {
                Event aEvent;
                rValue >>= aEvent;

                aEvent.Source = transformValue( aEvent.Source );

                return Any( aEvent );
            }
        }
        catch( Exception& )
        {
            TOOLS_WARN_EXCEPTION( "sd", "sd::CustomAnimationClonerImpl::transformValue()" );
        }

        return rValue;
    }

    Reference< XShape > CustomAnimationClonerImpl::getClonedShape( const Reference< XShape >& xSource ) const
    {
        if( xSource.is() )
        {
            if( maShapeMap.contains(xSource) )
            {
                return maShapeMap[xSource];
            }

            DBG_ASSERT( maShapeMap.empty(), "sd::CustomAnimationClonerImpl::getClonedShape() failed!" );
        }
        return xSource;
    }

    Reference< XAnimationNode > CustomAnimationClonerImpl::getClonedNode( const Reference< XAnimationNode >& xSource ) const
    {
        std::size_t nNodeCount = maSourceNodeVector.size();
        std::size_t nCloneNodeCount = maCloneNodeVector.size();

        if (nNodeCount != nCloneNodeCount)
            SAL_WARN("sd.core", "Sizes of maSourceNodeVector and maCloneNodeVector mismatch!");

        for( std::size_t nNode = 0; nNode < nNodeCount && nNode < nCloneNodeCount; ++nNode )
        {
            if( maSourceNodeVector[nNode] == xSource )
                return maCloneNodeVector[nNode];
        }

        OSL_FAIL( "sd::CustomAnimationClonerImpl::getClonedNode() failed!" );
        return xSource;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
