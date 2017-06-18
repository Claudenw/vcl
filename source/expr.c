/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/*
    Copyright (c) 1989-1993
    by Todd R. Hill
    All Rights Reserved
*/

/*unpubModule*****************************************************************
 NAME
    expr.c - Expression handling

 DESCRIPTION
    Expression evaluation routines.  Combined with the routines in primary.c
    these functions handle evaluating (parenthetical) expressions.  Operator
    precedence is enforced by the cascaded order of calling the functions.

    Highest precedence                                Associativity:
        ()  []  ->  ::  .                             left to right
        !   ~   +   -   ++  --  &  *  sizeof          right to left
        .*  ->*                                       left to right
        *   /   %                                     left to right
        +   -                                         left to right
        <<  >>                                        left to right
        <   <=   >   >=                               left to right
        ==  !=                                        left to right
        &                                             left to right
        ^                                             left to right
        |                                             left to right
        &&                                            left to right
        ||                                            right to left
        ?:                                            left to right
        =  *=  /=  %=  +=  -=  &=  ^=  |=  <<=  >>=   right to left
        ,                                             left to right
    Lowest precedence


 FUNCTIONS
    ExpressionOne()
    expression()                        count comma-separated subexpressions
    cond()                              ternary ?:
    assgn()                             assignment =, +=, ...
    logic1()                            logical OR ||
    logic2()                            logical AND &&
    bool1()                             binary OR |
    bool2()                             binary XOR ^
    bool3()                             binary AND &
    reln1()                             ==, !=
    reln2()                             <=, >=, <, >
    shift()                             <<, >>
    add()                               +, -
    mult()                              *, /m %

 STATIC
    binarySkip()                        Skip next expression based on function
    checkInteger()                      Check for integer type on stack
    catCheck()                          Check for illegal pointer operation
    compareItems()                      Setup a compare of 2 stack items
    skipExpr()                          Incr SkipExpression, skip, decr

 FILES
    vcldef.h

 NOTES

*****************************************************************unpubModule*/

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

int
VCLCLASS ExpressionOne (void)
{
    int             argc;
    ITEM            top = *Ctx.Stackptr;

    if ( ( argc = expression() ) > 1 )
    {
        /*-
         * an expression statement like:
         *      ++argv, --argc, ++k;
         * leaves the right-most expression
         * (k) on the stack when it's done.
         */
        popn( argc );
        *++Ctx.Stackptr = top;
        argc = 1;
    }
    return argc;
} /* ExpressionOne */


/*
 * Parse a comma-separated expression.  Returns a count of
 * the number of subexpressions.
 */
int
VCLCLASS expression (void)
{
    ITEM *          sp;
#ifdef __DOS    
    unsigned        stat87;
#endif

    sp = Ctx.Stackptr;

    cond();

    while ( Ctx.Token == T_COMMA )
    {
        if ( Ctx.Stackptr - sp )
        {
            getoken();
            if ( Ctx.Token == T_SEMICOLON )
                error( COMMAERR );
            else
                cond();
        }
        else
            error( COMMAERR );
    }

#ifdef __DOS
    stat87 = _status87();
    if ( stat87 & 0x5d )
    {
        _clear87();
        error( MATHERR );
    }
#endif
    return (int) ( Ctx.Stackptr - sp );
} /* expression */


/*
 * Parse a conditional expression; <expr> ? <expr> : <expr>
 */
void
VCLCLASS cond (void)
{
    assgn();

    if ( Ctx.Token == T_COND )
    {
        getoken();
        if ( SkipExpression )
        {
            cond();
            if ( Ctx.Token != T_COLON )
                error( COLONERR );
            getoken();
            cond();
            return;
        }
        else if ( popint() )
        {
            cond();
            if ( Ctx.Token == T_COLON )
            {
                getoken();
                skipExpr( ADDRVCLCLASS cond );
            }
            else
                error( COLONERR );
        }
        else
        {
            skipExpr( ADDRVCLCLASS cond );
            if ( Ctx.Token == T_COLON )
            {
                getoken();
                cond();
            }
            else
                error( COLONERR );
        }
    }
} /* cond */


/*
 * Parse an assignment expression; =, +=, -=, *=, etc.
 */
void
VCLCLASS assgn (void)
{
    logic1();

    /*
     * if there's an '=', perform the assignment
     */
    if ( Ctx.Token == T_ASSIGN )
    {
        getoken();
        cond();
        if ( ! SkipExpression )
            assignment();
    }
    else if ( Ctx.Token & OPASSIGN )
    {
        /*
         * It's an <op-assignment> operator.
         */
        unsigned char   svtoken;
        unsigned char   optoken;
        ITEM *          svstackptr;

        if ( SkipExpression )
        {
            getoken();
            cond();
            return;
        }

        optoken = Ctx.Token & 127;

        /*
         * We're going to be fiddling with the stack, so keep track of the
         * current top of stack then duplicate the item that's currently on
         * top.
         */
        svstackptr = Ctx.Stackptr;
        topdup();

        /*
         * Now parse another expression as if nothing happened.
         */
        getoken();
        cond();

        /*
         * Traverse down through the operator handlers to the one that does
         * the <op> part of <op-assignment>'s
         */
        svtoken = Ctx.Token;
        Ctx.Token = optoken;

        /*-
         * Tell the operator handlers that we're doing an
         * <op-assignment> by setting the "opAssign" variable.
         * If for example we had parsed the expression:
         *    "var += 2";
         * the stack would look like:
         *     2    <- current top of stack (Ctx.Stackptr)
         *    var
         *    var   <- bottom of stack
         */
        ++opAssign;
        Ctx.Stackptr -= 2;
        bool1();
        --opAssign;
        Ctx.Token = svtoken;

        /*
         * Now do the assignment and restore the stack.
         */
        assignment();
        Ctx.Stackptr = svstackptr;
    }
} /* assgn */


/*
 * Logical OR: ||
 */
void
VCLCLASS logic1 (void)
{
    logic2();

    while ( Ctx.Token == T_LIOR )
    {
        if ( binarySkip( ADDRVCLCLASS logic2 ) )
            continue;
        getoken();
        if ( poplng() )
        {
            /*
             * Expression is already true - short circuit logic
             */
            skipExpr( ADDRVCLCLASS logic2 );
            pushint( 1, FALSE );
        }
        else
        {
//            ITEM *      svstackptr = Ctx.Stackptr;

            logic2();
//            if ( Ctx.Stackptr == svstackptr )
//                error( EXPRERR );
            pushint( ! ! poplng(), FALSE );
        }
    }
} /* logic1 */


/*
 * Logical AND: &&
 */
void
VCLCLASS logic2 (void)
{
    bool1();

    while ( Ctx.Token == T_LAND )
    {
        if ( binarySkip( ADDRVCLCLASS bool1 ) )
            continue;
        getoken();
        if ( poplng() )
        {
//            ITEM *      svstackptr = Ctx.Stackptr;

            bool1();
//            if ( Ctx.Stackptr == svstackptr )
//                error( EXPRERR );
            pushint( ! ! poplng(), FALSE );
        }
        else
        {
            /*
             * Expression is already false - skip to end of expression.
             */
            skipExpr( ADDRVCLCLASS bool1 );
            pushint( 0, FALSE );
        }
    }
} /* logic2 */


/*
 * Binary OR: |
 */
void
VCLCLASS bool1 (void)
{
    char            isu;
    PROMO           promol;
    PROMO           promor;
    char            typ;

    bool2();

    while ( Ctx.Token == T_IOR )
    {
//        ITEM *      svstackptr;

        checkInteger( Ctx.Stackptr );
        topgetpromo( &promol );
        if ( opAssign )
            Ctx.Token = 0;
        else
            getoken();
//        svstackptr = Ctx.Stackptr;
        bool2();
//        if ( Ctx.Stackptr == svstackptr )
//            error( EXPRERR );
        checkInteger( Ctx.Stackptr );
        topgetpromo( &promor );
        if ( ! SkipExpression )
        {
            Promote( &typ, &isu, promol, promor );
            /* simple typ check works because the operator requires
               integral types checked by the checkInteger() above */
            if ( typ == LONG )
                pushlng( poplng() | poplng(), isu );
            else
                pushint( popint() | popint(), isu );
        }
    }
} /* bool1 */


/*
 * Binary XOR: ^
 */
void
VCLCLASS bool2 (void)
{
    char            isu;
    PROMO           promol;
    PROMO           promor;
    char            typ;

    bool3();

    while ( Ctx.Token == T_XOR )
    {
//        ITEM *      svstackptr;

        checkInteger( Ctx.Stackptr );
        topgetpromo( &promol );
        if ( opAssign )
            Ctx.Token = 0;
        else
            getoken();
//        svstackptr = Ctx.Stackptr;
        bool3();
//        if ( Ctx.Stackptr == svstackptr )
//            error( EXPRERR );
        checkInteger( Ctx.Stackptr );
        topgetpromo( &promor );
        if ( ! SkipExpression )
        {
            Promote( &typ, &isu, promol, promor );
            if ( typ == LONG )
                pushlng( poplng() ^ poplng(), isu );
            else
                pushint( popint() ^ popint(), isu );
        }
    }
} /* bool2 */


/*
 * Binary AND: &
 */
void
VCLCLASS bool3 (void)
{
    char            isu;
    PROMO           promol;
    PROMO           promor;
    char            typ;

    reln1();

    while ( Ctx.Token == T_AND )
    {
//        ITEM *      svstackptr;

        checkInteger( Ctx.Stackptr );
        topgetpromo( &promol );
        if ( opAssign )
            Ctx.Token = 0;
        else
            getoken();
//        svstackptr = Ctx.Stackptr;
        reln1();
//        if ( Ctx.Stackptr == svstackptr )
//            error( EXPRERR );
        checkInteger( Ctx.Stackptr );
        topgetpromo( &promor );
        if ( ! SkipExpression )
        {
            Promote( &typ, &isu, promol, promor );
            if ( typ == LONG )
                pushlng( poplng() & poplng(), isu );
            else
                pushint( popint() & popint(), isu );
        }
    }
} /* bool3 */


/*
 * Equality operators: ==, !=
 */
void
VCLCLASS reln1 (void)
{
    char            ityp;
    PROMO           promol;
    PROMO           promor;
//    ITEM *          svstackptr;


    reln2();

    for ( ;; )
    {
        if ( Ctx.Token == T_EQ )
        {
            topgetpromo( &promol );
            getoken();
//            svstackptr = Ctx.Stackptr;
            reln2();
//            if ( Ctx.Stackptr == svstackptr )
//                error( EXPRERR );
            topgetpromo( &promor );
            if ( ! SkipExpression )
            {
                if ( ItemisInteger( &promol ) && ItemisInteger( &promor ) )
                    ityp = Promote( NULL, NULL, promol, promor );
                else
                    ityp = I_FLOAT;

                switch ( ityp )
                {
                    /* pop the unsigned types as one type larger than ityp */
                    case I_FLOAT:
                        pushint( (int) ( popflt() == popflt() ), FALSE );
                        break;
                    case I_UNSLONG:
                        pushint( (int) ( (ulong) popflt() == (ulong) popflt() ), FALSE );
                        break;
                    case I_LONG:
                        pushint( (int) ( poplng() == poplng() ), FALSE );
                        break;
                    case I_UNSINT:
                        pushint( (int) ( (uint) poplng() == (uint) poplng() ), FALSE );
                        break;
                    case I_INT:
                        pushint( (int) ( popint() == popint() ), FALSE );
                        break;
                }
            }
        }
        else if ( Ctx.Token == T_NE )
        {
            topgetpromo( &promol );
            getoken();
//            svstackptr = Ctx.Stackptr;
            reln2();
//            if ( Ctx.Stackptr == svstackptr )
//                error( EXPRERR );
            topgetpromo( &promor );
            if ( ! SkipExpression )
            {
                if ( ItemisInteger( &promol ) && ItemisInteger( &promor ) )
                    ityp = Promote( NULL, NULL, promol, promor );
                else
                    ityp = I_FLOAT;

                switch ( ityp )
                {
                    /* pop the unsigned types as one type larger than ityp */
                    case I_FLOAT:
                        pushint( (int) ( popflt() != popflt() ), FALSE );
                        break;
                    case I_UNSLONG:
                        pushint( (int) ( (ulong) popflt() != (ulong) popflt() ), FALSE );
                        break;
                    case I_LONG:
                        pushint( (int) ( poplng() != poplng() ), FALSE );
                        break;
                    case I_UNSINT:
                        pushint( (int) ( (uint) poplng() != (uint) poplng() ), FALSE );
                        break;
                    case I_INT:
                        pushint( (int) ( popint() != popint() ), FALSE );
                        break;
                }
            }
        }
        else
            return;
    }
} /* reln1 */


/*
 * Relational operators: <=, >=, <, >
 */
void
VCLCLASS reln2 (void)
{
    double          fval;

    shift();

    for ( ;; )
    {
        switch ( Ctx.Token )
        {
            case T_LE:
                switch ( compareItems( &fval ) )
                {
                    case 0:
                        pushint( (unsigned) fval <= (unsigned) popint(), FALSE );
                        break;
                    case 1:
                        pushint( (unsigned) fval <= popflt(), FALSE );
                        break;
                    case 2:
                        pushint( fval <= (unsigned) popint(), FALSE );
                        break;
                    case 3:
                        pushint( fval <= popflt(), FALSE );
                        break;
                }
                break;

            case T_GE:
                switch ( compareItems( &fval ) )
                {
                    case 0:
                        pushint( (unsigned) fval >= (unsigned) popint(), FALSE );
                        break;
                    case 1:
                        pushint( (unsigned) fval >= popflt(), FALSE );
                        break;
                    case 2:
                        pushint( fval >= (unsigned) popint(), FALSE );
                        break;
                    case 3:
                        pushint( fval >= popflt(), FALSE );
                        break;
                }
                break;

            case T_LT:
                switch ( compareItems( &fval ) )
                {
                    case 0:
                        pushint( (unsigned) fval < (unsigned) popint(), FALSE );
                        break;
                    case 1:
                        pushint( (unsigned) fval < popflt(), FALSE );
                        break;
                    case 2:
                        pushint( fval < (unsigned) popint(), FALSE );
                        break;
                    case 3:
                        pushint( fval < popflt(), FALSE );
                        break;
                }
                break;

            case T_GT:
                switch ( compareItems( &fval ) )
                {
                    case 0:
                        pushint( (unsigned) fval > (unsigned) popint(), FALSE );
                        break;
                    case 1:
                        pushint( (unsigned) fval > popflt(), FALSE );
                        break;
                    case 2:
                        pushint( fval > (unsigned) popint(), FALSE );
                        break;
                    case 3:
                        pushint( fval > popflt(), FALSE );
                        break;
                }
                break;
            default:
                return;
        }
    }
} /* reln2 */


/*
 * Arithmetic shift left and right operators: <<, >>
 */
void
VCLCLASS shift (void)
{
    char            isu;
    char            typ;
    ulong           uval;
    long            val;
//    ITEM *          svstackptr;

    add();

    for ( ;; )
    {
        switch ( Ctx.Token )
        {
            case T_SHL:
                checkInteger( Ctx.Stackptr );
                typ = Ctx.Stackptr->type;
                isu = Ctx.Stackptr->isunsigned;
                uval = val = poplng();
                if ( opAssign )
                {
                    ++Ctx.Stackptr;
                    Ctx.Token = 0;
                }
                else
                    getoken();
//                svstackptr = Ctx.Stackptr;
                add();
//                if ( Ctx.Stackptr == svstackptr )
//                    error( EXPRERR );
                checkInteger( Ctx.Stackptr );
                if ( ! SkipExpression )
                {
/*
 * Disable "conversion may loose significant digits" warning under Borland.
 * It complains about the "v << poplng()" right-side long additive expression.
 * You really can't shift something a long quantity number of bits, however,
 * we want to allow any integral type on the right side, so a long is required.
 */
#if BORLAND
#pragma warn -sig
#endif
                    if ( isu )
                    {
                        if ( typ == LONG )
                            pushlng( ( uval << poplng() ), isu );
                        else
                            pushint( (unsigned int) ( uval << poplng() ), isu );
                    }
                    else
                    {
                        if ( typ == LONG )
                            pushlng( ( val << poplng() ), isu );
                        else
                            pushint( (int) ( val << poplng() ), isu );
                    }
/* Reenable the Borland warning */
#if BORLAND
#pragma warn +sig
#endif
                }
                break;

            case T_SHR:
                checkInteger( Ctx.Stackptr );
                typ = Ctx.Stackptr->type;
                isu = Ctx.Stackptr->isunsigned;
                uval = val = poplng();
                if ( opAssign )
                {
                    ++Ctx.Stackptr;
                    Ctx.Token = 0;
                }
                else
                    getoken();
//                svstackptr = Ctx.Stackptr;
                add();
//                if ( Ctx.Stackptr == svstackptr )
//                    error( EXPRERR );
                checkInteger( Ctx.Stackptr );
                if ( ! SkipExpression )
                {
#if BORLAND
#pragma warn -sig
#endif
                    if ( isu )
                    {
                        if ( typ == LONG )
                            pushlng( ( uval >> poplng() ), isu );
                        else
                            pushint( (unsigned int) ( uval >> poplng() ), isu );
                    }
                    else
                    {
                        if ( typ == LONG )
                            pushlng( ( val >> poplng() ), isu );
                        else
                            pushint( (int) ( val >> poplng() ), isu );
                    }
#if BORLAND
#pragma warn +sig
#endif
                }
                break;
            default:
                return;
        }
    }
} /* shift */


/*
 * Addition and subtraction: +, -
 *
 * Handles pointers
 */
void
VCLCLASS add (void)
{
    double          fval;
    double          fval2;
    int             ival;
    char            ityp;
    ITEM            p1;
    ITEM            p2;
    PROMO           promol;
    PROMO           promor;
    char *          pval;
    int             size1;
    int             size2;
//    ITEM *          svstackptr;

    mult();

    for ( ;; )
    {
        switch ( Ctx.Token )
        {
            case T_ADD:
                if ( binarySkip( ADDRVCLCLASS mult ) )
                    continue;

                /*
                 * Save characteristics of first operand
                 */
                topget( &p1 );
                topgetpromo( &promol );
                if ( ItemisAddressOrPointer( p1 ) )
                {
                    if ( p1.type == VOID )
                        error( VOIDPTRERR );
                    /*
                     * Watch out for pointers to pointers: we have to add the
                     * size of a pointer (not the size of the fundamental
                     * data type) to the base address.
                     */
                    size1 = ElementWidth( &p1 );
                    pval = (char *) popptr();
                }
                else
                    fval = popflt();

                if ( opAssign )
                {
                    ++Ctx.Stackptr;
                    Ctx.Token = 0;
                }
                else
                    getoken();

//                svstackptr = Ctx.Stackptr;    
                mult();
//                if ( Ctx.Stackptr == svstackptr )
//                    error( EXPRERR );

                /*
                 * Save characteristics of second operand
                 */
                topget( &p2 );
                topgetpromo( &promor );
                if ( ItemisAddressOrPointer( p2 ) )
                {
                    if ( p2.type == VOID )
                        error( VOIDPTRERR );
                    size2 = ElementWidth( &p2 );
                }

                /*
                 * Addition of pointers is not allowed, according to K&R
                 */
                if ( ItemisAddressOrPointer( p1 ) && ItemisAddressOrPointer( p2 ) )
                    error( PTROPERR );
                else if ( ItemisAddressOrPointer( p1 ) )
                {
                    checkInteger( Ctx.Stackptr );
                    pushptr( pval + (popint() * size1), p1.type, p1.isunsigned );
                    topset( &p1 );
                }
                else if ( ItemisAddressOrPointer( p2 ) )
                {
                    checkInteger( &p1 );
                    ival = fval;
                    pushptr( (ival * size2) + (char *) popptr(), p2.type, p2.isunsigned );
                    topset( &p2 );
                }
                else
                {
                    fval2 = popflt();
                    /* promote both operands prior to calculation */
                    ityp = Promote( NULL, NULL, promol, promor );
                    switch ( ityp )
                    {
                        case I_FLOAT:
                            pushflt( fval + fval2, FALSE );
                            break;
                        case I_UNSLONG:
                            pushlng( (ulong)fval + (ulong)fval2, TRUE );
                            break;
                        case I_LONG:
                            pushlng( (long)fval + (long)fval2, FALSE );
                            break;
                        case I_UNSINT:
                            pushint( (uint)fval + (uint)fval2, TRUE );
                            break;
                        case I_INT:
                            pushint( (int)fval + (int)fval2, FALSE );
                            break;
                    }
                }
                break;

            case T_SUB:
                if ( binarySkip( ADDRVCLCLASS mult ) )
                    continue;

                /*
                 * Save characteristics of first operand
                 */
                topget( &p1 );
                topgetpromo( &promol );
                if ( ItemisAddressOrPointer( p1 ) )
                {
                    if ( p1.type == VOID )
                        error( VOIDPTRERR );
                    size1 = ElementWidth( &p1 );
                    pval = (char *) popptr();
                }
                else
                    fval = popflt();

                if ( opAssign )
                {
                    ++Ctx.Stackptr;
                    Ctx.Token = 0;
                }
                else
                    getoken();

//                svstackptr = Ctx.Stackptr;
                mult();
//                if ( Ctx.Stackptr == svstackptr )
//                    error( EXPRERR );

                /*
                 * Save characteristics of second operand
                 */
                topget( &p2 );
                topgetpromo( &promor );
                if ( ItemisAddressOrPointer( p2 ) )
                {
                    if ( p2.type == VOID )
                        error( VOIDPTRERR );
                    size2 = ElementWidth( &p2 );
                }

                /*
                 * Subtraction of pointers is allowed, but only if they
                 * point to the same type of object.  The result is an
                 * integer object.
                 */
                if ( ItemisAddressOrPointer( p1 ) && ItemisAddressOrPointer( p2 ) )
                {
                    if ( p1.cat != p2.cat || p1.type != p2.type )
                        error( PTROPERR );
                    else
                        pushint( (int) ( ( pval - (char *) popptr() ) / size1 ),
                                 p1.isunsigned || p2.isunsigned );
                }
                else if ( ItemisAddressOrPointer( p1 ) )
                {
                    checkInteger( Ctx.Stackptr );
                    pushptr( pval - (popint() * size1), p1.type, p1.isunsigned );
                    topset( &p1 );
                }
                else if ( ItemisAddressOrPointer( p2 ) )
                {
                    /*-
                     * Can't subtract a pointer from an int:
                     *      int i, char *cp;
                     *      i - cp;
                     */
                    error( PTROPERR );
                }
                else
                {
                    fval2 = popflt();
                    /* promote both operands prior to calculation */
                    ityp = Promote( NULL, NULL, promol, promor );
                    switch ( ityp )
                    {
                        case I_FLOAT:
                            pushflt( fval - fval2, FALSE );
                            break;
                        case I_UNSLONG:
                            pushlng( (ulong)fval - (ulong)fval2, TRUE );
                            break;
                        case I_LONG:
                            pushlng( (long)fval - (long)fval2, FALSE );
                            break;
                        case I_UNSINT:
                            pushint( (uint)fval - (uint)fval2, TRUE );
                            break;
                        case I_INT:
                            pushint( (int)fval - (int)fval2, FALSE );
                            break;
                    }
                }
                break;
            default:
                return;
        }
    }
} /* add */


/*
 * Multiplication, division and modulo: *, /, %
 */
void
VCLCLASS mult (void)
{
    double          fval;
    double          fval2;
    char            ityp;
    unsigned long   lval;
    unsigned long   lval2;
    PROMO           promol;
    PROMO           promor;
//    ITEM *          svstackptr;

    if ( opAssign )
        ++Ctx.Stackptr;
    else
        primary();

    for ( ;; )
    {
        switch ( Ctx.Token )
        {
            case T_MUL:
                if ( binarySkip( (void (VCLCLASS *) ()) ADDRVCLCLASS primary ) )
                    continue;
                catCheck();
                topgetpromo( &promol );
                fval = popflt();
                if ( opAssign )
                {
                    Ctx.Stackptr += 2;
                    Ctx.Token = 0;
                }
                else
                {
                    getoken();
//                    svstackptr = Ctx.Stackptr;
                    primary();
//                    if ( Ctx.Stackptr == svstackptr )
//                        error( EXPRERR );
                }
                catCheck();
                topgetpromo( &promor );
                fval2 = popflt();
                /* promote both operands prior to calculation */
                ityp = Promote( NULL, NULL, promol, promor );
                switch ( ityp )
                {
                    case I_FLOAT:
                        pushflt( fval * fval2, FALSE );
                        break;
                    case I_UNSLONG:
                        pushlng( (ulong)fval * (ulong)fval2, TRUE );
                        break;
                    case I_LONG:
                        pushlng( (long)fval * (long)fval2, FALSE );
                        break;
                    case I_UNSINT:
                        pushint( (uint)fval * (uint)fval2, TRUE );
                        break;
                    case I_INT:
                        pushint( (int)fval * (int)fval2, FALSE );
                        break;
                }
                break;

            case T_DIV:
                if ( binarySkip( (void (VCLCLASS *) ()) ADDRVCLCLASS primary ) )
                    continue;
                catCheck();
                topgetpromo( &promol );
                fval = popflt();
                if ( opAssign )
                {
                    Ctx.Stackptr += 2;
                    Ctx.Token = 0;
                }
                else
                {
                    getoken();
//                    svstackptr = Ctx.Stackptr;
                    primary();
//                    if ( Ctx.Stackptr == svstackptr )
//                        error( EXPRERR );
                }
                catCheck();
                topgetpromo( &promor );
                fval2 = popflt();
                /* no division by zero faults */
                if ( fval2 == 0 )
                    error( DIV0ERR );
                /* promote both operands prior to calculation */
                ityp = Promote( NULL, NULL, promol, promor );
                switch ( ityp )
                {
                    case I_FLOAT:
                        pushflt( fval / fval2, FALSE );
                        break;
                    case I_UNSLONG:
                        pushlng( (ulong)fval / (ulong)fval2, TRUE );
                        break;
                    case I_LONG:
                        pushlng( (long)fval / (long)fval2, FALSE );
                        break;
                    case I_UNSINT:
                        pushint( (uint)fval / (uint)fval2, TRUE );
                        break;
                    case I_INT:
                        pushint( (int)fval / (int)fval2, FALSE );
                        break;
                }
                break;

            case T_MOD:
                if ( binarySkip( (void (VCLCLASS *) ()) ADDRVCLCLASS primary ) )
                    continue;
                /* modulo requires integral types */
                checkInteger( Ctx.Stackptr );
                topgetpromo( &promol );
                lval = poplng();
                if ( opAssign )
                {
                    Ctx.Stackptr += 2;
                    Ctx.Token = 0;
                }
                else
                {
                    getoken();
//                    svstackptr = Ctx.Stackptr;
                    primary();
//                    if ( Ctx.Stackptr == svstackptr )
//                        error( EXPRERR );
                }
                checkInteger( Ctx.Stackptr );
                topgetpromo( &promor );
                lval2 = poplng();
                /* no division by zero faults */
                if ( lval2 == 0L )
                    error( DIV0ERR );
                /* promote both operands prior to calculation */
                ityp = Promote( NULL, NULL, promol, promor );
                switch ( ityp )
                {
                    /* no floating point modulo */
                    case I_UNSLONG:
                        pushlng( (ulong)lval % (ulong)lval2, TRUE );
                        break;
                    case I_LONG:
                        pushlng( (long)lval % (long)lval2, FALSE );
                        break;
                    case I_UNSINT:
                        pushint( (uint)lval % (uint)lval2, TRUE );
                        break;
                    case I_INT:
                        pushint( (int)lval % (int)lval2, FALSE );
                        break;
                }
                break;
            default:
                return;
        }
    }
} /* mult */


/*
 * Skip next expression based on function
 */
int
VCLCLASS binarySkip (void (VCLCLASS *fn) (void))
{
    if ( SkipExpression )
    {
        Ctx.Token = 0;
        if ( ! opAssign )
            getoken();
        (THISPTR(*fn)) ();
    }
    return SkipExpression;
} /* binarySkip */


/*
 * Check for an integer data type on the stack
 */
void
VCLCLASS checkInteger (ITEM *item)
{
    if ( ! SkipExpression && ! ItemisInteger( item ) )
        error( INTTYPERR );
} /* checkInteger */


/*
 * Check for illegal pointer operation
 */
void
VCLCLASS catCheck (void)
{
    if ( Ctx.Stackptr->cat )
        error( PTROPERR );
} /* catCheck */


/*
 * Setup a compare of 2 stack items
 */
int
VCLCLASS compareItems (double *fval)
{
    ITEM            left;
//    ITEM *          svstackptr;

    if ( binarySkip( ADDRVCLCLASS shift ) )
        return 4;

    getoken();
    left = *Ctx.Stackptr;
    *fval = popflt();

//    svstackptr = Ctx.Stackptr;
    shift();
//    if ( Ctx.Stackptr == svstackptr )
//        error( EXPRERR );

    if ( left.cat != Ctx.Stackptr->cat )
        error( PTRCOMPERR );

    if ( left.cat || ( left.isunsigned && Ctx.Stackptr->isunsigned ) )
        return 0;
    else if ( left.isunsigned )
        return 1;
    else if ( Ctx.Stackptr->isunsigned )
        return 2;
    else
        return 3;
} /* compareItems */


void
VCLCLASS skipExpr (void (VCLCLASS * fn) (void))
{
    SkipExpression++;
    (THISPTR(*fn)) ();
    --SkipExpression;
} /* skipExpr */
