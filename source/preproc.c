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
    preproc.c - Preprocessor

 DESCRIPTION
    Preprocesses source code, handles conditional compilation, and
    expands macro definitions.  Uses preexpr.c for preprocessor expressions.

 FUNCTIONS
    PreProcessor()
    CleanUpPreProcessor()
    DeleteFileList()
    bypassWhite()
    ExtractWord()
    FindMacro()
    DefineMacro()
    SrcFileName()

    FreeBuffers()
    PreProcess()
    parmcmp()
    AddMacro()
    UnDefineAllMacros()
    UnDefineMacro()
    Include()
    TestIfLevel()
    If()
    IfDef()
    IfnDef()
    Else()
    Elif()
    Endif()
    Error()
    Pragma()
    OutputLine()
    WriteChar()
    WriteEOL()
    WriteWord()
    ReadString()

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
#include <string.h>
#include <stdlib.h>
#ifdef __DOS
#include <dos.h>
#include <dir.h>
#include <sys\stat.h>
#endif
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
//#include "debugger.h"
#endif

/*
 * Preprocess code in SourceCode into pSrc
 */
void
VCLCLASS PreProcessor (uchar *pSrc, uchar *SourceCode)
{
    /* init */
    Op = pSrc;
    Ip = SourceCode;
    /* Ctx.CurrFileno = 0;  set outside of this function */
    Ctx.CurrLineno = 0;
    IfLevel = 0;
    Word = (uchar *) getmem( MAXMACROLENGTH );
    Line = (uchar *) getmem( MAXLINE );
    FilePath = (uchar *) getmem( MAXPATH );
    /* Build the predefined preprocessor symbols */
    BuildPredefined();
    PreProcess();
    if ( IfLevel )
        error( IFSERR );
    FreeBuffers();
} /* PreProcessor */


/*
 * Free heap buffers used by preprocessor
 */
void
VCLCLASS FreeBuffers (void)
{
    UnDefineAllMacros();
    if ( Line )
        free( Line );
    if ( FilePath )
        free( FilePath );
    if ( Word )
        free( Word );
    Line = FilePath = Word = NULL;
} /* FreeBuffers */


/*
 * Delete all preprocessor heap usage on error
 */
void
VCLCLASS CleanUpPreProcessor (void)
{
    FreeBuffers();
    DeleteFileList( FirstFile->NextFile );  /* does not first (source) file */
} /* CleanUpPreProcessor */


/*
 * Delete files from the file list
 *
 * Deletes the entries from thisfile to the end of
 * the linked list, inclusive.  Adjusts global LastFile
 * if thisfile is not the head of the list.
 */
void
VCLCLASS DeleteFileList (SRCFILE *thisfile)
{
    if ( thisfile != NULL )
    {
        /* recurse down to end of linked list */
        if ( thisfile->NextFile )
            DeleteFileList( thisfile->NextFile );

        /* if this is the head of the list, reset head/tail pointers */
        if ( thisfile == FirstFile )
            FirstFile = LastFile = NULL;
        else
        /* otherwise, find the new end-of-list */
        {
            SRCFILE *       sp;

            for ( sp = FirstFile; sp; sp = sp->NextFile )
            {
                if ( sp->NextFile == thisfile )
                {
                    sp->NextFile = NULL;
                    LastFile = sp;
                    break;
                }
            }
        }

        /* free this file's allocations */
        if ( thisfile->fname )
            free( thisfile->fname );
        if ( thisfile->fullname )
            free( thisfile->fullname );
        if ( thisfile->IncludeIp )
            free( thisfile->IncludeIp );
        free( thisfile );

        --FileCount;
    }
} /* DeleteFileList */


/*
 * Bypass source code white space & comments
 *
 * May result in a line containing a newline+null only,
 * but this keeps the line count straight.
 *
 * Removes C and C++ style comments.
 * Comments are handled here, instead of in ReadString(),
 * because this is called when we're scanning for the next
 * symbol within the context of the program's syntax.  In
 * ReadString() we have no idea what the state of the
 * lexical analysis is.
 *
 * Returns TRUE if anything was skipped, else FALSE
 */
int
VCLCLASS bypassWhite (unsigned char **cpp)
{
    unsigned char * cp = *cpp;
    int             ret;

    while ( isSpace( *cp ) )
        cp++;
    ret = (cp != *cpp) ? TRUE : FALSE;

    /* single-line C++ style comment, go to end-of-line */
    if ( *cp == '/' && *( cp + 1 ) == '/' )
    {
        ret = TRUE;
        while ( *cp && *cp != '\n' )
            ++cp;
    }
    /* standard C comment, scan to end-of-comment */
    else if ( *cp == '/' && *( cp + 1 ) == '*' )
    {
        int inComment = TRUE;

        ret = TRUE;
        cp += 2;
        while ( inComment )
        {
            /* scan forward looking for end-of-comment */
            while ( *cp && *cp != '\n' )
            {
                if ( *cp == '*' && *( cp + 1 ) == '/' )
                {
                    cp += 2;
                    inComment = FALSE;  /* found, no longer in comment */
                    break;
                }
                cp++;
            }
            /* if still in comment read the next line */
            if ( inComment )
            {
                if ( ReadString() == FALSE )
                    error( UNTERMCOMMENT );
                cp = Line;
            }
        }
        while ( isSpace( *cp ) )
            cp++;
    }

    *cpp = cp;
    return ret;
} /* bypassWhite */


/*
 * Extract a word from input
 */
void
VCLCLASS ExtractWord (uchar *wd, uchar **cp, uchar *allowed)
{
    while ( **cp )
    {
        if ( isalnum( **cp ) || strchr( (char *) allowed, (char) **cp ) )
            *wd++ = *( ( *cp )++ );
        else
            break;
    }
    *wd = '\0';
} /* ExtractWord */


/*
 * Internal preprocess entry point
 */
void
VCLCLASS PreProcess (void)
{
    unsigned char * cp;

    while ( ReadString() )
    {
        /* handshake(); !?! to keep D-Flat clock ticking */
        if ( Line[strlen( (char *) Line ) - 1] != (unsigned char) '\n' )
            error( LINETOOLONGERR );
        cp = Line;
        bypassWhite( &cp );

        /* first non-whitespace character the preprocessor prefix? */
        if ( *cp != '#' )
        {
            if ( ! Skipping[IfLevel] )  /* if not skipping this section */
                OutputLine( cp );       /* ..output the line */
            continue;
        }
        cp++;

        /* line contains a preprocessor token */
        bypassWhite( &cp );
        ExtractWord( Word, &cp, (unsigned char *) "" );
        if ( *Word == '\0' )
            continue;                   /* '#' only, just skip it */

        switch ( FindPreProcessor( (char *) Word ) )
        {
            case P_DEFINE:
                if ( ! Skipping[IfLevel] )
                    DefineMacro( cp );
                break;
            case P_ELSE:
                Else();
                break;
            case P_ELIF:
                Elif( cp );
                break;
            case P_ENDIF:
                Endif();
                break;
            case P_ERROR:
                Error( cp );
                break;
            case P_IF:
                If( cp );
                break;
            case P_IFDEF:
                IfDef( cp );
                break;
            case P_IFNDEF:
                IfnDef( cp );
                break;
            case P_INCLUDE:
                if ( ! Skipping[IfLevel] )
                    Include( cp );
                break;
            case P_PRAGMA:
                Pragma( cp );
                break;
            case P_UNDEF:
                if ( ! Skipping[IfLevel] )
                    UnDefineMacro( cp );
                break;
            default:
                error( BADPREPROCERR );
                break;
        }
    }
    WriteEOL();
} /* PreProcess */


/*
 * Find a macro that is already #defined
 */
VCLCLASS MACRO *
VCLCLASS FindMacro (uchar *ident)
{
    MACRO *         ThisMacro = FirstMacro;

    while ( ThisMacro != NULL )
    {
        if ( strcmp( (char *) ident, (char *) ThisMacro->id ) == 0 )
            return ThisMacro;
        ThisMacro = ThisMacro->NextMacro;
    }
    return NULL;
} /* FindMacro */


/*
 * Compare macro parameter values
 */
int
VCLCLASS parmcmp (char *p, char *t)
{
    char            tt[80];
    char *          tp = tt;

    while ( alphanum( *t ) )
        *tp++ = *t++;
    *tp = '\0';
    return strcmp( p, tt );
} /* parmcmp */


/*
 * Add a newly #defined macro to the table
 */
void
VCLCLASS AddMacro (uchar *ident, uchar *plist, uchar *value)
{
    char *          prms[MAXPARMS];
    MACRO *         ThisMacro;

    ThisMacro = (MACRO *) getmem( sizeof(VCLCLASS MACRO) );
    ThisMacro->id = (uchar *) getmem( strlen( (char *) ident ) + 1 );
    strcpy( (char *) ThisMacro->id, (char *) ident );

    /* find parameters, place pointers in prms[] */
    if ( plist )
    {
        ThisMacro->isMacro = 1;
        plist++;
        while ( *plist != ')' )
        {
            while ( isSpace( *plist ) )
                ++plist;
            if ( alphanum( *plist ) )
            {
                if ( ThisMacro->parms == MAXPARMS )
                    error( DEFINERR );
                prms[ThisMacro->parms++] = (char *) plist;
                while ( alphanum( *plist ) )
                    ++plist;
            }
            while ( isSpace( *plist ) )
                ++plist;
            if ( *plist == ',' )
                ++plist;
            else if ( *plist != ')' )
                error( DEFINERR );
        }
    }

    /* build value substituting parameter numbers */
    if ( value )
    {
        ThisMacro->val =
            (uchar *) getmem( strlen( (char *) value ) + 1 + ThisMacro->parms );

        if ( ThisMacro->parms )
        {
            unsigned char * pp = ThisMacro->val;
            int             wasWord = 0;

            while ( *value )
            {
                int             wasWhite = isSpace( *value );

                bypassWhite( &value );

                if ( *value == '\'' || *value == '"' )
                {
                    char            term = *value;

                    /* by this point the string should be assembled */
                    do
                    {
                        if ( ! *value || *value == '\n' )
                            error( DEFINERR );
                        *pp++ = *value++;
                    } while ( *value != term );

                    *pp++ = *value++;
                    continue;
                }

                /* see if the string contains a nested substitution */
                if ( alphanum( *value ) )
                {
                    int             p = 0;

                    if ( wasWhite && wasWord )
                        *pp++ = ' ';
                    wasWord = 1;
                    ExtractWord( Word, &value, (unsigned char *) "_" );

                    while ( p < ThisMacro->parms )
                    {
                        if ( parmcmp( (char *) Word, (char *) prms[p] ) == 0 )
                        {
                            *pp++ = p | 0x80;
                            break;
                        }
                        p++;
                    }
                    if ( p == ThisMacro->parms )
                    {
                        strcpy( (char *) pp, (char *) Word );
                        pp += strlen( (char *) Word );
                    }
                    continue;
                }
                wasWord = 0;
                *pp++ = *value++;
            }
            *pp = '\0';
        }
        else
            /* no parameters, straight substitution */
            strcpy( (char *) ThisMacro->val, (char *) value );
    }

    ThisMacro->NextMacro = FirstMacro;
    FirstMacro = ThisMacro;
    MacroCount++;
} /* AddMacro */


/*
 * Define a new #define macro
 */
void
VCLCLASS DefineMacro (uchar *cp)
{
    int             hasValue = FALSE;
    unsigned char * pp = NULL;          /* parameter pointer */
    unsigned char * vp = NULL;          /* value pointer */
    unsigned char * vp1;                /* end of value */
    unsigned char * wp = NULL;

    bypassWhite( &cp );

    if ( ! Word )
        Word = wp = (uchar *) getmem( MAXMACROLENGTH );

    ExtractWord( Word, &cp, (unsigned char *) "_" );
    if ( *Word == NB )
        error( NEEDIDENT );
    if ( FindMacro( Word ) != NULL )
        error( REDEFPPERR );            /* already defined */

    /*
     * extract parameter list
     *
     * whitespace not allowed between an Identifier and it's Parameter list
     */
    if ( *cp == '(' )
    {
        int             len = 0;

        while ( cp[len] && cp[len] != ')' && cp[len] != '\n' )
            len++;
        if ( cp[len] != ')' )
            error( RPARENERR );
        len++;
        pp = (uchar *) getmem( len + 1 );
        strncpy( (char *) pp, (char *) cp, len );
        cp += len;
    }

    bypassWhite( &cp );

    /*
     * extract macro value
     */
    if ( *cp && *cp != '\n' )
    {
        vp = (uchar *) getmem( strlen( (char *) cp ) + 1 );
        *vp = NB;
    }
    vp1 = vp;
    while ( *cp && *cp != '\n' )
    {
        unsigned char * cp1;
        int             vlen;

        cp1 = cp;                       /* save beginning of value */

        /* scan to end of value */
        while ( *cp && *cp != '\n' )
            cp++;
        --cp;

        /* strip tailing whitespace */
        while ( isSpace( *cp ) )
            --cp;
        cp++;

        /* make sure there's something do define */
        vlen = (int) ( cp - cp1 );
        if ( vlen < 1 )
        {
            if ( ! hasValue )
            {
                free( vp );             /* if nothing was copied */
                vp = NULL;              /* ..let it go & reset pointer */
            }                           /* handle as '#define XXX' */
            break;
        }

        /* copy/append the value */
        strncpy( (char *) vp1, (char *) cp1, vlen );
        hasValue = TRUE;
        vp1[(int) ( cp - cp1 )] = NB;
        vp1 = vp + strlen( (char *) vp ) - 1;

        /* see if the macro definition is continued on the next line */
        if ( *vp1 != '\\' )
            break;
        *vp1 = NB;                      /* keep trailing whitespace */
        ReadString();                   /* read the next line */
        cp = Line;
        bypassWhite( &cp );             /* but strip leading whitespace */
        vp = (uchar *) realloc( vp, strlen( (char *) vp ) + strlen( (char *) cp ) + 1 );
        if ( vp == NULL )
            error( OMERR );
        vp1 = vp + strlen( (char *) vp );
    }

    /*-
     * do not add a symbol defined as itself (circular definition)
     * resolves as an 'undefined symbol' if used anywhere
     */
    if ( vp )                           /* don't strcmp() a null pointer */
    {
        if ( strcmp( (char *) Word, (char *) vp ) )
            AddMacro( Word, pp, vp );
    }
    else
        AddMacro( Word, pp, vp );

    if ( pp )
        free( pp );
    if ( vp )
        free( vp );
    if ( wp )
    {
        free( wp );
        Word = NULL;
    }    
} /* DefineMacro */


/*
 * Remove all macros
 */
void
VCLCLASS UnDefineAllMacros (void)
{
    MACRO *         ThisMacro = FirstMacro;

    while ( ThisMacro != NULL )
    {
        MACRO *         tm = ThisMacro;

        if ( ThisMacro->val )
            free( ThisMacro->val );
        free( ThisMacro->id );
        ThisMacro = ThisMacro->NextMacro;
        free( tm );
    }
    FirstMacro = NULL;
    MacroCount = 0;
} /* UnDefineAllMacros */


/*
 * Undefine a #undef macro
 */
void
VCLCLASS UnDefineMacro (uchar *cp)
{
    MACRO *         ThisMacro;

    bypassWhite( &cp );
    ExtractWord( Word, &cp, (unsigned char *) "_" );
    if ( ( ThisMacro = FindMacro( Word ) ) != NULL )
    {
        if ( ThisMacro == FirstMacro )
            FirstMacro = ThisMacro->NextMacro;
        else
        {
            MACRO *         tm = FirstMacro;

            while ( tm != NULL )
            {
                if ( ThisMacro == tm->NextMacro )
                {
                    tm->NextMacro = ThisMacro->NextMacro;
                    break;
                }
                tm = tm->NextMacro;
            }
        }
        if ( ThisMacro->val )
            free( ThisMacro->val );
        free( ThisMacro->id );
        free( ThisMacro );
        --MacroCount;
    }
} /* UnDefineMacro */


/*
 * #include a source code file
 */
void
VCLCLASS Include (uchar *cp)
{
    FILE *          fp;
    int             LocalInclude;
    int             holdcount;
    unsigned char   holdfileno;
    unsigned char * holdip;
    SRCFILE *       holdfile;
    struct stat     sb;

    if ( Nesting++ == MAXINCLUDES )
    {
        error( INCLUDENESTERR );
        Nesting = 0;
    }
    holdfile = ThisFile;                /* set below if NULL */
    *FilePath = '\0';
    bypassWhite( &cp );

    /* test for #include <file> or #include "file" */
    if ( *cp == '"' )
        LocalInclude = 1;
    else if ( *cp == '<' )
        LocalInclude = 0;
    else
        error( BADPREPROCERR );
    cp++;

    /* extract the file name */
    /* !?! uses legal DOS filename characters, requires porting */
    ExtractWord( Word, &cp, (unsigned char *) ":\\./_^$~!#%&-{}()@'`" );
    if ( *cp != ( LocalInclude ? '"' : '>' ) )
        error( BADPREPROCERR );

    /* build path to included file */
    if ( ! LocalInclude )
    {
        unsigned char * pp;

        strcpy( (char *) FilePath, _argv[0] );
        pp = (unsigned char *) strrchr( (char *) FilePath, '\\' );
        if ( pp != NULL )
            *( pp + 1 ) = '\0';
    }
    strcat( (char *) FilePath, (char *) Word );

    /* add to list of source files */
    ThisFile = (SRCFILE *) getmem( sizeof(VCLCLASS SRCFILE) );
    ThisFile->fname = (uchar *) getmem( strlen( (char *) Word ) + 1 );
    ThisFile->fullname = (uchar *) getmem( strlen( (char *) Word ) + 1 );
    strupr( (char *) Word );
    strcpy( (char *) ThisFile->fname, (char *) Word );
    strcpy( (char *) ThisFile->fullname, (char *) Word );
    ThisFile->isSource = FALSE;
    if ( LastFile != NULL )
        LastFile->NextFile = ThisFile;
    ThisFile->NextFile = NULL;
    LastFile = ThisFile;
    if ( FirstFile == NULL )
        FirstFile = ThisFile;

    /* get file size */
    stat( (char *) FilePath, &sb );

    /* save context of file currently being preprocessed */
    holdip = Ip;
    holdcount = Ctx.CurrLineno;
    holdfileno = Ctx.CurrFileno;

    /* file/line numbers for #included file */
    Ctx.CurrFileno = ++FileCount;
    Ctx.CurrLineno = 0;

    /* open the #included file */
    if ( ( fp = fopen( (char *) FilePath, "rt" ) ) == NULL )
        error( INCLUDEERR );

    /* allocate a buffer and read it in */
    /* !?! maximum file size is 64K by allocation and read technique */
    Ip = ThisFile->IncludeIp = (uchar *) getmem( (unsigned int) ( sb.st_size + 3 ) );
    fread( Ip, (unsigned int) ( sb.st_size ), 1, fp );
    fclose( fp );

    /* make sure there's a newline at the end */
    if ( Ip[(int) (sb.st_size - 1)] != '\n' )
        strcat( (char *) Ip, "\n" );

    /* preprocess the #included file */
    PreProcess();

    /* source code no longer needed */
    if ( ThisFile->IncludeIp )
        free( ThisFile->IncludeIp );
    ThisFile->IncludeIp = NULL;

    /* restore context of file previously being preprocessed */
    Ctx.CurrFileno = holdfileno;
    Ctx.CurrLineno = holdcount;
    Ip = holdip;
    ThisFile = holdfile;
    --Nesting;
} /* Include */


/*
 * Check and increment the 'if' nesting level
 */
int
VCLCLASS TestIfLevel (void)
{
    int             rtn;

    if ( IfLevel == MAXIFS )
        error( IFNESTERR );
    rtn = ! Skipping[IfLevel++];
    Skipping[IfLevel] = Skipping[IfLevel - 1];
    TrueTest[IfLevel] = TrueTest[IfLevel - 1];
    ElseDone[IfLevel] = FALSE;
    return rtn;
} /* TestIfLevel */


/*
 * #if preprocessing token
 */
void
VCLCLASS If (uchar *cp)
{
    if ( TestIfLevel() )
    {
        TrueTest[IfLevel] = ( MacroExpression( &cp ) != 0 );
        Skipping[IfLevel] = ! TrueTest[IfLevel];
    }
} /* If */


/*
 * #ifdef preprocessing token
 */
void
VCLCLASS IfDef (uchar *cp)
{
    if ( TestIfLevel() )
    {
        definedTest = 1;
        TrueTest[IfLevel] = ( MacroExpression( &cp ) != 0 );
        Skipping[IfLevel] = ! TrueTest[IfLevel];
    }
} /* IfDef */


/*
 * #ifndef preprocessing token
 */
void
VCLCLASS IfnDef (uchar *cp)
{
    if ( TestIfLevel() )
    {
        definedTest = -1;
        TrueTest[IfLevel] = ( MacroExpression( &cp ) != 0 );
        Skipping[IfLevel] = ! TrueTest[IfLevel];
    }
} /* IfnDef */


/*
 * #else preprocessing token
 */
void
VCLCLASS Else (void)
{
    if ( IfLevel == 0 )
        error( ELSEERR );
    if ( ElseDone[IfLevel] )
        error( ELSEERR );
    ElseDone[IfLevel] = TRUE;
    Skipping[IfLevel] = TrueTest[IfLevel];
} /* Else */


/*
 * #elif preprocessing token
 */
void
VCLCLASS Elif (uchar *cp)
{
    if ( IfLevel == 0 )
        error( ELIFERR );
    if ( ! TrueTest[IfLevel] )
    {
        TrueTest[IfLevel] = ( MacroExpression( &cp ) != 0 );
        Skipping[IfLevel] = ! TrueTest[IfLevel];
    }
    else
        Skipping[IfLevel] = 1;
} /* Elif */


/*
 * #endif preprocessing token, decrement 'if' nesting level
 */
void
VCLCLASS Endif (void)
{
    if ( IfLevel == 0 )
        error( ENDIFERR );
    ElseDone[IfLevel] = FALSE;
    Skipping[IfLevel] = FALSE;
    TrueTest[IfLevel--] = FALSE;
} /* Endif */


/*
 * #error preprocessing token
 */
void
VCLCLASS Error (uchar *cp)
{
    if ( IfLevel == 0 || ! Skipping[IfLevel] )
    {
        memset( ErrorMsg, 0, sizeof ErrorMsg );
        if ( cp && *cp )
        {
            unsigned char * ep;

            bypassWhite( &cp );
            for ( ep = cp; *ep && *ep != '\n'; ep++ )
                ;
            --ep;
            while ( isSpace( *ep ) )
                --ep;
            strncat( ErrorMsg, (char *) cp, (size_t) (ep - cp + 1) );
        }
        error( ERRORERR );
    }
} /* Error */


/*
 * #pragma preprocessing token
 */
void
VCLCLASS Pragma (uchar *cp)
{

    /* no #pragma directives supported at this time */
    /* any #pragma found will be skipped over and ignored */

    cp = cp;

} /* Pragma */


/*
 * Write a preprocessed line to output
 */
void
VCLCLASS OutputLine (uchar * cp)
{
    unsigned char   lastcp = 0;

    /* output the file & line count comment */
    if ( *cp != '\n' )
        WriteEOL();

    while ( *cp && *cp != '\n' )
    {
        if ( bypassWhite( &cp ) )
        {
            /* stop if we bypassed to the end-of-line */
            if ( ! (*cp) || *cp == '\n' )
                break;

            /* place a space between alphanumeric symbols and +/- operators */
            if ( ( alphanum( *cp ) && alphanum( lastcp ) ) ||
                 *cp == '+' || *cp == '-' )
                WriteChar( ' ' );
        }
        if ( alphanum( *cp ) )
        {
            ResolveMacro( Word, &cp );
            WriteWord( Word );
            lastcp = 'a';               /* output a space between words */
            continue;
        }
        if ( *cp == '"' || *cp == '\'' )
        {
            unsigned char   term = *cp;

            WriteChar( *cp++ );
            while ( *cp != term )
            {
                if ( *cp == '\n' || *cp == '\0' )
                    error( term == '"' ? UNTERMSTRERR : UNTERMCONSTERR );
                WriteChar( *cp++ );
            }
        }
        lastcp = *cp++;
        WriteChar( lastcp );
    }
} /* OutputLine */


/*
 * Write single character to output
 */
void
VCLCLASS WriteChar (uchar c)
{
    *Op++ = c;
} /* WriteChar */


/*
 * Insert fileNo:lineNo comment
 */
void
VCLCLASS WriteEOL (void)
{
    char            eol[20];

    sprintf( eol, "\n/*%d@%d*/",
             Ctx.CurrFileno, Ctx.CurrLineno );
    WriteWord( (unsigned char *) eol );
} /* WriteEOL */


/*
 * Write a null-terminated word to output
 */
void
VCLCLASS WriteWord (unsigned char *s)
{
    int             lastch = 0;

    while ( *s )
    {
        if ( *s == '"' )
        {
            /* the word has a string literal */
            do
                WriteChar( *s++ );
            while ( *s && *s != '"' );
            if ( *s )
                WriteChar( *s++ );
            continue;
        }
        else if ( isSpace( *s ) )
        {
            /* white space */
            while ( isSpace( *s ) )
                s++;
            /* insert one if char literal or id id */
            if ( lastch == '\'' ||
                 ( alphanum( lastch ) && alphanum( *s ) ) )
                WriteChar( ' ' );
            /* process next character separately */
        }
        else
        {
            lastch = *s;
            WriteChar( *s++ );
        }
    }
} /* WriteWord */


/*
 * Read a line from input buffer
 */
int
VCLCLASS ReadString ()
{
    unsigned char * lp;

    if ( *Ip )
    {
        int             len;

        /* compute the line length */
        lp = (unsigned char *) strchr( (char *) Ip, '\n' );
        if ( lp != NULL )
            len = (int) ( lp - Ip + 2 );
        else
            len = strlen( (char *) Ip ) + 1;
        if ( len )
        {
            Ctx.CurrLineno++;
            lp = Line;
            while ( ( *lp++ = *Ip++ ) != '\n' )
            {
                if ( *( lp - 1 ) == '\0' )
                {
                    --Ip;
                    break;
                }
            }
            if ( *( lp - 1 ) == '\n' )
                *lp = '\0';
            return TRUE;
        }
    }

    return FALSE;
} /* ReadString */


/*
 * Find file name from file number
 */
char *
VCLCLASS SrcFileName (int fileno)
{
#ifdef DEBUGGER

    extern WINDOW   editWnd;

    ThisFile = FirstFile;
    while ( ThisFile != NULL && --fileno )
        ThisFile = ThisFile->NextFile;

    return ThisFile ? ThisFile->fname : editWnd->extension;

#else

    if ( fileno > 0 )
    {
        ThisFile = FirstFile;
        while ( ThisFile != NULL && --fileno )
            ThisFile = ThisFile->NextFile;
    } else
        ThisFile = NULL;

    return ThisFile ? (char *) ThisFile->fname : (char *) BaseFile->fname;

#endif
} /* SrcFileName */
