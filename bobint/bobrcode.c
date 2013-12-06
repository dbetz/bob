/* bobrcode.c - object code loader functions */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include "bob.h"

/* prototypes */
static int ReadMethod(BobInterpreter *c,BobValue *pMethod,BobStream *s);
static int ReadValue(BobInterpreter *c,BobValue *pv,BobStream *s);
static int ReadCodeValue(BobInterpreter *c,BobValue *pv,BobStream *s);
static int ReadVectorValue(BobInterpreter *c,BobValue *pv,BobStream *s);
static int ReadObjectValue(BobInterpreter *c,BobValue *pv,BobStream *s);
static int ReadSymbolValue(BobInterpreter *c,BobValue *pv,BobStream *s);
static int ReadStringValue(BobInterpreter *c,BobValue *pv,BobStream *s);
static int ReadIntegerValue(BobInterpreter *c,BobValue *pv,BobStream *s);
static int ReadInteger(BobIntegerType *pn,BobStream *s);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static int ReadFloatValue(BobInterpreter *c,BobValue *pv,BobStream *s);
static int ReadFloat(BobFloatType *pn,BobStream *s);
#endif

/* BobLoadObjectFile - load an object file */
int BobLoadObjectFile(BobInterpreter *c,char *fname,BobStream *os)
{
    BobUnwindTarget target;
    BobIntegerType version;
    BobValue method;
    BobStream *s;
    
    /* open the object file */
    if ((s = BobOpenFileStream(c,fname,"rb")) == NULL)
        BobCallErrorHandler(c,BobErrFileNotFound,fname);
    
    /* check the file type */
    if (BobStreamGetC(s) != 'B'
    ||  BobStreamGetC(s) != 'O'
    ||  BobStreamGetC(s) != 'B'
    ||  BobStreamGetC(s) != 'O') {
        BobCloseStream(s);
        BobCallErrorHandler(c,BobErrNotAnObjectFile,fname);
    }

    /* check the version number */
    if (!ReadInteger(&version,s) || version != BobFaslVersion) {
        BobCloseStream(s);
        BobCallErrorHandler(c,BobErrWrongObjectVersion,version);
    }

    /* announce the file */
    if (os) {
        BobStreamPutS("Loading '",os);
        BobStreamPutS(fname,os);
        BobStreamPutS("'\n",os);
    }

    /* setup the unwind target */
    BobPushUnwindTarget(c,&target);
    if (BobUnwindCatch(c) != 0) {
        BobCloseStream(s);
        BobPopUnwindTarget(c);
        return FALSE;
    }
    
    /* read and evaluate each expression (thunk) */
    while (ReadMethod(c,&method,s)) {
        BobValue val = BobCallFunction(c,method,0);
        if (os) {
            BobPrint(c,val,os);
            BobStreamPutC('\n',os);
        }
    }

    /* return successfully */
    BobPopUnwindTarget(c);
    BobCloseStream(s);
    return TRUE;
}

/* BobLoadObjectStream - load a stream of object code */
int BobLoadObjectStream(BobInterpreter *c,BobStream *s,BobStream *os)
{
    BobUnwindTarget target;
    BobValue method;
    
    /* setup the unwind target */
    BobPushUnwindTarget(c,&target);
    if (BobUnwindCatch(c) != 0) {
        BobPopUnwindTarget(c);
        return FALSE;
    }
        
    /* read and evaluate each expression (thunk) */
    while (ReadMethod(c,&method,s)) {
        BobValue val = BobCallFunction(c,method,0);
        if (os) {
            BobPrint(c,val,os);
            BobStreamPutC('\n',os);
        }
    }

    /* return successfully */
    BobPopUnwindTarget(c);
    return TRUE;
}

/* ReadMethod - read a method from a fasl file */
static int ReadMethod(BobInterpreter *c,BobValue *pMethod,BobStream *s)
{
    BobValue code;
    if (BobStreamGetC(s) != BobFaslTagCode
    || !ReadCodeValue(c,&code,s))
        return FALSE;
    *pMethod = BobMakeMethod(c,code,c->nilValue);
    return TRUE;
}

/* ReadValue - read a value */
static int ReadValue(BobInterpreter *c,BobValue *pv,BobStream *s)
{
    switch (BobStreamGetC(s)) {
    case BobFaslTagNil:
        *pv = c->nilValue;
        return TRUE;
    case BobFaslTagCode:
        return ReadCodeValue(c,pv,s);
    case BobFaslTagVector:
        return ReadVectorValue(c,pv,s);
    case BobFaslTagObject:
        return ReadObjectValue(c,pv,s);
    case BobFaslTagSymbol:
        return ReadSymbolValue(c,pv,s);
    case BobFaslTagString:
        return ReadStringValue(c,pv,s);
    case BobFaslTagInteger:
        return ReadIntegerValue(c,pv,s);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    case BobFaslTagFloat:
        return ReadFloatValue(c,pv,s);
#endif
    default:
        return FALSE;
    }
}

/* ReadCodeValue - read a code value */
static int ReadCodeValue(BobInterpreter *c,BobValue *pv,BobStream *s)
{
    BobIntegerType size,i;
    if (!ReadInteger(&size,s))
        return FALSE;
    BobCPush(c,BobMakeBasicVector(c,&BobCompiledCodeDispatch,size));
    for (i = 0; i < size; ++i) {
        BobValue v;
        if (!ReadValue(c,&v,s)) {
            BobDrop(c,1);
            return FALSE;
        }
        BobSetBasicVectorElement(BobTop(c),i,v);
    }
    *pv = BobPop(c);
    return TRUE;
}

/* ReadVectorValue - read a vector value */
static int ReadVectorValue(BobInterpreter *c,BobValue *pv,BobStream *s)
{
    BobIntegerType size,i;
    if (!ReadInteger(&size,s))
        return FALSE;
    BobCPush(c,BobMakeVector(c,size));
    for (i = 0; i < size; ++i) {
        BobValue v;
        if (!ReadValue(c,&v,s)) {
            BobDrop(c,1);
            return FALSE;
        }
        BobSetVectorElement(BobTop(c),i,v);
    }
    *pv = BobPop(c);
    return TRUE;
}

/* ReadObjectValue - read an object value */
static int ReadObjectValue(BobInterpreter *c,BobValue *pv,BobStream *s)
{
    BobValue classSymbol,class;
    BobIntegerType size;
    if (!ReadValue(c,&classSymbol,s)
    ||  !ReadInteger(&size,s))
        return FALSE;
    if (BobSymbolP(classSymbol))
        class = BobGlobalValue(classSymbol);
    else
        class = c->nilValue;
    BobCheck(c,2);
    BobPush(c,BobMakeObject(c,class));
    while (--size >= 0) {
        BobValue tag,value;
        if (!ReadValue(c,&tag,s)) {
            BobDrop(c,1);
            return FALSE;
        }
        BobPush(c,tag);
        if (!ReadValue(c,&value,s)) {
            BobDrop(c,2);
            return FALSE;
        }
        tag = BobPop(c);
        BobSetProperty(c,BobTop(c),tag,value);
    }
    *pv = BobPop(c);
    return TRUE;
}

/* ReadSymbolValue - read a symbol value */
static int ReadSymbolValue(BobInterpreter *c,BobValue *pv,BobStream *s)
{
    BobValue name;
    if (!ReadStringValue(c,&name,s))
        return FALSE;
    *pv = BobIntern(c,name);
    return TRUE;
}

/* ReadStringValue - read a string value */
static int ReadStringValue(BobInterpreter *c,BobValue *pv,BobStream *s)
{
    BobIntegerType size;
    unsigned char *p;
    if (!ReadInteger(&size,s))
        return FALSE;
    *pv = BobMakeString(c,NULL,size);
    for (p = BobStringAddress(*pv); --size >= 0; ) {
        int ch = BobStreamGetC(s);
        if (ch == BobStreamEOF)
            return FALSE;
        *p++ = ch;
    }
    return TRUE;
}

/* ReadIntegerValue - read an integer value */
static int ReadIntegerValue(BobInterpreter *c,BobValue *pv,BobStream *s)
{
    BobIntegerType n;
    if (!ReadInteger(&n,s))
        return FALSE;
    *pv = BobMakeInteger(c,n);
    return TRUE;
}

/* ReadFloatValue - read a float value */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static int ReadFloatValue(BobInterpreter *c,BobValue *pv,BobStream *s)
{
    BobFloatType n;
    if (!ReadFloat(&n,s))
        return FALSE;
    *pv = BobMakeFloat(c,n);
    return TRUE;
}
#endif

/* ReadInteger - read an integer value from an image file */
static int ReadInteger(BobIntegerType *pn,BobStream *s)
{
    int c;
    if ((c = BobStreamGetC(s)) == BobStreamEOF)
        return FALSE;
    *pn = (long)c << 24;
    if ((c = BobStreamGetC(s)) == BobStreamEOF)
        return FALSE;
    *pn |= (long)c << 16;
    if ((c = BobStreamGetC(s)) == BobStreamEOF)
        return FALSE;
    *pn |= (long)c << 8;
    if ((c = BobStreamGetC(s)) == BobStreamEOF)
        return FALSE;
    *pn |= (long)c;
    return TRUE;
}

/* ReadFloat - read a float value from an image file */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static int ReadFloat(BobFloatType *pn,BobStream *s)
{
    int count = sizeof(BobFloatType);
#ifdef BOB_REVERSE_FLOATS_ON_READ
    char *p = (char *)pn + sizeof(BobFloatType);
    int c;
    while (--count >= 0) {
        if ((c = BobStreamGetC(s)) == BobStreamEOF)
            return FALSE;
        *--p = c;
    }
#else
    char *p = (char *)pn;
    int c;
    while (--count >= 0) {
        if ((c = BobStreamGetC(s)) == BobStreamEOF)
            return FALSE;
        *p++ = c;
    }
#endif
    return TRUE;
}
#endif

