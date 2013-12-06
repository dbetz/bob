/* bobparse.c - argument parsing function */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include "bob.h"

/* BobParseArguments - parse the argument list of a method */
void BobParseArguments(BobInterpreter *c,char *fmt,...)
{
    int spec,optionalP;
    BobValue *argv = c->argv;
    int argc = c->argc;
    BobValue arg;
    va_list ap;

    /* get the variable argument list */
    va_start(ap,fmt);

    /* no optional specifier seen yet */
    optionalP = FALSE;

    /* handle each argument specifier */
    while (*fmt) {

        /* check for the optional specifier */
        if ((spec = *fmt++) == '|')
            optionalP = TRUE;

        /* handle argument specifiers */
        else {

            /* check for another argument */
            if (--argc < 0)
				break;

            /* get the argument */
            arg = *--argv;

            /* dispatch on the specifier */
            switch (spec) {
            case '*':   /* skip */
                break;
            case 'c':   /* char */
                {   char *p = va_arg(ap,char *);
                    if (!BobIntegerP(arg))
                        BobTypeError(c,arg);
                    *p = (char)BobIntegerValue(arg);
                }
                break;
            case 's':   /* short */
                {   short *p = va_arg(ap,short *);
                    if (!BobIntegerP(arg))
                        BobTypeError(c,arg);
                    *p = (short)BobIntegerValue(arg);
                }
                break;
            case 'i':   /* int */
                {   int *p = va_arg(ap,int *);
                    if (!BobIntegerP(arg))
                        BobTypeError(c,arg);
                    *p = (int)BobIntegerValue(arg);
                }
                break;
            case 'l':   /* long */
                {   long *p = va_arg(ap,long *);
                    if (!BobIntegerP(arg))
                        BobTypeError(c,arg);
                    *p = (long)BobIntegerValue(arg);
                }
                break;
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
            case 'f':   /* float */
                {   float *p = va_arg(ap,float *);
                    if (!BobFloatP(arg))
                        BobTypeError(c,arg);
                    *p = (float)BobFloatValue(arg);
                }
                break;
            case 'd':   /* double */
                {   double *p = va_arg(ap,double *);
                    if (!BobFloatP(arg))
                        BobTypeError(c,arg);
                    *p = (double)BobFloatValue(arg);
                }
                break;
#endif
            case 'S':   /* string */
                {   char **p = va_arg(ap,char **);
                    int nilAllowedP = FALSE;
                    if (*fmt == '?') {
                        nilAllowedP = TRUE;
                        ++fmt;
                    }
                    if (nilAllowedP && arg == c->nilValue)
                        *p = NULL;
                    else if (BobStringP(arg))
                        *p = (char *)BobStringAddress(arg);
                    else
                        BobTypeError(c,arg);
                    if (*fmt == '#') {
                        int *p = va_arg(ap,int *);
                        *p = BobStringSize(arg);
                        ++fmt;
                    }
                }
                break;
            case 'V':   /* value */
                {   BobValue *p = va_arg(ap,BobValue *);
                    int nilAllowedP = FALSE;
                    if (*fmt == '?') {
                        nilAllowedP = TRUE;
                        ++fmt;
                    }
                    if (nilAllowedP && arg == c->nilValue)
                        *p = NULL;
                    else {
                        if (*fmt == '=') {
                            BobDispatch *desiredType = va_arg(ap,BobDispatch *);
                            BobDispatch *type = BobGetDispatch(arg);
                            for (;;) {
                                if (type->baseType == desiredType)
                                    break;
                                else if (!(type = type->parent))
                                    BobTypeError(c,arg);
                            }
                            ++fmt;
                        }
                        *p = arg;
                    }
                }
                break;
            case 'P':   /* foreign pointer */
                {   void **p = va_arg(ap,void **);
                    int nilAllowedP = FALSE;
                    if (*fmt == '?') {
                        nilAllowedP = TRUE;
                        ++fmt;
                    }
                    if (nilAllowedP && arg == c->nilValue)
                        *p = NULL;
                    else {
                        if (*fmt == '=') {
                            BobDispatch *desiredType = va_arg(ap,BobDispatch *);
                            BobDispatch *type = BobGetDispatch(arg);
                            for (;;) {
                                if (type->baseType == desiredType)
                                    break;
                                else if (!(type = type->parent))
                                    BobTypeError(c,arg);
                            }
                            ++fmt;
                        }
                        *p = BobCObjectValue(arg);
                    }
                }
                break;
            case 'B':   /* boolean */
                {   int *p = va_arg(ap,int *);
                    *p = BobTrueP(c,arg);
                }
                break;
            default:
                BobCallErrorHandler(c,BobErrBadParseCode,(void *)(BobIntegerType)spec);
                break;
            }
        }
    }

    /* finished with the variable arguments */
    va_end(ap);

    /* check for too many arguments */
    if (argc > 0)
        BobTooManyArguments(c);
    else if (argc < 0 && !optionalP)
	BobTooFewArguments(c);
}
