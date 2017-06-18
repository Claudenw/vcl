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
    vci-st.c - VAST Command Language Compiler and Interpreter main program

 SYNOPSIS
    vci-st

 DESCRIPTION
    This is the main program for an RT-Kernel, single-threaded VCL
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

#include <rtkernel.h>
#include <rtkeybrd.h>
#include <rttextio.h>

/* definitions */
#define PRIORITY_TK         3           /* generic task priority */

/* globals */
int             GlobalReturnValue = 0;

/* prototypes */
void            VclRtkInstance (void);  /* !?! should be in separate header */
void            terminate (int);        /* terminate execution of program */

/* externals */
extern unsigned int _stklen = 16384U;   /* this program's stack size */


/*
 * main entry point
 *------------------*/
void
main (int argc, char **argv)
{
    TaskHandle      hVclTask;
    TaskHandle      hWaitTask = NULL;

    argc = argc;
    argv = argv;

    /*
     *  Initialize the Real-Time Kernel
     */
    if (RTKDebugVersion())
        printf( "\n" );
    RTKernelInit( 3 );
    RTKeybrdInit();
    RTTextIOInit();                     /* needed for text output */

    /*
     *  Execute the interpreter
     */
    hVclTask = RTKCreateTask( VclRtkInstance, 3, 8192, "VCL Interpreter" );
    hWaitTask = hVclTask;

    /*
     *  Wait for task to complete
     */
    for ( ;; ) {
        if ( RTKGetTaskState( hWaitTask ) == Terminated )
            break;
        RTKDelay( 10 );
    }

    terminate( GlobalReturnValue );

} /* main */


/*
 * Terminate task
 *
 * Perform orderly shutdown & exit with code
 *-------------------------------------------*/
void
terminate (int errcode)
{
    /*
     * Other shut-down code goes here
     */

    exit( errcode );
} /* terminate */

