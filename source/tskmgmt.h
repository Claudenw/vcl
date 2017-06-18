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
    tskmgmt.h - VAST Task Management definitions

 DESCRIPTION
    Contains definitions of symbols and structures used by VAST for real-
    time task management.  Used only in multitasking implementations of
    VAST.

 GLOBAL VARIABLES
    TskMgmt_t *     TskList;            allocated array of structures
    ulong           TskCount;           perpetual task-run count metrix

 NOTES

**********************************************************************pubMan*/

#ifndef TSKMGMT_H                       /* avoid multiple inclusion */
#define TSKMGMT_H

#ifdef __cplusplus
extern "C" {
#endif

/* enums */
enum TSKSTATES
{
    TSK_FREE = 0,                       /* slot is not in use */
    TSK_RUNNING,                        /* running */
    TSK_WAITING,                        /* waiting for something, running */
    TSK_SUSPENDED,                      /* suspended, running */
    TSK_DEADLOCKED,                     /* send-blocked by halted task, running */
    TSK_HALTED,                         /* halted, pending kill */
    TSK_UNKNOWN                         /* unknown state, internal exception */
};

enum RUNMODES
{
    RUN_NOW = 1,
    RUN_ONDEMAND
};

/* typedefs */
typedef struct TSKMGMT
{
    int             state;              /* our definition of task states */
    TaskHandle      tskp;               /* pointer (handle) to engine task */
    Mailbox         boxp;               /* pointer (handle) to IPC mailbox */
    char *          cmdline;            /* pointer to allocated command line */
    int             retval;             /* return value of task */
} TskMgmt_t;

/* global references */
extern TskMgmt_t *  TskList;            /* allocated array of structures */
extern ulong        TskCount;           /* perpetual task-run count metrix */

/* macros */
#define hTskValid(th)           (th && th <= RunIni.maxTasks)
#define TskGetBox(th)           (TskList[th].boxp)
#define TskGetCmd(th)           (TskList[th].cmdline)
#define TskGetHandle(th)        (TskList[th].tskp)
#define TskGetRetval(th)        (TskList[th].retval)
#define TskPtrTo(th)            (&TskList[th])
#define TskSetBox(th,p)         (TskList[th].boxp = p)
#define TskSetCmd(th,p)         (TskList[th].cmdline = p)
#define TskSetState(th,i)       (TskList[th].state = i)
#define TskSetHandle(th,p)      (TskList[th].tskp = p)
#define TskSetRetval(th,i)      (TskList[th].retval = i)
#define TskRunning(th)          (TskList[th].state == TSK_RUNNING || \
                                 TskList[th].state == TSK_WAITING || \
                                 TskList[th].state == TSK_SUSPENDED || \
                                 TskList[th].state == TSK_DEADLOCKED)

/* prototypes */
int             TskAlloc (void);
int             TskExec (char *);
int             TskGetState (int);
int             TskKill (int);
int             TskKillAll (void);
void            TskYield (void);

#ifdef __cplusplus
}
#endif

#endif /* avoid mutiple inclusion */

