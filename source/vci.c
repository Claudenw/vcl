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

/*pubMain*********************************************************************
 NAME
    vci.c - VAST Command Language Compiler and Interpreter main program

 SYNOPSIS
    vci [program[.VCC]]

 DESCRIPTION
    This is the main program for a single-threaded VCL implementation.
    The compiler and interpreter are combined in this implementation.
    The .VCC source file specified on the command line is compiled and\
    executed.

 OPTIONS

 ENVIRONMENT SYMBOLS

 RETURN VALUE
    Returns any value returned from vclRuntime().

 FILES
    vcl.h

 SEE ALSO

 NOTES

 EXAMPLES

 BUGS

*********************************************************************pubMain*/

#include "vcl.h"                        /* interface header for VCL */

extern unsigned int _stklen = 16384U;   /* this program's stack size */

int
main (int argc, char **argv)
{
    int         i;

    /* passing a null source filename is ok, VCL reports the error */
    i = vclRuntime ( argc, argv );

    return i;
} /* main */
