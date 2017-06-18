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
    errs.h - Error message symbols

 DESCRIPTION
    Contains the declarations for symbols for all VCL internal errors.
    These symbols must be match with the strings declared in errs.c

    If VCL_DECL or __cplusplus are defined the global symbols are declared.

 FILES
    none

 SEE ALSO
    errs.c

 NOTES

 BUGS

********************************************************************unpubMan*/

#ifndef ERRS_H                          /* avoid multiple inclusion */
#define ERRS_H

enum errorcodes                         /* match with errs[] in statics.h */
{
    TRAPERR = 1,                        /* 1 */
    NOTPTRERR,
    ADDRERR,
    NOTARRAYERR,
    STRUCTPTRERR,
    RBRACERR,
    DECLARERR,
    FILERR,
    LBRACERR,
    LEXERR,                             /* 10 */
    LINKERR,
    LVALERR,
    MDEFERR,
    POPERR,
    PUSHERR,
    RBRACKETERR,
    EXPRERR,
    SYNTAXERR,
    DEPTHERR,
    MDIMERR,                            /* 20 */
    PTROPERR,
    FUNCERR,
    SEMIERR,
    STRUCERR,
    ELEMERR,
    INITERR,
    DIV0ERR,
    SIZERR,
    BREAKERR,
    CONTERR,                            /* 30 */
    FPEERR,
    TMDEFERR,
    DEFINERR,
    OMERR,
    SUBSERR,
    LPARENERR,
    BADLBRACERR,
    COLONERR,
    WHILERR,
    BADVARERR,                          /* 40 */
    COMMAERR,
    ENUMERR,
    VOIDRETERR,
    MISMATCHERR,
    NULLRETERR,
    REDEFERR,
    INCOMPATERR,
    NOFUNCERR,
    NOIDENTERR,
    VOIDPTRERR,                         /* 50 */
    NOTNUMERR,
    INCOMPTYPERR,
    UNTERMCOMMENT,
    TYPERR,
    TOOMANYVARERR,
    TOOMANYFUNCERR,
    DATASPACERR,
    NOSETJMPERR,
    CTRLBREAK,
    LINETOOLONGERR,                     /* 60 */
    BADPREPROCERR,
    REDEFPPERR,
    ENDIFERR,
    ELSEERR,
    ELIFERR,
    INCLUDEERR,
    IFSERR,
    UNTERMSTRERR,
    UNTERMCONSTERR,
    IFERR,                              /* 70 */
    ARGERR,
    STDINFILERR,
    STDOUTFILERR,
    NOMAINERR,
    SYMBOLTABLERR,
    CONSTEXPRERR,
    GOTOERR,
    ELSERR,
    SWITCHERR,
    DEFAULTERR,                         /* 80 */
    CASERR,
    TOOMANYDEFAULTSERR,
    FUNCNAMERR,
    TYPEDEFERR,
    DECLERR,
    STRINGIZERR,
    PTRCOMPERR,
    INTTYPERR,
    MATHERR,
    REGADDRERR,                         /* 90 */
    UNRESOLVEDERR,
    UNDEFUNCERR,
    TOOMANYINITERR,
    CONSTARGERR,
    INCLUDENESTERR,
    IFNESTERR,
    ERRORERR,
    ASSERTERR,
    UNSCONSTSUFF,
    LNGCONSTSUFF,                       /* 100 */
    CONSTISUNS,
    CONSTISLNG,
    BADIFDEF,
    NEEDIDENT,
    BADVCLOPT,
    STRTOOLONG,
    UNKNOWNSIZE,
    MULTIPLEDEF,
    MISSINGNAME,
    BADTYPEVOID,                        /* 110 */
    RPARENERR,
    COMMAEXPECTED,
    ELLIPSERR
};

#endif                                  /* avoid multiple inclusion */
