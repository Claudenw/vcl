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
    sys.h - System function code definitions

 DESCRIPTION
    Contains the declarations for system function symbols.

 FILES
    none

 SEE ALSO

 NOTES

 BUGS

********************************************************************unpubMan*/

#ifndef SYS_H                           /* avoid multiple inclusion */
#define SYS_H

enum sysfuncs
{

    /*
     * System functions
     */
    SYSEXIT = 1,
    SYSSYSTEM,
    SYSSETJMP,
    SYSLONGJMP,
    SYSFINDFIRST,
    SYSFINDNEXT,
    SYSLINENO,
    SYSFILENAME,
    SYSERRNO,
    /*
     * Console I/O functions
     */
    SYSGETCHAR,
    SYSPUTCHAR,
    SYSGETS,
    SYSPUTS,
    SYSSCANF,
    SYSPRINTF,
    SYSGETCH,
    SYSPUTCH,
    SYSCLRSCRN,
    SYSCURSOR,
    SYSCPRINTF,
    /*
     * File I/O functions
     */
    SYSFOPEN,
    SYSFCLOSE,
    SYSFGETC,
    SYSFPUTC,
    SYSFGETS,
    SYSFPUTS,
    SYSFREAD,
    SYSFWRITE,
    SYSFTELL,
    SYSFSEEK,
    SYSUNGETC,
    SYSRENAME,
    SYSREWIND,
    SYSFSCANF,
    SYSFPRINTF,
    SYSFFLUSH,
    SYSREMOVE,
    SYSTMPFILE,
    SYSTMPNAM,
    /*
     * String functions
     */
    SYSSTRCMP,
    SYSSTRCPY,
    SYSSTRNCMP,
    SYSSTRNCPY,
    SYSSTRLEN,
    SYSSTRCAT,
    SYSSTRNCAT,
    /*
     * Memory allocation functions
     */
    SYSMALLOC,
    SYSFREE,
    /*
     * String conversion routines
     */
    SYSATOF,
    SYSATOI,
    SYSATOL,
    /*
     * Format conversion routines
     */
    SYSSSCANF,
    SYSSPRINTF,
    /*
     * Time functions
     */
    SYSASCTIME,
    SYSGMTIME,
    SYSLOCALTIME,
    SYSMKTIME,
    SYSTIME,
    /*
     * Math functions
     */
    SYSABS,
    SYSACOS,
    SYSASIN,
    SYSATAN,
    SYSCEIL,
    SYSCOS,
    SYSCOSH,
    SYSEXP,
    SYSFABS,
    SYSFLOOR,
    SYSSIN,
    SYSSINH,
    SYSSQRT,
    SYSTAN,
    SYSTANH,
    SYSLOG,
    SYSLOG10,
    SYSPOW,
    SYSATAN2
};

#endif                                  /* avoid multiple inclusion */

