#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "bob.h"

/* prototypes */
static void Fatal(char *fmt,...);

/* main - the main routine */
int main(int argc,char *argv[])
{
	FILE *fp,*ifp,*ofp;
	char fileName[256];
	int ch;

	/* check the argument list */
	if (argc != 3) {
		fprintf(stderr,"usage: bobmerge <file-list-file> <output-file>\n");
		exit(1);
	}

	/* open the list of files */
	if ((fp = fopen(argv[1],"r")) == NULL)
		Fatal("can't open '%s'",argv[1]);

	/* create the output file */
	if ((ofp = fopen(argv[2],"wb")) == NULL)
		Fatal("can't create '%s'",argv[2]);

	/* write the object file header */
	putc('B',ofp);
	putc('O',ofp);
	putc('B',ofp);
	putc('O',ofp);
	putc((BobFaslVersion >> 24) & 0xff,ofp);
	putc((BobFaslVersion >> 16) & 0xff,ofp);
	putc((BobFaslVersion >>  8) & 0xff,ofp);
	putc( BobFaslVersion        & 0xff,ofp);

	/* read each input filename */
	while (fscanf(fp,"%s",fileName) == 1) {
		printf("Merging '%s'\n",fileName);
		if ((ifp = fopen(fileName,"rb")) == NULL)
			Fatal("can't open '%s'",fileName);
		fseek(ifp,8,SEEK_SET);
		while ((ch = getc(ifp)) != EOF)
			putc(ch,ofp);
		fclose(ifp);
	}

	/* close the files */
	fclose(ofp);
	fclose(fp);

	/* exit successfully */
	return 0;
}

/* Fatal - report a fatal error and exit */
static void Fatal(char *fmt,...)
{
    va_list ap;
    va_start(ap,fmt);
	fprintf(stderr,"error: ");
	vfprintf(stderr,fmt,ap);
	putc('\n',stderr);
    va_end(ap);
	exit(1);
}