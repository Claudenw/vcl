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
    preexpr.c - Preprocessor expression handling

 DESCRIPTION
    Handles preprocessor expressions and compiles macros.  Predefined
    symbols are processed dynamically.

 FUNCTIONS
    MacroExpression()
    ResolveMacro()

    CompileMacro()
    TestDefined()
    MacroDefined()
    MacroPrimary()
    MacroMultiplyDivide()
    MacroAddSubtract()
    MacroRelational()
    MacroEquality()
    MacroBoolAND()
    MacroBoolXOR()
    MacroBoolOR()
    MacroLogicalAND()
    MacroLogicalOR()

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
#include <time.h>
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
 * Compile a #define macro() with optional arguments
 */
void
VCLCLASS CompileMacro (uchar *wd, MACRO *mp, uchar **cp)
{
    char *          args[MAXPARMS];
    int             i;
    int             argno = 0;
    unsigned char * val;
    int             inString = FALSE;

    val = mp->val;

    if ( **cp != '(' )
        error( LPARENERR );

    /* pull the arguments out of the macro call */
    ( *cp )++;
    while ( **cp && **cp != ')' )
    {
        char *          ca = (char *) getmem( 255 );
        int             parens = 0;
        int             cs = 0;

        args[argno] = ca;
        bypassWhite( cp );
        while ( **cp )
        {
            if ( **cp == ',' && parens == 0 )
                break;
            else if ( **cp == '(' )
                parens++;
            else if ( **cp == ')' )
            {
                if ( parens == 0 )
                    break;
                --parens;
            }
            else if ( **cp == '"' || **cp == '\'' )
            {
                while ( **cp && **cp != ')' )
                {
                    if ( cs++ == 255 )
                        error( SYNTAXERR );
                    *ca++ = *( ( *cp )++ );
                    if ( **cp == '"' )
                        break;
                }
            }

            if ( cs++ == 255 )
                error( SYNTAXERR );
            *ca++ = *( ( *cp )++ );
        }
        *ca = '\0';
        argno++;
        if ( **cp == ',' )
            ( *cp )++;
    }

    /* build the statement substituting the arguments for the parameters */
    inString = FALSE;
    while ( *val )
    {
        if ( *val & 0x80 || ( *val == '#' && ! inString ) )
        {
            char *          arg;
            int             stringizing = 0;

            if ( *val == '#' )
            {
                val++;
                if ( *val == '#' )
                {
                    val++;
                    continue;
                }
                else
                {
                    *wd++ = '"';
                    stringizing = 1;
                }
            }
            arg = args[*val & 0x3f];
            while ( isspace( *arg ) )
                arg++;
            while ( *arg != '\0' )
                *wd++ = *arg++;
            if ( stringizing )
            {
                while ( isspace( *( wd - 1 ) ) )
                    --wd;
                *wd++ = '"';
            }
            val++;
        }
        else if ( ( *wd++ = *val++ ) == '"' )
            inString ^= 1;
    }
    *wd = '\0';

    for ( i = 0; i < argno; i++ )
        free( args[i] );
    if ( argno != mp->parms )
        error( ARGERR );
    if ( **cp != ')' )
        error( RPARENERR );
    ( *cp )++;
} /* CompileMacro */


/*
 * Resolve a macro to its #define value
 *
 * Macros with no value (defined only) resolve to 0.
 */
int
VCLCLASS ResolveMacro (uchar *wd, uchar **cp)
{
    uchar *         mywd;
    MACRO *         mp;
    int             sct = 0;
    int             predef;

    mywd = (uchar *) getmem( MAXMACROLENGTH );
    ExtractWord( wd, cp, (unsigned char *) "_" );
    while ( alphanum( *wd ) && ( mp = FindMacro( wd ) ) != NULL &&
            sct != MacroCount )
    {
        /* see if it's a predefined symbol */
        if ((predef = FindPreDefined( (char *) wd )) != 0)
        {
            switch (predef)
            {
                case PD_CDECL :
                    strcpy( (char *) wd, "1" );
                    break;
                case PD_VCL :
                    sprintf( (char *) wd, "0x%04x", PROGVERN );
                    break;
                case PD_DATE :
                    {
                        time_t      tim = time( NULL );

                        /* format: "Nov 20 1994" (including quotes) */
                        strcpy( (char *) wd, "\"" );
                        strftime( (char *) wd + 1, 13, "%b %d %Y", localtime( &tim ));
                        strcat( (char *) wd, "\"" );
                    }
                    break;
                case PD_FILE :
                    strcpy( (char *) wd, "\"" );
                    strcat( (char *) wd, SrcFileName( Ctx.CurrFileno ) );
                    strcat( (char *) wd, "\"" );
                    break;
                case PD_LINE :
                    sprintf( (char *) wd, "%d", Ctx.CurrLineno );
                    break;
                case PD_OS :
                    strcpy( (char *) wd, "1" );
                    break;
                case PD_TIME :
                    {
                        time_t      tim = time( NULL );

                        /* format: "hh:mm:ss" military time */
                        strcpy( (char *) wd, "\"" );
                        strftime( (char *) wd + 1, 10, "%H:%M:%S", localtime( &tim ));
                        strcat( (char *) wd, "\"" );
                    }
                    break;
                default:
                    /* internal logic error; switch() doesn't match array
                       Predefined[] in keyword.c */
                    break;
            }
        }
        else
        {
            *wd = '\0';

            /* standard macro resolution */
            if ( mp->val == NULL )      /* no value, done */
                break;
            /* is it a function macro? */
            if ( mp->isMacro )
            {
                unsigned char * mw = mywd;
                int             inString = 0;

                CompileMacro( mywd, mp, cp );
                while ( *mw )
                {
                    if ( *mw == '"' && ( mw == mywd || *( mw - 1 ) != '\\' ) )
                        inString ^= 1;
                    if ( ! inString && alphanum( *mw ) )
                    {
                        ResolveMacro( wd, &mw );
                        wd += (unsigned char) strlen( (char *) wd );
                    }
                    else
                        *wd++ = *mw++;
                }
                *wd = '\0';
            }
            else
                /* simply copy the value string */
                strcpy( (char *) wd, (char *) mp->val );
        }
        sct++;
    }
    free( mywd );
    return sct;
} /* ResolveMacro */


/*
 * Test whether expression is '[!] defined'
 *
 * Processing for #ifdef and #ifndef uses definedTest to coerce this
 * routine into thinking '[!] defined' is being used.
 *
 * returns: -1 == '! defined', 0 == not applicable, 1 == 'defined'
 */
int
VCLCLASS TestDefined (uchar **cpp)
{
    int             not = FALSE;
    uchar *         cp = *cpp;

    bypassWhite( &cp );

    /* see if this is a 'defined' test from ifdef or ifndef */
    if ( definedTest )
    {
        not = definedTest;
        definedTest = 0;                /* reset in case of complex expression */
        return not;                     /* use the not state */
    }

    /* see if next token is not */
    if ( *cp == '!' )
    {
        cp++;
        not = TRUE;
        bypassWhite( &cp );
    }
    ExtractWord( Word, &cp, (unsigned char *) "" );
    if ( strcmp( (char *) Word, "defined" ) == 0 )
    {
        *cpp = cp;                      /* 'defined' found, move pointer */
        return not ? -1 : 1;
    }
    return 0;
} /* TestDefined */


/*
 * Determine if a macro is defined
 */
int
VCLCLASS MacroDefined (uchar **cpp)
{
    int         paren = FALSE;

    bypassWhite( cpp );

    if ( **cpp == '(' )                 /* parenthesis on 'defined' is not */
    {                                   /* ..an expression */
        ++(*cpp);
        paren = TRUE;
        bypassWhite( cpp );
    }
    ExtractWord( Word, cpp, (unsigned char *) "_" );
    if ( ! strlen( (char *) Word ) )
        error( BADIFDEF );
    bypassWhite( cpp );
    if ( paren )                        /* if it was parenthesized make */
    {                                   /* ..sure it matches */
        if ( **cpp != ')' )
            error( RPARENERR );
        else
            ++(*cpp);
    }
    return ( FindMacro( Word ) != NULL );
} /* MacroDefined */


/*
 * MacroPrimary: highest precedence; bottom of the descent
 *
 * Recursive descent expression evaluation for #if
 */
int
VCLCLASS MacroPrimary (uchar **cp)
{
    int             result = 0;
    int             tok;

    bypassWhite( cp );
    if ( **cp == '(' )
    {
        /* parenthetical expression */
        ( *cp )++;
        result = MacroExpression( cp );
        if ( **cp != ')' )
            error( RPARENERR );
        ( *cp )++;
    }
    else if ( isdigit( **cp ) || **cp == '\'' )
    {
        /* numerical constant expression */
        char            num[80];
        char            con[80];
        unsigned char * ch = *cp;
        unsigned char * cc = (unsigned char *) con;

        while ( isdigit( *ch ) || strchr( ".Ee'xX", *ch ) )
            *cc++ = *ch++;
        *cc = '\0';
        tokenize( num, con );
        *cp += (unsigned char) ( cc - (unsigned char) con );
        switch ( *num )
        {
            case T_CHRCONST:
                result = *(unsigned char *) ( num + 1 );
                break;
            case T_INTCONST:
                result = *(int *) ( num + 1 );
                break;
            case T_UINTCONST:
                result = (int) *(unsigned int *) ( num + 1 );
                break;
            case T_LNGCONST:
                result = (int) *(long *) ( num + 1 );
                break;
            case T_ULNGCONST:
                result = (int) *(unsigned long *) ( num + 1 );
                break;
            default:
                error( CONSTEXPRERR );
        }
    }
    else if ( alphanum( **cp ) )
    {
        int             not;

        /* see if it's a 'defined' directive */
        /* returns: -1 == '! defined', 0 == not applicable, 1 == 'defined' */
        if ( ( not = TestDefined( cp ) ) != 0 )
        {
            int     isdef = MacroDefined( cp );

            result = ( not == -1 ) ? ! isdef : isdef;
        }
        else
        /* macro identifer expression */
        {
            uchar * np = (uchar *) getmem( MAXMACROLENGTH );
            uchar * npp = np;

            result = ( ResolveMacro( np, cp ) == 0 ) ? 0 : MacroPrimary( &npp );
            free( np );
        }
    }
    else
    {
        /* unary operators */
        tok = **cp;
        ( *cp )++;
        result = MacroPrimary( cp );
        switch ( tok )
        {
            case '+':
                break;
            case '-':
                result = -result;
                break;
            case '!':
                result = ! result;
                break;
            case '~':
                result = ~result;
                break;
            default:
                error( EXPRERR );
        }
    }
    bypassWhite( cp );
    return result;
} /* MacroPrimary */


/*
 * Multiplicative * and / operators
 */
int
VCLCLASS MacroMultiplyDivide (uchar **cp)
{
    int             result;
    int             iresult;
    int             op;

    result = MacroPrimary( cp );

    for ( ;; )
    {
        if ( **cp == '*' )
            op = 0;
        else if ( **cp == '/' )
            op = 1;
        else if ( **cp == '%' )
            op = 2;
        else
            break;
        ( *cp )++;
        iresult = MacroPrimary( cp );
        result = op == 0 ? ( result * iresult ) :
            op == 1 ? ( result / iresult ) :
            ( result % iresult );
    }
    return result;
} /* MacroMultiplyDivide */


/*
 * Additive + and - binary operators
 */
int
VCLCLASS MacroAddSubtract (uchar **cp)
{
    int             result;
    int             iresult;
    int             ad;

    result = MacroMultiplyDivide( cp );

    while ( **cp == '+' || **cp == '-' )
    {
        ad = **cp == '+';
        ( *cp )++;
        iresult = MacroMultiplyDivide( cp );
        result = ad ? ( result + iresult ) : ( result - iresult );
    }
    return result;
} /* MacroAddSubtract */


/*
 * Relational <, >, <=, and >= operators
 */
int
VCLCLASS MacroRelational (uchar **cp)
{
    int             result;
    int             iresult;

    result = MacroAddSubtract( cp );

    while ( **cp == '<' || **cp == '>' )
    {
        int             lt = **cp == '<';

        ( *cp )++;
        if ( **cp == '=' )
        {
            ( *cp )++;
            iresult = MacroAddSubtract( cp );
            result = lt ? ( result <= iresult ) :
                ( result >= iresult );
        }
        else
        {
            iresult = MacroAddSubtract( cp );
            result = lt ? ( result < iresult ) :
                ( result > iresult );
        }
    }
    return result;
} /* MacroRelational */


/*
 * Equality == and != operators
 */
int
VCLCLASS MacroEquality (uchar **cp)
{
    int             result;
    int             iresult;
    int             eq;

    result = MacroRelational( cp );

    while ( ( **cp == '=' || **cp == '!' ) && *( *cp + 1 ) == '=' )
    {
        eq = **cp == '=';
        ( *cp ) += 2;
        iresult = MacroRelational( cp );
        result = eq ? ( result == iresult ) : ( result != iresult );
    }
    return result;
} /* MacroEquality */


/*
 * Binary AND & operator
 */
int
VCLCLASS MacroBoolAND (uchar **cp)
{
    int             result;

    result = MacroEquality( cp );

    while ( **cp == '&' && *( *cp + 1 ) != '&' )
    {
        ( *cp ) += 2;
        result &= MacroEquality( cp );
    }
    return result;
} /* MacroBoolAND */


/*
 * Binary XOR ^ operator
 */
int
VCLCLASS MacroBoolXOR (uchar **cp)
{
    int             result;

    result = MacroBoolAND( cp );

    while ( **cp == '^' )
    {
        ( *cp )++;
        result ^= MacroBoolAND( cp );
    }
    return result;
} /* MacroBoolXOR */


/*
 * Binary OR | operator
 */
int
VCLCLASS MacroBoolOR (uchar **cp)
{
    int             result;

    result = MacroBoolXOR( cp );

    while ( **cp == '|' && *( *cp + 1 ) != '|' )
    {
        ( *cp ) += 2;
        result |= MacroBoolXOR( cp );
    }
    return result;
} /* MacroBoolOR */


/*
 * Logical AND && operator
 */
int
VCLCLASS MacroLogicalAND (uchar **cp)
{
    int             result;

    result = MacroBoolOR( cp );

    while ( **cp == '&' && *( *cp + 1 ) == '&' )
    {
        ( *cp ) += 2;
        result = MacroBoolOR( cp ) && result;
    }
    return result;
} /* MacroLogicalAND */


/*
 * Logical OR || operator
 */
int
VCLCLASS MacroLogicalOR (uchar **cp)
{
    int             result;

    result = MacroLogicalAND( cp );

    while ( **cp == '|' && *( *cp + 1 ) == '|' )
    {
        ( *cp ) += 2;
        result = MacroLogicalAND( cp ) || result;
    }
    return result;
} /* MacroLogicalOR */


/*
 * Top of the descent
 */
int
VCLCLASS MacroExpression (uchar **cp)
{
//    bypassWhite( cp );
    return MacroLogicalOR( cp );
} /* MacroExpression */
