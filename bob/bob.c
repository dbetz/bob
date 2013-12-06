/* bob.c - the main routine */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "bob.h"
#include "bobcom.h"

#if 0
#define INTERPRETER_SIZE    (1024 * 1024)
#define COMPILER_SIZE       (1024 * 1024)
#define STACK_SIZE          (64 * 1025)
#else
#define INTERPRETER_SIZE    (20 * 1024)
#define COMPILER_SIZE       (8 * 1024)
#define STACK_SIZE          (2 * 1024)
#endif

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
static char compilerSpace[COMPILER_SIZE];

/* prototypes */
static void ErrorHandler(BobInterpreter *c,int code,va_list ap);
static void CompileFile(BobInterpreter *c,char *inputName,char *outputName);
static void LoadFile(BobInterpreter *c,char *name,int verboseP);
static void ReadEvalPrint(BobInterpreter *c);
static void Usage(void);

/* main - the main routine */
int main(int argc,char **argv)
{
	int interactiveP = TRUE;
    BobUnwindTarget target;
    BobInterpreter *c;
    
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

    /* use the math routines */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    BobUseMath(c);
#endif

    /* use the eval package */
    BobUseEval(c,compilerSpace,sizeof(compilerSpace));

    /* catch errors and restart read/eval/print loop */
    if (BobUnwindCatch(c) == 0) {
        char *inputName,*outputName = NULL;
        int verboseP = FALSE;
        int i;

        /* process arguments */
        for (i = 1; i < argc; ++i) {
            if (argv[i][0] == '-') {
                switch (argv[i][1]) {
			    case '?':
                    Usage();
                    break;
                case 'c':   /* compile source file */
                    if (argv[i][2])
                        inputName = &argv[i][2];
                    else if (++i < argc)
                        inputName = argv[i];
                    else
                        Usage();
                    CompileFile(c,inputName,outputName);
                    interactiveP = FALSE;
                    outputName = NULL;
                    break;
                case 'g':   /* emit debugging information when compiling */
                    c->compiler->emitLineNumbersP = TRUE;
                    break; 
                case 'i':   /* enter interactive mode after loading */
                    interactiveP = TRUE;
                    break;
                case 'o':   /* specify output filename when compiling */
                    if (argv[i][2])
                        outputName = &argv[i][2];
                    else if (++i < argc)
                        outputName = argv[i];
                    else
                        Usage();
                    interactiveP = FALSE;
                    break;
                case 'v':   /* display values of expressions loaded */
                    verboseP = TRUE;
                    break;
                default:
                    Usage();
                    break;
                }
            }
            else {
                LoadFile(c,argv[i],verboseP);
                interactiveP = FALSE;
                verboseP = FALSE;
            }
        }
    }
    
    /* read/eval/print loop */
    if (interactiveP)
        ReadEvalPrint(c);
    
    /* pop the unwind target */
    BobPopUnwindTarget(c);

    /* return successfully */
    return 0;
}

/* CompileFile - compile a single file */
static void CompileFile(BobInterpreter *c,char *inputName,char *outputName)
{
    char iname[1024],oname[1024],*p;
    
    /* determine the input filename */
    if ((p = strrchr(inputName,'.')) == NULL) {
        strcpy(iname,inputName);
        strcat(iname,".bob");
        inputName = iname;
    }

    /* determine the output filename */
    if (outputName) {
        if ((p = strrchr(outputName,'.')) == NULL) {
            strcpy(oname,outputName);
            strcat(oname,".bbo");
            outputName = oname;
        }
    }

    /* construct an output filename */
    else {
        if ((p = strrchr(inputName,'.')) == NULL)
            strcpy(oname,inputName);
        else {
            int len = p - inputName;
            strncpy(oname,inputName,len);
            oname[len] = '\0';
        }
        strcat(oname,".bbo");
        outputName = oname;
    }

    /* compile the file */
    printf("Compiling '%s' -> '%s'\n",inputName,outputName);
    BobCompileFile(c,inputName,outputName);
}

/* LoadFile - load a single file */
static void LoadFile(BobInterpreter *c,char *name,int verboseP)
{
    BobStream *s = verboseP ? c->standardOutput : NULL;
    char *ext;

    /* default to .bob if the extension is missing */
    if ((ext = strrchr(name,'.')) == NULL) {
        char fullName[1024];
        sprintf(fullName,"%s.bob",name);
        BobLoadFile(c,fullName,s);
    }

    /* determine the file extension */
    else {
        if (strcmp(ext,".bob") == 0)
            BobLoadFile(c,name,s);
        else if (strcmp(ext,".bbo") == 0)
            BobLoadObjectFile(c,name,s);
        else {
            fprintf(stderr,"Unknown file extension '%s'\n",name);
            exit(1);
        }
    }
}

/* ReadEvalPrint - enter a read/eval/print loop */
static void ReadEvalPrint(BobInterpreter *c)
{
    char lineBuffer[256];
    BobValue val;

    /* protect a pointer from the garbage collector */
    BobProtectPointer(c,&val);
    
    for (;;) {
        printf("\n> "); fflush(stdout);
        if (fgets(lineBuffer,sizeof(lineBuffer),stdin)) {
            val = BobEvalString(c,lineBuffer);
            if (val) {
                printf("--> ");
                BobPrint(c,val,c->standardOutput);
                printf("\n");
            }
        }
        else
            break;
    }
}

/* ErrorHandler - error handler callback */
void ErrorHandler(BobInterpreter *c,int code,va_list ap)
{
    switch (code) {
    case BobErrExit:
        BobFreeInterpreter(c);
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

/* Usage - display a usage message and exit */
static void Usage(void)
{
    fprintf(stderr,"\
usage: bob [-c file]     compile a source file\n\
           [-g]          include debugging information\n\
           [-i]          enter interactive mode after loading\n\
           [-o file]     object file name for compile\n\
           [-v]          enable verbose mode\n\
           [-?]          display (this) help information\n\
           [file]        load a source or object file\n");
    exit(1);
}

