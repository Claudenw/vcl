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
    func.c - Function calling routines

 DESCRIPTION
    Contains routines for calling pcode and binary functions and handling 
    function arguments and prototypes.

 FUNCTIONS
    callfunc()                          Call a function
    TestZeroReturn()                    Test for required return

    ArgumentList()                      Build function argument list

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
#include <setjmp.h>
#include <errno.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

/*
 * Call a function
 */
void
VCLCLASS callfunc (void)
{
    unsigned char * svprogptr;
    FUNCTION *      svCurfunction = Ctx.Curfunction;
    ITEM *          args;
    ITEM *          proargs;
    int             argc;
    int             i;
    FUNCRUNNING     func;
    char            c;

    /*
     * If there are any arguments, evaluate them and leave their values on
     * the stack
     */
    proargs = args = Ctx.Stackptr + 1;
    getoken();                          /* get rid of the '(' */
    argc = expression();
    if ( Ctx.Token != ')' )
        error( RPARENERR );

    Ctx.Curfunction = svCurfunction;
    svprogptr = Ctx.Progptr;

    /*
     * If not main(), setup call to a function.  Save current program
     * pointer and set it to start address of function.
     */

    if ( ! Ctx.Curfunction->ismain )
    {
        VARIABLE *      svar;
        unsigned char * typ;
        int             proargc = argc;
        int             indir = 0;

        /*
         * Compare the parameters passed and the function's prototype.
         * Prototypes built by BuildPrototype()
         */
        typ = (unsigned char *) Ctx.Curfunction->proto;
        typ = typ ? typ : (unsigned char *) "";
        if ( proargc == 0 && *typ != 0xff && *typ != VOID )
            error( MISMATCHERR );

        while ( *typ != 0xff && *typ != T_ELLIPSE && proargc )
        {
            switch ( *typ )
            {
                case CHAR:
                case INT:
                case LONG:
                case FLOAT:
                    if ( proargs->type != *typ )
                        error( MISMATCHERR );
                    break;
                case VOID:
                    if ( ! *( typ + 1 ) )
                        error( MISMATCHERR );
                    break;
                case STRUCT:
                case UNION:
                    if ( proargs->type != *typ )
                        error( MISMATCHERR );
                    svar = *(VARIABLE **) ( typ + 1 );
                    Assert( svar != NULL );
                    typ += sizeof( VARIABLE * );
                    if ( proargs->vstruct != svar->vstruct )
                        error( MISMATCHERR );
                    break;
            }

            indir = *++typ;
            if ( proargs->cat != indir )
                error( MISMATCHERR );

            --proargc;
            proargs++;
            typ++;
        }
        if ( *typ != T_ELLIPSE && *typ != VOID && ( proargc || *typ != 0xff ) )
            error( MISMATCHERR );
    }

    Ctx.Progptr = (unsigned char *) Ctx.Curfunction->code;
    func.fvar = Ctx.Curfunction;
    func.fprev = Ctx.Curfunc;
    func.BlkNesting = 0;
    func.ldata = Ctx.NextData;
    func.arglength = 0;
    Ctx.Curfunc = &func;
    Saw_return = 0;

    if ( Ctx.Progptr == NULL )
    {
        /* internal binary function call */
        if ( Ctx.Curfunction->libcode == 0 )
            error( Ctx.Curfunction->ismain ? NOMAINERR : NOFUNCERR );

        switch ( Ctx.Curfunction->libcode )
        {
            case SYSERRNO:
                pushptr( &errno, INT, FALSE );
                break;
            case SYSLINENO:
                pushint( Ctx.CurrLineno, FALSE );
                break;
            case SYSFILENAME:
                pushptr( SrcFileName( Ctx.CurrFileno ), CHAR, FALSE );
                break;
            case SYSSETJMP:
                if ( longjumping )
                {
                    pushint( jmp_val, FALSE );
                    jmp_val = 0;
                    longjumping = FALSE;
                }
                else
                {
                    JMPBUF *        thisjmp = (JMPBUF *) popptr();

                    *thisjmp = stmtjmp;
                    thisjmp->jmp_id = 0x0a0a;
                    pushint( 0, FALSE );
                }
                break;
            case SYSLONGJMP:
                {
                    JMPBUF *        thisjmp;

                    jmp_val = popint();
                    thisjmp = (JMPBUF *) popptr();
                    if ( thisjmp->jmp_id != 0x0a0a )
                        error( NOSETJMPERR );
                    stmtjmp = *thisjmp;
                    Ctx = stmtjmp.jmp_ctx;
                    if ( Ctx.NextVar->vprev )
                        Ctx.NextVar->vprev->vnext = NULL;
                    longjumping = TRUE;
                    longjmp( stmtjmp.jb, 1 );
                    break;
                }
            default:
                {
                    for ( i = 0; vFuncs[i] != 0; i++ )
                        if ( Ctx.Curfunction->libcode == vFuncs[i] )
                            break;

                    if ( vFuncs[i] != 0 )
                        ArgumentList( argc, args );

                    pushint( Ctx.Curfunction->libcode, FALSE );

                    inSystem = 1;
                    if ( setjmp( BreakJmp ) == 0 )
                        sys();          /* let the system call handler do it */
                    else
                        pushint( 0, FALSE );
                    inSystem = 0;
                    break;
                }
        }
        Saw_return = 1;
    }
    else
    {
        /*-
         * pcode function call
         *
         * process the function's formal argument list and argument
         * declaration list
         */
        ArgumentList( argc, args );

        /*
         * beginning of statement for debugger
         */
        Ctx.CurrFileno = Ctx.Curfunction->fileno;
        Ctx.CurrLineno = Ctx.Curfunction->lineno;
        stmtbegin();
        getoken();

        /*
         * clear the arguments off the stack
         */
        Ctx.Stackptr -= argc;

        /*
         * execute the function
         */
        Ctx.svpptr = Ctx.Progptr;
        statement();
    }

    /*
     * If the function executed a return statement, the return value is
     * whatever was last on the stack.  Otherwise, the function returns a
     * value of zero.
     */
    if ( Saw_return )
    {
        if ( Ctx.Stackptr->lvalue )
        {
            c = rslvsize( Ctx.Stackptr->size, Ctx.Stackptr->cat );
            store( &Ctx.Stackptr->value.cval,
                   c,
                   Ctx.Stackptr->isunsigned,
                   Ctx.Stackptr->value.cptr,
                   c,
                   Ctx.Stackptr->isunsigned );
            Ctx.Stackptr->lvalue = 0;
        }
        Saw_return = 0;
    }
    else
    {
        /* no return from function */
        pushint( 0, FALSE );
        /* test for no return from function returning value */
        TestZeroReturn();
    }

    /*
     * Return local variable space
     */
    Ctx.NextData = Ctx.Curfunc->ldata;

    /*
     * Restore caller's environment.
     */
    Ctx.Progptr = svprogptr;
    Ctx.Curfunc = func.fprev;
    Ctx.Curfunction = func.fvar;
    getoken();                          /* prepare for next statement */

} /* callfunc */


/* test for no return from function returning value */
void
VCLCLASS TestZeroReturn (void)
{
    if ( ! Ctx.Curfunc->fvar->ismain )
        if ( Ctx.Curfunc->fvar->type || Ctx.Curfunc->fvar->cat )
            error( NULLRETERR );
} /* TestZeroReturn */


void
VCLCLASS ArgumentList (int argc, ITEM *args)
{
    VARIABLE *      pvar;

    Assert( Ctx.Curfunction != NULL );

    pvar = Ctx.Curfunction->locals.vfirst;
    if ( pvar != NULL )
        while ( pvar->vkind & LABEL )
            pvar = pvar->vnext;

    Assert( Ctx.Curfunc != NULL );
    Ctx.Curfunc->ldata = Ctx.NextData;

    while ( argc )
    {
        void *          adr = rslva( args->value.cptr, args->lvalue );
        int             siz = rslvs( args->size, args->cat );
        char *          vdata;
        int             rsiz = siz;

        /*
         * if pvar is NULL or pvar -> auto variable, there are more
         * arguments than parameters
         */
        if ( pvar != NULL )
        {
            if ( pvar->islocal == 2 )
                rsiz = isPointer( pvar ) ? sizeof( void * ) : pvar->vwidth;
            if ( ( args->vconst & 2 ) && ( pvar->vconst & 2 ) == 0 )
                if ( isPointer( pvar ) )
                    error( CONSTARGERR );
        }

        /* coerce chars to ints as arguments */
        if ( rsiz == sizeof( char ) )
            rsiz = sizeof( int );

        vdata = (char *) GetDataSpace( rsiz, 0 );

        store( vdata, rsiz, args->isunsigned, adr, siz, args->isunsigned );

        Ctx.Curfunc->arglength += rsiz;

        if ( pvar != NULL )
            pvar = pvar->vnext;
        ++args;
        --argc;
    }

    GetDataSpace( Ctx.Curfunction->width, 0 );

} /* ArgumentList */
