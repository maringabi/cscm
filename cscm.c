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
  int len;       /* number of tokens */
  int pos;       /* current string position */
};

/* Entry struct used to store environment variables */
struct  environment {
  char *sym;                 /* variable name */
  int val;                   /* variable value */
  struct environment *next;  /* pointer to next environment entry */
};

struct interpreter {
  struct environment *env; /* interpreter global environment */
  int result;              /* result after evaluation */
};

void initTokenizer(struct tokenizer *t) {
  t->tokens = NULL;
  t->len = 0;
  t->pos = 0;
}

void initInterpreter(struct interpreter *i) {
  i->env = NULL;
  i->result = 0;
}

/* Store variable with the name sym and value val */
void defineVariable(struct interpreter *i, const char *sym, int val) {
  struct environment *entry = malloc(sizeof(struct environment));
  entry->sym = strdup(sym);
  entry->val = val;
  entry->next = i->env;
  i->env = entry;
}

/* Lookup variable with the name 'sym' in interpreter global environment */
int lookupVariable(struct interpreter *i, const char *sym) {
  struct environment *current = i->env;
  while(strcmp(current->sym, sym)) {
    current  = current->next;
  }
  return current->val;
}

/* put spaces around parents useful later for tokens spliting with space delimier */
char *replaceParens(char *str) {
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
int validToken(const char *token) {
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

  char *s = replaceParens(*str);
  size_t j = 0; /* tokens array iterator */
  while ((token = strsep(&s, " ")) != NULL) {
    if(validToken(token)) {
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
int evalExpr(struct interpreter *i, struct tokenizer *t) {
  /* begging of s-expr */
  if(strcmp(t->tokens[t->pos], "(") == 0) {
    t->pos++; /* skip '(' */

    /* s-expr operation */
    const char *op = t->tokens[t->pos++];
    /* We cannot use 'i->result' because interpreter structure is passed around at
       every 'evalExpr' call and the result is remember from previous 'evalExpr' calls.
       We need a new res at every 'evalExpr' call. Final result here is stored in 'eval'. */
    int res;
    if(strcmp(op, "+") == 0) {
      res = evalExpr(i, t);
      while(t->pos < t->len && strcmp(t->tokens[t->pos], ")") != 0) {
        res += evalExpr(i, t);
      }
    } else if(strcmp(op, "-") == 0) {
      res = evalExpr(i, t);
      while(t->pos < t->len && strcmp(t->tokens[t->pos], ")") != 0) {
        res -= evalExpr(i, t);
      }
    } else if(strcmp(op, "*") == 0) {
      res = evalExpr(i, t);
      while(t->pos < t->len && strcmp(t->tokens[t->pos], ")") != 0) {
        res *= evalExpr(i, t);
      }
    } else if(strcmp(op, "/") == 0) {
      res = evalExpr(i, t);
      while(t->pos < t->len && strcmp(t->tokens[t->pos], ")") != 0) {
        res /= evalExpr(i, t);
      }
    } else if(strcmp(op, "define") == 0) {
      const char *sym = t->tokens[t->pos++]; // get variable name
      int val = atoi(t->tokens[t->pos++]); // get variable value
      defineVariable(i, sym, val); /* store variable in global environment */
      if(strcmp(t->tokens[t->pos], ")") != 0) {
        perror("Expected ')'\n");
        exit(1);
      }
    } else {
      fprintf(stderr, "Unsupported operator: %s\n", op);
      exit(1);
    }

    if(t->pos >= t->len || strcmp(t->tokens[t->pos], ")") != 0) {
      perror("Expected ')'\n");
      exit(1);
    }
    
    /* end of s-expr */
    t->pos++; /* skip ')' */
    return res;
  }

  /* base case: s-expr arguments */
  else {
    char *endptr;
    int val = strtol(t->tokens[t->pos], &endptr, 10);
    if(*endptr != '\0') { /* if token is a symbol */
      val = lookupVariable(i, t->tokens[t->pos]);
    } else {
      val = atoi(t->tokens[t->pos]);
    }
    t->pos++;
    return val;
  }
}

/* EVAL */
void eval(struct interpreter *i, struct tokenizer *t) {
  t->pos = 0; /* reset tokenizer position for the new line */
  i->result = evalExpr(i, t);
}

int main(int argc , char *argv[]) {
  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  struct tokenizer t;
  struct interpreter i;

  initTokenizer(&t);
  initInterpreter(&i);

  while(1) {
    printf("cscm> "); fflush(stdout); /* Prompt */
    linelen = getline(&line, &linecap, stdin);
    if(linelen <= 0) break; /* if we don't have any character or and error break */
    tokenize(&t, &line);
    eval(&i, &t);
    printf("%d\n", i.result);

    /* reset tokenizer */
    free(t.tokens);
    t.len = 0;
    t.pos = 0;
  }
  return 0;
}
