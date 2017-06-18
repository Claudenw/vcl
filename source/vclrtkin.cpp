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

/*pubMan**********************************************************************
 NAME

 SYNOPSIS

 DESCRIPTION

 GLOBAL TYPES & ENUM VARIABLES

 ENVIRONMENT SYMBOLS

 RETURN VALUE

 FILES

 SEE ALSO

 NOTES

 SIDE EFFECTS

 BUGS

**********************************************************************pubMan*/

extern "C" {

#include <errno.h>
#include <malloc.h>

#include <rtkernel.h>

#include <scdef.h>
#include <sclib.h>
#include "vcl.hpp"

#include "tskmgmt.h"

void
VclRtkInstance (void)
{
    int             hTsk;
    int             i;
    int             ret;
    int             vclArgc;
    char **         vclArgv;
    VclClass *      vclRun;

    extern char *   arg0;
    extern int      GlobalReturnValue;

    /* get this task's handle */
    RTKReceive( &hTsk, sizeof( hTsk ) );
    printf( "task num %d\ncmd line '%s'\n", hTsk, TskGetCmd( hTsk ));

    /* parse the command line */
    if ( ( vclArgc = parseLine( TskGetCmd( hTsk ), arg0, &vclArgv )) > 0 )
    {
        /* print the arguments */
        printf( "arg count %d\n", vclArgc );
        for (i = 0; i < vclArgc; ++i)
            printf( "arg %d: '%s'\n", i, vclArgv[i] );

        /* allocate a class object on the heap */
        vclRun = new VclClass;

        /* run the VCL program */
        TskSetState( hTsk, TSK_RUNNING );
        ret = vclRun->vclRuntime ( vclArgc, vclArgv );
        TskSetRetval( hTsk, ret );
        TskSetState( hTsk, TSK_WAITING );   /* wait to complete this function */

        /* free the allocated arguments */
        for ( i = 0; i < vclArgc; ++i )
            free( vclArgv[i] );

        /* free the allocated array */
        free( vclArgv );

        /* free the class object space */
        delete vclRun;
    }
    else
        ret = EINVAL;                   /* invalid argument */

    /* set the global return value */
    /* !?! sema */
    if ( ! GlobalReturnValue )          /* don't overwrite existing error */
        GlobalReturnValue = ret;

    printf( "Exiting VclRtkInstance \"%s\"\n", TskGetCmd( hTsk ) );

    TskSetState( hTsk, TSK_HALTED );    /* now we're halted */

} /* VclRtkInstance */

} /* extern "C" */
