/* bobint.h - interpreter definitions */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#ifndef __BOBINT_H__
#define __BOBINT_H__

/* opcodes */
#define BobOpBRT        0x01    /* branch on true */
#define BobOpBRF        0x02    /* branch on false */
#define BobOpBR         0x03    /* branch unconditionally */
#define BobOpT          0x04    /* load val with t */
#define BobOpNIL        0x05    /* load val with nil */
#define BobOpPUSH       0x06    /* push val onto stack */
#define BobOpNOT        0x07    /* logical negate top of stack */
#define BobOpADD        0x08    /* add two numeric expressions */
#define BobOpSUB        0x09    /* subtract two numeric expressions */
#define BobOpMUL        0x0a    /* multiply two numeric expressions */
#define BobOpDIV        0x0b    /* divide two numeric expressions */
#define BobOpREM        0x0c    /* remainder of two numeric expressions */
#define BobOpBAND       0x0d    /* bitwise and of top two stack entries */
#define BobOpBOR        0x0e    /* bitwise or of top two stack entries */
#define BobOpXOR        0x0f    /* bitwise xor of top two stack entries */
#define BobOpBNOT       0x10    /* bitwise not of top two stack entries */
#define BobOpSHL        0x11    /* shift left top two stack entries */
#define BobOpSHR        0x12    /* shift right top two stack entries */
#define BobOpLT         0x13    /* less than */
#define BobOpLE         0x14    /* less than or equal */
#define BobOpEQ         0x15    /* equal */
#define BobOpNE         0x16    /* not equal */
#define BobOpGE         0x17    /* greater than or equal */
#define BobOpGT         0x18    /* greater than */
#define BobOpLIT        0x19    /* load literal */
#define BobOpGREF       0x1a    /* load a global variable value */
#define BobOpGSET       0x1b    /* set the value of a global variable */
#define BobOpGETP       0x1c    /* get the value of an object property */
#define BobOpSETP       0x1d    /* set the value of an object property and return the value */
#define BobOpRETURN     0x1e    /* return from interpreter */
#define BobOpCALL       0x1f    /* call a function */
#define BobOpSEND       0x20    /* send a message to an object */
#define BobOpEREF       0x21    /* load an environment value */
#define BobOpESET       0x22    /* set an environment value */
#define BobOpFRAME      0x23    /* push an environment frame */
#define BobOpUNFRAME    0x24    /* pop an environment frame */
#define BobOpVREF       0x25    /* get an element of a vector */
#define BobOpVSET       0x26    /* set an element of a vector */
#define BobOpNEG        0x27    /* negate top of stack */
#define BobOpINC        0x28    /* increment */
#define BobOpDEC        0x29    /* decrement */
#define BobOpDUP2       0x2a    /* duplicate top two elements on the stack */
#define BobOpDROP       0x2b    /* drop the top entry from the stack */
#define BobOpDUP        0x2c    /* duplicate the top entry on the stack */
#define BobOpOVER       0x2d    /* duplicate the second entry on the stack */
#define BobOpNEWOBJECT  0x2e    /* create a new object */
#define BobOpCFRAME     0x2f    /* create an environment frame */
#define BobOpNEWVECTOR  0x30    /* create a new vector */
#define BobOpAFRAME     0x31    /* create an argument frame */
#define BobOpAFRAMER    0x32    /* create an argument frame with rest argument */
#define BobOpCLOSE      0x33    /* create a closure */
#define BobOpSWITCH     0x34    /* switch dispatch */
#define BobOpARGSGE     0x35    /* argc greater than or equal to */

#endif
