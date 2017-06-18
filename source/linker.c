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
    linker.c - Declarations linker

 DESCRIPTION
    Handles linking global variables and building and testing function
    prototypes.

 FUNCTIONS
    link()
    istypespec()
    AddPro()
    linkerror()
    isTypeDeclaration()

    isParameterType()
    isLocalType()
    TypeDeclaration()
    FunctionPrototype()
    AddStructProto()
    BuildPrototype()
    TestPrototype()
    FunctionDeclaration()
    isPcodeProto()
    CheckDeclarations()
    ConvertIdentifier()
    InnerDeclarations()
    LocalDeclarations()
    arglist()

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
#include <dos.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

/*
 * Allocate global variables
 */
void
VCLCLASS link (VARIABLELIST *vartab)
{
    protocat = 0;
    protoreturn = INT;
    fconst = 0;
    Linking = TRUE;

    Ctx.Progptr = Progstart;
    getoken();

    while ( Ctx.Token != T_EOF )
    {
//      handshake();   /* to keep D-Flat clock ticking */

        /* type declaration; function declaration, prototype or reference */
        isTypeDeclaration();

        /* immediate '(' is function pointer, which is a type */
        if ( Ctx.Token == T_LPAREN || getoken() != T_LPAREN )
        {
            if ( Ctx.Token != T_FUNCTREF && Ctx.Token != T_FUNCTION )
            {
                /* data type declaration */
                TypeDeclaration( vartab );
                continue;               /* keep going */
            }
        }

        /* function declaraction, prototype or reference */
        FunctionPrototype();
        if ( Ctx.Token == T_FUNCTREF || Ctx.Token == T_FUNCTION )
            FunctionDeclaration( vartab );
    }

    CheckDeclarations();
    Linking = FALSE;
} /* link */


int
VCLCLASS istypespec (void)
{
    if ( isSymbol() )
        return ( Ctx.Curvar && isTypedef( Ctx.Curvar ) );

    return strchr( typespecs, Ctx.Token ) != NULL;
} /* istypespec */


int
VCLCLASS isParameterType (void)
{
    return ( istypespec() ||
             Ctx.Token == T_VOID ||
             Ctx.Token == T_CONST ||
             Ctx.Token == T_VOLATILE ||
             Ctx.Token == T_ELLIPSE );
} /* isParameterType */


int
VCLCLASS isLocalType (void)
{
    return ( istypespec() ||
             Ctx.Token == T_STATIC ||
             Ctx.Token == T_VOID ||
             Ctx.Token == T_EXTERNAL ||
             Ctx.Token == T_CONST ||
             Ctx.Token == T_REGISTER ||
             Ctx.Token == T_AUTO ||
             Ctx.Token == T_VOLATILE );
} /* isLocalType */


/*
 * Add to a function's prototype
 */
void
VCLCLASS AddPro (unsigned char pro)
{
    if ( NextProto >= (unsigned char *) ( PrototypeMemory + vclCfg.MaxPrototype ) )
        error( OMERR );
    *++NextProto = pro;
} /* AddPro */


void
VCLCLASS linkerror (int errnum)
{
    Ctx.Progptr = errptr;
    error( errnum );
} /* linkerror */


/*
 * Test for type declaration and, if so, scan up to identifier
 */
int
VCLCLASS isTypeDeclaration (void)
{
    int             done = 0;
    int             istypedecl = 0;

    Ctx.svpptr = Ctx.Progptr;
    Ctx.svToken = Ctx.Token;
    Ctx.svCurvar = Ctx.Curvar;
    Ctx.svCurrLineno = Ctx.CurrLineno;
    Ctx.svisStruct = isStruct;
    errptr = Ctx.Progptr;

    while ( ! done )
    {
        switch ( Ctx.Token )
        {
            case T_SYMBOL:
            case T_IDENTIFIER:
                if ( Ctx.Curvar == NULL || ! isTypedef( Ctx.Curvar ) )
                {
                    done = 1;
                    break;
                }
            case T_CONST:
            case T_STATIC:
            case T_VOLATILE:
            case T_TYPEDEF:
            case T_VOID:
            case T_CHAR:
            case T_INT:
            case T_LONG:
            case T_FLOAT:
            case T_DOUBLE:
            case T_SHORT:
            case T_UNSIGNED:
                istypedecl = 1;
                getoken();
                break;
            case T_STRUCT:
            case T_UNION:
            case T_ENUM:
                istypedecl = 1;
                getoken();
                if ( isSymbol() )
                    getoken();
                break;
            default:
                done = 1;
                break;
        }
    }
    if ( istypedecl )
        while ( Ctx.Token == T_PTR )
            getoken();
    return istypedecl;
} /* isTypeDeclaration */


void
VCLCLASS TypeDeclaration (VARIABLELIST *vartab)
{
    VARIABLE *      var;

    Ctx.Progptr = Ctx.svpptr;
    Ctx.Token = Ctx.svToken;
    Ctx.Curvar = Ctx.svCurvar;
    Ctx.CurrLineno = Ctx.svCurrLineno;
    isStruct = Ctx.svisStruct;
    Ctx.svCurrLineno = 0;
    var = DeclareVariable( vartab, 0, 0, 0, 0 );
    if ( vartab == &Globals && ( var->vqualifier & ( REGISTER | AUTO ) ) )
        error( DECLERR );
    stmtend();
    fconst = 0;
} /* TypeDeclaration */


void
VCLCLASS FunctionPrototype (void)
{
    VARIABLE        FuncVariable; 
    int             sawunsigned;
    char            tokn;

    Ctx.Progptr = Ctx.svpptr;
    Ctx.Token = Ctx.svToken;
    Ctx.Curvar = Ctx.svCurvar;
    Ctx.CurrLineno = Ctx.svCurrLineno;
    isStruct = Ctx.svisStruct;
    Ctx.svCurrLineno = 0;

    NullVariable( &FuncVariable );
    FuncVariable.vtype = INT;
    FuncVariable.vsize = sizeof( int );

    TypeQualifier( &FuncVariable );
    tokn = Ctx.Token;
    if ( Ctx.Token != T_FUNCTION && Ctx.Token != T_FUNCTREF )
        getoken();
    tokn = MakeTypeToken( tokn, &sawunsigned );

    /* establish prototyped return value */
    protoreturn = MakeType( tokn );
    if ( tokn == T_STRUCT || tokn == T_UNION || tokn == T_ENUM )
    {
        if ( Ctx.Token != T_SYMBOL )
            linkerror( NOIDENTERR );
        getoken();
    }
    TypeQualifier( &FuncVariable );

    /* establish return type indirection */
    protocat = 0;
    while ( Ctx.Token == T_PTR )
    {
        protocat++;                   /* incr type indirection */
        getoken();
    }
    if ( Ctx.Token != T_FUNCTION && Ctx.Token != T_FUNCTREF )
        linkerror( NOIDENTERR );
} /* FunctionPrototype */


void
VCLCLASS AddStructProto (void)
{
    *(VARIABLE **) ( NextProto + 1 ) = Ctx.Curvar->vstruct;
    NextProto += sizeof( VARIABLE ** );
    if ( NextProto > (unsigned char *) ( PrototypeMemory + vclCfg.MaxPrototype ) )
        error( OMERR );
} /* AddStructProto */


/*
 * Build the parameter prototype
 *
 * Prototypes used by callfunc()
 */
void
VCLCLASS BuildPrototype (char stopToken, int isproto)
{
    char            donetype = FALSE;
    char            indir = 0;
    char            lastproto = 0;  /* 0=none, 1=qualifier, 2=type, 3=symbol */
    char            sawunsigned = FALSE;

    while ( Ctx.Token != stopToken && Ctx.Token != T_EOF )
    {
        if ( istypespec() || Ctx.Token == T_ELLIPSE || Ctx.Token == T_VOID )
        {
            if ( lastproto >= 2 )
            {
                strcpy( ErrorMsg, "misplaced type" );
                linkerror( SYNTAXERR );
            }
            lastproto = 2;
            if ( donetype )
            {
                AddPro( indir );
                indir = 0;
            }
            if ( isSymbol() )
            {
                /* argument is a typedef */
                AddPro( Ctx.Curvar->vtype );
                if ( Ctx.Curvar->vtype == STRUCT || Ctx.Curvar->vtype == UNION )
                    AddStructProto();
                indir = Ctx.Curvar->vcat;
                getoken();
            }
            else                        /* it's a type */
            {
                int             done = FALSE;
                char            st = 0;
                char            tokn = 0;

                /* setup the complete type */
                while ( ! done )
                {
                    switch ( Ctx.Token )
                    {
                        case T_UNSIGNED:
                            if ( st > 0 )
                            {
                                strcpy( ErrorMsg, "misplaced 'unsigned'" );
                                linkerror( SYNTAXERR );
                            }
                            sawunsigned = TRUE;
                            tokn = T_INT;
                            st = 1;
                            getoken();
                            break;
                        case T_SHORT:
                            if ( st > 1 )
                            {
                                strcpy( ErrorMsg, "misplaced 'short'" );
                                linkerror( SYNTAXERR );
                            }
                            tokn = T_INT;
                            st = 2;
                            getoken();
                            break;
                        case T_CHAR:
                        case T_DOUBLE:
                        case T_FLOAT:
                        case T_LONG:
                            if ( st > 2 )
                            {
                                sprintf( ErrorMsg, "misplaced '%s'",
                                    ( Ctx.Token == T_CHAR ) ? "char" :
                                    ( Ctx.Token == T_DOUBLE ) ? "double" :
                                    ( Ctx.Token == T_FLOAT ) ? "float" : "long" );
                                linkerror( SYNTAXERR );
                            }
                            tokn = Ctx.Token;
                            st = 3;
                            getoken();
                            break;
                        case T_INT:
                            if ( st > 2 && tokn != T_LONG )
                            {
                                strcpy( ErrorMsg, "misplaced 'int'" );
                                linkerror( SYNTAXERR );
                            }
                            if ( tokn != T_LONG )
                                tokn = T_INT;
                            st = 3;
                            getoken();
                            break;
                        case T_STRUCT:
                        case T_UNION:
                        case T_ENUM:
                        case T_ELLIPSE:
                        case T_VOID:
                            if ( st > 0 )
                            {
                                sprintf( ErrorMsg, "misplaced '%s'",
                                    ( Ctx.Token == T_STRUCT ) ? "struct" :
                                    ( Ctx.Token == T_UNION ) ? "union" :
                                    ( Ctx.Token == T_ENUM ) ? "enum" :
                                    ( Ctx.Token == T_ELLIPSE ) ? "..." : "void" );
                                linkerror( SYNTAXERR );
                            }
                            tokn = Ctx.Token;
                            st = 3;
                            getoken();
                            break;
                        default:
                            done = TRUE;
                            break;
                    }
                }

                /* !?! unsigned values are not tracked in prototypes.  The
                   problem is, prototyping isn't consistent.  Add unsigned
                   as a prototype attribute, then change TestPrototype() and
                   the runtime prototyping testing to accommodate it. */
                sawunsigned = sawunsigned;

                AddPro( MakeType( tokn ) );
                if ( tokn == T_STRUCT || tokn == T_UNION )
                {
                    getoken();
                    AddStructProto();
                }

                /* void only valid as a pointer (* or []) */
                if ( tokn == T_VOID &&
                     Ctx.Token != T_PTR &&
                     Ctx.Token != T_LBRACKET )
                {
                    /* unless it's the only argument */
                    if ( Ctx.Token == stopToken && ! donetype )
                        ;               /* empty */
                    else
                        linkerror( BADTYPEVOID );
                }
                else if ( ! isproto )
                {
                    /* if ANSI-style function declaration */
                    if ( stopToken == T_RPAREN )
                    {
                        /* if next token is not a symbol or type */
                        if ( tokn != T_ELLIPSE &&
                             ( Ctx.Token != T_SYMBOL && ! istypespec() ) &&
                             ( Ctx.Token != T_PTR && Ctx.Token != T_LBRACKET ) )
                            linkerror( MISSINGNAME );
                    }
                }
                if ( Ctx.Token == stopToken )
                    break;
            }
            donetype = TRUE;
        }
        else                            /* not a type */
        {
            char        t;

            t = Ctx.Token;

            /* track/check this and last prototype */
            if ( t == T_CONST || t == T_VOLATILE )
            {
                if ( lastproto != 0 && lastproto != 2 )
                {
                    strcpy( ErrorMsg, "misplaced const or volatile" );
                    linkerror( SYNTAXERR );
                }
                lastproto = 1;
                getoken();
            }
            else if ( isSymbol() )      /* variable */
            {
                if ( lastproto != 2 )
                {
                    strcpy( ErrorMsg, "misplaced symbol" );
                    linkerror( SYNTAXERR );
                }
                lastproto = 3;
                getoken();
            }

            /* check for comma or semicolon */
            if ( lastproto != 1 && lastproto != 3 && t != stopToken &&
                 t != T_PTR && t != T_LBRACKET && t != T_RBRACKET )
            {
                if ( stopToken == T_RPAREN ) /* if ANSI */
                {
                    if ( t != T_COMMA )
                        linkerror( COMMAEXPECTED );
                }
                else
                {
                    if ( t != T_SEMICOLON )
                        linkerror( SEMIERR );
                }
            }
        }

        /* keep the indirection straight */
        if ( Ctx.Token == T_PTR || Ctx.Token == T_LBRACKET )
        {
            if ( Ctx.Token == T_PTR && ( lastproto >= 3 || lastproto < 2 ) )
            {
                strcpy( ErrorMsg, "misplaced '*'" );
                linkerror( SYNTAXERR );
            }
            else if ( lastproto < 2 )
            {
                strcpy( ErrorMsg, "misplaced '['" );
                linkerror( SYNTAXERR );
            }
            indir++;
            getoken();
        }

        /* reset last prototype between arguments */
        if ( Ctx.Token == T_COMMA || Ctx.Token == T_SEMICOLON )
            lastproto = 0;

        if ( Ctx.Token == T_COMMA || Ctx.Token == T_SEMICOLON ||
             Ctx.Token == T_RBRACKET )
            getoken();
    }
    AddPro( indir );

    if ( Ctx.Token == T_EOF )
        linkerror( LBRACERR );
} /* BuildPrototype */


void
VCLCLASS TestPrototype (uchar *pros, int type, int fcat)
{
    unsigned char * pr1;
    unsigned char * pr2;

    pr1 = pros;
    pr2 = (unsigned char *) Ctx.Linkfunction->proto;
    while ( *pr1 != 0xff )
    {
        if ( *pr1 != *pr2 )
            linkerror( REDEFERR );
        ++pr1;
        ++pr2;
        if ( *( pr1 - 1 ) == STRUCT || *( pr1 - 1 ) == UNION )
        {
            if ( *(VARIABLE **) pr1 != *(VARIABLE **) pr2 )
                linkerror( REDEFERR );
            pr1 += sizeof( VARIABLE ** );
            pr2 += sizeof( VARIABLE ** );
        }
    }
    /* make sure they both have the same number of arguments */
    if ( *pr2 != 0xff )
        linkerror( REDEFERR );

    /* make sure the return types match */
    if ( Ctx.Linkfunction->type != type || protocat != fcat )
        linkerror( REDEFERR );
} /* TestPrototype */


/*
 * Function declaration
 *
 * Handles actual declaration, and global prototypes and references
 */
void
VCLCLASS FunctionDeclaration (VARIABLELIST *vartab)
{
    int             fcat;
    int             fileno;
    VARIABLE *      Funcvar;
    char            isproto;
    int             lineno;
    unsigned char * pros;
    char            stopToken;
    VARIABLE *      svNextvar;
    char            tokn;
    int             type;


    tokn = Ctx.Token;

    while ( Ctx.Token == T_FUNCTREF || Ctx.Token == T_FUNCTION )
    {
        Assert( Ctx.Curfunction != NULL );
        Ctx.Linkfunction = Ctx.Curfunction;
        Ctx.Linkfunction->type = protoreturn;
        Ctx.Linkfunction->fconst = fconst;
        lineno = Ctx.CurrLineno;
        fileno = Ctx.CurrFileno;
        errptr = Ctx.Progptr;
        type = Ctx.Linkfunction->type;
        fcat = Ctx.Linkfunction->cat;

        if ( getoken() != T_LPAREN )
            linkerror( LPARENERR );

        /* determine if current function is a prototype */
        isproto = isPcodeProto();

        /* declare local variables for declared function arguments */
        svNextvar = Ctx.NextVar;
        if ( ! isproto )                /* don't bother if a prototype */
            arglist();

        /* determine style of declaration */
        getoken();
        /* if it's a type then it's ANSI */
        if ( isParameterType() || Ctx.Token == T_PTR )
            stopToken = T_RPAREN;       /* ANSI declarator */
        else
            stopToken = T_LBRACE;       /* K&R declarator */

        /* build the prototype */
        pros = NextProto + 1;
        if ( Ctx.Token != T_RPAREN )
            BuildPrototype( stopToken, isproto );
        else
        {
            /* func(), insert void type and 0 indirection */
            AddPro( VOID );
            AddPro( 0 );
        }
        AddPro( 0xff );

        /* the prototype scan stopped at a left brace, K&R style */
        if ( Ctx.Token == T_LBRACE )
            break;

        /* the prototype argument scan stopped at right parenthesis */
        getoken();
        /* could be prototype, '{' for declaration, (or array of funcs?) */
        if ( Ctx.Token != T_SEMICOLON && Ctx.Token != T_COMMA )
            break;

        /* function prototype */
        Ctx.svpptr = Ctx.Progptr;
        Ctx.Linkfunction->fileno = fileno;
        Ctx.Linkfunction->lineno = lineno;
        if ( tokn != T_FUNCTION )
        {
            /* delete argument declarations */
            Ctx.Linkfunction->locals.vfirst = NULL;
            Ctx.Linkfunction->locals.vlast = NULL;
            Ctx.NextVar = svNextvar;
            Assert( Ctx.NextVar != NULL );
            if ( Ctx.NextVar->vprev )
                Ctx.NextVar->vprev->vnext = NULL;
        }
        if ( Ctx.Linkfunction->proto != NULL )
        {
            /* already have a prototype, test it */
            TestPrototype( pros, type, fcat );
            /* don't need this prototype any more */
            // !?! NextProto = pros - 1;
        }
        else
        {
            Ctx.Linkfunction->proto = (char *) pros;
            Ctx.Linkfunction->cat = protocat;
        }

        if ( Ctx.Token == T_SEMICOLON )
            break;

        /* handle arrays of function pointer references !?! */
        getoken();
        if ( Ctx.Token != T_FUNCTREF && Ctx.Token != T_FUNCTION )
            error( FUNCNAMERR );
    }

    if ( Ctx.Token == T_SEMICOLON )
        getoken();
    else
    {
        /* function declaration */
        if ( vartab != &Globals )
            error( RBRACERR );
        while ( Ctx.Token != T_LBRACE && Ctx.Token != T_EOF )
            getoken();
        if ( Ctx.Token != T_LBRACE )
            linkerror( LBRACERR );

        /* installed function must be a prototype */
        if ( Ctx.Linkfunction->code != NULL )
            linkerror( MDEFERR );
        Ctx.Linkfunction->code = Ctx.Progptr - 1;
        Ctx.Linkfunction->fileno = fileno;
        Ctx.Linkfunction->lineno = lineno;
        if ( Ctx.Linkfunction->proto != NULL )
        {
            /* already have a prototype, test it */
            TestPrototype( pros, type, fcat );
            /* don't need this prototype any more */
            // !?! NextProto = pros - 1;
        }
        else
        {
            /* if there's no prototype use the function declaration */
            Ctx.Linkfunction->proto = (char *) pros;
            Ctx.Linkfunction->cat = protocat;
        }

        /*-
         * local declarations
         *
         * declare local variables; pass through all function
         * statements, recursively declaring locals of inner
         * blocks and converting symbols to identifiers
         */
        Assert( Ctx.Token == T_LBRACE );
        LocalDeclarations();

        /* calculate total width of this function's data space */
        Funcvar = Ctx.Linkfunction->locals.vfirst;
        while ( Funcvar != NULL )
        {
            if ( ! Funcvar->vstatic && Funcvar->islocal == 1 )
                Ctx.Linkfunction->width += Funcvar->vwidth;
            Funcvar = Funcvar->vnext;
        }
    }

    protocat = 0;
    protoreturn = INT;
    Ctx.Linkfunction = NULL;
} /* FunctionDeclaration */


/*
 * Determine if current function is a prototype
 *
 * Ctx.Token must be positioned to opening left parenthesis.
 */
int
VCLCLASS isPcodeProto (void)
{
    int             fileno;
    int             lineno;
    int             parens = 1;         /* count the opening left paren */
    int             ret = FALSE;
    uchar *         savptr;

    lineno = Ctx.CurrLineno;
    fileno = Ctx.CurrFileno;
    savptr = Ctx.Progptr;
    getoken();                          /* eat the opening left paren */

    while ( parens > 0 && Ctx.Token != T_EOF )
    {
        if ( Ctx.Token == T_LPAREN )
            ++parens;
        else if ( Ctx.Token == T_RPAREN )
            --parens;
        getoken();
    }

    if ( Ctx.Token == T_SEMICOLON )
        ret = TRUE;
    else if ( Ctx.Token != T_LBRACE )
        linkerror( LBRACERR );

    Ctx.CurrLineno = lineno;
    Ctx.CurrFileno = fileno;
    Ctx.Progptr = savptr;

    return ret;
} /* isPcodeProto */


void
VCLCLASS CheckDeclarations (void)
{
    VARIABLE *      var = Globals.vfirst;
    FUNCTION *      Function = FunctionMemory;

    while ( var != NULL )
    {
        if ( var->vqualifier & EXTERNAL )
        {
            Ctx.CurrFileno = var->fileno;
            Ctx.CurrLineno = var->lineno;
            error( UNRESOLVEDERR );
        }
        var = var->vnext;
    }
    while ( Function < NextFunction )
    {
        int             braces = 0;

        Ctx.CurrFileno = Function->fileno;
        Ctx.CurrLineno = Function->lineno;

        if ( Function->libcode == 0 && Function->code == NULL )
            error( UNDEFUNCERR );

        Ctx.Progptr = (uchar *) Function->code;
        do
        {
            switch ( getoken() )
            {
                case T_LBRACE:
                    braces++;
                    break;
                case T_RBRACE:
                    --braces;
                    break;
                case T_ENUM:
                    getoken();
                    break;
                case T_SYMBOL:
                    getoken();
                    error( Ctx.Token == T_LPAREN ? NOFUNCERR : DECLARERR );
                default:
                    break;
            }
        } while ( braces != 0 );
        Function++;
    }
} /* CheckDeclarations */


/*
 * Convert T_SYMBOL to T_IDENTIFIER
 */
void
VCLCLASS ConvertIdentifier (void)
{
    if ( Ctx.Curvar != NULL )
    {
        *( Ctx.Progptr - ( 1 + sizeof( int ) ) ) = T_IDENTIFIER;
        *(unsigned *) ( Ctx.Progptr - sizeof( int ) ) = FP_OFF( Ctx.Curvar );
    }
} /* ConvertIdentifier */


/*
 * Pass through block's statements, recursively declaring locals
 * of inner blocks and converting symbols to identifiers
 */
void
VCLCLASS InnerDeclarations (int inStruct)
{
    VARIABLELIST *  cstruct = NULL;
    VARIABLELIST    svcs;

    while ( Ctx.Token != T_RBRACE && Ctx.Token != T_EOF )
    {
        if ( Ctx.Token == T_ARROW || Ctx.Token == T_DOT )
        {
            svcs = Ctx.Curstruct;
            Ctx.Curstruct = *cstruct;
            while ( getoken() != T_SYMBOL )
                if ( Ctx.Token == T_EOF )
                    break;
            if ( Ctx.Token != T_SYMBOL )
                error( ELEMERR );
            cstruct = NULL;
            Ctx.Curstruct = svcs;
        }
        if ( Ctx.Token == T_STRUCT || Ctx.Token == T_UNION )
        {
            getoken();
            svcs = Ctx.Curstruct;
            Ctx.Curstruct = *(VARIABLELIST *) &Ctx.Curvar->velem;
            while ( Ctx.Token == T_SYMBOL )
            {
                ConvertIdentifier();
                getoken();
            }
            if ( Ctx.Token == T_LBRACE )
            {
                getoken();
                InnerDeclarations( TRUE );  /* recurse for structure */
            }
            Ctx.Curstruct = svcs;
        }
        if ( Ctx.Token == T_LBRACE )
        {
            if ( ! inStruct )
                LocalDeclarations();        /* recurse for next block */
            continue;
        }
        if ( Ctx.Token == T_SYMBOL )
        {
            ConvertIdentifier();
            if ( Ctx.Curvar && Ctx.Curvar->velem.vfirst )
                cstruct = (VARIABLELIST *) &( Ctx.Curvar->velem );
        }
        getoken();
    }
    if ( Ctx.Token != T_RBRACE )
        error( RBRACERR );
} /* InnerDeclarations */


void
VCLCLASS LocalDeclarations (void)
{
    VARIABLE *      svBlkvar = Blkvar;
    unsigned char * svProgptr = Ctx.Progptr;

    getoken();

    /*
     * manage scope by limiting search for erroneous duplicate
     * definitions to within a block
     */

    /* point to the last auto variable */
    if ( Ctx.Linkfunction->BlkNesting++ )
        Blkvar = Ctx.Linkfunction->locals.vlast;

    while ( isLocalType() )
    {
        DeclareVariable( &Ctx.Linkfunction->locals, 0, 0,
                         Ctx.Linkfunction->BlkNesting, 0 );
        stmtend();
    }

    /*
     * pass through block's statements, recursively declaring locals
     * of inner blocks and converting symbols to identifiers
     */
    Ctx.Progptr = svProgptr;
    getoken();
    InnerDeclarations( FALSE );

    --( Ctx.Linkfunction->BlkNesting );

    Blkvar = svBlkvar;
    getoken();
} /* LocalDeclarations */


/*
 * Declare local variables for function arguments
 */
void
VCLCLASS arglist (void)
{
    VARIABLE        var;
    int             svl = Ctx.CurrLineno;
    unsigned char * svp = Ctx.Progptr;

    /*
     * Parse the function argument list, starting with the left paren.
     */
    if ( getoken() == T_VOID )
    {
        if ( getoken() == T_RPAREN )
        {
            Ctx.Progptr = svp;
            Ctx.CurrLineno = svl;
            return;                     /* void parameter block */
        }
    }
    Ctx.Progptr = svp;
    Ctx.CurrLineno = svl;
    getoken();
    if ( isSymbol() && ! isTypedef( Ctx.Curvar ) )
    {
        /*
         * It's a K&R style declaration
         */    
        do
        {
            /*
             * Build a pseudo variable table entry and install it
             */
            NullVariable( &var );
            var.vsymbolid = Ctx.Value.ival;
            var.vtype = INT;
            var.vsize = sizeof( int );
            InstallVariable( &var, &Ctx.Linkfunction->locals, 0, 1, 1, 0 );
            /*
             * If next token is a comma, there's more to follow
             */
            if ( getoken() != T_COMMA )
                break;
            getoken();
        } while ( isSymbol() );
        if ( Ctx.Token != T_RPAREN )
            error( RPARENERR );
        getoken();
        /*
         * change each of the argument's characteristics according to the
         * argument declaration list.
         */
        while ( istypespec() || Ctx.Token == T_VOID || Ctx.Token == T_CONST )
        {
            DeclareVariable( 0, 0, 1, 1, 0 );   /* (vartbl = 0 = search
                                                 * locals to update) */
            if ( Ctx.Token != T_SEMICOLON )
                error( SEMIERR );
            getoken();
        }
    }
    else
    {
        /*
         * It's an ANSI style declaration with types
         */
        while ( istypespec() || Ctx.Token == T_VOID ||
                Ctx.Token == T_CONST )
        {
            /*
             * Build a variable table entry and install it
             */
            DeclareVariable( &Ctx.Linkfunction->locals, 0, 1, 1, 1 );
            /*
             * If next token == comma, more arguments follow
             */
            if ( Ctx.Token != T_COMMA )
                break;
            getoken();
        }
        if ( Ctx.Token == T_ELLIPSE )
        {
            getoken();
            if ( Ctx.Token != T_RPAREN )
                error( ELLIPSERR );
        }
        if ( Ctx.Token != T_RPAREN )
            error( RPARENERR );
        getoken();
    }
    Ctx.Progptr = svp;
    Ctx.CurrLineno = svl;
} /* arglist */
