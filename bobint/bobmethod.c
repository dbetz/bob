/* bobmethod.c - method types */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include "bob.h"

/* method handlers */
static BobValue BIF_Decode(BobInterpreter *c);
static BobValue BIF_Apply(BobInterpreter *c);

/* virtual property methods */

/* Method methods */
static BobCMethod methods[] = {
BobMethodEntry( "Decode",           BIF_Decode          ),
BobMethodEntry( "Apply",            BIF_Apply           ),
BobMethodEntry(	0,                  0                   )
};

/* Method properties */
static BobVPMethod properties[] = {
BobVPMethodEntry( 0,                0,					0					)
};

/* BobInitMethod - initialize the 'Method' object */
void BobInitMethod(BobInterpreter *c)
{
    c->methodObject = BobEnterType(c,"Method",&BobMethodDispatch);
    BobEnterMethods(c,c->methodObject,methods);
    BobEnterVPMethods(c,c->methodObject,properties);
}

/* BIF_Decode - built-in method 'Decode' */
static BobValue BIF_Decode(BobInterpreter *c)
{
    BobStream *s = c->standardOutput;
    BobValue obj;
    BobParseArguments(c,"V=*|P=",&obj,&BobMethodDispatch,&s,BobFileDispatch);
    if (BobCMethodP(obj)) {
        BobPrint(c,obj,s);
        BobStreamPutC('\n',s);
    }
    else
        BobDecodeProcedure(c,obj,s);
    return c->trueValue;
}

/* BIF_Apply - built-in method 'Apply' */
static BobValue BIF_Apply(BobInterpreter *c)
{
    BobIntegerType i,vcnt,argc;
    BobValue argv;
    BobCheckArgMin(c,3);
    BobCheckType(c,1,BobMethodP);
    BobCheckType(c,BobArgCnt(c),BobVectorP);
    argv = BobGetArg(c,BobArgCnt(c));
    if (BobMovedVectorP(argv))
        argv = BobVectorForwardingAddr(argv);
    vcnt = BobVectorSizeI(argv);
    argc = BobArgCnt(c) + vcnt - 3;
    BobCheck(c,argc + 1);
    BobPush(c,BobGetArg(c,1));
    for (i = 3; i < BobArgCnt(c); ++i)
        BobPush(c,BobGetArg(c,i));
    for (i = 0; i < vcnt; ++i)
        BobPush(c,BobVectorElementI(argv,i));
    return BobInternalCall(c,argc);
}

/* METHOD */

#define SetMethodCode(o,v)              BobSetFixedVectorElement(o,0,v)
#define SetMethodEnv(o,v)               BobSetFixedVectorElement(o,1,v)
#define SetMethodGlobals(o,v)           BobSetFixedVectorElement(o,2,v)

/* Method handlers */
static int GetMethodProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
static int SetMethodProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
static int MethodPrint(BobInterpreter *c,BobValue val,BobStream *s);
static long MethodSize(BobValue obj);
static void MethodScan(BobInterpreter *c,BobValue obj);

/* Method dispatch */
BobDispatch BobMethodDispatch = {
    "Method",
    &BobMethodDispatch,
    GetMethodProperty,
    SetMethodProperty,
    BobDefaultNewInstance,
    MethodPrint,
    MethodSize,
    BobDefaultCopy,
    MethodScan,
    BobDefaultHash
};

/* GetMethodProperty - Method get property handler */
static int GetMethodProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    return BobGetVirtualProperty(c,obj,c->methodObject,tag,pValue);
}

/* SetMethodProperty - Method set property handler */
static int SetMethodProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
    return BobSetVirtualProperty(c,obj,c->methodObject,tag,value);
}

/* MethodSize - Method size handler */
static long MethodSize(BobValue obj)
{
    return sizeof(BobFixedVector) + BobMethodSize * sizeof(BobValue);
}

/* MethodScan - Method scan handler */
static void MethodScan(BobInterpreter *c,BobValue obj)
{
    long i;
    for (i = 0; i < BobMethodSize; ++i)
        BobSetFixedVectorElement(obj,i,BobCopyValue(c,BobFixedVectorElement(obj,i)));
}

/* MethodPrint - Method print handler */
static int MethodPrint(BobInterpreter *c,BobValue val,BobStream *s)
{
    BobValue name = BobCompiledCodeName(BobMethodCode(val));
    if (name == c->nilValue)
        return BobDefaultPrint(c,val,s);
    return BobStreamPutS("<Method-",s) == 0
        && BobStreamPutS((char *)BobStringAddress(name),s) == 0
        && BobStreamPutC('>',s) == '>';
}

/* BobMakeMethod - make a new method */
BobValue BobMakeMethod(BobInterpreter *c,BobValue code,BobValue env)
{
    BobValue new;
    BobCheck(c,2);
    BobPush(c,code);
    BobPush(c,env);
    new = BobMakeFixedVectorValue(c,&BobMethodDispatch,BobMethodSize);
    SetMethodEnv(new,BobPop(c));
    SetMethodCode(new,BobPop(c));
    return new;
}

/* CMETHOD */

#define SetCMethodName(o,v)             (((BobCMethod *)o)->name = (v))  
#define SetCMethodHandler(o,v)          (((BobCMethod *)o)->handler = (v))

/* CMethod handlers */
static int CMethodPrint(BobInterpreter *c,BobValue val,BobStream *s);
static long CMethodSize(BobValue obj);
static BobValue CMethodCopy(BobInterpreter *c,BobValue obj);

/* CMethod dispatch */
BobDispatch BobCMethodDispatch = {
    "CMethod",
    &BobMethodDispatch,
    GetMethodProperty,
    SetMethodProperty,
    BobDefaultNewInstance,
    CMethodPrint,
    CMethodSize,
    CMethodCopy,
    BobDefaultScan,
    BobDefaultHash
};

/* CMethodPrint - CMethod print handler */
static int CMethodPrint(BobInterpreter *c,BobValue val,BobStream *s)
{
    return BobStreamPutS("<CMethod-",s) == 0
        && BobStreamPutS(BobCMethodName(val),s) == 0
        && BobStreamPutC('>',s) == '>';
}

/* CMethodSize - CMethod size handler */
static long CMethodSize(BobValue obj)
{
    return sizeof(BobCMethod);
}

/* CMethodCopy - CMethod copy handler */
static BobValue CMethodCopy(BobInterpreter *c,BobValue obj)
{
    return obj;
}

/* BobMakeCMethod - make a new c method value */
BobValue BobMakeCMethod(BobInterpreter *c,char *name,BobCMethodHandler *handler)
{
    BobValue new;
    new = BobAllocate(c,sizeof(BobCMethod));
    BobSetDispatch(new,&BobCMethodDispatch);
    SetCMethodName(new,name);
    SetCMethodHandler(new,handler);
    return new;
}

/* COMPILED CODE */

#define SetCompiledCodeBytecodes(o,v)   BobSetBasicVectorElement(o,0,v)
#define SetCompiledCodeLiteral(o,i,v)   BobSetBasicVectorElement(o,i,v)

/* CompiledCode handlers */
static int CompiledCodePrint(BobInterpreter *c,BobValue val,BobStream *s);

/* CompiledCode dispatch */
BobDispatch BobCompiledCodeDispatch = {
    "CompiledCode",
    &BobCompiledCodeDispatch,
    BobDefaultGetProperty,
    BobDefaultSetProperty,
    BobDefaultNewInstance,
    CompiledCodePrint,
    BobBasicVectorSizeHandler,
    BobDefaultCopy,
    BobBasicVectorScanHandler,
    BobDefaultHash
};

/* CompiledCodePrint - CompiledCode print handler */
static int CompiledCodePrint(BobInterpreter *c,BobValue val,BobStream *s)
{
    BobValue name = BobCompiledCodeName(val);
    if (name == c->nilValue)
        return BobDefaultPrint(c,val,s);
    return BobStreamPutS("<CompiledCode-",s) == 0
        && BobStreamPutS((char *)BobStringAddress(name),s) == 0
        && BobStreamPutC('>',s) == '>';
}

/* BobMakeCompiledCode - make a compiled code value */
BobValue BobMakeCompiledCode(BobInterpreter *c,long size,BobValue bytecodes)
{
    BobValue code;
    BobCPush(c,bytecodes);
    code = BobMakeBasicVector(c,&BobCompiledCodeDispatch,size);
    SetCompiledCodeBytecodes(code,BobPop(c));
    return code;
}

