VCL Programming Notes                   +---------------------+
---------------------                   |  > to be done       |
                                        |  - not implemented  |
                                        |  . done             |
                                        +---------------------+

> Add member in function and variable structures for "referenced"
  so a 3rd phase can detect unused variables and functions.
  Also add an "assigned" flag for variables to detect "assigned
  but not used".

> Test multi-argument macros.

> Fix macros to handle character literals ('x').

> Check character value handling; are masks still needed?

> Use FindSymbolName() for symbol/identifier errors.

> Test K&R declarations.

> !! Casts have a problem.  The construct: double d = (double) i;
  causes the value of i to be set to 0 !!

> Backward goto's do not work.  Either fix existing longjmp()
  approach, or replace with old VCL's stack-unwinding logic.

> Test prototypes more.

> Finish prototype handling for signed/unsigned variables.  Add
  logic in BuildPrototype() for placing an unsigned indicator in the
  prototype char array; add logic in func.c, callfunc() to check the
  variable's runtime structure.

> Test preproc.vcc with the new macro parsing.

> Add #line handling; set Ctx.CurrLineno or eat and ignore?

> Do mismatched function arguments cause runtime stack problems?
  Can a function be called with mismatched or miscounted arguments?

> Add multi-file compiling capability.

> Test extern handling.

-eof-
