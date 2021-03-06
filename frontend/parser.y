%code requires
{
  #include "../common/ast.hpp"
  namespace stream { namespace parsing { class driver; } }
}

%skeleton "lalr1.cc"

%defines

%locations

%define api.namespace {stream::parsing}
%define api.value.type {stream::ast::semantic_value_type}
%parse-param { class stream::parsing::driver& driver }

%define parse.error verbose

%define lr.type ielr

%token END 0  "end of file"
%token INVALID "invalid token"
%token INT REAL COMPLEX TRUE FALSE STRING
%token ID QUALIFIED_ID
%token IF THEN CASE
%token MODULE IMPORT AS INPUT OUTPUT EXTERNAL
%token OTHERWISE

%left '='
%precedence ',' ':'
%right RIGHT_ARROW
%right LET IN
%right WHERE
%right ELSE
%left LOGIC_OR
%left LOGIC_AND
%left BIT_OR
%left BIT_XOR
%left BIT_AND
%left EQ NEQ LESS MORE LESS_EQ MORE_EQ
%left BIT_SHIFT_LEFT BIT_SHIFT_RIGHT
%left PLUSPLUS
%left '+' '-'
%left '*' '/' INT_DIV '%'
%left '^'
%left DOTDOT
%right LOGIC_NOT BIT_NOT
%right UMINUS '#'
%right '.'
// FIXME: review precedence and association
%left '[' '{' '('
%right '@'

%start program

%code
{
#include "driver.hpp"
#include "scanner.hpp"

#undef yylex
#define yylex driver.scanner.lex

using namespace stream::ast;
using op_type = stream::primitive_op;
}

%%


program:
  module_decl imports declarations
  {
    $$ = make_list(program, @$, { $1, $2, $3 });
    driver.m_ast = $$;
  }
;

module_decl:
  // empty
  { $$ = nullptr; }
  |
  MODULE id ';'
  { $$ = $2; }
;

imports:
  // empty
  { $$ = nullptr; }
  |
  import_list ';'
;

import_list:
  import
  {
    $$ = make_list( @$, { $1 } );
  }
  |
  import_list ';' import
  {
    $$ = $1;
    $$->as_list()->append( $3 );
    $$->location = @$;
  }
;

import:
  IMPORT id
  {
  $$ = make_list( @$, { $2, nullptr } );
  }
  |
  IMPORT id AS id
  {
  $$ = make_list( @$, { $2, $4 } );
  }
;

declarations:
  // empty
  { $$ = nullptr; }
  |
  declaration_list optional_semicolon
;

declaration_list:
  declaration
  {
    $$ = make_list( @$, { $1 } );
  }
  |
  declaration_list ';' declaration
  {
    $$ = $1;
    $$->as_list()->append( $3 );
    $$->location = @$;
  }
;

declaration:
  external_decl | nested_decl
;

nested_decl:
  binding | id_type_decl
;

nested_decl_list:
  nested_decl
  {
    $$ = make_list( @$, { $1 } );
  }
  |
  nested_decl_list ';' nested_decl
  {
    $$ = $1;
    $$->as_list()->append( $3 );
    $$->location = @$;
  }
;

external_decl:
  INPUT id ':' type
  { $$ = make_list(ast::input, @$, {$2, $4}); }
  |
  EXTERNAL id ':' type
  { $$ = make_list(ast::external, @$, {$2, $4}); }
  |
  OUTPUT id
  { $$ = make_list(ast::output, @$, {$2, nullptr}); }
  |
  OUTPUT id '=' expr
  // Use format of binding
  { $$ = make_list(ast::output_value, @$, {$2, nullptr, $4}); }
  |
  OUTPUT id_type_decl
  { $$ = $2; $$->type = ast::output_type; }
;


binding:
  id '=' expr
  {
    $$ = make_list( ast::binding, @$, {$1, nullptr, $3} );
  }
  |
  id '(' param_list ')' '=' expr
  {
    $$ = make_list( ast::binding, @$, {$1, $3, $6} );
  }
  |
  // We use the same structure as array_pattern
  id '[' expr_list ']' '=' array_exprs
  {
    auto pattern = make_list(@$, { $3, $6 });
    $$ = make_list( ast::array_element_def, @$, { $1, pattern });
  }
;

param_list:
  // empty
  { $$ = make_list( @$, {} ); }
  |
  id
  { $$ = make_list( @$, {$1} ); }
  |
  param_list ',' id
  {
    $$ = $1;
    $$->as_list()->append( $3 );
    $$->location = @$;
  }
;

id_type_decl:
    id ':' type
    { $$ = make_list(ast::id_type_decl, @$, {$1, $3}); }
;

type:
  data_type | function_type
;

function_type:
  data_type_list RIGHT_ARROW data_type
  { $$ = make_list(ast::function_type, @$, {$1, $3}); }
;

data_type_list:
  data_type
  { $$ = make_list( @$, {$1} ); }
  |
  data_type_list ',' data_type
  {
    $$ = $1;
    $$->as_list()->append( $3 );
    $$->location = @$;
  }
;

data_type:
  array_type | primitive_type
;

array_type:
  '[' expr_list ']' primitive_type
  { $$ = make_list(ast::array_type, @$, {$2, $4}); }
;

primitive_type:
  id
;


expr:
  id
  |
  qualified_id
  |
  number
  |
  inf
  |
  boolean
  |
  if_expr
  |
  func_lambda
  |
  func_apply
  |
  func_composition
  |
  array_lambda
  |
  array_enum
  |
  array_apply
  |
  array_size
  |
  expr PLUSPLUS expr
  { $$ = make_list( array_concat, @$, {$1, $3} ); }
  |
  LOGIC_NOT expr
  { $$ = make_list( primitive, @$, {make_const(@1,op_type::negate), $2} ); }
  |
  BIT_NOT expr
  { $$ = make_list( primitive, @$, {make_const(@1,op_type::bitwise_not), $2} ); }
  |
  expr LOGIC_OR expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::logic_or), $1, $3} ); }
  |
  expr LOGIC_AND expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::logic_and), $1, $3} ); }
  |
  expr EQ expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::compare_eq), $1, $3} ); }
  |
  expr NEQ expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::compare_neq), $1, $3} ); }
  |
  expr LESS expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::compare_l), $1, $3} ); }
  |
  expr LESS_EQ expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::compare_leq), $1, $3} ); }
  |
  expr MORE expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::compare_g), $1, $3} ); }
  |
  expr MORE_EQ expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::compare_geq), $1, $3} ); }
  |
  expr '+' expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::add), $1, $3} ); }
  |
  expr '-' expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::subtract), $1, $3} ); }
  |
  '-' expr %prec UMINUS
  { $$ = make_list( primitive, @$, {make_const(@1,op_type::negate), $2} ); }
  |
  expr '*' expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::multiply), $1, $3} ); }
  |
  expr '/' expr
    { $$ = make_list( primitive, @$, {make_const(@2,op_type::divide), $1, $3} ); }
  |
  expr INT_DIV expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::divide_integer), $1, $3} ); }
  |
  expr '%' expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::modulo), $1, $3} ); }
  |
  expr '^' expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::raise), $1, $3} ); }
  |
  expr BIT_AND expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::bitwise_and), $1, $3} ); }
  |
  expr BIT_OR expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::bitwise_or), $1, $3} ); }
  |
  expr BIT_XOR expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::bitwise_xor), $1, $3} ); }
  |
  expr BIT_SHIFT_LEFT expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::bitwise_lshift), $1, $3} ); }
  |
  expr BIT_SHIFT_RIGHT expr
  { $$ = make_list( primitive, @$, {make_const(@2,op_type::bitwise_rshift), $1, $3} ); }
  |
  '(' expr ')'
  { $$ = $2; }
  |
  let_expr
  |
  where_expr
  |
  id '=' expr
  {
    $$ = make_list( ast::binding, @$, {$1, nullptr, $3} );
  }
;


let_expr:
  LET binding IN expr
  {
    auto bnd_list = make_list(@2, {$2});
    $$ = make_list(ast::local_scope, @$, { bnd_list, $4 } );
  }
  |
  LET '{' nested_decl_list optional_semicolon '}' IN expr
  {
    $$ = make_list(ast::local_scope, @$, { $3, $7 } );
  }
;

where_expr:
  expr WHERE binding
  {
    auto bnd_list = make_list(@3, {$3});
    $$ = make_list(ast::local_scope, @$, { bnd_list, $1 } );
  }
  |
  expr WHERE '{' nested_decl_list optional_semicolon '}'
  {
    $$ = make_list(ast::local_scope, @$, { $4, $1 } );
  }
;

func_lambda:
  '(' expr[param] ')' RIGHT_ARROW expr[value]
  {
    auto params = make_list(@$, { $[param] });
    $$ = make_list(ast::lambda, @$, { params, $[value] } );
  }
  |
  '(' expr[first_param] ',' expr_list[other_params] ')' RIGHT_ARROW expr[value]
  {
    auto params = make_list(@$, {$[first_param]});
    params->as_list()->append($[other_params]->as_list()->elements);
    $$ = make_list(ast::lambda, @$, {params, $[value]} );
  }
;

array_apply:
  expr '[' expr_list ']'
  { $$ = make_list( ast::array_apply, @$, {$1, $3} ); }
;

array_lambda:
  '[' array_lambda_params ']' RIGHT_ARROW expr
  {
    auto ranges = make_list(@2, {});
    auto indexes = make_list(@2, {});

    for (auto & param : $array_lambda_params->as_list()->elements)
    {
      indexes->as_list()->append(param->as_list()->elements[0]);
      ranges->as_list()->append(param->as_list()->elements[1]);
    }

    auto piece = make_list(@expr, { nullptr, $expr });
    auto pieces = make_list(@expr, { piece });
    auto pattern = make_list(@$, { indexes, pieces });
    auto patterns = make_list(@$, { pattern });

    $$ = make_list( ast::array_def, @$, {ranges, patterns} );
  }
;

array_lambda_params:
  array_lambda_param
  { $$ = make_list( @$, {$1} ); }
  |
  array_lambda_params ',' array_lambda_param
  {
    $$ = $1;
    $$->as_list()->append( $3 );
    $$->location = @$;
  }
;

array_lambda_param:
    id
    { $$ = make_list( @$, {$1, make_node(infinity, @$)} ); }
    |
    id ':' expr
    { $$ = make_list( @$, {$1, $3} ); }
;

array_exprs:
  expr %prec RIGHT_ARROW
  {
    auto constrained_expr = make_list( @$, { nullptr, $1 });
    $$ = make_list( @$, {constrained_expr} );
  }
  |
  constrained_array_expr
  {
    $$ = make_list( @$, {$1} );
  }
  |
  '{' constrained_array_expr_list optional_semicolon '}'
  { $$ = $2; }
  |
  '{' constrained_array_expr_list ';' final_constrained_array_expr optional_semicolon '}'
  {
    $$ = $2;
    $$->as_list()->append( $4 );
    $$->location = @$;
  }
;

constrained_array_expr_list:
  constrained_array_expr
  { $$ = make_list( @$, {$1} ); }
  |
  constrained_array_expr_list ';' constrained_array_expr
  {
    $$ = $1;
    $$->as_list()->append( $3 );
    $$->location = @$;
  }
;

constrained_array_expr:
  expr ',' IF expr    %prec ','
  { $$ = make_list( @$, { $4, $1 } ); }
;

final_constrained_array_expr:
  expr ',' OTHERWISE
  { $$ = make_list( @$, { nullptr, $1 } ); }
;

array_enum:
  '(' expr ',' expr_list ')'
  {
    $$ = make_list(ast::array_enum, @$, { $expr });
    $$->as_list()->append($expr_list->as_list()->elements);
  }
;

array_size:
  '#' expr
  { $$ = make_list( array_size, @$, { $2, nullptr } ); }
  |
  '#' expr '@' expr
  { $$ = make_list( array_size, @$, { $2, $4 } ); }
;

func_apply:
  expr '(' expr_list ')'
  {
    $$ = make_list( ast::func_apply, @$, {$1, $3} );
  }
;

func_composition:
  expr '.' expr
  {
    $$ = make_list( ast::func_compose, @$, {$1, $3} );
  }
;

expr_list:
  expr
  { $$ = make_list( @$, {$1} ); }
  |
  expr_list ',' expr
  {
    $$ = $1;
    $$->as_list()->append( $3 );
  }
;

if_expr:
  IF expr THEN expr ELSE expr
  { $$ = make_list( primitive, @$, {make_const(@1,op_type::conditional), $2, $4, $6} ); }
;

number:
  int
  |
  real
  |
  complex
;

int: INT
;

real: REAL
;

complex: COMPLEX
;

boolean:
  TRUE
  |
  FALSE
;

id: ID
;

qualified_id: QUALIFIED_ID
;

inf: '~'
  { $$ = make_node(infinity, @$); }
;

optional_semicolon: ';' | // empty
;

%%

void
stream::parsing::parser::error (const location_type& l, const std::string& m)
{
  driver.error (l, m);
}
