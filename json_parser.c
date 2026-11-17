#include <ctype.h>
#include <stdio.h>
#define BTC_IMPLEMENTATION
#include "btc.h"
#include <stdarg.h>

// clang-format off
#define NULL_CHAR             '\0'
#define ESCAPE_CHAR           '\\'
#define OPEN_CURLY_CHAR       '{'
#define CLOSE_CURLY_CHAR      '}'
#define OPEN_BRACKET_CHAR     '['
#define CLOSE_BRACKET_CHAR    ']'
#define NEWLINE_CHAR          '\n'
#define TAB_CHAR              '\t'
#define FORM_FEED_CHAR        '\f'
#define CARRIAGE_RETURN_CHAR  '\r'
#define VERTICAL_TAB_CHAR     '\v'
#define SPACE_CHAR            ' '
#define DOUBLE_QUOTE_CHAR     '"'
#define COMMA_CHAR            ','
#define COLON_CHAR            ':'
#define T_CHAR                't'
#define F_CHAR                'f'
#define N_CHAR                'n'
#define MINUS_CHAR            '-'
#define PLUS_CHAR             '+'
#define ZERO_CHAR             '0'
#define ONE_CHAR              '1'
#define TWO_CHAR              '2'
#define THREE_CHAR            '3'
#define FOUR_CHAR             '4'
#define FIVE_CHAR             '5'
#define SIX_CHAR              '6'
#define SEVEN_CHAR            '7'
#define EIGHT_CHAR            '8'
#define NINE_CHAR             '9'
#define DECIMAL_POINT_CHAR    '.'
#define E_CHAR                'e'
#define CAP_E_CHAR            'E'

// clang-format off

typedef enum Logger_Type {
  Logger_Type_File_Logger,
} Logger_Type;

typedef struct File_Logger {
  FILE *file;
} File_Logger;

void file_logger_logv(File_Logger *logger, const char *fmt, va_list args) {
  vfprintf(logger->file, fmt, args);
}

void file_logger_log(File_Logger *logger, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  file_logger_logv(logger, fmt, ap);
  va_end(ap);
}

typedef struct Logger {
  Logger_Type type;
  union Logger_Context {
    File_Logger file_logger;
  } ctx;
} Logger;

Logger make_file_logger(FILE *file) {
  return cast(Logger){
      .type = Logger_Type_File_Logger,
      .ctx.file_logger = cast(File_Logger){.file = file},
  };
}

Logger default_logger() {
  static Logger k_default_logger = {0};
  k_default_logger = make_file_logger(stdout);
  return k_default_logger;
}

typedef enum Log_Level {
  Log_Level_Debug = 10,
  Log_Level_Info = 20,
  Log_Level_Warn = 30,
  Log_Level_Error = 40,
} Log_Level;

void logger_logv(Logger *logger, Log_Level level, const char *fmt,
                 va_list args) {

  switch (logger->type) {
  case Logger_Type_File_Logger: {
    file_logger_logv(&logger->ctx.file_logger, fmt, args);
  } break;
  default:
    assert(0 && "Unknown Logger Type");
  }
}

void logger_log(Logger *logger, Log_Level level, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  logger_logv(logger, level, fmt, ap);
  va_end(ap);
}

#define logger_debug(logger, fmt, ...)                                         \
  do {                                                                         \
    logger_log((logger), Log_Level_Debug, fmt, __VA_ARGS__);                   \
  } while (0)

// unit testing framework
typedef void *Test_Case_Context;
typedef bool (*Test_Fn)(void *);

typedef struct Test_Case {
  const char *name;
  Test_Fn fn;
} Test_Case;

typedef struct Test_Cases {
  Test_Case *items;
  usize len;
  usize capacity;
} Test_Cases;

typedef Test_Case_Context (*Test_Case_Setup_Fn)();
typedef void (*Test_Case_Teardown_Fn)(Test_Case_Context);

typedef struct Test_Suite {
  String name;
  Test_Cases tests;

  Test_Case_Setup_Fn setup;
  Test_Case_Teardown_Fn teardown;

  Logger logger;
} Test_Suite;

Test_Suite make_test_suite(String suite_name) {
  Test_Suite suite = cast(Test_Suite){
      .name = suite_name,
  };
  suite.logger = default_logger();
  return suite;
}

void test_suite_register_setup(Test_Suite *suite, Test_Case_Setup_Fn fn) {
  suite->setup = fn;
}

void test_suite_register_teardown(Test_Suite *suite, Test_Case_Teardown_Fn fn) {
  suite->teardown = fn;
}

#define make_test_case(test_fn)                                                \
  cast(Test_Case) { .name = #test_fn, .fn = (test_fn) }

#define test_suite_register_test(suite, test_fn, allocator)                    \
  do {                                                                         \
    da_append(&(suite)->tests, (make_test_case(test_fn)), (allocator));        \
  } while (0)

void test_suite_run(Test_Suite *suite) {
  for (u64 i = 0; i < suite->tests.len; ++i) {

    Test_Case_Context ctx = NULL;
    if (suite->setup) {
      ctx = suite->setup();
    }
    Test_Case *test_case = &suite->tests.items[i];
    bool test_pass = test_case->fn(ctx);
    const char *pass_fail = test_pass ? "[PASS]" : "[FAIL]";

    logger_debug(&suite->logger, "%s: %s\n", pass_fail, test_case->name);

    if (suite->teardown) {
      suite->teardown(ctx);
    }
  }
}

// json tokenizer
typedef enum Json_Token_Type {
  JTT_EOF = 0,
  JTT_Open_Curly,
  JTT_Close_Curly,
  JTT_Open_Bracket,
  JTT_Close_Bracket,
  JTT_Comma,
  JTT_Colon,
  JTT_True,
  JTT_False,
  JTT_String,
  JTT_Number,
  JTT_Null,
} Json_Token_Type;

typedef struct Json_Token {
  Json_Token_Type type;
  String content;
  u32 line;
  u32 col;
} Json_Token;

void print_token_type(Json_Token_Type type) {
  const char *typestr = "";
  switch (type) {
  	case JTT_EOF:           typestr = "JTT_EOF";            break;
  	case JTT_Open_Curly:    typestr = "JTT_Open_Curly";     break;
  	case JTT_Close_Curly:   typestr = "JTT_Close_Curly";    break;
  	case JTT_Open_Bracket:  typestr = "JTT_Open_Bracket";   break;
  	case JTT_Close_Bracket: typestr = "JTT_Close_Bracket";  break;
  	case JTT_Comma:         typestr = "JTT_Comma";          break;
  	case JTT_Colon:         typestr = "JTT_Colon";          break;
  	case JTT_True:          typestr = "JTT_True";           break;
  	case JTT_False:         typestr = "JTT_False";          break;
  	case JTT_String:        typestr = "JTT_String";         break;
  	case JTT_Number:        typestr = "JTT_Number";         break;
  	case JTT_Null:          typestr = "JTT_Null";           break;
  }

  printf("%s", typestr);
}

void print_json_token(const Json_Token *token) {
  printf("type: ");
  print_token_type(token->type);
  printf("\ncontent: %.*s\n", cast(int)token->content.len, token->content.data);
  printf("line: %d\n", cast(int)token->line);
  printf("col: %d\n", cast(int)token->col);
}

typedef struct Json_Tokenizer {
  String content;
  isize index;
  isize line;
  isize last_line_index;
} Json_Tokenizer;

Json_Tokenizer make_json_tokenizer(String content) {
  return cast(Json_Tokenizer){
      .content = content,
      .index = 0,
      .line = 1,
      .last_line_index = 0,
  };
}

u8 json_tokenizer_peek_char(Json_Tokenizer *tokenizer);
u8 json_tokenizer_advance(Json_Tokenizer *tokenizer);

bool is_whitespace(u8 c) {
  // clang-format off
  return (           SPACE_CHAR == c ) ||
         (         NEWLINE_CHAR == c ) ||
         ( CARRIAGE_RETURN_CHAR == c ) ||
         (             TAB_CHAR == c ) ||
         (       FORM_FEED_CHAR == c ) ||
         (    VERTICAL_TAB_CHAR == c ) ;
  // clang-format on
}

void json_tokenizer_eat_whitespace(Json_Tokenizer *tokenizer) {

  while (true) {
    if (tokenizer->index >= tokenizer->content.len) {
      return;
    }
    u8 c = json_tokenizer_peek_char(tokenizer);
    if (is_whitespace(c)) {
      json_tokenizer_advance(tokenizer);
    } else {
      return;
    }
  }
}

u8 json_tokenizer_peek_char(Json_Tokenizer *tokenizer) {
  if (tokenizer->index < tokenizer->content.len)
    return tokenizer->content.data[tokenizer->index];

  return NULL_CHAR;
}

u8 json_tokenizer_advance(Json_Tokenizer *tokenizer) {
  if (tokenizer->index < tokenizer->content.len) {
    if (tokenizer->index > 0) {
      u8 prev_c = tokenizer->content.data[tokenizer->index - 1];
      if (NEWLINE_CHAR == prev_c) {
        tokenizer->line++;
        tokenizer->last_line_index = tokenizer->index - 1;
      }
    }
    u8 c = tokenizer->content.data[tokenizer->index];
    tokenizer->index++;
    return c;
  }

  return NULL_CHAR;
}

Json_Token json_tokenizer_create_last_token(Json_Tokenizer *tokenizer) {
  assert(
      tokenizer->index >= tokenizer->content.len &&
      "json_tokenizer_create_last_token should only be called when the content "
      "the tokenizer is tokenizing is exhausted");
  return cast(Json_Token){
      .type = JTT_EOF,
      .content = str(""),
      .line = tokenizer->line,
      .col = (tokenizer->content.len) - tokenizer->last_line_index,
  };
}

// clang-format off
bool json_tokenizer_tokenize_string(Json_Tokenizer *tokenizer, Json_Token *token) {
  u8 first_char = json_tokenizer_peek_char(tokenizer);
  if (DOUBLE_QUOTE_CHAR != first_char) { return false; }

  isize line = tokenizer->line;
  isize first_char_index = tokenizer->index;
  u32 col = cast(u32)(first_char_index - tokenizer->last_line_index);
  json_tokenizer_advance(tokenizer);
  bool escaped = false;
  while (true) {
    u8 c = json_tokenizer_peek_char(tokenizer);
    if (NULL_CHAR == c) { return false; }

    escaped = (ESCAPE_CHAR == c) ^ escaped; // we xor the escaped to unescape a previous escape char

    if (!escaped && DOUBLE_QUOTE_CHAR == c) {
      *token = cast(Json_Token) {
        .type = JTT_String,
        .content = cast(String) {
          .data = &tokenizer->content.data[first_char_index],
          .len = tokenizer->index - first_char_index + 1,
          },
        .line = line,
        .col = col,
      };
      return true;
    }
    json_tokenizer_advance(tokenizer);
  }
}

bool json_tokenizer_tokenize_bool(Json_Tokenizer *tokenizer, Json_Token *token) {
  
  u8 first_char = json_tokenizer_peek_char(tokenizer);
  if ('f' == first_char) {
    // expecting 'false'
    if (tokenizer->index+4 >= tokenizer->content.len) { return false; }

    String val = cast(String) {
      .data = &tokenizer->content.data[tokenizer->index],
      .len = 5,
    };
    if (!string_compare(str("false"), val)) { return false; }

    *token = cast(Json_Token) {
      .type = JTT_False,
      .content = val,
      .line = tokenizer->line,
      .col = tokenizer->index - tokenizer->last_line_index,
    };
    // advance only to the last character
    json_tokenizer_advance(tokenizer);
    json_tokenizer_advance(tokenizer);
    json_tokenizer_advance(tokenizer);
    json_tokenizer_advance(tokenizer);
    return true;
  }

  if ('t' == first_char) {
    // expecting 'true'
    if (tokenizer->index+3 >= tokenizer->content.len) { return false; }

    String val = cast(String) {
      .data = &tokenizer->content.data[tokenizer->index],
      .len = 4,
    };
    if (!string_compare(str("true"), val)) { return false; }

    *token = cast(Json_Token) {
      .type = JTT_True,
      .content = val,
      .line = tokenizer->line,
      .col = tokenizer->index - tokenizer->last_line_index,
    };
    // advance only to the last character
    json_tokenizer_advance(tokenizer);
    json_tokenizer_advance(tokenizer);
    json_tokenizer_advance(tokenizer);
    return true;
  }

  return false;
}

bool json_tokenizer_tokenize_null_value(Json_Tokenizer *tokenizer, Json_Token *token) {
  u8 first_char = json_tokenizer_peek_char(tokenizer);
  if (N_CHAR == first_char) {
    // expecting 'null'
    if (tokenizer->index+3 >= tokenizer->content.len) { return false; }

    String val = cast(String) {
      .data = &tokenizer->content.data[tokenizer->index],
      .len = 4,
    };
    if (!string_compare(str("null"), val)) { return false; }

    *token = cast(Json_Token) {
      .type = JTT_False,
      .content = val,
      .line = tokenizer->line,
      .col = tokenizer->index - tokenizer->last_line_index,
    };
    // advance only to the last character
    json_tokenizer_advance(tokenizer);
    json_tokenizer_advance(tokenizer);
    json_tokenizer_advance(tokenizer);
    return true;
  }

  return false;
}
bool json_tokenizer_tokenize_number(Json_Tokenizer *tokenizer, Json_Token *token) {
  // integers 0
  // negative -1
  // decimal
  // scientific 1.0e1
  
  bool first_digit_found = false;
  bool decimal_point_found = false;
  bool scientific_mode = false;

  isize line = tokenizer->line;
  isize first_index = tokenizer->index;
  u32 col = tokenizer->index - tokenizer->last_line_index;

  u8 c = json_tokenizer_peek_char(tokenizer);
  if (MINUS_CHAR == c) {
    if (tokenizer->index + 1 >= tokenizer->content.len) {
      // minus cannot be alone at the end
      return false;
    }
    json_tokenizer_advance(tokenizer);
  }

  bool done_processing_number = false;
  while (!done_processing_number) {
    c = json_tokenizer_peek_char(tokenizer);

    if (!first_digit_found) {
      if (!isdigit(c)) { return false; }
      first_digit_found = true;
      json_tokenizer_advance(tokenizer);
      continue;
    }

    if (DECIMAL_POINT_CHAR == c) {
      // early escape if second decimal point found or if scientific mode already enabled
      if (decimal_point_found || scientific_mode) { return false; }

      // check next char is a digit
      if (tokenizer->index + 1 >= tokenizer->content.len) {
        return false;
      }
      u8 next_c = tokenizer->content.data[tokenizer->index+1];
      if (!isdigit(next_c)) { return false; }

      decimal_point_found = true;
      json_tokenizer_advance(tokenizer);
      json_tokenizer_advance(tokenizer);
      continue;
    }

    if (E_CHAR == c || CAP_E_CHAR == c) {
      if (scientific_mode) { return false; }

      json_tokenizer_advance(tokenizer);
      c = json_tokenizer_peek_char(tokenizer);
      
      if (PLUS_CHAR == c || MINUS_CHAR == c) {
        json_tokenizer_advance(tokenizer);
        c = json_tokenizer_peek_char(tokenizer);
      }

      if (!isdigit(c)) { return false; }

      scientific_mode = true;
      json_tokenizer_advance(tokenizer);
      continue;
    }

    if (isdigit(c)) {
      json_tokenizer_advance(tokenizer);
      continue;
    }

    done_processing_number = true;
  }

  // we are one character past the end of the number
  *token = cast(Json_Token) {
    .type = JTT_Number,
    .content = cast(String) {
      .data = &tokenizer->content.data[first_index],
      .len = tokenizer->index - first_index,
      },
    .col = col,
    .line = line,
  };

  // need to rewind one character so outer processing loop can process it
  tokenizer->index--;

  return true;
}
// clang-format off

bool json_tokenizer_next_token(Json_Tokenizer *tokenizer, Json_Token *token) {

  json_tokenizer_eat_whitespace(tokenizer);

  if (tokenizer->index >= tokenizer->content.len) {
    *token = json_tokenizer_create_last_token(tokenizer);
    return false;
  }

  Json_Token result = {0};
  result.line = tokenizer->line;
  result.col = tokenizer->index - tokenizer->last_line_index;

  u8 c = json_tokenizer_peek_char(tokenizer);

  bool success = true;
  switch (c) {
  case OPEN_CURLY_CHAR: {
    result.type = JTT_Open_Curly;
    result.content = str("{");
  } break;
  case CLOSE_CURLY_CHAR: {
    result.type = JTT_Close_Curly;
    result.content = str("}");
  } break;
  case OPEN_BRACKET_CHAR: {
    result.type = JTT_Open_Bracket;
    result.content = str("[");
  } break;
  case CLOSE_BRACKET_CHAR: {
    result.type = JTT_Close_Bracket;
    result.content = str("]");
  } break;
  case COMMA_CHAR: {
    result.type = JTT_Comma;
    result.content = str(",");
  } break;
  case COLON_CHAR: {
    result.type = JTT_Colon;
    result.content = str(":");
  } break;

  case DOUBLE_QUOTE_CHAR: {
    success = json_tokenizer_tokenize_string(tokenizer, &result);
  } break;
  case T_CHAR:
  case F_CHAR: {
    success = json_tokenizer_tokenize_bool(tokenizer, &result);
  } break;
  case N_CHAR: {
    success = json_tokenizer_tokenize_null_value(tokenizer, &result);
  } break;
  case MINUS_CHAR:
  case  ZERO_CHAR:
  case   ONE_CHAR:
  case   TWO_CHAR:
  case THREE_CHAR:
  case  FOUR_CHAR:
  case  FIVE_CHAR:
  case   SIX_CHAR:
  case SEVEN_CHAR:
  case EIGHT_CHAR:
  case  NINE_CHAR: {
    success = json_tokenizer_tokenize_number(tokenizer, &result);
  } break;
  }

  json_tokenizer_advance(tokenizer);
  if (success) {
    *token = result;
  }
  return success;
};

// json tokenizer tests
bool json_tokenizer_returns_eof_token_when_content_is_exhausted(
    Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str(""));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);
  return !success && token.type == JTT_EOF && string_compare(token.content, str(""));
}

bool json_tokenizer_returns_open_curly_token(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("{"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);
  return success && token.type == JTT_Open_Curly &&
         string_compare(token.content, str("{"));
}

bool json_tokenizer_shifts_contents_after_tokenizing(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("{"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  memset(&token, 0, sizeof(token));
  success = json_tokenizer_next_token(&tokenizer, &token);
  return !success && token.type == JTT_EOF && string_compare(token.content, str(""));
}

bool json_tokenizer_tokenizes_strings(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("\"the quick brown fox\""));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_String &&
    string_compare(token.content, str("\"the quick brown fox\""));
}

bool json_tokenizer_tokenizes_true(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("true"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_True &&
    string_compare(token.content, str("true"));
}

bool json_tokenizer_tokenizes_false(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("false"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_False &&
    string_compare(token.content, str("false"));
}

bool json_tokenizer_tokenizes_integer_number(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("1234567890"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_Number &&
    string_compare(token.content, str("1234567890"));
}

bool json_tokenizer_tokenizes_negative_integer_number(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("-1234567890"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_Number &&
    string_compare(token.content, str("-1234567890"));
}

bool json_tokenizer_tokenizes_decimal_number(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("1234.567890"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_Number &&
    string_compare(token.content, str("1234.567890"));
}

bool json_tokenizer_tokenizes_negative_decimal_number(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("-1234.567890"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_Number &&
    string_compare(token.content, str("-1234.567890"));
}

bool json_tokenizer_tokenizes_non_decimal_scientific(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("1e4"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_Number &&
    string_compare(token.content, str("1e4"));
}

bool json_tokenizer_tokenizes_negative_non_decimal_scientific_number(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("-1e4"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_Number &&
    string_compare(token.content, str("-1e4"));
}

bool json_tokenizer_tokenizes_decimal_scientific_number(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("1.03e4"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_Number &&
    string_compare(token.content, str("1.03e4"));
}

bool json_tokenizer_tokenizes_negative_decimal_scientific_number(Test_Case_Context ctx) {
  UNUSED(ctx);

  Json_Tokenizer tokenizer = make_json_tokenizer(str("-1.03e4"));

  Json_Token token = {0};
  bool success = json_tokenizer_next_token(&tokenizer, &token);

  return success &&
    token.type == JTT_Number &&
    string_compare(token.content, str("-1.03e4"));
}

#ifndef COMPILE_TESTS
#define COMPILE_TESTS 0
#endif
#if COMPILE_TESTS
int main(void) {
  Arena arena = {0};
  arena_init(&arena, 40000);

  Allocator allocator = arena_make_allocator(&arena);

  Test_Suite suite = make_test_suite(str("Json_Tokenizer_Tests"));
  // clang-format off
  test_suite_register_test(&suite, json_tokenizer_returns_eof_token_when_content_is_exhausted, &allocator);
  test_suite_register_test(&suite, json_tokenizer_returns_open_curly_token, &allocator);
  test_suite_register_test(&suite, json_tokenizer_shifts_contents_after_tokenizing, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_strings, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_true, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_false, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_integer_number, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_negative_integer_number, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_decimal_number, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_negative_decimal_number, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_non_decimal_scientific, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_negative_non_decimal_scientific_number, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_decimal_scientific_number, &allocator);
  test_suite_register_test(&suite, json_tokenizer_tokenizes_negative_decimal_scientific_number, &allocator);

  // clang-format off

  test_suite_run(&suite);
}

#else

int main(void) {
  Arena arena = {0};
  arena_init(&arena, 40000);

  Allocator allocator = arena_make_allocator(&arena);
  

  String json = str("{ \"x\": [ 1234, -12.23, 1e-4], \"y\": -1.2e4, \"z\": \"hello world\", \"alpha\": null, \"beta\": true, \"gamma\": false }");

  Json_Tokenizer tokenizer = make_json_tokenizer(json);

  printf("Input: %.*s\n", cast(int)json.len, json.data);
  printf("Length: %d\n", cast(int)json.len);
  bool success = true;
  for (int i=0; success; ++i) {
    Json_Token token = {0};

    printf("\nToken %d\n", i);
    success = json_tokenizer_next_token(&tokenizer, &token);
    print_json_token(&token);
  }
}

#endif
