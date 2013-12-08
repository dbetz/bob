/* bobheap.c - bob heap management routines */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdlib.h>
#include <string.h>
#include "bob.h"

/* VALUE */

#define ValueSize(o)                    (BobQuickGetDispatch(o)->size(o))
#define ScanValue(c,o)                  (BobQuickGetDispatch(o)->scan(c,o))

/* prototypes */
static void InitInterpreter(BobInterpreter *c);
static BobMemorySpace *InitMemorySpace(void *buf,size_t size);

/* BobMakeInterpreter - make a new interpreter */
BobInterpreter *BobMakeInterpreter(void *buf,size_t size,size_t stackSize)
{
    size_t stackSizeInBytes = stackSize * sizeof(BobValue);
    size_t memorySpaceSize = (size - sizeof(BobInterpreter) - stackSizeInBytes) / 2;
    BobInterpreter *c;
    
    /* make sure there is enough space */
    if (size < sizeof(BobInterpreter) + stackSizeInBytes + 2 * sizeof(BobMemorySpace))
        return NULL;
        
    /* initialize the interpreter */
    c = (BobInterpreter *)buf;
    memset(c,0,sizeof(BobInterpreter));
        
    /* initialize the stack */
    c->stack = (BobValue *)(c + 1);
    c->stackTop = c->stack + stackSize;
    c->fp = (BobFrame *)c->stackTop;
    c->sp = c->stackTop;
    
    /* initialize the semi-spaces */
    c->oldSpace = InitMemorySpace((char *)c->stack + stackSizeInBytes, memorySpaceSize);
    c->newSpace = InitMemorySpace((char *)c->oldSpace + memorySpaceSize, memorySpaceSize);
    c->gcCount = 0;
        
    /* return the new interpreter context */
    return c;
}

/* BobInitInterpreter - initialize a new interpreter */
BobInterpreter *BobInitInterpreter(BobInterpreter *c)
{
    int i;
    
    /* make the symbol table */
    c->symbols = BobMakeHashTable(c,BobSymbolHashTableSize);

    /* make the nil symbol */
    c->nilValue = BobMakeSymbol(c,(unsigned char *)"nil",3);
    BobSetGlobalValue(c->nilValue,c->nilValue);
    BobSetSymbolNext(c->nilValue,c->nilValue);

    /* finish initializing the symbol hash table */
    for (i = 0; i < BobHashTableSize(c->symbols); ++i)
        BobSetHashTableElement(c->symbols,i,c->nilValue);

    /* initialize the vm registers */
    c->val = c->nilValue;
    c->env = c->nilValue;

    /* make the true and false symbols */
    c->trueValue = BobInternCString(c,"true");
    BobSetGlobalValue(c->trueValue,c->trueValue);
    c->falseValue = c->nilValue;
    BobEnterVariable(c,"false",c->falseValue);

    /* create the base of the object heirarchy */
    BobInitObject(c);

    /* initialize the built-in types */
    BobInitTypes(c);
    BobInitMethod(c);
    BobInitVector(c);
    BobInitSymbol(c);
    BobInitString(c);
    BobInitInteger(c);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    BobInitFloat(c);
#endif
    BobAddTypeSymbols(c);

    /* initialize the external types */
    BobInitFile(c);

    /* initialize the interpreter */
    InitInterpreter(c);

    /* return successfully */
    return c;
}

/* BobFreeInterpreter - free an interpreter structure */
void BobFreeInterpreter(BobInterpreter *c)
{
    BobProtectedPtrs *p,*nextp;
    BobDispatch *d,*nextd;

    /* destroy cobjects */
    /*BobDestroyAllCObjects(c)*/;

    /* free the type list */
    for (d = c->types; d != NULL; d = nextd) {
        nextd = d->next;
        BobFreeDispatch(c,d);
    }

    /* free the protected pointer blocks */
    for (p = c->protectedPtrs; p != NULL; p = nextp) {
        nextp = p->next;
        BobFree(c,p);
    }
}

/* InitInterpreter - initialize an interpreter structure */
static void InitInterpreter(BobInterpreter *c)
{
    /* initialize the stack */
    c->fp = (BobFrame *)c->stackTop;
    c->sp = c->stackTop;

    /* initialize the registers */
    c->val = c->nilValue;
    c->env = c->nilValue;
    c->code = NULL;
}

/* InitMemorySpace - initialize a semi-space */
static BobMemorySpace *InitMemorySpace(void *buf,size_t size)
{
    BobMemorySpace *space = (BobMemorySpace *)buf;
    space->base = (unsigned char *)(space + 1);
    space->free = space->base;
    space->top = space->base + size - sizeof(BobMemorySpace);
    space->cObjects = NULL;
    return space;
}

/* BobAllocate - allocate memory for a value */
BobValue BobAllocate(BobInterpreter *c,long size)
{
    BobValue val;
    
    /* look for free space */
    if (c->newSpace->free + size <= c->newSpace->top) {
        val = (BobValue)c->newSpace->free;
        c->newSpace->free += size;
        return val;
    }

    /* collect garbage */
    BobCollectGarbage(c);

    /* look again */
    if (c->newSpace->free + size <= c->newSpace->top) {
        val = (BobValue)c->newSpace->free;
        c->newSpace->free += size;
        return val;
    }

    /* insufficient memory */
    BobInsufficientMemory(c);
    return c->nilValue; /* never reached */
}

/* BobMakeDispatch - make a new type dispatch */
BobDispatch *BobMakeDispatch(BobInterpreter *c,char *typeName,BobDispatch *prototype)
{
    int totalSize = sizeof(BobDispatch) + strlen(typeName) + 1;
    BobDispatch *d;

    /* allocate a new dispatch structure */
    if ((d = (BobDispatch *)BobAlloc(c,totalSize)) == NULL)
        return NULL;
    memset(d,0,totalSize);

    /* initialize */
    *d = *prototype;
    d->baseType = d;
    d->typeName = (char *)d + sizeof(BobDispatch);
    strcpy(d->typeName,typeName);

    /* add the type to the type list */
    d->next = c->types;
    c->types = d;

    /* return the new type */
    return d;
}

/* BobFreeDispatch - free a type dispatch structure */
void BobFreeDispatch(BobInterpreter *c,BobDispatch *d)
{
    BobFree(c,d);
}

/* BobCollectGarbage - garbage collect a heap */
void BobCollectGarbage(BobInterpreter *c)
{
    BobProtectedPtrs *ppb;
    unsigned char *scan;
    BobMemorySpace *ms;
    BobDispatch *d;
    BobValue obj;

	BobStreamPutS("[GC",c->standardError);

    /* reverse the memory spaces */
    ms = c->oldSpace;
    c->oldSpace = c->newSpace;
    c->newSpace = ms;
    ms->free = ms->base;
    
    /* copy the root objects */
    c->nilValue = BobCopyValue(c,c->nilValue);
    c->trueValue = BobCopyValue(c,c->trueValue);
    c->falseValue = BobCopyValue(c,c->falseValue);
    c->symbols = BobCopyValue(c,c->symbols);
    c->objectValue = BobCopyValue(c,c->objectValue);

    /* copy basic types */
    c->methodObject = BobCopyValue(c,c->methodObject);
    c->vectorObject = BobCopyValue(c,c->vectorObject);
    c->symbolObject = BobCopyValue(c,c->symbolObject);
    c->stringObject = BobCopyValue(c,c->stringObject);
    c->integerObject = BobCopyValue(c,c->integerObject);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    c->floatObject = BobCopyValue(c,c->floatObject);
#endif

    /* copy the type list */
    for (d = c->types; d != NULL; d = d->next) {
        if (d->object)
            d->object = BobCopyValue(c,d->object);
    }

    /* copy protected pointers */
    for (ppb = c->protectedPtrs; ppb != NULL; ppb = ppb->next) {
        BobValue **pp = ppb->pointers;
        int count = ppb->count;
        for (; --count >= 0; ++pp)
            **pp = BobCopyValue(c,**pp);
    }
    
    /* copy the stack */
    BobCopyStack(c);
        
    /* copy the current code object */
    if (c->code)
        c->code = BobCopyValue(c,c->code);
    
    /* copy the registers */
    c->val = BobCopyValue(c,c->val);
    c->env = BobCopyValue(c,c->env);
        
    /* copy any user objects */
    if (c->protectHandler)
        (*c->protectHandler)(c,c->protectData);

    /* scan and copy until all accessible objects have been copied */
    scan = c->newSpace->base;
	while (scan < c->newSpace->free) {
        obj = (BobValue)scan;
#if 0
        BobStreamPutS("Scanning ",c->standardOutput);
        BobPrint(c,obj,c->standardOutput);
        BobStreamPutC('\n',c->standardOutput);
#endif
        scan += ValueSize(obj);
        ScanValue(c,obj);
    }
    
    /* fixup cbase and pc */
    if (c->code) {
        long pcoff = c->pc - c->cbase;
        c->cbase = BobStringAddress(BobCompiledCodeBytecodes(c->code));
        c->pc = c->cbase + pcoff;
    }
    
    /* count the garbage collections */
    ++c->gcCount;

    {
		char buf[128];
		sprintf(buf,
				" - %lu bytes free out of %lu, collections %lu]\n",
				(unsigned long)(c->newSpace->top - c->newSpace->free),
				(unsigned long)(c->newSpace->top - c->newSpace->base),
				(unsigned long)c->gcCount);
		BobStreamPutS(buf,c->standardError);
	}
      
    /* destroy any unreachable cobjects */
    BobDestroyUnreachableCObjects(c);
}

/* BobDumpHeap - dump the contents of the bob heap */
void BobDumpHeap(BobInterpreter *c)
{
    unsigned char *scan;
	
    /* first collect the garbage */
    //BobCollectGarbage(c);

    /* dump each heap object */
    scan = c->newSpace->base;
    while (scan < c->newSpace->free) {
        BobValue val = (BobValue)scan;
        scan += ValueSize(val);
        //if (BobCObjectP(val)) {
            BobPrint(c,val,c->standardOutput);
            BobStreamPutC('\n',c->standardOutput);
        //}
    }
}

/* default handlers */

/* BobDefaultGetProperty - get the value of a property */
int BobDefaultGetProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    return FALSE;
}

/* BobDefaultSetProperty - set the value of a property */
int BobDefaultSetProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
	return FALSE;
}

/* BobDefaultNewInstance - create a new instance */
BobValue BobDefaultNewInstance(BobInterpreter *c,BobValue parent)
{
	BobCallErrorHandler(c,BobErrNewInstance,parent);
    return c->nilValue; /* never reached */
}

/* BobDefaultPrint - print an object */
int BobDefaultPrint(BobInterpreter *c,BobValue obj,BobStream *s)
{
    char buf[64];
    sprintf(buf,"%08lx",(long)obj);
    return BobStreamPutC('<',s) == '<'
        && BobStreamPutS(BobTypeName(obj),s) == 0
        && BobStreamPutC('-',s) == '-'
        && BobStreamPutS(buf,s) == 0
        && BobStreamPutC('>',s) == '>';
}

#define NewObjectP(c,obj)   ((unsigned char *)(obj) >= (c)->newSpace->base && (unsigned char *)(obj) < (c)->newSpace->free)

/* BobDefaultCopy - copy an object from old space to new space */
BobValue BobDefaultCopy(BobInterpreter *c,BobValue obj)
{
    long size = ValueSize(obj);
    BobValue newObj;
    
    /* don't copy an object that is already in new space */
    if (NewObjectP(c,obj))
        return obj;

    /* find a place to put the new object */
    newObj = (BobValue)c->newSpace->free;
    
    /* copy the object */
    memcpy(newObj,obj,(size_t)size);
    c->newSpace->free += size;
    
    /* store a forwarding address in the old object */
    BobSetDispatch(obj,&BobBrokenHeartDispatch);
    BobBrokenHeartSetForwardingAddr(obj,newObj);
    
    /* return the new object */
    return newObj;
}

/* BobBobDefaultScan - scan an object without embedded pointers */
void BobDefaultScan(BobInterpreter *c,BobValue obj)
{
}

/* BobDefaultHash - default object hash routine */
BobIntegerType BobDefaultHash(BobValue obj)
{
    return 0;
}

/* BROKEN HEART */

static long BrokenHeartSize(BobValue obj);
static BobValue BrokenHeartCopy(BobInterpreter *c,BobValue obj);

BobDispatch BobBrokenHeartDispatch = {
    "BrokenHeart",
    &BobBrokenHeartDispatch,
    BobDefaultGetProperty,
    BobDefaultSetProperty,
    BobDefaultNewInstance,
    BobDefaultPrint,
    BrokenHeartSize,
    BrokenHeartCopy,
    BobDefaultScan,
    BobDefaultHash
};

static long BrokenHeartSize(BobValue obj)
{
    return sizeof(BobBrokenHeart);
}

static BobValue BrokenHeartCopy(BobInterpreter *c,BobValue obj)
{
    return BobBrokenHeartForwardingAddr(obj);
}

/* BobProtectPointer - protect a pointer from the garbage collector */
int BobProtectPointer(BobInterpreter *c,BobValue *pp)
{
    BobProtectedPtrs *ppb = c->protectedPtrs;

    /* look for space in an existing block */
    while (ppb) {
        if (ppb->count < BobPPSize) {
            ppb->pointers[ppb->count++] = pp;
            *pp = c->nilValue;
            return TRUE;
        }
        ppb = ppb->next;
    }

    /* allocate a new block */
    if ((ppb = (BobProtectedPtrs *)BobAlloc(c,sizeof(BobProtectedPtrs))) == NULL)
        return FALSE;

    /* initialize the new block */
    ppb->next = c->protectedPtrs;
    c->protectedPtrs = ppb;
    ppb->count = 0;

    /* add the pointer to the new block */
    ppb->pointers[ppb->count++] = pp;
    *pp = c->nilValue;
    return TRUE;
}

/* BobUnprotectPointer - unprotect a pointer from the garbage collector */
int BobUnprotectPointer(BobInterpreter *c,BobValue *pp)
{
    BobProtectedPtrs *ppb = c->protectedPtrs;
    while (ppb) {
        BobValue **ppp = ppb->pointers;
        int cnt = ppb->count;
        while (--cnt >= 0) {
            if (*ppp++ == pp) {
                while (--cnt >= 0) {
                    ppp[-1] = *ppp;
                    ++ppp;
                }
                --ppb->count;
                return TRUE;
            }
        }
        ppb = ppb->next;
    }
    return FALSE;
}

static unsigned long totalAlloc = sizeof(BobInterpreter);

/* BobAlloc - allocate memory */
void *BobAlloc(BobInterpreter *c,unsigned long size)
{
    unsigned long totalSize = sizeof(unsigned long) + size;
    unsigned long *p = malloc(totalSize);
    if (p) {
        *p++ = totalSize;
        c->totalMemory += totalSize;
        ++c->allocCount;
        printf("BobAlloc %lu %lu\n", size, totalAlloc);
        totalAlloc += size;
    }
    return (void *)p;
}

/* BobFree - free memory */
void BobFree(BobInterpreter *c,void *p)
{
    unsigned long *p1 = (unsigned long *)p;
    unsigned long totalSize = *--p1;
    if (c) {
        c->totalMemory -= totalSize;
        --c->allocCount;
    }
    free(p1);
}

/* BobInsufficientMemory - report an insufficient memory error */
void BobInsufficientMemory(BobInterpreter *c)
{
    BobCallErrorHandler(c,BobErrInsufficientMemory,0);
}
