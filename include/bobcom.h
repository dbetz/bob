/* bobcom.h - compiler definitions */
/*
        Copyright (c) 1999, by David Michael Betz
        All rights reserved
*/

#ifndef __BOBCOM_H__
#define __BOBCOM_H__

#include "bob.h"

/* BobToken definitions */
#define T_NOTOKEN       -1
#define T_EOF           0

/* non-character tokens */
#define _TMIN           256
#define T_STRING        256
#define T_IDENTIFIER    257
#define T_INTEGER       258
#define T_FLOAT         259
#define T_FUNCTION      260
#define T_LOCAL         261
#define T_IF            262
#define T_ELSE          263
#define T_WHILE         264
#define T_RETURN        265
#define T_FOR           266
#define T_BREAK         267
#define T_CONTINUE      268
#define T_DO            269
#define T_SWITCH        270
#define T_CASE          271
#define T_DEFAULT       272
#define T_NIL           273
#define T_LE            274     /* '<=' */
#define T_EQ            275     /* '==' */
#define T_NE            276     /* '!=' */
#define T_GE            277     /* '>=' */
#define T_SHL           278     /* '<<' */
#define T_SHR           279     /* '>>' */
#define T_AND           280     /* '&&' */
#define T_OR            281     /* '||' */
#define T_INC           282     /* '++' */
#define T_DEC           283     /* '--' */
#define T_ADDEQ         284     /* '+=' */
#define T_SUBEQ         285     /* '-=' */
#define T_MULEQ         286     /* '*=' */
#define T_DIVEQ         287     /* '/=' */
#define T_REMEQ         288     /* '%=' */
#define T_ANDEQ         289     /* '&=' */
#define T_OREQ          290     /* '|=' */
#define T_XOREQ         291     /* '^=' */
#define T_SHLEQ         292     /* '<<=' */
#define T_SHREQ         293     /* '>>=' */
#define T_DEFINE        294
#define T_SUPER         295
#define T_NEW           296
#define T_DOTDOT        297
#define T_TRY           298
#define T_CATCH         299
#define T_FINALLY       300
#define T_THROW         301
#define _TMAX           301

/* argument structure */
typedef struct argument ARGUMENT;
struct argument {
    char *arg_name;             /* argument name */
    struct argument *arg_next;  /* next argument */
};

/* argument table structure */
typedef struct atable ATABLE;
struct atable {
    ARGUMENT *at_arguments;     /* first argument */
    ARGUMENT **at_pNextArgument;/* pointer to where to store the next argument */
    struct atable *at_next;     /* next argument table */
};

/* break/continue stack entry structure */
typedef struct sentry SENTRY;
struct sentry {
    int level;                  /* block level */
    int label;                  /* label */
};

/* case entry structure */
typedef struct centry CENTRY;
struct centry {
    int value;
    int label;
    CENTRY *next;
};

/* switch entry structure */
typedef struct swentry SWENTRY;
struct swentry {
    int nCases;
    CENTRY *cases;
    int defaultLabel;
    int label;
};

/* try/catch/finally entry structure */
typedef struct TCENTRY TCENTRY;
struct TCENTRY {
    int handler;
    int start;
    int end;
    TCENTRY *next;
};

/* limits */
#define TKNSIZE         255     /* maximum BobToken size */
#define LSIZE           255     /* maximum line size */
#define SSIZE           20      /* break/continue/switch stack size */

/* line number entry */
typedef struct LineNumberEntry LineNumberEntry;
struct LineNumberEntry {
    int line;
    int pc;
};

/* line number block */
#define kLineNumberBlockSize    128
typedef struct LineNumberBlock LineNumberBlock;
struct LineNumberBlock {
    LineNumberBlock *next;
    int count;
    LineNumberEntry entries[kLineNumberBlockSize];
};

/* compiler context structure */
struct BobCompiler {
    BobInterpreter *ic;                 /* compiler - interpreter context */
    BobStream *input;                   /* compiler - input stream */
    int blockLevel;                     /* compiler - nesting level */
    ATABLE *arguments;                  /* compiler - argument frames */
    TCENTRY *exceptions;                /* compiler - exceptions */
    SENTRY bstack[SSIZE],*bsp,*bsbase;  /* compiler - break stack */
    SENTRY cstack[SSIZE],*csp,*csbase;  /* compiler - continue stack */
    SWENTRY sstack[SSIZE],*ssp,*ssbase; /* compiler - switch stack */
    unsigned char *codebuf;             /* compiler - code buffer */
    unsigned char *cbase,*cptr,*ctop;   /* compiler - code buffer positions */
    BobValue literalbuf;                /* compiler - literal buffer */
    long lbase,lptr,ltop;               /* compiler - literal buffer positions */
    int emitLineNumbersP;               /* compiler - true to emit line number opcodes */
    LineNumberBlock *lineNumbers;       /* compiler - line number table entries */
    LineNumberBlock *currentBlock;      /* compiler - where to store new line numbers */
    BobIntegerType t_value;             /* scanner - integer value */
    BobFloatType t_fvalue;              /* scanner - float value */
    char t_token[TKNSIZE+1];            /* scanner - token string */
    int lineNumberChangedP;             /* scanner - line number has changed */
    int lineNumber;                     /* scanner - line number */
    int savedToken;                     /* scanner - look ahead BobToken */
    int savedChar;                      /* scanner - look ahead character */
    char line[LSIZE+1];                 /* scanner - last input line */
    char *linePtr;                      /* scanner - line pointer */
    int atEOF;                          /* scanner - input end of file flag */
};

/* prototypes for scanner.c */
int BobToken(BobCompiler *c);
void BobSaveToken(BobCompiler *c,int tkn);
char *BobTokenName(int tkn);
void BobParseError(BobCompiler *c,char *msg);

#endif
