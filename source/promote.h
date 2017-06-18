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
    promote.h - VAST Command Language data type promotion definitions

 DESCRIPTION
    Contains definitions for data type promotion functions and handling
    within the VAST Command Language (VCL).

    If VCL_DECL or __cplusplus are defined the global symbols are declared.

 FILES

 SEE ALSO

 NOTES

 BUGS

********************************************************************unpubMan*/

#ifndef PROMOTE_H                       /* avoid multiple inclusion */
#define PROMOTE_H

#ifndef VCLDEF_H
#include "vcldef.h"
#endif

/*-
 * Setup the assignment / type-promotion function pointer array.
 * This array is in a very specific order.  The appropriate assignment
 * function is selected by calculating an index into this array.
 *
 * The order is TO - FROM by type:
 *      char
 *          s - s       ---+------ signed TO first half
 *              char       |
 *              int        |
 *              long       |
 *              double     |
 *          s - u          |
 *              char       |       ---+------ unsigned FROM second quarter
 *              int        |          |
 *              long       |          |
 *              double  ---+       ---+
 *          u - s       ---+------ unsigned TO second half
 *              char       |
 *              int        |
 *              long       |
 *              double     |
 *          u - u          |       ---+------ unsigned FROM fourth quarter
 *              char       |          |
 *              int        |          |
 *              long       |          |
 *              double  ---+       ---+
 *      int
 *          s - s
 *              char    ---+-+-+-+ right-side types in this order
 *              int        | | | |
 *              long       | | | |
 *              double  ---+ | | |
 *          s - u            | | |
 *              char    -----+ | |
 *              int          | | |
 *              long         | | |
 *              double  -----+ | |
 *          u - s              | |
 *              char    -------+ |
 *              int            | |
 *              long           | |
 *              double  -------+ |
 *          u - u                |
 *              char    ---------+
 *              int              |
 *              long             |
 *              double  ---------+
 *      long
 *          ... same as above ...
 *      float and double
 *          ... same as above ...
 */
EXTERN PFVOID   storeFunc[] = {
    /*
     * TO: character
     */
    /* s - s */
    cscs,                               /* 0 */
    csis,
    csls,
    csds,
    /* s - u */
    cscu,                               /* 4 */
    csiu,
    cslu,
    csds,                               /* floating point is always signed */
    /* u - s */
    cucs,                               /* 8 */
    cuis,
    culs,
    cuds,
    /* u - u */
    cucu,                               /* 12 */
    cuiu,
    culu,
    cuds,                               /* floating point is always signed */
    /*
     * TO: integer
     */
    /* s - s */
    iscs,                               /* 16 */
    isis,
    isls,
    isds,
    /* s - u */
    iscu,                               /* 20 */
    isiu,
    islu,
    isds,                               /* floating point is always signed */
    /* u - s */
    iucs,                               /* 24 */
    iuis,
    iuls,
    iuds,
    /* u - u */
    iucu,                               /* 28 */
    iuiu,
    iulu,
    iuds,                               /* floating point is always signed */
    /*
     * TO: long
     */
    /* s - s */
    lscs,                               /* 32 */
    lsis,
    lsls,
    lsds,
    /* s - u */
    lscu,                               /* 36 */
    lsiu,
    lslu,
    lsds,                               /* floating point is always signed */
    /* u - s */
    lucs,                               /* 40 */
    luis,
    luls,
    luds,
    /* u - u */
    lucu,                               /* 44 */
    luiu,
    lulu,
    luds,                               /* floating point is always signed */
    /*
     * TO: double
     */
    /* s - s */
    dscs,                               /* 48*/
    dsis,
    dsls,
    dsds,
    /* s - u */
    dscu,                               /* 52 */
    dsiu,
    dslu,
    dsds,                               /* floating point is always signed */
    /* u - s floating point is always signed */
    dscs,                               /* 56 */
    dsis,
    dsls,
    dsds,
    /* u - u floating point is always signed */
    dscu,                               /* 60 */
    dsiu,
    dslu,
    dsds                                /* 63 ... The-End */
};

#endif                                  /* avoid multiple inclusion */
