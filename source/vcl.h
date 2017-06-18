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

/*pubMan**********************************************************************
 NAME
    vcl.h - VAST Command Language header

 DESCRIPTION
    Header used for interfacing to the VAST Command Language (VCL)
    environment for 'straight' C programs.

 FILES
    vcldef.h                            Internal VCL header

 SEE ALSO

 NOTES

 BUGS

**********************************************************************pubMan*/

#ifndef VCL_H                           /* avoid multiple inclusion */
#define VCL_H

int         vclRuntime (int, char **);
void        vclShutdown (void);

#ifndef VCLDEF_H
#ifndef __cplusplus
#ifndef WRAPVCL
#include "vcldef.h"
#endif
#endif
#endif

#endif                                  /* avoid multiple inclusion */
