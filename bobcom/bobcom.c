/* bobcom.c - the bytecode compiler */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bobcom.h"
#include "bobint.h"

/* local constants */
#define NIL     0       /* fixup list terminator */

/* partial value structure */
typedef struct pvalue {
    void (*fcn)(BobCompiler *,int,struct pvalue *);
    int val,val2;
} PVAL;

/* variable access function codes */
#define LOAD    1
#define STORE   2
#define PUSH    3
#define DUP     4

/* forward declarations */
static void SetupCompiler(BobCompiler *c);
static void do_statement(BobCompiler *c);
static void do_define(BobCompiler *c);
static void define_method(BobCompiler *c,char *name);
static void define_function(BobCompiler *c,char *name);
static void compile_code(BobCompiler *c,char *name);
static void do_if(BobCompiler *c);
static void do_while(BobCompiler *c);
static void do_dowhile(BobCompiler *c);
static void do_for(BobCompiler *c);
static void addbreak(BobCompiler *c,SENTRY *sentry,int lbl);
static int rembreak(BobCompiler *c);
static void do_break(BobCompiler *c);
static void addcontinue(BobCompiler *c,SENTRY *centry,int lbl);
static void remcontinue(BobCompiler *c);
static void do_continue(BobCompiler *c);
static void addswitch(BobCompiler *c,SWENTRY *swentry);
static void remswitch(BobCompiler *c);
static void do_switch(BobCompiler *c);
static void do_case(BobCompiler *c);
static void do_default(BobCompiler *c);
static void UnwindStack(BobCompiler *c,int levels);
static void do_block(BobCompiler *c);
static void do_return(BobCompiler *c);
static void do_try(BobCompiler *c);
static void do_throw(BobCompiler *c);
static void do_test(BobCompiler *c);
static void do_expr(BobCompiler *c);
static void do_init_expr(BobCompiler *c);
static void rvalue(BobCompiler *c,PVAL *pv);
static void chklvalue(BobCompiler *c,PVAL *pv);
static void do_expr1(BobCompiler *c,PVAL *pv);
static void do_expr2(BobCompiler *c,PVAL *pv);
static void do_assignment(BobCompiler *c,PVAL *pv,int op);
static void do_expr3(BobCompiler *c,PVAL *pv);
static void do_expr4(BobCompiler *c,PVAL *pv);
static void do_expr5(BobCompiler *c,PVAL *pv);
static void do_expr6(BobCompiler *c,PVAL *pv);
static void do_expr7(BobCompiler *c,PVAL *pv);
static void do_expr8(BobCompiler *c,PVAL *pv);
static void do_expr9(BobCompiler *c,PVAL *pv);
static void do_expr10(BobCompiler *c,PVAL *pv);
static void do_expr11(BobCompiler *c,PVAL *pv);
static void do_expr12(BobCompiler *c,PVAL *pv);
static void do_expr13(BobCompiler *c,PVAL *pv);
static void do_expr14(BobCompiler *c,PVAL *pv);
static void do_preincrement(BobCompiler *c,PVAL *pv,int op);
static void do_postincrement(BobCompiler *c,PVAL *pv,int op);
static void do_expr15(BobCompiler *c,PVAL *pv);
static void do_primary(BobCompiler *c,PVAL *pv);
static void do_selector(BobCompiler *c);
static void do_prop_reference(BobCompiler *c,PVAL *pv);
static void do_function(BobCompiler *c,PVAL *pv);
static void do_literal(BobCompiler *c,PVAL *pv);
static void do_literal_symbol(BobCompiler *c,PVAL *pv);
static void do_literal_vector(BobCompiler *c,PVAL *pv);
static void do_literal_object(BobCompiler *c,PVAL *pv);
static void do_new_object(BobCompiler *c,PVAL *pv);
static void do_call(BobCompiler *c,PVAL *pv);
static void do_super(BobCompiler *c,PVAL *pv);
static void do_method_call(BobCompiler *c,PVAL *pv);
static void do_index(BobCompiler *c,PVAL *pv);
static void AddArgument(BobCompiler *c,ATABLE *atable,char *name);
static void PushArgFrame(BobCompiler *c,ATABLE *atable);
static void PopArgFrame(BobCompiler *c);
static void FreeArguments(BobCompiler *c);
static int FindArgument(BobCompiler *c,char *name,int *plev,int *poff);
static int addliteral(BobCompiler *c,BobValue lit);
static void frequire(BobCompiler *c,int rtkn);
static void require(BobCompiler *c,int tkn,int rtkn);
static void do_lit_integer(BobCompiler *c,BobIntegerType n);
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static void do_lit_float(BobCompiler *c,BobFloatType n);
#endif
static void do_lit_string(BobCompiler *c,char *str);
static void do_lit_symbol(BobCompiler *c,char *pname);
static int make_lit_string(BobCompiler *c,char *str);
static int make_lit_symbol(BobCompiler *c,char *pname);
static void variable_ref(BobCompiler *c,char *name);
static void findvariable(BobCompiler *c,char *id,PVAL *pv);
static void code_constant(BobCompiler *c,int fcn,PVAL *);
static int load_argument(BobCompiler *c,char *name);
static void code_argument(BobCompiler *c,int fcn,PVAL *);
static void code_property(BobCompiler *c,int fcn,PVAL *);
static void code_variable(BobCompiler *c,int fcn,PVAL *);
static void code_index(BobCompiler *c,int fcn,PVAL *);
static void code_literal(BobCompiler *c,int n);
static int codeaddr(BobCompiler *c);
static int putcbyte(BobCompiler *c,int b);
static int putcword(BobCompiler *c,int w);
static void fixup(BobCompiler *c,int chn,int val);
static void AddLineNumber(BobCompiler *c,int line,int pc);
static LineNumberBlock *AddLineNumberBlock(BobCompiler *c);
static void FreeLineNumbers(BobCompiler *c);
static void DumpLineNumbers(BobCompiler *c);

/* BobMakeCompiler - initialize the compiler */
BobCompiler *BobMakeCompiler(BobInterpreter *ic,void *buf,size_t size,long lsize)
{
    size_t csize = size - sizeof(BobInterpreter);
    BobCompiler *c;

    /* make sure there is enough space */
    if (size < sizeof(BobInterpreter))
        return NULL;
        
    /* initialize the compiler context structure */
    c = (BobCompiler *)buf;
    
    /* allocate and initialize the code buffer */
    c->codebuf = (unsigned char *)(c + 1);
    c->cptr = c->codebuf; c->ctop = c->codebuf + csize;

    /* initialize the line number table */
    c->lineNumbers = c->currentBlock = NULL;

    /* allocate and initialize the literal buffer */
    BobProtectPointer(ic,&c->literalbuf);
    c->literalbuf = BobMakeVector(ic,lsize);
    c->lptr = 0; c->ltop = lsize;

    /* link the compiler and interpreter contexts to each other */
    c->ic = ic;

    /* don't emit line number opcodes */
    c->emitLineNumbersP = FALSE;

    /* return the new compiler context */
    return c;
}

/* BobFreeCompiler - free the compiler structure */
void BobFreeCompiler(BobCompiler *c)
{
    BobUnprotectPointer(c->ic,&c->literalbuf);
}

/* SetupCompiler - setup the compiler context */
static void SetupCompiler(BobCompiler *c)
{
    c->cbase = c->cptr = c->codebuf;
    c->lbase = c->lptr = 0;
    c->bsp = NULL;
    c->csp = NULL;
    c->ssp = NULL;
    c->arguments = NULL;
    c->blockLevel = 0;
}

/* BobCompileExpr - compile a single expression */
BobValue BobCompileExpr(BobInterpreter *ic)
{
    BobCompiler *c = ic->compiler;
    BobValue code,*src,*dst;
    BobUnwindTarget target;
    int sts,tkn;
    long size;
    
    /* initialize the compiler */
    SetupCompiler(c);
    
    /* setup an unwind target */
    BobPushUnwindTarget(ic,&target);
    if ((sts = BobUnwindCatch(ic)) != 0) {
        FreeArguments(c);
        BobPopAndUnwind(ic,sts);
    }
    
    /* check for end of file */
    if ((tkn = BobToken(c)) == T_EOF) {
        BobPopUnwindTarget(ic);
        return NULL;
    }
    BobSaveToken(c,tkn);
    
    /* make dummy function name */
    addliteral(c,ic->nilValue);
    
    /* generate the argument frame */
    c->lineNumberChangedP = FALSE;
    putcbyte(c,BobOpAFRAME);
    putcbyte(c,2);
    putcbyte(c,0);
    c->lineNumberChangedP = TRUE;
    
    /* compile the code */
    do_statement(c);
    putcbyte(c,BobOpRETURN);

    /* make the bytecode array */
    code = BobMakeString(ic,c->cbase,c->cptr - c->cbase);
    
    /* make the compiled code object */
    size = c->lptr - c->lbase;
    code = BobMakeCompiledCode(ic,BobFirstLiteral + size,code);
    src = BobVectorAddress(c->literalbuf) + c->lbase;
    dst = BobCompiledCodeLiterals(code) + BobFirstLiteral;
    while (--size >= 0)
        *dst++ = *src++;
    
    /* make a closure */
    code = BobMakeMethod(ic,code,ic->nilValue);
    
    /* return the function */
    //BobDecodeProcedure(ic,code,ic->standardOutput);
    BobPopUnwindTarget(ic);
    return code;
}

/* do_statement - compile a single statement */
static void do_statement(BobCompiler *c)
{
    int tkn;
    switch (tkn = BobToken(c)) {
    case T_DEFINE:      do_define(c);   break;
    case T_IF:          do_if(c);       break;
    case T_WHILE:       do_while(c);    break;
    case T_DO:          do_dowhile(c);  break;
    case T_FOR:         do_for(c);      break;
    case T_BREAK:       do_break(c);    break;
    case T_CONTINUE:    do_continue(c); break;
    case T_SWITCH:      do_switch(c);   break;
    case T_CASE:        do_case(c);     break;
    case T_DEFAULT:     do_default(c);  break;
    case T_RETURN:      do_return(c);   break;
    case T_TRY:         do_try(c);      break;
    case T_THROW:       do_throw(c);    break;
    case '{':           do_block(c);    break;
    case ';':           ;               break;
    default:            BobSaveToken(c,tkn);
                        do_expr(c);
                        frequire(c,';');  break;
    }
}

/* do_define - compile the 'define' expression */
static void do_define(BobCompiler *c)
{
    char name[256];
    int tkn;
    switch (tkn = BobToken(c)) {
    case T_IDENTIFIER:
        strcpy(name,c->t_token);
        switch (tkn = BobToken(c)) {
        case '.':
            define_method(c,name);
            break;
        default:
            BobSaveToken(c,tkn);
            define_function(c,name);
            break;
        }
        break;
    default:
        BobParseError(c,"Expecting a function or a method definition");
        break;
    }
}

/* define_method - handle method definition statement */
static void define_method(BobCompiler *c,char *name)
{
    char selector[256];
    int tkn;
    
    /* push the class */
    variable_ref(c,name);
    putcbyte(c,BobOpPUSH);
    
    /* get the selector */
    for (;;) {
        frequire(c,T_IDENTIFIER);
        strcpy(selector,c->t_token);
        if ((tkn = BobToken(c)) != '.')
            break;
        do_lit_symbol(c,selector);
        putcbyte(c,BobOpGETP);
        putcbyte(c,BobOpPUSH);
    }

    /* push the selector symbol */
    BobSaveToken(c,tkn);
    do_lit_symbol(c,selector);
    putcbyte(c,BobOpPUSH);
    
    /* compile the code */
    compile_code(c,selector);
    
     /* store the method as the value of the property */
    putcbyte(c,BobOpSETP);
}

/* define_function - handle function definition statement */
static void define_function(BobCompiler *c,char *name)
{
    /* compile the code */
    compile_code(c,name);
    
    /* store the function as the value of the global symbol */
    putcbyte(c,BobOpGSET);
    putcword(c,make_lit_symbol(c,name));
}

/* compile_code - compile a function or method */
static void compile_code(BobCompiler *c,char *name)
{
    BobInterpreter *ic = c->ic;
    int oldLevel,argc,rcnt,ocnt,nxt,tkn;
    BobValue code,*src,*dst;
    ATABLE atable;
    SENTRY *oldbsp,*oldcsp;
    SWENTRY *oldssp;
    LineNumberBlock *oldlines,*oldcblock;
    unsigned char *oldcbase,*cptr;
    long oldlbase,size;
    
    /* initialize */
    argc = 2;   /* 'this' and '_next' */
    rcnt = argc;
    ocnt = 0;

    /* save the previous compiler state */
    oldLevel = c->blockLevel;
    oldcbase = c->cbase;
    oldlbase = c->lbase;
    oldbsp = c->bsp;
    oldcsp = c->csp;
    oldssp = c->ssp;
    oldlines = c->lineNumbers;
    oldcblock = c->currentBlock;
    
    /* initialize new compiler state */
    PushArgFrame(c,&atable);
    c->blockLevel = 0;
    c->cbase = c->cptr;
    c->lbase = c->lptr;
    c->lineNumbers = c->currentBlock = NULL;

    /* name is the first literal */
    if (name)
        make_lit_string(c,name);
    else
        addliteral(c,ic->nilValue);
        
    /* the first arguments are always 'this' and '_next' */
    AddArgument(c,c->arguments,"this");
    AddArgument(c,c->arguments,"_next");

    /* generate the argument frame */
    cptr = c->cptr;
    c->lineNumberChangedP = FALSE;
    putcbyte(c,BobOpAFRAME);
    putcbyte(c,0);
    putcbyte(c,0);
    c->lineNumberChangedP = TRUE;
    
    /* get the argument list */
    frequire(c,'(');
    if ((tkn = BobToken(c)) != ')' && tkn != T_DOTDOT) {
        BobSaveToken(c,tkn);
        do {
            char id[TKNSIZE+1];
            frequire(c,T_IDENTIFIER);
            strcpy(id,c->t_token);
            if ((tkn = BobToken(c)) == '=') {
                int cnt = ++ocnt + rcnt;
                putcbyte(c,BobOpARGSGE);
                putcbyte(c,cnt);
                putcbyte(c,BobOpBRT);
                nxt = putcword(c,0);
                do_init_expr(c);
                AddArgument(c,c->arguments,id);
                putcbyte(c,BobOpESET);
                putcbyte(c,0);
                putcbyte(c,cnt);
                fixup(c,nxt,codeaddr(c));
                tkn = BobToken(c);
            }
            else if (tkn == T_DOTDOT) {
                AddArgument(c,c->arguments,id);
                cptr[0] = BobOpAFRAMER;
                tkn = BobToken(c);
                break;
            }
            else {
                AddArgument(c,c->arguments,id);
                if (ocnt > 0) ++ocnt;
                else ++rcnt;
            }
        } while (tkn == ',');
    }
    require(c,tkn,')');

    /* fixup the function header */
    cptr[1] = rcnt;
    cptr[2] = ocnt;

    /* compile the function body */
    frequire(c,'{');
    do_block(c);

    /* add the return */
    putcbyte(c,BobOpRETURN);

    /* make the bytecode array */
    code = BobMakeString(ic,c->cbase,c->cptr - c->cbase);
    
    /* make the literal vector */
    size = c->lptr - c->lbase;
    code = BobMakeCompiledCode(ic,BobFirstLiteral + size,code);
    src = BobVectorAddress(c->literalbuf) + c->lbase;
    dst = BobCompiledCodeLiterals(code) + BobFirstLiteral;
    while (--size >= 0)
        *dst++ = *src++;
    
    /* free the line number table */
    if (name && c->emitLineNumbersP) {
        printf("%s:\n",name);
        DumpLineNumbers(c);
        printf("\n");
    }
    FreeLineNumbers(c);

    /* pop the current argument frame and buffer pointers */
    PopArgFrame(c);
    c->cptr = c->cbase; c->cbase = oldcbase;
    c->lptr = c->lbase; c->lbase = oldlbase;
    c->bsp = oldbsp;
    c->csp = oldcsp;
    c->ssp = oldssp;
    c->blockLevel = oldLevel;
    c->lineNumbers = oldlines;
    c->currentBlock = oldcblock;
    
    /* make a closure */
    code_literal(c,addliteral(c,code));
    putcbyte(c,BobOpCLOSE);
}

/* do_if - compile the 'if/else' expression */
static void do_if(BobCompiler *c)
{
    int tkn,nxt,end;

    /* compile the test expression */
    do_test(c);

    /* skip around the 'then' clause if the expression is false */
    putcbyte(c,BobOpBRF);
    nxt = putcword(c,NIL);

    /* compile the 'then' clause */
    do_statement(c);

    /* compile the 'else' clause */
    if ((tkn = BobToken(c)) == T_ELSE) {
        putcbyte(c,BobOpBR);
        end = putcword(c,NIL);
        fixup(c,nxt,codeaddr(c));
        do_statement(c);
        nxt = end;
    }
    else
        BobSaveToken(c,tkn);

    /* handle the end of the statement */
    fixup(c,nxt,codeaddr(c));
}

/* do_while - compile the 'while' expression */
static void do_while(BobCompiler *c)
{
    SENTRY bentry,centry;
    int nxt,end;

    /* compile the test expression */
    nxt = codeaddr(c);
    do_test(c);

    /* skip around the loop body if the expression is false */
    putcbyte(c,BobOpBRF);
    end = putcword(c,NIL);

    /* compile the loop body */
    addbreak(c,&bentry,end);
    addcontinue(c,&centry,nxt);
    do_statement(c);
    end = rembreak(c);
    remcontinue(c);

    /* branch back to the start of the loop */
    putcbyte(c,BobOpBR);
    putcword(c,nxt);

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));
}

/* do_dowhile - compile the do/while' expression */
static void do_dowhile(BobCompiler *c)
{
    SENTRY bentry,centry;
    int nxt,end=0;

    /* remember the start of the loop */
    nxt = codeaddr(c);

    /* compile the loop body */
    addbreak(c,&bentry,end);
    addcontinue(c,&centry,nxt);
    do_statement(c);
    end = rembreak(c);
    remcontinue(c);

    /* compile the test expression */
    frequire(c,T_WHILE);
    do_test(c);
    frequire(c,';');

    /* branch to the top if the expression is true */
    putcbyte(c,BobOpBRT);
    putcword(c,nxt);

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));
}

/* do_for - compile the 'for' statement */
static void do_for(BobCompiler *c)
{
    int tkn,nxt,end,body,update;
    SENTRY bentry,centry;

    /* compile the initialization expression */
    frequire(c,'(');
    if ((tkn = BobToken(c)) != ';') {
        BobSaveToken(c,tkn);
        do {
            PVAL pv;
            do_expr2(c,&pv); rvalue(c,&pv);
        } while ((tkn = BobToken(c)) == ',');
        require(c,tkn,';');
    }

    /* compile the test expression */
    nxt = codeaddr(c);
    if ((tkn = BobToken(c)) == ';')
        putcbyte(c,BobOpT);
    else {
        BobSaveToken(c,tkn);
        do_expr(c);
        frequire(c,';');
    }

    /* branch to the loop body if the expression is true */
    putcbyte(c,BobOpBRT);
    body = putcword(c,NIL);

    /* branch to the end if the expression is false */
    putcbyte(c,BobOpBR);
    end = putcword(c,NIL);

    /* compile the update expression */
    update = codeaddr(c);
    if ((tkn = BobToken(c)) != ')') {
        BobSaveToken(c,tkn);
        do_expr(c);
        frequire(c,')');
    }

    /* branch back to the test code */
    putcbyte(c,BobOpBR);
    putcword(c,nxt);

    /* compile the loop body */
    fixup(c,body,codeaddr(c));
    addbreak(c,&bentry,end);
    addcontinue(c,&centry,nxt);
    do_statement(c);
    end = rembreak(c);
    remcontinue(c);

    /* branch back to the update code */
    putcbyte(c,BobOpBR);
    putcword(c,update);

    /* handle the end of the statement */
    fixup(c,end,codeaddr(c));
}

/* addbreak - add a break level to the stack */
static void addbreak(BobCompiler *c,SENTRY *sentry,int lbl)
{
    sentry->level = c->blockLevel;
    sentry->label = lbl;
    sentry->next = c->bsp;
    c->bsp = sentry;
}

/* rembreak - remove a break level from the stack */
static int rembreak(BobCompiler *c)
{
   int lbl = c->bsp->label;
   c->bsp = c->bsp->next;
   return lbl;
}

/* do_break - compile the 'break' statement */
static void do_break(BobCompiler *c)
{
    if (c->bsp) {
        UnwindStack(c,c->blockLevel - c->bsp->level);
        putcbyte(c,BobOpBR);
        c->bsp->label = putcword(c,c->bsp->label);
    }
    else
        BobParseError(c,"Break outside of loop or switch");
}

/* addcontinue - add a continue level to the stack */
static void addcontinue(BobCompiler *c,SENTRY *centry,int lbl)
{
    centry->level = c->blockLevel;
    centry->label = lbl;
    centry->next = c->csp;
    c->csp = centry;
}

/* remcontinue - remove a continue level from the stack */
static void remcontinue(BobCompiler *c)
{
    c->csp = c->csp->next;
}

/* do_continue - compile the 'continue' statement */
static void do_continue(BobCompiler *c)
{
    if (c->csp) {
        UnwindStack(c,c->blockLevel - c->bsp->level);
        putcbyte(c,BobOpBR);
        putcword(c,c->csp->label);
    }
    else
        BobParseError(c,"Continue outside of loop");
}

/* UnwindStack - pop frames off the stack to get back to a previous nesting level */
static void UnwindStack(BobCompiler *c,int levels)
{
    while (--levels >= 0)
        putcbyte(c,BobOpUNFRAME);
}

/* addswitch - add a switch level to the stack */
static void addswitch(BobCompiler *c,SWENTRY *swentry)
{
    swentry->nCases = 0;
    swentry->cases = NULL;
    swentry->defaultLabel = NIL;
    swentry->next = c->ssp;
}

/* remswitch - remove a switch level from the stack */
static void remswitch(BobCompiler *c)
{
    CENTRY *entry,*next;
    for (entry = c->ssp->cases; entry != NULL; entry = next) {
        next = entry->next;
        BobFree(c->ic,entry);
    }
    c->ssp = c->ssp->next;
}

/* do_switch - compile the 'switch' statement */
static void do_switch(BobCompiler *c)
{
    int dispatch,end,cnt;
    SENTRY bentry;
    SWENTRY swentry;
    CENTRY *e;

    /* compile the test expression */
    do_test(c);

    /* branch to the dispatch code */
    putcbyte(c,BobOpBR);
    dispatch = putcword(c,NIL);

    /* compile the body of the switch statement */
    addswitch(c,&swentry);
    addbreak(c,&bentry,0);
    do_statement(c);
    end = rembreak(c);

    /* branch to the end of the statement */
    putcbyte(c,BobOpBR);
    end = putcword(c,end);

    /* compile the dispatch code */
    fixup(c,dispatch,codeaddr(c));
    putcbyte(c,BobOpSWITCH);
    putcword(c,c->ssp->nCases);

    /* output the case table */
    cnt = c->ssp->nCases;
    e = c->ssp->cases;
    while (--cnt >= 0) {
        putcword(c,e->value);
        putcword(c,e->label);
        e = e->next;
    }
    if (c->ssp->defaultLabel)
        putcword(c,c->ssp->defaultLabel);
    else
        end = putcword(c,end);

    /* resolve break targets */
    fixup(c,end,codeaddr(c));

    /* remove the switch context */
    remswitch(c);
}

/* do_case - compile the 'case' statement */
static void do_case(BobCompiler *c)
{
    if (c->ssp) {
        CENTRY **pNext,*entry;
        int value;

        /* get the case value */
        switch (BobToken(c)) {
        case '\\':
            switch (BobToken(c)) {
            case T_IDENTIFIER:
                value = addliteral(c,BobInternCString(c->ic,c->t_token));
                break;
            default:
                BobParseError(c,"Expecting a literal symbol");
                value = 0; /* never reached */
            }
            break;
        case T_INTEGER:
            value = addliteral(c,BobMakeInteger(c->ic,c->t_value));
            break;
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
        case T_FLOAT:
            value = addliteral(c,BobMakeFloat(c->ic,c->t_fvalue));
            break;
#endif
        case T_STRING:
            value = addliteral(c,BobMakeCString(c->ic,c->t_token));
            break;
        case T_NIL:
            value = addliteral(c,c->ic->nilValue);
            break;
        default:
            BobParseError(c,"Expecting a literal value");
            value = 0; /* never reached */
        }
        frequire(c,':');

        /* find the place to add the new case */
        for (pNext = &c->ssp->cases; (entry = *pNext) != NULL; pNext = &entry->next) {
            if (value < entry->value)
                break;
            else if (value == entry->value)
                BobParseError(c,"Duplicate case");
        }

        /* add the case to the list of cases */
        if ((entry = (CENTRY *)BobAlloc(c->ic,sizeof(CENTRY))) == NULL)
            BobInsufficientMemory(c->ic);
        entry->value = value;
        entry->label = codeaddr(c);
        entry->next = *pNext;
        *pNext = entry;

        /* increment the number of cases */
        ++c->ssp->nCases;
    }
    else
        BobParseError(c,"Case outside of switch");
}

/* do_default - compile the 'default' statement */
static void do_default(BobCompiler *c)
{
    if (c->ssp) {
        frequire(c,':');
        c->ssp->defaultLabel = codeaddr(c);
    }
    else
        BobParseError(c,"Default outside of switch");
}

/* do_try - compile the 'try' statement */
static void do_try(BobCompiler *c)
{
    int finally,tkn;
    TCENTRY *entry;
    
    /* make a new try/catch entry */
    if (!(entry = (TCENTRY *)BobAlloc(c->ic,sizeof(TCENTRY))))
        BobInsufficientMemory(c->ic);

    /* compile the protected block */
    frequire(c,'{');
    entry->start = codeaddr(c);
    do_block(c);
    entry->end = codeaddr(c);

    /* branch to the 'finally' code */
    putcbyte(c,BobOpBR);
    finally = putcword(c,NIL);

    /* handle a 'catch' clause */
    if ((tkn = BobToken(c)) == T_CATCH) {

        /* get the formal parameter */
        frequire(c,'(');
        frequire(c,T_IDENTIFIER);
        frequire(c,')');

        /* compile the catch block */
        entry->handler = codeaddr(c);
        frequire(c,'{');
        do_block(c);
        tkn = BobToken(c);
    }

    /* start of the 'finally' code or the end of the statement */
    fixup(c,finally,codeaddr(c));

    /* handle a 'finally' clause */
    if (tkn == T_FINALLY) {
        frequire(c,'{');
        do_block(c);
    }
    else
        BobSaveToken(c,tkn);

    /* add the new exception entry */
    entry->next = c->exceptions;
    c->exceptions = entry;
}

/* do_throw - compile the 'throw' statement */
static void do_throw(BobCompiler *c)
{
    do_expr(c);
    putcbyte(c,BobOpTHROW);
    frequire(c,';');
}

/* do_block - compile the {} expression */
static void do_block(BobCompiler *c)
{
    ATABLE atable;
    int tcnt = 0;
    int tkn;
    
    /* handle local declarations */
    if ((tkn = BobToken(c)) == T_LOCAL) {
        int ptr;

        /* establish the new frame */
        PushArgFrame(c,&atable);

        /* create a new argument frame */
        putcbyte(c,BobOpCFRAME);
        ptr = putcbyte(c,0);
        ++c->blockLevel;

        /* parse each local declaration */
        do {

            /* parse each variable and initializer */
            do {
                char name[TKNSIZE+1];
                frequire(c,T_IDENTIFIER);
                strcpy(name,c->t_token);
                if ((tkn = BobToken(c)) == '=') {
                    do_init_expr(c);
                    putcbyte(c,BobOpESET);
                    putcbyte(c,0);
                    putcbyte(c,1 + tcnt);
                }
                else
                    BobSaveToken(c,tkn);
                AddArgument(c,c->arguments,name);
                ++tcnt;
            } while ((tkn = BobToken(c)) == ',');
            require(c,tkn,';');

        } while ((tkn = BobToken(c)) == T_LOCAL);

        /* fixup the local count */
        c->cbase[ptr] = tcnt;
    }
    
    /* compile the statements in the block */
    if (tkn != '}') {
        do {
            BobSaveToken(c,tkn);
            do_statement(c);
        } while ((tkn = BobToken(c)) != '}');
    }
    else
        putcbyte(c,BobOpNIL);

    /* pop the local frame */
    if (tcnt > 0) {
        putcbyte(c,BobOpUNFRAME);
        PopArgFrame(c);
        --c->blockLevel;
    }
}

/* do_return - handle the 'return' statement */
static void do_return(BobCompiler *c)
{
    int tkn;
	if ((tkn = BobToken(c)) == ';')
		putcbyte(c,BobOpNIL);
	else {
		BobSaveToken(c,tkn);
		do_expr(c);
		frequire(c,';');
	}
    UnwindStack(c,c->blockLevel);
    putcbyte(c,BobOpRETURN);
}

/* do_test - compile a test expression */
static void do_test(BobCompiler *c)
{
    frequire(c,'(');
    do_expr(c);
    frequire(c,')');
}

/* do_expr - parse an expression */
static void do_expr(BobCompiler *c)
{
    PVAL pv;
    do_expr1(c,&pv);
    rvalue(c,&pv);
}

/* do_init_expr - parse an initialization expression */
static void do_init_expr(BobCompiler *c)
{
    PVAL pv;
    do_expr2(c,&pv);
    rvalue(c,&pv);
}

/* rvalue - get the rvalue of a partial expression */
static void rvalue(BobCompiler *c,PVAL *pv)
{
    if (pv->fcn) {
        (*pv->fcn)(c,LOAD,pv);
        pv->fcn = NULL;
    }
}

/* chklvalue - make sure we've got an lvalue */
static void chklvalue(BobCompiler *c,PVAL *pv)
{
    if (pv->fcn == NULL)
        BobParseError(c,"Expecting an lvalue");
}

/* do_expr1 - handle the ',' operator */
static void do_expr1(BobCompiler *c,PVAL *pv)
{
    int tkn;
    do_expr2(c,pv);
    while ((tkn = BobToken(c)) == ',') {
        rvalue(c,pv);
        do_expr1(c,pv); rvalue(c,pv);
    }
    BobSaveToken(c,tkn);
}

/* do_expr2 - handle the assignment operators */
static void do_expr2(BobCompiler *c,PVAL *pv)
{
    int tkn;
    do_expr3(c,pv);
    while ((tkn = BobToken(c)) == '='
    ||     tkn == T_ADDEQ || tkn == T_SUBEQ
    ||     tkn == T_MULEQ || tkn == T_DIVEQ || tkn == T_REMEQ
    ||     tkn == T_ANDEQ || tkn == T_OREQ  || tkn == T_XOREQ
    ||     tkn == T_SHLEQ || tkn == T_SHREQ) {
        chklvalue(c,pv);
        switch (tkn) {
        case '=':
            {
                PVAL pv2;
                (*pv->fcn)(c,PUSH,0);
                do_expr2(c,&pv2);
                rvalue(c,&pv2);
                (*pv->fcn)(c,STORE,pv);
            }
            break;
        case T_ADDEQ:       do_assignment(c,pv,BobOpADD);           break;
        case T_SUBEQ:       do_assignment(c,pv,BobOpSUB);           break;
        case T_MULEQ:       do_assignment(c,pv,BobOpMUL);           break;
        case T_DIVEQ:       do_assignment(c,pv,BobOpDIV);           break;
        case T_REMEQ:       do_assignment(c,pv,BobOpREM);           break;
        case T_ANDEQ:       do_assignment(c,pv,BobOpBAND);    break;
        case T_OREQ:        do_assignment(c,pv,BobOpBOR);           break;
        case T_XOREQ:       do_assignment(c,pv,BobOpXOR);           break;
        case T_SHLEQ:       do_assignment(c,pv,BobOpSHL);           break;
        case T_SHREQ:       do_assignment(c,pv,BobOpSHR);           break;
        }
        pv->fcn = NULL;
    }
    BobSaveToken(c,tkn);
}

/* do_assignment - handle assignment operations */
static void do_assignment(BobCompiler *c,PVAL *pv,int op)
{
    PVAL pv2;
    (*pv->fcn)(c,DUP,0);
    (*pv->fcn)(c,LOAD,pv);
    putcbyte(c,BobOpPUSH);
    do_expr2(c,&pv2);
    rvalue(c,&pv2);
    putcbyte(c,op);
    (*pv->fcn)(c,STORE,pv);
}

/* do_expr3 - handle the '?:' operator */
static void do_expr3(BobCompiler *c,PVAL *pv)
{
    int tkn,nxt,end;
    do_expr4(c,pv);
    while ((tkn = BobToken(c)) == '?') {
        rvalue(c,pv);
        putcbyte(c,BobOpBRF);
        nxt = putcword(c,NIL);
        do_expr1(c,pv); rvalue(c,pv);
        frequire(c,':');
        putcbyte(c,BobOpBR);
        end = putcword(c,NIL);
        fixup(c,nxt,codeaddr(c));
        do_expr1(c,pv); rvalue(c,pv);
        fixup(c,end,codeaddr(c));
    }
    BobSaveToken(c,tkn);
}

/* do_expr4 - handle the '||' operator */
static void do_expr4(BobCompiler *c,PVAL *pv)
{
    int tkn,nxt,end=0;
    do_expr5(c,pv);
    while ((tkn = BobToken(c)) == T_OR) {
        rvalue(c,pv);
        putcbyte(c,BobOpBRT);
        nxt = putcword(c,end);
        do_expr5(c,pv); rvalue(c,pv);
        end = nxt;
    }
    fixup(c,end,codeaddr(c));
    BobSaveToken(c,tkn);
}

/* do_expr5 - handle the '&&' operator */
static void do_expr5(BobCompiler *c,PVAL *pv)
{
    int tkn,nxt,end=0;
    do_expr6(c,pv);
    while ((tkn = BobToken(c)) == T_AND) {
        rvalue(c,pv);
        putcbyte(c,BobOpBRF);
        nxt = putcword(c,end);
        do_expr6(c,pv); rvalue(c,pv);
        end = nxt;
    }
    fixup(c,end,codeaddr(c));
    BobSaveToken(c,tkn);
}

/* do_expr6 - handle the '|' operator */
static void do_expr6(BobCompiler *c,PVAL *pv)
{
    int tkn;
    do_expr7(c,pv);
    while ((tkn = BobToken(c)) == '|') {
        rvalue(c,pv);
        putcbyte(c,BobOpPUSH);
        do_expr7(c,pv); rvalue(c,pv);
        putcbyte(c,BobOpBOR);
    }
    BobSaveToken(c,tkn);
}

/* do_expr7 - handle the '^' operator */
static void do_expr7(BobCompiler *c,PVAL *pv)
{
    int tkn;
    do_expr8(c,pv);
    while ((tkn = BobToken(c)) == '^') {
        rvalue(c,pv);
        putcbyte(c,BobOpPUSH);
        do_expr8(c,pv); rvalue(c,pv);
        putcbyte(c,BobOpXOR);
    }
    BobSaveToken(c,tkn);
}

/* do_expr8 - handle the '&' operator */
static void do_expr8(BobCompiler *c,PVAL *pv)
{
    int tkn;
    do_expr9(c,pv);
    while ((tkn = BobToken(c)) == '&') {
        rvalue(c,pv);
        putcbyte(c,BobOpPUSH);
        do_expr9(c,pv); rvalue(c,pv);
        putcbyte(c,BobOpBAND);
    }
    BobSaveToken(c,tkn);
}

/* do_expr9 - handle the '==' and '!=' operators */
static void do_expr9(BobCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr10(c,pv);
    while ((tkn = BobToken(c)) == T_EQ || tkn == T_NE) {
        switch (tkn) {
        case T_EQ: op = BobOpEQ; break;
        case T_NE: op = BobOpNE; break;
        default:   BobCallErrorHandler(c->ic,BobErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BobOpPUSH);
        do_expr10(c,pv); rvalue(c,pv);
        putcbyte(c,op);
    }
    BobSaveToken(c,tkn);
}

/* do_expr10 - handle the '<', '<=', '>=' and '>' operators */
static void do_expr10(BobCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr11(c,pv);
    while ((tkn = BobToken(c)) == '<' || tkn == T_LE || tkn == T_GE || tkn == '>') {
        switch (tkn) {
        case '<':  op = BobOpLT; break;
        case T_LE: op = BobOpLE; break;
        case T_GE: op = BobOpGE; break;
        case '>':  op = BobOpGT; break;
        default:   BobCallErrorHandler(c->ic,BobErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BobOpPUSH);
        do_expr11(c,pv); rvalue(c,pv);
        putcbyte(c,op);
    }
    BobSaveToken(c,tkn);
}

/* do_expr11 - handle the '<<' and '>>' operators */
static void do_expr11(BobCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr12(c,pv);
    while ((tkn = BobToken(c)) == T_SHL || tkn == T_SHR) {
        switch (tkn) {
        case T_SHL: op = BobOpSHL; break;
        case T_SHR: op = BobOpSHR; break;
        default:    BobCallErrorHandler(c->ic,BobErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BobOpPUSH);
        do_expr12(c,pv); rvalue(c,pv);
        putcbyte(c,op);
    }
    BobSaveToken(c,tkn);
}

/* do_expr12 - handle the '+' and '-' operators */
static void do_expr12(BobCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr13(c,pv);
    while ((tkn = BobToken(c)) == '+' || tkn == '-') {
        switch (tkn) {
        case '+': op = BobOpADD; break;
        case '-': op = BobOpSUB; break;
        default:  BobCallErrorHandler(c->ic,BobErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BobOpPUSH);
        do_expr13(c,pv); rvalue(c,pv);
        putcbyte(c,op);
    }
    BobSaveToken(c,tkn);
}

/* do_expr13 - handle the '*' and '/' operators */
static void do_expr13(BobCompiler *c,PVAL *pv)
{
    int tkn,op;
    do_expr14(c,pv);
    while ((tkn = BobToken(c)) == '*' || tkn == '/' || tkn == '%') {
        switch (tkn) {
        case '*': op = BobOpMUL; break;
        case '/': op = BobOpDIV; break;
        case '%': op = BobOpREM; break;
        default:  BobCallErrorHandler(c->ic,BobErrImpossible,c); op = 0; break;
        }
        rvalue(c,pv);
        putcbyte(c,BobOpPUSH);
        do_expr14(c,pv); rvalue(c,pv);
        putcbyte(c,op);
    }
    BobSaveToken(c,tkn);
}

/* do_expr14 - handle unary operators */
static void do_expr14(BobCompiler *c,PVAL *pv)
{
    int tkn;
    switch (tkn = BobToken(c)) {
    case '-':
        do_expr15(c,pv); rvalue(c,pv);
        putcbyte(c,BobOpNEG);
        break;
    case '!':
        do_expr15(c,pv); rvalue(c,pv);
        putcbyte(c,BobOpNOT);
        break;
    case '~':
        do_expr15(c,pv); rvalue(c,pv);
        putcbyte(c,BobOpBNOT);
        break;
    case T_INC:
        do_preincrement(c,pv,BobOpINC);
        break;
    case T_DEC:
        do_preincrement(c,pv,BobOpDEC);
        break;
    default:
        BobSaveToken(c,tkn);
        do_expr15(c,pv);
        return;
    }
}

/* do_preincrement - handle prefix '++' and '--' */
static void do_preincrement(BobCompiler *c,PVAL *pv,int op)
{
    do_expr15(c,pv);
    chklvalue(c,pv);
    (*pv->fcn)(c,DUP,0);
    (*pv->fcn)(c,LOAD,pv);
    putcbyte(c,op);
    (*pv->fcn)(c,STORE,pv);
    pv->fcn = NULL;
}

/* do_postincrement - handle postfix '++' and '--' */
static void do_postincrement(BobCompiler *c,PVAL *pv,int op)
{
    chklvalue(c,pv);
    (*pv->fcn)(c,DUP,0);
    (*pv->fcn)(c,LOAD,pv);
    putcbyte(c,op);
    (*pv->fcn)(c,STORE,pv);
    putcbyte(c,op == BobOpINC ? BobOpDEC : BobOpINC);
    pv->fcn = NULL;
}

/* do_expr15 - handle function calls */
static void do_expr15(BobCompiler *c,PVAL *pv)
{
    int tkn;
    do_primary(c,pv);
    while ((tkn = BobToken(c)) == '('
    ||     tkn == '['
    ||     tkn == '.'
    ||     tkn == T_INC
    ||     tkn == T_DEC)
        switch (tkn) {
        case '(':
            do_call(c,pv);
            break;
        case '[':
            do_index(c,pv);
            break;
        case '.':
            do_prop_reference(c,pv);
            break;
        case T_INC:
            do_postincrement(c,pv,BobOpINC);
            break;
        case T_DEC:
            do_postincrement(c,pv,BobOpDEC);
            break;
        }
    BobSaveToken(c,tkn);
}

/* do_prop_reference - parse a property reference */
static void do_prop_reference(BobCompiler *c,PVAL *pv)
{
    int tkn;

    /* push the object reference */
    rvalue(c,pv);
    putcbyte(c,BobOpPUSH);

    /* get the selector */
    do_selector(c);

    /* check for a method call */
    if ((tkn = BobToken(c)) == '(') {
        putcbyte(c,BobOpPUSH);
        putcbyte(c,BobOpOVER);
        do_method_call(c,pv);
    }

    /* handle a property reference */
    else {
        BobSaveToken(c,tkn);
        pv->fcn = code_property;
    }
}

/* do_selector - parse a property selector */
static void do_selector(BobCompiler *c)
{
    int tkn;
    switch (tkn = BobToken(c)) {
    case T_IDENTIFIER:
        code_literal(c,addliteral(c,BobInternCString(c->ic,c->t_token)));
        break;
    case '(':
        do_expr(c);
        frequire(c,')');
        break;
    default:
        BobParseError(c,"Expecting a property selector");
        break;
    }
}

/* do_primary - parse a primary expression and unary operators */
static void do_primary(BobCompiler *c,PVAL *pv)
{
    switch (BobToken(c)) {
    case T_FUNCTION:
        do_function(c,pv);
        break;
    case '\\':
        do_literal(c,pv);
        break;
    case '(':
        do_expr1(c,pv);
        frequire(c,')');
        break;
    case T_INTEGER:
        do_lit_integer(c,c->t_value);
        pv->fcn = NULL;
        break;
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    case T_FLOAT:
        do_lit_float(c,c->t_fvalue);
        pv->fcn = NULL;
        break;
#endif
    case T_STRING:
        do_lit_string(c,c->t_token);
        pv->fcn = NULL;
        break;
    case T_NIL:
        putcbyte(c,BobOpNIL);
        pv->fcn = NULL;
        break;
    case T_IDENTIFIER:
        findvariable(c,c->t_token,pv);
        break;
    case T_SUPER:
        do_super(c,pv);
        break;
    case T_NEW:
        do_new_object(c,pv);
        break;
    case '{':
        do_block(c);
        pv->fcn = NULL;
        break;
    default:
        BobParseError(c,"Expecting a primary expression");
        break;
    }
}

/* do_function - parse a regular definition */
static void do_function(BobCompiler *c,PVAL *pv)
{
    char name[256];
    int nameP,tkn;
    
    /* check for a function name */
    switch (tkn = BobToken(c)) {
    case T_IDENTIFIER:
    case T_STRING:
        strcpy(name,c->t_token);
        nameP = TRUE;
        break;
    default:
        BobSaveToken(c,tkn);
        nameP = FALSE;
        break;
    }

    /* compile function body */
    compile_code(c,nameP ? name : NULL);

    /* store the function as the value of the global symbol */
    if (tkn == T_IDENTIFIER) {
        putcbyte(c,BobOpGSET);
        putcword(c,make_lit_symbol(c,name));
    }
    pv->fcn = NULL;
}

/* do_literal - parse a literal expression */
static void do_literal(BobCompiler *c,PVAL *pv)
{
    switch (BobToken(c)) {
    case T_IDENTIFIER:  /* symbol */
        do_literal_symbol(c,pv);
        break;
    case '[':           /* vector */
        do_literal_vector(c,pv);
        break;
    case '{':           /* object */
        do_literal_object(c,pv);
        break;
    default:
        BobParseError(c,"Expecting a symbol, vector or object literal");
        break;
    }
}

/* do_literal_symbol - parse a literal symbol */
static void do_literal_symbol(BobCompiler *c,PVAL *pv)
{
    code_literal(c,addliteral(c,BobInternCString(c->ic,c->t_token)));
    pv->fcn = NULL;
}

/* do_literal_vector - parse a literal vector */
static void do_literal_vector(BobCompiler *c,PVAL *pv)
{
    long cnt = 0;
    int tkn;
    if ((tkn = BobToken(c)) != ']') {
        BobSaveToken(c,tkn);
        do {
            ++cnt;
            do_init_expr(c);
            putcbyte(c,BobOpPUSH);
        } while ((tkn = BobToken(c)) == ',');
        require(c,tkn,']');
    }
    do_lit_integer(c,cnt);
    putcbyte(c,BobOpNEWVECTOR);
    pv->fcn = NULL;
}

/* do_literal_object - parse a literal object */
static void do_literal_object(BobCompiler *c,PVAL *pv)
{
    int tkn;
    if ((tkn = BobToken(c)) == '}') {
        variable_ref(c,"Object");
        putcbyte(c,BobOpNEWOBJECT);
    }
    else {
        char token[TKNSIZE+1];
        require(c,tkn,T_IDENTIFIER);
        strcpy(token,c->t_token);
        if ((tkn = BobToken(c)) == ':') {
            variable_ref(c,"Object");
            putcbyte(c,BobOpNEWOBJECT);
            for (;;) {
                putcbyte(c,BobOpPUSH);
                putcbyte(c,BobOpPUSH);
                code_literal(c,addliteral(c,BobInternCString(c->ic,token)));
                putcbyte(c,BobOpPUSH);
                do_init_expr(c);
                putcbyte(c,BobOpSETP);
                putcbyte(c,BobOpDROP);
                if ((tkn = BobToken(c)) != ',')
                    break;
                frequire(c,T_IDENTIFIER);
                strcpy(token,c->t_token);
                frequire(c,':');
            }
            require(c,tkn,'}');
        }
        else {
            variable_ref(c,token);
            putcbyte(c,BobOpNEWOBJECT);
            if (tkn != '}') {
                BobSaveToken(c,tkn);
                do {
                    frequire(c,T_IDENTIFIER);
                    putcbyte(c,BobOpPUSH);
                    putcbyte(c,BobOpPUSH);
                    code_literal(c,addliteral(c,BobInternCString(c->ic,c->t_token)));
                    putcbyte(c,BobOpPUSH);
                    frequire(c,':');
                    do_init_expr(c);
                    putcbyte(c,BobOpSETP);
                    putcbyte(c,BobOpDROP);
                } while ((tkn = BobToken(c)) == ',');
                require(c,tkn,'}');
            }
        }
    }
    pv->fcn = NULL;
}

/* do_call - compile a function call */
static void do_call(BobCompiler *c,PVAL *pv)
{
    int tkn,n=2;
    
    /* get the value of the function */
    rvalue(c,pv);
    putcbyte(c,BobOpPUSH);
    putcbyte(c,BobOpNIL);
    putcbyte(c,BobOpPUSH);
    putcbyte(c,BobOpPUSH);

    /* compile each argument expression */
    if ((tkn = BobToken(c)) != ')') {
        BobSaveToken(c,tkn);
        do {
            do_expr2(c,pv); rvalue(c,pv);
            putcbyte(c,BobOpPUSH);
            ++n;
        } while ((tkn = BobToken(c)) == ',');
    }
    require(c,tkn,')');

    /* call the function */
    putcbyte(c,BobOpCALL);
    putcbyte(c,n);

    /* we've got an rvalue now */
    pv->fcn = NULL;
}

/* do_super - compile a super.selector() expression */
static void do_super(BobCompiler *c,PVAL *pv)
{
    /* object is 'this' */
    if (!load_argument(c,"this"))
        BobParseError(c,"Use of super outside of a method");
    putcbyte(c,BobOpPUSH);
    frequire(c,'.');
    do_selector(c);
    putcbyte(c,BobOpPUSH);
    frequire(c,'(');
    load_argument(c,"_next");
    putcbyte(c,BobOpPUSH);
    do_method_call(c,pv);
}

/* do_new_object - compile a new object expression */
static void do_new_object(BobCompiler *c,PVAL *pv)
{
    int tkn;

    /* get the property */
    if ((tkn = BobToken(c)) == T_IDENTIFIER)
        variable_ref(c,c->t_token);
    else if (tkn == '(') {
        do_expr(c);
        frequire(c,')');
    }
    else
        BobParseError(c,"Expecting an object expression");

    /* create the new object */
    putcbyte(c,BobOpNEWOBJECT);

    /* check for needing to call the 'initialize' method */
    if ((tkn = BobToken(c)) == '(') {
        putcbyte(c,BobOpPUSH);
        code_literal(c,addliteral(c,BobInternCString(c->ic,"initialize")));
        putcbyte(c,BobOpPUSH);
        putcbyte(c,BobOpOVER);
        do_method_call(c,pv);
    }

    /* no 'initialize' call */
    else {
        BobSaveToken(c,tkn);
        pv->fcn = NULL;
    }
}

/* do_method_call - compile a method call expression */
static void do_method_call(BobCompiler *c,PVAL *pv)
{
    int tkn,n=2;
    
    /* compile each argument expression */
    if ((tkn = BobToken(c)) != ')') {
        BobSaveToken(c,tkn);
        do {
            do_expr2(c,pv); rvalue(c,pv);
            putcbyte(c,BobOpPUSH);
            ++n;
        } while ((tkn = BobToken(c)) == ',');
    }
    require(c,tkn,')');
    
    /* call the method */
    putcbyte(c,BobOpSEND);
    putcbyte(c,n);
    pv->fcn = NULL;
}

/* do_index - compile an indexing operation */
static void do_index(BobCompiler *c,PVAL *pv)
{
    rvalue(c,pv);
    putcbyte(c,BobOpPUSH);
    do_expr(c);
    frequire(c,']');
    pv->fcn = code_index;
}

/* AddArgument - add a formal argument */
static void AddArgument(BobCompiler *c,ATABLE *atable,char *name)
{
    ARGUMENT *arg;
    if ((arg = (ARGUMENT *)BobAlloc(c->ic,sizeof(ARGUMENT) + strlen(name))) == NULL)
        BobInsufficientMemory(c->ic);
    strcpy(arg->arg_name,name);
    arg->arg_next = NULL;
    *atable->at_pNextArgument = arg;
    atable->at_pNextArgument = &arg->arg_next;
}

/* PushArgFrame - push an argument frame onto the stack */
static void PushArgFrame(BobCompiler *c,ATABLE *atable)
{
    atable->at_arguments = NULL;
    atable->at_pNextArgument = &atable->at_arguments;
    atable->at_next = c->arguments;
    c->arguments = atable;
}

/* PopArgFrame - push an argument frame off the stack */
static void PopArgFrame(BobCompiler *c)
{
    ARGUMENT *arg,*nxt;
    for (arg = c->arguments->at_arguments; arg != NULL; arg = nxt) {
        nxt = arg->arg_next;
        BobFree(c->ic,(char *)arg);
    }
    c->arguments = c->arguments->at_next;
}

/* FreeArguments - free all argument frames */
static void FreeArguments(BobCompiler *c)
{
    while (c->arguments)
        PopArgFrame(c);
}

/* FindArgument - find an argument offset */
static int FindArgument(BobCompiler *c,char *name,int *plev,int *poff)
{
    ATABLE *table;
    ARGUMENT *arg;
    int lev,off;
    lev = 0;
    for (table = c->arguments; table != NULL; table = table->at_next) {
        off = 1;
        for (arg = table->at_arguments; arg != NULL; arg = arg->arg_next) {
            if (strcmp(name,arg->arg_name) == 0) {
                *plev = lev;
                *poff = off;
                return TRUE;
            }
            ++off;
        }
        ++lev;
    }
    return FALSE;
}

/* addliteral - add a literal to the literal vector */
static int addliteral(BobCompiler *c,BobValue lit)
{
    long p;
    for (p = c->lbase; p < c->lptr; ++p)
        if (BobVectorElement(c->literalbuf,p) == lit)
            return (int)(BobFirstLiteral + (p - c->lbase));
    if (c->lptr >= c->ltop)
        BobParseError(c,"too many literals");
    BobSetVectorElement(c->literalbuf,p = c->lptr++,lit);
    return (int)(BobFirstLiteral + (p - c->lbase));
}

/* frequire - fetch a BobToken and check it */
static void frequire(BobCompiler *c,int rtkn)
{
    require(c,BobToken(c),rtkn);
}

/* require - check for a required BobToken */
static void require(BobCompiler *c,int tkn,int rtkn)
{
    char msg[100],tknbuf[100];
    if (tkn != rtkn) {
        strcpy(tknbuf,BobTokenName(rtkn));
        sprintf(msg,"Expecting '%s', found '%s'",tknbuf,BobTokenName(tkn));
        BobParseError(c,msg);
    }
}

/* do_lit_integer - compile a literal integer */
static void do_lit_integer(BobCompiler *c,BobIntegerType n)
{
    code_literal(c,addliteral(c,BobMakeInteger(c->ic,n)));
}

/* do_lit_float - compile a literal float */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
static void do_lit_float(BobCompiler *c,BobFloatType n)
{
    code_literal(c,addliteral(c,BobMakeFloat(c->ic,n)));
}
#endif

/* do_lit_string - compile a literal string */
static void do_lit_string(BobCompiler *c,char *str)
{
    code_literal(c,make_lit_string(c,str));
}

/* do_lit_symbol - compile a literal symbol */
static void do_lit_symbol(BobCompiler *c,char *pname)
{
    code_literal(c,make_lit_symbol(c,pname));
}

/* make_lit_string - make a literal string */
static int make_lit_string(BobCompiler *c,char *str)
{
    return addliteral(c,BobMakeCString(c->ic,str));
}

/* make_lit_symbol - make a literal reference to a symbol */
static int make_lit_symbol(BobCompiler *c,char *pname)
{
    return addliteral(c,BobInternCString(c->ic,pname));
}

/* variable_ref - compile a variable reference */
static void variable_ref(BobCompiler *c,char *name)
{
    PVAL pv;
    findvariable(c,name,&pv);
    rvalue(c,&pv);
}

/* findvariable - find a variable */
static void findvariable(BobCompiler *c,char *id,PVAL *pv)
{    
    int lev,off;
    if (strcmp(id,"true") == 0) {
        pv->fcn = code_constant;
        pv->val = BobOpT;
    }
    else if (strcmp(id,"false") == 0 || strcmp(id,"nil") == 0) {
        pv->fcn = code_constant;
        pv->val = BobOpNIL;
    }
    else if (FindArgument(c,id,&lev,&off)) {
        pv->fcn = code_argument;
        pv->val = lev;
        pv->val2 = off;
    }
    else {
        pv->fcn = code_variable;
        pv->val = make_lit_symbol(c,id);
    }
}
                
/* code_constant - compile a constant reference */
static void code_constant(BobCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,pv->val);
                break;
    case STORE: BobCallErrorHandler(c->ic,BobErrStoreIntoConstant,c);
                break;
    }
}

/* load_argument - compile code to load an argument */
static int load_argument(BobCompiler *c,char *name)
{
    int lev,off;
    if (!FindArgument(c,name,&lev,&off))
        return FALSE;
    putcbyte(c,BobOpEREF);
    putcbyte(c,lev);
    putcbyte(c,off);
    return TRUE;
}

/* code_argument - compile an argument (environment) reference */
static void code_argument(BobCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BobOpEREF);
                putcbyte(c,pv->val);
                putcbyte(c,pv->val2);
                break;
    case STORE: putcbyte(c,BobOpESET);
                putcbyte(c,pv->val);
                putcbyte(c,pv->val2);
                break;
    }
}

/* code_property - compile a property reference */
static void code_property(BobCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BobOpGETP);
                break;
    case STORE: putcbyte(c,BobOpSETP);
                break;
    case PUSH:  putcbyte(c,BobOpPUSH);
                break;
    case DUP:   putcbyte(c,BobOpDUP2);
                break;
    }
}

/* code_variable - compile a variable reference */
static void code_variable(BobCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BobOpGREF);
                putcword(c,pv->val);
                break;
    case STORE: putcbyte(c,BobOpGSET);
                putcword(c,pv->val);
                break;
    }
}

/* code_index - compile an indexed reference */
static void code_index(BobCompiler *c,int fcn,PVAL *pv)
{
    switch (fcn) {
    case LOAD:  putcbyte(c,BobOpVREF);
                break;
    case STORE: putcbyte(c,BobOpVSET);
                break;
    case PUSH:  putcbyte(c,BobOpPUSH);
                break;
    case DUP:   putcbyte(c,BobOpDUP2);
                break;
    }
}

/* code_literal - compile a literal reference */
static void code_literal(BobCompiler *c,int n)
{
    putcbyte(c,BobOpLIT);
    putcword(c,n);
}

/* codeaddr - get the current code address (actually, offset) */
static int codeaddr(BobCompiler *c)
{
    return c->cptr - c->cbase;
}

/* putcbyte - put a code byte into the code buffer */
static int putcbyte(BobCompiler *c,int b)
{
    int addr = codeaddr(c);
    if (c->cptr >= c->ctop)
        BobCallErrorHandler(c->ic,BobErrTooMuchCode,c);
	if (c->emitLineNumbersP && c->lineNumberChangedP) {
		c->lineNumberChangedP = FALSE;
        AddLineNumber(c,c->lineNumber,codeaddr(c));
    }
    *c->cptr++ = b;
    return addr;
}

/* putcword - put a code word into the code buffer */
static int putcword(BobCompiler *c,int w)
{
    int addr = codeaddr(c);
    if (c->cptr >= c->ctop)
        BobCallErrorHandler(c->ic,BobErrTooMuchCode,c);
    *c->cptr++ = w;
    if (c->cptr >= c->ctop)
        BobCallErrorHandler(c->ic,BobErrTooMuchCode,c);
    *c->cptr++ = w >> 8;
    return addr;
}

/* fixup - fixup a reference chain */
static void fixup(BobCompiler *c,int chn,int val)
{
    int hval,nxt;
    for (hval = val >> 8; chn != NIL; chn = nxt) {
        nxt = (c->cbase[chn] & 0xFF) | (c->cbase[chn+1] << 8);
        c->cbase[chn] = val;
        c->cbase[chn+1] = hval;
    }
}

/* AddLineNumber - add a line number entry */
static void AddLineNumber(BobCompiler *c,int line,int pc)
{
    LineNumberBlock *current;
    LineNumberEntry *entry;
    if (!(current = c->currentBlock) || current->count >= kLineNumberBlockSize)
        current = AddLineNumberBlock(c);
    entry = &current->entries[current->count++];
    entry->line = line;
    entry->pc = pc;
}

/* AddLineNumberBlock - add a new block of line numbers */
static LineNumberBlock *AddLineNumberBlock(BobCompiler *c)
{
    LineNumberBlock *current,*block;
    if (!(block = (LineNumberBlock *)BobAlloc(c->ic,sizeof(LineNumberBlock))))
        BobInsufficientMemory(c->ic);
    block->count = 0;
    block->next = NULL;
    if (!(current = c->currentBlock))
        c->lineNumbers = block;
    else
        current->next = block;
    c->currentBlock = block;
    return block;
}

/* FreeLineNumbers - free the line number table */
static void FreeLineNumbers(BobCompiler *c)
{
    LineNumberBlock *block,*next;
    for (block = c->lineNumbers; block != NULL; block = next) {
        next = block->next;
        BobFree(c->ic,block);
    }
    c->lineNumbers = c->currentBlock = NULL;
}

/* DumpLineNumbers - dump the line number table */
static void DumpLineNumbers(BobCompiler *c)
{
    LineNumberBlock *block;
    for (block = c->lineNumbers; block != NULL; block = block->next) {
        int i;
        for (i = 0; i < block->count; ++i) {
            LineNumberEntry *entry = &block->entries[i];
            printf("  %d %04x\n",entry->line,entry->pc);
        }
    }
}
