/* bob.h - bob definitions */
/*
        Copyright (c) 2013, by David Michael Betz
        All rights reserved
*/

#ifndef __BOB_H__
#define __BOB_H__

#include <stdio.h>
#include <stdarg.h>
#include <setjmp.h>

/* boolean values */
#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

/* determine whether the machine is little endian */
#if defined(WIN32)
#define BOB_REVERSE_FLOATS_ON_READ
#define BOB_REVERSE_FLOATS_ON_WRITE
#endif

/* object file version number */
#define BobFaslVersion      4

/* symbol hash table size */
#define BobSymbolHashTableSize      256         /* power of 2 */

/* hash table thresholds */
#define BobHashTableCreateThreshold 4           /* power of 2 */
#define BobHashTableExpandThreshold 2           /* power of 2 */
#define BobHashTableExpandMaximum   (64 * 1024) /* power of 2 */

/* vector expansion thresholds */
/* amount to expand is:
    min(neededSize,
        min(BobVectorExpandMaximum,
            max(BobVectorExpandMinimum,
                currentSize / BobVectorExpandDivisor)))
*/
#define BobVectorExpandMinimum      8
#define BobVectorExpandMaximum      128
#define BobVectorExpandDivisor      2

/* object file tags */
#define BobFaslTagNil       0
#define BobFaslTagCode      1
#define BobFaslTagVector    2
#define BobFaslTagObject    3
#define BobFaslTagSymbol    4
#define BobFaslTagString    5
#define BobFaslTagInteger   6
#define BobFaslTagFloat     7

/* basic types */
typedef struct BobHeader *BobValue;
typedef long BobIntegerType;
typedef double BobFloatType;

/* pointer sized integer */
typedef long BobPointerType;

/* output conversion functions */
#ifdef BobIntegerToStringExt
extern void BobIntegerToString(char *buf,BobIntegerType v);
#else
#define BobIntegerToString(buf,v)	sprintf(buf,"%ld",(long)(v))
#endif
#ifdef BobFloatToStringExt
extern void BobFloatToString(char *buf,BobFloatType v);
#else
#define BobFloatToString(buf,v)		sprintf(buf,"%g",(double)(v))
#endif

/* forward types */
typedef struct BobInterpreter BobInterpreter;
typedef struct BobCompiler BobCompiler;
typedef struct BobDispatch BobDispatch;
typedef struct BobStream BobStream;
typedef struct BobStreamDispatch BobStreamDispatch;
typedef struct BobFrame BobFrame;
typedef struct BobCMethod BobCMethod;
typedef struct BobVPMethod BobVPMethod;

/*  boolean macros */
#define BobToBoolean(c,v) ((v) ? (c)->trueValue : (c)->falseValue)
#define BobTrueP(c,v)     ((v) != (c)->falseValue)
#define BobFalseP(c,v)    ((v) == (c)->falseValue)

/* rounding mask */
#define BobValueMask  (sizeof(BobValue) - 1)

/* round a size up to a multiple of the size of a long */
#define BobRoundSize(x)   (((x) + BobValueMask) & ~BobValueMask)

/* unwind target structure */
typedef struct BobUnwindTarget BobUnwindTarget;
struct BobUnwindTarget {
    BobUnwindTarget *next;
    jmp_buf target;
};

#define BobUnwindCatch(c)       setjmp((c)->unwindTarget->target)
#define BobUnwind(c,v)          longjmp((c)->unwindTarget->target,(v))
#define BobPopUnwindTarget(c)   ((c)->unwindTarget = (c)->unwindTarget->next)

/* memory space structure */
typedef struct BobMemorySpace BobMemorySpace;
struct BobMemorySpace {
    unsigned char *base;
    unsigned char *free;
    unsigned char *top;
    BobValue cObjects;
};

/* number of pointers in a protected pointer block */
#define BobPPSize   100

/* protected pointer block structure */
typedef struct BobProtectedPtrs BobProtectedPtrs;
struct BobProtectedPtrs {
    struct BobProtectedPtrs *next;
    BobValue *pointers[BobPPSize];
    int count;
};

/* cmethod handler */
typedef BobValue BobCMethodHandler(BobInterpreter *c);

#define BobMethodEntry(name,fcn)		{ &BobCMethodDispatch,name,fcn }

/* virtual property handlers */
typedef BobValue (*BobVPGetHandler)(BobInterpreter *c,BobValue obj);
typedef void (*BobVPSetHandler)(BobInterpreter *c,BobValue obj,BobValue value);

#define BobVPMethodEntry(name,get,set)	{ &BobVPMethodDispatch,name,get,set }

/* interpreter context structure */
struct BobInterpreter {
    BobCompiler *compiler;          /* compiler structure */
    char *errorMessage;             /* last error message */
    BobUnwindTarget *unwindTarget;  /* unwind target */
    BobValue *argv;                 /* arguments for current function */
    int argc;                       /* argument count for current function */
    BobValue *stack;                /* stack base */
    BobValue *stackTop;             /* stack top */
    BobValue *sp;                   /* stack pointer */
    BobFrame *fp;                   /* frame pointer */
    BobValue code;                  /* code object */
    unsigned char *cbase;           /* code base */
    unsigned char *pc;              /* program counter */
    BobValue val;                   /* value register */
    BobValue env;                   /* environment register */
    BobValue nilValue;              /* nil value */
    BobValue trueValue;             /* true value */
    BobValue falseValue;            /* false value */
    BobValue objectValue;           /* base of object inheritance tree */
    BobValue methodObject;          /* object for the Method type */
    BobValue vectorObject;          /* object for the Vector type */
    BobValue symbolObject;          /* object for the Symbol type */
    BobValue stringObject;          /* object for the String type */
    BobValue integerObject;         /* object for the Integer type */
    BobValue floatObject;           /* object for the Float type */
    BobValue symbols;               /* symbol table */
    void (*errorHandler)(BobInterpreter *c,int code,va_list ap);
    BobProtectedPtrs *protectedPtrs;/* protected pointers */
    BobMemorySpace *oldSpace;       /* old memory space */
    BobMemorySpace *newSpace;       /* new memory space */
    unsigned long totalMemory;      /* total memory allocated */
    unsigned long allocCount;       /* number of calls to BobAlloc */
    BobStream *standardInput;       /* standard input stream */
    BobStream *standardOutput;      /* standard output stream */
    BobStream *standardError;       /* standard error stream */
    void *(*fileOpen)(BobInterpreter *c,char *name,char *mode);
    int (*fileClose)(void *fp);
    int (*fileGetC)(void *fp);
    int (*filePutC)(int ch,void *fp);
    BobDispatch *typeDispatch;      /* the Type type */
    BobDispatch *types;             /* derived types */
    void (*protectHandler)(BobInterpreter *c,void *data);
    void *protectData;
};

/* argument list macros */
#define BobCheckArgCnt(c,m)         BobCheckArgRange(c,m,m)
#define BobCheckArgMin(c,m)         do { \
                                        if ((c)->argc < (m)) \
                                            BobTooFewArguments(c); \
                                    } while (0)
#define BobCheckArgRange(c,mn,mx)   do { \
                                        int n = (c)->argc; \
                                        if (n < (mn)) \
                                            BobTooFewArguments(c); \
                                        else if (n > (mx)) \
                                            BobTooManyArguments(c); \
                                    } while (0)
#define BobCheckType(c,n,tp)        do { \
                                        if (!tp(BobGetArg(c,n))) \
                                            BobTypeError(c,BobGetArg(c,n)); \
                                    } while (0)
#define BobArgCnt(c)                ((c)->argc)
#define BobArgPtr(c)                ((c)->argv)
#define BobGetArg(c,n)              ((c)->argv[-(n)])

/* stack manipulation macros */
#define BobCheck(c,n)   do { if ((c)->sp - (n) < &(c)->stack[0]) BobStackOverflow(c); } while (0)
#define BobCPush(c,v)   do { if ((c)->sp > &(c)->stack[0]) BobPush(c,v); else BobStackOverflow(c); } while (0)
#define BobPush(c,v)    (*--(c)->sp = (v))
#define BobTop(c)       (*(c)->sp)
#define BobSetTop(c,v)  (*(c)->sp = (v))
#define BobPop(c)       (*(c)->sp++)
#define BobDrop(c,n)    ((c)->sp += (n))

/* destructor type */
typedef void (BobDestructor)(BobInterpreter *c,void *data,void *ptr);

/* type dispatch structure */
struct BobDispatch {
    char *typeName;
    BobDispatch *baseType;
    int (*getProperty)(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
    int (*setProperty)(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
    BobValue (*newInstance)(BobInterpreter *c,BobValue parent);
    int (*print)(BobInterpreter *c,BobValue obj,BobStream *s);
    long (*size)(BobValue obj);
    BobValue (*copy)(BobInterpreter *c,BobValue obj);
    void (*scan)(BobInterpreter *c,BobValue obj);
    BobIntegerType (*hash)(BobValue obj);
    BobValue object;
    long dataSize;
	void (*destroy)(BobInterpreter *c,BobValue obj);
    BobDispatch *parent;
    BobDispatch *next;
};

/* type macros */
#define BobPointerP(o)                  (((BobPointerType)(o) & 1) == 0)
#define BobGetDispatch(o)               (BobPointerP(o) ? BobQuickGetDispatch(o) : &BobIntegerDispatch)
#define BobQuickGetDispatch(o)          ((o)->dispatch)
#define BobIsType(o,t)                  (BobGetDispatch(o) == (t))
#define BobIsBaseType(o,t)              (BobGetDispatch(o)->baseType == (t))
#define BobQuickIsType(o,t)             (BobQuickGetDispatch(o) == (t))
#define BobTypeName(o)                  (BobGetDispatch(o)->typeName)
#define BobQuickTypeName(o)             (BobQuickGetDispatch(o)->typeName)

/* VALUE */

struct BobHeader {
    BobDispatch *dispatch;
};

#define BobSetDispatch(o,v)             ((o)->dispatch = (v))
#define BobGetProperty1(c,o,t,pv)       (BobGetDispatch(o)->getProperty(c,o,t,pv))
#define BobSetProperty(c,o,t,v)         (BobGetDispatch(o)->setProperty(c,o,t,v))
#define BobNewInstance(c,o)             (BobGetDispatch(o)->newInstance(c,o))
#define BobPrintValue(c,o,s)            (BobGetDispatch(o)->print(c,o,s))
#define BobCopyValue(c,o)               (BobGetDispatch(o)->copy(c,o))
#define BobHashValue(o)                 (BobGetDispatch(o)->hash(o))

int BobGetProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);

/* BROKEN HEART */

typedef struct {
    BobDispatch *dispatch;
    BobValue forwardingAddr;
} BobBrokenHeart;

#define BobBrokenHeartP(o)                      BobIsType(o,&BobBrokenHeartDispatch)
#define BobBrokenHeartForwardingAddr(o)         (((BobBrokenHeart *)o)->forwardingAddr)  
#define BobBrokenHeartSetForwardingAddr(o,v)    (((BobBrokenHeart *)o)->forwardingAddr = (v))  
extern BobDispatch BobBrokenHeartDispatch;

/* SMALL INTEGER */

#define BobSmallIntegerMin (-(1 << 30))
#define BobSmallIntegerMax ((1 << 30) - 1)

#define BobSmallIntegerValueP(v)        ((v) >= BobSmallIntegerMin && (v) <= BobSmallIntegerMax)
#define BobSmallIntegerValue(o)         ((BobIntegerType)o >> 1)
#define BobMakeSmallInteger(n)          ((BobValue)(((BobIntegerType)(n) << 1) | 1))

/* NUMBER */

#define BobNumberP(o)                   (BobIntegerP(o) || BobFloatP(o))

/* INTEGER */

typedef struct {
    BobDispatch *dispatch;
    BobIntegerType value;
} BobInteger;

#define BobIntegerP(o)                  (!BobPointerP(o) || BobIsType(o,&BobIntegerDispatch))
#define BobIntegerValue(o)              (BobPointerP(o) ? BobHeapIntegerValue(o) : BobSmallIntegerValue(o))
#define BobHeapIntegerValue(o)          (((BobInteger *)o)->value)
#define BobSetHeapIntegerValue(o,v)     (((BobInteger *)o)->value = (v))
BobValue BobMakeInteger(BobInterpreter *c,BobIntegerType value);
extern BobDispatch BobIntegerDispatch;

/* FLOAT */

typedef struct {
    BobDispatch *dispatch;
    BobFloatType value;
} BobFloat;

#define BobFloatP(o)                    BobIsType(o,&BobFloatDispatch)
#define BobFloatValue(o)                (((BobFloat *)o)->value)
#define BobSetFloatValue(o,v)           (((BobFloat *)o)->value = (v))
BobValue BobMakeFloat(BobInterpreter *c,BobFloatType value);
extern BobDispatch BobFloatDispatch;

/* STRING */

typedef struct {
    BobDispatch *dispatch;
    BobIntegerType size;
/*  unsigned char data[0]; */
} BobString;

#define BobStringP(o)                   BobIsType(o,&BobStringDispatch)
#define BobStringSize(o)                (((BobString *)o)->size)  
#define BobSetStringSize(o,v)           (((BobString *)o)->size = (v))
#define BobStringAddress(o)             ((unsigned char *)o + sizeof(BobString))
#define BobStringElement(o,i)           (BobStringAddress(o)[i])  
#define BobSetStringElement(o,i,v)      (BobStringAddress(o)[i] = (v))  
BobValue BobMakeString(BobInterpreter *c,unsigned char *data,BobIntegerType size);
BobValue BobMakeCString(BobInterpreter *c,char *str);
extern BobDispatch BobStringDispatch;

/* SYMBOL */

typedef struct {
    BobDispatch *dispatch;
    BobIntegerType hashValue;
    BobValue value;
    BobValue next;
    int printNameLength;
    unsigned char printName[1];
} BobSymbol;

#define BobSymbolP(o)                   BobIsType(o,&BobSymbolDispatch)
#define BobSymbolPrintName(o)           (((BobSymbol *)o)->printName)
#define BobSymbolPrintNameLength(o)     (((BobSymbol *)o)->printNameLength)
#define BobSymbolHashValue(o)			(((BobSymbol *)o)->hashValue)
#define BobGlobalValue(o)               (((BobSymbol *)o)->value)
#define BobSetGlobalValue(o,v)          (((BobSymbol *)o)->value = (v))
#define BobSymbolNext(o)                (((BobSymbol *)o)->next)
#define BobSetSymbolNext(o,v)           (((BobSymbol *)o)->next = (v))
BobValue BobMakeSymbol(BobInterpreter *c,unsigned char *printName,int length);
BobValue BobIntern(BobInterpreter *c,BobValue printName);
BobValue BobInternCString(BobInterpreter *c,char *printName);
BobValue BobInternString(BobInterpreter *c,unsigned char *printName,int length);
extern BobDispatch BobSymbolDispatch;

/* OBJECT */

typedef struct {
    BobDispatch *dispatch;
    BobValue parent;
    BobValue properties;
    BobIntegerType propertyCount;
} BobObject;

#define BobObjectP(o)                   BobIsBaseType(o,&BobObjectDispatch)
#define BobObjectClass(o)               (((BobObject *)o)->parent)
#define BobSetObjectClass(o,v)          (((BobObject *)o)->parent = (v))
#define BobObjectProperties(o)          (((BobObject *)o)->properties)
#define BobSetObjectProperties(o,v)     (((BobObject *)o)->properties = (v))
#define BobObjectPropertyCount(o)       (((BobObject *)o)->propertyCount)
#define BobSetObjectPropertyCount(o,v)  (((BobObject *)o)->propertyCount = (v))

BobValue BobMakeObject(BobInterpreter *c,BobValue parent);
BobValue BobCloneObject(BobInterpreter *c,BobValue obj);
BobValue BobFindProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobIntegerType *pHashValue,BobIntegerType *pIndex);
extern BobDispatch BobObjectDispatch;

/* COBJECT */

typedef struct {
    BobDispatch *dispatch;
    BobValue parent;
    BobValue properties;
    BobIntegerType propertyCount;
    BobValue next;
} BobCObject;

typedef struct {
    BobCObject hdr;
    void *ptr;
} BobCPtrObject;

#define BobCObjectSize(o)            (((BobCObject *)o)->size)
#define BobSetCObjectSize(o,v)       (((BobCObject *)o)->size = (v))
#define BobCObjectDataAddress(o)     ((void *)((char *)o + sizeof(BobCObject)))
#define BobCObjectValue(o)           (((BobCPtrObject *)o)->ptr)
#define BobSetCObjectValue(o,v)      (((BobCPtrObject *)o)->ptr = (v))

int BobCObjectP(BobValue val);
BobDispatch *BobMakeCObjectType(BobInterpreter *c,BobDispatch *parent,char *typeName,BobCMethod *methods,BobVPMethod *properties,long size);
BobValue BobMakeCObject(BobInterpreter *c,BobDispatch *d);
BobDispatch *BobMakeCPtrObjectType(BobInterpreter *c,BobDispatch *parent,char *typeName,BobCMethod *methods,BobVPMethod *properties);
BobValue BobMakeCPtrObject(BobInterpreter *c,BobDispatch *d,void *ptr);
void BobEnterCObjectMethods(BobInterpreter *c,BobDispatch *d,BobCMethod *methods,BobVPMethod *properties);
int BobGetVirtualProperty(BobInterpreter *c,BobValue obj,BobValue parent,BobValue tag,BobValue *pValue);
int BobSetVirtualProperty(BobInterpreter *c,BobValue obj,BobValue parent,BobValue tag,BobValue value);
extern BobDispatch BobCObjectDispatch;

/* VECTOR */

typedef struct {
    BobDispatch *dispatch;
    BobIntegerType maxSize;
    union {
        BobIntegerType size;
        BobValue forwardingAddr;
    } d;
/*  BobValue data[0]; */
} BobVector;

#define BobVectorP(o)                   BobIsBaseType(o,&BobVectorDispatch)
#define BobMovedVectorP(o)              BobIsType(o,&BobMovedVectorDispatch)
#define BobVectorSizeI(o)               (((BobVector *)o)->d.size)
#define BobSetVectorSize(o,s)           (((BobVector *)o)->d.size = (s))
#define BobVectorForwardingAddr(o)      (((BobVector *)o)->d.forwardingAddr)
#define BobSetVectorForwardingAddr(o,a) (((BobVector *)o)->d.forwardingAddr = (a))
#define BobVectorMaxSize(o)             (((BobVector *)o)->maxSize)
#define BobSetVectorMaxSize(o,s)        (((BobVector *)o)->maxSize = (s))
#define BobVectorAddressI(o)            ((BobValue *)((char *)o + sizeof(BobVector))) 
#define BobVectorElementI(o,i)          (BobVectorAddress(o)[i])
#define BobSetVectorElementI(o,i,v)     (BobVectorAddress(o)[i] = (v))
BobValue BobMakeVector(BobInterpreter *c,BobIntegerType size);
BobValue BobCloneVector(BobInterpreter *c,BobValue obj);
BobIntegerType BobVectorSize(BobValue obj);
BobValue *BobVectorAddress(BobValue obj);
BobValue BobVectorElement(BobValue obj,BobIntegerType i);
void BobSetVectorElement(BobValue obj,BobIntegerType i,BobValue val);
extern BobDispatch BobVectorDispatch;
extern BobDispatch BobMovedVectorDispatch;

/* BASIC VECTOR */

typedef struct {
    BobDispatch *dispatch;
    BobIntegerType size;
/*  BobValue data[0]; */
} BobBasicVector;

#define BobBasicVectorSize(o)           (((BobBasicVector *)o)->size)
#define BobSetBasicVectorSize(o,s)      (((BobBasicVector *)o)->size = (s))
#define BobBasicVectorAddress(o)        ((BobValue *)((char *)o + sizeof(BobBasicVector))) 
#define BobBasicVectorElement(o,i)      (BobBasicVectorAddress(o)[i])
#define BobSetBasicVectorElement(o,i,v) (BobBasicVectorAddress(o)[i] = (v))
BobValue BobMakeBasicVector(BobInterpreter *c,BobDispatch *type,BobIntegerType size);

/* FIXED VECTOR */

typedef struct {
    BobDispatch *dispatch;
/*  BobValue data[0]; */
} BobFixedVector;

#define BobFixedVectorAddress(o)        ((BobValue *)((char *)o + sizeof(BobFixedVector))) 
#define BobFixedVectorElement(o,i)      (BobFixedVectorAddress(o)[i])
#define BobSetFixedVectorElement(o,i,v) (BobFixedVectorAddress(o)[i] = (v))
BobValue BobMakeFixedVectorValue(BobInterpreter *c,BobDispatch *type,int size);

/* CMETHOD */

struct BobCMethod {
    BobDispatch *dispatch;
    char *name;
    BobCMethodHandler *handler;
};

#define BobCMethodP(o)                  BobIsType(o,&BobCMethodDispatch)
#define BobCMethodName(o)               (((BobCMethod *)o)->name)  
#define BobCMethodHandler(o)            (((BobCMethod *)o)->handler)  
BobValue BobMakeCMethod(BobInterpreter *c,char *name,BobCMethodHandler *handler);
extern BobDispatch BobCMethodDispatch;

/* VIRTUAL PROPERTY METHOD */

struct BobVPMethod {
    BobDispatch *dispatch;
    char *name;
    BobVPGetHandler getHandler;
    BobVPSetHandler setHandler;
};

#define BobVPMethodP(o)                  BobIsType(o,&BobVPMethodDispatch)
#define BobVPMethodName(o)               (((BobVPMethod *)o)->name)  
#define BobVPMethodGetHandler(o)         (((BobVPMethod *)o)->getHandler)  
#define BobVPMethodSetHandler(o)         (((BobVPMethod *)o)->setHandler)  
BobValue BobMakeVPMethod(BobInterpreter *c,char *name,BobVPGetHandler getHandler,BobVPSetHandler setHandler);
extern BobDispatch BobVPMethodDispatch;

/* HASH TABLE */

#define BobHashTableP(o)                BobIsType(o,&BobHashTableDispatch)
#define BobHashTableSize(o)             BobBasicVectorSize(o)
#define BobHashTableElement(o,i)        BobBasicVectorElement(o,i)
#define BobSetHashTableElement(o,i,v)   BobSetBasicVectorElement(o,i,v)
BobValue BobMakeHashTable(BobInterpreter *c,long size);
extern BobDispatch BobHashTableDispatch;

/* PROPERTY */

#define BobPropertyP(o)                 BobIsType(o,&BobPropertyDispatch)
#define BobPropertyTag(o)               BobFixedVectorElement(o,0)
#define BobPropertyValue(o)             BobFixedVectorElement(o,1) 
#define BobSetPropertyValue(o,v)        BobSetFixedVectorElement(o,1,v) 
#define BobPropertyNext(o)              BobFixedVectorElement(o,2)
#define BobSetPropertyNext(o,v)         BobSetFixedVectorElement(o,2,v)
#define BobPropertySize                 3

BobValue BobMakeProperty(BobInterpreter *c,BobValue key,BobValue value);
extern BobDispatch BobPropertyDispatch;

/* METHOD */

#define BobMethodP(o)                   BobIsBaseType(o,&BobMethodDispatch)
#define BobMethodCode(o)                BobFixedVectorElement(o,0) 
#define BobMethodEnv(o)                 BobFixedVectorElement(o,1)
#define BobMethodSize                   2

BobValue BobMakeMethod(BobInterpreter *c,BobValue code,BobValue env);
extern BobDispatch BobMethodDispatch;

/* COMPILED CODE */

#define BobCompiledCodeP(o)             BobIsType(o,&BobCompiledCodeDispatch)
#define BobCompiledCodeLiterals(o)      BobBasicVectorAddress(o)
#define BobCompiledCodeLiteral(o,i)     BobBasicVectorElement(o,i)
#define BobCompiledCodeBytecodes(o)     BobBasicVectorElement(o,0)
#define BobCompiledCodeName(o)          BobBasicVectorElement(o,1)
#define BobFirstLiteral                 1

BobValue BobMakeCompiledCode(BobInterpreter *c,long size,BobValue bytecodes);
extern BobDispatch BobCompiledCodeDispatch;

/* ENVIRONMENT */

#define BobEnvironmentP(o)              BobIsBaseType(o,&BobEnvironmentDispatch)
#define BobEnvSize(o)                   BobBasicVectorSize(o)
#define BobSetEnvSize(o,v)              BobSetBasicVectorSize(o,v)
#define BobEnvAddress(o)                BobBasicVectorAddress(o)
#define BobEnvElement(o,i)              BobBasicVectorElement(o,i)
#define BobSetEnvElement(o,i,v)         BobSetBasicVectorElement(o,i,v)
#define BobEnvNextFrame(o)              BobBasicVectorElement(o,0)
#define BobSetEnvNextFrame(o,v)         BobSetBasicVectorElement(o,0,v)
#define BobEnvNames(o)                  BobBasicVectorElement(o,1)
#define BobSetEnvNames(o,v)             BobSetBasicVectorElement(o,1,v)
#define BobFirstEnvElement              2

typedef BobBasicVector BobEnvironment;
BobValue BobMakeEnvironment(BobInterpreter *c,long size);
extern BobDispatch BobEnvironmentDispatch;

/* STACK ENVIRONMENT */

#define BobStackEnvironmentP(o)         BobIsType(o,&BobStackEnvironmentDispatch)

extern BobDispatch BobStackEnvironmentDispatch;

/* MOVED ENVIRONMENT */

#define BobMovedEnvironmentP(o)         BobIsType(o,&BobMovedEnvironmentDispatch)
#define BobMovedEnvForwardingAddr(o)    BobEnvNextFrame(o)
#define BobSetMovedEnvForwardingAddr(o,v) BobSetEnvNextFrame(o,v)

extern BobDispatch BobMovedEnvironmentDispatch;

/* FILE */

#define BobFileP(o)                     BobIsType(o,BobFileDispatch)
#define BobFileStream(o)                ((BobStream *)BobCObjectValue(o))
#define BobFileSetStream(o,v)           BobSetCObjectValue(o,(void *)v)

extern BobDispatch *BobFileDispatch;

/* TYPE */

#define BobTypeDispatch(o)              ((BobDispatch *)BobCObjectValue(o))
#define BobTypeSetDispatch(o,v)         BobSetCObjectValue(o,(void *)v)

/* default handlers */
int BobDefaultPrint(BobInterpreter *c,BobValue obj,BobStream *s);

/* end of file indicator for StreamGetC */
#define BobStreamEOF    (-1)

/* stream dispatch structure */
struct BobStreamDispatch {
    int (*close)(BobStream *s);
    int (*getc)(BobStream *s);
    int (*putc)(int ch,BobStream *s);
};

/* stream structure */
struct BobStream {
    BobStreamDispatch *d;
};

/* string stream structure */
typedef struct {
    BobStreamDispatch *d;
    unsigned char *buf;
    unsigned char *ptr;
    long len;
} BobStringStream;

/* string output stream structure */
typedef struct {
    BobStreamDispatch *d;
    unsigned char *buf;
    unsigned char *ptr;
    unsigned char *end;
    long len;
} BobStringOutputStream;

/* indirect stream structure */
typedef struct {
    BobStreamDispatch *d;
    BobStream **pStream;
} BobIndirectStream;

/* file stream structure */
typedef struct {
    BobStreamDispatch *d;
    BobInterpreter *c;
    void *fp;
} BobFileStream;

/* macros */
#define BobCloseStream(s)       ((*(s)->d->close)(s))
#define BobStreamGetC(s)        ((*(s)->d->getc)(s))
#define BobStreamPutC(ch,s)     ((*(s)->d->putc)(ch,s))

/* globals */
extern BobStream BobNullStream;

/* error codes */
#define BobErrExit                  0
#define BobErrInsufficientMemory    1
#define BobErrStackOverflow         2
#define BobErrTooManyArguments      3
#define BobErrTooFewArguments       4
#define BobErrTypeError             5
#define BobErrUnboundVariable       6
#define BobErrIndexOutOfBounds      7
#define BobErrNoMethod              8
#define BobErrBadOpcode             9
#define BobErrRestart               10
#define BobErrWrite                 11
#define BobErrBadParseCode          12
#define BobErrImpossible            13
#define BobErrNoHashValue           14
#define BobErrReadOnlyProperty		15
#define BobErrWriteOnlyProperty		16
#define BobErrFileNotFound			17
#define BobErrNewInstance           18
#define BobErrNoProperty            19
#define BobErrStackEmpty            20
#define BobErrNotAnObjectFile       21
#define BobErrWrongObjectVersion    22
#define BobErrValueError            23

/* compiler error codes */
#define BobErrSyntaxError       0x1000
#define BobErrStoreIntoConstant 0x1001
#define BobErrTooMuchCode       0x1002
#define BobErrTooManyLiterals   0x1003

/* bobcom.c prototypes */
void BobInitScanner(BobCompiler *c,BobStream *s);
BobCompiler *BobMakeCompiler(BobInterpreter *ic,void *buf,size_t size,long lsize);
void BobFreeCompiler(BobCompiler *c);
BobValue BobCompileExpr(BobInterpreter *c);

/* bobint.c prototypes */
BobValue BobCallFunction(BobInterpreter *c,BobValue fun,int argc,...);
BobValue BobCallFunctionByName(BobInterpreter *c,char *fname,int argc,...);
BobValue BobSendMessage(BobInterpreter *c,BobValue obj,BobValue selector,int argc,...);
BobValue BobSendMessageByName(BobInterpreter *c,BobValue obj,char *sname,int argc,...);
BobValue BobInternalCall(BobInterpreter *c,int argc);
BobValue BobInternalSend(BobInterpreter *c,int argc);
void BobPushUnwindTarget(BobInterpreter *c,BobUnwindTarget *target);
void BobPopAndUnwind(BobInterpreter *c,int value);
void BobTooManyArguments(BobInterpreter *c);
void BobTooFewArguments(BobInterpreter *c);
void BobTypeError(BobInterpreter *c,BobValue v);
void BobStackOverflow(BobInterpreter *c);
void BobAbort(BobInterpreter *c);
int BobEql(BobValue obj1,BobValue obj2);
void BobCopyStack(BobInterpreter *c);
void BobStackTrace(BobInterpreter *c);

/* bobenter.c prototypes */
void BobEnterVariable(BobInterpreter *c,char *name,BobValue value);
void BobEnterFunction(BobInterpreter *c,BobCMethod *function);
void BobEnterFunctions(BobInterpreter *c,BobCMethod *functions);
BobValue BobEnterObject(BobInterpreter *c,char *name,BobValue parent,BobCMethod *methods);
BobDispatch *BobEnterCObjectType(BobInterpreter *c,BobDispatch *parent,char *typeName,BobCMethod *methods,BobVPMethod *properties,long size);
BobDispatch *BobEnterCPtrObjectType(BobInterpreter *c,BobDispatch *parent,char *typeName,BobCMethod *methods,BobVPMethod *properties);
void BobEnterMethods(BobInterpreter *c,BobValue object,BobCMethod *methods);
void BobEnterMethod(BobInterpreter *c,BobValue object,BobCMethod *method);
void BobEnterVPMethods(BobInterpreter *c,BobValue object,BobVPMethod *methods);
void BobEnterVPMethod(BobInterpreter *c,BobValue object,BobVPMethod *method);
void BobEnterProperty(BobInterpreter *c,BobValue object,char *selector,BobValue value);

/* bobparse.c prototypes */
void BobParseArguments(BobInterpreter *c,char *fmt,...);

/* bobheap.c prototypes */
BobInterpreter *BobMakeInterpreter(void *buf,size_t size,size_t stackSize);
BobInterpreter *BobInitInterpreter(BobInterpreter *c);
void BobFreeInterpreter(BobInterpreter *c);
void BobCollectGarbage(BobInterpreter *c);
void BobDumpHeap(BobInterpreter *c);
BobDispatch *BobMakeDispatch(BobInterpreter *c,char *typeName,BobDispatch *prototype);
void BobFreeDispatch(BobInterpreter *c,BobDispatch *d);
int BobProtectPointer(BobInterpreter *c,BobValue *pp);
int BobUnprotectPointer(BobInterpreter *c,BobValue *pp);
BobValue BobAllocate(BobInterpreter *c,long size);
void *BobAlloc(BobInterpreter *c,unsigned long size);
void BobFree(BobInterpreter *c,void *ptr);
void BobInsufficientMemory(BobInterpreter *c);

/* default type handlers */
int BobDefaultGetProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
int BobDefaultSetProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
BobValue BobDefaultNewInstance(BobInterpreter *c,BobValue parent);
BobValue BobDefaultCopy(BobInterpreter *c,BobValue obj);
void BobDefaultScan(BobInterpreter *c,BobValue obj);
BobIntegerType BobDefaultHash(BobValue obj);

/* bobhash.c prototypes */
BobIntegerType BobHashString(unsigned char *str,int length);

/* bobtype.c prototypes */
void BobInitTypes(BobInterpreter *c);
void BobAddTypeSymbols(BobInterpreter *c);
BobValue BobEnterType(BobInterpreter *c,char *name,BobDispatch *d);

/* bobstream.c prototypes */
int BobPrint(BobInterpreter *c,BobValue val,BobStream *s);
int BobDisplay(BobInterpreter *c,BobValue val,BobStream *s);
char *BobStreamGetS(char *buf,int size,BobStream *s);
int BobStreamPutS(char *str,BobStream *s);
int BobStreamPrintF(BobStream *s,char *fmt,...);
BobStream *BobInitStringStream(BobInterpreter *c,BobStringStream *s,unsigned char *buf,long len);
BobStream *BobMakeStringStream(BobInterpreter *c,unsigned char *buf,long len);
BobStream *BobInitStringOutputStream(BobInterpreter *c,BobStringOutputStream *s,unsigned char *buf,long len);
BobStream *BobMakeIndirectStream(BobInterpreter *c,BobStream **pStream);
BobStream *BobInitFileStream(BobInterpreter *c,BobFileStream *s,FILE *fp);
BobStream *BobMakeFileStream(BobInterpreter *c,FILE *fp);
BobStream *BobOpenFileStream(BobInterpreter *c,char *fname,char *mode);

/* bobstdio.c prototypes */
void BobUseStandardIO(BobInterpreter *c);

/* bobmath.c prototypes */
void BobUseMath(BobInterpreter *c);

/* bobobject.c prototypes */
void BobInitObject(BobInterpreter *c);
void BobAddProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value,BobIntegerType hashValue,BobIntegerType i);

/* bobmethod.c prototypes */
void BobInitMethod(BobInterpreter *c);

/* bobsymbol.c prototypes */
void BobInitSymbol(BobInterpreter *c);

/* bobvector.c prototypes */
void BobInitVector(BobInterpreter *c);
long BobBasicVectorSizeHandler(BobValue obj);
void BobBasicVectorScanHandler(BobInterpreter *c,BobValue obj);

/* bobstring.c prototypes */
void BobInitString(BobInterpreter *c);

/* bobcobject.c prototypes */
void BobDestroyUnreachableCObjects(BobInterpreter *c);
void BobDestroyAllCObjects(BobInterpreter *c);

/* bobinteger.c prototypes */
void BobInitInteger(BobInterpreter *c);

/* bobfloat.c prototypes */
void BobInitFloat(BobInterpreter *c);

/* bobfile.c prototypes */
void BobInitFile(BobInterpreter *c);
BobValue BobMakeFile(BobInterpreter *c,BobStream *s);

/* bobfcn.c prototypes */
void BobEnterLibrarySymbols(BobInterpreter *c);

/* bobeval.c prototypes */
void BobUseEval(BobInterpreter *c,void *buf,size_t size);
BobValue BobEvalString(BobInterpreter *c,char *str);
BobValue BobEvalStream(BobInterpreter *c,BobStream *s);
int BobLoadFile(BobInterpreter *c,char *fname,BobStream *os);
void BobLoadStream(BobInterpreter *c,BobStream *is,BobStream *os);

/* bobwcode.c prototypes */
int BobCompileFile(BobInterpreter *c,char *iname,char *oname);
int BobCompileString(BobInterpreter *c,char *str,BobStream *os);
int BobCompileStream(BobInterpreter *c,BobStream *is,BobStream *os);

/* bobrcode.c prototypes */
int BobLoadObjectFile(BobInterpreter *c,char *fname,BobStream *os);
int BobLoadObjectStream(BobInterpreter *c,BobStream *s,BobStream *os);

/* bobdebug.c prototypes */
void BobDecodeProcedure(BobInterpreter *c,BobValue method,BobStream *stream);
int BobDecodeInstruction(BobInterpreter *c,BobValue code,int lc,BobStream *stream);

/* boberror.c prototypes */
void BobCallErrorHandler(BobInterpreter *c,int code,...);
void BobShowError(BobInterpreter *c,int code,va_list ap);
char *BobGetErrorText(int code);

#endif
