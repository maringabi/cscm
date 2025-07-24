/*
 * Simple C Scheme Interpreter
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Entry struct used to store environment variables */
typedef struct  EnvEntry {
  char *sym;             // variable name
  int val;               // variable value
  struct EnvEntry *next; // pointer to next environment entry
} EnvEntry;

EnvEntry *global_env = NULL;

/* Store variable with the name sym and value val */
void define_variable(const char *sym, int val) {
  EnvEntry *entry = malloc(sizeof(EnvEntry));
  entry->sym = strdup(sym);
  entry->val = val;
  entry->next = global_env;
  global_env = entry;
}

int lookup_variable(const char *sym) {
  EnvEntry *current = global_env;
  while(strcmp(current->sym, sym)) {
    current  = current->next;
  }
  return current->val;
}

/* put spaces around parents '(' and ')'
 * useful later for tokens spliting with space delimier
 */
char *replace_parens(char *str) {
  size_t len = strlen(str);
  size_t extra_space = 0;

  for(size_t i = 0; i < len; ++i) {
    if(str[i] == '(' || str[i] == ')') {
      extra_space += 2;
    }
  }

  /* alocate string result space adding also the extra space */
  char *res = malloc(sizeof(char) * (len + extra_space + 1));
  if(!res) {
    fprintf(stderr, "malloc failed");
    exit(1);
  }

  /* copy all chars from str and add space around '(' and ')' */
  size_t j = 0; /* res iterator */
  for(size_t i = 0; i < len; ++i) {
    if(str[i] == '(' || str[i] == ')') {
      res[j++] = ' ';
      res[j++] = str[i];
      res[j++] = ' ';
    } else {
      res[j++] = str[i];
    }
  }
  res[j] = '\0'; /* null-terminated */

  return res;
}

/* verify if string is a valid token */
int is_valid_token(const char *token) {
  if(*token == '\0' || token == NULL) {
    return 0;
  }

  /* if token has any character that is not space return 1 */
  for(const char *p = token; *p; ++p) {
    if(!isspace(*p)) {
      return 1;
    }
  }
  
  return 0;
}


/* split str into array of tokens */
char **tokenize(char **str, int *num_tokens) {
  char *token;
  size_t tokens_len = 10; /* starting token length */
  char **tokens = malloc(sizeof(char*) * tokens_len);
  if(!tokens) {
    fprintf(stderr, "malloc failed");
    exit(1);
  }

  char *s = replace_parens(*str);
  size_t j = 0; /* tokens array iterator */
  while ((token = strsep(&s, " ")) != NULL) {
    if(is_valid_token(token)) {
      /* when to increase tokens_len? */
      if(j >= tokens_len) {
        tokens_len *= 2;
        tokens = realloc(tokens, sizeof(char*)*tokens_len);
        if(!tokens) {
          fprintf(stderr, "realloc failed");
          exit(1);
        }
      }
      tokens[j] = strdup(token);
      if(!(tokens[j])) {
        fprintf(stderr, "strdup(token) failed");
        exit(1);
      }
      j++;
    }
  }
  *num_tokens = j;
  
  free(s);
  
  return tokens;
}

/* recursive eval expression function to evaluate s-expr */
int eval_expr(char **tokens, int num_tokens, int *pos) {
  /* begging of s-expr */
  if(strcmp(tokens[*pos], "(") == 0) {
    (*pos)++; /* skip '(' */

    /* s-expr operation */
    const char *op = tokens[(*pos)++];
    int res;

    if(strcmp(op, "+") == 0) {
      res = 0;
      while(*pos < num_tokens && strcmp(tokens[(*pos)], ")") != 0) {
        res += eval_expr(tokens, num_tokens, pos);
      }
    } else if(strcmp(op, "-") == 0) {
      res = eval_expr(tokens, num_tokens, pos);
      while(*pos < num_tokens && strcmp(tokens[(*pos)], ")") != 0) {
        res -= eval_expr(tokens, num_tokens, pos);
      }
    } else if(strcmp(op, "*") == 0) {
      res = eval_expr(tokens, num_tokens, pos);
      while(*pos < num_tokens && strcmp(tokens[(*pos)], ")") != 0) {
        res *= eval_expr(tokens, num_tokens, pos);
      }
    } else if(strcmp(op, "/") == 0) {
      res = eval_expr(tokens, num_tokens, pos);
      while(*pos < num_tokens && strcmp(tokens[(*pos)], ")") != 0) {
        res /= eval_expr(tokens, num_tokens, pos);
      }
    } else if(strcmp(op, "define") == 0) {
      const char *sym = tokens[(*pos)++]; // get variable name
      int val = atoi(tokens[(*pos)++]); // get variable value
      define_variable(sym, val); // store variable in global environment
      if(strcmp(tokens[(*pos)], ")") != 0) {
        fprintf(stderr, "Expected ')'\n");
        exit(1);
      }
    } else {
      fprintf(stderr, "Unsupported operator: %s\n", op);
      exit(1);
    }

    if(*pos >= num_tokens || strcmp(tokens[*pos], ")") != 0) {
      fprintf(stderr, "Expected ')'\n");
      exit(1);
    }
    
    /* end of s-expr */
    (*pos)++; /* skip ')' */
    return res;
  }

  /* base case: s-expr arguments */
  else {
    char *endptr;
    int val = strtol(tokens[*pos], &endptr, 10);
    if(*endptr != '\0') { // if token is a symbol
      val = lookup_variable(tokens[*pos]);
    } else {
      val = atoi(tokens[*pos]);
    }
    (*pos)++;
    return val;
  }
}

/* eval function */
int eval(char **tokens, int num_tokens) {
  int pos = 0;
  return eval_expr(tokens, num_tokens, &pos);
}

int main(int argc , char *argv[]) {
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  char **tokens;
  int num_tokens;

  int res = 0;
  while((linelen = getline(&line, &linecap, stdin)) > 0) {
    tokens = tokenize(&line, &num_tokens);
    res = eval(tokens, num_tokens);
    printf("%d\n", res);
  }
  
  return 0;
}
