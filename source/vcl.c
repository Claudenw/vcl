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
    vcl.c - VAST Command Language Compiler/Interpreter

 DESCRIPTION
    This module contains the main entry point for executing VCL, vclRuntime().
    Also contains the external shutdown routine vclShutdown().

    Contains routines for executing a VCL program from source code,
    memory allocation and error handling.

 FUNCTIONS
    vclRuntime()
    vclShutdown()
    error()
    warning()
    getmem()
    AssertFail()

    InitVcl()
    CompileVcl()
    ExecuteVcl()
    DumpStats()
    LoadSource()
    SetConfig()
    PrintPreprocess()
    ClearMemory()

 ENVIRONMENT SYMBOLS
    none as yet

 FILES
    vcldef.h
    vcl.h
    sclib.h

 SEE ALSO

 NOTES
    Function SetConfig() sets various runtime memory allocation sizes.
    !?! Some of these should come from a .INI file.  There may be others
    such as MAXALLOCS in sys.c that may be moved to a .INI file.

 BUGS

*****************************************************************unpubModule*/

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <string.h>
#include <dir.h>
#include <dos.h>
#include <mem.h>
#include <setjmp.h>
#include <time.h>
#include <sys\stat.h>
#include <alloc.h>
#include <errno.h>

#include <sclib.h>
#ifdef __cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcl.h"
#include "vcldef.h"
#endif

//#include "dflat.h"
//#include "debugger.h"


/*pubMan**********************************************************************
 NAME
    vclRuntime - VAST Command Language Compiler/Interpreter entry point

 SYNOPSIS
    int vclRuntime (int argc, char **argv)

 DESCRIPTION
    This is the main entry point for executing VCL.  Arguments argc and
    argv similar to those passed to a C program's main() function.  Argv[0]
    must contain the full path to the underlying binary program for the
    preprocessor default include file path to work correctly.  The order
    of the options is:

        exePath(0) [runtimeOptions] programSourceFilename [programOptions]

    Runtime options are specified before the VCL source filename.  The first
    non-option argument is taken as the source filename.  Any options
    following the source filename are passed on to that program.  The
    following runtime options are supported:

    Compiler options:
        -c              Compile only, default compile and execute.

        -l              No line numbers in pcode, default line numbers
                        are embedded in the pcode.

        -Dmac[=val]     Define mac, optionally equal value.  No
                        spaces.  Non-numeric values must be enclosed
                        in double quotes (e.g. -DFPATH="F:\\DATA").
                        Whitespace within quotes is preserved.
                        Note: The compiler handles command-line
                        defined strings the same as source code
                        #defined strings, as string constants.  Any
                        backslash character sequences are converted,
                        e.g. \t = tab, \a = bell.  If backslashes
                        are not to be expanded use 2 in a row as in
                        the example above.

        -P              Print the preprocessed code to sourcename.PRE.
                        Note: This file is overwritten without warning.

        -q              Quiet mode, print only errors and warnings.

        -V              Print version information

    Interpreter options:
        none as yet

    Predefine symbols provided by the preprocessor:
        CDECL           TRUE, calling convention is c
        VCL             hex constant MMmm, MMajor and mminor VCL version
        __DATE__        string constant date, format "Mmm dd yyyy"
        __FILE__        string constant current file short name
        __LINE__        unsigned constant, current file line
        __MSDOS__       TRUE if operating system is MSDOS
        __TIME__        string constant time, format "hh:mm:ss"

 ENVIRONMENT SYMBOLS
    none as yet

 FILES
    vcl.h

 SEE ALSO

 NOTES

 BUGS

**********************************************************************pubMan*/

int
VCLCLASS vclRuntime (int argc, char **argv)
{
    uchar *         buff = NULL;        /* source code buffer */
    int             ret = 0;            /* return value */
    char *          srcFilename;        /* source filename from command line */

    /* allocate internal message buffer */
    ErrorMsg = (char *) getmem( MAXERRMSG );
    *ErrorMsg = '\0';

    VclGlobalInit();

    /*-
     * Process the command line arguments starting with 1 (not 0).
     *
     * Synopsis: [runtimeOptions] programSourceFilename [programOptions]
     *
     * Options begin with '-' (or '/' in DOS).  Processes only the leading
     * runtimeOptions and programSourceFilename.  As runtimeOptions are
     * processed all remaining argv[] pointers are shuffled down in the
     * argv array.  The first non-option argument is taken as the source
     * code filename.  All programOptions are past on to the VCL program
     * to be executed.
     */

    for ( srcFilename = NULL; srcFilename == NULL && argc > 1 ; )
    {
        int         argstaken;
        char *      cp;
        int         i;

        cp = argv[1];
        argstaken = 1;

        /* process leading runtime options */
        if ( *cp == '-'
#if DOS                                     /* allow a slash in DOS */
             || *cp == '/'
#endif
           )
        {
            ++cp;                           /* incr past option character */

            switch ( *cp )                  /* case-sensitive comparison */
            {
                case 'c' :                  /* compile only */
                    rtopt.CompileOnly = TRUE;
                    break;
                case 'l' :                  /* line numbers */
                    rtopt.NoLineNumbers = TRUE;
                    break;
                case 'D' :                  /* define a symbol */
                    {
                        ++cp;               /* incr past D */
                        if ( *cp )
                        {
                            char *      eqp;
                            char *      mp;
                            char        gotMem = FALSE;

                            mp = cp;

                            /* replace any equal sign with a space */
                            if (( eqp = strchr( cp, '=' )) != NULL )
                                *eqp = ' ';

                            /* if there was an '=' */
                            if ( eqp )
                            {
                                /* if there was nothing after the '=' .. */
                                /* and there are more arguments */
                                if ( ! *(eqp + 1) && argc > 2 )
                                {
                                    char        c;

                                    /* if the next arg is not an option */
                                    c = *argv[2];
                                    if ( c != '-'
#if DOS
                                         && c != '/'
#endif
                                       )
                                    {
                                        /* build 'mac "value"' string */
                                        mp = (char *) getmem( strlen( cp ) +
                                                     strlen( argv[2] ) + 3 );
                                        gotMem = TRUE;
                                        strcpy( mp, cp );
                                        strcat( mp, "\"" );
                                        strcat( mp, argv[2] );
                                        strcat( mp, "\"" );
                                        ++argstaken;       
                                    }
                                }
                            }
                            DefineMacro( (uchar *) mp );
                            if ( gotMem )
                                free( mp );
                        }
                    }
                    break;
                case 'H' :              /* print usage */
                    Usage();
                    exit( 1 );
                    break;
                case 'P' :              /* print preprocessed code */
                    rtopt.PrintPreprocess = TRUE;
                    break;
                case 'q' :              /* quiet mode */
                    rtopt.QuietMode = TRUE;
                    break;
                case 'V' :              /* print version information */
                    PrintVersion();
                    exit( 1 );
                    break;
                default :
                    sprintf( ErrorMsg, "'%c' ignored", *cp );
                    warning( BADVCLOPT );
                    break;
            }
        }
        else
        {
            /* otherwise, take the first non-option arg as source filename */
            srcFilename = argv[1];
        }

        /* shuffle remaining pointers (if any) down in argv array */
        for ( ; argstaken; --argstaken, --argc )
        {
            for ( i = 1; ( i + 1 ) < argc; ++i )
            {
                argv[i] = argv[i + 1];
            }
        }    
    }

    if ( ! rtopt.QuietMode )
        PrintBanner();

    /* set configuration parameters */
    SetConfig();

    /* load the source code */
    if ((buff = LoadSource( srcFilename )) == NULL )
    {
        if ( srcFilename && *srcFilename )
            printf( "Cannot find file %s(.VCC)\n", srcFilename );
        else
            printf( "No source file(s) specified\nUse -H for help\n" );

        if ( FirstFile )
            DeleteFileList( FirstFile );
        ret = 1;
    }
    else
    {
        /* allocate VCL runtime memory */
        InitVcl();

        /* preprocess & tokenize the source code */
        CompileVcl( buff );

        /* link & execute the pseudocode */
        ret = ExecuteVcl( &buff, argc, argv );

        if ( rtopt.CompileOnly && ret == 0 )
            printf( "compile successful\n" );

        /* dump verbose runtime statistics */
        if ( ! rtopt.QuietMode )
            DumpStats();
        
        /* cleanup runtime environment */
        vclShutdown();
    }

    free( ErrorMsg );

    return ret;
} /* vclRuntime */


/*
 * Initialize the VCL Runtime
 */
int
VCLCLASS InitVcl (void)
{
    /* pcode area, reallocated to precise size after tokenization */
    Progstart = (uchar *) getmem( vclCfg.MaxProgram );

    /* allocate memory for runtime stack */
    Stackbtm = (ITEM *) getmem( ( vclCfg.MaxStack + 1 ) * sizeof( struct item ) );
    Ctx.Stackptr = Stackbtm;
    Stacktop = Stackbtm + vclCfg.MaxStack;

    /* allocate memory for VARIABLE structures */
    VariableMemory = (VARIABLE *) getmem( vclCfg.MaxVariables * sizeof( VARIABLE ) );
    Ctx.NextVar = VariableMemory;

    /* allocate memory for user variable data space */
    if ( ( DataSpace = (char *) malloc( vclCfg.MaxDataSpace ) ) == NULL )
        error( OMERR );
    Ctx.NextData = DataSpace;

    /* allocate memory for FUNCTION structures */
    FunctionMemory = (FUNCTION *) getmem( vclCfg.MaxFunctions * sizeof( FUNCTION ) );
    NextFunction = FunctionMemory;

    /* allocate memory for SYMBOLTABLE structures */
    SymbolTable = (SYMBOLTABLE *) getmem( vclCfg.MaxSymbolTable * sizeof( SYMBOLTABLE ) );

    /* allocate memory for function prototype characters */
    NextProto = (uchar *) PrototypeMemory = (uchar *) getmem( vclCfg.MaxPrototype );

    return TRUE;
} /* InitVcl */


/*
 * Compile VCL program(s)
 *
 * Preprocesses and tokenizes the source code
 */
int
VCLCLASS CompileVcl (uchar *src)
{
#ifdef DEBUGGER
    wwnd = WatchIcon();
#endif

    if ( setjmp( Shelljmp ) == 0 )
    {
        unsigned char * pSrc;

        fflush( stdin );
        fflush( stdout );

        /* get a buffer & preprocess source code */
        pSrc = (uchar *) getmem( vclCfg.MaxProgram );
        PreProcessor( pSrc, src );

        if ( rtopt.PrintPreprocess )
            PrintPreprocess( pSrc );

        /* tokenize preprocessed code */
        Progused = tokenize( (char *) Progstart, (char *) pSrc );

        fflush( stdin );
        fflush( stdout );

        /* free the preprocessor buffer */
        free( pSrc );

        /* reallocate the pcode buffer */
        Progstart = (uchar *) realloc( Progstart, Progused + 1 );
    }
    return TRUE;
} /* CompileVcl */


/*
 * Execute a VCL program
 *
 * Links and executes the pseudocode
 */
int
VCLCLASS ExecuteVcl (uchar **srcp, int argc, char *argv[])
{
    int     i;
    char    startupVcl[] =              /* decl array for ln[ sizeof()...] */
            "return main(%d,(char**)%luUL);";
    uchar   ln[ sizeof( startupVcl )+ 6 + 32 + 1 ];  /* argc=6, big argv=32 */
    uchar   Tknbuf[128];                /* just a good size buffer */

    if ( setjmp( Shelljmp ) == 0 )
    {
        char *  sargv0;                 /* save argv[0] pointer */

        ErrorCode = 0;

        /* link global symbols */
        if ( ! rtopt.QuietMode )
            printf( "Linking   %s:\n", ThisFile->fname );
        link( &Globals );

        if ( rtopt.CompileOnly )        /* return if compile only */
            return ErrorCode;

        /*
         * Release the source code memory.  Done here in case of errors
         * during link phase and the IDE needs to position to source.
         */
        if ( ! KEEPSOURCE && FirstFile && FirstFile->IncludeIp )
        {
            free( FirstFile->IncludeIp );
            FirstFile->IncludeIp = NULL;
            *srcp = NULL;
        }

        /* reallocate the variable space */
        VariableMemory = (VARIABLE *) realloc( VariableMemory, (VariablesUsed + 1) * sizeof( VARIABLE ) );

        /* reallocate the function space */
        FunctionMemory = (FUNCTION *) realloc( FunctionMemory, (FunctionsCount + 1) * sizeof( FUNCTION ) );

        /* reallocate the symbol table */
        SymbolTable = (SYMBOLTABLE *) realloc( SymbolTable, (SymbolCount + 1) * sizeof( SYMBOLTABLE ) );

        /* reallocate the prototype buffer */
        i = (int) (NextProto - (uchar *) PrototypeMemory) + 1;
        PrototypeMemory = (char *) realloc( PrototypeMemory, i );
        NextProto = (uchar *) PrototypeMemory + i - 1;

        /*
         * Setup argv[0] to the fully qualified VCL program path.  Set here
         * so the original executable's path can be used during compilation
         * as an alternate path to the "system" include files.
         */
        sargv0 = argv[0];
        argv[0] = qualify_path( NULL, (char *) FirstFile->fullname );

        /* tokenize the startupVcl code */
        sprintf( (char *) ln, startupVcl, argc, argv );
        tokenize( (char *) Tknbuf, (char *) ln );
        Ctx.Progptr = Tknbuf;

        /* get the first token of startupVcl */
        getoken();

#ifdef DEBUGGER
        SendMessage( wwnd, CLOSE_WINDOW, 0, 0 );
        wwnd = NULL;
        if ( ! Stepping )
            HideIDE();
#endif

        /*
         * execute the psuedo-code
         */
        if ( ! rtopt.QuietMode )
            printf( "Executing %s:\n", ThisFile->fname );

        fflush( stdin );
        fflush( stdout );

        statement();

        fflush( stdin );
        fflush( stdout );

        /* restore original argv[0] */
        free( argv[0] );
        argv[0] = sargv0;
    }

#ifdef DEBUGGER
    if ( wwnd != NULL )
        SendMessage( wwnd, CLOSE_WINDOW, 0, 0 );
#endif

    return ErrorCode ? ErrorCode : popint();

} /* ExecuteVcl */


/*
 * Shutdown a VCL program
 *
 * Releases -all- allocated memory
 */
void
VCLCLASS vclShutdown (void)
{
    CloseAllOpenFiles();                /* close any remaining open files */
    ClearHeap();                        /* free all runtime allocations */
    DeleteSymbols();                    /* free symbol values */
    CleanUpPreProcessor();              /* free all macros & file list */
    if ( FirstFile )                    /* free main program file */
        DeleteFileList( FirstFile );

    ClearMemory( &(void *) PrototypeMemory, &(void *) NextProto, NULL );
    ClearMemory( &(void *) SymbolTable, NULL, &SymbolCount );
    ClearMemory( &(void *) FunctionMemory, &(void *) NextFunction, NULL );
    ClearMemory( &(void *) DataSpace, &(void *) Ctx.NextData, NULL );
    ClearMemory( &(void *) VariableMemory, &(void *) Ctx.NextVar, NULL );
    ClearMemory( &(void *) Stackbtm, &(void *) Ctx.Stackptr, NULL );
    ClearMemory( &(void *) Progstart, NULL, &(int) Progused );
    errno = 0;
} /* vclShutDown */


/*
 * Dump runtime statistics
 */
void
VCLCLASS DumpStats (void)
{
    long        l;

#ifndef DEBUGGER
    printf( "\n" );
    printf( "Program... %6ld of %6ld bytes, %6.02lf%%*\n",
            (long) Progused,
            (long) vclCfg.MaxProgram,
            (((double)((double)Progused /
              (double)vclCfg.MaxProgram)) * 100.0) );
    printf( "Symbol.... %6ld of %6ld bytes, %6.02lf%%*\n",
            (long) SymbolCount * sizeof (SYMBOLTABLE),
            (long) vclCfg.MaxSymbolTable * sizeof (SYMBOLTABLE),
            (((double)((double)(SymbolCount * sizeof (SYMBOLTABLE)) /
              ((double)(vclCfg.MaxSymbolTable * sizeof (SYMBOLTABLE))))) * 100.0) );
    printf( "Prototype. %6ld of %6ld bytes, %6.02lf%%*\n",
            (long) ( (char *) NextProto - (char *) PrototypeMemory ),
            (long) vclCfg.MaxPrototype,
            (((double)((double)((char *) NextProto - (char *) PrototypeMemory)) /
              (double)vclCfg.MaxPrototype) * 100.0) );
    printf( "Function.. %6ld of %6ld bytes, %6.02lf%%*\n",
            (long) ( (char *) NextFunction - (char *) FunctionMemory ),
            (long) vclCfg.MaxFunctions * sizeof (FUNCTION),
            (((double)((double)((char *) NextFunction - (char *) FunctionMemory)) /
              ((double)(vclCfg.MaxFunctions * sizeof (FUNCTION)))) * 100.0) );
    printf( "Variable.. %6ld of %6ld bytes, %6.02lf%%*\n",
            (long) ( (char *) Ctx.NextVar - (char *) VariableMemory ),
            (long) vclCfg.MaxVariables * sizeof (VARIABLE),
            (((double)((double)((char *) Ctx.NextVar - (char *) VariableMemory)) /
              ((double)(vclCfg.MaxVariables * sizeof (VARIABLE)))) * 100.0) );
    l = (long) ( (char *) MaxDataSpace - (char *) DataSpace );
    printf( "Data...... %6ld of %6ld bytes, %6.02lf%%\n",
            ( l > 0L ) ? l : 0L,
            (long) vclCfg.MaxDataSpace,
            ( l > 0L ) ? (((double)((double)((char *) MaxDataSpace - (char *) DataSpace )) /
              (double)vclCfg.MaxDataSpace) * 100.0) : 0.00 );
    l = (long) ( (char *) Stackmax - (char *) Ctx.Stackptr );
    printf( "Stack..... %6ld of %6ld bytes, %6.02lf%%\n",
            ( l > 0L ) ? l : 0L,
            (long) vclCfg.MaxStack * sizeof (struct item),
            ( l > 0L ) ? (((double)((double)((char *) Stackmax - (char *) Ctx.Stackptr)) /
              ((double)(vclCfg.MaxStack * sizeof (struct item)))) * 100.0) : 0.00 );
    printf( "* = reallocated prior to runtime\n" );
#endif
} /* DumpStats */


/*
 * Load the source code into memory
 *
 * The fullname of the source file is not expanded to
 * a fully qualified pathname, i.e. the fullname is
 * kept the way it was specified.  This way, a developer
 * can use relative pathnames by choice, and we don't
 * change them.
 *
 * Returns NULL on error
 */
unsigned char *
VCLCLASS LoadSource (char *name)
{
    unsigned char * buff;
    FILE *          fp;
    char            dir[MAXDIR];
    char            drive[MAXDRIVE];
    char            filename[MAXFILE];
    char            ext[MAXEXT];
    struct stat     sb;
    char            fullpath[MAXPATH];
    char            shortpath[MAXFILE+MAXEXT];

    /* don't bother if source path not specified */
    if ( ! strlen( name ) )
        return NULL;

    /* split the source path into it's components */
    fnsplit( name, drive, dir, filename, ext );

    /* add a default file extension if not specified */
    if ( *ext == 0 )
        strcpy( ext, ".VCC" );

    /* merge the components back together */
    fnmerge( fullpath, drive, dir, filename, ext );
    fnmerge( shortpath, "", "", filename, ext );

    /* add to list of source files */
    ThisFile = (SRCFILE *) getmem( sizeof( SRCFILE ) );
    ThisFile->fullname = (uchar *) getmem( strlen( fullpath ) + 1 );
    ThisFile->fname = (uchar *) getmem( strlen( shortpath ) + 1 );
    ThisFile->isSource = TRUE;
    if ( LastFile != NULL )
        LastFile->NextFile = ThisFile;
    ThisFile->NextFile = NULL;
    LastFile = ThisFile;
    if ( FirstFile == NULL )
        FirstFile = ThisFile;
    BaseFile = ThisFile;

    /* file/line numbers for source file */
    Ctx.CurrFileno = ++FileCount;
    Ctx.CurrLineno = 0;

    /* setup the source path */
    strupr( fullpath );
    strupr( shortpath );
    if ( ! rtopt.QuietMode )
        printf( "Compiling %s:\n", shortpath );
    strcpy( (char *) ThisFile->fullname, fullpath );
    strcpy( (char *) ThisFile->fname, shortpath );

    /* get file size */
    if ( stat( fullpath, &sb ) )
        return NULL;
    if ( (sb.st_size + 3) > 65534L )
    {
        printf( "Error %s 0: Internal error, source file > 64K bytes (999)\n",
                shortpath );
        return NULL;
    }

    /* allocate the buffer */
    /* !?! 64K file size + predefined limit */
    buff = (uchar *) getmem( (unsigned int) ( sb.st_size + 3 ) );
    ThisFile->IncludeIp = buff;

    /* open, read and close the source file */
    /* !?! 64K file read limit */
    if ( ( fp = fopen( fullpath, "r" ) ) == NULL )
        return NULL;
    fread( buff, (unsigned int) ( sb.st_size ), 1, fp );
    fclose( fp );

    /* make sure there's a newline at the end */
    if ( buff[(int) (sb.st_size - 1)] != '\n' )
        strcat( (char *) buff, "\n" );

    return buff;
} /* LoadSource */


/*
 * Set configuration parameters
 *
 * !?! (Some of) these should be placed in a .INI file
 *
 * Be caution of ripple effects if any of these values exceed 64K.
 * Other variables may require wider data types (e.g. Progused relative
 * to vclCfg.MaxProgram).
 */
void
VCLCLASS SetConfig (void)
{
    vclCfg.MaxProgram     = 60 * 1024;  /* pseudocode space, bytes */
    vclCfg.MaxStack       = 256;        /* number of stack items */
    vclCfg.MaxVariables   = 512;        /* number of variables */
    vclCfg.MaxFunctions   = 256;        /* number of functions */
    vclCfg.MaxDataSpace   = 16 * 1024;  /* data space, bytes */
    vclCfg.MaxSymbolTable = 1024;       /* number of symbol table entries */
    vclCfg.MaxPrototype   = 2048;       /* prototype table space, bytes */
} /* SetConfig */


/*
 * Print preprocessed source code
 */
void
VCLCLASS PrintPreprocess (uchar *pSrc)
{
    char *      cp;
    FILE *      fp;
    char        pn[MAXPATH];

    /* build the sourcename.pre path */
    strcpy( pn, (char *) ThisFile->fullname );
    for ( cp = &pn[ strlen( pn ) - 1 ]; cp > pn; --cp )
    {
        if ( *cp == '.' )
        {
            *cp = NB;
            break;
        }
        else if ( *cp == '\\' || *cp == '/' || *cp == ':' )
            break;
    }
    strcat( pn, ".pre" );

    if ( (fp = fopen( pn, "w" )) == NULL )
    {
        sprintf( ErrorMsg, "cannot open '%s' for writing", pn );
        warning( FILERR );
        return;
    }    

    cp = (char *) pSrc;
    while ( *cp )
        fprintf( fp, "%c", *cp++ );

    fclose( fp );
} /* PrintPreprocess */


/*
 * Release an allocated block and it's pointer and item count
 */
void
VCLCLASS ClearMemory (void **bufp, void **endp, int *countp)
{
    if ( *bufp )
    {
        free( *bufp );
        *bufp = NULL;
    }
    if ( endp )
        *endp = NULL;
    if ( countp )
        *countp = 0;
} /* ClearMemory */


void
VCLCLASS error (int errnum)
{
    ErrorCode = errnum;

    if ( Ctx.CurrFileno == 0 )
        printf( "Line number information not available\n" );
    printf( "Error %s %d: %s (id:%d)",
            SrcFileName( Ctx.CurrFileno ),
            Ctx.CurrLineno, errs[errnum - 1], errnum );
    if ( ErrorMsg && *ErrorMsg )
    {
        printf( ": %s", ErrorMsg );
        *ErrorMsg = '\0';
    }
    printf( "\n" );
#if DEBUGGER
    if ( Watching )
        longjmp( Watchjmp, 1 );
    else if ( Running )
        longjmp( Shelljmp, 1 );
#endif
    exit( 1 );
} /* error */


void
VCLCLASS warning (int errnum)
{
    ErrorCode = errnum;

    if ( Ctx.CurrFileno == 0 )
        printf( "Line number information not available\n" );
    printf( "Warning %s %d: %s (id:%d)",
            strupr( SrcFileName( Ctx.CurrFileno ) ),
            Ctx.CurrLineno, errs[errnum - 1], errnum );
    if ( ErrorMsg && *ErrorMsg )
    {
        printf( ": %s", ErrorMsg );
        *ErrorMsg = '\0';
    }
    printf( "\n" );
} /* warning */


void *
VCLCLASS getmem (unsigned size)
{
    void *          ptr;

    if ( ( ptr = calloc( 1, size ) ) == NULL )
        error( OMERR );
    return ptr;
} /* getmem */


#ifndef NDEBUG
void
VCLCLASS AssertFail (char *cond, char *file, int lno)
{
    sprintf( errs[ASSERTERR - 1], "Internal assertion failed: %s, file %s, line %d",
             cond, file, lno );
    error( ASSERTERR );
} /* AssertFail */
#endif


/*
 * Print usage information
 *
 * !?! This routine should be moved to the underlying executable.
 */
void
VCLCLASS Usage (void)
{
    PrintBanner();

    printf( "usage:\n" );
    printf( "    %s [options] programName programOptions\n\n", PROGNAME );
    
    printf( "options:\n" );
    printf( "    -c              Compile only\n" );
    printf( "    -l              No line numbers in pcode\n" );
    printf( "    -Dmac[=num]     Define mac, optionally equal numeric value\n" );
    printf( "    -Dmac[=\"str\"]   Define mac, optionally equal string\n" );
    printf( "    -H              Print this help\n" );
    printf( "    -P              Print the preprocessed code to programName.PRE\n" );
    printf( "    -q              Quiet mode, print errors & warnings only\n" );
    printf( "    -V              Print version information\n" );
} /* Usage */


void
VCLCLASS PrintVersion (void)
{
    PrintBanner();
    printf( "Version %s, %s\n", PROGVERS, COMPILER_NAME );
    printf( "Compiled %s at %s\n", __DATE__, __TIME__ );
} /* PrintVersion */


void
VCLCLASS PrintBanner (void)
{
    printf( "\n%s v%s: %s\n\n", PROGNAME, PROGVERS, PROGDESC );
} /* PrintBanner */

