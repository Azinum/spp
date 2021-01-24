// parser.c

#include "common.h"
#include "ast.h"
#include "util.h"
#include "lexer.h"
#include "token.h"
#include "parser.h"

#define parse_error(fmt, ...) \
  fprintf(stderr, "[ParseError]: %s:%i:%i: " fmt, p->l->filename, p->l->line, p->l->count, ##__VA_ARGS__)

#define TAB_SIZE 2

static void parser_init(Parser* parser, Lexer* lexer, Ast* ast);
static void print_tabs(FILE* file, i32 level);
static i32 expect(Parser* p, i32 type);
static i32 end(Parser* p);
static i32 block_end(Parser* p);
static i32 block(Parser* p);
static i32 lambda_return(Parser* p);
static i32 lambda_arglist(Parser* p);
static i32 lambda(Parser* p, i32 id);
static i32 statement(Parser* p);
static i32 statements(Parser* p);

void parser_init(Parser* p, Lexer* l, Ast* ast) {
  p->l = l;
  p->ast = ast;
  p->lambda_id_counter = 0;
  p->status = 0;
}

void print_tabs(FILE* file, i32 level) {
  for (i32 i = 0; i < level * TAB_SIZE; ++i) {
    fprintf(file, " ");
  }
}

i32 expect(Parser* p, i32 type) {
  return p->l->token.type == type;
}

i32 end(Parser* p) {
  struct Token token = get_token(p->l);
  return token.type == T_EOF;
}

i32 block_end(Parser* p) {
  struct Token token = get_token(p->l);
  return token.type == T_EOF || token.type == T_BLOCKEND;
}

i32 block(Parser* p) {
  while (!block_end(p)) {
    statement(p);
  }
  return 0;
}

i32 lambda_return(Parser* p) {
  struct Token token = get_token(p->l);
  for (;;) {
    if (token.type == T_SEMICOLON || token.type == T_BLOCKBEGIN) {
      return 0;
    }
    ast_add_node(p->ast, token);
    token = next_token(p->l);
  }
  return 0;
}

i32 lambda_arglist(Parser* p) {
  struct Token token = get_token(p->l);
  if (!expect(p, T_OPENPAREN)) {
    parse_error("Missing open '(' parenthesis in lambda expression\n");
    return p->status = -1;
  }
  ast_add_node(p->ast, token);

  for (;;) {
    token = next_token(p->l);
    if (token.type == T_CLOSEDPAREN) {
      ast_add_node(p->ast, token);
      next_token(p->l);
      break;
    }
    else if (token.type == T_EOF) {
      parse_error("Missing closing ')' parenthesis in lambda expression\n");
      return p->status = -1;
    }
    ast_add_node(p->ast, token);
  }
  return 0;
}

// $ (arg list) { body }
// Ast output:
// T_LAMBDA_BODY (the place in the ast where all lambda expressions are located)
// \--T_LAMBDA  TODO(lucas): Put each lambda expression in a seperate branch
//    \-- arglist
//
//    \-- return type NOTE(lucas): For now we store the id of the lambda expression here. Should probably be in a more sensible place, like in the lambda_body node.
//
//    \-- body contents
i32 lambda(Parser* p, i32 id) {
  Ast* orig_branch = p->ast;

  ast_add_node(p->ast, (struct Token) { .type = T_ARGLIST });
  Ast arglist_branch = ast_get_last(p->ast);

  // Argument list
  p->ast = &arglist_branch;
  lambda_arglist(p);
  p->ast = orig_branch;

  if (!expect(p, T_ARROW)) {
    parse_error("Expected '->' in lambda expression\n");
    return p->status = -1;
  }
  next_token(p->l);

  ast_add_node(p->ast, (struct Token) { .type = T_RETURN_TYPE, .id = id });
  Ast return_type_branch = ast_get_last(p->ast);

  p->ast = &return_type_branch;
  lambda_return(p);
  p->ast = orig_branch;

  // Lambda body
  if (!expect(p, T_BLOCKBEGIN)) {
    parse_error("Expected '{' in lambda expression\n");
    return p->status = -1;
  }
  next_token(p->l); // Skip '{'

  ast_add_node(p->ast, (struct Token) { .type = T_BODY });
  Ast body_branch = ast_get_last(p->ast);

  p->ast = &body_branch;
  block(p); // Parse the lambda body
  p->ast = orig_branch;

  if (!expect(p, T_BLOCKEND)) {
    parse_error("Expected '}' in lambda expression\n");
    return p->status = -1;
  }
  next_token(p->l); // Skip '}'
  return 0;
}

i32 statement(Parser* p) {
  struct Token token = get_token(p->l);
  switch (token.type) {
    case T_EOF:
      break;

    case T_DOLLAR: {
      i32 lambda_id = p->lambda_id_counter++;
      char* lambda_identifier = "lambda";
      ast_add_node(p->ast, (struct Token) { .type = T_SYMBOL, .string = lambda_identifier, .length = strlen(lambda_identifier), .id = lambda_id});
      Ast* orig_branch = p->ast;
      p->ast = &p->lambda_branch;
      next_token(p->l); // Skip '$'
      lambda(p, lambda_id);
      p->ast = orig_branch;
      break;
    }

    default:
      next_token(p->l);
      ast_add_node(p->ast, token);
      break;
  }
  return 0;
}

i32 statements(Parser* p) {
  while (!end(p)) {
    p->status = statement(p);
  }
  return p->status;
}

i32 parser_parse(char* input, char* filename, Ast* ast) {
  Lexer lexer;
  lexer_init(&lexer, input, filename);

  Parser parser;
  parser_init(&parser, &lexer, ast);

  char* lambda_body = "lambda_body";
  ast_add_node(parser.ast, (struct Token) { .type = T_LAMBDA_BODY, .string = lambda_body, .length = strlen(lambda_body) });
  parser.lambda_branch = ast_get_last(parser.ast);

  next_token(parser.l);
  statements(&parser);
  return 0;
}
