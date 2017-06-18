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
    promote.c - Data type promotion and storage functions

 DESCRIPTION
    Contains functions related to data type promotion logic and storing
    one type to another (e.g. assignment).

 FUNCTIONS
    store()
    type promotion static functions too numerous to list
    promote()

 SEE ALSO
    stack.c

 NOTES

 BUGS

*****************************************************************unpubModule*/

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

/*-
 * Store the from item into the object pointed to by toaddr.  The data type
 * is promoted using the appropriate assignment function, including unsigned
 * values.
 *
 * This function relies on each data type being a different size, and
 * all floats being handled as doubles internally.
 */
void
VCLCLASS store (void *toaddr, int tosize, char toisu, void *fraddr, int frsize, char frisu)
{
    int             index;

    /* Used to calculate the initial TO data type block, and FROM offset */
    enum typeblocks
    {
        CHAR_OFFSET, INT_OFFSET, LONG_OFFSET, DOUBLE_OFFSET
    };


    to.cptr = (char *) toaddr;          /* set the TO address pointer */
    fr.cptr = (char *) fraddr;          /* set the FROM address pointer */

    /*-
     * Set the base index by the TO size.
     * These values are 0, 1, ...
     *
     * If not one of the predefined sizes, it must be a struct or union.
     */
    if ( tosize == sizeof( char ) )
        index = CHAR_OFFSET;
    else if ( tosize == sizeof( int ) )
        index = INT_OFFSET;
    else if ( tosize == sizeof( long ) )
        index = LONG_OFFSET;
    else if ( tosize == sizeof( double ) )
        index = DOUBLE_OFFSET;
    else
    {
        /* must be a struct or union */
        if ( tosize != frsize )
            error( INCOMPTYPERR );
        for ( index = 0; index < tosize; ++index )
            *( to.cptr + index ) = *( fr.cptr + index );
        return;                         /* finished */
    }

    /*--------------------------------------------------------------------
     * Calculate an index to the appropriate assignment / type-promotion
     * function.  The sequence is by TO size (above), then by TO sign,
     * then by FROM sign, then FROM size.
     *--------------------------------------------------------------------*/

    /*-
     * Multiply by the number of assignment functions per TO type (16) to
     * position the index to the correct TO data type.
     *
     * 16 ::=   2 TO qualifiers (signed, unsigned) *
     *          4 data types (char, int, long, double) *
     *          2 FROM qualifiers (signed, unsigned)
     *
     * or 2 * 4 * 2 = 16.
     *
     */
    index *= 16;

    /*
     * If TO is unsigned, add offset to last half of that type's
     * group of assignment functions.
     */
    if ( toisu )
        index += 8;

    /*
     * If FROM is unsigned, add offset to next higher quarter of that type's
     * group of assignment functions.
     */
    if ( frisu )
        index += 4;

    /*
     * Now, offset the index by the FROM size.
     * These values are 0, 1, ... so checking for CHAR_OFFSET
     * is wasteful because we would only add index + 0.
     */
    if ( frsize == sizeof( int ) )
        index += INT_OFFSET;
    else if ( frsize == sizeof( long ) )
        index += LONG_OFFSET;
    else if ( frsize == sizeof( double ) )
        index += DOUBLE_OFFSET;

    /*
     * Call the assignment function
     */
    (THISPTR(*storeFunc[ index ]))();

} /* store */


/*
 * TO: character assignment functions
 *------------------------------------*/

/* s - s */
void
VCLCLASS cscs (void)                             /* 0 */
{
    *to.cptr = *fr.cptr;
}


void
VCLCLASS csis (void)
{
    *to.cptr = *fr.iptr;
}


void
VCLCLASS csls (void)
{
    *to.cptr = (char) *fr.lptr;
}


void
VCLCLASS csds (void)
{
    *to.cptr = *fr.fptr;
}


/* s - u */
void
VCLCLASS cscu (void)                             /* 4 */
{
    *to.cptr = *fr.ucptr;
}


void
VCLCLASS csiu (void)
{
    *to.cptr = *fr.uiptr;
}


void
VCLCLASS cslu (void)
{
    *to.cptr = (char) *fr.ulptr;
}


/* u - s */
void
VCLCLASS cucs (void)                             /* 8 */
{
    *to.ucptr = *fr.cptr;
}


void
VCLCLASS cuis (void)
{
    *to.ucptr = *fr.iptr;
}


void
VCLCLASS culs (void)
{
    *to.ucptr = (unsigned char) *fr.lptr;
}


void
VCLCLASS cuds (void)
{
    *to.ucptr = *fr.fptr;
}


/* u - u */
void
VCLCLASS cucu (void)                             /* 12 */
{
    *to.ucptr = *fr.ucptr;
}


void
VCLCLASS cuiu (void)
{
    *to.ucptr = *fr.uiptr;
}


void
VCLCLASS culu (void)
{
    *to.ucptr = (unsigned char) *fr.ulptr;
}


/*
 * TO: integer assignment functions
 *----------------------------------*/

/* s - s */
void
VCLCLASS iscs (void)                             /* 16 */
{
    *to.iptr = *fr.cptr;
}


void
VCLCLASS isis (void)
{
    *to.iptr = *fr.iptr;
}


void
VCLCLASS isls (void)
{
    *to.iptr = (int) *fr.lptr;
}


void
VCLCLASS isds (void)
{
    *to.iptr = *fr.fptr;
}


/* s - u */
void
VCLCLASS iscu (void)                             /* 20 */
{
    *to.iptr = *fr.ucptr;
}


void
VCLCLASS isiu (void)
{
    *to.iptr = *fr.uiptr;
}


void
VCLCLASS islu (void)
{
    *to.iptr = (int) *fr.ulptr;
}


/* u - s */
void
VCLCLASS iucs (void)                             /* 24 */
{
    *to.uiptr = *fr.cptr;
}


void
VCLCLASS iuis (void)
{
    *to.uiptr = *fr.iptr;
}


void
VCLCLASS iuls (void)
{
    *to.uiptr = (unsigned int) *fr.lptr;
}


void
VCLCLASS iuds (void)
{
    *to.uiptr = *fr.fptr;
}


/* u - u */
void
VCLCLASS iucu (void)                             /* 28 */
{
    *to.uiptr = *fr.ucptr;
}


void
VCLCLASS iuiu (void)
{
    *to.uiptr = *fr.uiptr;
}


void
VCLCLASS iulu (void)
{
    *to.uiptr = (unsigned int) *fr.ulptr;
}


/*
 * TO: long assignment functions
 *-------------------------------*/

/* s - s */
void
VCLCLASS lscs (void)                             /* 32 */
{
    *to.lptr = *fr.cptr;
}


void
VCLCLASS lsis (void)
{
    *to.lptr = *fr.iptr;
}


void
VCLCLASS lsls (void)
{
    *to.lptr = *fr.lptr;
}


void
VCLCLASS lsds (void)
{
    *to.lptr = *fr.fptr;
}


/* s - u */
void
VCLCLASS lscu (void)                             /* 36 */
{
    *to.lptr = *fr.ucptr;
}


void
VCLCLASS lsiu (void)
{
    *to.lptr = *fr.uiptr;
}


void
VCLCLASS lslu (void)
{
    *to.lptr = (long) *fr.ulptr;
}


/* u - s */
void
VCLCLASS lucs (void)                             /* 40 */
{
    *to.ulptr = *fr.cptr;
}


void
VCLCLASS luis (void)
{
    *to.ulptr = *fr.iptr;
}


void
VCLCLASS luls (void)
{
    *to.ulptr = (unsigned long) *fr.lptr;
}


void
VCLCLASS luds (void)
{
    *to.ulptr = *fr.fptr;
}


/* u - u */
void
VCLCLASS lucu (void)                             /* 44 */
{
    *to.ulptr = *fr.ucptr;
}


void
VCLCLASS luiu (void)
{
    *to.ulptr = *fr.uiptr;
}


void
VCLCLASS lulu (void)
{
    *to.ulptr = (unsigned long) *fr.ulptr;
}


/*
 * TO: double assignment functions
 *---------------------------------*/

/* s - s */
void
VCLCLASS dscs (void)                             /* 48 */
{
    *to.fptr = *fr.cptr;
}


void
VCLCLASS dsis (void)
{
    *to.fptr = *fr.iptr;
}


void
VCLCLASS dsls (void)
{
    *to.fptr = (double) *fr.lptr;
}


void
VCLCLASS dsds (void)
{
    *to.fptr = *fr.fptr;
}


/* s - u */
void
VCLCLASS dscu (void)                             /* 52 */
{
    *to.fptr = *fr.ucptr;
}


void
VCLCLASS dsiu (void)
{
    *to.fptr = *fr.uiptr;
}


void
VCLCLASS dslu (void)
{
    *to.fptr = (double) *fr.ulptr;
}


/*
 * Determine integral type promotion between 2 stack operands
 *
 * Handles types INT, LONG, (unsigned) INT, (unsigned) LONG, FLOAT.
 * Note! Does not handle pointer, arrays or unions.
 */
int
VCLCLASS Promote (char *typp, char *isup, PROMO lo, PROMO ro)
{
    char            t;
    char            u;
    int             ityp;

    if ( lo.type == FLOAT ||
         ro.type == FLOAT )
    {
        t = FLOAT;
        u = FALSE;
        ityp = I_FLOAT;
    }
    else if ( ( (lo.type == LONG && lo.isu) ||
                (ro.type == LONG && ro.isu) ) ||
              ( (lo.type == LONG && (ro.type == INT && ro.isu)) ||
                (ro.type == LONG && (lo.type == INT && lo.isu)) ) )
    {
        t = LONG;
        u = TRUE;
        ityp = I_UNSLONG;
    }
    else if ( lo.type == LONG ||
              ro.type == LONG )
    {
        t = LONG;
        u = FALSE;
        ityp = I_LONG;
    }
    else if ( (lo.type == INT && lo.isu) ||
              (ro.type == INT && ro.isu) )
    {
        t = INT;
        u = TRUE;
        ityp = I_UNSINT;
    }
    else
    {
        t = INT;
        u = FALSE;
        ityp = I_INT;
    }

    if ( typp )                         /* if pointer defined */
        *typp = t;                      /* ..set promotion type */

    if ( isup )                         /* if pointer defined */
        *isup = u;                      /* ..set unsigned flag */

    return ityp;                        /* return integral type */
} /* Promote */
