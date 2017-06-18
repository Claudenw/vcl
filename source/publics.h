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
    publics.h - VCL local public data definitions

 DESCRIPTION
    Contains public definitions required internally by the VAST Command
    Language (VCL).  See NOTES.

 GLOBAL VARIABLES

 FILES

 SEE ALSO

 NOTES
    This header is public -within- VCL.  It is essentially private, but
    to allow one (and only one) instance of data definitions which are
    constant and are not changed during runtime, some definitions are
    required to be public.

 BUGS

**********************************************************************pubMan*/

#ifndef PUBLICS_H                       /* avoid multiple inclusion */
#define PUBLICS_H

/* Preprocessor pcode tokens */
enum PreProcTokens
{
    P_DEFINE = 1,
    P_ELIF,
    P_ELSE,
    P_ENDIF,
    P_ERROR,
    P_IF,
    P_IFDEF,
    P_IFNDEF,
    P_INCLUDE,
    P_PRAGMA,
    P_UNDEF
};

enum PreDefinedTokens
{
    PD_CDECL = 1,
    PD_VCL,
    PD_DATE,
    PD_FILE,
    PD_LINE,
    PD_OS,
    PD_TIME
};

/* Symbol table */
typedef struct symbol
{
    char *          symbol;             /* symbol name */
    int             ident;              /* symbol identification */
} SYMBOLTABLE;

/* TO: character assignment functions prototypes */
/* s - s */
void            cscs (void);            /* 0 */
void            csis (void);
void            csls (void);
void            csds (void);
/* s - u */
void            cscu (void);            /* 4 */
void            csiu (void);
void            cslu (void);
/* csds() would be a duplicate.  Floating point is always signed */
/* u - s */
void            cucs (void);            /* 8 */
void            cuis (void);
void            culs (void);
void            cuds (void);
/* u - u */
void            cucu (void);            /* 12 */
void            cuiu (void);
void            culu (void);
/* cudu() would be a duplicate.  Floating point is always signed */
/* TO: integer assignment functions prototypes */
/* s - s */
void            iscs (void);            /* 16 */
void            isis (void);
void            isls (void);
void            isds (void);
/* s - u */
void            iscu (void);            /* 20 */
void            isiu (void);
void            islu (void);
/* isds() would be a duplicate.  Floating point is always signed */
/* u - s */
void            iucs (void);            /* 24 */
void            iuis (void);
void            iuls (void);
void            iuds (void);
/* u - u */
void            iucu (void);            /* 28 */
void            iuiu (void);
void            iulu (void);
/* iudu() would be a duplicate.  Floating point is always signed */
/* TO: long assignment functions prototypes */
/* s - s */
void            lscs (void);            /* 32 */
void            lsis (void);
void            lsls (void);
void            lsds (void);
/* s - u */
void            lscu (void);            /* 36 */
void            lsiu (void);
void            lslu (void);
/* lsds() would be a duplicate.  Floating point is always signed */
/* u - s */
void            lucs (void);            /* 40 */
void            luis (void);
void            luls (void);
void            luds (void);
/* u - u */
void            lucu (void);            /* 44 */
void            luiu (void);
void            lulu (void);
/* ludu() would be a duplicate.  Floating point is always signed */
/* TO: double assignment functions prototypes */
/* s - s */
void            dscs (void);            /* 48 */
void            dsis (void);
void            dsls (void);
void            dsds (void);
/* s - u */
void            dscu (void);            /* 52 */
void            dsiu (void);
void            dslu (void);
/* dsdu() would be a duplicate.  Floating point is always signed */
/* u - s */
/* No TO unsigned double.  Floating point is always signed */
/* u - u */
/* No TO unsigned double.  Floating point is always signed */

#endif /* avoid mutiple inclusion */

