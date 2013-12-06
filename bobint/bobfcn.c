/* bobfcn.c - built-in functions */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bob.h"

/* prototypes */
static BobValue BIF_Type(BobInterpreter *c);
static BobValue BIF_Hash(BobInterpreter *c);
static BobValue BIF_toString(BobInterpreter *c);
static BobValue BIF_rand(BobInterpreter *c);
static BobValue BIF_gc(BobInterpreter *c);
static BobValue BIF_LoadObjectFile(BobInterpreter *c);
static BobValue BIF_Quit(BobInterpreter *c);

/* function table */
static BobCMethod functionTable[] = {
BobMethodEntry( "Type",             BIF_Type            ),
BobMethodEntry( "Hash",             BIF_Hash            ),
BobMethodEntry( "toString",         BIF_toString        ),
BobMethodEntry( "rand",             BIF_rand            ),
BobMethodEntry( "gc",               BIF_gc              ),
BobMethodEntry( "LoadObjectFile",   BIF_LoadObjectFile  ),
BobMethodEntry( "Quit",             BIF_Quit            ),
BobMethodEntry( 0,					0					)
};

/* BobEnterLibrarySymbols - enter the built-in functions and symbols */
void BobEnterLibrarySymbols(BobInterpreter *c)
{
    BobEnterFunctions(c,functionTable);
}

/* BIF_Type - built-in function 'Type' */
static BobValue BIF_Type(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    return BobInternCString(c,BobGetDispatch(BobGetArg(c,3))->typeName);
}

/* BIF_Hash - built-in function 'Hash' */
static BobValue BIF_Hash(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    return BobMakeInteger(c,BobHashValue(BobGetArg(c,3)));
}

/* BIF_toString - built-in function 'toString' */
static BobValue BIF_toString(BobInterpreter *c)
{
    int width,maxWidth,rightP,ch;
    BobStringOutputStream s;
    unsigned char buf[1024],*start;

    /* check the argument count */
    BobCheckArgRange(c,3,5);

    /* get the field width and fill character */
    switch (BobArgCnt(c)) {
    case 3:
        width = 0;
        ch = ' ';
        break;
    case 4:
        BobCheckType(c,4,BobIntegerP);
        width = (int)BobIntegerValue(BobGetArg(c,4));
        ch = ' ';
        break;
    case 5:
        BobCheckType(c,4,BobIntegerP);
        BobCheckType(c,5,BobIntegerP);
        width = (int)BobIntegerValue(BobGetArg(c,4));
        ch = (int)BobIntegerValue(BobGetArg(c,5));
        break;
    default:
        /* never reached */
        width = 0;
        ch = 0;
        break;
    }

    /* check for right fill */
    if (width <= 0) {
        width = -width;
        rightP = TRUE;
        maxWidth = sizeof(buf);
        start = buf;
    }
    else {
        rightP = FALSE;
        maxWidth = sizeof(buf) / 2;
        start = buf + maxWidth;
    }

    /* make sure the width is within range */
    if (width > maxWidth)
        width = maxWidth;

    /* print the value to the buffer */
    BobInitStringOutputStream(c,&s,start,maxWidth);
    BobDisplay(c,BobGetArg(c,3),(BobStream *)&s);

    /* fill if necessary */
    if (width > 0 && s.len < width) {
        int fill = width - s.len;
        if (rightP) {
            while (--fill >= 0)
                *s.ptr++ = ch;
        }
        else {
            while (--fill >= 0)
                *--start = ch;
        }
        s.len = width;
    }

    /* return the resulting string */
    return BobMakeString(c,start,s.len);
}

/* BIF_rand - built-in function 'rand' */
static BobValue BIF_rand(BobInterpreter *c)
{
    static long rseed = 1;
    long k1,i;

    /* parse the arguments */
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobIntegerP);
    i = (long)BobIntegerValue(BobGetArg(c,3));

    /* make sure we don't get stuck at zero */
    if (rseed == 0) rseed = 1;

    /* algorithm taken from Dr. Dobbs Journal, November 1985, page 91 */
    k1 = rseed / 127773L;
    if ((rseed = 16807L * (rseed - k1 * 127773L) - k1 * 2836L) < 0L)
        rseed += 2147483647L;

    /* return a random number between 0 and n-1 */
    return BobMakeInteger(c,(BobIntegerType)(rseed % i));
}

/* BIF_gc - built-in function 'gc' */
static BobValue BIF_gc(BobInterpreter *c)
{
    BobCheckArgCnt(c,2);
    BobCollectGarbage(c);
    return c->nilValue;
}

/* BIF_LoadObjectFile - built-in function 'LoadObjectFile' */
static BobValue BIF_LoadObjectFile(BobInterpreter *c)
{
    BobStream *s = NULL;
    char *name;
    BobParseArguments(c,"**S|P?=",&name,&s,BobFileDispatch);
    return BobLoadObjectFile(c,name,s) ? c->trueValue : c->falseValue;
}

/* BIF_Quit - built-in function 'Quit' */
static BobValue BIF_Quit(BobInterpreter *c)
{
    BobCheckArgCnt(c,2);
    BobCallErrorHandler(c,BobErrExit,0);
    return 0; /* never reached */
}
