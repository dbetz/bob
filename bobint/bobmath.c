/* bobmath.c - math functions */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#ifdef BOB_INCLUDE_FLOAT_SUPPORT

#include <math.h>
#include "bob.h"

/* prototypes */
static BobValue BIF_abs(BobInterpreter *c);
static BobValue BIF_sin(BobInterpreter *c);
static BobValue BIF_cos(BobInterpreter *c);
static BobValue BIF_tan(BobInterpreter *c);
static BobValue BIF_asin(BobInterpreter *c);
static BobValue BIF_acos(BobInterpreter *c);
static BobValue BIF_atan(BobInterpreter *c);
static BobValue BIF_sqrt(BobInterpreter *c);
static BobValue BIF_ceil(BobInterpreter *c);
static BobValue BIF_floor(BobInterpreter *c);
static BobValue BIF_exp(BobInterpreter *c);
static BobValue BIF_log(BobInterpreter *c);
static BobValue BIF_log2(BobInterpreter *c);
static BobValue BIF_log10(BobInterpreter *c);
static BobValue BIF_pow(BobInterpreter *c);

/* function table */
static BobCMethod functionTable[] = {
BobMethodEntry( "abs",              BIF_abs             ),
BobMethodEntry( "sin",              BIF_sin             ),
BobMethodEntry( "cos",              BIF_cos             ),
BobMethodEntry( "tan",              BIF_tan             ),
BobMethodEntry( "asin",             BIF_asin            ),
BobMethodEntry( "acos",             BIF_acos            ),
BobMethodEntry( "atan",             BIF_atan            ),
BobMethodEntry( "sqrt",             BIF_sqrt            ),
BobMethodEntry( "ceil",             BIF_ceil            ),
BobMethodEntry( "floor",            BIF_floor           ),
BobMethodEntry( "exp",              BIF_exp             ),
BobMethodEntry( "log",              BIF_log             ),
BobMethodEntry( "log2",             BIF_log2            ),
BobMethodEntry( "log10",            BIF_log10           ),
BobMethodEntry( "pow",              BIF_pow             ),
BobMethodEntry( 0,					0					)
};

/* local variables */
static BobFloatType oneOverLog2;

/* prototypes */
static BobFloatType FloatValue(BobValue val);

/* BobUseMath - initialize the math functions */
void BobUseMath(BobInterpreter *c)
{
    /* compute the inverse of the log of 2 */
    oneOverLog2 = 1.0 / log(2.0);

    /* enter the built-in functions */
    BobEnterFunctions(c,functionTable);

    /* enter the built-in variables */
    BobEnterVariable(c,"pi",BobMakeFloat(c,(BobFloatType)(4.0 * atan(1.0))));
}

/* BIF_abs - built-in function 'abs' */
static BobValue BIF_abs(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    if (BobIntegerP(BobGetArg(c,3))) {
        BobIntegerType v = BobIntegerValue(BobGetArg(c,3));
        return BobMakeInteger(c,v >= 0 ? v : -v);
    }
    else {
        BobFloatType v = BobFloatValue(BobGetArg(c,3));
        return BobMakeFloat(c,v >= 0 ? v : -v);
    }
}

/* BIF_sin - built-in function 'sin' */
static BobValue BIF_sin(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)sin(FloatValue(BobGetArg(c,3))));
}

/* BIF_cos - built-in function 'cos' */
static BobValue BIF_cos(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)cos(FloatValue(BobGetArg(c,3))));
}

/* BIF_tan - built-in function 'tan' */
static BobValue BIF_tan(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)tan(FloatValue(BobGetArg(c,3))));
}

/* BIF_asin - built-in function 'asin' */
static BobValue BIF_asin(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)asin(FloatValue(BobGetArg(c,3))));
}

/* BIF_acos - built-in function 'acos' */
static BobValue BIF_acos(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)acos(FloatValue(BobGetArg(c,3))));
}

/* BIF_atan - built-in function 'atan' */
static BobValue BIF_atan(BobInterpreter *c)
{
    BobValue val;
    BobCheckType(c,3,BobNumberP);
    switch (BobArgCnt(c)) {
    case 3:
        val = BobMakeFloat(c,(BobFloatType)atan(FloatValue(BobGetArg(c,3))));
        break;
    case 4:
        BobCheckType(c,2,BobNumberP);
        val = BobMakeFloat(c,(BobFloatType)atan2(FloatValue(BobGetArg(c,3)),
                                                 FloatValue(BobGetArg(c,4))));
        break;
    default:
        BobTooManyArguments(c);
        val = c->nilValue; /* never reached */
    }
    return val;
}

/* BIF_sqrt - built-in function 'sqrt' */
static BobValue BIF_sqrt(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)sqrt(FloatValue(BobGetArg(c,3))));
}

/* BIF_ceil - built-in function 'ceil' */
static BobValue BIF_ceil(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)ceil(FloatValue(BobGetArg(c,3))));
}

/* BIF_floor - built-in function 'floor' */
static BobValue BIF_floor(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)floor(FloatValue(BobGetArg(c,3))));
}

/* BIF_exp - built-in function 'exp' */
static BobValue BIF_exp(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)exp(FloatValue(BobGetArg(c,3))));
}

/* BIF_log - built-in function 'log' */
static BobValue BIF_log(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)log(FloatValue(BobGetArg(c,3))));
}

/* BIF_log2 - built-in function 'log2' */
static BobValue BIF_log2(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)log(FloatValue(BobGetArg(c,3))) * oneOverLog2);
}

/* BIF_log10 - built-in function 'log10' */
static BobValue BIF_log10(BobInterpreter *c)
{
    BobCheckArgCnt(c,3);
    BobCheckType(c,3,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)log10(FloatValue(BobGetArg(c,3))));
}

/* BIF_pow - built-in function 'pow' */
static BobValue BIF_pow(BobInterpreter *c)
{
    BobCheckArgCnt(c,4);
    BobCheckType(c,3,BobNumberP);
    BobCheckType(c,4,BobNumberP);
    return BobMakeFloat(c,(BobFloatType)pow(FloatValue(BobGetArg(c,3)),FloatValue(BobGetArg(c,4))));
}

/* FloatValue - convert a value to float */
static BobFloatType FloatValue(BobValue val)
{
    if (BobFloatP(val))
        return BobFloatValue(val);
    return (BobFloatType)BobIntegerValue(val);
}

#endif