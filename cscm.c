/*
 * Simple C Scheme Interpreter
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* Structure reprezenting the tokenizer where each token is stored*/
struct tokenizer {
  char **tokens; /* array of token */
  int len; /* number of tokens */
};

/* Entry struct used to store environment variables */
struct  envEntry {
  char *sym;             // variable name
  int val;               // variable value
  struct envEntry *next; // pointer to next environment entry
};

struct envEntry *global_env = NULL;

void initTokenizer(struct tokenizer *t) {
  t->tokens = NULL;
  t->len = 0;
}

/* Store variable with the name sym and value val */
void define_variable(const char *sym, int val) {
  struct envEntry *entry = malloc(sizeof(struct envEntry));
  entry->sym = strdup(sym);
  entry->val = val;
  entry->next = global_env;
  global_env = entry;
}

int lookup_variable(const char *sym) {
  struct envEntry *current = global_env;
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


/* split str into array of tokens and store them into tokenizer */
void tokenize(struct tokenizer *t, char **str) {
  char *token;
  size_t tokens_len = 10; /* starting token length */
  t->tokens = malloc(sizeof(char*) * tokens_len);
  if(!t->tokens) {
    perror("malloc failed");
    exit(1);
  }

  char *s = replace_parens(*str);
  size_t j = 0; /* tokens array iterator */
  while ((token = strsep(&s, " ")) != NULL) {
    if(is_valid_token(token)) {
      /* when to increase tokens_len? */
      if(j >= tokens_len) {
        tokens_len *= 2;
        t->tokens = realloc(t->tokens, sizeof(char*) * tokens_len);
        if(!t->tokens) {
          perror("realloc failed");
          exit(1);
        }
      }
      t->tokens[j] = strdup(token);
      if(!(t->tokens[j])) {
        perror("strdup(token) failed");
        exit(1);
      }
      j++;
    }
  }
  t->len = j; /* set number of tokens */
  free(s);
}

/* recursive eval expression function to evaluate s-expr */
int eval_expr(struct tokenizer *t, int *pos) {
  /* begging of s-expr */
  if(strcmp(t->tokens[*pos], "(") == 0) {
    (*pos)++; /* skip '(' */

    /* s-expr operation */
    const char *op = t->tokens[(*pos)++];
    int res;

    if(strcmp(op, "+") == 0) {
      res = 0;
      while(*pos < t->len && strcmp(t->tokens[(*pos)], ")") != 0) {
        res += eval_expr(t, pos);
      }
    } else if(strcmp(op, "-") == 0) {
      res = eval_expr(t, pos);
      while(*pos < t->len && strcmp(t->tokens[(*pos)], ")") != 0) {
        res -= eval_expr(t, pos);
      }
    } else if(strcmp(op, "*") == 0) {
      res = eval_expr(t, pos);
      while(*pos < t->len && strcmp(t->tokens[(*pos)], ")") != 0) {
        res *= eval_expr(t, pos);
      }
    } else if(strcmp(op, "/") == 0) {
      res = eval_expr(t, pos);
      while(*pos < t->len && strcmp(t->tokens[(*pos)], ")") != 0) {
        res /= eval_expr(t, pos);
      }
    } else if(strcmp(op, "define") == 0) {
      const char *sym = t->tokens[(*pos)++]; // get variable name
      int val = atoi(t->tokens[(*pos)++]); // get variable value
      define_variable(sym, val); // store variable in global environment
      if(strcmp(t->tokens[(*pos)], ")") != 0) {
        perror("Expected ')'\n");
        exit(1);
      }
    } else {
      fprintf(stderr, "Unsupported operator: %s\n", op);
      exit(1);
    }

    if(*pos >= t->len || strcmp(t->tokens[*pos], ")") != 0) {
      perror("Expected ')'\n");
      exit(1);
    }
    
    /* end of s-expr */
    (*pos)++; /* skip ')' */
    return res;
  }

  /* base case: s-expr arguments */
  else {
    char *endptr;
    int val = strtol(t->tokens[*pos], &endptr, 10);
    if(*endptr != '\0') { // if token is a symbol
      val = lookup_variable(t->tokens[*pos]);
    } else {
      val = atoi(t->tokens[*pos]);
    }
    (*pos)++;
    return val;
  }
}

/* eval function */
int eval(struct tokenizer *t) {
  int pos = 0;
  return eval_expr(t, &pos);
}

int main(int argc , char *argv[]) {
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  struct tokenizer t;
  int res = 0;

  initTokenizer(&t);

  while(1) {
    printf("cscm> "); fflush(stdout); /* Prompt */
    linelen = getline(&line, &linecap, stdin);
    if(linelen <= 0) break; /* if we don't have any character or and error break */
    tokenize(&t, &line);
    res = eval(&t);
    printf("%d\n", res);
  }
  return 0;
}
