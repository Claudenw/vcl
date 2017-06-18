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

/*unpubModule*****************************************************************
 NAME
    preproc.c - Preprocessor

 DESCRIPTION
    Preprocesses source code, handles conditional compilation, and
    expands macro definitions.  Uses preexpr.c for preprocessor expressions.

 FUNCTIONS
    PreProcessor()
    CleanUpPreProcessor()
    DeleteFileList()
    bypassWhite()
    ExtractWord()
    FindMacro()
    DefineMacro()
    SrcFileName()

    FreeBuffers()
    PreProcess()
    parmcmp()
    AddMacro()
    UnDefineAllMacros()
    UnDefineMacro()
    Include()
    TestIfLevel()
    If()
    IfDef()
    IfnDef()
    Else()
    Elif()
    Endif()
    Error()
    Pragma()
    OutputLine()
    WriteChar()
    WriteEOL()
    WriteWord()
    ReadString()

 FILES
    vcldef.h

 SEE ALSO

 NOTES

 BUGS

*****************************************************************unpubModule*/
#ifndef PREPROC_H                       /* avoid multiple inclusion */
#define PREPROC_H

