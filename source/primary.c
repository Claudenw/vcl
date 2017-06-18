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
    primary.c - Interpreter primary expression parser module

 DESCRIPTION
    Expression evaluation routines.  Combined with the routines in expr.c
    these functions handle evaluating (parenthetical) expressions.  Operator
    precedence is enforced by the cascaded order of calling the functions.

 FUNCTIONS
    ItemArrayDimensions()
    ElementWidth()
    primary()

    ComputeDimension()
    Subscript()
    element()
    postop()
    prepostop()

 FILES
    vcldef.h

 SEE ALSO

 NOTES

 BUGS

*****************************************************************unpubModule*/

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

int
VCLCLASS ItemArrayDimensions (ITEM *item)
{
    int             i;

    for ( i = MAXDIM; i > 0; --i )
        if ( item->dims[i - 1] )
            break;
    return i;
} /* ItemArrayDimensions */


/*
int
VCLCLASS ItemArrayElements (ITEM *item)
{
    int             i;
    int             j = 1;

    for ( i = 0; i < item->cat; i++ )
        if ( item->dims[i] )
            j *= item->dims[i];
    return j;
}*/ /* ItemArrayElements */


int
VCLCLASS ElementWidth (ITEM *item)
{
    int             rtn = sizeof( void * );
    int             off = 1;
    int             j = item->cat;
    int             m;

    if ( ItemisArray( *item ) || item->dims[1] )
    {
        int             k = ItemArrayDimensions( item );

        if ( j <= k )
        {
            while ( --j )
            {
                m = item->dims[--k];
                off *= m ? m : 1;
            }
            rtn = item->size * off;
        }
    }
    else if ( j < 2 )
        rtn = item->size;
    return rtn;
} /* ElementWidth */


int
VCLCLASS ComputeDimension (int dim)
{
    int             width;

    if ( dim == MAXDIM )
        error( MDIMERR );
    getoken();

    /*
     * compute the subscript
     */
    expression();
    if ( Ctx.Token != T_RBRACKET )
        error( RBRACKETERR );
    getoken();
    width = popint();

    /*
     * Compute the offset (subscript times the size of the thing the pointer
     * is pointing at) and then the effective address.
     */
    width *= ElementWidth( Ctx.Stackptr );
    return width;
} /* ComputeDimension */


void
VCLCLASS Subscript (int dim)
{
    int             width;
    int             dims = ItemArrayDimensions( Ctx.Stackptr );

    width = ComputeDimension( dim );
    if ( Ctx.Stackptr->lvalue && dim == 0 )
        /*
         * pointer: the stack top item's value is the address of the pointer,
         * so do another level of indirection.
         */
        Ctx.Stackptr->value.pptr = (char **) ( *Ctx.Stackptr->value.pptr );

    Ctx.Stackptr->value.cptr += width;

    /*
     * The stack top item becomes an LVALUE, with reduced indirection level.
     */
    --Ctx.Stackptr->cat;
    if ( Ctx.Stackptr->cat == dims ||
         Ctx.Stackptr->cat == 0 ||
         ! ItemisArray( *Ctx.Stackptr ) )
        Ctx.Stackptr->lvalue = 1;
} /* Subscript */


VCLCLASS VARIABLE *
VCLCLASS primary (void)
{
    /*-
     * Parse a primary.  This function handles the following left-to-right
     * associative operators:
     *
     *    ()  function call
     *    []  array reference (only single dimension arrays)
     *    ->  structure pointer member selection
     *    .   structure member selection
     */
    VARIABLE *      pvar;
    VARIABLELIST    svstruct;
    int             done = 0;
    int             dim = 0;

    pvar = element();

    while ( ! done )
    {
        switch ( Ctx.Token )
        {
            case T_LBRACKET:
                if ( SkipExpression )
                {
                    skip( T_LBRACKET, T_RBRACKET );
                    break;
                }
                /*
                 * If it's not an address it can't be used as an array, but
                 * the pointer operator is still allowed.
                 */
                if ( ! StackItemisAddressOrPointer() )
                    error( NOTARRAYERR );
                if ( Ctx.Stackptr->type == VOID )
                    error( VOIDPTRERR );
                while ( Ctx.Token == T_LBRACKET )
                    Subscript( dim++ );
                dim = 0;
                break;

            case T_LPAREN:
                if ( SkipExpression )
                {
                    skip( T_LPAREN, T_RPAREN );
                    break;
                }
                if ( ! ( Ctx.Stackptr->kind & FUNCT ) )
                    error( FUNCERR );
                Ctx.Curfunction = (FUNCTION *) popptr();
                Assert( Ctx.Curfunction != NULL );
                if ( ConstExpression )
                    error( CONSTEXPRERR );
                /*
                 * function call
                 */
#ifdef DEBUGGER
                if ( SteppingOver )
                    StepCount++;
#endif
                callfunc();
#ifdef DEBUGGER
                if ( SteppingOver )
                    --StepCount;
#endif
                break;

            case T_ARROW:
                /*
                 * union or structure pointer reference
                 */
                if ( ConstExpression )
                    error( CONSTEXPRERR );
                if ( ! SkipExpression )
                {
                    if ( ! StackItemisAddressOrPointer() )
                        /*
                         * not a pointer
                         */
                        error( STRUCTPTRERR );
                    if ( Ctx.Stackptr->lvalue )
                        Ctx.Stackptr->value.cptr =
                            *Ctx.Stackptr->value.pptr;
                    /*
                     * item on top of the stack is promoted to LVALUE, and
                     * its indirection level decreases.
                     */
                    Ctx.Stackptr->lvalue = 1;
                    --Ctx.Stackptr->cat;
                    /*
                     * same as a struct.elem reference, fall into next case
                     */
                }
            case T_DOT:
                {
                    ITEM            elem;

                    if ( ! SkipExpression )
                    {
                        if ( Ctx.Token == T_DOT && StackItemisAddressOrPointer() )
                            error( PTROPERR );
                        /*
                         * Union or Structure element reference
                         */
                        if ( ! Ctx.Stackptr->elem )
                            /*
                             * Thing on stack is not a structure
                             */
                            error( STRUCERR );

                    }

                    /* Ctx.Curstruct supports the debugger */
                    svstruct = Ctx.Curstruct;
                    Ctx.Curstruct = *Ctx.Stackptr->elem;
                    getoken();
                    pvar = element();
                    Ctx.Curstruct = svstruct;

                    if ( SkipExpression )
                        break;

                    if ( ! ( pvar->vkind & STRUCTELEM ) )
                        error( ELEMERR );
                    elem = *Ctx.Stackptr;
                    /*
                     * pop the element
                     */
                    pop();
                    /*
                     * replace the structure with the element
                     */
                    *Ctx.Stackptr = elem;
                    postop();
                    break;
                }
            default:
                done = 1;
                break;
        }
    }
    return pvar;
} /* primary */


/*
 * Evaluate an element
 */
VCLCLASS VARIABLE *
VCLCLASS element (void)
{
    ITEM *          svstackptr;
    int             i;
    int             csize;
    int             ctype;
    char            ccat;
    char            isu;
    char            typ;
    VARIABLE        cvar;

    if ( Ctx.Token == T_LPAREN )
    {
        if ( SkipExpression )
        {
            skip( T_LPAREN, T_RPAREN );
            return NULL;
        }
        getoken();
        if ( istypespec() || Ctx.Token == T_VOID )
        {
            /*
             * typecast
             */
            cvar = *DeclareVariable( 0, 0, 0, 0, 0 );

            if ( Ctx.Token == T_RPAREN )
                getoken();
            else
                error( RPARENERR );
            svstackptr = Ctx.Stackptr;
            elementpvar = primary();
            if ( SkipExpression )
                return elementpvar;
            if ( Ctx.Stackptr == svstackptr )
                /*
                 * no primary after the typecast; push null variable to keep
                 * the stack straight.
                 */
                push( 0, 0, 0, 0, 0, 0, 0, &Ctx.Value, 0 );
            else
            {
                /*
                 * cast item on stack to new type
                 */
                if ( cvar.vtype != VOID || cvar.vcat )
                    store( rslvaddr( &Ctx.Stackptr->value.cptr,
                                     Ctx.Stackptr->lvalue ),
                           rslvsize( cvar.vsize,
                                     cvar.vcat ),
                           cvar.isunsigned,
                           rslvaddr( &Ctx.Stackptr->value.cptr,
                                     Ctx.Stackptr->lvalue ),
                           rslvsize( Ctx.Stackptr->size,
                                     Ctx.Stackptr->cat ),
                           Ctx.Stackptr->isunsigned
                         );
                Ctx.Stackptr->cat = cvar.vcat;
                Ctx.Stackptr->size = cvar.vsize;
                Ctx.Stackptr->type = cvar.vtype;
                Ctx.Stackptr->isunsigned = cvar.isunsigned;
                Ctx.Stackptr->vstruct = cvar.vstruct;
                for ( i = 0; i < MAXDIM; i++ )
                    Ctx.Stackptr->dims[i] = cvar.vdims[i];
            }
            return elementpvar;
        }
        /*
         * it's a parenthesized expression
         */
        ExpressionOne();                /* then do an expression */
        if ( Ctx.Token == T_RPAREN )
        {                               /* look for a ')' */
            getoken();
            postop();
            return elementpvar;
        }
        error( RPARENERR );
    }

    elementpvar = NULL;

    switch ( Ctx.Token )
    {
        case T_LNOT:
            /*
             * Logical NOT.

             * NOTE: All arithmetic and logical unary operators turn the item
             * currently on the top of the stack into an integer RVALUE.  So,
             * it's safe to just to say: pushint( <unary_operator>popint() );
             */
            getoken();
            elementpvar = primary();
            if ( ! SkipExpression )
                pushint( ! poplng(), FALSE );
            break;
        case T_NOT:
            /*
             * One's complement
             */
            getoken();
            primary();
            isu = Ctx.Stackptr->isunsigned;
            if ( ! SkipExpression )
                pushlng( ~poplng(), isu );
            break;
        case T_ADD:
            /*
             * Unary positive operator
             */
            getoken();
            primary();
            break;
        case T_SUB:
            /*
             * Negation (two's complement)
             */
            getoken();
            primary();
            isu = Ctx.Stackptr->isunsigned;
            typ = Ctx.Stackptr->type;
            if ( ! SkipExpression )
            {
                switch ( typ )
                {
                    case CHAR:
                    case INT:
                        pushint( -popint(), isu );
                        break;
                    case LONG:
                        pushlng( -poplng(), isu );
                        break;
                    default:
                        pushflt( -popflt(), isu );
                        break;
                }
            }
            break;
        case T_INCR:
            getoken();
            primary();
            prepostop( 0, T_INCR );
            break;
        case T_DECR:
            getoken();
            primary();
            prepostop( 0, T_DECR );
            break;
        case T_PTR:
            /*
             * Pointer operator
             */
            if ( ConstExpression )
                error( CONSTEXPRERR );
            getoken();
            primary();
            if ( SkipExpression )
                break;
            if ( ( ccat = Ctx.Stackptr->cat ) == 0 )
                error( NOTPTRERR );
            if ( ( Ctx.Stackptr->kind & FUNCT ) == 0 )
            {
                if ( Ctx.Stackptr->type == VOID )
                    error( VOIDPTRERR );
                /*
                 * indirection level (cat) decreases
                 */
                Ctx.Stackptr->cat = --ccat;
                Ctx.Stackptr->vconst = ( Ctx.Stackptr->vconst >> 1 );
                /*
                 * If item on stack is already an LVALUE, do an extra level
                 * of indirection
                 */
                if ( Ctx.Stackptr->lvalue )
                    Ctx.Stackptr->value.pptr =
                        (char **) *Ctx.Stackptr->value.pptr;
                if ( ccat == 0 || ! ItemisArray( *Ctx.Stackptr ) )
                    Ctx.Stackptr->lvalue = 1;
            }
            break;
        case T_AND:
            /*
             * Address operator
             */
            getoken();
            primary();
            if ( ( Ctx.Stackptr->vqualifier ) & REGISTER )
                error( REGADDRERR );
            if ( SkipExpression )
                break;
            /*
             * The item must be an LVALUE (can't take the address of a
             * constant, now can we?)
             */

            if ( ! Ctx.Stackptr->lvalue &&
                 Ctx.Stackptr->elem == 0 &&
                 Ctx.Stackptr->cat == 0 )

                error( LVALERR );
            /*
             * inverse of the pointer operator
             */
            if ( ( Ctx.Stackptr->kind & FUNCT ) == 0 )
            {
                Ctx.Stackptr->lvalue = 0;
                Ctx.Stackptr->cat++;
                Ctx.Stackptr->vconst <<= 1;
            }
            /*
             * The item on stack is now an address (pointer)
             */
            break;
        case T_SIZEOF:
            /*
             * Sizeof operator
             */
            getoken();

            if ( Ctx.Token == T_LPAREN )
            {
                getoken();
                if ( istypespec() )
                {
                    /*
                     * Data type specifier
                     */
                    elementpvar = DeclareVariable( 0, 0, 0, 0, 0 );
                    csize = elementpvar->vsize;
                    ctype = elementpvar->vtype;
                    ccat = elementpvar->vcat;
                    if ( ! SkipExpression )
                        push( elementpvar->vkind, elementpvar->isunsigned,
                           ccat, ccat, csize, ctype, 0, &Ctx.Value, 0 );
                }
                else
                    /*
                     * Otherwise a parenthesized expression
                     */
                    primary();
                if ( Ctx.Token == T_RPAREN )
                    getoken();
                else
                    error( RPARENERR );
            }
            else
                primary();

            if ( elementpvar )
                elementpvar->vconst = 0;

            if ( SkipExpression )
                break;
            if ( StackItemisAddressOrPointer() )
            {
                /*
                 * Pointer or an array.
                 */
                if ( Ctx.Stackptr->lvalue )
                    /*
                     * Pointer
                     */
                    i = sizeof( void * );
                else
                {
                    /*
                     * Array
                     */
                    i = Ctx.Stackptr->size;
                    if ( elementpvar && isArray( elementpvar ) )
                    {
                        if ( isPointerArray( elementpvar ) )
                            i = ArrayElements( elementpvar ) * sizeof( void * );
                        else
                            i *= ArrayElements( elementpvar );
                    }
                    else if ( Ctx.Stackptr->size == 1 )
                        /*
                         * String constant
                         */
                        i = strlen( Ctx.Stackptr->value.cptr ) + 1;
                }
            }
            else
                i = Ctx.Stackptr->size;

            pop();
            pushint( i, TRUE );         /* sizeof returns unsigned size_t */
            break;
        case T_CHRCONST:
            /*
             * Character constant
             */
            if ( ! SkipExpression )
            {
                pushint( Ctx.Value.ival, FALSE );
                /*
                 * Change the size of the item on the stack to CHAR
                 */
                Ctx.Stackptr->size = sizeof( char );
                /*
                 * byte order problem
                 */
                Ctx.Stackptr->value.cval = Ctx.Stackptr->value.ival;
            }
            getoken();
            break;
        case T_INTCONST:
            /*
             * Numeric constant
             */
            if ( ! SkipExpression )
                pushint( Ctx.Value.ival, FALSE );
            getoken();
            break;
        case T_UINTCONST:
            /*
             * Unsigned numeric constant
             */
            if ( ! SkipExpression )
                pushint( Ctx.Value.uival, TRUE );
            getoken();
            break;
        case T_LNGCONST:
            /*
             * Long numeric constant
             */
            if ( ! SkipExpression )
                pushlng( Ctx.Value.lval, FALSE );
            getoken();
            break;
        case T_ULNGCONST:
            /*
             * Unsigned long numeric constant
             */
            if ( ! SkipExpression )
                pushlng( Ctx.Value.ulval, TRUE );
            getoken();
            break;
        case T_FLTCONST:
            /*
             * Floating point constant
             */
            if ( ! SkipExpression )
                pushflt( Ctx.Value.fval, FALSE );
            getoken();
            break;
        case T_STRCONST:
            /*
             * String constant; similar to an array
             */
            if ( ! SkipExpression )
                push( 0, 0, 1, RVALUE, sizeof( char ), CHAR, NULL, &Ctx.Value, 1 );
            getoken();
            break;
        case T_FUNCTREF:
        case T_FUNCTION:
            {
                if ( Ctx.Curfunction == NULL )
                    error( DECLARERR );
                if ( ! SkipExpression )
                    push( FUNCT, 0, 1, RVALUE, 0, Ctx.Curfunction->type, NULL,
                          (DATUM *) &Ctx.Curfunction, 1 );
                getoken();
                return NULL;
            }
        case T_SYMBOL:                  /* debugger support */
        case T_IDENTIFIER:
            /*
             * Variable identifier
             */
            if ( SkipExpression )
            {
                getoken();
                break;
            }
            if ( ( elementpvar = Ctx.Curvar ) != 0 )
            {
                int             isLvalue;

                if ( Ctx.Curvar->vtype == ENUM )
                {
                    pushint( Ctx.Curvar->enumval, FALSE );
                    getoken();
                    break;
                }
                getoken();
                /*-
                 * It's a variable reference. The way a
                 * variable is represented on the stack
                 * depends on its type:
                 *
                 *            Ctx.Stackptr->
                 *            lvalue  cat  value.iptr
                 * plain:       1      0   address of var
                 * pointers:    1      1   ptr to address of ptr
                 * arrays:      0      1   address of var
                 */

                Ctx.Value.cptr = (char *) DataAddress( elementpvar );

                /* arrays, functions, and addresses aren't lvalues */
                isLvalue = ( ! ( isArray( elementpvar ) ||
                                 ( ( elementpvar->vkind & FUNCT ) &&
                                   ! isAddressOrPointer( elementpvar ) ) ) );

                push( elementpvar->vkind, elementpvar->isunsigned,
                      elementpvar->vcat, isLvalue, elementpvar->vsize,
                      elementpvar->vtype, (VARIABLELIST *) &elementpvar->velem,
                      &Ctx.Value, elementpvar->vconst );
                Ctx.Stackptr->vstruct = elementpvar->vstruct;
                Ctx.Stackptr->vqualifier = elementpvar->vqualifier;
                for ( i = 0; i < MAXDIM; i++ )
                    Ctx.Stackptr->dims[i] = elementpvar->vdims[i];
            }
            else
                /*
                 * Undeclared symbol
                 */
                error( DECLARERR );
            break;
        case T_RPAREN:
            break;
        case T_EOF:
        case T_SEMICOLON:
            break;
        default:
            error( EXPRERR );
    }

    if ( elementpvar && ( ( elementpvar->vkind & STRUCTELEM ) == 0 ) )
        postop();
    return elementpvar;
} /* element */


/*
 * post increment/decrement: x++ x--
 */
void
VCLCLASS postop (void)
{
    if ( Ctx.Token == T_INCR || Ctx.Token == T_DECR )
    {
        prepostop( 1, Ctx.Token );
        getoken();
    }
} /* postop */


/*
 * pre- and post- increment/decrement: ++ --
 *
 * These are right-to-left associative operators
 */
void
VCLCLASS prepostop (int ispost, char tok)
{
    char *          p;
    DATUM           v;
    char            sign = tok == T_INCR ? 1 : -1;

    if ( SkipExpression )
        return;

    if ( ConstExpression )
        error( CONSTEXPRERR );

    if ( ! readonly( Ctx.Stackptr ) )
    {
        if ( StackItemisAddressOrPointer() )
        {
            if ( Ctx.Stackptr->type == VOID )
                error( VOIDPTRERR );
            /*
             * It's a pointer - save its old value then increment/decrement
             * the pointer. This makes the item on top of the stack look like
             * an array, which means it can no longer be used as an LVALUE.
             * This doesn't really hurt, since it doesn't make much sense to
             * say:
             */
            /*-
             *   char *cp;
             *   cp++ = value;
             */
            p = *Ctx.Stackptr->value.pptr;

            *Ctx.Stackptr->value.pptr += sign * ElementWidth( Ctx.Stackptr );

            if ( ispost )
            {
                Ctx.Stackptr->value.pptr = (char **) p;
                Ctx.Stackptr->lvalue = 0;
            }
        }
        else
        {
            /*
             * It's a simple variable - save its old value then
             * increment/decrement the variable. This makes the item on top
             * of the stack look like a constant, which means it can no
             * longer be used as an LVALUE.
             */
            if ( Ctx.Stackptr->type == CHAR )
            {
                v.ival = *Ctx.Stackptr->value.cptr;
                *Ctx.Stackptr->value.cptr += sign;
                if ( ispost )
                    Ctx.Stackptr->value.cval = v.ival;
            }
            else if ( Ctx.Stackptr->type == INT )
            {
                v.ival = *Ctx.Stackptr->value.iptr;
                *Ctx.Stackptr->value.iptr += sign;
                if ( ispost )
                    Ctx.Stackptr->value.ival = v.ival;
            }
            else if ( Ctx.Stackptr->type == LONG )
            {
                v.lval = *Ctx.Stackptr->value.lptr;
                *Ctx.Stackptr->value.lptr += sign;
                if ( ispost )
                    Ctx.Stackptr->value.lval = v.lval;
            }
            else if ( Ctx.Stackptr->type == FLOAT )
            {
                v.fval = *Ctx.Stackptr->value.fptr;
                *Ctx.Stackptr->value.fptr += sign;
                if ( ispost )
                    Ctx.Stackptr->value.fval = v.fval;
            }
            else
                error( LVALERR );
            if ( ispost )
                Ctx.Stackptr->lvalue = 0;
        }
    }
    else
        error( LVALERR );
} /* prepostop */
