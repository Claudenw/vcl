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

/*pubMain*********************************************************************
 NAME
    vci-mt.c - VAST Command Language Compiler and Interpreter main program

 SYNOPSIS
    vci-mt

 DESCRIPTION
    This is the main program for an RT-Kernel, multi-threaded VCL
    implementation using the C++ encapsulated VCL engine.  The compiler and
    interpreter are combined in this implementation.

    The .VCC source file specified on the command line is compiled and
    executed.

 OPTIONS

 ENVIRONMENT SYMBOLS

 RETURN VALUE

 FILES

 SEE ALSO

 NOTES

 EXAMPLES

 BUGS

*********************************************************************pubMain*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <malloc.h>

#include <inifile.h>

#include <rtkernel.h>
#include <rtkeybrd.h>
#include <rttextio.h>

#include "tskmgmt.h"                    /* must be after RTKernel headers */

/* definitions */
#define BOOT                "Boot"
#define INIFILE             "vci-mt.ini"
#define PRIORITY_TK         3           /* generic task priority */
#define PROGNAME            "VCI-MT"

/* task management globals */
TskMgmt_t *     TskList;                /* allocated array of structures */
ulong           TskCount = 0L;          /* perpetual task-run count metrix */

/* runtime .INI file parameters */
typedef struct RUNINI
{
    uint        boxSize;
    uint        boxSlots;
    uint        mainPriority;
    int         maxTasks;
    uint        priority;
    uint        stack;
    int         yield;
} RunIni_t;

RunIni_t        RunIni;

/* globals */
char *          arg0;
int             GlobalReturnValue = 0;

/* prototypes */
void            VclRtkInstance (void);  /* !?! should be in separate header */
void            terminate (int);        /* terminate execution of program */
int             launch (char *);
int             runtimeINI (void);

/* externals */
extern unsigned int _stklen = 16384U;   /* this program's stack size */


/*
 * main entry point
 *------------------*/
void
main (int argc, char **argv)
{

    argc = argc;                        /* avoid 'not used' compiler warning */
    arg0 = argv[0];                     /* global for first argument */

    /*
     * read the .INI for runtime parameters
     */
    if ( ! runtimeINI() )
        terminate( errno );

    /* allocate runtime task management array */
    TskList = (TskMgmt_t *) calloc( 1, (RunIni.maxTasks + 1) * sizeof(TskMgmt_t) );
    if ( TskList == NULL )
        terminate( ENOMEM );

    /*
     * initialize RT-Kernel
     */
    if ( RTKDebugVersion() )
        printf( "\n" );
    RTKernelInit( RunIni.mainPriority );
    RTKeybrdInit();
    RTTextIOInit();                     /* needed for text output */
    RTKProtectLibrary = TRUE;
    RTKPreemptionsON();
    RTKTimeSlice( 9 );

    /*
     * launch the [Boot] section, "Load" keywords of .INI file
     */
    if ( launch( NULL ) )
    {
        int     allDone = FALSE;
        int     hTsk;
        int     state;

        /*-####
         #
         # Main Task loop
         #
         # Wait for all tasks to terminate
         #
         #-####*/

        while ( ! allDone )
        {
            for ( allDone = TRUE, hTsk = 1; hTskValid( hTsk ); ++hTsk )
            {
                state = TskGetState( hTsk );

                /* check exception states */
                if ( state == TSK_DEADLOCKED || state == TSK_UNKNOWN )
                {
                    fprintf( stderr,
                        "\n*** *** *** ***\n%s TASK STATE !\n*** *** *** ***\n",
                        (state == TSK_DEADLOCKED) ? "DEADLOCKED" : "UNKNOWN" );
                    allDone = TRUE;
                    break;
                }
                /* check if task is halted pending kill/free */
                else if ( state == TSK_HALTED )
                {
                    TskKill( hTsk );
                }
                /* all others are running states */
                else if ( state != TSK_FREE )
                {
                    allDone = FALSE;
                    TskYield();     /* yield CPU for each running task */
                }
            }
        }
    }
    else
        fprintf( stderr, "\n*** *** *** ***\nLAUNCH FAILED !\n*** *** *** ***\n" );

    terminate( ( GlobalReturnValue ) ? GlobalReturnValue : errno );

} /* main */


/*
 * Terminate program
 *
 * Perform orderly shutdown & exit with code
 *-------------------------------------------*/
void
terminate (int errcode)
{
    /*
     * Other shut-down code goes here
     */

    TskKillAll();                       /* kill/free all tasks */

    if ( errcode )
        fprintf( stderr, "%s: Error code %d\n", PROGNAME, errcode );
    if ( errno )
        perror( PROGNAME );

    exit( errcode );
} /* terminate */


/*
 * Launch the [Boot] via RT-Kernel
 *
 * Loads all tasks specified in the
 * [Boot] section with "Load" keyword
 *------------------------------------*/
int
launch (char *sub)
{
    char *      cmd;
    int         i = 1;                  /* entry number is 1-based */
    char        key[KEYSZ + 1];

    /* !?! add RunMode parameter to "Load" entries; only exec if RUN_NOW */
    /* !?! then handle mapping to RUN_ONDEMAND tasks via NRM */

    while ( (cmd = iniReadAll( INIFILE, NULL, BOOT, sub, &i, key )) != NULL )
    {
        if ( ! stricmp( key, "Load" ) )
        {
            if ( ! TskExec( cmd ))
                return FALSE;
        }
        free( cmd );
    }
    return TRUE;
} /* launch */


/*
 * Read the .INI for runtime parameters
 *--------------------------------------*/
int
runtimeINI (void)
{
    char *      sec;

    /* [Main] section */
    sec = "Main";
    RunIni.mainPriority = iniReadInt( INIFILE, NULL, sec, NULL, "Priority", 32 );
    RunIni.maxTasks = iniReadInt( INIFILE, NULL, sec, NULL, "MaxTasks", 16 );
    /* use yield = 0 for RTK "cooperative time-slicing" */
    RunIni.yield = iniReadInt( INIFILE, NULL, sec, NULL, "Yield", 10 );

    /* [Task] section */
    sec = "Task";
    RunIni.boxSize = iniReadInt( INIFILE, NULL, sec, NULL, "BoxSize", 1024 );
    RunIni.boxSlots = iniReadInt( INIFILE, NULL, sec, NULL, "BoxSlots", 2 );
    RunIni.priority = iniReadInt( INIFILE, NULL, sec, NULL, "Priority", 32 );
    RunIni.stack = iniReadInt( INIFILE, NULL, sec, NULL, "Stack", 8192 );
    return TRUE;
} /* runtimeINI */


/*==========================================================================
  TASK MANAGEMENT ROUTINES

  !?! Should be broken into a separate module ??
  ==========================================================================*/

/*
 *  Allocate a task handle
 *-------------------------*/
int
TskAlloc (void)
{
    int         hTsk;

    /* !?! this routine will need to be semaphored before added to VCL API */

    /* find first available task management slot */
    for ( hTsk = 1; hTskValid( hTsk); ++hTsk )
    {
        if ( TskGetState( hTsk ) == TSK_FREE )
        {
            TskSetState( hTsk, TSK_WAITING );
            break;
        }
    }

    if ( ! hTskValid( hTsk ) )
    {
        errno = EAGAIN;                 /* resource not available */
        hTsk = 0;                       /* no slot available, fail */
    }

    return hTsk;
} /* TskAlloc */


/*
 * Execute a task
 *
 * The task handle int is 1-based; a 0 handle is invalid.  The array
 * is allocated as RunIni.MaxTasks + 1, so the first [0] structure is
 * not used.  Yes, it's wasted, but this saves more bytes of code than
 * it costs in data.
 *
 * Could conceivably use the [0]th structure to keep the data for the
 * Main Task (TaskHandle Tskp returned by RTKernelInit().
 *--------------------------------------------------------------------*/
int
TskExec (char *commandLine)
{
    int             hTsk;               /* handle to task, 1-based, 0 invalid */
    char            nam[12];            /* name buffer */

    /* allocate a task handle */
    hTsk = TskAlloc();                  /* sets state to TSK_WAITING */
    if ( ! hTskValid( hTsk ) )
        return 0;

    /* setup this task's command line */
    TskSetCmd( hTsk, strdup( commandLine ) );
    if ( TskGetCmd( hTsk ) == NULL )
    {
        errno = ENOMEM;
        return 0;
    }

    /* setup the mailbox name and task name */
    /* !?! sema */
    sprintf( nam, "%d", hTsk );

    ++TskCount;                         /* inc task-run count metrix */

    /* create a mailbox for the task */
    TskSetBox( hTsk, RTKCreateMailbox( RunIni.boxSize, RunIni.boxSlots, nam ) );

    /* execute the task */
    TskSetRetval( hTsk, 0 );
    TskSetHandle( hTsk, RTKCreateTask( VclRtkInstance, RunIni.priority, RunIni.stack, nam ) );

    /* message-pass the task handle number */
    RTKSend( TskGetHandle( hTsk), &hTsk );

    return TRUE;
} /* TskExec */


/*
 *  Get a task's state
 *---------------------*/
int
TskGetState (int hTsk)
{
    int         state = TSK_UNKNOWN;

    if ( hTskValid( hTsk ) )
    {
        if ( TskList[hTsk].state == TSK_RUNNING )
        {
            /* task is running, check with engine for update */
            switch ( RTKGetTaskState( TskGetHandle( hTsk )) )
            {
                case Ready :
                case Current :
                case Delaying :
                    state = TSK_RUNNING;
                    break;
                case Suspended :
                    state = TSK_SUSPENDED;
                    break;
                case BlockedWait :
                case TimedWait :
                case BlockedPut :
                case BlockedGet :
                case TimedPut :
                case TimedGet :
                case BlockedSend :
                case BlockedReceive :
                case TimedSend :
                case TimedReceive :
                    state = TSK_WAITING;
                    break;
                case Deadlocked :
                    state = TSK_DEADLOCKED;
                    break;
                case Illegal :
                case Terminated :
                    state = TSK_HALTED;
                    break;
            }
            TskSetState( hTsk, state ); /* update the task state */
        }
        else                            /* return (not running) state */
            state = TskList[hTsk].state;
    }
    return state;
} /* TskGetState */


/*
 * Kill a task
 *-------------*/
int
TskKill (int hTsk)
{
    Mailbox     mBox;
    TaskHandle  tHdl;

    if ( hTskValid( hTsk ) )
    {
        /* kill task if in a running state */
        if ( TskRunning( hTsk ) )
        {
            /* !?! raise internal signal?, or notify task it's getting killed */
            /* let/cause task to clean up */

            /* terminate task */
            tHdl = TskGetHandle( hTsk );
            RTKTerminateTask( &tHdl );
        }

        /* delete task mailbox */
        if ( (mBox = TskGetBox( hTsk )) != NULL )
            RTKDeleteMailbox( &mBox );

        /* reset task handle */
        TskSetHandle( hTsk, NULL );

        /* reset box handle */
        TskSetBox( hTsk, NULL );

        /* free any attached command line */
        if ( TskGetCmd( hTsk ) )
            free( TskGetCmd( hTsk ) );
        TskSetCmd( hTsk, NULL );

        /* reset task return value */
        TskSetRetval( hTsk, 0 );

        /* now the slot is free for reuse */
        TskSetState( hTsk, TSK_FREE );
    }

    return TRUE;
} /* TskKill */


/*
 *  Kill all tasks
 *------------------*/
int
TskKillAll (void)
{
    int         hTsk;

    for ( hTsk = 1; hTskValid( hTsk); ++hTsk )
    {
        if ( TskGetState( hTsk ) != TSK_FREE )
        {
            if ( ! TskKill( hTsk ) )
                return FALSE;
        }
    }
    return TRUE;
} /* TskKillAll */


/*
 *  Yield a task's execution
 *---------------------------*/
void
TskYield (void)
{
    RTKDelay( RunIni.yield );
}

