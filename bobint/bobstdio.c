/* bobstdio.c - bob interface to stdio */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdio.h>
#include "bob.h"

/* prototypes */
static void *FileOpen(BobInterpreter *c,char *name,char *mode);
static int FileClose(void *data);
static int FileGetC(void *data);
static int FilePutC(int ch,void *data);

/* BobUseStandardIO - setup to use stdio for file i/o */
void BobUseStandardIO(BobInterpreter *c)
{
    c->fileOpen = FileOpen;
    c->fileClose = FileClose;
    c->fileGetC = FileGetC;
    c->filePutC = FilePutC;
}

/* FileOpen - open a stdio file */
static void *FileOpen(BobInterpreter *c,char *name,char *mode)
{
    return (void *)fopen(name,mode);
}

/* FileClose - close a stdio file */
static int FileClose(void *data)
{
    return fclose((FILE *)data);
}

/* FileGetC - get a character from a stdio file */
static int FileGetC(void *data)
{
    return getc((FILE *)data);
}

/* FilePutC - write a character to a stdio file */
static int FilePutC(int ch,void *data)
{
    return putc(ch,(FILE *)data);
}
