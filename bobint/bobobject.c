/* bobobject.c - 'Object' handler */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include "bob.h"

/* method handlers */
static BobValue BIF_initialize(BobInterpreter *c);
static BobValue BIF_Class(BobInterpreter *c);
static BobValue BIF_Clone(BobInterpreter *c);
static BobValue BIF_Exists(BobInterpreter *c);
static BobValue BIF_ExistsLocally(BobInterpreter *c);
static BobValue BIF_Send(BobInterpreter *c);
static BobValue BIF_Show(BobInterpreter *c);

/* Object methods */
static BobCMethod methods[] = {
BobMethodEntry( "initialize",       BIF_initialize      ),
BobMethodEntry( "Class",            BIF_Class           ),
BobMethodEntry( "Clone",            BIF_Clone           ),
BobMethodEntry( "Exists",           BIF_Exists          ),
BobMethodEntry( "ExistsLocally",    BIF_ExistsLocally   ),
BobMethodEntry( "Send",             BIF_Send            ),
BobMethodEntry( "Show",             BIF_Show            ),
BobMethodEntry(	0,                  0                   )
};

/* BobInitObject - initialize the 'Object' object */
void BobInitObject(BobInterpreter *c)
{
    /* make the base of the object inheritance tree */
    c->objectValue = BobEnterObject(c,"Object",c->nilValue,methods);
}

/* BIF_initialize - built-in method 'initialize' */
static BobValue BIF_initialize(BobInterpreter *c)
{
    BobCheckArgCnt(c,2);
    BobCheckType(c,1,BobObjectP);
    return BobGetArg(c,1);
}

/* BIF_Class - built-in function 'Class' */
static BobValue BIF_Class(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobObjectDispatch);
    return BobObjectClass(obj);
}

/* BIF_Clone - built-in method 'Clone' */
static BobValue BIF_Clone(BobInterpreter *c)
{
    BobValue obj;
    BobParseArguments(c,"V=*",&obj,&BobObjectDispatch);
    return BobCloneObject(c,obj);
}

/* BIF_Exists - built-in method 'Exists' */
static BobValue BIF_Exists(BobInterpreter *c)
{
    BobValue obj,tag;
    BobParseArguments(c,"V=*V",&obj,&BobObjectDispatch,&tag);
    while (BobObjectP(obj)) {
        if (BobFindProperty(c,obj,tag,NULL,NULL))
            return c->trueValue;
        obj = BobObjectClass(obj);
    }
    return c->falseValue;
}

/* BIF_ExistsLocally - built-in method 'ExistsLocally' */
static BobValue BIF_ExistsLocally(BobInterpreter *c)
{
    BobValue obj,tag;
    BobParseArguments(c,"V=*V",&obj,&BobObjectDispatch,&tag);
    return BobToBoolean(c,BobFindProperty(c,obj,tag,NULL,NULL) != NULL);
}

/* BIF_Send - built-in method 'Send' */
static BobValue BIF_Send(BobInterpreter *c)
{
    BobIntegerType i,vcnt,argc;
    BobValue argv;
    BobCheckArgMin(c,4);
    BobCheckType(c,1,BobObjectP);
    BobCheckType(c,BobArgCnt(c),BobVectorP);
    argv = BobGetArg(c,BobArgCnt(c));
    if (BobMovedVectorP(argv))
        argv = BobVectorForwardingAddr(argv);
    vcnt = BobVectorSizeI(argv);
    argc = BobArgCnt(c) + vcnt - 2;
    BobCheck(c,argc + 1);
    BobPush(c,BobGetArg(c,1));
    BobPush(c,BobGetArg(c,3));
    BobPush(c,BobGetArg(c,1));
    for (i = 4; i < BobArgCnt(c); ++i)
        BobPush(c,BobGetArg(c,i));
    for (i = 0; i < vcnt; ++i)
        BobPush(c,BobVectorElementI(argv,i));
    return BobInternalSend(c,argc);
}

/* BIF_Show - built-in method 'Show' */
static BobValue BIF_Show(BobInterpreter *c)
{
    BobStream *s = c->standardOutput;
    BobValue obj,props;
    BobParseArguments(c,"V=*|P=",&obj,&BobObjectDispatch,&s,BobFileDispatch);
    props = BobObjectProperties(obj);
    BobStreamPutS("Class: ",s);
    BobPrint(c,BobObjectClass(obj),s);
    BobStreamPutC('\n',s);
    if (BobObjectPropertyCount(obj)) {
        BobStreamPutS("Properties:\n",s);
        if (BobHashTableP(props)) {
            BobIntegerType cnt = BobHashTableSize(props);
            BobIntegerType i;
            for (i = 0; i < cnt; ++i) {
                BobValue prop = BobHashTableElement(props,i);
                for (; prop != c->nilValue; prop = BobPropertyNext(prop)) {
                    BobStreamPutS("  ",s);
                    BobPrint(c,BobPropertyTag(prop),s);
                    BobStreamPutS(": ",s);
                    BobPrint(c,BobPropertyValue(prop),s);
                    BobStreamPutC('\n',s);
                }
            }
        }
        else {
            for (; props != c->nilValue; props = BobPropertyNext(props)) {
                BobStreamPutS("  ",s);
                BobPrint(c,BobPropertyTag(props),s);
                BobStreamPutS(": ",s);
                BobPrint(c,BobPropertyValue(props),s);
                BobStreamPutC('\n',s);
            }
        }
    }
    return obj;
}

/* OBJECT */

#define IncObjectPropertyCount(o)       (++((BobObject *)o)->propertyCount)

/* Object handlers */
static int GetObjectProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
static int SetObjectProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
static void ObjectScan(BobInterpreter *c,BobValue obj);
static long ObjectSize(BobValue obj);
static BobValue CopyPropertyTable(BobInterpreter *c,BobValue table);
static BobValue CopyPropertyList(BobInterpreter *c,BobValue plist);
static void CreateHashTable(BobInterpreter *c,BobValue obj,BobValue p);
static int ExpandHashTable(BobInterpreter *c,BobValue obj,BobIntegerType i);

/* Object dispatch */
BobDispatch BobObjectDispatch = {
    "Object",
    &BobObjectDispatch,
    GetObjectProperty,
    SetObjectProperty,
    BobMakeObject,
    BobDefaultPrint,
    ObjectSize,
    BobDefaultCopy,
    ObjectScan,
    BobDefaultHash
};

/* BobGetProperty - recursively search for a property value */
int BobGetProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    while (!BobGetProperty1(c,obj,tag,pValue))
        if (!BobObjectP(obj) || (obj = BobObjectClass(obj)) == NULL)
            return FALSE;
    return TRUE;
}

/* GetObjectProperty - Object get property handler */
static int GetObjectProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    BobValue p;
    if (!(p = BobFindProperty(c,obj,tag,NULL,NULL)))
        return FALSE;
    *pValue = BobPropertyValue(p);
    return TRUE;
}

/* SetObjectProperty - Object set property handler */
static int SetObjectProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
    BobIntegerType hashValue,i;
    BobValue p;
    if (!(p = BobFindProperty(c,obj,tag,&hashValue,&i)))
        BobAddProperty(c,obj,tag,value,hashValue,i);
    else
        BobSetPropertyValue(p,value);
    return TRUE;
}

/* ObjectSize - Object size handler */
static long ObjectSize(BobValue obj)
{
    return sizeof(BobObject);
}

/* ObjectScan - Object scan handler */
static void ObjectScan(BobInterpreter *c,BobValue obj)
{
    BobSetObjectClass(obj,BobCopyValue(c,BobObjectClass(obj)));
    BobSetObjectProperties(obj,BobCopyValue(c,BobObjectProperties(obj)));
}

/* BobMakeObject - make a new object */
BobValue BobMakeObject(BobInterpreter *c,BobValue parent)
{
    BobValue new;
    BobCPush(c,parent);
    new = BobAllocate(c,sizeof(BobObject));
    BobSetDispatch(new,&BobObjectDispatch);
    BobSetObjectClass(new,BobPop(c));
    BobSetObjectProperties(new,c->nilValue);
    BobSetObjectPropertyCount(new,0);
    return new;
}

/* BobCloneObject - clone an existing object */
BobValue BobCloneObject(BobInterpreter *c,BobValue obj)
{
    BobValue properties;
    BobCheck(c,2);
    BobPush(c,obj);
    BobPush(c,BobMakeObject(c,BobObjectClass(obj)));
    properties = BobObjectProperties(c->sp[1]);
    if (BobHashTableP(properties))
        BobSetObjectProperties(BobTop(c),CopyPropertyTable(c,properties));
    else
        BobSetObjectProperties(BobTop(c),CopyPropertyList(c,properties));
    BobSetObjectPropertyCount(BobTop(c),BobObjectPropertyCount(c->sp[1]));
    obj = BobPop(c);
    BobDrop(c,1);
    return obj;
}

/* BobFindProperty - find a property of a non-inherited object property */
BobValue BobFindProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobIntegerType *pHashValue,BobIntegerType *pIndex)
{
    BobValue p = BobObjectProperties(obj);
    if (BobHashTableP(p)) {
        BobIntegerType hashValue = BobHashValue(tag);
        BobIntegerType i = hashValue & (BobHashTableSize(p) - 1);
        p = BobHashTableElement(p,i);
        if (pHashValue) *pHashValue = hashValue;
        if (pIndex) *pIndex = i;
    }
    else {
        if (pIndex) *pIndex = -1;
    }
    for (; p != c->nilValue; p = BobPropertyNext(p))
        if (BobEql(BobPropertyTag(p),tag))
            return p;
    return NULL;
}

/* CopyPropertyTable - copy the property hash table of an object */
static BobValue CopyPropertyTable(BobInterpreter *c,BobValue table)
{
    BobIntegerType size = BobHashTableSize(table);
    BobIntegerType i;
    BobCheck(c,2);
    BobPush(c,BobMakeHashTable(c,size));
    BobPush(c,table);
    for (i = 0; i < size; ++i) {
        BobValue properties = CopyPropertyList(c,BobHashTableElement(BobTop(c),i));
        BobSetHashTableElement(c->sp[1],i,properties);
    }
    BobDrop(c,1);
    return BobPop(c);
}

/* CopyPropertyList - copy the property list of an object */
static BobValue CopyPropertyList(BobInterpreter *c,BobValue plist)
{
    BobCheck(c,2);
    BobPush(c,c->nilValue);
    BobPush(c,plist);
    for (; BobTop(c) != c->nilValue; BobSetTop(c,BobPropertyNext(BobTop(c)))) {
        BobValue new = BobMakeProperty(c,BobPropertyTag(BobTop(c)),BobPropertyValue(BobTop(c)));
        BobSetPropertyNext(new,c->sp[1]);
        c->sp[1] = new;
    }
    BobDrop(c,1);
    return BobPop(c);
}

/* BobAddProperty - add a property to an object */
void BobAddProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value,BobIntegerType hashValue,BobIntegerType i)
{
    BobValue p;
    BobCPush(c,obj);
    p = BobMakeProperty(c,tag,value);
    obj = BobPop(c);
    if (i >= 0) {
        BobIntegerType currentSize = BobHashTableSize(BobObjectProperties(obj));
        if (BobObjectPropertyCount(obj) >= currentSize * BobHashTableExpandThreshold) {
            BobCheck(c,2);
            BobPush(c,obj);
            BobPush(c,p);
            i = ExpandHashTable(c,c->sp[1],hashValue);
            p = BobPop(c);
            obj = BobPop(c);
        }
        BobSetPropertyNext(p,BobHashTableElement(BobObjectProperties(obj),i));
        BobSetHashTableElement(BobObjectProperties(obj),i,p);
    }
    else {
        if (BobObjectPropertyCount(obj) >= BobHashTableCreateThreshold)
            CreateHashTable(c,obj,p);
        else {
            BobSetPropertyNext(p,BobObjectProperties(obj));
            BobSetObjectProperties(obj,p);
        }
    }
    IncObjectPropertyCount(obj);
}

/* CreateHashTable - create an object hash table and enter a property */
static void CreateHashTable(BobInterpreter *c,BobValue obj,BobValue p)
{
    BobIntegerType i;
    BobValue table;
    BobCheck(c,2);
    BobPush(c,p);
    BobPush(c,obj);
    table = BobMakeHashTable(c,BobHashTableCreateThreshold);
    obj = BobPop(c);
    p = BobObjectProperties(obj);
    BobSetObjectProperties(obj,table);
    while (p != c->nilValue) {
        BobValue next = BobPropertyNext(p);
        i = BobHashValue(BobPropertyTag(p)) & (BobHashTableCreateThreshold - 1);
        BobSetPropertyNext(p,BobHashTableElement(table,i));
        BobSetHashTableElement(table,i,p);
        p = next;
    }
    p = BobPop(c);
    i = BobHashValue(BobPropertyTag(p)) & (BobHashTableCreateThreshold - 1);
    BobSetPropertyNext(p,BobHashTableElement(table,i));
    BobSetHashTableElement(table,i,p);
}

/* ExpandHashTable - expand an object hash table and return the new hash index */
static int ExpandHashTable(BobInterpreter *c,BobValue obj,BobIntegerType hashValue)
{
    BobIntegerType oldSize,newSize;
    oldSize = BobHashTableSize(BobObjectProperties(obj));
    newSize = oldSize << 1;
    if (newSize <= BobHashTableExpandMaximum) {
        BobValue oldTable,newTable;
        BobIntegerType j;
        BobPush(c,obj);
        newTable = BobMakeHashTable(c,newSize);
        oldTable = BobObjectProperties(BobTop(c));
        for (j = 0; j < oldSize; ++j) {
            BobValue p = BobHashTableElement(oldTable,j);
            BobValue new0 = c->nilValue;
            BobValue new1 = c->nilValue;
            while (p != c->nilValue) {
                BobValue next = BobPropertyNext(p);
                if (BobHashValue(BobPropertyTag(p)) & oldSize) {
                    BobSetPropertyNext(p,new1);
                    new1 = p;
                }
                else {
                    BobSetPropertyNext(p,new0);
                    new0 = p;
                }
                p = next;
            }
            BobSetHashTableElement(newTable,j,new0);
            BobSetHashTableElement(newTable,j + oldSize,new1);
        }
        BobSetObjectProperties(BobPop(c),newTable);
    }
    return hashValue & (newSize - 1);
}

/* PROPERTY */

#define SetPropertyTag(o,v)             BobSetFixedVectorElement(o,0,v)

/* Property handlers */
static long PropertySize(BobValue obj);
static void PropertyScan(BobInterpreter *c,BobValue obj);

/* Property dispatch */
BobDispatch BobPropertyDispatch = {
    "Property",
    &BobPropertyDispatch,
    BobDefaultGetProperty,
    BobDefaultSetProperty,
    BobDefaultNewInstance,
    BobDefaultPrint,
    PropertySize,
    BobDefaultCopy,
    PropertyScan,
    BobDefaultHash
};

/* PropertySize - Property size handler */
static long PropertySize(BobValue obj)
{
    return sizeof(BobFixedVector) + BobPropertySize * sizeof(BobValue);
}

/* PropertyScan - Property scan handler */
static void PropertyScan(BobInterpreter *c,BobValue obj)
{
    long i;
    for (i = 0; i < BobPropertySize; ++i)
        BobSetFixedVectorElement(obj,i,BobCopyValue(c,BobFixedVectorElement(obj,i)));
}

/* BobMakeProperty - make a property */
BobValue BobMakeProperty(BobInterpreter *c,BobValue key,BobValue value)
{
    BobValue new;
    BobCheck(c,2);
    BobPush(c,value);
    BobPush(c,key);
    new = BobMakeFixedVectorValue(c,&BobPropertyDispatch,BobPropertySize);
    SetPropertyTag(new,BobPop(c));
    BobSetPropertyValue(new,BobPop(c));
    return new;
}
