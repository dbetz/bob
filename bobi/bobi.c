/* bobi.c - the main routine */
/*
	Copyright (c) 2001, by David Michael Betz
	All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include "bob.h"

// #define HEAP_SIZE   (1024 * 1024)
// #define EXPAND_SIZE (512 * 1024)
#define STACK_SIZE  (64 * 1025)
#define INTERPRETER_SIZE    (1024 * 1024)

/* console stream structure */
typedef struct {
    BobStreamDispatch *d;
} ConsoleStream;

/* CloseConsoleStream - console stream close handler */
static int CloseConsoleStream(BobStream *s)
{
    return 0;
}

/* ConsoleStreamGetC - console stream getc handler */
static int ConsoleStreamGetC(BobStream *s)
{
    return getchar();
}

/* ConsoleStreamPutC - console stream putc handler */
static int ConsoleStreamPutC(int ch,BobStream *s)
{
    return putchar(ch);
}

/* dispatch structure for null streams */
BobStreamDispatch consoleDispatch = {
  CloseConsoleStream,
  ConsoleStreamGetC,
  ConsoleStreamPutC
};

/* console stream */
ConsoleStream consoleStream = { &consoleDispatch };

/* space for interpreter */
static char interpreterSpace[INTERPRETER_SIZE];

/* ErrorHandler - error handler callback */
void ErrorHandler(BobInterpreter *c,int code,va_list ap)
{
    switch (code) {
    case BobErrExit:
        exit(1);
    case BobErrRestart:
        /* just restart the restart the read/eval/print loop */
        break;
    default:
        BobShowError(c,code,ap);
        BobStackTrace(c);
    	break;
    }
    BobAbort(c);
}

/* main - the main routine */
int main(int argc,char **argv)
{
    BobUnwindTarget target;
    BobInterpreter *c;
    
    /* check the argument list */
    if (argc != 2) {
        fprintf(stderr,"usage: bobi <object-file>\n");
        exit(1);
    }

    /* make the workspace */
    if ((c = BobMakeInterpreter(interpreterSpace,sizeof(interpreterSpace),STACK_SIZE)) == NULL)
        exit(1);

    /* setup standard i/o */
    c->standardInput = (BobStream *)&consoleStream;
    c->standardOutput = (BobStream *)&consoleStream;
    c->standardError = (BobStream *)&consoleStream;
    
    /* setup the error handler */
    c->errorHandler = ErrorHandler;
    
    /* setup the error handler target */
    BobPushUnwindTarget(c,&target);

    /* abort if any errors occur during initialization */
    if (BobUnwindCatch(c))
        exit(1);

    /* initialize the workspace */
    if (!BobInitInterpreter(c))
        exit(1);

    /* use stdio for file i/o */
    BobUseStandardIO(c);
     
    /* add the library functions to the symbol table */
    BobEnterLibrarySymbols(c);

    /* load the command line argument */
    BobLoadObjectFile(c,argv[1],NULL);
    
    /* catch errors and restart read/eval/print loop */
    BobUnwindCatch(c);
    
    /* pop the unwind target */
    BobPopUnwindTarget(c);

    /* return successfully */
    return 0;
}

