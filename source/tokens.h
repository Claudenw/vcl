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
    tokens.h - Pseudocode token definitions

 DESCRIPTION
    Contains the definitions for pseudocode tokens used within VCL.

 FILES
    none

 SEE ALSO
    sys.h

 NOTES

 BUGS

********************************************************************unpubMan*/

#ifndef TOKENS_H                        /* avoid multiple inclusion */
#define TOKENS_H

#define T_LNOT          '!'
#define T_LINENO        '#'
#define T_MOD           '%'
#define T_AND           '&'
#define T_LPAREN        '('
#define T_RPAREN        ')'
#define T_MUL           '*'
#define T_PTR           '*'
#define T_ADD           '+'
#define T_COMMA         ','
#define T_SUB           '-'
#define T_DOT           '.'
#define T_DIV           '/'
#define T_CHAR          '1'
#define T_INT           '2'
#define T_STATIC        '3'
#define T_LONG          '4'
#define T_AUTO          '5'
#define T_EXTERNAL      '6'
#define T_REGISTER      '7'
#define T_FLOAT         '8'
#define T_DOUBLE        '9'             /* alias for "float" */
#define T_COLON         ':'
#define T_SEMICOLON     ';'
#define T_LT            '<'
#define T_ASSIGN        '='
#define T_GT            '>'
#define T_COND          '?'
#define T_COMMENT       '@'
#define T_FUNCTION      'A'
#define T_LAND          'a'
#define T_VOLATILE      'B'
#define T_BREAK         'b'
#define T_CHRCONST      'C'
#define T_CASE          'c'
#define T_DEFAULT       'd'
#define T_IDENTIFIER    'D'
#define T_ENUM          'E'
#define T_ELSE          'e'
#define T_FLTCONST      'F'
#define T_FOR           'f'
#define T_GOTO          'G'
#define T_GE            'g'
#define T_FUNCTREF      'H'
#define T_SWITCH        'h'
#define T_INTCONST      'I'
#define T_IF            'i'
#define T_LNGCONST      'J'
#define T_UNSIGNED      'j'             /* unsigned type qualifier */
#define T_UINTCONST     'k'
#define T_ULNGCONST     'K'
#define T_SHL           'L'
#define T_LE            'l'
#define T_DECR          'm'
#define T_NE            'n'
#define T_CONST         'N'
#define T_OCTCONST      'O'
#define T_LIOR          'o'
#define T_INCR          'p'
#define T_ENDCOMMENT    'Q'
#define T_EQ            'q'
#define T_SHR           'R'
#define T_RETURN        'r'
#define T_STRCONST      'S'
#define T_ELLIPSE       's'
#define T_SHORT         'T'
#define T_TYPEDEF       't'
#define T_PLUSCOMMENT   'U'
#define T_UNION         'u'
#define T_VOID          'V'
#define T_STRUCT        'v'
#define T_WHILE         'w'
#define T_DO            'W'
#define T_HEXCONST      'X'
#define T_CONTINUE      'x'
#define T_SYMBOL        'Y'
#define T_ENTRY         'y'
#define T_EOF           'Z'
#define T_SIZEOF        'z'
#define T_LBRACKET      '['
#define T_EOL           '\\'
#define T_RBRACKET      ']'
#define T_XOR           '^'
#define T_ARROW         '_'
#define T_LBRACE        '{'
#define T_IOR           '|'
#define T_RBRACE        '}'
#define T_NOT           '~'

#endif                                  /* avoid multiple inclusion */
