/* bobstream.c - stream i/o routines */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "bob.h"

/* prototypes */
static int DisplayVectorValue(BobInterpreter *c,BobValue val,BobStream *s);
static int DisplayStringValue(BobValue val,BobStream *s);

/* BobPrint - print a value */
int BobPrint(BobInterpreter *c,BobValue val,BobStream *s)
{
    if (BobVectorP(val))
        return DisplayVectorValue(c,val,s);
    return BobPrintValue(c,val,s);
}

/* BobDisplay - display a value */
int BobDisplay(BobInterpreter *c,BobValue val,BobStream *s)
{
    if (BobStringP(val))
        return DisplayStringValue(val,s);
    return BobPrint(c,val,s);
}

/* DisplayVectorValue - display a vector value */
static int DisplayVectorValue(BobInterpreter *c,BobValue val,BobStream *s)
{
    BobIntegerType size,i;
    if (BobStreamPutC('[',s) == BobStreamEOF)
	    return BobStreamEOF;
    size = BobVectorSize(val);
    BobCheck(c,1);
    for (i = 0; i < size; ) {
        BobPush(c,val);
        BobPrint(c,BobVectorElement(val,i),s);
        if (++i < size)
            BobStreamPutC(',',s);
        val = BobPop(c);
    }
    if (BobStreamPutC(']',s) == BobStreamEOF)
	    return BobStreamEOF;
    return 0;
}

/* DisplayStringValue - display a string value */
static int DisplayStringValue(BobValue val,BobStream *s)
{
    unsigned char *p = BobStringAddress(val);
    long size = BobStringSize(val);
    while (--size >= 0)
        if (BobStreamPutC(*p++,s) == BobStreamEOF)
            return BobStreamEOF;
    return 0;
}

/* BobStreamGetS - input a string from a stream */
char *BobStreamGetS(char *buf,int size,BobStream *s)
{
    int ch = BobStreamEOF;
    char *ptr = buf;

    /* read up to a newline */
    while (size > 1 && (ch = BobStreamGetC(s)) != '\n' && ch != BobStreamEOF) {
        *ptr++ = ch;
        --size;
    }
    *ptr = '\0';
    return ch == BobStreamEOF && ptr == buf ? NULL : buf;
}

/* BobStreamPutS - output a string to a stream */
int BobStreamPutS(char *str,BobStream *s)
{
    while (*str != '\0')
        if (BobStreamPutC(*str++,s) == -1)
            return -1;
    return 0;
}

/* BobStreamPrintF - formated output to a stream */
int BobStreamPrintF(BobStream *s,char *fmt,...)
{
    char buf[1024];
    va_list ap;
    va_start(ap,fmt);
    vsprintf(buf,fmt,ap);
    va_end(ap);
    return BobStreamPutS(buf,s);
}

/* prototypes for null streams */
static int CloseNullStream(BobStream *s);
static int NullStreamGetC(BobStream *s);
static int NullStreamPutC(int ch,BobStream *s);

/* dispatch structure for null streams */
static BobStreamDispatch nullDispatch = {
  CloseNullStream,
  NullStreamGetC,
  NullStreamPutC
};

BobStream BobNullStream = { &nullDispatch };

static int CloseNullStream(BobStream *s)
{
    return 0;
}

static int NullStreamGetC(BobStream *s)
{
    return BobStreamEOF;
}

static int NullStreamPutC(int ch,BobStream *s)
{
    return ch;
}

/* prototypes for string streams */
static int CloseStringStream(BobStream *s);
static int StringStreamGetC(BobStream *s);
static int StringStreamPutC(int ch,BobStream *s);

/* dispatch structure for string streams */
static BobStreamDispatch stringDispatch = {
  CloseStringStream,
  StringStreamGetC,
  StringStreamPutC
};

/* BobInitStringStream - initialize a string input stream */
BobStream *BobInitStringStream(BobInterpreter *c,BobStringStream *s,unsigned char *buf,long len)
{
    s->d = &stringDispatch;
    s->buf = NULL;
    s->ptr = buf;
    s->len = len;
    return (BobStream *)s;
}

/* BobMakeStringStream - make a string input stream */
BobStream *BobMakeStringStream(BobInterpreter *c,unsigned char *buf,long len)
{
    BobStringStream *s = NULL;
    if ((s = (BobStringStream *)malloc(sizeof(BobStringStream))) != NULL) {
        if ((s->buf = (unsigned char *)malloc((size_t)len)) == NULL) {
            free((void *)s);
            return NULL;
        }
        memcpy(s->buf,buf,(size_t)len);
        s->d = &stringDispatch;
        s->ptr = s->buf;
        s->len = len;
    }
    return (BobStream *)s;
}

static int CloseStringStream(BobStream *s)
{
    BobStringStream *ss = (BobStringStream *)s;
    if (ss->buf) {
        free((void *)ss->buf);
        free((void *)ss);
    }
    return 0;
}

static int StringStreamGetC(BobStream *s)
{
    BobStringStream *ss = (BobStringStream *)s;
    if (ss->len <= 0)
        return BobStreamEOF;
    --ss->len;
    return *ss->ptr++;
}

static int StringStreamPutC(int ch,BobStream *s)
{
    return BobStreamEOF;
}

/* prototypes for string output streams */
static int CloseStringOutputStream(BobStream *s);
static int StringOutputStreamGetC(BobStream *s);
static int StringOutputStreamPutC(int ch,BobStream *s);

/* dispatch structure for string streams */
static BobStreamDispatch stringOutputDispatch = {
  CloseStringOutputStream,
  StringOutputStreamGetC,
  StringOutputStreamPutC
};

/* BobInitStringOutputStream - initialize a string output stream */
BobStream *BobInitStringOutputStream(BobInterpreter *c,BobStringOutputStream *s,unsigned char *buf,long len)
{
    s->d = &stringOutputDispatch;
    s->buf = buf;
    s->ptr = buf;
    s->end = buf + len;
    s->len = 0;
    return (BobStream *)s;
}

static int CloseStringOutputStream(BobStream *s)
{
    return 0;
}

static int StringOutputStreamGetC(BobStream *s)
{
    return BobStreamEOF;
}

static int StringOutputStreamPutC(int ch,BobStream *s)
{
    BobStringOutputStream *ss = (BobStringOutputStream *)s;
    if (ss->ptr >= ss->end)
        return BobStreamEOF;
    *ss->ptr++ = ch;
    ++ss->len;
    return ch;
}

/* prototypes for indirect streams */
static int CloseIndirectStream(BobStream *s);
static int IndirectStreamGetC(BobStream *s);
static int IndirectStreamPutC(int ch,BobStream *s);

/* dispatch structure for file streams */
BobStreamDispatch indirectDispatch = {
  CloseIndirectStream,
  IndirectStreamGetC,
  IndirectStreamPutC
};

/* BobMakeIndirectStream - make an indirect stream */
BobStream *BobMakeIndirectStream(BobInterpreter *c,BobStream **pStream)
{
    BobIndirectStream *s;
    if ((s = (BobIndirectStream *)malloc(sizeof(BobIndirectStream))) != NULL) {
        s->d = &indirectDispatch;
        s->pStream = pStream;
    }
    return (BobStream *)s;
}

static int CloseIndirectStream(BobStream *s)
{
    free((void *)s);
    return 0;
}

static int IndirectStreamGetC(BobStream *s)
{
    BobIndirectStream *is = (BobIndirectStream *)s;
    return BobStreamGetC(*is->pStream);
}

static int IndirectStreamPutC(int ch,BobStream *s)
{
    BobIndirectStream *is = (BobIndirectStream *)s;
    return BobStreamPutC(ch,*is->pStream);
}

/* prototypes for file streams */
static int CloseFileStream(BobStream *s);
static int FileStreamGetC(BobStream *s);
static int FileStreamPutC(int ch,BobStream *s);

/* dispatch structure for file streams */
BobStreamDispatch fileDispatch = {
  CloseFileStream,
  FileStreamGetC,
  FileStreamPutC
};

/* BobInitFileStream - make a file stream from an existing file pointer */
BobStream *BobInitFileStream(BobInterpreter *c,BobFileStream *s,FILE *fp)
{
    s->c = c;
    s->d = &fileDispatch;
    s->fp = fp;
    return (BobStream *)s;
}

/* BobMakeFileStream - make a file stream from an existing file pointer */
BobStream *BobMakeFileStream(BobInterpreter *c,FILE *fp)
{
    BobFileStream *s = NULL;
    if ((s = (BobFileStream *)malloc(sizeof(BobFileStream))) != NULL) {
        s->c = c;
        s->d = &fileDispatch;
        s->fp = fp;
    }
    return (BobStream *)s;
}

/* BobOpenFileStream - open a file stream */
BobStream *BobOpenFileStream(BobInterpreter *c,char *fname,char *mode)
{
    BobFileStream *s = NULL;
    void *fp;
    if ((fp = (*c->fileOpen)(c,fname,mode)) != NULL) {
        if ((s = (BobFileStream *)malloc(sizeof(BobFileStream))) == NULL)
            fclose(fp);
        else {
            s->c = c;
            s->d = &fileDispatch;
            s->fp = fp;
        }
    }
    return (BobStream *)s;
}

static int CloseFileStream(BobStream *s)
{
    BobFileStream *fs = (BobFileStream *)s;
    (*fs->c->fileClose)(fs->fp);
    free((void *)s);
    return 0;
}

static int FileStreamGetC(BobStream *s)
{
    BobFileStream *fs = (BobFileStream *)s;
    return (*fs->c->fileGetC)(fs->fp);
}

static int FileStreamPutC(int ch,BobStream *s)
{
    BobFileStream *fs = (BobFileStream *)s;
    return (*fs->c->filePutC)(ch,fs->fp);
}
