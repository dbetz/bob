/* bobwcode.c - write a code value to a stream */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <string.h>
#include "bob.h"

/* prototypes */
static int WriteHeader(BobInterpreter *c,BobStream *s);
static int WriteMethod(BobInterpreter *c,BobValue method,BobStream *s);
static int WriteValue(BobInterpreter *c,BobValue v,BobStream *s);
static int WriteCodeValue(BobInterpreter *c,BobValue v,BobStream *s);
static int WriteVectorValue(BobInterpreter *c,BobValue v,BobStream *s);
static int WriteObjectValue(BobInterpreter *c,BobValue v,BobStream *s);
static int WriteSymbolValue(BobInterpreter *c,BobValue v,BobStream *s);
static int WriteStringValue(BobInterpreter *c,BobValue v,BobStream *s);
static int WriteIntegerValue(BobInterpreter *c,BobValue v,BobStream *s);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static int WriteFloatValue(BobInterpreter *c,BobValue v,BobStream *s);
#endif
static int WriteVector(BobInterpreter *c,BobValue v,BobStream *s);
static int WriteString(unsigned char *str,BobIntegerType size,BobStream *s);
static int WriteInteger(BobIntegerType n,BobStream *s);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static int WriteFloat(BobFloatType n,BobStream *s);
#endif

/* BobCompileFile - read and compile expressions from a file */
int BobCompileFile(BobInterpreter *c,char *iname,char *oname)
{
    BobUnwindTarget target;
    BobStream *is,*os;
    BobValue expr;

    /* open the source and object files */
    if ((is = BobOpenFileStream(c,iname,"r")) == NULL)
        return FALSE;
    if ((os = BobOpenFileStream(c,oname,"wb")) == NULL) {
        BobCloseStream(is);
        return FALSE;
    }

    /* write the object file header */
    if (!WriteHeader(c,os)) {
        BobCloseStream(is);
        BobCloseStream(os);
        return FALSE;
    }

    /* setup the unwind target */
    BobPushUnwindTarget(c,&target);
    if (BobUnwindCatch(c) != 0) {
        BobCloseStream(is);
        BobCloseStream(os);
        BobPopUnwindTarget(c);
        return FALSE;
    }
        
    /* initialize the scanner */
    BobInitScanner(c->compiler,is);

    /* compile each expression from the source file */
    while ((expr = BobCompileExpr(c)) != NULL)
        if (!WriteMethod(c,expr,os))
            BobCallErrorHandler(c,BobErrWrite,0);

    /* return successfully */
    BobPopUnwindTarget(c);
    BobCloseStream(is);
    BobCloseStream(os);
    return TRUE;
}

/* BobCompileString - read and compile an expression from a string */
int BobCompileString(BobInterpreter *c,char *str,BobStream *os)
{
    BobStream *is;
    int sts;
    if (!(is = BobMakeStringStream(c,(unsigned char *)str,strlen(str))))
        return FALSE;
    sts = BobCompileStream(c,is,os);
    BobCloseStream(is);
    return sts;
}

/* BobCompileStream - read and compile an expression from a stream */
int BobCompileStream(BobInterpreter *c,BobStream *is,BobStream *os)
{
    BobValue expr;
        
    /* initialize the scanner */
    BobInitScanner(c->compiler,is);

    /* compile the expression */
    expr = BobCompileExpr(c);

    /* write the compiled method */
    if (!WriteMethod(c,expr,os))
        BobCallErrorHandler(c,BobErrWrite,0);

    /* return successfully */
    return TRUE;
}

/* WriteHeader - write an object file header */
static int WriteHeader(BobInterpreter *c,BobStream *s)
{
    return BobStreamPutC('B',s) != BobStreamEOF
        && BobStreamPutC('O',s) != BobStreamEOF
        && BobStreamPutC('B',s) != BobStreamEOF
        && BobStreamPutC('O',s) != BobStreamEOF
        && WriteInteger(BobFaslVersion,s);
}

/* WriteMethod - write a method to a fasl file */
static int WriteMethod(BobInterpreter *c,BobValue method,BobStream *s)
{
    return WriteCodeValue(c,BobMethodCode(method),s);
}

/* WriteValue - write a value */
static int WriteValue(BobInterpreter *c,BobValue v,BobStream *s)
{
    if (v == c->nilValue)
        return BobStreamPutC(BobFaslTagNil,s) != BobStreamEOF;
    else if (BobCompiledCodeP(v))
        return WriteCodeValue(c,v,s);
    else if (BobVectorP(v))
        return WriteVectorValue(c,v,s);
    else if (BobObjectP(v))
        return WriteObjectValue(c,v,s);
    else if (BobSymbolP(v))
        return WriteSymbolValue(c,v,s);
    else if (BobStringP(v))
        return WriteStringValue(c,v,s);
    else if (BobIntegerP(v))
        return WriteIntegerValue(c,v,s);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    else if (BobFloatP(v))
        return WriteFloatValue(c,v,s);
#endif
    else
        return FALSE;
    return TRUE;
}

/* WriteCodeValue - write a code value */
static int WriteCodeValue(BobInterpreter *c,BobValue v,BobStream *s)
{
    return BobStreamPutC(BobFaslTagCode,s) != BobStreamEOF
        && WriteVector(c,v,s);
}

/* WriteVectorValue - write a vector value */
static int WriteVectorValue(BobInterpreter *c,BobValue v,BobStream *s)
{
    return BobStreamPutC(BobFaslTagVector,s) != BobStreamEOF
        && WriteVector(c,v,s);
}

/* WriteObjectValue - write an object value */
static int WriteObjectValue(BobInterpreter *c,BobValue v,BobStream *s)
{
    BobValue p;

    /* write the type tag, class and object size */
    if (BobStreamPutC(BobFaslTagObject,s) == BobStreamEOF
    ||  !WriteValue(c,c->nilValue,s) /* class symbol */
    ||  !WriteInteger(BobObjectPropertyCount(v),s))
        return FALSE;

    /* write out the properties */
    p = BobObjectProperties(v);
    if (BobVectorP(p)) {
        BobIntegerType size = BobVectorSize(p);
        BobIntegerType i;
        for (i = 0; i < size; ++i) {
            BobValue pp = BobVectorElement(p,i);
            for (; pp != c->nilValue; pp = BobPropertyNext(pp)) {
                if (!WriteValue(c,BobPropertyTag(pp),s)
                ||  !WriteValue(c,BobPropertyValue(pp),s))
                    return FALSE;
            }
        }
    }
    else {
        for (; p != c->nilValue; p = BobPropertyNext(p)) {
            if (!WriteValue(c,BobPropertyTag(p),s)
            ||  !WriteValue(c,BobPropertyValue(p),s))
                return FALSE;
        }
    }

    /* return successfully */
    return TRUE;
}

/* WriteSymbolValue - write a symbol value */
static int WriteSymbolValue(BobInterpreter *c,BobValue v,BobStream *s)
{
    return BobStreamPutC(BobFaslTagSymbol,s) != BobStreamEOF
        && WriteString(BobSymbolPrintName(v),BobSymbolPrintNameLength(v),s);
}

/* WriteStringValue - write a string value */
static int WriteStringValue(BobInterpreter *c,BobValue v,BobStream *s)
{
    return BobStreamPutC(BobFaslTagString,s) != BobStreamEOF
        && WriteString(BobStringAddress(v),BobStringSize(v),s);
}

/* WriteIntegerValue - write an integer value */
static int WriteIntegerValue(BobInterpreter *c,BobValue v,BobStream *s)
{
    return BobStreamPutC(BobFaslTagInteger,s) != BobStreamEOF
        && WriteInteger(BobIntegerValue(v),s);
}

/* WriteFloatValue - write a float value */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static int WriteFloatValue(BobInterpreter *c,BobValue v,BobStream *s)
{
    return BobStreamPutC(BobFaslTagFloat,s) != BobStreamEOF
        && WriteFloat(BobFloatValue(v),s);
}
#endif

/* WriteVector - write a vector value */
static int WriteVector(BobInterpreter *c,BobValue v,BobStream *s)
{
    BobIntegerType size = BobBasicVectorSize(v);
    BobValue *p = BobBasicVectorAddress(v);
    if (!WriteInteger(size,s))
        return FALSE;
    while (--size >= 0)
        if (!WriteValue(c,*p++,s))
            return FALSE;
    return TRUE;
}

/* WriteString - write a string value */
static int WriteString(unsigned char *str,BobIntegerType size,BobStream *s)
{
    if (!WriteInteger(size,s))
        return FALSE;
    while (--size >= 0)
        if (BobStreamPutC(*str++,s) == BobStreamEOF)
            return FALSE;
    return TRUE;
}

/* WriteInteger - write an integer value */
static int WriteInteger(BobIntegerType n,BobStream *s)
{
    return BobStreamPutC((int)(n >> 24),s) != BobStreamEOF
    &&     BobStreamPutC((int)(n >> 16),s) != BobStreamEOF
    &&     BobStreamPutC((int)(n >>  8),s) != BobStreamEOF
    &&     BobStreamPutC((int)(n)      ,s) != BobStreamEOF;
}

/* WriteFloat - write a float value to an image file */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static int WriteFloat(BobFloatType n,BobStream *s)
{
    int count = sizeof(BobFloatType);
#ifdef BOB_REVERSE_FLOATS_ON_WRITE
    char *p = (char *)&n + sizeof(BobFloatType);
    while (--count >= 0) {
        if (BobStreamPutC(*--p,s) == BobStreamEOF)
            return FALSE;
	}
#else
    char *p = (char *)&n;
    while (--count >= 0) {
        if (BobStreamPutC(*p++,s) == BobStreamEOF)
            return FALSE;
	}
#endif
    return TRUE;
}
#endif
