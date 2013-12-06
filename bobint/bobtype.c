/* bobtype.c - derived types */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "bob.h"

/* TYPE */

static BobValue TypeNewInstance(BobInterpreter *c,BobValue parent)
{
    BobDispatch *d = (BobDispatch *)BobCObjectValue(parent);
    return (*d->newInstance)(c,parent);
}

static int TypePrint(BobInterpreter *c,BobValue val,BobStream *s)
{
    BobDispatch *d = BobTypeDispatch(val);
    char *p = d->typeName;
    BobStreamPutS("<Type-",s);
    while (*p)
        if (BobStreamPutC(*p++,s) == BobStreamEOF)
            return BobStreamEOF;
    BobStreamPutC('>',s);
    return 0;
}

/* BobInitTypes - initialize derived types */
void BobInitTypes(BobInterpreter *c)
{
    /* make all of the derived types */
    if (!(c->typeDispatch = BobMakeDispatch(c,"Type",&BobCObjectDispatch)))
        BobInsufficientMemory(c);

    /* initialize the 'Type' type */
    c->typeDispatch->dataSize = sizeof(BobCPtrObject) - sizeof(BobCObject);
    c->typeDispatch->object = BobEnterType(c,"Type",c->typeDispatch);
    c->typeDispatch->newInstance = TypeNewInstance;
    c->typeDispatch->print = TypePrint;

    /* fixup the 'Type' class */
    BobSetObjectClass(c->typeDispatch->object,c->typeDispatch->object);
}

/* BobAddTypeSymbols - add symbols for the built-in types */
void BobAddTypeSymbols(BobInterpreter *c)
{
    BobEnterType(c,"CObject",         &BobCObjectDispatch);
    BobEnterType(c,"Symbol",          &BobSymbolDispatch);
    BobEnterType(c,"CMethod",         &BobCMethodDispatch);
    BobEnterType(c,"Property",        &BobPropertyDispatch);
    BobEnterType(c,"CompiledCode",    &BobCompiledCodeDispatch);
    BobEnterType(c,"Environment",     &BobEnvironmentDispatch);
    BobEnterType(c,"StackEnvironment",&BobStackEnvironmentDispatch);
    BobEnterType(c,"MovedEnvironment",&BobMovedEnvironmentDispatch);
}

/* BobEnterType - enter a type */
BobValue BobEnterType(BobInterpreter *c,char *name,BobDispatch *d)
{
	BobCPush(c,BobMakeCPtrObject(c,c->typeDispatch,d));
    BobSetGlobalValue(BobInternCString(c,name),BobTop(c));
    return BobPop(c);
}
