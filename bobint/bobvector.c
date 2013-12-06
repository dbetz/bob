/* bobvector.c - 'Vector' handler */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <string.h>
#include "bob.h"

/* method handlers */
static BobValue BIF_initialize(BobInterpreter *c);
static BobValue BIF_Clone(BobInterpreter *c);
static BobValue BIF_Push(BobInterpreter *c);
static BobValue BIF_PushFront(BobInterpreter *c);
static BobValue BIF_Pop(BobInterpreter *c);
static BobValue BIF_PopFront(BobInterpreter *c);

/* virtual property methods */
static BobValue BIF_size(BobInterpreter *c,BobValue obj);
static void BIF_Set_size(BobInterpreter *c,BobValue obj,BobValue value);

/* Vector methods */
static BobCMethod methods[] = {
BobMethodEntry( "initialize",       BIF_initialize      ),
BobMethodEntry( "Clone",            BIF_Clone           ),
BobMethodEntry( "Push",             BIF_Push            ),
BobMethodEntry( "PushFront",        BIF_PushFront       ),
BobMethodEntry( "Pop",              BIF_Pop             ),
BobMethodEntry( "PopFront",         BIF_PopFront        ),
BobMethodEntry(	0,                  0                   )
};

/* Vector properties */
static BobVPMethod properties[] = {
BobVPMethodEntry( "size",           BIF_size,           BIF_Set_size        ),
BobVPMethodEntry( 0,                0,					0					)
};

/* prototypes */
static BobValue ResizeVector(BobInterpreter *c,BobValue obj,BobIntegerType newSize);

/* BobInitVector - initialize the 'Vector' object */
void BobInitVector(BobInterpreter *c)
{
    c->vectorObject = BobEnterType(c,"Vector",&BobVectorDispatch);
    BobEnterMethods(c,c->vectorObject,methods);
    BobEnterVPMethods(c,c->vectorObject,properties);
}

/* BIF_initialize - built-in method 'initialize' */
static BobValue BIF_initialize(BobInterpreter *c)
{
    long size = 0;
    BobValue obj;
    BobParseArguments(c,"V=*|i",&obj,&BobVectorDispatch,&size);
    return ResizeVector(c,obj,size);
}

/* BIF_Clone - built-in method 'Clone' */
static BobValue BIF_Clone(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobVectorDispatch);
    return BobCloneVector(c,obj);
}

/* BIF_Push - built-in method 'Push' */
static BobValue BIF_Push(BobInterpreter *c)
{
    BobValue obj,val;
    BobIntegerType size;
    BobParseArguments(c,"V=*V",&obj,&BobVectorDispatch,&val);
    size = BobVectorSize(obj);
    BobCPush(c,val);
    obj = ResizeVector(c,obj,size + 1);
    if (BobMovedVectorP(obj))
        obj = BobVectorForwardingAddr(obj);
    BobSetVectorElementI(obj,size,BobTop(c));
    return BobPop(c);
}

/* BIF_PushFront - built-in method 'PushFront' */
static BobValue BIF_PushFront(BobInterpreter *c)
{
    BobValue obj,val,*p;
    BobIntegerType size;
    BobParseArguments(c,"V=*V",&obj,&BobVectorDispatch,&val);
    size = BobVectorSize(obj);
    BobCPush(c,val);
    obj = ResizeVector(c,obj,size + 1);
    if (BobMovedVectorP(obj))
        obj = BobVectorForwardingAddr(obj);
    for (p = BobVectorAddress(obj) + size; --size >= 0; --p)
        *p = p[-1];
    BobSetVectorElementI(obj,0,BobTop(c));
    return BobPop(c);
}

/* BIF_Pop - built-in method 'Pop' */
static BobValue BIF_Pop(BobInterpreter *c)
{
    BobValue obj,vector,val;
    BobIntegerType size;
    BobParseArguments(c,"V=*",&obj,&BobVectorDispatch);
    if (BobMovedVectorP(obj))
        vector = BobVectorForwardingAddr(obj);
    else
        vector = obj;
    size = BobVectorSizeI(vector);
    if (size <= 0)
        BobCallErrorHandler(c,BobErrStackEmpty,obj);
    val = BobVectorElementI(vector,--size);
    BobSetVectorSize(vector,size);
    return val;
}

/* BIF_PopFront - built-in method 'PopFront' */
static BobValue BIF_PopFront(BobInterpreter *c)
{
    BobValue obj,vector,val,*p;
    BobIntegerType size;
    BobParseArguments(c,"V=*",&obj,&BobVectorDispatch);
    if (BobMovedVectorP(obj))
        vector = BobVectorForwardingAddr(obj);
    else
        vector = obj;
    size = BobVectorSizeI(vector);
    if (size <= 0)
        BobCallErrorHandler(c,BobErrStackEmpty,obj);
    val = BobVectorElementI(vector,0);
    BobSetVectorSize(vector,--size);
    for (p = BobVectorAddress(vector); --size >= 0; ++p)
        *p = p[1];
    return val;
}

/* BIF_size - built-in property 'size' */
static BobValue BIF_size(BobInterpreter *c,BobValue obj)
{
    return BobMakeInteger(c,BobVectorSize(obj));
}

/* BIF_Set_size - built-in property 'size' */
static void BIF_Set_size(BobInterpreter *c,BobValue obj,BobValue value)
{
    if (!BobIntegerP(value))
        BobTypeError(c,value);
    ResizeVector(c,obj,BobIntegerValue(value));
}

/* VECTOR */

/* Vector handlers */
static int GetVectorProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
static int SetVectorProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
static BobValue VectorNewInstance(BobInterpreter *c,BobValue parent);
static long VectorSize(BobValue obj);
static void VectorScan(BobInterpreter *c,BobValue obj);

/* Vector dispatch */
BobDispatch BobVectorDispatch = {
    "Vector",
    &BobVectorDispatch,
    GetVectorProperty,
    SetVectorProperty,
    VectorNewInstance,
    BobDefaultPrint,
    VectorSize,
    BobDefaultCopy,
    VectorScan,
    BobDefaultHash
};

/* GetVectorProperty - Vector get property handler */
static int GetVectorProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    if (BobIntegerP(tag)) {
        BobIntegerType i;
        if ((i = BobIntegerValue(tag)) < 0 || i >= BobVectorSizeI(obj))
            BobCallErrorHandler(c,BobErrIndexOutOfBounds,tag);
        *pValue = BobVectorElementI(obj,i);
        return TRUE;
    }
    return BobGetVirtualProperty(c,obj,c->vectorObject,tag,pValue);
}

/* SetVectorProperty - Vector set property handler */
static int SetVectorProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
    if (BobIntegerP(tag)) {
        BobIntegerType i;
        if ((i = BobIntegerValue(tag)) < 0)
            BobCallErrorHandler(c,BobErrIndexOutOfBounds,tag);
        else if (i >= BobVectorSizeI(obj)) {
            BobCPush(c,value);
            obj = ResizeVector(c,obj,i + 1);
            if (BobMovedVectorP(obj))
                obj = BobVectorForwardingAddr(obj);
            value = BobPop(c);
        }
        BobSetVectorElementI(obj,i,value);
        return TRUE;
    }
    return BobSetVirtualProperty(c,obj,c->vectorObject,tag,value);
}

/* VectorNewInstance - create a new vector */
static BobValue VectorNewInstance(BobInterpreter *c,BobValue parent)
{
    return BobMakeVector(c,0);
}

/* VectorSize - Vector size handler */
static long VectorSize(BobValue obj)
{
    return sizeof(BobVector) + BobVectorMaxSize(obj) * sizeof(BobValue);
}

/* VectorScan - Vector scan handler */
static void VectorScan(BobInterpreter *c,BobValue obj)
{
    long i;
    for (i = 0; i < BobVectorSizeI(obj); ++i)
        BobSetVectorElementI(obj,i,BobCopyValue(c,BobVectorElementI(obj,i)));
}

/* MOVED VECTOR */

/* MovedVector handlers */
static int GetMovedVectorProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
static int SetMovedVectorProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
static BobValue MovedVectorCopy(BobInterpreter *c,BobValue obj);

/* MovedVector dispatch */
BobDispatch BobMovedVectorDispatch = {
    "MovedVector",
    &BobVectorDispatch,
    GetMovedVectorProperty,
    SetMovedVectorProperty,
    BobDefaultNewInstance,
    BobDefaultPrint,
    VectorSize,
    MovedVectorCopy,
    BobDefaultScan,
    BobDefaultHash
};

/* GetMovedVectorProperty - MovedVector get property handler */
static int GetMovedVectorProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    return GetVectorProperty(c,BobVectorForwardingAddr(obj),tag,pValue);
}

/* SetMovedVectorProperty - MovedVector set property handler */
static int SetMovedVectorProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
    BobValue resizedVector = BobVectorForwardingAddr(obj);
    if (BobIntegerP(tag)) {
        BobIntegerType i;
        if ((i = BobIntegerValue(tag)) < 0)
            BobCallErrorHandler(c,BobErrIndexOutOfBounds,tag);
        else if (i >= BobVectorSizeI(resizedVector)) {
            BobCPush(c,value);
            resizedVector = ResizeVector(c,obj,i + 1);
            if (BobMovedVectorP(resizedVector))
                resizedVector = BobVectorForwardingAddr(resizedVector);
            value = BobPop(c);
        }
        BobSetVectorElementI(resizedVector,i,value);
        return TRUE;
    }
    return BobSetVirtualProperty(c,obj,c->vectorObject,tag,value);
}

/* MovedVectorCopy - MovedVector scan handler */
static BobValue MovedVectorCopy(BobInterpreter *c,BobValue obj)
{
    BobValue newObj = BobCopyValue(c,BobVectorForwardingAddr(obj));
    BobSetDispatch(obj,&BobBrokenHeartDispatch);
    BobBrokenHeartSetForwardingAddr(obj,newObj);
    return newObj;
}

/* BobMakeFixedVectorValue - make a new vector value */
BobValue BobMakeFixedVectorValue(BobInterpreter *c,BobDispatch *type,int size)
{
    long allocSize = sizeof(BobFixedVector) + size * sizeof(BobValue);
    BobValue new = BobAllocate(c,allocSize);
    BobValue *p = BobFixedVectorAddress(new);
    BobSetDispatch(new,type);
    while (--size >= 0)
        *p++ = c->nilValue;
    return new;
}

/* BASIC VECTOR */

/* BobBasicVectorSizeHandler - BasicVector size handler */
long BobBasicVectorSizeHandler(BobValue obj)
{
    return sizeof(BobBasicVector) + BobBasicVectorSize(obj) * sizeof(BobValue);
}

/* BobBasicVectorScanHandler - BasicVector scan handler */
void BobBasicVectorScanHandler(BobInterpreter *c,BobValue obj)
{
    long i;
    for (i = 0; i < BobBasicVectorSize(obj); ++i)
        BobSetBasicVectorElement(obj,i,BobCopyValue(c,BobBasicVectorElement(obj,i)));
}

/* BobMakeBasicVector - make a new vector value */
BobValue BobMakeBasicVector(BobInterpreter *c,BobDispatch *type,BobIntegerType size)
{
    long allocSize = sizeof(BobBasicVector) + size * sizeof(BobValue);
    BobValue new = BobAllocate(c,allocSize);
    BobValue *p = BobBasicVectorAddress(new);
    BobSetDispatch(new,type);
    BobSetBasicVectorSize(new,size);
    while (--size >= 0)
        *p++ = c->nilValue;
    return new;
}

/* BobMakeVector - make a new vector value */
BobValue BobMakeVector(BobInterpreter *c,BobIntegerType size)
{
    long allocSize = sizeof(BobVector) + size * sizeof(BobValue);
    BobValue *p,new = BobAllocate(c,allocSize);
    BobSetDispatch(new,&BobVectorDispatch);
    BobSetVectorSize(new,size);
    BobSetVectorMaxSize(new,size);
    p = BobVectorAddress(new);
    while (--size >= 0)
        *p++ = c->nilValue;
    return new;
}

/* BobCloneVector - clone an existing vector */
BobValue BobCloneVector(BobInterpreter *c,BobValue obj)
{
    BobIntegerType size = BobVectorSize(obj);
    long allocSize = sizeof(BobVector) + size * sizeof(BobValue);
    BobValue *src,*dst,new = BobAllocate(c,allocSize);
    BobSetDispatch(new,&BobVectorDispatch);
    BobSetVectorSize(new,size);
    BobSetVectorMaxSize(new,size);
    src = BobVectorAddress(obj);
    dst = BobVectorAddress(new);
    while (--size >= 0)
        *dst++ = *src++;
    return new;
}

/* ResizeVector - resize a vector */
static BobValue ResizeVector(BobInterpreter *c,BobValue obj,BobIntegerType newSize)
{
    BobIntegerType size;
    BobValue resizeVector;

    /* handle a vector that has already been moved */
    if (BobMovedVectorP(obj))
        resizeVector = BobVectorForwardingAddr(obj);
    else
        resizeVector = obj;
    
    /* make sure the size is really changing */
    if ((size = BobVectorSizeI(resizeVector)) != newSize) {

        /* check for extra existing space */
        if (newSize <= BobVectorMaxSize(resizeVector)) {

            /* fill the extra space with nil */
            if (newSize > size) {
                BobValue *dst = BobVectorAddressI(resizeVector) + size;
                while (++size <= newSize)
                    *dst++ = c->nilValue;
            }

            /* store the new vector size */
            BobSetVectorSize(resizeVector,newSize);
        }

        /* expand the vector */
        else {
            BobValue newVector,*src,*dst;
            BobIntegerType allocSize;

            /* try expanding by a fraction of the current size */
            allocSize = size / BobVectorExpandDivisor;

            /* but make sure we expand by at least BobVectorExpandMinimum */
            if (allocSize < BobVectorExpandMinimum)
                allocSize = BobVectorExpandMinimum;

            /* and at most BobVectorExpandMaximum */
            if (allocSize > BobVectorExpandMaximum)
                allocSize = BobVectorExpandMaximum;

            /* but at least what we need */
            if ((allocSize += size) < newSize)
                allocSize = newSize;

            /* make a new vector */
            BobCheck(c,2);
            BobPush(c,obj);
            BobPush(c,resizeVector);
            newVector = BobMakeVector(c,allocSize);
            BobSetVectorSize(newVector,newSize);
            resizeVector = BobPop(c);
            obj = BobPop(c);

            /* copy the data from the old to the new vector */
            src = BobVectorAddress(resizeVector);
            dst = BobVectorAddress(newVector);
            while (--size >= 0)
                *dst++ = *src++;

            /* set the forwarding address of the old vector */
            BobSetDispatch(obj,&BobMovedVectorDispatch);
            BobSetVectorForwardingAddr(obj,newVector);
        }
    }

    /* return the resized vector */
    return obj;
}

/* BobVectorSize - get the size of a vector */
BobIntegerType BobVectorSize(BobValue obj)
{
    if (BobMovedVectorP(obj))
        obj = BobVectorForwardingAddr(obj);
    return BobVectorSizeI(obj);
}

/* BobVectorAddress - get the address of the vector data */
BobValue *BobVectorAddress(BobValue obj)
{
    if (BobMovedVectorP(obj))
        obj = BobVectorForwardingAddr(obj);
    return BobVectorAddressI(obj);
}

/* BobVectorElement - get a vector element */
BobValue BobVectorElement(BobValue obj,BobIntegerType i)
{
    if (BobMovedVectorP(obj))
        obj = BobVectorForwardingAddr(obj);
    return BobVectorElementI(obj,i);
}

/* BobSetVectorElement - set a vector element */
void BobSetVectorElement(BobValue obj,BobIntegerType i,BobValue val)
{
    if (BobMovedVectorP(obj))
        obj = BobVectorForwardingAddr(obj);
    BobSetVectorElementI(obj,i,val);
}
