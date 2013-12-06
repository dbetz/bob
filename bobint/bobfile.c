/* bobfile.c - 'File' handler */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include "bob.h"

/* 'File' dispatch */
BobDispatch *BobFileDispatch = NULL;

/* method handlers */
static BobValue BIF_initialize(BobInterpreter *c);
static BobValue BIF_Close(BobInterpreter *c);
static BobValue BIF_Print(BobInterpreter *c);
static BobValue BIF_Display(BobInterpreter *c);
static BobValue BIF_GetC(BobInterpreter *c);
static BobValue BIF_PutC(BobInterpreter *c);

/* file methods */
static BobCMethod methods[] = {
BobMethodEntry( "initialize",       BIF_initialize      ),
BobMethodEntry( "Close",            BIF_Close           ),
BobMethodEntry( "Print",            BIF_Print           ),
BobMethodEntry( "Display",          BIF_Display         ),
BobMethodEntry( "GetC",             BIF_GetC            ),
BobMethodEntry( "PutC",             BIF_PutC            ),
BobMethodEntry(	0,                  0                   )
};

/* file properties */
static BobVPMethod properties[] = {
BobVPMethodEntry( 0,                0,					0					)
};

/* prototypes */
static void EnterPort(BobInterpreter *c,char *name,BobStream **pStream);
static void DestroyFile(BobInterpreter *c,BobValue obj);

/* BobInitFile - initialize the 'File' object */
void BobInitFile(BobInterpreter *c)
{
    /* create the 'File' type */
    if (!(BobFileDispatch = BobEnterCPtrObjectType(c,NULL,"File",methods,properties)))
        BobInsufficientMemory(c);

    /* setup alternate handlers */
    BobFileDispatch->destroy = DestroyFile;

    /* enter the built-in ports */
    EnterPort(c,"stdin",&c->standardInput);
    EnterPort(c,"stdout",&c->standardOutput);
    EnterPort(c,"stderr",&c->standardError);
}

/* EnterPort - add a built-in port to the symbol table */
static void EnterPort(BobInterpreter *c,char *name,BobStream **pStream)
{
	BobStream *s;
	if (!(s = BobMakeIndirectStream(c,pStream)))
        BobInsufficientMemory(c);
    BobCPush(c,BobMakeFile(c,s));
    BobSetGlobalValue(BobInternCString(c,name),BobTop(c));
    BobDrop(c,1);
}

/* BobMakeFile - make a 'File' object */
BobValue BobMakeFile(BobInterpreter *c,BobStream *s)
{
    return BobMakeCPtrObject(c,BobFileDispatch,s);
}

/* BIF_initialize - built-in method 'initialize' */
static BobValue BIF_initialize(BobInterpreter *c)
{
    char *fname,*mode;
    BobStream *s;
    BobValue val;
    BobParseArguments(c,"V=*SS",&val,BobFileDispatch,&fname,&mode);
    if (!(s = BobOpenFileStream(c,fname,mode)))
        return c->nilValue;
    BobSetCObjectValue(val,s);
    return val;
}

/* DestroyFile - destroy a file object */
static void DestroyFile(BobInterpreter *c,BobValue obj)
{
    BobStream *s = (BobStream *)BobCObjectValue(obj);
    if (s) BobCloseStream(s);
}

/* BIF_Close - built-in method 'Close' */
static BobValue BIF_Close(BobInterpreter *c)
{
    BobValue val;
    BobStream *s;
    int sts;
    BobParseArguments(c,"V=*",&val,BobFileDispatch);
    s = (BobStream *)BobCObjectValue(val);
    if (!s) return c->falseValue;
    sts = BobCloseStream(s);
    BobSetCObjectValue(val,0);
    return sts == 0 ? c->trueValue : c->falseValue;
}

/* BIF_Print - built-in function 'Print' */
static BobValue BIF_Print(BobInterpreter *c)
{
    BobIntegerType i;
    BobStream *s;
    BobCheckArgMin(c,2);
    BobCheckType(c,1,BobFileP);
    if (!(s = BobFileStream(BobGetArg(c,1))))
        return c->nilValue;
    for (i = 3; i <= BobArgCnt(c); ++i)
        BobPrint(c,BobGetArg(c,i),s);
    return c->trueValue;
}

/* BIF_Display - built-in function 'Display' */
static BobValue BIF_Display(BobInterpreter *c)
{
    BobIntegerType i;
    BobStream *s;
    BobCheckArgMin(c,2);
    BobCheckType(c,1,BobFileP);
    if (!(s = BobFileStream(BobGetArg(c,1))))
        return c->nilValue;
    for (i = 3; i <= BobArgCnt(c); ++i)
        BobDisplay(c,BobGetArg(c,i),s);
    return c->trueValue;
}

/* BIF_GetC - built-in method 'GetC' */
static BobValue BIF_GetC(BobInterpreter *c)
{
    BobStream *s;
    int ch;
    BobParseArguments(c,"P=*",&s,BobFileDispatch);
    if (!s) return c->nilValue;
    if ((ch = BobStreamGetC(s)) == BobStreamEOF)
        return c->nilValue;
    return BobMakeSmallInteger(ch);
}

/* BIF_PutC - built-in method 'PutC' */
static BobValue BIF_PutC(BobInterpreter *c)
{
    BobStream *s;
    int ch;
    BobParseArguments(c,"P=*i",&s,BobFileDispatch,&ch);
    if (!s) return c->falseValue;
    return BobToBoolean(c,BobStreamPutC(ch,s) != BobStreamEOF);
}
