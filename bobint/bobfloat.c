/* bobfloat.c - 'Float' handler */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#ifdef BOB_INCLUDE_FLOAT_SUPPORT

#include <string.h>
#include "bob.h"

/* method handlers */
static BobValue BIF_toFloat(BobInterpreter *c);
static BobValue BIF_toInteger(BobInterpreter *c);

/* Float methods */
static BobCMethod methods[] = {
BobMethodEntry( "toFloat",          BIF_toFloat         ),
BobMethodEntry( "toInteger",        BIF_toInteger       ),
BobMethodEntry(	0,                  0                   )
};

/* Float properties */
static BobVPMethod properties[] = {
BobVPMethodEntry( 0,                0,					0					)
};

/* BobInitFloat - initialize the 'Float' object */
void BobInitFloat(BobInterpreter *c)
{
    c->floatObject = BobEnterType(c,"Float",&BobFloatDispatch);
    BobEnterMethods(c,c->floatObject,methods);
    BobEnterVPMethods(c,c->floatObject,properties);
}

/* BIF_toFloat - built-in method 'toFloat' */
static BobValue BIF_toFloat(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobFloatDispatch);
    return obj;
}

/* BIF_toInteger - built-in method 'toInteger' */
static BobValue BIF_toInteger(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobFloatDispatch);
    return BobMakeInteger(c,(BobIntegerType)BobFloatValue(obj));
}

static int GetFloatProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
static int SetFloatProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
static int FloatPrint(BobInterpreter *c,BobValue obj,BobStream *s);
static long FloatSize(BobValue obj);

BobDispatch BobFloatDispatch = {
    "Float",
    &BobFloatDispatch,
    GetFloatProperty,
    SetFloatProperty,
    BobDefaultNewInstance,
    FloatPrint,
    FloatSize,
    BobDefaultCopy,
    BobDefaultScan,
    BobDefaultHash
};

/* GetFloatProperty - Float get property handler */
static int GetFloatProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    return BobGetVirtualProperty(c,obj,c->floatObject,tag,pValue);
}

/* SetFloatProperty - Float set property handler */
static int SetFloatProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
    return BobSetVirtualProperty(c,obj,c->floatObject,tag,value);
}

/* FloatPrint - Float print handler */
static int FloatPrint(BobInterpreter *c,BobValue obj,BobStream *s)
{
    char buf[64];
    BobFloatToString(buf,BobFloatValue(obj));
    if (!strchr(buf,'.'))
        strcat(buf,".0");
    return BobStreamPutS(buf,s);
}

/* FloatSize - Float size handler */
static long FloatSize(BobValue obj)
{
    return sizeof(BobFloat);
}

/* BobMakeFloat - make a new float value */
BobValue BobMakeFloat(BobInterpreter *c,BobFloatType value)
{
    BobValue new = BobAllocate(c,sizeof(BobFloat));
    BobSetDispatch(new,&BobFloatDispatch);
    BobSetFloatValue(new,value);
    return new;
}

#endif