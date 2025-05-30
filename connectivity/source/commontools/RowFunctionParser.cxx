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


// Makes parser a static resource,
// we're synchronized externally.
// But watch out, the parser might have
// state not visible to this code!
#define BOOST_SPIRIT_SINGLE_GRAMMAR_INSTANCE

#if OSL_DEBUG_LEVEL >= 2 && defined(DBG_UTIL)
#include <typeinfo>
#define BOOST_SPIRIT_DEBUG
#endif
#include <boost/spirit/include/classic_core.hpp>
#include <RowFunctionParser.hxx>
#include <rtl/ustring.hxx>


#if (OSL_DEBUG_LEVEL > 0)
#include <iostream>
#endif
#include <algorithm>
#include <stack>
#include <utility>

namespace connectivity
{
using namespace com::sun::star;

namespace
{


// EXPRESSION NODES


class ConstantValueExpression : public ExpressionNode
{
    ORowSetValueDecoratorRef maValue;

public:

    explicit ConstantValueExpression(ORowSetValueDecoratorRef aValue)
        : maValue(std::move(aValue))
    {
    }
    virtual ORowSetValueDecoratorRef evaluate(const ODatabaseMetaDataResultSet::ORow& /*_aRow*/ ) const override
    {
        return maValue;
    }
    virtual void fill(const ODatabaseMetaDataResultSet::ORow& /*_aRow*/ ) const override
    {
    }
};


/** ExpressionNode implementation for unary
    function over two ExpressionNodes
    */
class BinaryFunctionExpression : public ExpressionNode
{
    const ExpressionFunct   meFunct;
    std::shared_ptr<ExpressionNode> mpFirstArg;
    std::shared_ptr<ExpressionNode> mpSecondArg;

public:

    BinaryFunctionExpression( const ExpressionFunct eFunct, std::shared_ptr<ExpressionNode> xFirstArg, std::shared_ptr<ExpressionNode> xSecondArg ) :
        meFunct( eFunct ),
        mpFirstArg(std::move( xFirstArg )),
        mpSecondArg(std::move( xSecondArg ))
    {
    }
    virtual ORowSetValueDecoratorRef evaluate(const ODatabaseMetaDataResultSet::ORow& _aRow ) const override
    {
        ORowSetValueDecoratorRef aRet;
        switch(meFunct)
        {
            case ExpressionFunct::Equation:
                aRet = new ORowSetValueDecorator( ORowSetValue(mpFirstArg->evaluate(_aRow )->getValue() == mpSecondArg->evaluate(_aRow )->getValue()) );
                break;
            case ExpressionFunct::And:
                aRet = new ORowSetValueDecorator( ORowSetValue(mpFirstArg->evaluate(_aRow )->getValue().getBool() && mpSecondArg->evaluate(_aRow )->getValue().getBool()) );
                break;
            case ExpressionFunct::Or:
                aRet = new ORowSetValueDecorator( ORowSetValue(mpFirstArg->evaluate(_aRow )->getValue().getBool() || mpSecondArg->evaluate(_aRow )->getValue().getBool()) );
                break;
            default:
                break;
        }
        return aRet;
    }
    virtual void fill(const ODatabaseMetaDataResultSet::ORow& _aRow ) const override
    {
        switch(meFunct)
        {
            case ExpressionFunct::Equation:
                (*mpFirstArg->evaluate(_aRow )) = mpSecondArg->evaluate(_aRow )->getValue();
                break;
            default:
                break;
        }
    }
};


// FUNCTION PARSER


typedef const char* StringIteratorT;

struct ParserContext
{
    typedef std::stack< std::shared_ptr<ExpressionNode> > OperandStack;

    // stores a stack of not-yet-evaluated operands. This is used
    // by the operators (i.e. '+', '*', 'sin' etc.) to pop their
    // arguments from. If all arguments to an operator are constant,
    // the operator pushes a precalculated result on the stack, and
    // a composite ExpressionNode otherwise.
    OperandStack                            maOperandStack;
};

typedef std::shared_ptr< ParserContext > ParserContextSharedPtr;

/** Generate apriori constant value
    */

class ConstantFunctor
{
    ParserContextSharedPtr          mpContext;

public:

    explicit ConstantFunctor( ParserContextSharedPtr xContext ) :
        mpContext(std::move( xContext ))
    {
    }
    void operator()( StringIteratorT rFirst,StringIteratorT rSecond) const
    {
        OUString sVal( rFirst, rSecond - rFirst, RTL_TEXTENCODING_UTF8 );
        mpContext->maOperandStack.push(std::make_shared<ConstantValueExpression>(ORowSetValueDecoratorRef(new ORowSetValueDecorator(sVal))));
    }
};

/** Generate parse-dependent-but-then-constant value
    */
class IntConstantFunctor
{
    ParserContextSharedPtr  mpContext;

public:
    explicit IntConstantFunctor( ParserContextSharedPtr xContext ) :
        mpContext(std::move( xContext ))
    {
    }
    void operator()( sal_Int32 n ) const
    {
        mpContext->maOperandStack.push(std::make_shared<ConstantValueExpression>(ORowSetValueDecoratorRef(new ORowSetValueDecorator(n))));
    }
};

/** Implements a binary function over two ExpressionNodes

    @tpl Generator
    Generator functor, to generate an ExpressionNode of
    appropriate type

    */
class BinaryFunctionFunctor
{
    const ExpressionFunct   meFunct;
    ParserContextSharedPtr  mpContext;

public:

    BinaryFunctionFunctor( const ExpressionFunct eFunct, ParserContextSharedPtr xContext ) :
        meFunct( eFunct ),
        mpContext(std::move( xContext ))
    {
    }

    void operator()( StringIteratorT, StringIteratorT ) const
    {
        ParserContext::OperandStack& rNodeStack( mpContext->maOperandStack );

        if( rNodeStack.size() < 2 )
            throw ParseError( "Not enough arguments for binary operator" );

        // retrieve arguments
        std::shared_ptr<ExpressionNode> pSecondArg( std::move(rNodeStack.top()) );
        rNodeStack.pop();
        std::shared_ptr<ExpressionNode> pFirstArg( std::move(rNodeStack.top()) );
        rNodeStack.pop();

        // create combined ExpressionNode
        auto pNode = std::make_shared<BinaryFunctionExpression>( meFunct, pFirstArg, pSecondArg );
        // check for constness
        rNodeStack.push( pNode );
    }
};
/** ExpressionNode implementation for unary
    function over one ExpressionNode
    */
class UnaryFunctionExpression : public ExpressionNode
{
    std::shared_ptr<ExpressionNode> mpArg;

public:
    explicit UnaryFunctionExpression( std::shared_ptr<ExpressionNode> xArg ) :
        mpArg(std::move( xArg ))
    {
    }
    virtual ORowSetValueDecoratorRef evaluate(const ODatabaseMetaDataResultSet::ORow& _aRow ) const override
    {
        return _aRow[mpArg->evaluate(_aRow )->getValue().getUInt32()];
    }
    virtual void fill(const ODatabaseMetaDataResultSet::ORow& /*_aRow*/ ) const override
    {
    }
};

class UnaryFunctionFunctor
{
    ParserContextSharedPtr  mpContext;

public:

    explicit UnaryFunctionFunctor(ParserContextSharedPtr xContext)
        : mpContext(std::move(xContext))
    {
    }
    void operator()( StringIteratorT, StringIteratorT ) const
    {

        ParserContext::OperandStack& rNodeStack( mpContext->maOperandStack );

        if( rNodeStack.empty() )
            throw ParseError( "Not enough arguments for unary operator" );

        // retrieve arguments
        std::shared_ptr<ExpressionNode> pArg( std::move(rNodeStack.top()) );
        rNodeStack.pop();

        rNodeStack.push( std::make_shared<UnaryFunctionExpression>( pArg ) );
    }
};

/* This class implements the following grammar (more or
    less literally written down below, only slightly
    obfuscated by the parser actions):

    basic_expression =
                       number |
                       '(' additive_expression ')'

    unary_expression =
                    basic_expression

    multiplicative_expression =
                       unary_expression ( ( '*' unary_expression )* |
                                        ( '/' unary_expression )* )

    additive_expression =
                       multiplicative_expression ( ( '+' multiplicative_expression )* |
                                                   ( '-' multiplicative_expression )* )

    */
class ExpressionGrammar : public ::boost::spirit::classic::grammar< ExpressionGrammar >
{
public:
    /** Create an arithmetic expression grammar

        @param rParserContext
        Contains context info for the parser
        */
    explicit ExpressionGrammar( ParserContextSharedPtr xParserContext ) :
        mpParserContext(std::move( xParserContext ))
    {
    }

    template< typename ScannerT > class definition
    {
    public:
        // grammar definition
        explicit definition( const ExpressionGrammar& self )
        {
            using ::boost::spirit::classic::space_p;
            using ::boost::spirit::classic::range_p;
            using ::boost::spirit::classic::lexeme_d;
            using ::boost::spirit::classic::ch_p;
            using ::boost::spirit::classic::int_p;
            using ::boost::spirit::classic::as_lower_d;
            using ::boost::spirit::classic::strlit;
            using ::boost::spirit::classic::inhibit_case;


            typedef inhibit_case<strlit<> > token_t;
            token_t COLUMN  = as_lower_d[ "column" ];
            token_t OR_     = as_lower_d[ "or" ];
            token_t AND_    = as_lower_d[ "and" ];

            integer =
                    int_p
                                [IntConstantFunctor(self.getContext())];

            argument =
                    integer
                |    lexeme_d[ +( range_p('a','z') | range_p('A','Z') | range_p('0','9') ) ]
                                [ ConstantFunctor(self.getContext()) ]
               ;

            unaryFunction =
                    (COLUMN >> '(' >> integer >> ')' )
                                [ UnaryFunctionFunctor( self.getContext()) ]
                ;

            assignment =
                    unaryFunction >> ch_p('=') >> argument
                                [ BinaryFunctionFunctor( ExpressionFunct::Equation,  self.getContext()) ]
               ;

            andExpression =
                    assignment
                |   ( '(' >> orExpression >> ')' )
                |   ( assignment >> AND_ >> assignment )  [ BinaryFunctionFunctor( ExpressionFunct::And,  self.getContext()) ]
                ;

            orExpression =
                    andExpression
                |   ( orExpression >> OR_ >> andExpression ) [ BinaryFunctionFunctor( ExpressionFunct::Or,  self.getContext()) ]
                ;

            basicExpression =
                    orExpression
                ;

            BOOST_SPIRIT_DEBUG_RULE(basicExpression);
            BOOST_SPIRIT_DEBUG_RULE(unaryFunction);
            BOOST_SPIRIT_DEBUG_RULE(assignment);
            BOOST_SPIRIT_DEBUG_RULE(argument);
            BOOST_SPIRIT_DEBUG_RULE(integer);
            BOOST_SPIRIT_DEBUG_RULE(orExpression);
            BOOST_SPIRIT_DEBUG_RULE(andExpression);
        }

        const ::boost::spirit::classic::rule< ScannerT >& start() const
        {
            return basicExpression;
        }

    private:
        // the constituents of the Spirit arithmetic expression grammar.
        // For the sake of readability, without 'ma' prefix.
        ::boost::spirit::classic::rule< ScannerT >   basicExpression;
        ::boost::spirit::classic::rule< ScannerT >   unaryFunction;
        ::boost::spirit::classic::rule< ScannerT >   assignment;
        ::boost::spirit::classic::rule< ScannerT >   integer,argument;
        ::boost::spirit::classic::rule< ScannerT >   orExpression,andExpression;
    };

    const ParserContextSharedPtr& getContext() const
    {
        return mpParserContext;
    }

private:
    ParserContextSharedPtr          mpParserContext; // might get modified during parsing
};

const ParserContextSharedPtr& getParserContext()
{
    static ParserContextSharedPtr lcl_parserContext = std::make_shared<ParserContext>();

    // clear node stack (since we reuse the static object, that's
    // the whole point here)
    while( !lcl_parserContext->maOperandStack.empty() )
        lcl_parserContext->maOperandStack.pop();

    return lcl_parserContext;
}

}

std::shared_ptr<ExpressionNode> const & FunctionParser::parseFunction( const OUString& _sFunction)
{
    // TODO(Q1): Check if a combination of the RTL_UNICODETOTEXT_FLAGS_*
    // gives better conversion robustness here (we might want to map space
    // etc. to ASCII space here)
    const OString aAsciiFunction(
        OUStringToOString( _sFunction, RTL_TEXTENCODING_ASCII_US ) );

    StringIteratorT aStart( aAsciiFunction.getStr() );
    StringIteratorT aEnd( aAsciiFunction.getStr()+aAsciiFunction.getLength() );

    // static parser context, because the actual
    // Spirit parser is also a static object
    const ParserContextSharedPtr& pContext = getParserContext();

    ExpressionGrammar aExpressionGrammer( pContext );

    const ::boost::spirit::classic::parse_info<StringIteratorT> aParseInfo(
            ::boost::spirit::classic::parse( aStart,
                                    aEnd,
                                    aExpressionGrammer,
                                    ::boost::spirit::classic::space_p ) );

#if (OSL_DEBUG_LEVEL > 0)
    std::cout.flush(); // needed to keep stdout and cout in sync
#endif

    // input fully congested by the parser?
    if( !aParseInfo.full )
        throw ParseError( "RowFunctionParser::parseFunction(): string not fully parseable" );

    // parser's state stack now must contain exactly _one_ ExpressionNode,
    // which represents our formula.
    if( pContext->maOperandStack.size() != 1 )
        throw ParseError( "RowFunctionParser::parseFunction(): incomplete or empty expression" );

    return pContext->maOperandStack.top();
}
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
