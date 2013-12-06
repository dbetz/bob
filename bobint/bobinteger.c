/* bobinteger.c - 'Integer' handler */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include "bob.h"

/* method handlers */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static BobValue BIF_toFloat(BobInterpreter *c);
#endif
static BobValue BIF_toInteger(BobInterpreter *c);
static BobValue BIF_toString(BobInterpreter *c);

/* Integer methods */
static BobCMethod methods[] = {
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
BobMethodEntry( "toFloat",          BIF_toFloat         ),
#endif
BobMethodEntry( "toInteger",        BIF_toInteger       ),
BobMethodEntry( "toString",         BIF_toString        ),
BobMethodEntry(	0,                  0                   )
};

/* Integer properties */
static BobVPMethod properties[] = {
BobVPMethodEntry( 0,                0,					0					)
};

/* BobInitInteger - initialize the 'Integer' object */
void BobInitInteger(BobInterpreter *c)
{
    c->integerObject = BobEnterType(c,"Integer",&BobIntegerDispatch);
    BobEnterMethods(c,c->integerObject,methods);
    BobEnterVPMethods(c,c->integerObject,properties);
}

/* BIF_toFloat - built-in method 'toFloat' */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static BobValue BIF_toFloat(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobIntegerDispatch);
    return BobMakeFloat(c,(BobFloatType)BobIntegerValue(obj));
}
#endif

/* BIF_toInteger - built-in method 'toInteger' */
static BobValue BIF_toInteger(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobIntegerDispatch);
    return obj;
}

/* BIF_toString - built-in method 'toString' */
static BobValue BIF_toString(BobInterpreter *c)
{
    int radix = 10;
    char buf[100],*fmt;
    BobValue obj;
    BobParseArguments(c,"V=*|i",&obj,&BobIntegerDispatch,&radix);
    switch (radix) {
    case 8:
        fmt = "%lo";
        break;
    case 10:
        fmt = "%ld";
        break;
    case 16:
        fmt = "%lx";
        break;
    default:
        return c->nilValue;
    }
    sprintf(buf,fmt,(int)BobIntegerValue(obj));
    return BobMakeCString(c,buf);
}

static int GetIntegerProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
static int SetIntegerProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
static int IntegerPrint(BobInterpreter *c,BobValue obj,BobStream *s);
static long IntegerSize(BobValue obj);
static BobValue IntegerCopy(BobInterpreter *c,BobValue obj);
static BobIntegerType IntegerHash(BobValue obj);

BobDispatch BobIntegerDispatch = {
    "Integer",
    &BobIntegerDispatch,
    GetIntegerProperty,
    SetIntegerProperty,
    BobDefaultNewInstance,
    IntegerPrint,
    IntegerSize,
    IntegerCopy,
    BobDefaultScan,
    IntegerHash
};

/* GetIntegerProperty - Integer get property handler */
static int GetIntegerProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    return BobGetVirtualProperty(c,obj,c->integerObject,tag,pValue);
}

/* SetIntegerProperty - Integer set property handler */
static int SetIntegerProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
    return BobSetVirtualProperty(c,obj,c->integerObject,tag,value);
}

/* IntegerPrint - Integer print handler */
static int IntegerPrint(BobInterpreter *c,BobValue obj,BobStream *s)
{
    char buf[32];
    BobIntegerToString(buf,BobIntegerValue(obj));
    return BobStreamPutS(buf,s);
}

/* IntegerSize - Integer size handler */
static long IntegerSize(BobValue obj)
{
    return sizeof(BobInteger);
}

/* IntegerCopy - Integer copy handler */
static BobValue IntegerCopy(BobInterpreter *c,BobValue obj)
{
    return BobPointerP(obj) ? BobDefaultCopy(c,obj) : obj;
}

/* IntegerHash - Integer hash handler */
static BobIntegerType IntegerHash(BobValue obj)
{
    return BobIntegerValue(obj);
}

/* BobMakeInteger - make a new integer value */
BobValue BobMakeInteger(BobInterpreter *c,BobIntegerType value)
{
    if (BobSmallIntegerValueP(value))
        return BobMakeSmallInteger(value);
    else {
        BobValue new = BobAllocate(c,sizeof(BobInteger));
        BobSetDispatch(new,&BobIntegerDispatch);
        BobSetHeapIntegerValue(new,value);
        return new;
    }
}

