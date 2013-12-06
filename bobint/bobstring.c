/* bobstring.c - 'String' handler */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "bob.h"

/* method handlers */
static BobValue BIF_initialize(BobInterpreter *c);
static BobValue BIF_Intern(BobInterpreter *c);
static BobValue BIF_Index(BobInterpreter *c);
static BobValue BIF_ReverseIndex(BobInterpreter *c);
static BobValue BIF_Substring(BobInterpreter *c);
static BobValue BIF_toInteger(BobInterpreter *c);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static BobValue BIF_toFloat(BobInterpreter *c);
#endif

/* virtual property methods */
static BobValue BIF_size(BobInterpreter *c,BobValue obj);

/* String methods */
static BobCMethod methods[] = {
BobMethodEntry( "initialize",       BIF_initialize      ),
BobMethodEntry( "Intern",           BIF_Intern          ),
BobMethodEntry( "Index",            BIF_Index           ),
BobMethodEntry( "ReverseIndex",     BIF_ReverseIndex    ),
BobMethodEntry( "Substring",        BIF_Substring       ),
BobMethodEntry( "toInteger",        BIF_toInteger       ),
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
BobMethodEntry( "toFloat",          BIF_toFloat         ),
#endif
BobMethodEntry(	0,                  0                   )
};

/* String properties */
static BobVPMethod properties[] = {
BobVPMethodEntry( "size",           BIF_size,           0                   ),
BobVPMethodEntry( 0,                0,					0					)
};

/* BobInitString - initialize the 'String' object */
void BobInitString(BobInterpreter *c)
{
    c->stringObject = BobEnterType(c,"String",&BobStringDispatch);
    BobEnterMethods(c,c->stringObject,methods);
    BobEnterVPMethods(c,c->stringObject,properties);
}

/* BIF_initialize - built-in method 'initialize' */
static BobValue BIF_initialize(BobInterpreter *c)
{
    long size = 0;
    BobValue obj;
    BobParseArguments(c,"V=*|i",&obj,&BobStringDispatch,&size);
    return BobMakeString(c,NULL,size);
}

/* BIF_Intern - built-in method 'Intern' */
static BobValue BIF_Intern(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobStringDispatch);
    return BobIntern(c,obj);
}

/* BIF_Index - built-in method 'Index' */
static BobValue BIF_Index(BobInterpreter *c)
{
    char *str,*p;
    int len,ch;
    
    /* parse the arguments */
    BobParseArguments(c,"S#*i",&str,&len,&ch);

    /* find the character */
    if (!(p = strchr(str,ch)))
        return c->nilValue;
    
    /* return the index of the character */
    return BobMakeInteger(c,p - str);
}

/* BIF_ReverseIndex - built-in method 'ReverseIndex' */
static BobValue BIF_ReverseIndex(BobInterpreter *c)
{
    char *str,*p;
    int len,ch;
    
    /* parse the arguments */
    BobParseArguments(c,"S#*i",&str,&len,&ch);

    /* find the character */
    if (!(p = strrchr(str,ch)))
        return c->nilValue;
    
    /* return the index of the character */
    return BobMakeInteger(c,p - str);
}

/* BIF_Substring - built-in method 'Substring' */
static BobValue BIF_Substring(BobInterpreter *c)
{
    int len,i,cnt = -1;
    char *str;
    
    /* parse the arguments */
    BobParseArguments(c,"S#*i|i",&str,&len,&i,&cnt);

    /* handle indexing from the left */
    if (i > 0) {
        if (i > len)
            return c->nilValue;
    }
    
    /* handle indexing from the right */
    else if (i < 0) {
        if ((i = len + i) < 0)
            return c->nilValue;
    }

    /* handle the count */
    if (cnt < 0)
        cnt = len - i;
    else if (i + cnt > len)
        return c->nilValue;
    
    /* return the substring */
    return BobMakeString(c,(unsigned char *)(str + i),cnt);
}

/* BIF_toInteger - built-in method 'toInteger' */
static BobValue BIF_toInteger(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobStringDispatch);
    return BobMakeInteger(c,atoi((char *)BobStringAddress(obj)));
}

/* BIF_toFloat - built-in method 'toFloat' */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static BobValue BIF_toFloat(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobStringDispatch);
    return BobMakeFloat(c,atof((char *)BobStringAddress(obj)));
}
#endif

/* BIF_size - built-in property 'size' */
static BobValue BIF_size(BobInterpreter *c,BobValue obj)
{
    return BobMakeInteger(c,BobStringSize(obj));
}

/* String handlers */
static int GetStringProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
static int SetStringProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
static BobValue StringNewInstance(BobInterpreter *c,BobValue parent);
static int StringPrint(BobInterpreter *c,BobValue val,BobStream *s);
static long StringSize(BobValue obj);
static BobIntegerType StringHash(BobValue obj);

/* String dispatch */
BobDispatch BobStringDispatch = {
    "String",
    &BobStringDispatch,
    GetStringProperty,
    SetStringProperty,
    StringNewInstance,
    StringPrint,
    StringSize,
    BobDefaultCopy,
    BobDefaultScan,
    StringHash
};

/* GetStringProperty - String get property handler */
static int GetStringProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    if (BobIntegerP(tag)) {
        BobIntegerType i;
        if ((i = BobIntegerValue(tag)) < 0 || i >= BobStringSize(obj))
            BobCallErrorHandler(c,BobErrIndexOutOfBounds,tag);
        *pValue = BobMakeSmallInteger(BobStringElement(obj,i));
        return TRUE;
    }
    return BobGetVirtualProperty(c,obj,c->stringObject,tag,pValue);
}

/* SetStringProperty - String set property handler */
static int SetStringProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
    if (BobIntegerP(tag)) {
        BobIntegerType i;
        if (!BobIntegerP(value))
            BobTypeError(c,value);
        if ((i = BobIntegerValue(tag)) < 0 || i >= BobStringSize(obj))
            BobCallErrorHandler(c,BobErrIndexOutOfBounds,tag);
        BobSetStringElement(obj,i,(int)BobIntegerValue(value));
        return TRUE;
    }
    return BobSetVirtualProperty(c,obj,c->stringObject,tag,value);
}

/* StringNewInstance - create a new string */
static BobValue StringNewInstance(BobInterpreter *c,BobValue parent)
{
    return BobMakeString(c,NULL,0);
}

/* StringPrint - String print handler */
static int StringPrint(BobInterpreter *c,BobValue val,BobStream *s)
{
    unsigned char *p = BobStringAddress(val);
    long size = BobStringSize(val);
    if (BobStreamPutC('"',s) == BobStreamEOF)
        return BobStreamEOF;
    while (--size >= 0)
        if (BobStreamPutC(*p++,s) == BobStreamEOF)
            return BobStreamEOF;
    return BobStreamPutC('"',s) == BobStreamEOF ? BobStreamEOF : 0;
}

/* StringSize - String size handler */
static long StringSize(BobValue obj)
{
    return sizeof(BobString) + BobRoundSize(BobStringSize(obj) + 1);
}

/* StringHash - String hash handler */
static BobIntegerType StringHash(BobValue obj)
{
    return BobHashString(BobStringAddress(obj),BobStringSize(obj));
}

/* BobMakeString - make and initialize a new string value */
BobValue BobMakeString(BobInterpreter *c,unsigned char *data,BobIntegerType size)
{
    long allocSize = sizeof(BobString) + BobRoundSize(size + 1); /* space for zero terminator */
    BobValue new = BobAllocate(c,allocSize);
    unsigned char *p = BobStringAddress(new);
    BobSetDispatch(new,&BobStringDispatch);
    BobSetStringSize(new,size);
    if (data)
        memcpy(p,data,size);
    else
        memset(p,0,size);
    p[size] = '\0'; /* in case we need to use it as a C string */
    return new;
}

/* BobMakeCString - make a string value from a C string */
BobValue BobMakeCString(BobInterpreter *c,char *str)
{
    return BobMakeString(c,(unsigned char *)str,(BobIntegerType)strlen(str));
}
