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
    scanner.c - Lexical analysis routines

 DESCRIPTION
    Processes runtime token values, tokenizes the source into pcode and
    tokenizes constants.

 FUNCTIONS
    getoken()
    tokenize()
    skip()
    isProto()
    uncesc()
    fltnum()
    intnum()

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
#include <limits.h>
#include <string.h>
#include <math.h>
#include <mem.h>
#include <dos.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

#undef isxdigit

/*
 * Get the next operable pcode token
 *
 * Processes any file/line or space tokens
 */
int
VCLCLASS getoken (void)
{
    if ( Ctx.Progptr != NULL )
    {
        /* search for a token */
        for ( ;; )
        {
            switch ( Ctx.Token = *Ctx.Progptr++ )
            {
                case T_LINENO:
                    Ctx.CurrFileno = *Ctx.Progptr++;
                    Ctx.CurrLineno = *(int *) Ctx.Progptr;
                    Ctx.Progptr += sizeof( int );
                    break;
                case ' ':
                    break;
                case T_EOF:
                    Ctx.Value.ival = *Ctx.Progptr--;
                    isStruct = 0;
                    return Ctx.Token;
                case T_SYMBOL:
                    Ctx.Value.ival = *(int *) Ctx.Progptr;
                    Ctx.Curvar = SearchVariable( Ctx.Value.ival, isStruct );
                    if ( ! isStruct && Ctx.Curvar == NULL )
                        Ctx.Curvar = SearchVariable( Ctx.Value.ival, 1 );
                    Ctx.Progptr += sizeof( int );
                    isStruct = 0;
                    return Ctx.Token;
                case T_IDENTIFIER:
                    isStruct = 0;
                    Ctx.Curvar = (VARIABLE *) MK_FP( FP_SEG( VariableMemory ),
                                        *(unsigned *) Ctx.Progptr );
                    Ctx.Progptr += sizeof( int );
                    return Ctx.Token;
                case T_FUNCTION:
                    Ctx.Curfunction = FindFunction( *(int *) Ctx.Progptr );
                    Ctx.Progptr += sizeof( int );
                    return Ctx.Token;
                case T_FUNCTREF:
                    Ctx.Curfunction = FunctionMemory + *(int *) Ctx.Progptr;
                    Ctx.Progptr += sizeof( int );
                    return Ctx.Token;
                case T_CHRCONST:
                    Ctx.Value.ival = *Ctx.Progptr++;
                    return Ctx.Token;
                case T_STRCONST:
                    Ctx.Value.cptr = (char *) Ctx.Progptr + 1;
                    /* length byte = string length including null + 1 */ 
                    Ctx.Progptr += *Ctx.Progptr;
                    return Ctx.Token;
                case T_INTCONST:
                    Ctx.Value.ival = *( (int *) Ctx.Progptr );
                    Ctx.Progptr += sizeof( int );
                    return Ctx.Token;
                case T_UINTCONST:
                    Ctx.Value.uival = *( (unsigned int *) Ctx.Progptr );
                    Ctx.Progptr += sizeof( int );
                    return Ctx.Token;
                case T_LNGCONST:
                    Ctx.Value.lval = *( (long *) Ctx.Progptr );
                    Ctx.Progptr += sizeof( long );
                    return Ctx.Token;
                case T_ULNGCONST:
                    Ctx.Value.ulval = *( (unsigned long *) Ctx.Progptr );
                    Ctx.Progptr += sizeof( long );
                    return Ctx.Token;
                case T_FLTCONST:
                    Ctx.Value.fval = *( (double *) Ctx.Progptr );
                    Ctx.Progptr += sizeof( double );
                    return Ctx.Token;
                case T_STRUCT:
                case T_UNION:
                    isStruct = 1;
                    return Ctx.Token;
                default:
                    isStruct = 0;
                    return Ctx.Token;
            }
        }
    }
    else
    {
        /* The program pointer was NULL so don't search for tokens */
        isStruct = Ctx.Token = 0;
        return Ctx.Token;
    }
} /* getoken */


/*
 * Lexical analyzer converts C code in srcbuf to tokens in tknbuf
 */
int
VCLCLASS tokenize (char *tknbuf, char *srcp)
{
    char *          start;
    char *          laststring = NULL;
    char *          cp;
    char            c;
    char            c2;
    char            c3;
    char            op;
    int             i;
    int             BraceCount = 0;
    char *          tknptr = tknbuf;
    int             sawCond = 0;
    int             sawCase = 0;

    while ( *srcp )
    {
        /* handle 3 character operators '<<= and >>=' */
        if ( ( i = FindOperator( srcp ) ) != 0 )
        {
            srcp += 2;
            if ( ( i == T_SHL || i == T_SHR ) && *srcp == '=' )
            {
                srcp++;
                i |= OPASSIGN;          /* set multi-op assign bit */
            }
            *tknptr++ = i;
            continue;
        }

        c = *srcp++;
        c &= 0x7f;                      /* strip hibit reserved for multi-op */
        op = 0;
        c2 = *srcp;
        c3 = *( srcp + 1 );
        if ( c != '"' && c != '\n' )
            laststring = NULL;

        switch ( c )
        {
            case '\n':
                {
                    /*-
                     * File/Line (command-line option)
                     *  _____________
                     * |  T_LINENO   |
                     * |_____________|
                     * |fileno (byte)|
                     * |_____________|
                     * |             |
                     * |lineno (word)|
                     * |             |
                     * |_____________|
                     */
                    /* handshake(); to keep D-Flat clock ticking */
                    if ( ! rtopt.NoLineNumbers )
                    {
                        *tknptr++ = T_LINENO;
                        Ctx.CurrFileno = atoi( srcp + 2 );
                        *tknptr++ = (unsigned char) Ctx.CurrFileno;
                    }
                    else
                        Ctx.CurrFileno = 0;
                    srcp = strchr( srcp, '@' );
                    Assert( srcp != NULL );
                    srcp++;
                    if ( ! rtopt.NoLineNumbers )
                    {
                        Ctx.CurrLineno = atoi( srcp );
                        *(int *) tknptr = Ctx.CurrLineno;
                        tknptr += sizeof( int );
                    }
                    else
                        Ctx.CurrLineno = 0;
                    srcp = strchr( srcp, '/' );
                    Assert( srcp != NULL );
                    srcp++;
                    break;
                }
            case '"':
                /*-
                 * String constant
                 *  ___________
                 * | T_STRCONST|
                 * |___________|
                 * |   length  |
                 * |___________|
                 * |   char    |
                 * |___________|
                 * |     .     |
                 * |___________|
                 * |     0     |
                 * |___________|
                 */
                if ( laststring != NULL )
                    /* concatenated string */
                    tknptr = laststring + strlen( laststring );
                else
                {
                    *tknptr++ = T_STRCONST;
                    laststring = tknptr++;
                }
                /* copy unescaped string */
                for ( i = 0; ( c = *srcp ) != '"' && c; ++i )
                {
                    if ( i >= 255 )
                        error( STRTOOLONG );
                    *tknptr++ = uncesc( &srcp );
                }
                *tknptr++ = '\0';
                /* length byte = string length including null + 1 */
                *laststring = (char) ( tknptr - laststring );
                if ( c )
                    ++srcp;
                break;
            case '\'':
                /*-
                 * Character constant
                 *  ___________
                 * | T_CHRCONST|
                 * |___________|
                 * |   value   |
                 * |___________|
                 */
                *tknptr++ = T_CHRCONST;
                *tknptr++ = uncesc( &srcp );

                /*
                 * Skip to delimiting apostrophe or end of line.
                 */
                while ( ( c = *srcp++ ) != '\'' && c )
                    ;
                if ( ! c )
                    --srcp;
                break;
            case '=':
                /*-
                 * Equality & assignment operators
                 *  ___________
                 * |  op token |
                 * |___________|
                 */
                if ( op )               /* multi-op, e.g. '+=' */
                {
                    tknptr[-1] |= OPASSIGN; /* set multi-op bit */
                    ++srcp;
                }
                else if ( c2 == '=' )
                {
                    *tknptr++ = T_EQ;
                    ++srcp;
                }
                else
                    *tknptr++ = T_ASSIGN;
                break;
                /*-
                 * All kinds of other operators
                 *  ___________
                 * |  op token |
                 * |___________|
                 */
            case '*':                   /* opAssign'able operators */
            case '^':
            case '%':
            case '&':
            case '|':
            case '+':
            case '-':
            case '/':
                op = c;
            case '!':                   /* non-opAssign operators */
            case '<':
            case '>':
            case '[':
            case ']':
            case '(':
            case ')':
            case ',':
            case '~':
            case ' ':
            case ';':
                *tknptr++ = c;
                break;
            case '?':
                sawCond++;
                *tknptr++ = c;
                break;
            case ':':
                if ( sawCond )
                    --sawCond;
                sawCase = 0;
                *tknptr++ = c;
                break;
            case '{':
                BraceCount++;
                *tknptr++ = c;
                break;
            case '}':
                --BraceCount;
                *tknptr++ = c;
                break;
            case '.':
                if ( c2 == '.' && c3 == '.' )
                {
                    *tknptr++ = T_ELLIPSE;
                    srcp += 2;
                }
                else if ( isdigit( c2 ) )
                {
                    /*
                     * floating pointer number
                     */
                    --srcp;
                    fltnum( &srcp, &tknptr );
                }
                else
                    *tknptr++ = c;
                break;
            default:
                if ( isdigit( c ) )
                {
                    /*-
                     * Some type of numeric
                     *  ___________
                     * | T_INTCONST| (T_LNGCONST, T_FLTCONST, T_UINTCONST, etc)
                     * |___________|
                     * |   value   | <- binary value of the
                     * |___________|    number. Number of
                     * |     .     |    bytes depends on type
                     * |___________|
                     * |     .     |
                     * |___________|
                     * |     .     |
                     * |___________|
                     */
                    --srcp;
                    intnum( &srcp, &tknptr );
                }
                else if ( alphanum( c ) )
                {
                    /*
                     * identifier
                     */
                    start = cp = tknptr + 2;    // Use the tknptr+2 as a
                                                // scratch area
                    --srcp;
                    while ( alphanum( *srcp ) )
                        *cp++ = *srcp++;
                    *cp++ = 0;

                    if ( ( i = FindKeyword( start ) ) != 0 )
                    {
                        /*-
                         * keyword
                         *  ___________
                         * | key token |
                         * |___________|
                         */
                        *tknptr++ = i;
                        if ( i == T_CASE )
                            sawCase = 1;
                    }
                    else if ( ! sawCond && ! sawCase && *srcp == ':' )
                    {
                        /*
                         * label for gotos
                         */
                        VARIABLE        var;
                        VARIABLE *      lvar;

                        NullVariable( &var );
                        var.vkind = LABEL;
                        var.vsymbolid = AddSymbol( start );
                        var.vcat = BraceCount;
                        lvar = InstallVariable( &var, &Ctx.Curfunction->locals, 0, 0, 1, 0 );
                        lvar->voffset = (int) ( tknptr - tknbuf );
                        srcp++;
                    }
                    else
                    {
                        /*
                         * symbol, function declaration, prototype, or call?
                         */
                        char *          sp;
                        FUNCTION *      funcp;
                        int             fsymbol;

                        /* !?! adds any symbol following a type in a function
                           prototype, e.g. int myFunc (int aaa), aaa is placed
                           in the symbol table.  Doesn't hurt anything, but
                           takes memory */ 
                        fsymbol = AddSymbol( start );

                        /* end-of-line, scan past next fileno/lineno */
                        sp = srcp;
                        if ( *sp == '\n' )
                        {
                            sp = strchr( sp + 2, '/' );
                            Assert( sp != NULL );
                            ++sp;
                        }

                        if ( BraceCount == 0 && *sp == '(' )
                        {
                            FUNCTION        func;

                            /*-
                             * function instance
                             *
                             * declaration ("int myFunc (type args) {") or
                             * prototype ("int myFunc (type);")
                             *  _____________
                             * | T_FUNCTION  |
                             * |_____________|
                             * |symbol offset|
                             * |             |
                             * |_____________|
                             */
                            if ( ( funcp = FindFunction( fsymbol ) ) == NULL )
                            {
                                /* function not declared or prototyped */
                                NullFunction( &func );
                                /*
                                 * this data is taken from the first instance
                                 * whether it's the prototype or declaration.
                                 */
                                func.symbol = fsymbol;
                                func.libcode = SearchLibrary( start );
                                func.ismain = ( strcmp( start, "main" ) == 0 );
                                funcp = NextFunction;
                                /* install it */
                                InstallFunction( &func );
                            }
                            *tknptr++ = T_FUNCTION;
                            *(int *) tknptr = fsymbol;
                            tknptr += sizeof( int );
                            if ( isProto( sp ) )
                            {
                                /* set location of function prototype */
                                funcp->protofileno = Ctx.CurrFileno;
                                funcp->protolineno = Ctx.CurrLineno;
                            }
                            else        /* it's the function declaration */
                            {
                                /* set location of function declaration */
                                funcp->fileno = Ctx.CurrFileno;
                                funcp->lineno = Ctx.CurrLineno;
                                /* set new current function */
                                Ctx.Curfunction = funcp;
                            }
                        }
                        else if ( ( funcp = FindFunction( fsymbol ) ) != NULL )
                        {
                            /*-
                             * function reference
                             *
                             * call (BraceCount > 0) or
                             * address ("functionPtr = myFunc;")
                             *  _____________
                             * | T_FUNCTREF  |
                             * |_____________|
                             * | Function No |
                             * |             |
                             * |_____________|
                             */
                            *tknptr++ = T_FUNCTREF;
                            *(unsigned *) tknptr = (unsigned) ( funcp - FunctionMemory );
                            tknptr += sizeof( unsigned );
                        }
                        else
                        {
                            /*-
                             * variable reference
                             *  _____________
                             * |  T_SYMBOL   |
                             * |_____________|
                             * |symbol offset|
                             * |             |
                             * |_____________|
                             */
                            *tknptr++ = T_SYMBOL;
                            *(int *) tknptr = fsymbol;
                            tknptr += sizeof( int );
                        }
                    }
                }
                else
                    /* Bad character in input line */
                    error( LEXERR );
        }

        if ( *srcp == '=' && op )
        {
            tknptr[-1] |= OPASSIGN;     /* set multi-op bit */
            ++srcp;
        }
    }

    *tknptr++ = T_EOF;                  /* mark the end-of-file */
    *tknptr = '\0';

    return (int) ( tknptr - tknbuf );
} /* tokenize */


/*-
 * Determine if a function symbol is a prototype
 *
 * Argument cp should be positioned on a left parenthesis.
 * The BraceCount should be 0 for prototypes or declarations.
 *
 * Scans for the matching right parenthesis; if the next
 * character is a semi-colon it's a prototype, otherwise a
 * function declaration.
 *
 * This routine understands function declarations and
 * prototypes spread across multiple lines.
 */
int
VCLCLASS isProto (char *cp)
{
    int     aproto = FALSE;

    /* end-of-line, scan past next fileno/lineno */
    if ( *cp == '\n' )
    {
        cp = strchr( cp + 2, '/' );
        Assert( cp != NULL );
        ++cp;
    }

    /* if next is parenthesis it's a function */
    if ( *cp == '(' )
    {
        int     parens = 1;

        /* scan for matching parenthesis */
        for ( ++cp; *cp; ++cp )
        {
            /* scan past next fileno/lineno */
            if ( *cp == '\n' )
            {
                cp = strchr( cp + 2, '/' );
                Assert( cp != NULL );
                cp++;
            }
            /* match casts, etc. */
            if ( *cp == '(' )
                ++parens;
            /* opening parenthesis matched, or match casts, etc. */
            if ( *cp == ')' )
                if ( --parens == 0 )
                    break;
        }
        if ( *cp != ')' )
            error( RPARENERR );
        ++cp;                           /* incr past closing parenthesis */

        /* end-of-line, scan past next fileno/lineno */
        if ( *cp == '\n' )
        {
            cp = strchr( cp + 2, '/' );
            Assert( cp != NULL );
            ++cp;
        }

        /* if next is semi-colon it's a prototype */
        if ( *cp == ';' )
            aproto = TRUE;
        else if ( *cp != '{' )
            error( LBRACERR );
    }

    return aproto;
} /* isProto */


/*
 * Unescape character escapes
 */
int
VCLCLASS uncesc (char **bufp)
{
    char *          buf;
    char            c;

    buf = *bufp;
    if ( ( c = *buf++ ) == '\\' )
    {
        int             i;
        char            n[4];

        switch ( c = *buf++ )
        {
            case 'a':
                c = '\a';
                break;
            case 'b':
                c = '\b';
                break;
            case 'f':
                c = '\f';
                break;
            case 'n':
                c = '\n';
                break;
            case 'r':
                c = '\r';
                break;
            case 't':
                c = '\t';
                break;
            case 'v':
                c = '\v';
                break;
            case '\\':
                c = '\\';
                break;
            case '\'':
                c = '\'';
                break;
            case '"':
                c = '"';
                break;
            case 'x':
                sscanf( buf, "%x", &i );
                c = i;
                while ( isxdigit( *buf ) )
                    buf++;
                break;
            default:
                if ( isdigit( c ) )
                {
                    --buf;
                    for ( i = 0; i < 3 && isdigit( *buf ); ++i )
                        n[i] = *buf++;
                    n[i] = 0;
                    sscanf( n, "%o", &i );
                    c = i;
                }
                break;
        }
    }
    *bufp = buf;

    return c;
} /* uncesc */


/*
 * Skip balanced delimiters and everything in between
 */
void
VCLCLASS skip (char left, char right)
{
    int             parity;
    unsigned char * svprogptr;

    parity = 1;
    svprogptr = Ctx.Progptr;
    while ( getoken() != T_EOF )
    {
        if ( Ctx.Token == left )
            ++parity;
        else if ( Ctx.Token == right )
            --parity;
        if ( ! parity )
        {
            Ctx.svpptr = Ctx.Progptr;
            getoken();
            return;
        }
    }
    Ctx.Progptr = svprogptr;
    error( RBRACERR );
} /* skip */


/*
 * Parse a floating point number
 */
void
VCLCLASS fltnum (char **srcstr, char **tknstr)
{
    char *          srcp;
    char *          cp;
    char            numbuf[65];
    char            c;
    char            n;
    char            dot;
    char            e;
    char            sign;
    double          f;

    n = dot = e = sign = 0;
    srcp = *srcstr;
    **tknstr = T_FLTCONST;
    ++( *tknstr );

    while ( *srcp )
    {
        if ( ( c = *srcp++ ) == '.' )
        {
            if ( dot )
            {
                /*
                 * Already saw a dot.
                 */
                --srcp;
                break;
            }
            ++dot;
        }
        else if ( c == 'e' || c == 'E' )
        {
            if ( ! ( dot || n ) || e )
            {
                /*
                 * 'E' does not immediately follow a dot or number.
                 */
                --srcp;
                break;
            }
            ++e;
        }
        else if ( c == '+' || c == '-' )
        {
            if ( e != 1 || sign )
            {
                /*
                 * Sign does not immediately follow an 'E'
                 */
                --srcp;
                break;
            }
            ++sign;
        }
        else if ( isdigit( c ) )
        {
            ++n;
            if ( e )
            {
                /*
                 * number follows an 'E' - don't allow the sign anymore.
                 */
                ++e;
            }
        }
        else
        {
            --srcp;
            break;
        }
    }
    /*
     * copy the number into a local buffer and NULL terminate it.
     */
    n = 0;
    cp = *srcstr;
    while ( cp < srcp )
        numbuf[n++] = *cp++;
    numbuf[n] = 0;

    f = atof( numbuf );
    movmem( &f, *tknstr, sizeof( double ) );
    *srcstr = srcp;
    *tknstr += sizeof( double );
} /* fltnum */


/*
 * Parse a decimal, octal or hexadecimal number
 */
void
VCLCLASS intnum (char **srcstr, char **tknstr)
{
    char *          srcp;
    char            c;
    unsigned long   l;
    int             isDecimal = TRUE;
    int             isUnsigned = FALSE;
    int             isLong = FALSE;

    /*
     * check for floating point value
     */
    srcp = *srcstr;
    while ( isdigit( *srcp ) )
        ++srcp;
    if ( *srcp == '.' || *srcp == 'e' || *srcp == 'E' )
    {
        fltnum( srcstr, tknstr );
        return;
    }

    /* not floating point */
    c = T_INTCONST;
    srcp = *srcstr;
    if ( *srcp++ == '0' )
    {
        if ( isdigit( *srcp ) )
        {
            /*
             * octal constant
             */
            sscanf( srcp, "%lo", &l );
            while ( isdigit( *srcp ) )  /* pass over digits */
                ++srcp;
            isDecimal = FALSE;
        }
        else if ( tolower( *srcp ) == 'x' )
        {
            /*
             * hexadecimal constant
             */
            sscanf( ++srcp, "%lx", &l );
            while ( isxdigit( *srcp ) ) /* pass over HEX digits */
                ++srcp;
            isDecimal = FALSE;
        }
    }

    if ( isDecimal )
    {
        /*
         * decimal integer number
         */
        sscanf( --srcp, "%lu", &l );
        while ( isdigit( *srcp ) )      /* pass over digits */
            ++srcp;
    }

    /* check first suffix character */
    if ( *srcp == 'u' || *srcp == 'U' )
    {
        isUnsigned = TRUE;
        ++srcp;
    }
    if ( *srcp == 'l' || *srcp == 'L' )
    {
        isLong = TRUE;
        ++srcp;
    }

    /* check second suffix character */
    if ( *srcp == 'u' || *srcp == 'U' )
    {
        if ( isUnsigned )
            error( UNSCONSTSUFF );
        isUnsigned = TRUE;
        ++srcp;
    }
    if ( *srcp == 'l' || *srcp == 'L' )
    {
        if ( isLong )
            error( LNGCONSTSUFF );
        isLong = TRUE;
        ++srcp;
    }

    /* promote type based on ANSI rules */
    if ( ! isUnsigned && ! isLong )     /* unsuffixed */
    {
        if ( isDecimal )                /* decimal */
        {
            if ( l > LONG_MAX )
            {
                warning( CONSTISUNS );
                isUnsigned = TRUE;
            }
            if ( l > SHRT_MAX || (long) l < SHRT_MIN )
            {
                warning( CONSTISLNG );
                isLong = TRUE;
            }
        }
        else                            /* octal or hex */
        {
            if ( l > SHRT_MAX && l <= USHRT_MAX )
            {
                warning( CONSTISUNS );
                isUnsigned = TRUE;
            }
            else
            {
                if ( l > LONG_MAX )
                {
                    warning( CONSTISUNS );
                    isUnsigned = TRUE;
                }
                if ( l > USHRT_MAX )
                {
                    warning( CONSTISLNG );
                    isLong = TRUE;
                }
            }
        }
    }
    else                                /* suffixed */
    {
        if ( isUnsigned )               /* unsigned */
        {
            if ( l > USHRT_MAX )
            {
                if ( ! isLong )
                {
                    warning( CONSTISLNG );
                    isLong = TRUE;
                }
            }
        }
        else                            /* long */
        {
            if ( l > LONG_MAX )
            {
                if ( ! isUnsigned )
                {
                    warning( CONSTISUNS );
                    isUnsigned = TRUE;
                }
            }
        }
    }

    /* set the pseudocode token */
    if ( isLong )
    {
        if ( isUnsigned )
            c = T_ULNGCONST;            /* unsigned long constant */
        else
            c = T_LNGCONST;             /* long constant */
    }
    else
    {
        if ( isUnsigned )
            c = T_UINTCONST;            /* unsigned int constant */
        else
            c = T_INTCONST;             /* int constant */
    }

    *srcstr = srcp;                     /* update source string pointer */

    **tknstr = c;                       /* store the token */
    ++( *tknstr );                      /* incr the token string pointer */

    /* store the value & incr the token string point accordingly */
    if ( isLong )
    {
        if ( isUnsigned )
            *( (unsigned long *) *tknstr ) = (unsigned long) l;
        else
            *( (long *) *tknstr ) = (long) l;
        *tknstr += sizeof( long );
    }
    else
    {
        if ( isUnsigned )
            *( (unsigned int *) *tknstr ) = (unsigned int) l;
        else
            *( (int *) *tknstr ) = (int) l;
        *tknstr += sizeof( int );
    }
} /* intnum */
