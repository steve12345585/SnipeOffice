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


#include <expressionnodefactory.hxx>

#include <algorithm>
#include <utility>


/* Implementation of ExpressionNodeFactory class */

namespace slideshow::internal
{
        namespace
        {
            class ConstantValueExpression : public ExpressionNode
            {
            public:
                explicit ConstantValueExpression( double rValue ) :
                    maValue( rValue )
                {
                }

                virtual double operator()( double /*t*/ ) const override
                {
                    return maValue;
                }

                virtual bool isConstant() const override
                {
                    return true;
                }

            private:
                double  maValue;
            };

            class TValueExpression : public ExpressionNode
            {
            public:
                TValueExpression()
                {
                }

                virtual double operator()( double t ) const override
                {
                    return t;
                }

                virtual bool isConstant() const override
                {
                    return false;
                }
            };

            /** Base class for following binary functions (*+-/)

                Does not pay off to have all this as a template, since
                we'd have to hold the functor as a member (+33% object
                size).
             */
            class BinaryExpressionBase : public ExpressionNode
            {
            public:
                BinaryExpressionBase( std::shared_ptr<ExpressionNode> pFirstArg,
                                      std::shared_ptr<ExpressionNode> pSecondArg ) :
                    mpFirstArg(std::move( pFirstArg )),
                    mpSecondArg(std::move( pSecondArg ))
                {
                }

                virtual bool isConstant() const override
                {
                    return
                        mpFirstArg->isConstant() &&
                        mpSecondArg->isConstant();
                }

            protected:
                std::shared_ptr<ExpressionNode> mpFirstArg;
                std::shared_ptr<ExpressionNode> mpSecondArg;
            };

            class PlusExpression : public BinaryExpressionBase
            {
            public:
                PlusExpression( const std::shared_ptr<ExpressionNode>&  rFirstArg,
                                const std::shared_ptr<ExpressionNode>&  rSecondArg ) :
                    BinaryExpressionBase( rFirstArg, rSecondArg )
                {
                }

                virtual double operator()( double t ) const override
                {
                    return (*mpFirstArg)(t) + (*mpSecondArg)(t);
                }
            };

            class MinusExpression : public BinaryExpressionBase
            {
            public:
                MinusExpression( const std::shared_ptr<ExpressionNode>& rFirstArg,
                                 const std::shared_ptr<ExpressionNode>& rSecondArg ) :
                    BinaryExpressionBase( rFirstArg, rSecondArg )
                {
                }

                virtual double operator()( double t ) const override
                {
                    return (*mpFirstArg)(t) - (*mpSecondArg)(t);
                }
            };

            class MultipliesExpression : public BinaryExpressionBase
            {
            public:
                MultipliesExpression( const std::shared_ptr<ExpressionNode>&    rFirstArg,
                                      const std::shared_ptr<ExpressionNode>&    rSecondArg ) :
                    BinaryExpressionBase( rFirstArg, rSecondArg )
                {
                }

                virtual double operator()( double t ) const override
                {
                    return (*mpFirstArg)(t) * (*mpSecondArg)(t);
                }
            };

            class DividesExpression : public BinaryExpressionBase
            {
            public:
                DividesExpression( const std::shared_ptr<ExpressionNode>&   rFirstArg,
                                   const std::shared_ptr<ExpressionNode>&   rSecondArg ) :
                    BinaryExpressionBase( rFirstArg, rSecondArg )
                {
                }

                virtual double operator()( double t ) const override
                {
                    return (*mpFirstArg)(t) / (*mpSecondArg)(t);
                }
            };

            class MinExpression : public BinaryExpressionBase
            {
            public:
                MinExpression( const std::shared_ptr<ExpressionNode>&   rFirstArg,
                               const std::shared_ptr<ExpressionNode>&   rSecondArg ) :
                    BinaryExpressionBase( rFirstArg, rSecondArg )
                {
                }

                virtual double operator()( double t ) const override
                {
                    return ::std::min( (*mpFirstArg)(t), (*mpSecondArg)(t) );
                }
            };

            class MaxExpression : public BinaryExpressionBase
            {
            public:
                MaxExpression( const std::shared_ptr<ExpressionNode>&   rFirstArg,
                               const std::shared_ptr<ExpressionNode>&   rSecondArg ) :
                    BinaryExpressionBase( rFirstArg, rSecondArg )
                {
                }

                virtual double operator()( double t ) const override
                {
                    return ::std::max( (*mpFirstArg)(t), (*mpSecondArg)(t) );
                }
            };
        }

        std::shared_ptr<ExpressionNode> ExpressionNodeFactory::createConstantValueExpression( double rConstantValue )
        {
            return std::make_shared<ConstantValueExpression>(rConstantValue);
        }

        std::shared_ptr<ExpressionNode> ExpressionNodeFactory::createValueTExpression()
        {
            return std::make_shared<TValueExpression>();
        }

        std::shared_ptr<ExpressionNode> ExpressionNodeFactory::createPlusExpression( const std::shared_ptr<ExpressionNode>& rLHS,
                                                                             const std::shared_ptr<ExpressionNode>& rRHS )
        {
            return std::make_shared<PlusExpression>(rLHS, rRHS);
        }

        std::shared_ptr<ExpressionNode> ExpressionNodeFactory::createMinusExpression( const std::shared_ptr<ExpressionNode>&    rLHS,
                                                                              const std::shared_ptr<ExpressionNode>&    rRHS )
        {
            return std::make_shared<MinusExpression>(rLHS, rRHS);
        }

        std::shared_ptr<ExpressionNode> ExpressionNodeFactory::createMultipliesExpression( const std::shared_ptr<ExpressionNode>&   rLHS,
                                                                                   const std::shared_ptr<ExpressionNode>&   rRHS )
        {
            return std::make_shared<MultipliesExpression>(rLHS, rRHS);
        }

        std::shared_ptr<ExpressionNode> ExpressionNodeFactory::createDividesExpression( const std::shared_ptr<ExpressionNode>&  rLHS,
                                                                                const std::shared_ptr<ExpressionNode>&  rRHS )
        {
            return std::make_shared<DividesExpression>(rLHS, rRHS);
        }

        std::shared_ptr<ExpressionNode> ExpressionNodeFactory::createMinExpression   ( const std::shared_ptr<ExpressionNode>&   rOuterFunction,
                                                                               const std::shared_ptr<ExpressionNode>&   rInnerFunction )
        {
            return std::make_shared<MinExpression>(rOuterFunction, rInnerFunction);
        }

        std::shared_ptr<ExpressionNode> ExpressionNodeFactory::createMaxExpression   ( const std::shared_ptr<ExpressionNode>&   rOuterFunction,
                                                                               const std::shared_ptr<ExpressionNode>&   rInnerFunction )
        {
            return std::make_shared<MaxExpression>(rOuterFunction, rInnerFunction);
        }

}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
