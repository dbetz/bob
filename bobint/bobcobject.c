/* bobcobject.c - 'CObject' handler */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <string.h>
#include "bob.h"

/* COBJECT */

#define CObjectNext(o)                  (((BobCObject *)o)->next)  
#define SetCObjectNext(o,v)             (((BobCObject *)o)->next = (v))  

/* CObject handlers */
static int GetCObjectProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue);
static int SetCObjectProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value);
static BobValue CObjectNewInstance(BobInterpreter *c,BobValue parent);
static BobValue CPtrObjectNewInstance(BobInterpreter *c,BobValue parent);
static long CObjectSize(BobValue obj);
static void CObjectScan(BobInterpreter *c,BobValue obj);

/* CObject dispatch */
BobDispatch BobCObjectDispatch = {
    "CObject",
    &BobObjectDispatch,
    GetCObjectProperty,
    SetCObjectProperty,
    CObjectNewInstance,
    BobDefaultPrint,
    CObjectSize,
    BobDefaultCopy,
    CObjectScan,
    BobDefaultHash
};

/* BobCObjectP - is this value a cobject? */
int BobCObjectP(BobValue val)
{
    BobDispatch *d = BobGetDispatch(val);
    return d->size == CObjectSize;
}

/* GetCObjectProperty - CObject get property handler */
static int GetCObjectProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue *pValue)
{
    BobValue p;

    /* look for a local property */
    if ((p = BobFindProperty(c,obj,tag,NULL,NULL)) != NULL) {
		*pValue = BobPropertyValue(p);
        return TRUE;
    }

    /* look for a class property */
    else {
        BobDispatch *d;

        /* look for a method in the CObject parent chain */
        for (d = BobQuickGetDispatch(obj); d != NULL; d = d->parent) {
            if ((p = BobFindProperty(c,d->object,tag,NULL,NULL)) != NULL) {
		        BobValue propValue = BobPropertyValue(p);
		        if (BobVPMethodP(propValue)) {
			        BobVPMethod *method = (BobVPMethod *)propValue;
                    if (method->getHandler) {
				        *pValue = (*method->getHandler)(c,obj);
                        return TRUE;
                    }
			        else
				        BobCallErrorHandler(c,BobErrWriteOnlyProperty,tag);
		        }
                else {
			        *pValue = propValue;
                    return TRUE;
                }
            }
        }
	}

    /* not found */
    return FALSE;
}

/* SetCObjectProperty - CObject set property handler */
static int SetCObjectProperty(BobInterpreter *c,BobValue obj,BobValue tag,BobValue value)
{
    BobIntegerType hashValue,i;
    BobValue p;

    /* look for a local property */
    if ((p = BobFindProperty(c,obj,tag,&hashValue,&i)) != NULL) {
		BobSetPropertyValue(p,value);
        return TRUE;
    }

    /* look for a class property */
    else {
        BobDispatch *d;

        /* look for a method in the CObject parent chain */
        for (d = BobQuickGetDispatch(obj); d != NULL; d = d->parent) {
	        if ((p = BobFindProperty(c,d->object,tag,NULL,NULL)) != NULL) {
		        BobValue propValue = BobPropertyValue(p);
		        if (BobVPMethodP(propValue)) {
			        BobVPMethod *method = (BobVPMethod *)propValue;
                    if (method->setHandler) {
				        (*method->setHandler)(c,obj,value);
                        return TRUE;
                    }
			        else
				        BobCallErrorHandler(c,BobErrReadOnlyProperty,tag);
		        }
                else {
			        BobAddProperty(c,obj,tag,value,hashValue,i);
                    return TRUE;
                }
            }
        }
	}

    /* add a property */
    BobAddProperty(c,obj,tag,value,hashValue,i);
    return TRUE;
}

/* CObjectNewInstance - CObject new instance handler */
static BobValue CObjectNewInstance(BobInterpreter *c,BobValue parent)
{
    BobDispatch *d = (BobDispatch *)BobCObjectValue(parent);
    return BobMakeCObject(c,d);
}

/* CPtrObjectNewInstance - CPtrObject new instance handler */
static BobValue CPtrObjectNewInstance(BobInterpreter *c,BobValue parent)
{
    BobDispatch *d = (BobDispatch *)BobCObjectValue(parent);
    BobValue obj = BobMakeCObject(c,d);
    BobSetCObjectValue(obj,NULL);
    return obj;
}

/* CObjectSize - CObject size handler */
static long CObjectSize(BobValue obj)
{
    BobDispatch *d = BobQuickGetDispatch(obj);
    return sizeof(BobCObject) + d->dataSize;
}

/* CObjectScan - CObject scan handler */
static void CObjectScan(BobInterpreter *c,BobValue obj)
{
    SetCObjectNext(obj,c->newSpace->cObjects);
    c->newSpace->cObjects = obj;
    BobObjectDispatch.scan(c,obj);
}

/* BobMakeCObjectType - make a new cobject type */
BobDispatch *BobMakeCObjectType(BobInterpreter *c,BobDispatch *parent,char *typeName,BobCMethod *methods,BobVPMethod *properties,long size)
{
    BobDispatch *d;

    /* make and initialize the type dispatch structure */
    if (!(d = BobMakeDispatch(c,typeName,&BobCObjectDispatch)))
        return NULL;
    d->parent = parent;
    d->dataSize = size;

    /* make the type object */
    d->object = BobMakeCPtrObject(c,c->typeDispatch,d);

    /* enter the methods and properties */
    BobEnterCObjectMethods(c,d,methods,properties);

    /* return the new type */
    return d;
}

/* BobMakeCPtrObjectType - make a new cobject type */
BobDispatch *BobMakeCPtrObjectType(BobInterpreter *c,BobDispatch *parent,char *typeName,BobCMethod *methods,BobVPMethod *properties)
{
    BobDispatch *d = BobMakeCObjectType(c,parent,typeName,methods,properties,sizeof(BobCPtrObject) - sizeof(BobCObject));
    if (d) d->newInstance = CPtrObjectNewInstance;
    return d;
}

/* BobEnterCObjectMethods - add methods and properties to a cobject type */
void BobEnterCObjectMethods(BobInterpreter *c,BobDispatch *d,BobCMethod *methods,BobVPMethod *properties)
{
    /* enter the methods */
    if (methods)
        BobEnterMethods(c,d->object,methods);

    /* enter the virtual properties */
    if (properties)
        BobEnterVPMethods(c,d->object,properties);
}

/* BobMakeCObject - make a new cobject value */
BobValue BobMakeCObject(BobInterpreter *c,BobDispatch *d)
{
    BobValue new;
    new = BobAllocate(c,sizeof(BobCObject) + d->dataSize);
    BobSetDispatch(new,d);
    SetCObjectNext(new,c->newSpace->cObjects);
    c->newSpace->cObjects = new;
    BobSetObjectClass(new,c->nilValue);
    BobSetObjectProperties(new,c->nilValue);
    BobSetObjectPropertyCount(new,0);
    return new;
}

/* BobMakeCPtrObject - make a new pointer cobject value */
BobValue BobMakeCPtrObject(BobInterpreter *c,BobDispatch *d,void *ptr)
{
    BobValue new = BobMakeCObject(c,d);
    BobSetCObjectValue(new,ptr);
    return new;
}

/* BobDestroyUnreachableCObjects - destroy unreachable cobjects */
void BobDestroyUnreachableCObjects(BobInterpreter *c)
{
    BobValue obj = c->oldSpace->cObjects;
    while (obj != NULL) {
        if (!BobBrokenHeartP(obj)) {
            BobDispatch *d = BobQuickGetDispatch(obj);
            if (d->destroy) {
				void *value = BobCObjectValue(obj);
                if (value)
					(*d->destroy)(c,obj);
			}
        }
        obj = CObjectNext(obj);
    }
    c->oldSpace->cObjects = NULL;
}

/* BobDestroyAllCObjects - destroy all cobjects */
void BobDestroyAllCObjects(BobInterpreter *c)
{
    BobValue obj = c->newSpace->cObjects;
    while (obj != NULL) {
        if (!BobBrokenHeartP(obj)) {
            BobDispatch *d = BobQuickGetDispatch(obj);
            if (d->destroy) {
				void *value = BobCObjectValue(obj);
                if (value) {
					(*d->destroy)(c,value);
                    BobSetCObjectValue(obj,NULL);
                }
			}
        }
        obj = CObjectNext(obj);
    }
    c->newSpace->cObjects = NULL;
}

/* VIRTUAL PROPERTY METHOD */

#define SetVPMethodName(o,v)             (((BobVPMethod *)o)->name = (v))  
#define SetVPMethodGetHandler(o,v)       (((BobVPMethod *)o)->getHandler = (v))
#define SetVPMethodSetHandler(o,v)       (((BobVPMethod *)o)->setHandler = (v))

/* VPMethod handlers */
static int VPMethodPrint(BobInterpreter *c,BobValue val,BobStream *s);
static long VPMethodSize(BobValue obj);
static BobValue VPMethodCopy(BobInterpreter *c,BobValue obj);

/* VPMethod dispatch */
BobDispatch BobVPMethodDispatch = {
    "VPMethod",
    &BobVPMethodDispatch,
    BobDefaultGetProperty,
    BobDefaultSetProperty,
    BobDefaultNewInstance,
    VPMethodPrint,
    VPMethodSize,
    VPMethodCopy,
    BobDefaultScan,
    BobDefaultHash
};

/* VPMethodPrint - VPMethod print handler */
static int VPMethodPrint(BobInterpreter *c,BobValue val,BobStream *s)
{
    return BobStreamPutS("<VPMethod-",s) == 0
        && BobStreamPutS(BobVPMethodName(val),s) == 0
        && BobStreamPutC('>',s) == '>';
}

/* VPMethodSize - VPMethod size handler */
static long VPMethodSize(BobValue obj)
{
    return sizeof(BobVPMethod);
}

/* VPMethodCopy - VPMethod copy handler */
static BobValue VPMethodCopy(BobInterpreter *c,BobValue obj)
{
    return obj;
}

/* BobMakeVPMethod - make a new c method value */
BobValue BobMakeVPMethod(BobInterpreter *c,char *name,BobVPGetHandler getHandler,BobVPSetHandler setHandler)
{
    BobValue new;
    new = BobAllocate(c,sizeof(BobVPMethod));
    BobSetDispatch(new,&BobVPMethodDispatch);
    SetVPMethodName(new,name);
    SetVPMethodGetHandler(new,getHandler);
    SetVPMethodSetHandler(new,setHandler);
    return new;
}

/* BobGetVirtualProperty - get a property value that might be virtual */
int BobGetVirtualProperty(BobInterpreter *c,BobValue obj,BobValue parent,BobValue tag,BobValue *pValue)
{
	BobValue p;
    if ((p = BobFindProperty(c,parent,tag,NULL,NULL)) != NULL) {
		BobValue propValue = BobPropertyValue(p);
		if (BobVPMethodP(propValue)) {
			BobVPMethod *method = (BobVPMethod *)propValue;
            if (method->getHandler) {
				*pValue = (*method->getHandler)(c,obj);
                return TRUE;
            }
			else
				BobCallErrorHandler(c,BobErrWriteOnlyProperty,tag);
		}
        else {
			*pValue = propValue;
            return TRUE;
        }
	}
    return FALSE;
}

/* BobSetVirtualProperty - set a property value that might be virtual */
int BobSetVirtualProperty(BobInterpreter *c,BobValue obj,BobValue parent,BobValue tag,BobValue value)
{
    BobIntegerType hashValue,i;
	BobValue p;
	if ((p = BobFindProperty(c,parent,tag,&hashValue,&i)) != NULL) {
		BobValue propValue = BobPropertyValue(p);
		if (BobVPMethodP(propValue)) {
			BobVPMethod *method = (BobVPMethod *)propValue;
            if (method->setHandler) {
				(*method->setHandler)(c,obj,value);
                return TRUE;
            }
			else
				BobCallErrorHandler(c,BobErrReadOnlyProperty,tag);
		}
	}
    return FALSE;
}
