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
    Contains global data declarations and initializations for items
    with static (constant) data that is not changed during runtime.

    Other global non-static runtime data declarations are made in
    vcldef.h, some of which are initialized in VclGlobalInit().
*/

#ifndef STATICS_H                       /* avoid multiple inclusion */
#define STATICS_H

#ifndef VCLDEF_H
#include "vcldef.h"
#endif

#ifndef SYS_H
#include "sys.h"                        /* internal system function codes */
#endif

#ifndef TOKENS_H
#include "tokens.h"                     /* internal pcode token definitions */
#endif

typedef void (VCLCLASS * VCLPFVOID)(void);

/*===========================================================================*/
/*            S T A T I C   G L O B A L   D E C L A R A T I O N S            */
/*===========================================================================*/

#if VCL_DECL

/* linker globals */
char            typespecs[] =           /* array of type specifications */
{
    T_CHAR, T_INT, T_LONG, T_FLOAT, T_DOUBLE,
    T_STRUCT, T_UNION, T_ENUM, T_UNSIGNED, T_SHORT, 0
};

/* function handling globals */
int             vFuncs[] =              /* array of printf/scanf functions */
{
    SYSSSCANF, SYSSPRINTF, SYSFSCANF, SYSFPRINTF,
    SYSSCANF, SYSPRINTF, SYSCPRINTF, 0
};

/* system call globals */
FILE *          handles[] =             /* standard file handles, in order */
{
    stdin, stdout, stderr, stdaux, stdprn
};
double          (*mfunc[]) (double) =   /* array of double math functions */
{
    acos, asin, atan, ceil, cos, cosh, exp, fabs,
    floor, sin, sinh, sqrt, tan, tanh, log, log10
};

/* keyword handling */
VCLCLASS SYMBOLTABLE     LibraryFunctions[] =
{
    /* NOTE: These must be maintained in collating sequence */
    {"_Errno", SYSERRNO},
    {"_filename", SYSFILENAME},
    {"_lineno", SYSLINENO},
    {"abs", SYSABS},
    {"acos", SYSACOS},
    {"asctime", SYSASCTIME},
    {"asin", SYSASIN},
    {"atan", SYSATAN},
    {"atan2", SYSATAN},
    {"atof", SYSATOF},
    {"atoi", SYSATOI},
    {"atol", SYSATOL},
    {"ceil", SYSCEIL},
    {"clrscr", SYSCLRSCRN},
    {"cos", SYSCOS},
    {"cosh", SYSCOSH},
    {"cprintf", SYSCPRINTF},
    {"cursor", SYSCURSOR},
    {"exit", SYSEXIT},
    {"exp", SYSEXP},
    {"fabs", SYSFABS},
    {"fclose", SYSFCLOSE},
    {"fflush", SYSFFLUSH},
    {"fgetc", SYSFGETC},
    {"fgets", SYSFGETS},
    {"findfirst", SYSFINDFIRST},
    {"findnext", SYSFINDNEXT},
    {"floor", SYSFLOOR},
    {"fopen", SYSFOPEN},
    {"fprintf", SYSFPRINTF},
    {"fputc", SYSFPUTC},
    {"fputs", SYSFPUTS},
    {"fread", SYSFREAD},
    {"free", SYSFREE},
    {"fscanf", SYSFSCANF},
    {"fseek", SYSFSEEK},
    {"ftell", SYSFTELL},
    {"fwrite", SYSFWRITE},
    {"getch", SYSGETCH},
    {"getchar", SYSGETCHAR},
    {"gets", SYSGETS},
    {"gmtime", SYSGMTIME},
    {"localtime", SYSLOCALTIME},
    {"log", SYSLOG},
    {"log10", SYSLOG10},
    {"longjmp", SYSLONGJMP},
    {"malloc", SYSMALLOC},
    {"mktime", SYSMKTIME},
    {"pow", SYSPOW},
    {"printf", SYSPRINTF},
    {"putch", SYSPUTCH},
    {"putchar", SYSPUTCHAR},
    {"puts", SYSPUTS},
    {"remove", SYSREMOVE},
    {"rename", SYSRENAME},
    {"rewind", SYSREWIND},
    {"scanf", SYSSCANF},
    {"setjmp", SYSSETJMP},
    {"sin", SYSSIN},
    {"sinh", SYSSINH},
    {"sprintf", SYSSPRINTF},
    {"sqrt", SYSSQRT},
    {"sscanf", SYSSSCANF},
    {"strcat", SYSSTRCAT},
    {"strcmp", SYSSTRCMP},
    {"strcpy", SYSSTRCPY},
    {"strlen", SYSSTRLEN},
    {"strncat", SYSSTRNCAT},
    {"strncmp", SYSSTRNCMP},
    {"strncpy", SYSSTRNCPY},
    {"system", SYSSYSTEM},
    {"tan", SYSTAN},
    {"tanh", SYSTANH},
    {"time", SYSTIME},
    {"tmpfile", SYSTMPFILE},
    {"tmpnam", SYSTMPNAM},
    {"ungetc", SYSUNGETC}
};

#define LIBFUNCTIONS_Q (sizeof(LibraryFunctions)/sizeof(VCLCLASS SYMBOLTABLE))
int             MAXLIBFUNCTIONS = LIBFUNCTIONS_Q;

/* keyword lookup table */
VCLCLASS SYMBOLTABLE Keywords[] =
{
    /* NOTE: These must be maintained in collating sequence */
    {"auto", T_AUTO},
    {"break", T_BREAK},
    {"case", T_CASE},
    {"char", T_CHAR},
    {"const", T_CONST},
    {"continue", T_CONTINUE},
    {"default", T_DEFAULT},
    {"do", T_DO},
    {"double", T_DOUBLE},
    {"else", T_ELSE},
    {"enum", T_ENUM},
    {"extern", T_EXTERNAL},
    {"float", T_FLOAT},
    {"for", T_FOR},
    {"goto", T_GOTO},
    {"if", T_IF},
    {"int", T_INT},
    {"long", T_LONG},
    {"register", T_REGISTER},
    {"return", T_RETURN},
    {"short", T_SHORT},
    {"sizeof", T_SIZEOF},
    {"static", T_STATIC},
    {"struct", T_STRUCT},
    {"switch", T_SWITCH},
    {"typedef", T_TYPEDEF},
    {"union", T_UNION},
    {"unsigned", T_UNSIGNED},
    {"void", T_VOID},
    {"volatile", T_VOLATILE},
    {"while", T_WHILE}
};

#define KEYWORDS_Q (sizeof(Keywords)/sizeof(VCLCLASS SYMBOLTABLE))
int             MAXKEYWORDS = KEYWORDS_Q;

/* multi-character operator lookup tbl */
VCLCLASS SYMBOLTABLE Operators[] =
{
    /* NOTE: These must be maintained in collating sequence */
    {"!=", T_NE},
    {"&&", T_LAND},
    {"++", T_INCR},
    {"--", T_DECR},
    {"->", T_ARROW},
    {"<<", T_SHL},
    {"<=", T_LE},
    {"==", T_EQ},
    {">=", T_GE},
    {">>", T_SHR},
    {"||", T_LIOR}
};

#define OPERATORS_Q (sizeof(Operators)/sizeof(VCLCLASS SYMBOLTABLE))
int             MAXOPERATORS = OPERATORS_Q;

VCLCLASS SYMBOLTABLE PreProcessors[] =
{
    /* NOTE: These must be maintained in collating sequence */
    {"define", VCLCLASS P_DEFINE},
    {"elif", VCLCLASS P_ELIF},
    {"else", VCLCLASS P_ELSE},
    {"endif", VCLCLASS P_ENDIF},
    {"error", VCLCLASS P_ERROR},
    {"if", VCLCLASS P_IF},
    {"ifdef", VCLCLASS P_IFDEF},
    {"ifndef", VCLCLASS P_IFNDEF},
    {"include", VCLCLASS P_INCLUDE},
    {"pragma", VCLCLASS P_PRAGMA},
    {"undef", VCLCLASS P_UNDEF}
};

#define PREPROCESSORS_Q (sizeof(PreProcessors)/sizeof(VCLCLASS SYMBOLTABLE))
int             MAXPREPROCESSORS = PREPROCESSORS_Q;

VCLCLASS SYMBOLTABLE PreDefined[] =
{
    /* NOTE: These must be maintained in collating sequence */
    {"CDECL", VCLCLASS PD_CDECL},       /* TRUE, calling convention is c */
    {"VCL", VCLCLASS PD_VCL},           /* MMmm, MMajor and mminor VCL version */
    {"__DATE__", VCLCLASS PD_DATE},     /* string constant date */
    {"__FILE__", VCLCLASS PD_FILE},     /* string constant current filename */
    {"__LINE__", VCLCLASS PD_LINE},     /* unsigned constant, current file line */
    {"__MSDOS__", VCLCLASS PD_OS},      /* TRUE if operating system is MSDOS */
    {"__TIME__", VCLCLASS PD_TIME}      /* string constant time */
};

#define PREDEFINED_Q (sizeof(PreDefined)/sizeof(VCLCLASS SYMBOLTABLE))
int             MAXPREDEFINED = PREDEFINED_Q;

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
VCLPFVOID   storeFunc[] =
{
    /*
     * TO: character
     */
    /* s - s */
    VCLCLASS cscs,                      /* 0 */
    VCLCLASS csis,
    VCLCLASS csls,
    VCLCLASS csds,
    /* s - u */
    VCLCLASS cscu,                      /* 4 */
    VCLCLASS csiu,
    VCLCLASS cslu,
    VCLCLASS csds,                      /* floating point is always signed */
    /* u - s */
    VCLCLASS cucs,                      /* 8 */
    VCLCLASS cuis,
    VCLCLASS culs,
    VCLCLASS cuds,
    /* u - u */
    VCLCLASS cucu,                      /* 12 */
    VCLCLASS cuiu,
    VCLCLASS culu,
    VCLCLASS cuds,                      /* floating point is always signed */
    /*
     * TO: integer
     */
    /* s - s */
    VCLCLASS iscs,                      /* 16 */
    VCLCLASS isis,
    VCLCLASS isls,
    VCLCLASS isds,
    /* s - u */
    VCLCLASS iscu,                      /* 20 */
    VCLCLASS isiu,
    VCLCLASS islu,
    VCLCLASS isds,                      /* floating point is always signed */
    /* u - s */
    VCLCLASS iucs,                      /* 24 */
    VCLCLASS iuis,
    VCLCLASS iuls,
    VCLCLASS iuds,
    /* u - u */
    VCLCLASS iucu,                      /* 28 */
    VCLCLASS iuiu,
    VCLCLASS iulu,
    VCLCLASS iuds,                      /* floating point is always signed */
    /*
     * TO: long
     */
    /* s - s */
    VCLCLASS lscs,                      /* 32 */
    VCLCLASS lsis,
    VCLCLASS lsls,
    VCLCLASS lsds,
    /* s - u */
    VCLCLASS lscu,                      /* 36 */
    VCLCLASS lsiu,
    VCLCLASS lslu,
    VCLCLASS lsds,                      /* floating point is always signed */
    /* u - s */
    VCLCLASS lucs,                      /* 40 */
    VCLCLASS luis,
    VCLCLASS luls,
    VCLCLASS luds,
    /* u - u */
    VCLCLASS lucu,                      /* 44 */
    VCLCLASS luiu,
    VCLCLASS lulu,
    VCLCLASS luds,                      /* floating point is always signed */
    /*
     * TO: double
     */
    /* s - s */
    VCLCLASS dscs,                      /* 48*/
    VCLCLASS dsis,
    VCLCLASS dsls,
    VCLCLASS dsds,
    /* s - u */
    VCLCLASS dscu,                      /* 52 */
    VCLCLASS dsiu,
    VCLCLASS dslu,
    VCLCLASS dsds,                      /* floating point is always signed */
    /* u - s floating point is always signed */
    VCLCLASS dscs,                      /* 56 */
    VCLCLASS dsis,
    VCLCLASS dsls,
    VCLCLASS dsds,
    /* u - u floating point is always signed */
    VCLCLASS dscu,                      /* 60 */
    VCLCLASS dsiu,
    VCLCLASS dslu,
    VCLCLASS dsds                       /* 63, end of array */
};

/* error handling */
char *          errs[] =                /* match with errorcodes in errs.h */
{
    "Debug trap",                       /* 1 */
    "Pointer required",
    "Address required",
    "Not a pointer or array",
    "Not a struct pointer",
    "'}' expected",
    "Unknown identifier",
    "File error",
    "'{' expected",
    "Lexical error",                    /* 10 */
    "Misplaced statement",
    "lvalue expected",
    "Variable redeclaration",
    "Stack underflow",
    "Stack space exhausted",
    "']' expected",
    "Expression error",
    "Syntax error",
    "Expression nesting too deep",
    "Too many dimensions",              /* 20 */
    "Illegal pointer reference",
    "Invalid function call",
    "';' expected",
    "Not a struct or union",
    "Not a struct or union member",
    "Initialization error",
    "Divide by 0",
    "Size error",
    "Misplaced break",
    "Misplaced continue",               /* 30 */
    "Floating point error",
    "#define nesting too deep",
    "#define error",
    "Out of memory",
    "Negative array dimension",
    "'(' expected",
    "'(' or '[' unexpected",
    "':' expected",
    "while expected",
    "Misplaced variable declaration",   /* 40 */
    "Misplaced comma",
    "enum error",
    "void function may not return a value",
    "Argument mismatch",
    "Function should return a value",
    "Function redefinition",
    "Incompatible pointers",
    "Unknown function",
    "Identifier expected",
    "Improper void* usage",             /* 50 */
    "Not a numeric type",
    "Incompatible types",
    "Unterminated comment",
    "Type expected",
    "Out of variable space",
    "Out of function space",
    "Out of data space",
    "longjmp missing setjmp",
    "Program interrupted",
    "Line too long",                    /* 60 */
    "Invalid preprocessor directive",
    "Macro redefinition",
    "Misplaced #endif",
    "Misplaced #else",
    "Misplaced #elif",
    "#include file not found",
    "#if, #ifdef or #ifndef missing #endif",
    "Unterminated string literal",
    "Unterminated character constant",
    "#if error",                        /* 70 */
    "Mismatched macro arguments",
    "stdin file error",
    "stdout file error",
    "No main() function",
    "Symbol table full",
    "Constant expression expected",
    "Unknown label",
    "else missing if",
    "switch error",
    "Misplaced default",                /* 80 */
    "Misplaced case",
    "Too many defaults",
    "Function name expected",
    "Invalid typedef",
    "Declaration error",
    "Misplaced # in #define",
    "Invalid pointer conversion",
    "Integral type expected",
    "Arithmetic exception",
    "Illegal &register variable reference", /* 90 */
    "Unresolved extern",
    "Undefined function",
    "Too many initializers",
    "Const argument to non-const pointer",
    "#include nesting too deep",
    "#if nesting too deep",
    "#error directive",
    "-----------------------------------------------------------------------",
    "Duplicate unsigned constant suffix 'U'",
    "Duplicate long constant suffix 'L'",   /* 100 */
    "Constant is unsigned",
    "Constant is long",
    "Bad ifdef directive syntax",
    "define directive needs an identifier",
    "Unknown runtime option",
    "String constant too long",
    "Size of type is unknown or zero",
    "Multiple definition",
    "type missing name",
    "type void not allowed",            /* 110 */
    "')' expected",
    "',' expected",
    "ellipse error"
};

/*===========================================================================*/
/*            S T A T I C   G L O B A L   R E F E R E N C E S                */
/*===========================================================================*/
#else                                   /* (not) VCL_DECL */

/* linker globals */
extern char     typespecs[];            /* array of type specifications */

/* function handling globals */
extern int      vFuncs[];               /* array of printf/scanf functions */

/* system call globals */
extern FILE *   handles[];              /* standard file handles, in order */
extern double   (*mfunc[]) (double);    /* array of double math functions */

/* keyword handling */
extern VCLCLASS SYMBOLTABLE LibraryFunctions[];/* internally bound library functions */
extern VCLCLASS SYMBOLTABLE Keywords[];        /* c keywords */
extern VCLCLASS SYMBOLTABLE Operators[];       /* multi-character c operators */
extern VCLCLASS SYMBOLTABLE PreProcessors[];   /* preprocessor keywords */
extern VCLCLASS SYMBOLTABLE PreDefined[];      /* predefined preprocessor symbols */
extern int MAXLIBFUNCTIONS;
extern int MAXKEYWORDS;
extern int MAXOPERATORS;
extern int MAXPREPROCESSORS;
extern int MAXPREDEFINED;

/* Assignment / type-promotion function pointer array */
extern VCLPFVOID storeFunc[];

/* error handling */
extern char *   errs[];                 /* external reference only */

#endif                                  /* VCL_DECL */

#endif                                  /* avoid multiple inclusion */

