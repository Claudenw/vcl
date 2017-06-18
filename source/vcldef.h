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
    vcldef.h - VAST Command Language data definitions

 DESCRIPTION
    Contains global definitions and symbols used internally by the VAST Command
    Language (VCL).  This header defines the data structures and global runtime
    symbols, and contains the prototypes for cross-module functions.

 FILES
    errs.h                              Internal error definitions
    sys.h                               Internal system function codes
    tokens.h                            Internal pcode token definitions
    publics.h                           Internal public symbols
    statics.h                           Internal static unchanging data

 SEE ALSO

 NOTES

 BUGS

********************************************************************unpubMan*/

#ifndef VCLDEF_H                        /* avoid multiple inclusion */
#define VCLDEF_H

#ifndef __cplusplus
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

#endif                                  /* __cplusplus */

#undef isxdigit                         /* use our own definitions */
#undef isalnum
#undef isdigit
#undef isalpha

#ifdef WRAPVCL                          /* for C++ */
#define VCLCLASS        VclClass::
#define ADDRVCLCLASS    &VCLCLASS
#define THISPTR(m)      this->m
#else                                   /* for C */
#define VCLCLASS
#define ADDRVCLCLASS
#define THISPTR(m)      m
#endif

#include "errs.h"                       /* internal error definitions */

#ifndef SYS_H
#include "sys.h"                        /* internal system function codes */
#endif

#ifndef TOKENS_H
#include "tokens.h"                     /* internal pcode token definitions */
#endif

#define PROGNAME        "VCL"
#define PROGDESC        "VAST Command Language"
#define PROGVERS        "0.88"          /* version as string */
#define PROGVERN        0x0088          /* version as hex MMmm numeric */

/* no integrated debugger yet */
/* #define DEBUGGER        1 */

/*
 * Table size constants
 */
#define MAXALLOC        100             /* maximum runtime malloc()'s */    
#define MAXDIM          4               /* maximum dimensions for arrays */
#define MAXERRMSG       255             /* maximum internal error msg buffer */
#define MAXIFS          25              /* maximum nested #if's */
#define MAXINCLUDES     16              /* maximum nested #include files */
#define MAXLINE         512             /* maximum source code line length */
#define MAXMACROLENGTH  2048            /* maximum length of a macro */
#define MAXNESTS        20              /* maximum statement nesting */
#define MAXOPENFILES    15              /* maximum open FILEs */
#define MAXPARMS        10              /* maximum macro parameters */

/*
 * Runtime constants
 */
#define RVALUE          0               /* a constant */
#define LVALUE          1               /* a variable */
#define OPASSIGN        0x80            /* multi-op assignment, e.g. += */

/* Kinds */
#define FUNCT           1               /* a function */
#define STRUCTELEM      2               /* structure element */
#define LABEL           4               /* goto label */
#define TYPEDEF         8               /* typedef */

/* Storage categories */
#define AUTO            1               /* auto (stack) variable */
#define REGISTER        2               /* register variable */
#define VOLATILE        4               /* volatile variable */
#define EXTERNAL        8               /* external variable */


#define uchar 		unsigned char

enum Types
{
    VOID, CHAR, INT, LONG, FLOAT, STRUCT, UNION, ENUM
};

enum Integrals
{
    I_INT, I_UNSINT, I_LONG, I_UNSLONG, I_FLOAT
};

/*
 * Macro table enter (one for each defined macro)
 */
typedef struct MacroTbl
{
    unsigned char * id;                 /* macro identification */
    unsigned char * val;                /* macro value          */
    int             isMacro;            /* true if () macro     */
    unsigned char   parms;              /* number of parameters */
    struct MacroTbl *NextMacro;
} MACRO;

/*
 * Variable table entry (one for each declared variable)
 */
typedef struct variable
{
    int             vsymbolid;          /* variable identifier (assoc w/name) */
    char            vcat;               /* storage category indirection level */
    char            vkind;              /* kind: func, struct, elem, etc. */
    int             vtype;              /* type: INT, CHAR, etc. */
    int             vsize;              /* size of variable */
    int             vdims[MAXDIM];      /* lengths (if an array) */
    char            vconst;             /* 0 = read/write */
                                        /* 1 = variable is const */
                                        /* 2 = pointer -> const */
                                        /* 3 = both */
    char            vstatic;            /* 1 = static */
    char            vqualifier;         /* 1 = auto 2 = register 4 = volatile
                                         * 8 = external */
    char            islocal;            /* 1 = local variable, 2 = argument */
    char            isunsigned;         /* 1 = unsigned, 0 = signed */
    char            isinitialized;      /* 1 = variable is initialized */
    int             voffset;            /* offset of data from start of buffer */
    int             vwidth;             /* width of data space */
    int             vBlkNesting;        /* block nesting level */
    struct variable *vstruct;           /* for a struct var, -> definition */
    int             fileno;             /* file number where declared */
    int             lineno;             /* line number where declared */
    int             enumval;            /* integer value for an enum constant */
    struct                              /* must be same structure */
    {                                   /* ..as VARIABLELIST below */
        struct variable *vfirst;
        struct variable *vlast;
    } velem;                            /* VARIABLELIST of elements if struct */
    struct variable *vprev;             /* backward link (1st item ->last) */
    struct variable *vnext;             /* forward link */
} VARIABLE;

/*
 * Variable list
 */
typedef struct variablelist
{
    VARIABLE *      vfirst;             /* first in list */
    VARIABLE *      vlast;              /* last in list */
} VARIABLELIST;

/*
 * Symbol table,
 * Preprocessor pcode tokens,
 * VCLPFVOID typedef
 */
#ifndef __cplusplus
#include "publics.h"
#endif

/*
 * Function definition (one for each declared function)
 */
typedef struct function
{
    int             symbol;             /* function symbol id */
    char            ismain;             /* TRUE = main() */
    int             libcode;            /* > 0 = standard library function */
    void *          code;               /* function code */
    int             type;               /* return type, INT, CHAR, etc. */
    char            cat;                /* indirection level of func return */
    uchar           fileno;             /* file containing the function */
    int             lineno;             /* line no of function declaration */
    char            fconst;             /* 0 = read/write */
                                        /* 1 = function is const */
                                        /* 2 = pointer -> const */
                                        /* 3 = both */
    int             width;              /* width of auto variables */
    VARIABLELIST    locals;             /* list of local variables */
    int             BlkNesting;         /* block nesting level */
    char *          proto;              /* function prototype */
    uchar           protofileno;        /* file containing the prototype */
    int             protolineno;        /* line no of function prototype */
} FUNCTION;

/*
 * Running function table entry
 * (one instance for each iteration of recursive function)
 */
typedef struct funcrunning
{
    FUNCTION *      fvar;               /* function variable */
    char *          ldata;              /* local data */
    int             arglength;          /* length of arguments */
    struct funcrunning *fprev;          /* calling function */
    int             BlkNesting;         /* block nesting level */
} FUNCRUNNING;

/*
 * Stack item data value
 */
typedef union datum
{
    char            cval;               /* character values */
    uchar           ucval;              /* unsigned character values */
    int             ival;               /* integer values */
    unsigned int    uival;              /* unsigned integer values */
    long            lval;               /* long values */
    unsigned long   ulval;              /* unsigned long values */
    double          fval;               /* floating point values */
    char *          cptr;               /* pointers to chars */
    uchar *         ucptr;              /* pointers to unsigned chars */
    int *           iptr;               /* pointers to ints */
    unsigned int *  uiptr;              /* pointers to unsigned ints */
    long *          lptr;               /* pointers to longs */
    unsigned long * ulptr;              /* pointers to unsigned longs */
    double *        fptr;               /* pointers to floats */
    FUNCTION *      funcptr;            /* pointers to functions */
    char **         pptr;               /* pointers to pointers */
} DATUM;

/*
 * Stack item with attributes
 */
typedef struct item
{
    char            kind;               /* STRUCTELEM, FUNCT, LABEL, TYPEDEF */
    char            isunsigned;         /* 1 = unsigned, 0 = signed */
    char            cat;                /* pointer or array indirection level */
    char            lvalue;             /* 1 == LVALUE, 0 == RVALUE */
    char            vconst;             /* 0 = read/write, 1,2,3 = const */
    char            vqualifier;         /* storage category, etc. */
    int             size;               /* size of item on stack */
    char            type;               /* type of item on stack */
    int             dims[MAXDIM];       /* array dimensions */
    VARIABLE *      vstruct;            /* for a struct var, -> definition */
    VARIABLELIST *  elem;               /* structure's element variable list */
    DATUM           value;              /* value of item */
} ITEM;

/*
 * Data type promotion attributes
 */
typedef struct promo
{
    char            type;               /* type of item */
    char            isu;                /* item is unsigned flag */
} PROMO;

/*
 * Running program context
 */


/* Start of reconstruction */

#define TRUE ((char)1)
#define FALSE ((char)0)

/* reconstructed structure and global vars*/

int SkipExpression;
int opAssign;


struct _ctxt
{
    int            CurrFileno;
    int            CurrLineno;
    VARIABLELIST   Curstruct;
    ITEM *         Stackptr;
    VARIABLE *     Curvar;
    VARIABLE *     NextVar;
    char *         NextData;
    FUNCRUNNING *  Curfunc;
    FUNCTION *     Curfunction;
    FUNCTION *     NextFunction;
    char           Token;
    DATUM          Value;
    unsigned char * Progptr;
    unsigned char * svpptr;
} Ctx;

typedef struct _vclCfg
{
    int 	   MaxProgram;
    int            MaxStack;	
    int            MaxVariables;
    int            MaxFunctions;
    int            MaxDataSpace;
    int            MaxSymbolTable;
    int            MaxPrototype;
} VclCfg;

typedef struct _srcfile 
{
    char *        fname;
    char *        fullname; 
    char          isSource;
    uchar *       IncludeIp;
    struct _srcfile * NextFile;
} SRCFILE;
/* missing vars
// uchar * Progstart
// ITEM * Stackbtm
// ITME * Stacktop
// VARIABLE * VariableMemory
// char * DataSpace
// FUNCTION * FuncetionMemory
// uchar * NextProto
// int Saw_return

/* missing funcs */

// ItemisAddressOrPointer
// ItemisInteger
// ItemisArray
// StackItemisAddressOrPointer
// Assert
// error
// _clear87()
// _status87()
// rslvaddr()
// rslvsize()
// isArray()
// isPointerArray()


/* Expr headers */


int
VCLCLASS ExpressionOne (void);
int
VCLCLASS expression (void);
void
VCLCLASS cond (void);
void
VCLCLASS assgn (void);
void
VCLCLASS logic1 (void);
void
VCLCLASS logic2 (void);
void
VCLCLASS bool1 (void);
void
VCLCLASS bool2 (void);
void
VCLCLASS bool3 (void);
void
VCLCLASS reln1 (void);
void
VCLCLASS reln2 (void);
void
VCLCLASS shift (void);
void
VCLCLASS add (void);
void
VCLCLASS mult (void);
int
VCLCLASS binarySkip (void (VCLCLASS *fn) (void));
void
VCLCLASS checkInteger (ITEM *item);
void
VCLCLASS catCheck (void);
int
VCLCLASS compareItems (double *fval);
void
VCLCLASS skipExpr (void (VCLCLASS * fn) (void));

/* stack headers */

void
VCLCLASS assignment (void);
void
VCLCLASS pop (void);
void
VCLCLASS popn (int argc);
double
VCLCLASS popflt (void);
int
VCLCLASS popint (void);
int
VCLCLASS popnint (int argc);
long
VCLCLASS poplng (void);
void *
VCLCLASS popptr (void);
void
VCLCLASS psh (void);
void
VCLCLASS push (char pkind, char isunsigned, char pcat, char plvalue,
      unsigned int psize, char ptype, VARIABLELIST *pelem,
      DATUM *pdatum, char pconst);
void
VCLCLASS pushflt (double fvalue, char isu);
void
VCLCLASS pushint (int ivalue, char isu);
void
VCLCLASS pushlng (long lvalue, char isu);
void
VCLCLASS pushptr (void *pvalue, char ptype, char isu);
void
VCLCLASS topdup (void);
void
VCLCLASS topget (ITEM *pitem);
void
VCLCLASS topgetpromo (PROMO *promop);
void
VCLCLASS topset (ITEM *pitem);
int
VCLCLASS readonly (ITEM *sp);
int
VCLCLASS StackItemisNumericType (void);
int
VCLCLASS incompatible (ITEM *d, ITEM *s);
void
VCLCLASS numcheck (void);
char *
VCLCLASS resolveaddr (char **addr, char lval);
void
VCLCLASS torvalue (ITEM *item);

/* scanner headers */

int
VCLCLASS getoken (void);
int
VCLCLASS tokenize (char *tknbuf, char *srcp);
int
VCLCLASS isProto (char *cp);
int
VCLCLASS uncesc (char **bufp);
void
VCLCLASS skip (char left, char right);
void
VCLCLASS fltnum (char **srcstr, char **tknstr);
void
VCLCLASS intnum (char **srcstr, char **tknstr);

/* promote headers */

int
VCLCLASS Promote (char *typp, char *isup, PROMO lo, PROMO ro);

/* primary headers */
int
VCLCLASS ItemArrayDimensions (ITEM *item);
int
VCLCLASS ItemArrayElements (ITEM *item);
int
VCLCLASS ElementWidth (ITEM *item);
int
VCLCLASS ComputeDimension (int dim);
void
VCLCLASS Subscript (int dim);
VCLCLASS VARIABLE *
VCLCLASS primary (void);
VCLCLASS VARIABLE *
VCLCLASS element (void);
void
VCLCLASS postop (void);
void
VCLCLASS prepostop (int ispost, char tok);

/* func headers */
void
VCLCLASS callfunc (void);
void
VCLCLASS TestZeroReturn (void);
void
VCLCLASS ArgumentList (int argc, ITEM *args);

/* linker headers */
void
VCLCLASS link (VARIABLELIST *vartab);
int
VCLCLASS istypespec (void);
int
VCLCLASS isParameterType (void);
int
VCLCLASS isLocalType (void);
void
VCLCLASS AddPro (unsigned char pro);
void
VCLCLASS linkerror (int errnum);
int
VCLCLASS isTypeDeclaration (void);
void
VCLCLASS TypeDeclaration (VARIABLELIST *vartab);
void
VCLCLASS FunctionPrototype (void);
void
VCLCLASS AddStructProto (void);
void
VCLCLASS BuildPrototype (char stopToken, int isproto);
void
VCLCLASS TestPrototype (uchar *pros, int type, int fcat);
void
VCLCLASS FunctionDeclaration (VARIABLELIST *vartab);
int
VCLCLASS isPcodeProto (void);
void
VCLCLASS CheckDeclarations (void);
void
VCLCLASS ConvertIdentifier (void);
void
VCLCLASS InnerDeclarations (int inStruct);
void
VCLCLASS LocalDeclarations (void);
void
VCLCLASS arglist (void);

/* symbol headers */

VCLCLASS FUNCTION *
VCLCLASS FindFunction (int fsymbolid);
void
VCLCLASS InstallFunction (FUNCTION *funcp);
VCLCLASS VARIABLE *
VCLCLASS FindVariable (int symbolid, VARIABLELIST *vartab,
              int BlkNesting, VARIABLE *Stopper, int isStruct);
VCLCLASS VARIABLE *
VCLCLASS SearchVariable (int symbolid, int isStruct);
VCLCLASS VARIABLE *
VCLCLASS InstallVariable (VARIABLE *pvar,        /* the variable to be installed */
                 VARIABLELIST *pvartab, /* linked list of variables */
                 int alloc,             /* allocate & init to zero if TRUE */
                 int isArgument,
                 int BlkNesting,
                 int isStructdef);
int
VCLCLASS ArrayElements (VARIABLE *pvar);
int
VCLCLASS ArrayDimensions (VARIABLE *pvar);
int
VCLCLASS VariableWidth (VARIABLE *pvar);
void
VCLCLASS TypeQualifier (VARIABLE *pvar);
VCLCLASS VARIABLE *
VCLCLASS DefineEnum (VARIABLELIST *pvartab, VARIABLE var,
            int isArgument, int BlkNesting, int StopComma,
            uchar **svp);
VCLCLASS VARIABLE *
VCLCLASS DeclareEnum (VARIABLELIST *pvartab, VARIABLE var,
             int isArgument, int BlkNesting, int StopComma);
VCLCLASS VARIABLE *
VCLCLASS DeclareStructure (VARIABLELIST *pvartab, VARIABLE var,
                int isMember, int isArgument, int BlkNesting, int StopComma);
VCLCLASS VARIABLE *
VCLCLASS DeclareTypedef (VARIABLELIST *pvartab, VARIABLE var,
                int isMember, int isArgument, int BlkNesting, int StopComma);
VCLCLASS VARIABLE *
VCLCLASS DeclareNative (VARIABLELIST *pvartab, VARIABLE var,
               int isMember, int isArgument, int BlkNesting, int StopComma);
VCLCLASS VARIABLE *
VCLCLASS DeclareVariable (VARIABLELIST *pvartab,
                 int isMember, int isArgument, int BlkNesting, int StopComma);
void
VCLCLASS strucdef (char tokn, VARIABLE *psvar);
char
VCLCLASS MakeTypeToken (char tokn, int *sawunsigned);
VCLCLASS VARIABLE *
VCLCLASS varlist (char tokn, char VarisTypedef,
         char vconst, char vstatic, char vqualifier,
         VARIABLELIST *pvartab, VARIABLE *Typedef,
         VARIABLE *psvar,
         int isMember, int isArgument, int BlkNesting,
         int StopComma);
void
VCLCLASS Initializer (VARIABLE *pvar, char *baseaddr, int level);
void
VCLCLASS initexpr (VARIABLE *pvar);
char
VCLCLASS MakeType (char tok);
int
VCLCLASS TypeSize (char type);
void
VCLCLASS SetType (VARIABLE *var, char tok);
VCLCLASS VARIABLE *
VCLCLASS Declarator (char tokn, VARIABLELIST *pvartab, int isArgument,
            VARIABLE *Typedef);
void *
VCLCLASS GetDataSpace (int sz, int init);
void *
VCLCLASS AllocVariable (void);
void *
VCLCLASS DataAddress (VARIABLE *pvar);

/* promote headers */

void
VCLCLASS store (void *toaddr, int tosize, char toisu, void *fraddr, int frsize, char frisu);
void
VCLCLASS cscs (void) ;
void
VCLCLASS csis (void);
void
VCLCLASS csls (void);
void
VCLCLASS csds (void);
void
VCLCLASS cscu (void);
void
VCLCLASS csiu (void);
void
VCLCLASS cslu (void);
void
VCLCLASS cucs (void);
void
VCLCLASS cuis (void);
void
VCLCLASS culs (void);
void
VCLCLASS cuds (void);
void
VCLCLASS cucu (void);
void
VCLCLASS cuiu (void);
void
VCLCLASS culu (void);
void
VCLCLASS iscs (void);
void
VCLCLASS isis (void);
void
VCLCLASS isls (void);
void
VCLCLASS isds (void);
void
VCLCLASS iscu (void);
void
VCLCLASS isiu (void);
void
VCLCLASS islu (void);
void
VCLCLASS iucs (void);
void
VCLCLASS iuis (void);
void
VCLCLASS iuls (void);
void
VCLCLASS iuds (void);
void
VCLCLASS iucu (void);
void
VCLCLASS iuiu (void);
void
VCLCLASS iulu (void);
void
VCLCLASS lscs (void);
void
VCLCLASS lsis (void);
void
VCLCLASS lsls (void);
void
VCLCLASS lsds (void);
void
VCLCLASS lscu (void);
void
VCLCLASS lsiu (void);
void
VCLCLASS lslu (void);
void
VCLCLASS lucs (void);
void
VCLCLASS luis (void);
void
VCLCLASS luls (void);
void
VCLCLASS luds (void);
void
VCLCLASS lucu (void);
void
VCLCLASS luiu (void);
void
VCLCLASS lulu (void);
void
VCLCLASS dscs (void);
void
VCLCLASS dsis (void);
void
VCLCLASS dsls (void);
void
VCLCLASS dsds (void);
void
VCLCLASS dscu (void);
void
VCLCLASS dsiu (void);
void
VCLCLASS dslu (void);
int
VCLCLASS Promote (char *typp, char *isup, PROMO lo, PROMO ro);


/* preproc headers */
void
VCLCLASS PreProcessor (uchar *pSrc, uchar *SourceCode);
void
VCLCLASS FreeBuffers (void);
void
VCLCLASS CleanUpPreProcessor (void);
void
VCLCLASS DeleteFileList (SRCFILE *thisfile);
int
VCLCLASS bypassWhite (unsigned char **cpp);
void
VCLCLASS ExtractWord (uchar *wd, uchar **cp, uchar *allowed);
void
VCLCLASS PreProcess (void);
VCLCLASS MACRO *
VCLCLASS FindMacro (uchar *ident);
int
VCLCLASS parmcmp (char *p, char *t);
void
VCLCLASS AddMacro (uchar *ident, uchar *plist, uchar *value);
void
VCLCLASS DefineMacro (uchar *cp);
void
VCLCLASS UnDefineAllMacros (void);
void
VCLCLASS UnDefineMacro (uchar *cp);
void
VCLCLASS Include (uchar *cp);
int
VCLCLASS TestIfLevel (void);
void
VCLCLASS If (uchar *cp);
void
VCLCLASS IfDef (uchar *cp);
void
VCLCLASS IfnDef (uchar *cp);
void
VCLCLASS Else (void);
void
VCLCLASS Elif (uchar *cp);
void
VCLCLASS Endif (void);
void
VCLCLASS Error (uchar *cp);
void
VCLCLASS Pragma (uchar *cp);
void
VCLCLASS OutputLine (uchar * cp);
void
VCLCLASS WriteChar (uchar c);
void
VCLCLASS WriteEOL (void);
void
VCLCLASS WriteWord (unsigned char *s);
int
VCLCLASS ReadString ();
char *
VCLCLASS SrcFileName (int fileno);

/* VCL.c */

void
VCLCLASS error (int errnum);
void
VCLCLASS warning (int errnum);
void *
VCLCLASS getmem (unsigned size);

#define EXTERN extern

#include "publics.h"

#include "statics.h"



EXTERN SYMBOLTABLE * SymbolTable;
EXTERN int SymbolCount;
EXTERN VclCfg vclCfg;


/* PREPROC */
EXTERN uchar * Op;
EXTERN uchar * Ip;
EXTERN int   IfLevel;
EXTERN uchar * Word;
EXTERN uchar * Line;
EXTERN uchar * FilePath;
EXTERN SRCFILE * FirstFile;
EXTERN SRCFILE * LastFile;
EXTERN int  FileCount;

EXTERN int * Skipping;
EXTERN int * TrueTest;
EXTERN byte *  ElseDone;

#endif

