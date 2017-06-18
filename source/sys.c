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

/*unpubMan********************************************************************
 NAME
    sys.c - System calls module

 DESCRIPTION
    Handles calling bound library function arguments and calling functions.
    Also tracks internally opened files.

 FUNCTIONS
    CloseAllOpenFiles()
    ClearHeap()
    sys()

    AddOpenFile()
    RemoveOpenFile()
    chkchannel()
    cvtfmt()
    pprintf()
    pscanf()
    psprintf()
    psscanf()
    pfprintf()
    pfscanf()
    pcprintf()
    FixStacktmStructure()
    unstkmem()

 FILES
    vcldef.h

 SEE ALSO

 NOTES
    !?! This module will be rewritten to provide .DLL dynamic library support.

 BUGS

********************************************************************unpubMan*/

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <conio.h>                      // For cprintf(), getch etc...
#include <dos.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <dir.h>
#include <time.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

FILE *
VCLCLASS AddOpenFile (FILE *fp)
{
    if ( fp )
    {
        if ( OpenFileCount == MAXOPENFILES )
        {
            fclose( fp );
            return NULL;
        }
        OpenFiles[OpenFileCount++] = fp;
    }
    return fp;
} /* AddOpenFile */


void
VCLCLASS RemoveOpenFile (FILE *fp)
{
    int             found;
    int             i;

    if ( fp )
    {
        /* find open file pointer */
        for ( i = 0, found = FALSE; i < OpenFileCount; i++ )
        {
            if ( OpenFiles[i] == fp )
            {
                found = TRUE;
                break;
            }
        }
        /* shuffle remaining pointers down in array */
        if ( found )
            for ( --OpenFileCount; i < OpenFileCount; i++ )
                OpenFiles[i] = OpenFiles[i + 1];
    }
} /* RemoveOpenFile */


void
VCLCLASS CloseAllOpenFiles (void)
{
    int             i;

    for ( i = 0; i < OpenFileCount; i++ )
        fclose( OpenFiles[i] );
    OpenFileCount = 0;
} /* CloseAllOpenFiles */


FILE *
VCLCLASS chkchannel (FILE *channel)
{
    long            lchan = (long) channel;

#if DEBUGGER
    /*
     * Converts vcl std handles to real ones Opens screen display in IDE for
     * stdout, stderr
     */
    WasFileFunction = 1;
    if ( lchan < 5 )
    {
        switch ( lchan )
        {
            case 0:
                /* stdin */
                WasConsole = DupStdin == -1;
                break;
            case 1:
                /* stdout */
                WasConsole = DupStdout == -1;
                break;
            case 2:
                /* stderr */
                WasConsole = 1;
                break;
            default:
                WasConsole = 0;
                break;
        }
        if ( WasConsole )
            OpenStdout();
        return handles[(int) lchan];
    }
#else
    if ( lchan < 5L )
        return handles[(int) lchan];
#endif
    return channel;
} /* chkchannel */


/* convert scanf strings so that all %f or %Lf formats become %lf */
char *
VCLCLASS cvtfmt (char *f2)
{
    char *          f1;
    char            ch;

    f1 = ConvertedFormat;
    while ( *f2 )
    {
        Assert( f1 < ConvertedFormat + 200 );
        if ( ( *f1++ = *f2++ ) == '%' )
        {
            /*
             * found a format specifier, bypass suppression char, width,
             * and pointer size modifier
             */
            while ( *f2 == '*' || isdigit( *f2 ) || *f2 == 'L' )
                *f1++ = *f2++;
            ch = tolower( *f2 );
            if ( ( *f2 == 'f' || ch == 'e' || ch == 'g' ) && *( f2 - 1 ) != 'l' )
            {
                if ( *( f2 - 1 ) == 'L' )       /* convert %Lf to %lf */
                    *( f1 - 1 ) = 'l';
                else
                    *f1++ = 'l';        /* convert %f to %lf */
            }
        }
    }
    *f1 = '\0';
    return ConvertedFormat;
} /* cvtfmt */


/*
 * need these functions to get variable arguments
 * on the stack, which is the only place that vprintf, etc,
 * will process them when DS != SS
 */
int
VCLCLASS pprintf (char *fmt, struct vstack vs)
{
    return vprintf( fmt, &vs );
} /* pprintf */


int
VCLCLASS pscanf (char *fmt, struct vstack vs)
{
    return vscanf( cvtfmt( fmt ), &vs );
} /* pscanf */


int
VCLCLASS psprintf (char *str, char *fmt, struct vstack vs)
{
    return vsprintf( str, fmt, &vs );
} /* psprintf */


int
VCLCLASS psscanf (char *str, char *fmt, struct vstack vs)
{
    return vsscanf( str, cvtfmt( fmt ), &vs );
} /* psscanf */


int
VCLCLASS pfprintf (FILE *fp, char *fmt, struct vstack vs)
{
    FILE *          fp1 = chkchannel( fp );

    return vfprintf( fp1, fmt, &vs );
} /* pfprintf */


int
VCLCLASS pfscanf (FILE *fp, char *fmt, struct vstack vs)
{
    FILE *          fp1 = chkchannel( fp );

    return vfscanf( fp1, cvtfmt( fmt ), &vs );
} /* pfscanf */


int
VCLCLASS pcprintf (char *fmt, struct vstack vs)
{
    char *          buf = (char *) getmem( 512 );
    int             rtn;

    vsprintf( buf, fmt, &vs );
    rtn = cprintf( buf );
    free( buf );
    return rtn;
} /* pcprintf */


void
VCLCLASS FixStacktmStructure (void)
{
    int             symbolid = FindSymbol( "tm" );

    if ( symbolid )
    {
        VARIABLE *      tvar = SearchVariable( symbolid, 1 );

        if ( tvar != NULL )
            Ctx.Stackptr->vstruct = tvar->vstruct;
    }
} /* FixStacktmStructure */


/*
 * keep track of memory allocations
 */

/* remove all allocated blocks */
void
VCLCLASS ClearHeap (void)
{
    int             i;

    for ( i = 0; i < memctr; i++ )
    {
        if ( allocs[i] )
        {
            free( allocs[i] );
            allocs[i] = NULL;
        }
    }        
    memctr = 0;
} /* ClearHeap */


/* remove an allocated block from the list */
void
VCLCLASS unstkmem (char *adr)
{
    int             i;

    free( adr );
    for ( i = 0; i < memctr; i++ )
        if ( adr == allocs[i] )
            break;
    if ( i < memctr )
    {
        --memctr;
        while ( i < memctr )
        {
            allocs[i] = allocs[i + 1];
            i++;
        }
    }
} /* unstkmem */


/*
 * Call an internal binary bound library function
 */
void
VCLCLASS sys (void)
{
    FILE *          fp;
    char *          cp,
    *               cp1;
    int             n;
    int             l;
    enum sysfuncs   c;
    double          f1;
    double          f2;
    int             ch;

#ifdef DEBUGGER
    int             x;
    int             y;
#endif

    /* first argument on stack is the sys function number */
    c = (enum sysfuncs) popint();

    /* note: as in C, arguments are pushed right-to-left */
    WasFileFunction = WasConsole = 0;
    switch ( c )
    {
            /*
             * System functions
             */
        case SYSEXIT:
            longjmp( Shelljmp, 5 );
        case SYSSYSTEM:
#ifdef DEBUGGER
            OpenStdout();
#endif
            pushint( system( (char *) popptr() ), FALSE );
#ifdef DEBUGGER
            if ( Stepping )
                PromptIDE();
            CloseStdout();
#endif
            return;
        case SYSFINDFIRST:
            n = popint();
            cp1 = (char *) popptr();
            cp = (char *) popptr();
            pushint( findfirst( cp, (struct ffblk *) cp1, n ), FALSE );
            return;
        case SYSFINDNEXT:
            pushint( findnext( (struct ffblk *) popptr() ), FALSE );
            return;
            /*
             * String conversion functions
             */
        case SYSATOF:
            pushflt( atof( (const char *) popptr() ), FALSE );
            return;
        case SYSATOI:
            pushint( atoi( (const char *) popptr() ), FALSE );
            return;
        case SYSATOL:
            pushlng( atol( (const char *) popptr() ), FALSE );
            return;
            /*
             * String functions
             */
        case SYSSTRCMP:
            cp1 = (char *) popptr();
            cp = (char *) popptr();
            pushint( strcmp( cp, cp1 ), FALSE );
            return;
        case SYSSTRNCMP:
            n = popint();
            cp1 = (char *) popptr();
            cp = (char *) popptr();
            pushint( strncmp( cp, cp1, n ), FALSE );
            return;
        case SYSSTRCPY:
            cp1 = (char *) popptr();
            cp = (char *) popptr();
            pushptr( strcpy( cp, cp1 ), CHAR, FALSE );
            return;
        case SYSSTRNCPY:
            n = popint();
            cp1 = (char *) popptr();
            cp = (char *) popptr();
            pushptr( strncpy( cp, cp1, n ), CHAR, FALSE );
            return;
        case SYSSTRLEN:
            pushint( strlen( (const char *) popptr() ), TRUE );
            return;
        case SYSSTRCAT:
            cp1 = (char *) popptr();
            cp = (char *) popptr();
            pushptr( strcat( cp, cp1 ), CHAR, FALSE );
            return;
        case SYSSTRNCAT:
            n = popint();
            cp1 = (char *) popptr();
            cp = (char *) popptr();
            pushptr( strncat( cp, cp1, n ), CHAR, FALSE );
            return;
            /*
             * memory allocation functions
             */
        case SYSMALLOC:
            {
                int             siz = popint();
                char *          mem = (char *) malloc( siz );

                if ( mem != NULL && memctr < MAXALLOC )
                    allocs[memctr++] = mem;
                else
                {
                    free( mem );
                    mem = NULL;
                }
                pushptr( mem, VOID, FALSE );
                return;
            }
        case SYSFREE:
            unstkmem( (char *) popptr() );
            pushint( 0, FALSE );
            return;
            /*
             * Format conversion functions
             */
        case SYSSSCANF:
            pushint( psscanf( *(char **) Ctx.Curfunc->ldata,
                              *(char **) ( Ctx.Curfunc->ldata + sizeof( char * ) ),
                              *( struct vstack * ) ( Ctx.Curfunc->ldata + ( sizeof(char * ) * 2) ) ),
                              FALSE );
            return;
        case SYSSPRINTF:
            pushint( psprintf( *(char **) Ctx.Curfunc->ldata,
                               *(char **) ( Ctx.Curfunc->ldata + sizeof( char * ) ),
                               *( struct vstack * ) ( Ctx.Curfunc->ldata + ( sizeof(char * ) * 2) ) ),
                               FALSE );
            return;

        case SYSABS:
            pushint( abs( popint() ), FALSE );
            return;
            /*
             * File I/O functions
             */
        case SYSFSEEK:
            {
                int             md = popint();
                long            j = poplng();

                fp = chkchannel( (FILE *) popptr() );
                pushint( fseek( fp, j, md ), FALSE );
                return;
            }
        case SYSFOPEN:
            {
                char *          md = (char *) popptr();
                char *          fn = (char *) popptr();

                if ( ( fp = fopen( fn, md ) ) != NULL )
                    fp = AddOpenFile( fp );
                pushptr( fp, INT, FALSE );
                return;
            }
        case SYSTMPFILE:
            if ( ( fp = tmpfile() ) != NULL )
                fp = AddOpenFile( fp );
            pushptr( fp, INT, FALSE );
            return;
        case SYSTMPNAM:
            pushptr( tmpnam( (char *) popptr() ), CHAR, FALSE );
            return;
        case SYSREMOVE:
            pushint( remove( (char *) popptr() ), FALSE );
            return;
        case SYSFCLOSE:
            fp = (FILE *) popptr();
            RemoveOpenFile( fp );
            pushint( fclose( fp ), FALSE );
            return;
        case SYSFFLUSH:
            fp = chkchannel( (FILE *) popptr() );
            pushint( fflush( fp ), FALSE );
            break;
        case SYSFGETC:
            fp = chkchannel( (FILE *) popptr() );
            pushint( getc( fp ), FALSE );
            break;
        case SYSUNGETC:
            fp = chkchannel( (FILE *) popptr() );
            pushint( ungetc( popint(), fp ), FALSE );
            break;
        case SYSFPUTC:
            fp = chkchannel( (FILE *) popptr() );
            pushint( putc( popint(), fp ), FALSE );
            break;
        case SYSFGETS:
            fp = chkchannel( (FILE *) popptr() );
            n = popint();
            pushptr( fgets( (char *) popptr(), n, fp ), CHAR, FALSE );
            break;
        case SYSFPUTS:
            fp = chkchannel( (FILE *) popptr() );
            pushint( fputs( (char *) popptr(), fp ), FALSE );
            break;
        case SYSFREAD:
            fp = chkchannel( (FILE *) popptr() );
            n = popint();
            l = popint();
            cp = (char *) popptr();
            pushint( fread( cp, l, n, fp ), FALSE );
            break;
        case SYSFWRITE:
            fp = chkchannel( (FILE *) popptr() );
            n = popint();
            l = popint();
            cp = (char *) popptr();
            pushint( fwrite( cp, l, n, fp ), FALSE );
            break;
        case SYSFTELL:
            fp = chkchannel( (FILE *) popptr() );
            pushlng( ftell( fp ), FALSE );
            break;
        case SYSRENAME:
            cp1 = (char *) popptr();
            cp = (char *) popptr();
            pushint( rename( cp, cp1 ), FALSE );
            return;
        case SYSREWIND:
            fp = chkchannel( (FILE *) popptr() );
            rewind( fp );
            pushint( 0, FALSE );
            break;
        case SYSFSCANF:
            pushint( pfscanf( *(FILE **) Ctx.Curfunc->ldata,
                              *(char **) ( Ctx.Curfunc->ldata + sizeof( char * ) ),
                              *( struct vstack * ) ( Ctx.Curfunc->ldata + sizeof( FILE * ) + sizeof( char * ) ) ),
                              FALSE );
            break;
        case SYSFPRINTF:
            pushint( pfprintf( *(FILE **) Ctx.Curfunc->ldata,
                               *(char **) ( Ctx.Curfunc->ldata + sizeof( char * ) ),
                               *( struct vstack * ) ( Ctx.Curfunc->ldata + sizeof( FILE * ) + sizeof( char * ) ) ),
                               FALSE );
            break;
        case SYSASCTIME:
            pushptr( asctime( (const struct tm *) popptr() ), CHAR, FALSE );
            return;
        case SYSGMTIME:
            pushptr( gmtime( (const long *) popptr() ), STRUCT, FALSE );
            Ctx.Stackptr->size = sizeof( struct tm );
            FixStacktmStructure();
            return;
        case SYSLOCALTIME:
            pushptr( localtime( (const long *) popptr() ), STRUCT, FALSE );
            Ctx.Stackptr->size = sizeof( struct tm );
            FixStacktmStructure();
            return;
        case SYSMKTIME:
            pushlng( mktime( (struct tm *) popptr() ), FALSE );
            return;
        case SYSTIME:
            pushlng( time( (long *) popptr() ), FALSE );
            return;
        default:
            if ( c >= SYSACOS )
            {
                /*
                 * math function
                 */
                f2 = popflt();
                if ( c == SYSPOW )
                {
                    f1 = popflt();
                    pushflt( pow( f1, f2 ), FALSE );
                }
                else if ( c == SYSATAN2 )
                {
                    f1 = popflt();
                    pushflt( atan2( f1, f2 ), FALSE );
                }
                else if ( c < SYSPOW )
                    pushflt( ( *mfunc[c - SYSACOS] ) ( f2 ), FALSE );
                else
                    pushint( 0, FALSE );
                return;
            }
            break;
    }

    if ( WasFileFunction )
    {
#if DEBUGGER
        if ( WasConsole )
            CloseStdout();
#endif
        return;
    }

    /* these functions will or might use the screen */
    switch ( c )
    {
            /*
             * Console input functions
             */
        case SYSGETCH:
#ifdef DEBUGGER
            OpenStdout();
            if ( ( ch = getch() ) == 3 )
                CBreak();
#else
            ch = getch();
#endif
            if ( ch == 0 )
                ch = getch() | 0x80;    /* set the high bit */
            pushint( ch, TRUE );
#if DEBUGGER
            CloseStdout();
#endif
            return;
        case SYSPUTCH:
#if DEBUGGER
            OpenStdout();
#endif
            putch( popint() );
            pushint( 0, FALSE );
#if DEBUGGER
            CloseStdout();
#endif
            return;
        case SYSCLRSCRN:
#if DEBUGGER
            OpenStdout();
            clearscreen();
            pushint( 0 );
            CloseStdout();
#else
            pushint( 0, FALSE );

#endif
            return;
        case SYSCURSOR:
#if DEBUGGER
            OpenStdout();
#endif
#ifdef DEBUGGER
            y = popint();
            x = popint();
            cursor( x, y );
#else
            popint();                   /* Get the x, and y off to keep the */
            popint();                   /* ..stack straight */
#endif
            pushint( 0, FALSE );
#if DEBUGGER
            CloseStdout();
#endif
            return;
        case SYSGETCHAR:
#if DEBUGGER
            if ( DupStdin == -1 )
                OpenStdout();
#endif
            pushint( getchar(), FALSE );
#if DEBUGGER
            if ( DupStdin == -1 )
                CloseStdout();
#endif
            return;
        case SYSGETS:
#if DEBUGGER
            if ( DupStdin == -1 )
                OpenStdout();
#endif
            pushptr( gets( (char *) popptr() ), CHAR, FALSE );
#if DEBUGGER
            if ( DupStdin == -1 )
                CloseStdout();
#endif
            return;
        case SYSSCANF:
#if DEBUGGER
            if ( DupStdin == -1 )
                OpenStdout();
#endif
            pushint( pscanf( *(char **) Ctx.Curfunc->ldata,
                             *( struct vstack * ) ( Ctx.Curfunc->ldata + sizeof( char * ) ) ),
                             FALSE );
#if DEBUGGER
            if ( DupStdin == -1 )
            {
                fflush( stdin );
                CloseStdout();
            }
#endif
            return;
        default:
            break;
    }

    switch ( c )
    {
            /*
             * Console output functions
             */
        case SYSPUTCHAR:
#if DEBUGGER
            if ( DupStdout == -1 )
                OpenStdout();
#endif
            pushint( putchar( popint() ), FALSE );
#if DEBUGGER
            if ( DupStdout == -1 )
                CloseStdout();
#endif
            break;
        case SYSPUTS:
#if DEBUGGER
            if ( DupStdout == -1 )
                OpenStdout();
#endif
            pushint( puts( (char *) popptr() ), FALSE );
#if DEBUGGER
            if ( DupStdout == -1 )
                CloseStdout();
#endif
            break;
        case SYSPRINTF:
#if DEBUGGER
            if ( DupStdout == -1 )
                OpenStdout();
#endif
            pushint( pprintf( *(char **) Ctx.Curfunc->ldata,
                              *( struct vstack * ) ( Ctx.Curfunc->ldata + sizeof( char * ) ) ),
                              FALSE );
#if DEBUGGER
            if ( DupStdout == -1 )
                CloseStdout();
#endif
            break;
        case SYSCPRINTF:
#if DEBUGGER
            OpenStdout();
#endif
            pushint( pcprintf( *(char **) Ctx.Curfunc->ldata,
                               *( struct vstack * ) ( Ctx.Curfunc->ldata + sizeof( char * ) ) ),
                               FALSE );
#if DEBUGGER
            CloseStdout();
#endif
            break;
        default:
            break;
    }
} /* sys */
