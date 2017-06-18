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
    keyword.c - Keyword symbol tables & symbol table handling routines

 DESCRIPTION
    Contains functions for building, searching and deleting symbol tables.
    Both built-in searches and dynamic table searches are handled.

 FUNCTIONS
    BuildPredefined()
    SearchSymbols()
    SearchLibrary()
    FindKeyword()
    FindOperator()
    FindPreDefined()
    FindPreProcessor()
    FindSymbol()
    FindSymbolName()
    AddSymbol()
    DeleteSymbols()

 FILES
    vcldef.h
    keyword.h

 SEE ALSO

 NOTES

 BUGS

*****************************************************************unpubModule*/

#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>
#include <stdlib.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif


/*
 * Build the predefined symbols
 */
void
VCLCLASS BuildPredefined (void)
{
    int             i;

    for ( i = 0; i < MAXPREDEFINED; ++i)
        DefineMacro( (uchar *) PreDefined[i].symbol );
}


/*
 * Search a symbol table
 *
 * Search for arg, in tbl, which is siz.  If wd is non-zero
 * it specifies the maximum width to compare (using strncmp()).
 */
int
VCLCLASS SearchSymbols (char *arg, SYMBOLTABLE *tbl, int siz, int wd)
{
    int             i;
    int             mid;
    int             lo;
    int             hi;

    lo = 0;
    hi = siz - 1;

    while ( lo <= hi )
    {
        mid = ( lo + hi ) / 2;
        i = ( wd ) ? strncmp( arg, tbl[mid].symbol, wd ) :
                      strcmp( arg, tbl[mid].symbol );
        if ( i < 0 )
            hi = mid - 1;
        else if ( i )
            lo = mid + 1;
        else
            return tbl[mid].ident;
    }
    return 0;
} /* SearchSymbols */


int
VCLCLASS SearchLibrary (char *fname)
{
    return SearchSymbols( fname, LibraryFunctions, MAXLIBFUNCTIONS, 0 );
} /* SearchLibrary */


int
VCLCLASS FindKeyword (char *keyword)
{
    return SearchSymbols( keyword, Keywords, MAXKEYWORDS, 0 );
} /* FindKeyword */


int
VCLCLASS FindOperator (char *oper)
{
    return SearchSymbols( oper, Operators, MAXOPERATORS, 2 );
} /* FindOperator */


int
VCLCLASS FindPreDefined (char *predef)
{
    return SearchSymbols( predef, PreDefined, MAXPREDEFINED, 0 );
} /* FindPreDefined */


int
VCLCLASS FindPreProcessor (char *preproc)
{
    return SearchSymbols( preproc, PreProcessors, MAXPREPROCESSORS, 0 );
} /* FindPreProcessor */


int
VCLCLASS FindSymbol (char *sym)
{
    if ( SymbolTable != NULL )
        return SearchSymbols( sym, SymbolTable, SymbolCount, 0 );
    return 0;
} /* FindSymbol */


char *
VCLCLASS FindSymbolName (int id)
{
    int             i;

    for ( i = 0; i < SymbolCount; i++ )
        if ( SymbolTable[i].ident == id )
            return SymbolTable[i].symbol;
    return NULL;
} /* FindSymbolName */


int
VCLCLASS AddSymbol (char *sym)
{
    int             symbolid = 0;

    if ( SymbolTable != NULL )
    {
        symbolid = FindSymbol( sym );
        if ( symbolid == 0 )
        {
            if ( SymbolCount < vclCfg.MaxSymbolTable )
            {
                int             i;
                int             j;
                int             len = strlen( sym ) + 1;
                char *          s = (char *) getmem( len );

                strcpy( s, sym );
                /* Insert the symbol in the correct alphabetical order */
                for ( i = 0; i < SymbolCount; i++ )
                    if ( strcmp( sym, SymbolTable[i].symbol ) < 0 )
                        break;
                for ( j = SymbolCount; j > i; --j )
                    SymbolTable[j] = SymbolTable[j - 1];
                SymbolTable[i].symbol = s;
                SymbolTable[i].ident = ++SymbolCount;
                symbolid = SymbolCount;
            }
            else
                error( SYMBOLTABLERR );
        }
    }
    return symbolid;
} /* AddSymbol */


void
VCLCLASS DeleteSymbols (void)
{
    int             i;

    for ( i = 0; i < SymbolCount; i++ )
    {
        if ( SymbolTable[i].symbol )
        {
            free( SymbolTable[i].symbol );
            SymbolTable[i].symbol = NULL;
        }    
    }
} /* DeleteSymbols */
