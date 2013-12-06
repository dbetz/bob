/* boberror.c - error strings */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdarg.h>
#include "bob.h"
#include "bobcom.h"

/* error output buffer size */
#define ERR_BUF_SIZE    100

/* error table entry */
typedef struct {
    int code;
    char *text;
} ErrString;

static ErrString errStrings[] = {
{       BobErrExit,                 "Exit"									},
{       BobErrInsufficientMemory,   "Insufficient memory"					},
{       BobErrStackOverflow,        "Stack overflow"						},
{       BobErrTooManyArguments,     "Too many arguments - %V"				},
{       BobErrTooFewArguments,      "Too few arguments - %V"				},
{       BobErrTypeError,            "Wrong type - %V"						},
{       BobErrUnboundVariable,      "Unbound variable - %V"				    },
{       BobErrIndexOutOfBounds,     "Index out of bounds - %V"				},
{       BobErrNoMethod,             "Object %V has no method - %V"          },
{       BobErrBadOpcode,            "Bad opcode - %b"						},
{       BobErrRestart,              "Restart"								},
{       BobErrWrite,                "Writing file"							},
{       BobErrBadParseCode,         "Bad parse code"						},
{       BobErrImpossible,           "Impossible error"						},
{       BobErrNoHashValue,          "Can't use as an index"					},
{       BobErrTooManyLiterals,      "Too many literals"						},
{       BobErrTooMuchCode,          "Too much code"							},
{       BobErrStoreIntoConstant,    "Attempt to store into a constant"		},
{       BobErrSyntaxError,          "Syntax error - %E"				        },
{		BobErrReadOnlyProperty,		"Attempt to set a read-only property"	},
{		BobErrWriteOnlyProperty,	"Attempt to get a write-only property"	},
{		BobErrFileNotFound,			"File not found - %s"					},
{       BobErrNewInstance,          "Can't create an instance"              },
{       BobErrNoProperty,           "Object %V has no property - %V"        },
{       BobErrStackEmpty,           "Stack empty - %V"                      },
{       BobErrNotAnObjectFile,      "Not an object file - %s"               },
{       BobErrWrongObjectVersion,   "Wrong object file version number - %i" },
{       BobErrValueError,           "Bad value - %V"                        },
{       0,                          0                                       }
};

/* BobCallErrorHandler - call the error handler */
void BobCallErrorHandler(BobInterpreter *c,int code,...)
{
    va_list ap;
    va_start(ap,code);
    (*c->errorHandler)(c,code,ap);
    va_end(ap);
}

/* BobShowError - show an error */
void BobShowError(BobInterpreter *c,int code,va_list ap)
{
    char buf[ERR_BUF_SIZE + 1],*fmt,*dst;
    int cnt,ch;

    /* get the error text format string */
    fmt = BobGetErrorText(code);

    /* initialize the output buffer */
    dst = buf;
    cnt = 0;

    /* parse the format string */
    BobStreamPutS("Error: ",c->standardError);
    while ((ch = *fmt++) != '\0')
        switch (ch) {
        case '%':
            if (*fmt != '\0') {
                if (cnt > 0) {
                    *dst = '\0';
                    BobStreamPutS(buf,c->standardError);
                    dst = buf;
                    cnt = 0;
                }
                switch (*fmt++) {
                case 'E':
                    BobStreamPutS(c->errorMessage,c->standardError);
                    sprintf(buf,"\n  in line %d\n",c->compiler->lineNumber);
                    BobStreamPutS(buf,c->standardError);
                    break;
                case 'V':
                    BobPrint(c,va_arg(ap,BobValue),c->standardError);
                    break;
                case 's':
                    BobStreamPutS(va_arg(ap,char *),c->standardError);
                    break;
                case 'i':
                    sprintf(buf,"%d",(int)va_arg(ap,BobIntegerType));
                    BobStreamPutS(buf,c->standardError);
                    break;
                case 'b':
                    sprintf(buf,"%02x",(int)va_arg(ap,BobIntegerType));
                    BobStreamPutS(buf,c->standardError);
                    break;
                }
                break;
            }
            /* fall through */
        default:
            *dst++ = ch;
            if (++cnt >= ERR_BUF_SIZE) {
                *dst = '\0';
                BobStreamPutS(buf,c->standardError);
                dst = buf;
                cnt = 0;
            }
        }
    if (cnt > 0) {
        *dst = '\0';
        BobStreamPutS(buf,c->standardError);
        dst = buf;
        cnt = 0;
    }
	BobStreamPutC('\n',c->standardError);
}

/* BobGetErrorText - get the text for an error code */
char *BobGetErrorText(int code)
{
    ErrString *p;
    for (p = errStrings; p->text != 0; ++p)
        if (code == p->code)
            return p->text;
    return "Unknown error";
}

