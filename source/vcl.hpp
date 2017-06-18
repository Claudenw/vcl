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
    Requires "IDE define WRAPVCL=1" or "-DWRAPVCL=1" in makefile
*/

#ifndef VCL_HPP                         /* avoid multiple inclusion */
#define VCL_HPP

extern "C" {
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <setjmp.h>

#ifndef SCDEF_H
#include <scdef.h>
#endif

int VclInstance (int, char **);         /* instance a VCL runtime object */
}

#ifndef WRAPVCL
#define WRAPVCL     1
#endif

#ifndef SYS_H
#include "sys.h"                        /* internal system function codes */
#endif

class VclClass
{

public:

#include "publics.h"                    /* public data definitions */

    VclClass (void)
    {
        /* empty constructor */
    };

    ~VclClass (void)
    {
        /* empty destructor */
    };

#include "vcl.h"                        /* public function prototypes */

private:

#include "vcldef.h"                     /* private data definitions */

}; /* class Vcl */

/*
 * Include static (non-changing) data defintions.
 *
 * Included here after other definitions and inclusions above.
 */
#include "statics.h"

#endif                                  /* avoid multiple inclusion */

