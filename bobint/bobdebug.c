/* bobdebug.c - debug routines */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdio.h>
#include <string.h>
#include "bob.h"
#include "bobint.h"

/* instruction output formats */
#define FMT_NONE        0
#define FMT_BYTE        1
#define FMT_2BYTE       2
#define FMT_WORD        3
#define FMT_LIT         4
#define FMT_SWITCH      5

typedef struct { int ot_code; char *ot_name; int ot_fmt; } OTDEF;
OTDEF otab[] = {
{       BobOpBRT,       "BRT",          FMT_WORD        },
{       BobOpBRF,       "BRF",          FMT_WORD        },
{       BobOpBR,        "BR",           FMT_WORD        },
{       BobOpT,         "T",            FMT_NONE        },
{       BobOpNIL,       "NIL",          FMT_NONE        },
{       BobOpPUSH,      "PUSH",         FMT_NONE        },
{       BobOpNOT,       "NOT",          FMT_NONE        },
{       BobOpADD,       "ADD",          FMT_NONE        },
{       BobOpSUB,       "SUB",          FMT_NONE        },
{       BobOpMUL,       "MUL",          FMT_NONE        },
{       BobOpDIV,       "DIV",          FMT_NONE        },
{       BobOpREM,       "REM",          FMT_NONE        },
{       BobOpBAND,      "BAND",         FMT_NONE        },
{       BobOpBOR,       "BOR",          FMT_NONE        },
{       BobOpXOR,       "XOR",          FMT_NONE        },
{       BobOpBNOT,      "BNOT",         FMT_NONE        },
{       BobOpSHL,       "SHL",          FMT_NONE        },
{       BobOpSHR,       "SHR",          FMT_NONE        },
{       BobOpLT,        "LT",           FMT_NONE        },
{       BobOpLE,        "LE",           FMT_NONE        },
{       BobOpEQ,        "EQ",           FMT_NONE        },
{       BobOpNE,        "NE",           FMT_NONE        },
{       BobOpGE,        "GE",           FMT_NONE        },
{       BobOpGT,        "GT",           FMT_NONE        },
{       BobOpLIT,       "LIT",          FMT_LIT         },
{       BobOpGREF,      "GREF",         FMT_LIT         },
{       BobOpGSET,      "GSET",         FMT_LIT         },
{       BobOpGETP,      "GETP",         FMT_NONE        },
{       BobOpSETP,      "SETP",         FMT_NONE        },
{       BobOpRETURN,    "RETURN",       FMT_NONE        },
{       BobOpCALL,      "CALL",         FMT_BYTE        },
{       BobOpSEND,      "SEND",         FMT_BYTE        },
{       BobOpEREF,      "EREF",         FMT_2BYTE       },
{       BobOpESET,      "ESET",         FMT_2BYTE       },
{       BobOpFRAME,     "FRAME",        FMT_BYTE        },
{       BobOpCFRAME,    "CFRAME",       FMT_BYTE        },
{       BobOpUNFRAME,   "UNFRAME",      FMT_NONE        },
{       BobOpVREF,      "VREF",         FMT_NONE        },
{       BobOpVSET,      "VSET",         FMT_NONE        },
{       BobOpNEG,       "NEG",          FMT_NONE        },
{       BobOpINC,       "INC",          FMT_NONE        },
{       BobOpDEC,       "DEC",          FMT_NONE        },
{       BobOpDUP2,      "DUP2",         FMT_NONE        },
{       BobOpDROP,      "DROP",         FMT_NONE        },
{       BobOpDUP,       "DUP",          FMT_NONE        },
{       BobOpOVER,      "OVER",         FMT_NONE        },
{       BobOpNEWOBJECT, "NEWOBJECT",    FMT_NONE        },
{       BobOpNEWVECTOR, "NEWVECTOR",    FMT_NONE        },
{       BobOpAFRAME,    "AFRAME",       FMT_2BYTE       },
{       BobOpAFRAMER,   "AFRAMER",      FMT_2BYTE       },
{       BobOpCLOSE,     "CLOSE",        FMT_NONE        },
{       BobOpSWITCH,    "SWITCH",       FMT_SWITCH      },
{       BobOpARGSGE,    "ARGSGE",       FMT_BYTE        },
{0,0,0}
};

/* BobDecodeProcedure - decode the instructions in a code object */
void BobDecodeProcedure(BobInterpreter *c,BobValue method,BobStream *stream)
{
    BobValue code = BobMethodCode(method);
    int len,lc,n;
    len = (int)BobStringSize(BobCompiledCodeBytecodes(code));
    for (lc = 0; lc < len; lc += n)
        n = BobDecodeInstruction(c,code,lc,stream);
}

/* BobDecodeInstruction - decode a single bytecode instruction */
int BobDecodeInstruction(BobInterpreter *c,BobValue code,int lc,BobStream *stream)
{
    char buf[100];
    BobValue name;
    unsigned char *cp;
    int i,cnt,n=1;
    OTDEF *op;

    /* get bytecode pointer for this instruction and the method name */
    cp = BobStringAddress(BobCompiledCodeBytecodes(code)) + lc;
    name = BobCompiledCodeName(code);
    
    /* show the address and opcode */
    if (BobStringP(name)) {
        unsigned char *data = BobStringAddress(name);
        long size = BobStringSize(name);
        if (size > 32) size = 32;
        strncpy(buf,(char *)data,(size_t)size);
        sprintf(&buf[size],":%04x %02x ",lc,*cp);
    }
    else
        sprintf(buf,"%08lx:%04x %02x ",(long)code,lc,*cp);
    BobStreamPutS(buf,stream);

    /* display the operands */
    for (op = otab; op->ot_name; ++op)
        if (*cp == op->ot_code) {
            switch (op->ot_fmt) {
            case FMT_NONE:
                sprintf(buf,"      %s\n",op->ot_name);
                BobStreamPutS(buf,stream);
                break;
            case FMT_BYTE:
                sprintf(buf,"%02x    %s %02x\n",cp[1],op->ot_name,cp[1]);
                BobStreamPutS(buf,stream);
                n += 1;
                break;
            case FMT_2BYTE:
                sprintf(buf,"%02x %02x %s %02x %02x\n",cp[1],cp[2],
                        op->ot_name,cp[1],cp[2]);
                BobStreamPutS(buf,stream);
                n += 2;
                break;
            case FMT_WORD:
                sprintf(buf,"%02x %02x %s %02x%02x\n",cp[1],cp[2],
                        op->ot_name,cp[2],cp[1]);
                BobStreamPutS(buf,stream);
                n += 2;
                break;
            case FMT_LIT:
                sprintf(buf,"%02x %02x %s %02x%02x ; ",cp[1],cp[2],
                        op->ot_name,cp[2],cp[1]);
                BobStreamPutS(buf,stream);
                BobPrint(c,BobCompiledCodeLiteral(code,(cp[2] << 8) | cp[1]),stream);
                BobStreamPutC('\n',stream);
                n += 2;
                break;
            case FMT_SWITCH:
                sprintf(buf,"%02x %02x %s %02x%02x\n",cp[1],cp[2],
                        op->ot_name,cp[2],cp[1]);
                BobStreamPutS(buf,stream);
                cnt = cp[2] << 8 | cp[1];
                n += 2 + cnt * 4 + 2;
                i = 3;
                while (--cnt >= 0) {
                    sprintf(buf,"                 %02x%02x %02x%02x ; ",cp[i+1],cp[i],cp[i+3],cp[i+2]);
                    BobStreamPutS(buf,stream);
                    BobPrint(c,BobCompiledCodeLiteral(code,(cp[i+1] << 8) | cp[i]),stream);
                    BobStreamPutC('\n',stream);
                    i += 4;
                }
                sprintf(buf,"                 %02x%02x\n",cp[i+1],cp[i]);
                BobStreamPutS(buf,stream);
                break;
            }
            return n;
        }
    
    /* unknown opcode */
    sprintf(buf,"      <UNKNOWN>\n");
    BobStreamPutS(buf,stream);
    return 1;
}
