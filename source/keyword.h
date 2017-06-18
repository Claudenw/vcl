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
    keyword.h - VAST Command Language keyword definitions

 DESCRIPTION
    Contains definitions for keywords used within the VAST Command
    Language (VCL).

    If VCL_DECL or __cplusplus are defined the global symbols are declared.

 FILES
    sys.h

 SEE ALSO

 NOTES

 BUGS

********************************************************************unpubMan*/

#ifndef KEYWORD_H                       /* avoid multiple inclusion */
#define KEYWORD_H

#ifndef VCLDEF_H
#include "vcldef.h"
#endif

#ifndef SYS_H
#include "sys.h"
#endif

extern SYMBOLTABLE LibraryFunctions[];

#define MAXLIBFUNCTIONS (sizeof(LibraryFunctions)/sizeof(SYMBOLTABLE))

/* keyword lookup table */
EXTERN SYMBOLTABLE Keywords[];

#define MAXKEYWORDS (sizeof(Keywords)/sizeof(SYMBOLTABLE))

/* multi-character operator lookup tbl */
EXTERN SYMBOLTABLE Operators[] ;

#define MAXOPERATORS (sizeof(Operators)/sizeof(SYMBOLTABLE))

EXTERN SYMBOLTABLE PreProcessors[];

#define MAXPREPROCESSORS (sizeof(PreProcessors)/sizeof(SYMBOLTABLE))

EXTERN SYMBOLTABLE PreDefined[];

#define MAXPREDEFINED (sizeof(PreDefined)/sizeof(SYMBOLTABLE))

#endif                                  /* avoid multiple inclusion */
