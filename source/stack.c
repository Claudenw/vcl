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
    stack.c - Assignment & stack handling routines

 DESCRIPTION
    Assignment and stack management routines.

 FUNCTIONS
    assignment()
    pop()
    popn()
    popflt()
    popint()
    popnint()
    poplng()
    popptr()
    push()
    pushflt()
    pushint()
    pushlng()
    pushptr()
    topdup()
    topget()
    topgetpromo()
    topset()
    readonly()
    StackItemisNumericType()

    psh()
    incompatible()
    numcheck()
    resolveaddr()
    torvalue()

 FILES
    vcldef.h

 SEE ALSO
    promote.c

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

/*
 * Perform an assignment
 */
void
VCLCLASS assignment (void)
{
    ITEM *          sp;

    if ( ( sp = Ctx.Stackptr - 1 ) <= Stackbtm )
        error( POPERR );

    /*
     * Make sure we've got an lvalue and compatible pointers
     */
    if ( readonly( sp ) )
        error( LVALERR );
    else if ( incompatible( sp, Ctx.Stackptr ) )
        error( INCOMPATERR );
    else
    {
        /*
         * Store the item value to the second item on the stack,
         * then pop it from stack.  The value is logically cast
         * to the signed or unsigned type of the lvalue.
         */
        if ( ! SkipExpression )
            store( sp->value.cptr,
                   rslvsize( sp->size, sp->cat ),
                   sp->isunsigned,
                   resolveaddr( &Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue ),
                   rslvsize( Ctx.Stackptr->size, Ctx.Stackptr->cat ),
                   Ctx.Stackptr->isunsigned );
        pop();

        /*
         * The result of an assignment operation is that the item on top of
         * the stack inherits the attributes and value of the right-hand
         * operand, but is demoted to an RVALUE.
         */
        torvalue( Ctx.Stackptr );
    }
} /* assignment */


/*
 * Pop the stack and check for underflow
 */
void
VCLCLASS pop (void)
{
    if ( Ctx.Stackptr-- <= Stackbtm )
        error( POPERR );
} /* pop */


/*
 * Pop the given number of arguments off the stack.
 * Returns nothing useful.
 */
void
VCLCLASS popn (int argc)
{
    while ( argc-- > 0 )
        pop();
} /* popn */


/*
 * Resolve the item on the top of the stack and return it
 * as a floating point number.
 */
double
VCLCLASS popflt (void)
{
    double          fvalue;

    numcheck();
    store( &fvalue,
           sizeof( double ),
           FALSE,
           resolveaddr( &Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue ),
           rslvsize( Ctx.Stackptr->size, Ctx.Stackptr->cat ),
           Ctx.Stackptr->isunsigned );
    pop();
    return fvalue;
} /* popflt */


/*
 * Resolve the item on the top of the stack and return it
 * as an integer.
 */
int
VCLCLASS popint (void)
{
    int             ivalue;

    numcheck();
    store( &ivalue,
           sizeof( int ),
           FALSE,
           resolveaddr( &Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue ),
           rslvsize( Ctx.Stackptr->size, Ctx.Stackptr->cat ),
           Ctx.Stackptr->isunsigned );
    pop();
    return ivalue;
} /* popint */


/*
 * Pop the given number of arguments off the stack.
 * Returns the integer value of the first item popped.
 */
int
VCLCLASS popnint (int argc)
{
    int             ivalue;

    ivalue = popint();
    popn( --argc );

    return ivalue;
} /* popnint */


/*
 * Resolve the item on the top of the stack and return it
 * as a long integer.
 */
long
VCLCLASS poplng (void)
{
    long            lvalue;

    numcheck();
    store( &lvalue,
           sizeof( long ),
           FALSE,
           resolveaddr( &Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue ),
           rslvsize( Ctx.Stackptr->size, Ctx.Stackptr->cat ),
           Ctx.Stackptr->isunsigned );
    pop();
    return lvalue;
} /* poplng */


/*
 * Resolve the item on the top of the stack and return it
 * as a pointer.
 */
void *
VCLCLASS popptr (void)
{
    void *          pvalue;

    if ( Ctx.Stackptr->cat == 0 && Ctx.Stackptr->value.ival )
        error( ADDRERR );
    store( &pvalue,
           sizeof( void * ),
           FALSE,
           resolveaddr( &Ctx.Stackptr->value.cptr, Ctx.Stackptr->lvalue ),
           rslvsize( Ctx.Stackptr->size, Ctx.Stackptr->cat ),
           Ctx.Stackptr->isunsigned );
    pop();
    return pvalue;
} /* popptr */


/*
 * Advance stack pointer and check for overflow
 */
void
VCLCLASS psh (void)
{
    if ( ++Ctx.Stackptr > Stacktop )
        error( PUSHERR );
    Stackmax = ( Ctx.Stackptr > Stackmax ) ? Ctx.Stackptr : Stackmax;    
} /* psh */


/*
 * Push item parts onto the stack
 */
void
VCLCLASS push (char pkind, char isunsigned, char pcat, char plvalue,
      unsigned int psize, char ptype, VARIABLELIST *pelem,
      DATUM *pdatum, char pconst)
{
    int             i;

    psh();
    Ctx.Stackptr->kind = pkind;
    Ctx.Stackptr->isunsigned = isunsigned;
    Ctx.Stackptr->cat = pcat;
    Ctx.Stackptr->lvalue = plvalue;
    Ctx.Stackptr->size = psize;
    Ctx.Stackptr->type = ptype;
    Ctx.Stackptr->elem = pelem;
    Ctx.Stackptr->value = *pdatum;
    Ctx.Stackptr->vconst = pconst;
    Ctx.Stackptr->vqualifier = 0;
    Ctx.Stackptr->vstruct = NULL;
    for ( i = 0; i < MAXDIM; i++ )
        Ctx.Stackptr->dims[i] = 0;
} /* push */


/*
 * push a floating point number onto the stack
 */
void
VCLCLASS pushflt (double fvalue, char isu)
{
    psh();
    Ctx.Stackptr->kind =
        Ctx.Stackptr->cat =
        Ctx.Stackptr->vconst =
        Ctx.Stackptr->vqualifier =
        Ctx.Stackptr->lvalue = 0;
    Ctx.Stackptr->isunsigned = isu;
    Ctx.Stackptr->elem = NULL;
    Ctx.Stackptr->type = FLOAT;
    Ctx.Stackptr->size = sizeof( double );
    Ctx.Stackptr->value.fval = fvalue;
} /* pushflt */


/*
 * push an integer onto the stack
 */
void
VCLCLASS pushint (int ivalue, char isu)
{
    psh();
    Ctx.Stackptr->kind =
        Ctx.Stackptr->cat =
        Ctx.Stackptr->vconst =
        Ctx.Stackptr->vqualifier =
        Ctx.Stackptr->lvalue = 0;
    Ctx.Stackptr->isunsigned = isu;
    Ctx.Stackptr->elem = NULL;
    Ctx.Stackptr->size = sizeof( int );
    Ctx.Stackptr->type = INT;
    Ctx.Stackptr->value.ival = ivalue;
} /* pushint */


/*
 * push a long integer onto the stack
 */
void
VCLCLASS pushlng (long lvalue, char isu)
{
    psh();
    Ctx.Stackptr->kind =
        Ctx.Stackptr->cat =
        Ctx.Stackptr->vconst =
        Ctx.Stackptr->vqualifier =
        Ctx.Stackptr->lvalue = 0;
    Ctx.Stackptr->isunsigned = isu;
    Ctx.Stackptr->elem = NULL;
    Ctx.Stackptr->size = sizeof( long );
    Ctx.Stackptr->type = LONG;
    Ctx.Stackptr->value.lval = lvalue;
} /* pushlng */


/*
 * push a pointer onto the stack
 */
void
VCLCLASS pushptr (void *pvalue, char ptype, char isu)
{
    psh();
    Ctx.Stackptr->kind = 0;
    Ctx.Stackptr->isunsigned = isu;
    Ctx.Stackptr->cat = 1;
    Ctx.Stackptr->vqualifier = 0;
    Ctx.Stackptr->vconst = 0;
    Ctx.Stackptr->lvalue = 0;
    Ctx.Stackptr->elem = NULL;
    Ctx.Stackptr->size = TypeSize( ptype );
    if ( Ctx.Stackptr->size == 0 )
        Ctx.Stackptr->size = sizeof( void * );
    Ctx.Stackptr->type = ptype;
    Ctx.Stackptr->value.cptr = (char *) pvalue;
} /* pushptr */


/*
 * Push a duplicate of the item on top of the stack
 */
void
VCLCLASS topdup (void)
{
    push( Ctx.Stackptr->kind, Ctx.Stackptr->isunsigned,
          Ctx.Stackptr->cat, Ctx.Stackptr->lvalue,
          Ctx.Stackptr->size, Ctx.Stackptr->type,
          Ctx.Stackptr->elem, &Ctx.Stackptr->value,
          Ctx.Stackptr->vconst );
} /* topdup */


/*
 * Get the -attributes- of the item on top of the stack
 * Does not copy or set the item's value(s)
 */
void
VCLCLASS topget (ITEM *pitem)
{
    int             i;

    pitem->kind = Ctx.Stackptr->kind;
    pitem->isunsigned = Ctx.Stackptr->isunsigned;
    pitem->cat = Ctx.Stackptr->cat;
    pitem->size = Ctx.Stackptr->size;
    pitem->type = Ctx.Stackptr->type;
    pitem->elem = Ctx.Stackptr->elem;
    pitem->vconst = Ctx.Stackptr->vconst;
    pitem->vqualifier = Ctx.Stackptr->vqualifier;
    for ( i = 0; i < MAXDIM; i++ )
        pitem->dims[i] = Ctx.Stackptr->dims[i];
} /* topget */


/*
 * Get the promotion attributes of the item on top of the stack
 */
void
VCLCLASS topgetpromo (PROMO *promop)
{
    promop->type = Ctx.Stackptr->type;
    promop->isu = Ctx.Stackptr->isunsigned;
} /* topgetpromo */


/*
 * Set the attributes of the item on top of the stack
 * This routine is used by the add() function to duplicate
 * the attributes of a pointer variable on the stack
 * after addition or subtraction of pointers.
 */
void
VCLCLASS topset (ITEM *pitem)
{
    int             i;

    Ctx.Stackptr->kind = pitem->kind;
    Ctx.Stackptr->isunsigned = pitem->isunsigned;
    Ctx.Stackptr->cat = pitem->cat;
    Ctx.Stackptr->lvalue = 0;           /* the item always turns into an
                                         * RVALUE */
    Ctx.Stackptr->type = pitem->type;
    Ctx.Stackptr->size = pitem->size;
    Ctx.Stackptr->elem = pitem->elem;
    Ctx.Stackptr->vconst = pitem->vconst;
    Ctx.Stackptr->vqualifier = pitem->vqualifier;
    for ( i = 0; i < MAXDIM; i++ )
        Ctx.Stackptr->dims[i] = pitem->dims[i];
} /* topset */


int
VCLCLASS readonly (ITEM *sp)
{
    return (
             ( ! sp->cat && sp->vconst )
             ||
             ( sp->cat && ( sp->vconst & 1 ) )
             ||
             ( ! sp->lvalue )
     );
} /* readonly */


int
VCLCLASS StackItemisNumericType (void)
{
    char            type = Ctx.Stackptr->type;

    return ( StackItemisAddressOrPointer() ||
             ! ( type == STRUCT || type == VOID || type == UNION
                 || ( Ctx.Stackptr->kind & FUNCT ) ) );
} /* StackItemisNumericType */


/*
 * Determine if 2 items are of compatible types
 */
int
VCLCLASS incompatible (ITEM *d, ITEM *s)
{
    if ( d->cat == 0 )
        return 0;
    if ( d->vconst & 1 )
        return 1;
    if ( d->vconst == 2 )
        return 0;
    return ( s->vconst > 1 );
} /* incompatible */


/*
 * Check for numeric type
 */
void
VCLCLASS numcheck (void)
{
    if ( ! StackItemisNumericType() )
        error( NOTNUMERR );
} /* numcheck */


/*
 * Resolve a stack address
 */
char *
VCLCLASS resolveaddr (char **addr, char lval)
{
    if ( lval )
    {
        if ( ConstExpression )
            error( CONSTEXPRERR );
        return *addr;
    }
    return (char *) addr;
} /* resolveaddr */


/*
 * Convert the item pointed at by "item" to an RVALUE.
 */
void
VCLCLASS torvalue (ITEM *item)
{
    int             i;

    if ( item->lvalue )
    {
        i = rslvsize( item->size, item->cat );
        store( &item->value.cval,
               i,
               item->isunsigned,
               item->value.cptr,
               i,
               item->isunsigned );
        item->lvalue = 0;
    }
} /* torvalue */
