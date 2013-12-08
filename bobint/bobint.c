/* bobint.c - bytecode interpreter */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "bob.h"
#include "bobint.h"

/* interpreter dispatch codes */
#define itReturn        1
#define itAbort         2

/* types */
typedef struct FrameDispatch FrameDispatch;

/* frame */
struct BobFrame {
    FrameDispatch *dispatch;
    BobFrame *next;
};

/* frame dispatch structure */
struct FrameDispatch {
    void (*restore)(BobInterpreter *c);
    BobValue *(*copy)(BobInterpreter *c,BobFrame *frame);
};

/* call frame dispatch */
static void CallRestore(BobInterpreter *c);
static BobValue *CallCopy(BobInterpreter *c,BobFrame *frame);
FrameDispatch BobCallCDispatch = {
    CallRestore,
    CallCopy
};

/* call frame */
typedef struct CallFrame CallFrame;
struct CallFrame {
    BobFrame hdr;
    BobValue env;
    BobValue code;
    int pcOffset;
    int argc;
    BobEnvironment stackEnv;
};

/* top frame dispatch */
static void TopRestore(BobInterpreter *c);
FrameDispatch BobTopCDispatch = {
    TopRestore,
    CallCopy
};

/* block frame dispatch */
static void BlockRestore(BobInterpreter *c);
static BobValue *BlockCopy(BobInterpreter *c,BobFrame *frame);
FrameDispatch BobBlockCDispatch = {
    BlockRestore,
    BlockCopy
};

/* block frame */
typedef struct BlockFrame BlockFrame;
struct BlockFrame {
    BobFrame hdr;
    BobValue *fp;
    BobValue env;
    BobEnvironment stackEnv;
};

/* macro to convert a byte size to a stack entry size */
#define WordSize(n) ((n) / sizeof(BobValue))
     
/* prototypes */
static BobValue ExecuteCall(BobInterpreter *c,BobValue fun,int argc,va_list ap);
static void Execute(BobInterpreter *c);
static void UnaryOp(BobInterpreter *c,int op);
static void BinaryOp(BobInterpreter *c,int op);
static int Send(BobInterpreter *c,FrameDispatch *d,int argc);
static int Call(BobInterpreter *c,FrameDispatch *d,int argc);
static void PushFrame(BobInterpreter *c,int size);
static BobValue UnstackEnv(BobInterpreter *c,BobValue env);
static void BadOpcode(BobInterpreter *c,int opcode);
static int CompareObjects(BobInterpreter *c,BobValue obj1,BobValue obj2);
static int CompareStrings(BobValue str1,BobValue str2);
static BobValue ConcatenateStrings(BobInterpreter *c,BobValue str1,BobValue str2);

/* BobPushUnwindTarget - push an unwind target onto the stack */
void BobPushUnwindTarget(BobInterpreter *c,BobUnwindTarget *target)
{
    target->next = c->unwindTarget;
    c->unwindTarget = target;
}

/* BobPopAndUnwind - pop and unwind after an error */
void BobPopAndUnwind(BobInterpreter *c,int value)
{
    BobPopUnwindTarget(c);
    BobUnwind(c,value);
}

/* BobCallFunction - call a function */
BobValue BobCallFunction(BobInterpreter *c,BobValue fun,int argc,...)
{
    BobValue result;
    va_list ap;

    /* call the function */
    va_start(ap,argc);
    result = ExecuteCall(c,fun,argc,ap);
    va_end(ap);

    /* return the result */
    return result;
}

/* ExecuteCall - execute a function call */
static BobValue ExecuteCall(BobInterpreter *c,BobValue fun,int argc,va_list ap)
{
    BobUnwindTarget target;
    int sts,n;
    
    /* setup the unwind target */
    BobPushUnwindTarget(c,&target);
    if ((sts = BobUnwindCatch(c)) != 0) {
        switch (sts) {
        case itReturn:
            BobPopUnwindTarget(c);
            return c->val;
        case itAbort:
            BobPopAndUnwind(c,sts);
        }
    }
        
    /* push the function */
    BobCheck(c,argc + 3);
    BobPush(c,fun);
    BobPush(c,c->nilValue);
    BobPush(c,c->nilValue);

    /* push the arguments */
    for (n = argc; --n >= 0; )
        BobPush(c,va_arg(ap,BobValue));
    va_end(ap);
    
    /* setup the call */
    if (Call(c,&BobTopCDispatch,argc + 2)) {
        BobPopUnwindTarget(c);
        return c->val;
    }

	/* execute the function code */
	Execute(c);
    return 0; /* never reached */
}

/* Execute - execute code */
static void Execute(BobInterpreter *c)
{
    for (;;) {
        BobValue p1,p2,*p;
        unsigned int off;
        long n;
        int i;

        //BobDecodeInstruction(c,c->code,c->pc - c->cbase,c->standardOutput);
        switch (*c->pc++) {
        case BobOpCALL:
            Call(c,&BobCallCDispatch,*c->pc++);
            break;
        case BobOpSEND:
            Send(c,&BobCallCDispatch,*c->pc++);
            break;
        case BobOpRETURN:
        case BobOpUNFRAME:
            (*c->fp->dispatch->restore)(c);
            break;
        case BobOpFRAME:
            PushFrame(c,*c->pc++);
            break;
        case BobOpCFRAME:
            i = *c->pc++;
            BobCheck(c,i);
            for (n = i; --n >= 0; )
                BobPush(c,c->nilValue);
            PushFrame(c,i);
            break;
        case BobOpAFRAME:       /* handled by BobOpCALL */
        case BobOpAFRAMER:
            BadOpcode(c,c->pc[-1]);
            break;
        case BobOpARGSGE:
            i = *c->pc++;
            c->val = BobToBoolean(c,c->argc >= i);
            break;
        case BobOpCLOSE:
            c->env = UnstackEnv(c,c->env);
            c->val = BobMakeMethod(c,c->val,c->env);
            break;
        case BobOpEREF:
            i = *c->pc++;
            for (p2 = c->env; --i >= 0; )
                p2 = BobEnvNextFrame(p2);
            i = BobEnvSize(p2) - *c->pc++;
            c->val = BobEnvElement(p2,i);
            break;
        case BobOpESET:
            i = *c->pc++;
            for (p2 = c->env; --i >= 0; )
                p2 = BobEnvNextFrame(p2);
            i = BobEnvSize(p2) - *c->pc++;
            BobSetEnvElement(p2,i,c->val);
            break;
        case BobOpBRT:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if (BobTrueP(c,c->val))
                c->pc = c->cbase + off;
            break;
        case BobOpBRF:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            if (BobFalseP(c,c->val))
                c->pc = c->cbase + off;
            break;
        case BobOpBR:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->pc = c->cbase + off;
            break;
        case BobOpSWITCH:
            i = *c->pc++;
            i |= *c->pc++ << 8;
            while (--i >= 0) {
                off = *c->pc++;
                off |= *c->pc++ << 8;
                if (BobEql(c->val,BobCompiledCodeLiteral(c->code,off)))
                    break;
                c->pc += 2;
            }
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->pc = c->cbase + off;
            break;
        case BobOpT:
            c->val = c->trueValue;
            break;
        case BobOpNIL:
            c->val = c->nilValue;
            break;
        case BobOpPUSH:
            BobCPush(c,c->val);
            break;
        case BobOpNOT:
            c->val = BobToBoolean(c,!BobTrueP(c,c->val));
            break;
        case BobOpNEG:
            UnaryOp(c,'-');
            break;
        case BobOpADD:
            if (BobStringP(c->val)) {
                p1 = BobPop(c);
                if (!BobStringP(p1)) BobTypeError(c,p1);
                c->val = ConcatenateStrings(c,p1,c->val);
            }
            else
                BinaryOp(c,'+');
            break;
        case BobOpSUB:
            BinaryOp(c,'-');
            break;
        case BobOpMUL:
            BinaryOp(c,'*');
            break;
        case BobOpDIV:
            BinaryOp(c,'/');
            break;
        case BobOpREM:
            BinaryOp(c,'%');
            break;
        case BobOpINC:
            UnaryOp(c,'I');
            break;
        case BobOpDEC:
            UnaryOp(c,'D');
            break;
        case BobOpBAND:
            BinaryOp(c,'&');
            break;
        case BobOpBOR:
            BinaryOp(c,'|');
            break;
        case BobOpXOR:
            BinaryOp(c,'^');
            break;
        case BobOpBNOT:
            UnaryOp(c,'~');
            break;
        case BobOpSHL:
            BinaryOp(c,'L');
            break;
        case BobOpSHR:
            BinaryOp(c,'R');
            break;
        case BobOpLT:
            p1 = BobPop(c);
            c->val = BobToBoolean(c,CompareObjects(c,p1,c->val) < 0);
            break;
        case BobOpLE:
            p1 = BobPop(c);
            c->val = BobToBoolean(c,CompareObjects(c,p1,c->val) <= 0);
            break;
        case BobOpEQ:
            p1 = BobPop(c);
            c->val = BobToBoolean(c,BobEql(p1,c->val));
            break;
        case BobOpNE:
            p1 = BobPop(c);
            c->val = BobToBoolean(c,!BobEql(p1,c->val));
            break;
        case BobOpGE:
            p1 = BobPop(c);
            c->val = BobToBoolean(c,CompareObjects(c,p1,c->val) >= 0);
            break;
        case BobOpGT:
            p1 = BobPop(c);
            c->val = BobToBoolean(c,CompareObjects(c,p1,c->val) > 0);
            break;
        case BobOpLIT:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            c->val = BobCompiledCodeLiteral(c->code,off);
            break;
        case BobOpGREF:
            off = *c->pc++;
            off |= *c->pc++ << 8;
			c->val = BobGlobalValue(BobCompiledCodeLiteral(c->code,off));
            break;
        case BobOpGSET:
            off = *c->pc++;
            off |= *c->pc++ << 8;
            BobSetGlobalValue(BobCompiledCodeLiteral(c->code,off),c->val);
            break;
        case BobOpGETP:
            p1 = BobPop(c);
            if (!BobGetProperty(c,p1,c->val,&c->val))
                BobCallErrorHandler(c,BobErrNoProperty,p1,c->val);
            break;
        case BobOpSETP:
            p2 = BobPop(c);
            p1 = BobPop(c);
            if (!BobSetProperty(c,p1,p2,c->val))
                BobCallErrorHandler(c,BobErrNoProperty,p1,p2);
            break;
        case BobOpVREF:
            p1 = BobPop(c);
            if (!BobGetProperty(c,p1,c->val,&c->val))
                BobCallErrorHandler(c,BobErrNoProperty,p1,c->val);
            break;
        case BobOpVSET:
            p2 = BobPop(c);
            p1 = BobPop(c);
            if (!BobSetProperty(c,p1,p2,c->val))
                BobCallErrorHandler(c,BobErrNoProperty,p1,c->val);
            break;
        case BobOpDUP2:
            BobCheck(c,2);
            c->sp -= 2;
            c->sp[1] = c->val;
            BobSetTop(c,c->sp[2]);
            break;
        case BobOpDROP:
            c->val = BobPop(c);
            break;
        case BobOpDUP:
            BobCheck(c,1);
            c->sp -= 1;
            BobSetTop(c,c->sp[1]);
            break;
        case BobOpOVER:
            BobCheck(c,1);
            c->sp -= 1;
            BobSetTop(c,c->sp[2]);
            break;
        case BobOpNEWOBJECT:
            c->val = BobNewInstance(c,c->val);
            break;
        case BobOpNEWVECTOR:
            if (!BobIntegerP(c->val)) BobTypeError(c,c->val);
            n = BobIntegerValue(c->val);
            c->val = BobMakeVector(c,n);
            p = BobVectorAddressI(c->val) + n;
            while (--n >= 0)
                *--p = BobPop(c);
            break;
        default:
            BadOpcode(c,c->pc[-1]);
            break;
        }
    }
}

/* UnaryOp - handle unary opcodes */
static void UnaryOp(BobInterpreter *c,int op)
{
    BobValue p1 = c->val;

    if (BobIntegerP(p1)) {
        BobIntegerType i1 = BobIntegerValue(p1);
        BobIntegerType ival;
        switch (op) {
        case '+':
            ival = i1;
            break;
        case '-':
            ival = -i1;
            break;
        case '~':
            ival = ~i1;
            break;
        case 'I':
            ival = i1 + 1;
            break;
        case 'D':
            ival = i1 - 1;
            break;
        default:
            ival = 0; /* never reached */
            break;
        }
        c->val = BobMakeInteger(c,ival);
    }

#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    else if (BobFloatP(p1)) {
        BobFloatType f1 = BobFloatValue(p1);
        BobFloatType fval;
        switch (op) {
        case '+':
            fval = f1;
            break;
        case '-':
            fval = -f1;
            break;
        case 'I':
            fval = f1 + 1;
            break;
        case 'D':
            fval = f1 - 1;
            break;
        case '~':
            BobTypeError(c,p1);
            /* fall through */
        default:
            fval = 0.0; /* never reached */
            break;
        }
        c->val = BobMakeFloat(c,fval);
    }
#endif

    else
        BobTypeError(c,p1);
}

/* BinaryOp - handle binary opcodes */
static void BinaryOp(BobInterpreter *c,int op)
{
    BobValue p1 = BobPop(c);
    BobValue p2 = c->val;

    if (BobIntegerP(p1) && BobIntegerP(p2)) {
        BobIntegerType i1 = BobIntegerValue(p1);
        BobIntegerType i2 = BobIntegerValue(p2);
        BobIntegerType ival;
        switch (op) {
        case '+':
            ival = i1 + i2;
            break;
        case '-':
            ival = i1 - i2;
            break;
        case '*':
            ival = i1 * i2;
            break;
        case '/':
            ival = i2 == 0 ? 0 : i1 / i2;
            break;
        case '%':
            ival = i2 == 0 ? 0 : i1 % i2;
            break;
        case '&':
            ival = i1 & i2;
            break;
        case '|':
            ival = i1 | i2;
            break;
        case '^':
            ival = i1 ^ i2;
            break;
        case 'L':
            ival = i1 << i2;
            break;
        case 'R':
            ival = i1 >> i2;
            break;
        default:
            ival = 0; /* never reached */
            break;
        }
        c->val = BobMakeInteger(c,ival);
    }
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    else {
        BobFloatType f1,f2,fval;
        if (BobFloatP(p1))
            f1 = BobFloatValue(p1);
        else if (BobIntegerP(p1))
            f1 = (BobFloatType)BobIntegerValue(p1);
        else {
            BobTypeError(c,p1);
            f1 = 0.0; /* never reached */
        }
        if (BobFloatP(p2))
            f2 = BobFloatValue(p2);
        else if (BobIntegerP(p2))
            f2 = (BobFloatType)BobIntegerValue(p2);
        else {
            BobTypeError(c,p2);
            f2 = 0.0; /* never reached */
        }
        switch (op) {
        case '+':
            fval = f1 + f2;
            break;
        case '-':
            fval = f1 - f2;
            break;
        case '*':
            fval = f1 * f2;
            break;
        case '/':
            fval = f2 == 0 ? 0 : f1 / f2;
            break;
        case '%':
        case '&':
        case '|':
        case '^':
        case 'L':
        case 'R':
            BobTypeError(c,p1);
            /* fall through */;
        default:
            fval = 0.0; /* never reached */
            break;
        }
        c->val = BobMakeFloat(c,fval);
    }
#else
    else if (BobIntegerP(p1))
        BobTypeError(c,p2);
    else
        BobTypeError(c,p1);
#endif
}

/* BobInternalSend - internal function to send a message */
BobValue BobInternalSend(BobInterpreter *c,int argc)
{
    BobUnwindTarget target;
    int sts;
    
    /* setup the unwind target */
    BobPushUnwindTarget(c,&target);
    if ((sts = BobUnwindCatch(c)) != 0) {
        switch (sts) {
        case itReturn:
            BobPopUnwindTarget(c);
            return c->val;
        case itAbort:
            BobPopAndUnwind(c,sts);
        }
    }
        
    /* setup the call */
    if (Send(c,&BobTopCDispatch,argc)) {
        BobPopUnwindTarget(c);
        return c->val;
    }

	/* execute the function code */
	Execute(c);
    return 0; /* never reached */
}

/* Send - setup to call a method */
static int Send(BobInterpreter *c,FrameDispatch *d,int argc)
{
    BobValue next = c->sp[argc - 2];
    BobValue selector = c->sp[argc - 1];
    BobValue method;

    /* find the method */
    while (!BobGetProperty1(c,next,selector,&method))
        if (!BobObjectP(next) || (next = BobObjectClass(next)) == NULL)
            BobCallErrorHandler(c,BobErrNoMethod,c->sp[argc],selector);

    /* setup the 'this' parameter */
    c->sp[argc - 1] = c->sp[argc];

    /* setup the '_next' parameter */
    if (!BobObjectP(next) || (c->sp[argc - 2] = BobObjectClass(next)) == NULL)
        c->sp[argc - 2] = c->nilValue;

    /* set the method */
    c->sp[argc] = method;

    /* call the method */
    return Call(c,d,argc);
}

/* BobInternalCall - internal function to call a function */
BobValue BobInternalCall(BobInterpreter *c,int argc)
{
    BobUnwindTarget target;
    int sts;
    
    /* setup the unwind target */
    BobPushUnwindTarget(c,&target);
    if ((sts = BobUnwindCatch(c)) != 0) {
        switch (sts) {
        case itReturn:
            BobPopUnwindTarget(c);
            return c->val;
        case itAbort:
            BobPopAndUnwind(c,sts);
        }
    }
        
    /* setup the call */
    if (Call(c,&BobTopCDispatch,argc)) {
        BobPopUnwindTarget(c);
        return c->val;
    }

	/* execute the function code */
	Execute(c);
    return 0; /* never reached */
}

/* Call - setup to call a function */
static int Call(BobInterpreter *c,FrameDispatch *d,int argc)
{
    int rflag,rargc,oargc,targc,n;
    BobValue method = c->sp[argc];
    unsigned char *cbase,*pc;
    int oldArgC = c->argc;
    CallFrame *frame;
    BobValue code;

	/* setup the argument list */
    c->argv = &c->sp[argc];
    c->argc = argc;

    /* handle built-in methods */
    if (BobCMethodP(method)) {
        c->val = (*BobCMethodHandler(method))(c);
        BobDrop(c,argc + 1);
        return TRUE;
    }
    
    /* otherwise, it had better be a bytecode method */
    else if (!BobMethodP(method))
        BobTypeError(c,method);

    /* get the code object */
    code = BobMethodCode(method);
    cbase = pc = BobStringAddress(BobCompiledCodeBytecodes(code));
    
    /* parse the argument frame instruction */
    rflag = *pc++ == BobOpAFRAMER;
    rargc = *pc++; oargc = *pc++;
    targc = rargc + oargc;

    /* check the argument count */
    if (argc < rargc)
        BobTooFewArguments(c);
    else if (!rflag && argc > rargc + oargc)
        BobTooManyArguments(c);
        
    /* fill out the optional arguments */
    if ((n = targc - argc) > 0) {
        BobCheck(c,n);
        while (--n >= 0)
            BobPush(c,c->nilValue);
    }
    
    /* build the rest argument */
    if (rflag) {
        BobValue value,*p;
        int rcnt;
        if ((rcnt = argc - targc) < 0)
            rcnt = 0;
        value = BobMakeVector(c,rcnt);
        p = BobVectorAddressI(value) + rcnt;
        while (--rcnt >= 0)
            *--p = BobPop(c);
        BobCPush(c,value);
        ++targc;
    }
    
    /* reserve space for the call frame */
    BobCheck(c,WordSize(sizeof(CallFrame)) + BobFirstEnvElement);
    
    /* complete the environment frame */
    BobPush(c,c->nilValue);         /* names */
    BobPush(c,BobMethodEnv(method));/* nextFrame */
    
    /* initialized the frame */
    frame = (CallFrame *)(c->sp - WordSize(sizeof(CallFrame)));
    frame->hdr.dispatch = d;
    frame->hdr.next = c->fp;
    frame->env = c->env;
    frame->code = c->code;
    frame->pcOffset = c->pc - c->cbase;
    frame->argc = oldArgC;
    BobSetDispatch(&frame->stackEnv,&BobStackEnvironmentDispatch);
    BobSetEnvSize(&frame->stackEnv,BobFirstEnvElement + targc);

    /* establish the new frame */
    c->env = (BobValue)&frame->stackEnv;
    c->fp = (BobFrame *)frame;
    c->sp = (BobValue *)frame;

    /* setup the new method */
    c->code = code;
    c->cbase = cbase;
    c->pc = pc;

    /* didn't complete the call */
    return FALSE;
}

/* TopRestore - restore a top continuation */
static void TopRestore(BobInterpreter *c)
{
    CallRestore(c);
    BobUnwind(c,itReturn);
}

/* CallRestore - restore a call continuation */
static void CallRestore(BobInterpreter *c)
{
    CallFrame *frame = (CallFrame *)c->fp;
    BobValue env = c->env;
    
    /* restore the previous frame */
    c->fp = frame->hdr.next;
    c->env = frame->env;
    if ((c->code = frame->code) != NULL) {
        c->cbase = BobStringAddress(BobCompiledCodeBytecodes(c->code));
        c->pc = c->cbase + frame->pcOffset;
    }
    c->argc = frame->argc;
    
    /* fixup moved environments */
    if (c->env && BobMovedEnvironmentP(c->env))
        c->env = BobMovedEnvForwardingAddr(c->env);

    /* reset the stack pointer */
    c->sp = (BobValue *)frame;
    BobDrop(c,WordSize(sizeof(CallFrame)) + BobEnvSize(env) + 1);
}

/* CallCopy - copy a call continuation during garbage collection */
static BobValue *CallCopy(BobInterpreter *c,BobFrame *frame)
{
    CallFrame *call = (CallFrame *)frame;
    BobValue env = (BobValue)&call->stackEnv;
    BobValue *data = BobEnvAddress(env);
    BobIntegerType count = BobEnvSize(env);
    call->env = BobCopyValue(c,call->env);
    if (call->code)
        call->code = BobCopyValue(c,call->code);
    if (BobStackEnvironmentP(env)) {
        while (--count >= 0) {
            *data = BobCopyValue(c,*data);
            ++data;
        }
    }
    else
        data += count;
    return data;
}

/* PushFrame - push a frame on the stack */
static void PushFrame(BobInterpreter *c,int size)
{
    BlockFrame *frame;
    
    /* reserve space for the call frame */
    BobCheck(c,WordSize(sizeof(BlockFrame)) + BobFirstEnvElement);
    
    /* complete the environment frame */
    BobPush(c,c->nilValue);     /* names */
    BobPush(c,c->env);          /* nextFrame */

    /* initialized the frame */
    frame = (BlockFrame *)(c->sp - WordSize(sizeof(BlockFrame)));
    frame->hdr.dispatch = &BobBlockCDispatch;
    frame->hdr.next = c->fp;
    frame->env = c->env;
    BobSetDispatch(&frame->stackEnv,&BobStackEnvironmentDispatch);
    BobSetEnvSize(&frame->stackEnv,BobFirstEnvElement + size);

    /* establish the new frame */
    c->env = (BobValue)&frame->stackEnv;
    c->fp = (BobFrame *)frame;
    c->sp = (BobValue *)frame;
}

/* BlockRestore - restore a frame continuation */
static void BlockRestore(BobInterpreter *c)
{
    BlockFrame *frame = (BlockFrame *)c->fp;
    
    /* restore the previous frame */
    c->fp = frame->hdr.next;
    c->env = frame->env;

    /* fixup moved environments */
    if (c->env && BobMovedEnvironmentP(c->env))
        c->env = BobMovedEnvForwardingAddr(c->env);

    /* reset the stack pointer */
    c->sp = (BobValue *)frame;
    BobDrop(c,WordSize(sizeof(BlockFrame)) + BobEnvSize(&frame->stackEnv));
}

/* BlockCopy - copy a frame continuation during garbage collection */
static BobValue *BlockCopy(BobInterpreter *c,BobFrame *frame)
{
    BlockFrame *block = (BlockFrame *)frame;
    BobValue env = (BobValue)&block->stackEnv;
    BobValue *data = BobEnvAddress(env);
    BobIntegerType count = BobEnvSize(env);
    block->env = BobCopyValue(c,block->env);
    if (BobStackEnvironmentP(env)) {
        while (--count >= 0) {
            *data = BobCopyValue(c,*data);
            ++data;
        }
    }
    else
        data += count;
    return data;
}

/* UnstackEnv - unstack the environment */
static BobValue UnstackEnv(BobInterpreter *c,BobValue env)
{
    BobValue new,*src,*dst;
    long size;

    /* initialize */
    BobCheck(c,3);
    BobPush(c,c->nilValue);
    BobPush(c,c->nilValue);

    /* copy each stack environment frame to the heap */
    while (BobStackEnvironmentP(env)) {

        /* allocate a new frame */
        BobPush(c,env);
        size = BobEnvSize(env);
        new = BobMakeEnvironment(c,size);
        env = BobPop(c);
        
        /* copy the data */
        src = BobEnvAddress(env);
        dst = BobEnvAddress(new);
        while (--size >= 0)
            *dst++ = *src++;

        /* link the new frame into the new environment */
        if (BobTop(c) == c->nilValue)
            c->sp[1] = new;
        else
            BobSetEnvNextFrame(BobTop(c),new);
        BobSetTop(c,new);
        
        /* get next frame */
        BobPush(c,BobEnvNextFrame(env));
        
        /* store the forwarding address */
        BobSetDispatch(env,&BobMovedEnvironmentDispatch);
        BobSetMovedEnvForwardingAddr(env,new);
        
        /* move ahead to the next frame */
        env = BobPop(c);
    }

    /* link the first heap frame into the new environment */
    if (BobTop(c) == c->nilValue)
        c->sp[1] = env;
    else
        BobSetEnvNextFrame(BobTop(c),env);
    BobDrop(c,1);

    /* return the new environment */
    return BobPop(c);
}

/* BobTypeError - signal a 'type' error */
void BobTypeError(BobInterpreter *c,BobValue v)
{
    BobCallErrorHandler(c,BobErrTypeError,v);
}

/* BobStackOverflow - signal a 'stack overflow' error */
void BobStackOverflow(BobInterpreter *c)
{
    BobCallErrorHandler(c,BobErrStackOverflow,0);
}

/* BobTooManyArguments - signal a 'too many arguments' error */
void BobTooManyArguments(BobInterpreter *c)
{
    BobCallErrorHandler(c,BobErrTooManyArguments,c->sp[c->argc]);
}

/* BobTooFewArguments - signal a 'too few arguments' error */
void BobTooFewArguments(BobInterpreter *c)
{
    BobCallErrorHandler(c,BobErrTooFewArguments,c->sp[c->argc]);
}

/* BadOpcode - signal a 'bad opcode' error */
static void BadOpcode(BobInterpreter *c,int opcode)
{
    BobCallErrorHandler(c,BobErrBadOpcode,opcode);
}

/* BobAbort - abort the current call to the interpreter */
void BobAbort(BobInterpreter *c)
{
    BobUnwind(c,itAbort);
}

/* BobEql - compare two objects for equality */
int BobEql(BobValue obj1,BobValue obj2)
{
    if (BobIntegerP(obj1)) {
        if (BobIntegerP(obj2))
            return BobIntegerValue(obj1) == BobIntegerValue(obj2);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
        else if (BobFloatP(obj2))
            return (BobFloatType)BobIntegerValue(obj1) == BobFloatValue(obj2);
#endif
        else
            return FALSE;
    }
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    else if (BobFloatP(obj1)) {
        if (BobFloatP(obj2))
            return BobFloatValue(obj1) == BobFloatValue(obj2);
        else if (BobIntegerP(obj2))
            return BobFloatValue(obj1) == (BobFloatType)BobIntegerValue(obj2);
        else
            return FALSE;
    }
#endif
    else if (BobStringP(obj1))
        return BobStringP(obj2) && CompareStrings(obj1,obj2) == 0;
    else
        return obj1 == obj2;
}

/* CompareObjects - compare two objects */
static int CompareObjects(BobInterpreter *c,BobValue obj1,BobValue obj2)
{
    if (BobIntegerP(obj1) && BobIntegerP(obj2)) {
        BobIntegerType diff = BobIntegerValue(obj1) - BobIntegerValue(obj2);
        return diff < 0 ? -1 : diff == 0 ? 0 : 1;
    }
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    else if (BobFloatP(obj1)) {
        BobFloatType diff;
        if (BobFloatP(obj2))
            diff = BobFloatValue(obj1) - BobFloatValue(obj2);
        else if (BobIntegerP(obj2))
            diff = BobFloatValue(obj1) - (BobFloatType)BobIntegerValue(obj2);
        else {
            BobTypeError(c,obj2);
            diff = 0; /* never reached */
        }
        return diff < 0 ? -1 : diff == 0 ? 0 : 1;
    }
    else if (BobFloatP(obj2)) {
        BobFloatType diff;
        if (BobIntegerP(obj1))
            diff = (BobFloatType)BobIntegerValue(obj1) - BobFloatValue(obj2);
        else {
            BobTypeError(c,obj1);
            diff = 0; /* never reached */
        }
        return diff < 0 ? -1 : diff == 0 ? 0 : 1;
    }
#endif
    else if (BobStringP(obj1) && BobStringP(obj2))
        return CompareStrings(obj1,obj2);
    else {
        BobTypeError(c,obj1);
        return 0; /* never reached */
    }
}

/* CompareStrings - compare two strings */
static int CompareStrings(BobValue str1,BobValue str2)
{
    unsigned char *p1 = BobStringAddress(str1);
    long len1 = BobStringSize(str1);
    unsigned char *p2 = BobStringAddress(str2);
    long len2 = BobStringSize(str2);
    while (len1 > 0 && len2 > 0 && *p1++ == *p2++)
        --len1, --len2;
    if (len1 == 0) return len2 == 0 ? 0 : -1;
    if (len2 == 0) return 1;
    return *--p1 - *--p2 < 0 ? -1 : *p1 == *p2 ? 0 : 1;
}

/* ConcatenateStrings - concatenate two strings */
static BobValue ConcatenateStrings(BobInterpreter *c,BobValue str1,BobValue str2)
{
    unsigned char *src,*dst;
    long len1 = BobStringSize(str1);
    long len2 = BobStringSize(str2);
    long len = len1 + len2;
    BobValue new;
    BobCheck(c,2);
    BobPush(c,str1);
    BobPush(c,str2);
    new = BobMakeString(c,NULL,len);
    dst = BobStringAddress(new);
    src = BobStringAddress(c->sp[1]);
    while (--len1 >= 0)
        *dst++ = *src++;
    src = BobStringAddress(BobTop(c));
    while (--len2 >= 0)
        *dst++ = *src++;
    BobDrop(c,2);
    return new;
}

/* BobCopyStack - copy the stack for the garbage collector */
void BobCopyStack(BobInterpreter *c)
{
    BobFrame *fp = c->fp;
    BobValue *sp = c->sp;
    while (sp < c->stackTop) {
        if (sp >= (BobValue *)fp) {
            sp = (*fp->dispatch->copy)(c,fp);
            fp = fp->next;
        }
        else {
            *sp = BobCopyValue(c,*sp);
            ++sp;
        }
    }
}

/* BobStackTrace - display a stack trace */
void BobStackTrace(BobInterpreter *c)
{
    int calledFromP = FALSE;
    BobFrame *fp = c->fp;
    if (c->code) {
        BobValue name = BobCompiledCodeName(c->code);
        BobStreamPutS("Happened in ",c->standardOutput);
        if (name == c->nilValue)
            BobPrint(c,c->code,c->standardOutput);
        else
            BobDisplay(c,name,c->standardOutput);
        BobStreamPrintF(c->standardOutput," at %04x\n",c->pc - c->cbase);
    }
    while (fp < (BobFrame *)c->stackTop) {
        CallFrame *frame = (CallFrame *)fp;
        if (frame->hdr.dispatch == &BobCallCDispatch && frame->code) {
            BobValue name = BobCompiledCodeName(frame->code);
            if (!calledFromP) {
                BobStreamPutS("Called from:\n",c->standardOutput);
                calledFromP = TRUE;
            }
            BobStreamPutS("  ",c->standardOutput);
            if (name == c->nilValue)
                BobPrint(c,frame->code,c->standardOutput);
            else
                BobDisplay(c,name,c->standardOutput);
            BobStreamPutC('\n',c->standardOutput);
        }
        fp = fp->next;
    }
}
