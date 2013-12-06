/* bobenv.c - environment types */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include "bob.h"

/* ENVIRONMENT */

/* Environment dispatch */
BobDispatch BobEnvironmentDispatch = {
    "Environment",
    &BobEnvironmentDispatch,
    BobDefaultGetProperty,
    BobDefaultSetProperty,
    BobDefaultNewInstance,
    BobDefaultPrint,
    BobBasicVectorSizeHandler,
    BobDefaultCopy,
    BobBasicVectorScanHandler,
    BobDefaultHash
};

/* BobMakeEnvironment - make an environment value */
BobValue BobMakeEnvironment(BobInterpreter *c,long size)
{
    return BobMakeBasicVector(c,&BobEnvironmentDispatch,size);
}

/* STACK ENVIRONMENT */

/* StackEnvironment handlers */
static BobValue StackEnvironmentCopy(BobInterpreter *c,BobValue obj);

/* StackEnvironment dispatch */
BobDispatch BobStackEnvironmentDispatch = {
    "StackEnvironment",
    &BobEnvironmentDispatch,
    BobDefaultGetProperty,
    BobDefaultSetProperty,
    BobDefaultNewInstance,
    BobDefaultPrint,
    BobBasicVectorSizeHandler,
    StackEnvironmentCopy,
    BobBasicVectorScanHandler,
    BobDefaultHash
};

/* StackEnvironmentCopy - StackEnvironment copy handler */
static BobValue StackEnvironmentCopy(BobInterpreter *c,BobValue obj)
{
    /* copying the stack will copy the elements */
    return obj;
}

/* MOVED ENVIRONMENT */

/* MovedEnvironment handlers */
static BobValue MovedEnvironmentCopy(BobInterpreter *c,BobValue obj);

/* MovedEnvironment dispatch */
BobDispatch BobMovedEnvironmentDispatch = {
    "MovedEnvironment",
    &BobEnvironmentDispatch,
    BobDefaultGetProperty,
    BobDefaultSetProperty,
    BobDefaultNewInstance,
    BobDefaultPrint,
    BobBasicVectorSizeHandler,
    MovedEnvironmentCopy,
    BobBasicVectorScanHandler,
    BobDefaultHash
};

/* MovedEnvironmentCopy - MovedEnvironment copy handler */
static BobValue MovedEnvironmentCopy(BobInterpreter *c,BobValue obj)
{
    return BobCopyValue(c,BobMovedEnvForwardingAddr(obj));
}

