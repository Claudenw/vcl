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
 * VCL internal declarations are performed by this module.
 */
#ifndef VCL_DECL
#define VCL_DECL        1
#endif

#ifdef _cplusplus
extern "C" {
#endif
#include <mem.h>
extern int errno;
#ifdef _cplusplus
}
#endif

#ifdef WRAPVCL
#include "vcl.hpp"
#else
#include "vcldef.h"
#endif

void
VCLCLASS VclGlobalInit (void)
{
    /* configuration data */
    rtopt.CompileOnly = FALSE;
    rtopt.NoLineNumbers = FALSE;
    rtopt.PrintPreprocess = FALSE;
    rtopt.QuietMode = FALSE;

    /* source file tracking */
    BaseFile = NULL;                    /* current source file */
    FirstFile = NULL;                   /* head of list */
    LastFile = NULL;                    /* last file added */
    ThisFile = NULL;                    /* current file */
    FileCount = 0;                      /* for Ctx.CurrFileno */

    /* context and pcode */
    memset( &Ctx, 0, sizeof( Ctx ) );   /* master context */
    Progstart = NULL;                   /* start of pcode space */
    Progused = 0;                       /* bytes of pcode space used */

    /* variables */
    VariableMemory = NULL;              /* variable space */
    VariablesUsed = 0;
    Globals.vfirst = NULL;
    Globals.vlast = NULL;
    Blkvar = NULL;                      /* local block auto variables */

    /* data space */
    DataSpace = NULL;                   /* data space */
    MaxDataSpace = NULL;                /* maximum data space used */

    /* functions */
    FunctionMemory = NULL;              /* function space */
    FunctionsCount = 0;                 /* functions count */
    NextFunction = NULL;                /* next available function in table */

    /* function prototypes */
    PrototypeMemory = NULL;             /* function prototype space */
    NextProto = NULL;                   /* addr of next prototype */

    /* symbol table */
    SymbolTable = NULL;                 /* symbol table */
    SymbolCount = 0;                    /* count of symbols in table */

    /* stack */
    Stackbtm = NULL;                    /* start of program stack */
    Stackmax = NULL;                    /* maximum program stack used */
    Stacktop = NULL;                    /* end of program stack */

    /* preprocessor globals */
    definedTest = 0;                    /* -1='! defined', 0=none, 1='defined' */
    FirstMacro = NULL;                  /* head of macro list */
    memset( ElseDone, 0, sizeof( ElseDone ) );
    memset( Skipping, 0, sizeof( Skipping ) );
    memset( TrueTest, 0, sizeof( TrueTest ) );
    IfLevel = 0;                        /* current #if level */
    FilePath = NULL;                    /* include file path buffer */
    Line = NULL;                        /* source line buffer */
    Ip = NULL;                          /* input source pointer */
    Op = NULL;                          /* output source pointer */
    MacroCount = 0;                     /* preprocessor macro count */
    Nesting = 0;                        /* #include nesting level */
    Word = NULL;                        /* preprocessor 'word' */

    /* tokenizer globals */
    isStruct = FALSE;                   /* last getoken was a struct */

    /* linker globals */
    Linking = FALSE;                    /* in linker */
    errptr = NULL;                      /* pcode pointer on error */
    fconst = 0;                         /* function is a const */
    protoreturn = 0;                    /* function return type */
    protocat = 0;                       /* function return indirection level */

    /* runtime globals */
    ConstExpression = 0;
    elementpvar = NULL;                 /* VARIABLE * for element() */
    GotoOffset = 0;                     /* offset of a goto */
    GotoNesting = 0;                    /* goto nesting level */
    memset( gotojmp, 0, sizeof( gotojmp ) );
    opAssign = 0;                       /* multi-char assignment operation */
    Saw_return = 0;                     /* "return" found in pcode */
    Saw_break = 0;                      /* "break" found in pcode */
    Saw_continue = 0;                   /* "continue" found in pcode */
    SkipExpression = 0;                 /* skipping the effect of expression */
    memset( &Shelljmp, 0, sizeof( Shelljmp ) );
    memset( &stmtjmp, 0, sizeof( stmtjmp ) );

    /* function handling globals */
    memset( &BreakJmp, 0, sizeof( BreakJmp ) );
    inSystem = 0;                       /* 'in system' indicator */
    jmp_val = 0;                        /* pcode longjump() handling */
    longjumping = 0;                    /* pcode longjump() in process */

    /* system call globals */
    memctr = 0;                         /* memory allocation counter */
    OpenFileCount = 0;                  /* open file count */
    WasConsole = 0;                     /* console i/o function indicator */
    WasFileFunction = 0;                /* file function indicator */

    /* error handling */
    ErrorCode = 0;                      /* internal error code */
    errno = 0;

#ifdef DEBUGGER
    Running = FALSE;
    wwnd = NULL;
#endif
} /* VclGlobalInit */
