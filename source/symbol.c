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
    symbol.c - Symbol table routines.

 DESCRIPTION
    Contains routines for declaring and manipulating function symbols and
    runtime variables.

    Handles runtime dataspace management, auto variable declarations and
    auto variable initialization.

 FUNCTIONS
    FindFunction()
    InstallFunction()
    SearchVariable()
    InstallVariable()
    ArrayElements()
    ArrayDimensions()
    VariableWidth()
    TypeQualifier()
    DeclareVariable()
    MakeTypeToken()
    Initializer()
    MakeType()
    TypeSize()
    GetDataSpace()
    AllocVariable()
    DataAddress()

    FindVariable()
    DefineEnum()
    DeclareEnum()
    DeclareStructure()
    DeclareTypedef()
    DeclareNative()
    strucdef()
    varlist()
    initexpr()
    SetType()
    Declarator()

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
#include <mem.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

VCLCLASS FUNCTION *
VCLCLASS FindFunction (int fsymbolid)
{
    FUNCTION *      FirstFunction = FunctionMemory;

    while ( FirstFunction != NULL && FirstFunction < NextFunction )
    {
        if ( fsymbolid == FirstFunction->symbol )
            return FirstFunction;
        FirstFunction++;
    }
    return NULL;
} /* FindFunction */


void
VCLCLASS InstallFunction (FUNCTION *funcp)
{
    if ( NextFunction == FunctionMemory + vclCfg.MaxFunctions )
        error( TOOMANYFUNCERR );
    ++FunctionsCount;
    *NextFunction++ = *funcp;
} /* InstallFunction */


/*
 * Search for a symbol name in the given variable
 * linked list (variable table "vartab").
 */
VCLCLASS VARIABLE *
VCLCLASS FindVariable (int symbolid, VARIABLELIST *vartab,
              int BlkNesting, VARIABLE *Stopper, int isStruct)
{
    VARIABLE *      tvar = vartab->vlast;

    while ( tvar != NULL )
    {
        if ( tvar == Stopper )
        {
            tvar = NULL;
            break;
        }
        if ( BlkNesting >= tvar->vBlkNesting )
            if ( symbolid == tvar->vsymbolid )
                if ( isStruct == ( tvar->velem.vfirst != NULL ) )
                    break;
        tvar = tvar->vprev;
    }
    return tvar;
} /* FindVariable */


/*
 * Search for a symbol name:
 *  1) current structure
 *  2) current locals
 *  3) current globals
 *  4) library globals
 */
VCLCLASS VARIABLE *
VCLCLASS SearchVariable (int symbolid, int isStruct)
{
    VARIABLE *      pvar;

    if ( ( pvar = FindVariable( symbolid, &Ctx.Curstruct, 999, NULL, isStruct ) ) != NULL )
        return pvar;
    if ( Ctx.Linkfunction )
    {
        if ( ( pvar = FindVariable( symbolid, &Ctx.Linkfunction->locals,
                  Ctx.Linkfunction->BlkNesting, NULL, isStruct ) ) != NULL )
            return pvar;
    }
    /* this supports the debugger at runtime */
    if ( Ctx.Curfunc )
    {
        if ( ( pvar = FindVariable( symbolid,
                                    &Ctx.Curfunc->fvar->locals,
                       Ctx.Curfunc->BlkNesting, NULL, isStruct ) ) != NULL )
            return pvar;
    }
    if ( ( pvar = FindVariable( symbolid, &Globals, 0, NULL, isStruct ) ) != NULL )
        return pvar;
    return NULL;
} /* SearchVariable */


/*
 * Install a new variable in the variable table
 */
VCLCLASS VARIABLE *
VCLCLASS InstallVariable (VARIABLE *pvar,        /* the variable to be installed */
                 VARIABLELIST *pvartab, /* linked list of variables */
                 int alloc,             /* allocate & init to zero if TRUE */
                 int isArgument,
                 int BlkNesting,
                 int isStructdef)
{
    VARIABLE *      Nextvar = (VARIABLE *) AllocVariable();
    VARIABLE *      Thisvar;

    Assert( pvar != NULL );
    Assert( pvartab != NULL );

    *Nextvar = *pvar;
    Nextvar->vwidth = VariableWidth( Nextvar );
    Nextvar->vBlkNesting = BlkNesting;

    if ( Nextvar->vqualifier & EXTERNAL )
    {
        pvartab = &Globals;
        Nextvar->vBlkNesting = 0;
        alloc = 1;
    }

    /*
     * Check for redeclaration
     */
    if ( pvar->vsymbolid )
    {
        VARIABLE *      ev;

        /* scan variable table for symbol id match */
        for ( ev = pvartab->vlast; ev != NULL; ev = ev->vprev )
        {
            if ( ev == Blkvar )
            {
                ev = NULL;
                break;
            }

            /* match current & existing symbol id's matches */
            if ( pvar->vsymbolid == ev->vsymbolid )
            {
                if ( isStructdef && ( pvar->vtype == ev->vtype ) )
                    break;
                /* if existing is just a tag name keep going */
                else if ((ev->vtype == STRUCT || ev->vtype == UNION) && ev->vwidth == 0)
                    continue;
                /* if current is just a tag name keep going */    
                else if ((pvar->vtype == STRUCT || pvar->vtype == UNION) && pvar->vwidth == 0)
                    continue;
                /* if the names match but the types don't, see if it's extern */    
                else if ( pvar->vtype != ev->vtype )
                    break;
            }
        }

        /* there's one with the same name */
        if ( ev != NULL )
        {
            NullVariable( Nextvar );
            --Nextvar;
            if ( pvar->vtype == ev->vtype )
            {
                /* extern declaration of declared variable */
                if ( ( pvar->vqualifier ) & EXTERNAL )
                    return ev;          /* return extern reference */
                if ( ( ev->vqualifier ) & EXTERNAL )
                {
                    ev->vqualifier &= ~EXTERNAL;
                    return ev;          /* it is extern, resolve it */
                }
            }
            error( MDEFERR );
        }
    }

    /* not there, add the variable to the table */
    if ( alloc )
    {
        char *          ds = (char *) GetDataSpace( Nextvar->vwidth, 1 );

        /* allocate some data space */
        Nextvar->voffset = (int) ( ds - DataSpace );
    }

    Nextvar->vnext = NULL;
    Nextvar->vprev = pvartab->vlast;
    if ( pvartab->vlast != NULL )
        pvartab->vlast->vnext = Nextvar;
    pvartab->vlast = Nextvar;
    if ( pvartab->vfirst == NULL )
        pvartab->vfirst = Nextvar;

    if ( ( Nextvar->vkind & LABEL ) == 0 && ! isTypedef( Nextvar ) )
    {
        Nextvar->islocal = isArgument ? 2 : ( pvartab != &Globals );

        if ( ! alloc && Nextvar->vtype != ENUM )
        {
            /* compute offset of variable's data space */
            Thisvar = pvartab->vfirst;
            while ( Thisvar != Nextvar )
            {
                if ( ! isTypedef( Thisvar ) )
                    if ( Thisvar->vtype != ENUM )
                        if ( Thisvar->islocal == Nextvar->islocal )
                            if ( ! Nextvar->vstatic )
                                Nextvar->voffset += Thisvar->vwidth;
                Thisvar = Thisvar->vnext;
                Assert( Thisvar != NULL );
            }
        }
    }
    Nextvar->fileno = Ctx.CurrFileno;
    Nextvar->lineno = Ctx.CurrLineno;
    return Nextvar;
} /* InstallVariable */


int
VCLCLASS ArrayElements (VARIABLE *pvar)
{
    int             i;
    int             j = 1;

    for ( i = 0; i < MAXDIM; i++ )
        if ( pvar->vdims[i] )
            j *= pvar->vdims[i];
    return j;
} /* ArrayElements */


int
VCLCLASS ArrayDimensions (VARIABLE *pvar)
{
    int             i;
    int             j = 0;

    for ( i = 0; i < MAXDIM; i++ )
        if ( pvar->vdims[i] )
            j++;
    return j;
} /* ArrayDimensions */


int
VCLCLASS VariableWidth (VARIABLE *pvar)
{
    int             nbytes = pvar->vsize;

    if ( isPointerArray( pvar ) )
        nbytes = ArrayElements( pvar ) * sizeof( void * );
    else if ( isArray( pvar ) )
        nbytes = ArrayElements( pvar ) * pvar->vsize;
    else if ( isPointer( pvar ) )
        nbytes = sizeof( void * );
    return nbytes;
} /* VariableWidth */


void
VCLCLASS TypeQualifier (VARIABLE *pvar)
{
    int             done = 0;

    while ( ! done )
    {
        switch ( Ctx.Token )
        {
            case T_STATIC:
                if ( pvar->vqualifier & ( AUTO | EXTERNAL | REGISTER ) )
                    error( DECLERR );
                pvar->vstatic = 1;
                break;
            case T_AUTO:
                if ( pvar->vstatic || ( pvar->vqualifier & ( EXTERNAL | REGISTER ) ) )
                    error( DECLERR );
                pvar->vqualifier |= AUTO;
                break;
            case T_REGISTER:
                if ( pvar->vstatic || ( pvar->vqualifier & ( EXTERNAL | AUTO ) ) )
                    error( DECLERR );
                pvar->vqualifier |= REGISTER;
                break;
            case T_CONST:
                if ( pvar->vqualifier & VOLATILE )
                    error( DECLERR );
                pvar->vconst = 1;
                break;
            case T_EXTERNAL:
                if ( pvar->vstatic || ( pvar->vqualifier & ( AUTO | REGISTER ) ) )
                    error( DECLERR );
                pvar->vqualifier |= EXTERNAL;
                break;
            case T_VOLATILE:
                if ( pvar->vconst )
                    error( DECLERR );
                pvar->vqualifier |= VOLATILE;
                break;
            default:
                done = 1;
                break;
        }
        if ( ! done )
            getoken();
    }
} /* TypeQualifier */


/*
 * enum definition
 */
VCLCLASS VARIABLE *
VCLCLASS DefineEnum (VARIABLELIST *pvartab, VARIABLE var,
            int isArgument, int BlkNesting, int StopComma,
            uchar **svp)
{
    VARIABLE *      psvar = NULL;
    int             einit = 0;

    // To get rid of the error
    if ( StopComma == 0 )
        StopComma = 0;

    if ( Ctx.Token != T_LBRACE )
        error( ENUMERR );
    while ( getoken() == T_SYMBOL )
    {
        var.vsymbolid = Ctx.Value.ival;
        var.vtype = ENUM;
        var.vsize = sizeof( int );
        var.vconst = 1;
        psvar = InstallVariable( &var, pvartab, 0, isArgument, BlkNesting, 0 );
        if ( getoken() == T_ASSIGN )
        {
            /* enum initializer */
            getoken();
            if ( Ctx.Token == T_INTCONST )
                einit = Ctx.Value.ival;
            else if ( Ctx.Token == T_CHRCONST )
                einit = Ctx.Value.cval;    
            else
                error( ENUMERR );
            getoken();
        }
        psvar->enumval = einit++;
        if ( Ctx.Token != T_COMMA )
            break;
    }
    if ( Ctx.Token != T_RBRACE )
        error( ENUMERR );
    *svp = Ctx.Progptr;
    getoken();
    return psvar;
} /* DefineEnum */


VCLCLASS VARIABLE *
VCLCLASS DeclareEnum (VARIABLELIST *pvartab, VARIABLE var,
             int isArgument, int BlkNesting, int StopComma)
{
    VARIABLE *      evar = NULL;
    unsigned char * svp;

    svp = Ctx.Progptr;
    getoken();
    /* enum may have optional tagname */
    if ( isSymbol() )
    {
        svp = Ctx.Progptr;
        getoken();
    }
    if ( Ctx.Token != T_SYMBOL )
        evar = DefineEnum( pvartab, var, isArgument, BlkNesting, StopComma, &svp );
    if ( Ctx.Token == T_SYMBOL )
    {
        /* enum declaration becomes int */
        Ctx.Progptr = svp;
        Ctx.Token = T_INT;
        evar = NULL;
    }
    return evar;
} /* DeclareEnum */


/*
 * Prepare a local variable entry
 */
VCLCLASS VARIABLE *
VCLCLASS DeclareStructure (VARIABLELIST *pvartab, VARIABLE var,
                int isMember, int isArgument, int BlkNesting, int StopComma)
{
    int             done = FALSE;
    VARIABLE *      psvar;
    char            tokn = Ctx.Token;

    var.vtype = MakeType( tokn );

    getoken();

    /* struct has a tag name */
    if ( isSymbol() )
    {
        /* found existing name = tag name */
        if ( ( psvar = Ctx.Curvar ) != NULL )
        {
            /*-
             * Can get here if (note: applies to unions too):
             *
             *     Syntax:          Tag matches a:         Action:
             *  a) struct tag var   struct definition      declare struct var
             *  b) struct tag {     struct definition      error
             *  c) struct tag var   variable               define struct
             *  d) struct tag {     variable               define struct
             */

            /* save context  match is valid */
            Ctx.svpptr = Ctx.Progptr;
            Ctx.svToken = Ctx.Token;
            Ctx.svCurvar = Ctx.Curvar;
            Ctx.svCurrLineno = Ctx.CurrLineno;
            Ctx.svisStruct = isStruct;

            getoken();

            /* constant */
            if ( Ctx.Token == T_CONST || Ctx.Token == T_VOLATILE )
            {
                var.vconst = ( Ctx.Token == T_CONST );
                getoken();
            }

            /*
             * Check for redefinition
             *
             * [ Not sure what '! psvar->velem.vfirst' does, but if
             *   it isn't broken ... ]
             */
            if ( ( psvar->vtype == STRUCT || psvar->vtype == UNION ) &&
                 ( ! psvar->velem.vfirst || Ctx.Token == T_LBRACE /* b) */ ) )
                error( MULTIPLEDEF );

            if ( psvar->vtype == STRUCT || psvar->vtype == UNION )
            {
                /*-
                 * a) declare variable from existing struct definition
                 *
                 * expect a list of variables of type "struct tag var, ..."
                 */
                var.vsize = psvar->vsize;
                var.vtype = psvar->vtype;
                var.velem = psvar->velem;
                var.vstruct = psvar;
                psvar = varlist( tokn, isTypedef( &var ),
                                 var.vconst, var.vstatic, var.vqualifier,
                                 pvartab, NULL, &var,
                                 isMember, isArgument, BlkNesting, StopComma );
                done = TRUE;
            }
            else
            {   /*
                 * c), d) must have matched with a variable which is ok,
                 * restore context and proceed with a new definition
                 */
                Ctx.Progptr = Ctx.svpptr;
                Ctx.Token = Ctx.svToken;
                Ctx.Curvar = Ctx.svCurvar;
                Ctx.CurrLineno = Ctx.svCurrLineno;
                isStruct = Ctx.svisStruct;
            }
        }

        /* new definition, tag name does not exist or matched a variable */
        if ( pvartab && ! done )
        {
            /* expect struct definition, install in the given symbol table */
            var.vsymbolid = Ctx.Value.ival;
            getoken();
            psvar = InstallVariable( &var, pvartab, 0, isArgument, BlkNesting, 1 );
            psvar->vstruct = psvar;
            if ( Ctx.Token != T_LBRACE )
                error( STRUCERR );
            strucdef( tokn, psvar );
            varlist( tokn, isTypedef( psvar ), 0, 0, 0,
                     pvartab, NULL, psvar,
                     isMember, isArgument, BlkNesting, StopComma );
        }
        else
            if ( ! done )
            /* Don't know what the struct tag is supposed to look like */
                error( DECLARERR );
    }
    else if ( Ctx.Token == T_LBRACE )
    {
        /*
         * found a "struct {", expect its definition
         */
        if ( isArgument )
            error( SYNTAXERR );
        var.vsymbolid = 0;              /* it has no id yet */
        strucdef( tokn, &var );
        psvar = varlist( tokn, isTypedef( &var ), 0, 0, 0,
                         pvartab, NULL, &var,
                         isMember, 0, BlkNesting, StopComma );
        psvar->vstruct = psvar;
    }
    else
        error( LBRACERR );

    return psvar;
} /* DeclareStructure */


/*
 * typedef, provide implicit declarator
 */
VCLCLASS VARIABLE *
VCLCLASS DeclareTypedef (VARIABLELIST *pvartab, VARIABLE var,
                int isMember, int isArgument, int BlkNesting, int StopComma)
{
    VARIABLE        tvar;

    if ( Ctx.Curvar == NULL || ! isTypedef( Ctx.Curvar ) )
        error( DECLERR );
    tvar = *Ctx.Curvar;
    tvar.vsymbolid = 0;
    tvar.vkind &= ~TYPEDEF;
    tvar.vstatic |= var.vstatic;
    tvar.vconst |= var.vconst;
    getoken();
    TypeQualifier( &tvar );
    return varlist( 0, 0, tvar.vconst,
                    tvar.vstatic, tvar.vqualifier, pvartab, &tvar, 0,
                    isMember, isArgument, BlkNesting, StopComma );
} /* DeclareTypedef */


/*
 * Native type declaration (int, char, etc.)
 */
VCLCLASS VARIABLE *
VCLCLASS DeclareNative (VARIABLELIST *pvartab, VARIABLE var,
               int isMember, int isArgument, int BlkNesting, int StopComma)
{
    char            tokn = Ctx.Token;

    getoken();
    TypeQualifier( &var );
    return varlist( tokn, isTypedef( &var ), var.vconst,
                    var.vstatic, var.vqualifier, pvartab, NULL, NULL,
                    isMember, isArgument, BlkNesting, StopComma );
} /* DeclareNative */


/*
 * Parse a declaration statement and add variables to the
 * variable table pointed to by pvartab.
 */
VCLCLASS VARIABLE *
VCLCLASS DeclareVariable (VARIABLELIST *pvartab,
                 int isMember, int isArgument, int BlkNesting, int StopComma)
{
    VARIABLE        var;

    NullVariable( &var );
    if ( Ctx.Token == T_TYPEDEF )
    {
        if ( isArgument )
            error( TYPEDEFERR );
        var.vkind = TYPEDEF;
        getoken();
    }

    TypeQualifier( &var );

    if ( Ctx.Token == T_ENUM )
    {
        VARIABLE *      evar;

        evar = DeclareEnum( pvartab, var, isArgument, BlkNesting, StopComma );
        if ( evar )
            return evar;
    }

    if ( Ctx.Token == T_STRUCT || Ctx.Token == T_UNION )
        return DeclareStructure( pvartab, var, isMember, isArgument, BlkNesting, StopComma );

    if ( isSymbol() )
        return DeclareTypedef( pvartab, var, isMember, isArgument, BlkNesting, StopComma );

    return DeclareNative( pvartab, var, isMember, isArgument, BlkNesting, StopComma );
} /* DeclareVariable */


/*
 * Parse a struct definition.
 */
void
VCLCLASS strucdef (char tokn, VARIABLE *psvar)
{
    unsigned        wid;
    VARIABLE *      pvar;

    /*
     * Get rid of the "{" and any blank lines
     */
    getoken();

    while ( istypespec() ||
            Ctx.Token == T_CONST ||
            Ctx.Token == T_VOLATILE ||
            Ctx.Token == T_VOID )
    {
        pvar = DeclareVariable( (VARIABLELIST *) &psvar->velem, 1, 0, psvar->vBlkNesting, 0 );
        stmtend();
    }

    if ( Ctx.Token == T_RBRACE )
        getoken();
    else
        error( RBRACERR );

    /*
     * Now change all of the variables declared in the structure or union to
     * structure element types, build their data offsets, and compute the
     * structure width.
     */
    pvar = psvar->velem.vfirst;
    while ( pvar != NULL )
    {
        pvar->vkind |= STRUCTELEM;
        wid = VariableWidth( pvar );
        if ( tokn == T_UNION )
        {
            pvar->voffset = 0;
            psvar->vsize = max( psvar->vsize, (int) wid );
        }
        else
        {
            pvar->voffset = psvar->vsize;
            psvar->vsize += wid;
        }
        pvar = pvar->vnext;
    }
} /* strucdef */


/*
 * Start from the first type token and resolve the
 * declaration's type
 */
char
VCLCLASS MakeTypeToken (char tokn, int *sawunsigned)
{
    int             done = 0;

    if ( sawunsigned )
        *sawunsigned = tokn == T_UNSIGNED;      /* set if not null */
    while ( ! done )
    {
        switch ( Ctx.Token )
        {
            case T_CHAR:
            case T_DOUBLE:
            case T_FLOAT:
            case T_LONG:
                tokn = Ctx.Token;
                getoken();
                break;
            case T_INT:
                if ( tokn != T_LONG )
                    tokn = T_INT;
                getoken();
                break;
            case T_UNSIGNED:
                if ( sawunsigned )      /* set if not null */
                    *sawunsigned = 1;
            case T_SHORT:
                tokn = T_INT;
                getoken();
                break;
            default:
                done = 1;
                break;
        }
    }
    if ( tokn == T_UNSIGNED )
        tokn = T_INT;
    return tokn;
} /* MakeTypeToken */


/*
 * Parse a declarator list
 *
 * declarator-list:
 *    declarator
 *    declarator = initializer
 *    declarator-list , declarator
 *
 * The argument "tokn" is the Ctx.Token form of the declaration-specifier
 * "char", "int", "struct", "union", etc.
 */
VCLCLASS VARIABLE *
VCLCLASS varlist (char tokn, char VarisTypedef,
         char vconst, char vstatic, char vqualifier,
         VARIABLELIST *pvartab, VARIABLE *Typedef,
         VARIABLE *psvar,
         int isMember, int isArgument, int BlkNesting,
         int StopComma)
{
    VARIABLE *      pvar;
    int             sawunsigned;

    tokn = MakeTypeToken( tokn, &sawunsigned );

    while ( ( pvar = Declarator( tokn, pvartab, isArgument, Typedef ) ) != NULL )
    {
        int             alloc = 0;

        if ( sawunsigned )
            pvar->isunsigned = 1;

        if ( Typedef != NULL )
        {
            /* variable is defined by a typedef */
            if ( isArgument )
            {
                /* function args don't have dimensions */
                int             i;

                for ( i = 0; i < MAXDIM; i++ )
                    pvar->vdims[i] = 0;
            }
            while ( Ctx.Token == T_PTR )
            {
                pvar->vcat++;
                getoken();
            }
            if ( isSymbol() )
            {
                pvar->vsymbolid = Ctx.Value.ival;
                getoken();
            }
        }

        /* if it's a pointer, vconst means what it points to */
        if ( pvar->vcat )
            vconst <<= 1;

        if ( Typedef == NULL )
        {
            if ( VarisTypedef )
                pvar->vkind |= TYPEDEF;
            pvar->vconst |= vconst;
            pvar->vstatic = vstatic;
            pvar->vqualifier = vqualifier;

            if ( tokn == T_STRUCT || tokn == T_UNION )
            {
                /*
                 * Structures and unions:  We need to copy over the
                 * declarator's attributes to the structure variable.
                 */
                pvar->vsize = psvar->vsize;
                pvar->vtype = psvar->vtype;
                pvar->velem = psvar->velem;
                pvar->vconst |= psvar->vconst;
                pvar->vstruct = psvar->vstruct;
                pvar->vqualifier = psvar->vqualifier;
            }
        }

        if ( pvartab )
        {
            /*-
             * It's a plain variable; install it.
             *
             * Only allocate memory for globals and statics when linking
             */
            alloc = ( ! isTypedef( pvar ) &&
                      pvar->vtype != ENUM &&
                      ! isMember &&
                      ! isArgument &&
                      ( pvar->vstatic || pvartab == &Globals ) );
            pvar = InstallVariable( pvar, pvartab, alloc, isArgument, BlkNesting, 0 );
        }

        if ( Ctx.Token == T_ASSIGN )
        {
            if ( pvar->isinitialized )
                error( INITERR );
            if ( pvartab != &Globals && ( vqualifier & EXTERNAL ) )
                error( INITERR );
            if ( isMember )             /* can't init a member directly */
                error( SEMIERR );
            pvar->isinitialized = 1;
            pvar->vqualifier &= ~EXTERNAL;
            if ( isArgument || ( pvar->vkind & TYPEDEF ) )
                error( INITERR );

            if ( ! Linking && ! pvar->vstatic )
                stmtbegin();
            getoken();                  /* get rid of '=' */

            if ( alloc )
            {
                int             braceneeded;

                braceneeded = isArray( pvar ) &&
                    ! ( pvar->vtype == CHAR && Ctx.Token == T_STRCONST );

                /* initializers */
                if ( braceneeded && Ctx.Token != T_LBRACE )
                    error( LBRACERR );

                Initializer( pvar, (char *) DataAddress( pvar ), 0 );

                if ( Ctx.Token == T_RBRACE )
                    getoken();
                else if ( braceneeded )
                    error( RBRACERR );
            }
            else
            {
                /*
                 * skip initializers for args, members, and when linking autos
                 */
                if ( Ctx.Token == T_LBRACE )
                    skip( T_LBRACE, T_RBRACE );
                else
                {
                    while ( Ctx.Token != T_COMMA && Ctx.Token != T_SEMICOLON )
                    {
                        if ( getoken() == T_EOF )
                            error( INITERR );
                        if ( Ctx.Token == T_LPAREN )
                            skip( T_LPAREN, T_RPAREN );
                    }
                }
            }
        }

        if ( Ctx.Token != T_COMMA || StopComma )
            break;
        getoken();
    }
    return pvar;
} /* varlist */


/*
 * Parse variable initialization expressions
 */
void
VCLCLASS Initializer (VARIABLE *pvar, char *baseaddr, int level)
{
    union
    {
        char *          cp;
        int *           ip;
        long *          jp;
        double *        fp;
        void **         pp;
    }               ptrs;

    VARIABLE *      pevar;

    int             i;

    if ( level == 0 && isArray( pvar ) )
    {
        /* initializing the elements of an array */
        int             dims;
        int             elems;

        if ( pvar->vtype == CHAR && Ctx.Token == T_STRCONST )
        {
            /* string literal initialization */
            Initializer( pvar, baseaddr, 1 );
            return;
        }

        /* get number of dimensions */
        dims = ArrayDimensions( pvar );

        /*-
         * get number of elements
         *
         * Special case for handling two-dimensional character arrays.
         * The total number of elements for such an array is the size
         * of the first dimension.
         */
        if ( pvar->vtype == CHAR && dims > 1 )
            elems = pvar->vdims[0];
        else
            elems = ArrayElements( pvar );

        if ( Ctx.Token == T_LBRACE )
            getoken();

        for ( i = 0; i < elems; i++ )
        {
            int             ln;

            if ( isPointer( pvar ) )
                ln = sizeof( void * );
            else
            {
                /*-
                 * get size of an element
                 *
                 * Special case for handling two-dimensional character arrays.
                 * An element size is the size of the second dimension.
                 */
                if ( pvar->vtype == CHAR && dims > 1 )
                    ln = pvar->vdims[1];
                else
                    ln = pvar->vsize;
            }

            if ( Ctx.Token == T_LBRACE && dims > 1 )
            {
                /* compute # elements in the inner array */
                VARIABLE        avar;
                int             dm;
                int             wd = 1;
                int             j = pvar->vcat - 1;

                if ( j < 0 )
                    error( INITERR );

                while ( j )
                    wd *= pvar->vdims[j--];

                /* build a variable to represent the inner array */
                avar = *pvar;
                --avar.vcat;
                for ( dm = 0; dm < MAXDIM - 1; dm++ )
                    avar.vdims[dm] = avar.vdims[dm + 1];
                avar.vdims[dm] = 0;

                /* initialize the inner array */
                Initializer( &avar, baseaddr + i * ln, 0 );
                i += wd - 1;

                if ( Ctx.Token != T_RBRACE )
                    error( RBRACERR );
                getoken();
            }
            else
                /* scale up in data space by element * size */
                Initializer( pvar, baseaddr + ( i * ln ), level + 1 );

            if ( Ctx.Token == ',' )
            {
                getoken();
                /* test for too many initializers */
                if ( i >= elems - 1 )
                    error( TOOMANYINITERR );
            }
            else
                break;
        }
        return;
    }

    if ( ( pvar->vtype == STRUCT || pvar->vtype == UNION ) && ! isPointer( pvar ) )
    {
        /* initializing a struct or union */
        if ( Ctx.Token != T_LBRACE )
        {
            /* initializing with another struct or union */
            /* push the receiving struct */
            push( pvar->vkind, pvar->isunsigned, pvar->vcat, 1,
                  pvar->vsize, pvar->vtype,
                  (VARIABLELIST *) &pvar->velem,
                  (DATUM *) &baseaddr, pvar->vconst );
            initexpr( pvar );
            if ( pvar->vstruct != Ctx.Stackptr->vstruct )
                error( INITERR );
            assignment();
            pop();
            return;
        }
        getoken();
        pevar = pvar->velem.vfirst;
        for ( ;; )
        {
            pevar->vstatic = pvar->vstatic;
            Initializer( pevar, baseaddr + (int) pevar->voffset, level );
            pevar = pevar->vnext;
            if ( Ctx.Token == ',' )
            {
                getoken();
                /* test for too many initializers */
                if ( pevar == NULL )
                    error( TOOMANYINITERR );
            }
            else
                break;
        }
        if ( Ctx.Token != T_RBRACE )
            error( RBRACERR );
        getoken();
        return;
    }

    /*
     * Get variable's data address
     */
    ptrs.cp = baseaddr;

    initexpr( pvar );

    /* Item on stack is a string literal */
    if ( StackItemisString() )
    {
        /* can initialize only char *, char [] or char [n] with string */
        if ( ! ( pvar->vtype == CHAR && pvar->vcat ) )
            error( INITERR );

        if ( ! isPointerArray( pvar ) )
        {
            if ( isArray( pvar ) )
            {
                char *          cp = (char *) popptr();

                /*-
                 * initializer cannot be longer than char array.
                 *
                 * Special case for two dimensional character arrays.
                 */
                if ( ArrayDimensions( pvar ) > 1 )
                {
                    if ( pvar->vdims[1] < strlen( cp ) )
                        error( INITERR );
                }
                else if ( pvar->vdims[0] < strlen( cp ) )
                    error( INITERR );
                strcpy( ptrs.cp, cp );
                return;
            }
        }
    }

    if ( isPointer( pvar ) )
    {
        /* initializing a pointer with an address */
        *ptrs.pp = popptr();
        return;
    }

    /*
     * Variable is a plain variable.
     */
    if ( StackItemisAddressOrPointer() )
        error( INITERR );
    switch ( pvar->vtype )
    {
        case CHAR:
            *ptrs.cp = popint();
            break;
        case INT:
            *ptrs.ip = popint();
            break;
        case LONG:
            *ptrs.jp = poplng();
            break;
        case FLOAT:
            *ptrs.fp = popflt();
            break;
        default:
            pop();
            break;
    }
} /* Initializer */


/*
 * Parse an initialization expression
 */
void
VCLCLASS initexpr (VARIABLE *pvar)
{
    ITEM *          sp = Ctx.Stackptr;

    ConstExpression = ( ! pvar->islocal ) ||
        pvar->vstatic ||
        isArray( pvar ) ||
        ( pvar->vkind & STRUCTELEM );

    cond();

    if ( sp == Ctx.Stackptr )
        /* nothing was pushed (null expression) */
        error( INITERR );

    ConstExpression = 0;
} /* initexpr */


char
VCLCLASS MakeType (char tok)
{
    switch ( tok )
    {
        case T_CHAR:
            return CHAR;
        case T_SHORT:
        case T_UNSIGNED:
        case T_ENUM:
        case T_INT:
            return INT;
        case T_LONG:
            return LONG;
        case T_FLOAT:
        case T_DOUBLE:
            return FLOAT;
        case T_STRUCT:
            return STRUCT;
        case T_UNION:
            return UNION;
        case T_VOID:
            return VOID;
        default:
            break;
    }
    return tok;
} /* MakeType */


int
VCLCLASS TypeSize (char type)
{
    char            typesize[] = {
        0,
        sizeof( char ),
        sizeof( int ),
        sizeof( long ),
        sizeof( double ),
        0,
        0
    };

    return typesize[type];
} /* TypeSize */


void
VCLCLASS SetType (VARIABLE *var, char tok)
{
    var->vtype = MakeType( tok );
    var->vsize = TypeSize( var->vtype );
} /* SetType */


/*
 * Parse a declarator:
 *
 * declarator:
 *    symbol
 *    * declarator
 *    declarator [ constant expression ]
 */
VCLCLASS VARIABLE *
VCLCLASS Declarator (char tokn, VARIABLELIST *pvartab, int isArgument,
            VARIABLE *Typedef)
{
    VARIABLE *      pvar;
    VARIABLE        var;
    static VARIABLE svar;
    int             argc;

    svar.velem.vfirst = svar.velem.vlast = 0;
    if ( Ctx.Token == T_LPAREN )
    {
        getoken();
        if ( ( pvar = Declarator( tokn, pvartab, isArgument, Typedef ) ) != NULL )
        {
            if ( Ctx.Token == T_RPAREN )
            {
                /*
                 * Check for function pointer declaration
                 */
                if ( getoken() == T_LPAREN )
                {
                    pvar->vkind |= FUNCT;
                    skip( T_LPAREN, T_RPAREN );
                }
            }
            else
                error( RPARENERR );
        }
    }
    else if ( Ctx.Token == T_PTR )
    {
        /*
         * it's a pointer
         */
        int             isconst = 0;

        getoken();
        if ( Ctx.Token == T_VOLATILE )
            getoken();
        else if ( ( isconst = ( Ctx.Token == T_CONST ) ) != 0 )
            getoken();
        if ( ( pvar = Declarator( tokn, pvartab, isArgument, Typedef ) ) != NULL )
        {
            pvar->vconst |= isconst;
            ++pvar->vcat;
        }
    }
    else if ( isSymbol() )
    {
        /*
         * Save the symbol's name and size and initialize to zero all other
         * attributes.
         */
        if ( Typedef != NULL )
            var = *Typedef;
        else
        {
            NullVariable( &var );
            SetType( &var, tokn );
        }
        var.vsymbolid = Ctx.Value.ival;
        getoken();
        /*
         * compute its length
         */
        while ( Ctx.Token == T_LBRACKET )
        {
            if ( var.vcat++ == MAXDIM )
                error( MDIMERR );

            getoken();
            /*
             * compute the dimension
             */
            if ( Ctx.Token == T_RBRACKET )
            {
                /* arrayname[] (empty dimension, could be pointer) */
                getoken();
                if ( pvartab != NULL && Ctx.Token == T_ASSIGN && var.vcat < 2 )
                {
                    unsigned char * svprogptr = Ctx.Progptr;

                    getoken();
                    /* parse ahead to compute the dimension */
                    if ( var.vtype == STRUCT || var.vtype == UNION )
                    {
                        if ( Ctx.Token != T_LBRACE )
                            error( LBRACERR );
                        var.vdims[var.vcat - 1]++;
                        while ( Ctx.Token != T_SEMICOLON )
                        {
                            if ( Ctx.Token == T_COMMA )
                                var.vdims[var.vcat - 1]++;
                            else if ( Ctx.Token == T_EOF )
                                error( INITERR );
                            getoken();
                            if ( Ctx.Token == T_LBRACE )
                                skip( T_LBRACE, T_RBRACE );
                        }
                    }
                    else
                    {
                        if ( Ctx.Token == T_LBRACE )
                        {
                            getoken();
                            ConstExpression = 1;
                            argc = expression();
                            ConstExpression = 0;
                            if ( Ctx.Token != T_RBRACE )
                                error( RBRACERR );
                            var.vdims[var.vcat - 1] = argc;
                            popnint( argc );
                        }
                        else if ( var.vtype == CHAR && Ctx.Token == T_STRCONST )
                        {
                            char *          cp;

                            primary();
                            cp = (char *) popptr();
                            var.vdims[var.vcat - 1] = strlen( cp ) + 1;
                        }
                    }
                    /* return to assignment position */
                    Ctx.Progptr = svprogptr;
                    Ctx.Token = T_ASSIGN;
                }
                /* arrayname[][n] (empty first dimension of multidim array) */
                else if ( pvartab != NULL && Ctx.Token == T_LBRACKET )
                {
                    unsigned char * svprogptr = Ctx.Progptr;

                    /* skip to & count elements */
                    do
                    {
                        getoken();
                    } while ( Ctx.Token != T_LBRACE && Ctx.Token != T_EOF );

                    if ( Ctx.Token == T_LBRACE )
                    {
                        /* move to left-most, inner braces */
                        while ( Ctx.Token == T_LBRACE )
                            getoken();
                        ConstExpression = 1;
                        argc = expression();
                        ConstExpression = 0;
                        if ( Ctx.Token != T_RBRACE )
                            error( RBRACERR );
                        var.vdims[var.vcat - 1] = argc;
                        popnint( argc );
                    }

                    /* return to second dimension position */
                    Ctx.Progptr = svprogptr;
                    Ctx.Token = T_LBRACKET;
                } else
                    /* error other combinations we can't calculate */
                    if ( ! isArgument )
                        error( UNKNOWNSIZE );
            }
            else
            {
                ConstExpression = 1;
                argc = expression();
                ConstExpression = 0;

                if ( Ctx.Token != T_RBRACKET )
                    error( RBRACKETERR );
                getoken();
                if ( ( var.vdims[var.vcat - 1] = popnint( argc ) ) < 0 )
                    /*
                     * negative array size.
                     */
                    error( SUBSERR );
                if ( isArgument )
                    /*
                     * This is a function argument, the inner dimension
                     * length is discarded.
                     */
                    var.vdims[0] = 0;
            }
        }

        if ( pvartab )
        {
            /*
             * We are going to be installing this variable in a symbol table
             * somewhere, so... copy the auto to the static VARIABLE.
             */
            svar = var;
            pvar = &svar;
        }
        else
        {
            /*
             * pvartab==NULL implies that this is a declaration of a formal
             * argument and it was already installed in the function's local
             * symbol table.  Search for the variable in the current locals
             * and change its attributes.
             */
            if ( ( pvar = FindVariable( var.vsymbolid,
                         &Ctx.Linkfunction->locals, 1, NULL, 0 ) ) != NULL )
            {
                int             i;

                pvar->vcat = var.vcat;
                pvar->vtype = var.vtype;
                pvar->vsize = var.vsize;
                for ( i = 0; i < MAXDIM; i++ )
                    pvar->vdims[i] = var.vdims[i];
                pvar->vconst = var.vconst;
            }
            else
                /*
                 * Declared variable not in formal argument list.
                 */
                error( DECLARERR );
        }
    }
    else
    {
        /*-
         * This is a kludge to allow definition of struct tags:
         * struct stype {
         *  .
         *  .
         *  .
         * } ;
         * normally, this function expects to parse a declarator
         * but instead in the above case it sees only the end
         * of the declaration statement.  This kludge allows
         * illegal declaration statements like:
         *
         *    char ;
         *
         * to sneak by undetected...
         */
        if ( pvartab )
            return NULL;
        /*
         * If we're not installing this symbol in a symbol table, then it's
         * either a function argument declaration or a typecast.  In either
         * case, set up the static variable with as much info we have about
         * it.
         */
        if ( Typedef != NULL )
            return Typedef;
        NullVariable( &svar );
        SetType( &svar, tokn );
        return &svar;
    }

    return pvar;
} /* Declarator */


void *
VCLCLASS GetDataSpace (int sz, int init)
{
    char *          ds;

    if ( Ctx.NextData == DataSpace + vclCfg.MaxDataSpace )
        error( DATASPACERR );
    ds = Ctx.NextData;
    if ( sz )
    {
        Ctx.NextData += sz;
        if ( init )
            memset( ds, 0, sz );
    }
    MaxDataSpace = ( ds > MaxDataSpace ) ? ds : MaxDataSpace;
    return ds;
} /* GetDataSpace */


void *
VCLCLASS AllocVariable (void)
{
    if ( Ctx.NextVar == VariableMemory + vclCfg.MaxVariables )
        error( TOOMANYVARERR );
    ++VariablesUsed;
    return Ctx.NextVar++;
} /* AllocVariable */


void *
VCLCLASS DataAddress (VARIABLE *pvar)
{
    char *          vdata;

    if ( pvar->vkind & STRUCTELEM )
    {
        /*
         * the only time DataAddress is asked for the address of a structure
         * element is when the structure itself is on the stack and the
         * program is dereferencing the member.  Get the structure object's
         * address and add the element's offset.
         */
        vdata = Ctx.Stackptr->value.cptr + pvar->voffset;
    }
    else if ( pvar->islocal && ! pvar->vstatic )
    {
        Assert( Ctx.Curfunc != NULL );
        vdata = Ctx.Curfunc->ldata + pvar->voffset;
        if ( pvar->islocal == 1 )
            /* auto and not argument */
            vdata += Ctx.Curfunc->arglength;
    }
    else
        vdata = DataSpace + pvar->voffset;
    return vdata;
} /* DataAddress */

