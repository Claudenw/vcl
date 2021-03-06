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
    stmt.c - Interpreter statement processing

 DESCRIPTION
    Interprets keywords, initializers, and recursive statement skipping.

 FUNCTIONS
    statement()
    stmtbegin()
    stmtend()

    DoStatement()
    InitializeLocalVariables()
    skipstatement()

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
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

/*
 * Execute one statement or block
 * return false if longjmp (goto) occurred
 */
int
VCLCLASS DoStatement (void)
{
    if ( Ctx.Curfunc->BlkNesting < MAXNESTS )
        if ( setjmp( gotojmp[Ctx.Curfunc->BlkNesting] ) != 0 )
            return 0;
    setjmp( stmtjmp.jb );
    stmtjmp.jmp_ctx = Ctx;
    statement();
    return 1;
} /* DoStatement */


/*
 * Evaluate a statement
 */
void
VCLCLASS statement (void)
{
    unsigned char * repeat;
    unsigned char * body;
    unsigned char * iterate;
    int             svlineno;
    int             svfileno;
    int             argc;
    int             i;

    if ( Ctx.Token != T_LBRACE && Ctx.Token != T_RBRACE )
        stmtbegin();

    if ( isTypeDeclaration() )
        error( BADVARERR );
    switch ( Ctx.Token )
    {
        case T_LBRACE:
            /*
             * parse a compound statement
             */
            Ctx.svpptr = Ctx.Progptr;
            getoken();

            Assert( Ctx.Curfunc != NULL );

            /* supports the debugger */
            Ctx.Curfunc->BlkNesting++;

            while ( istypespec() || Ctx.Token == T_CONST ||
                    Ctx.Token == T_VOLATILE || Ctx.Token == T_VOID ||
                    Ctx.Token == T_REGISTER || Ctx.Token == T_AUTO ||
                    Ctx.Token == T_STATIC || Ctx.Token == T_EXTERNAL )
            {
                InitializeLocalVariables();
                stmtend();
            }

            /* loop executing statements until return, break, continue, and
               goto nesting not exceeded or the closing right brace */
            while ( ! ( Saw_return || Saw_break || Saw_continue ) &&
                    ( GotoNesting || Ctx.Token != T_RBRACE ) )
                DoStatement();

            if ( Saw_break || Saw_continue )
                while ( Ctx.Token != T_RBRACE && Ctx.Token != T_EOF )
                    getoken();

            if ( Ctx.Token == T_RBRACE )
                getoken();
            else if ( ! Saw_return )
                error( RBRACERR );

            /* supports the debugger */
            --Ctx.Curfunc->BlkNesting;
            break;

        case T_RBRACE:
            getoken();
            if ( GotoNesting )
                if ( --GotoNesting == Ctx.Curfunc->BlkNesting )
                    GotoNesting = 0;
            break;

        case T_GOTO:
            if ( getoken() != T_IDENTIFIER )
                error( GOTOERR );
            if ( Ctx.Curvar == NULL )
                error( GOTOERR );

            GotoOffset = Ctx.Curvar->voffset;
            getoken();
            stmtend();
            Ctx.Progptr = Progstart + GotoOffset;
            getoken();
            Saw_break = Saw_return = Saw_continue = 0;
            if ( Ctx.Curvar->vcat > Ctx.Curfunc->BlkNesting )
                GotoNesting = Ctx.Curvar->vcat;
            else if ( Ctx.Curvar->vcat < Ctx.Curfunc->BlkNesting )
                if ( Ctx.Curvar->vcat < MAXNESTS )
                    longjmp( gotojmp[Ctx.Curvar->vcat], 1 );
            break;

        case T_IF:
            /*
             * parse an "if-else" statement
             */
            if ( getoken() != T_LPAREN )
                error( LPARENERR );
            getoken();
            argc = expression();
            if ( Ctx.Token != T_RPAREN )
                error( RPARENERR );
            getoken();
            if ( popnint( argc ) )
            {
                if ( ! DoStatement() )
                    break;
                if ( Ctx.Token == T_ELSE )
                {
                    getoken();
                    skipstatement();
                }
            }
            else
            {
                skipstatement();
                if ( Ctx.Token == T_ELSE )
                {
                    getoken();
                    DoStatement();
                }
            }
            break;

        case T_ELSE:
            getoken();
            skipstatement();
            break;

        case T_WHILE:
            {
                int             dost = 1;

                /*
                 * parse a "while" statement
                 */
                ++Ctx.Looping;
                repeat = Ctx.Progptr;
                svlineno = Ctx.CurrLineno;
                svfileno = Ctx.CurrFileno;
                for ( ;; )
                {
                    if ( getoken() != T_LPAREN )
                        error( LPARENERR );

                    stmtbegin();
                    getoken();
                    argc = expression();
                    if ( Ctx.Token != T_RPAREN )
                        error( RPARENERR );

                    if ( popnint( argc ) )
                    {
                        body = Ctx.Progptr;
                        getoken();
                        if ( ! ( dost = DoStatement() ) )
                            break;
                        if ( Saw_return || Saw_break )
                        {
                            Ctx.Progptr = body;
                            Saw_break = 0;
                            break;
                        }
                        Ctx.Progptr = repeat;
                        Ctx.CurrLineno = svlineno;
                        Ctx.CurrFileno = svfileno;
                        Saw_continue = 0;
                    }
                    else
                        break;
                }
                --Ctx.Looping;
                if ( dost )
                {
                    getoken();
                    skipstatement();
                }
                break;
            }

        case T_DO:
            /*
             * Parse a "do-while" statement:
             */
            ++Ctx.Looping;
            repeat = Ctx.Progptr;
            svlineno = Ctx.CurrLineno;
            svfileno = Ctx.CurrFileno;
            for ( ;; )
            {
                stmtbegin();
                getoken();
                if ( ! DoStatement() )
                    break;
                if ( Ctx.Token != T_WHILE )
                    error( WHILERR );
                if ( getoken() != T_LPAREN )
                    error( LPARENERR );
                getoken();
                argc = expression();
                if ( Ctx.Token != T_RPAREN )
                    error( RPARENERR );
                getoken();
                stmtend();

                if ( popnint( argc ) )
                {
                    if ( Saw_return || Saw_break )
                    {
                        Saw_break = 0;
                        break;
                    }
                    Ctx.CurrLineno = svlineno;
                    Ctx.CurrFileno = svfileno;
                    Ctx.Progptr = repeat;
                    Saw_continue = 0;
                }
                else
                    break;
            }
            --Ctx.Looping;
            break;

        case T_FOR:
            {
                int             dost = 1;

                /*-
                 * Parse a "for" statement:
                 *
                 *   for (<initialization> ; <test> ; <iteration>)
                 *      <statement>
                 */
                if ( getoken() != T_LPAREN )
                    error( LPARENERR );
                stmtbegin();
                if ( getoken() != T_SEMICOLON )
                {
                    /*
                     * Do <initialization> part
                     */
                    popn( expression() );
                }
                if ( Ctx.Token != T_SEMICOLON )
                    error( SEMIERR );

                /*
                 * Progptr now points to <test> part - save this address.
                 */
                ++Ctx.Looping;
                svlineno = Ctx.CurrLineno;
                svfileno = Ctx.CurrFileno;
                repeat = Ctx.Progptr;
                for ( ;; )
                {
                    stmtbegin();
                    if ( getoken() == T_SEMICOLON )
                    {
                        /*
                         * Missing <test> part means it's always TRUE.
                         */
                        pushint( argc = 1, FALSE );
                    }
                    else
                    {
                        /*
                         * Do the <test> part.
                         */
                        argc = expression();
                        if ( Ctx.Token != T_SEMICOLON )
                            error( SEMIERR );
                    }

                    /*
                     * Progptr now points to <iteration> part, skip over it
                     * now.
                     */
                    iterate = Ctx.Progptr;
                    getoken();          /* get rid of ';' */
                    while ( Ctx.Token != T_RPAREN && Ctx.Token != T_EOF )
                    {
                        if ( Ctx.Token == T_LPAREN )
                            skip( T_LPAREN, T_RPAREN );
                        else
                            getoken();
                    }
                    if ( Ctx.Token != T_RPAREN )
                    {
                        Ctx.Progptr = iterate;
                        getoken();
                        error( RPARENERR );
                    }

                    /*
                     * Check <test> part for non-zero value
                     */
                    if ( popnint( argc ) )
                    {
                        /*
                         * Do <statement> part, the body of the loop.
                         */
                        body = Ctx.Progptr;
                        getoken();
                        if ( ! ( dost = DoStatement() ) )
                            break;
                        if ( Saw_return || Saw_break )
                        {
                            /*
                             * Encountered either a "return", a "break" or an
                             * error; break out of loop.
                             */
                            Ctx.Progptr = body;
                            Saw_break = 0;
                            break;
                        }
                        /*
                         * Now go back to <iteration> part and do it.
                         */
                        Ctx.Progptr = iterate;
                        if ( getoken() != T_RPAREN )
                            popn( expression() );

                        Saw_continue = 0;
                        Ctx.CurrLineno = svlineno;
                        Ctx.CurrFileno = svfileno;
                        Ctx.Progptr = repeat;
                    }
                    else
                        break;
                }
                --Ctx.Looping;
                if ( dost )
                {
                    getoken();
                    skipstatement();
                }
                break;
            }

        case T_SWITCH:
            {
                int             dost = 1;

                /*
                 * Parse a "switch" statement
                 */
                if ( getoken() != T_LPAREN )
                    error( LPARENERR );
                getoken();
                i = popnint( expression() );
                if ( Ctx.Token != T_RPAREN )
                    error( RPARENERR );

                body = Ctx.Progptr;     /* just after ')' */
                getoken();              /* get rid of ')' */

                if ( Ctx.Token == T_LBRACE )
                    getoken();          /* get rid of '{' */

                ++Ctx.Switching;
                while ( ! ( Saw_break || Saw_return ) && Ctx.Token != T_RBRACE )
                {
                    if ( Ctx.Token == T_CASE )
                    {
                        getoken();
                        argc = expression();
                        if ( Ctx.Token != T_COLON )
                            error( COLONERR );
                        getoken();
                        if ( i == popnint( argc ) )
                        {
                            while ( ! ( Saw_break || Saw_continue || Saw_return ) &&
                                    Ctx.Token != T_RBRACE )
                            {
                                if ( ! ( dost = DoStatement() ) )
                                    break;
                            }
                        }
                        else
                        {
                            if ( Ctx.Token != T_CASE && Ctx.Token != T_DEFAULT )
                                skipstatement();
                        }
                    }
                    else if ( Ctx.Token == T_DEFAULT )
                    {
                        if ( getoken() != T_COLON )
                            error( COLONERR );
                        getoken();
                        while ( ! ( Saw_break || Saw_return ) && Ctx.Token != T_RBRACE )
                            if ( ! ( dost = DoStatement() ) )
                                break;
                    }
                    else
                        skipstatement();
                }
                --Ctx.Switching;
                Saw_break = 0;
                if ( dost )
                {
                    Ctx.Progptr = body;
                    getoken();
                    skipstatement();
                }
                break;
            }

        case T_CASE:
        case T_DEFAULT:
            skipstatement();
            break;

        case T_RETURN:
            /*
             * parse a "return" statement
             */
            if ( getoken() == T_SEMICOLON )
            {
                /* return; */
                TestZeroReturn();
                pushint( 0, FALSE );
            }
            else
            {
                /* return val; */
                if ( Ctx.Curfunc != NULL )
                {
                    if ( Ctx.Curfunc->fvar->type == VOID &&
                         Ctx.Curfunc->fvar->cat == 0 )
                        error( VOIDRETERR );
                }
                expression();

            }
            ++Saw_return;
            stmtend();
            break;

        case T_BREAK:
            /*
             * parse a "break" statement
             */
            if ( Ctx.Looping || Ctx.Switching )
            {
                getoken();
                ++Saw_break;
            }
            else
                error( BREAKERR );
            stmtend();
            break;

        case T_CONTINUE:
            /*
             * parse a "continue" statement
             */
            if ( Ctx.Looping )
            {
                getoken();
                ++Saw_continue;
            }
            else
                error( CONTERR );
            stmtend();
            break;

        case T_SEMICOLON:
            stmtend();
            break;

        default:
            /*
             * parse an expression statement
             */
            if ( ExpressionOne() )
                pop();
            if ( Ctx.Token == T_RPAREN || Ctx.Token == T_RBRACKET )
                error( BADLBRACERR );
            stmtend();
    }
} /* statement */


void
VCLCLASS InitializeLocalVariables (void)
{
    while ( Ctx.Token != T_SEMICOLON )
    {
        while ( Ctx.Token == T_STATIC ||
                Ctx.Token == T_CONST ||
                Ctx.Token == T_AUTO ||
                Ctx.Token == T_REGISTER ||
                Ctx.Token == T_VOLATILE )
            getoken();
        if ( Ctx.Token == T_STRUCT ||
             Ctx.Token == T_UNION ||
             Ctx.Token == T_ENUM )
        {
            getoken();                  /* get a struct/union/enum tag or a { */
            if ( Ctx.Token == T_IDENTIFIER )
                getoken();              /* get the one past the tag */
            if ( Ctx.Token == T_LBRACE )
                skip( T_LBRACE, T_RBRACE );
        }
        else
            getoken();

        if ( Ctx.Token == T_IDENTIFIER && ! Ctx.Curvar->vstatic )
        {
            getoken();
            while ( Ctx.Token == T_LBRACKET )
            {
                do
                    getoken();
                while ( Ctx.Token != T_RBRACKET );
                getoken();
            }
            if ( Ctx.Token == T_ASSIGN )
            {
                VARIABLE *      pvar = Ctx.Curvar;

                Assert( pvar != NULL );
                stmtbegin();
                getoken();              /* get initializer expression */
                Initializer( pvar, (char *) DataAddress( pvar ), 0 );
            }
        }
    }
} /* InitializeLocalVariables */


/*
 * Beginning of statement processing.
 * Stop at every line and activate debugger.
 */
void
VCLCLASS stmtbegin (void)
{
#if DEBUGGER
    int             kbhit(void);

    if ( StepCount == 0 )
    {
        int             debugger(void);

        if ( debugger() )
        {
            pushint( 0 );
            longjmp( Shelljmp, 2 );
        }
    }
    kbhit();
#endif
} /* stmtbegin */


/*
 * Handle end of statement
 */
void
VCLCLASS stmtend (void)
{
    if ( ! longjumping )
    {
        Ctx.svpptr = Ctx.Progptr;
        if ( Ctx.Token == T_SEMICOLON )
            getoken();
        else
            error( SEMIERR );
    }
} /* stmtend */


/*
 * Skip a statement
 */
void
VCLCLASS skipstatement (void)
{
    switch ( Ctx.Token )
    {
        case T_LBRACE:
            /*
             * skip a compound statement
             */
            skip( T_LBRACE, T_RBRACE );
            break;

        case T_IF:
            /*
             * skip an "if-else" statement
             */
            getoken();                  /* skip 'if' */
            skip( T_LPAREN, T_RPAREN );
            skipstatement();
            if ( Ctx.Token == T_ELSE )
            {
                getoken();              /* skip 'else' */
                skipstatement();
            }
            break;

        case T_DO:
            /*
             * skip a "do-while" statement
             */
            getoken();                  /* skip 'do' */
            skipstatement();
            getoken();                  /* skip 'while' */
            skip( T_LPAREN, T_RPAREN );
            stmtend();
            break;

        case T_WHILE:
        case T_FOR:
        case T_SWITCH:
            /*
             * skip a "while", "for" or "switch" statement
             */
            getoken();                  /* skip 'while' */
            skip( T_LPAREN, T_RPAREN );
            skipstatement();
            break;

        case T_CASE:
        case T_DEFAULT:
            while ( getoken() != T_COLON )
                ;
            getoken();
            break;

        default:
            /*
             * skip a one-liner
             */
            while ( Ctx.Token != T_SEMICOLON && Ctx.Token != T_RBRACE && Ctx.Token != T_EOF )
                getoken();
            if ( Ctx.Token == T_EOF )
                error( RBRACERR );
            if ( Ctx.Token == T_SEMICOLON )
                getoken();
    }
} /* skipstatement */
