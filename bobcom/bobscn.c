/* bobscn.c - the lexical scanner */
/*
        Copyright (c) 2001, by David Michael Betz
        All rights reserved
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <setjmp.h>
#include "bobcom.h"

/* keyword table */
static struct { char *kt_keyword; int kt_token; } ktab[] = {
{ "define",     T_DEFINE        },
{ "function",   T_FUNCTION      },
{ "local",      T_LOCAL         },
{ "if",         T_IF            },
{ "else",       T_ELSE          },
{ "while",      T_WHILE         },
{ "return",     T_RETURN        },
{ "for",        T_FOR           },
{ "break",      T_BREAK         },
{ "continue",   T_CONTINUE      },
{ "do",         T_DO            },
{ "switch",     T_SWITCH        },
{ "case",       T_CASE          },
{ "default",    T_DEFAULT       },
{ "nil",        T_NIL           },
{ "super",      T_SUPER         },
{ "new",        T_NEW           },
{ NULL,         0               }};

/* BobToken name table */
static char *t_names[] = {
"<string>",
"<identifier>",
"<integer>",
"<float>",
"function",
"local",
"if",
"else",
"while",
"return",
"for",
"break",
"continue",
"do",
"switch",
"case",
"default",
"nil",
"<=",
"==",
"!=",
">=",
"<<",
">>",
"&&",
"||",
"++",
"--",
"+=",
"-=",
"*=",
"/=",
"%=",
"&=",
"|=",
"^=",
"<<=",
">>=",
"define",
"super",
"new",
".."
};

/* prototypes */
static int rtoken(BobCompiler *c);
static int getstring(BobCompiler *c);
static int getcharacter(BobCompiler *c);
static int literalch(BobCompiler *c,int ch);
static int getid(BobCompiler *c,int ch);
static int getnumber(BobCompiler *c,int ch);
static int getradixnumber(BobCompiler *c,int radix);
static int isradixdigit(int ch,int radix);
static int getdigit(int ch);
static int skipspaces(BobCompiler *c);
static int isidchar(int ch);
static int getch(BobCompiler *c);

/* BobInitScanner - initialize the scanner */
void BobInitScanner(BobCompiler *c,BobStream *s)
{
    /* remember the input stream */
    c->input = s;
    
    /* setup the line buffer */
    c->linePtr = c->line; *c->linePtr = '\0';
    c->lineNumber = 0;

    /* no lookahead yet */
    c->savedToken = T_NOTOKEN;
    c->savedChar = '\0';

    /* not eof yet */
    c->atEOF = FALSE;
}

/* BobToken - get the next BobToken */
int BobToken(BobCompiler *c)
{
    int tkn;

    if ((tkn = c->savedToken) != T_NOTOKEN)
        c->savedToken = T_NOTOKEN;
    else
        tkn = rtoken(c);
    return tkn;
}

/* BobSaveToken - save a BobToken */
void BobSaveToken(BobCompiler *c,int tkn)
{
    c->savedToken = tkn;
}

/* BobTokenName - get the name of a BobToken */
char *BobTokenName(int tkn)
{
    static char tname[2];
    if (tkn == T_EOF)
        return "<eof>";
    else if (tkn >= _TMIN && tkn <= _TMAX)
        return t_names[tkn-_TMIN];
    tname[0] = tkn;
    tname[1] = '\0';
    return tname;
}

/* rtoken - read the next BobToken */
static int rtoken(BobCompiler *c)
{
    int ch,ch2;

    /* check the next character */
    for (;;)
        switch (ch = skipspaces(c)) {
        case EOF:       return T_EOF;
        case '"':	return getstring(c);
        case '\'':      return getcharacter(c);
        case '<':       switch (ch = getch(c)) {
                        case '=':
                            return T_LE;
                        case '<':
                            if ((ch = getch(c)) == '=')
                                return T_SHLEQ;
                            c->savedChar = ch;
                            return T_SHL;
                        default:
                            c->savedChar = ch;
                            return '<';
                        }
        case '=':       if ((ch = getch(c)) == '=')
                            return T_EQ;
                        c->savedChar = ch;
                        return '=';
        case '!':       if ((ch = getch(c)) == '=')
                            return T_NE;
                        c->savedChar = ch;
                        return '!';
        case '>':       switch (ch = getch(c)) {
                        case '=':
                            return T_GE;
                        case '>':
                            if ((ch = getch(c)) == '=')
                                return T_SHREQ;
                            c->savedChar = ch;
                            return T_SHR;
                        default:
                            c->savedChar = ch;
                            return '>';
                        }
        case '&':       switch (ch = getch(c)) {
                        case '&':
                            return T_AND;
                        case '=':
                            return T_ANDEQ;
                        default:
                            c->savedChar = ch;
                            return '&';
                        }
        case '|':       switch (ch = getch(c)) {
                        case '|':
                            return T_OR;
                        case '=':
                            return T_OREQ;
                        default:
                            c->savedChar = ch;
                            return '|';
                        }
        case '^':       if ((ch = getch(c)) == '=')
                            return T_XOREQ;
                        c->savedChar = ch;
                        return '^';
        case '+':       switch (ch = getch(c)) {
                        case '+':
                            return T_INC;
                        case '=':
                            return T_ADDEQ;
                        default:
                            c->savedChar = ch;
                            return '+';
                        }
        case '-':       switch (ch = getch(c)) {
                        case '-':
                            return T_DEC;
                        case '=':
                            return T_SUBEQ;
                        default:
                            c->savedChar = ch;
                            return '-';
                        }
        case '*':       if ((ch = getch(c)) == '=')
                            return T_MULEQ;
                        c->savedChar = ch;
                        return '*';
        case '/':       switch (ch = getch(c)) {
                        case '=':
                            return T_DIVEQ;
                        case '/':
                            while ((ch = getch(c)) != EOF)
                                if (ch == '\n')
                                    break;
                            break;
                        case '*':
                            ch = ch2 = EOF;
                            for (; (ch2 = getch(c)) != EOF; ch = ch2)
                                if (ch == '*' && ch2 == '/')
                                    break;
                            break;
                        default:
                            c->savedChar = ch;
                            return '/';
                        }
                        break;
        case '.':       if ((ch = getch(c)) != EOF && isdigit(ch)) {
                            c->savedChar = ch;
                            return getnumber(c,'.');
                        }
                        else if (ch == '.') {
                            c->t_token[0] = '.';
                            c->t_token[1] = '.';
                            c->t_token[2] = '\0';
                            return T_DOTDOT;
                        }
                        else {
                            c->savedChar = ch;
                            c->t_token[0] = '.';
                            c->t_token[1] = '\0';
                            return '.';
                        }
                        break;
        case '0':       switch (ch = getch(c)) {
                        case 'x':
                        case 'X':
                            return getradixnumber(c,16);
                        default:
                            c->savedChar = ch;
                            if (ch >= '0' && ch <= '7')
                                return getradixnumber(c,8);
                            else
                                return getnumber(c,'0');
                        }
                        break;
        case '#':       // add # as single line comment so we can do #! /usr/bin/env bob 
                        while ((ch = getch(c)) != EOF)
                            if (ch == '\n')
                                break;
                        break;
        default:        if (isdigit(ch))
                            return getnumber(c,ch);
                        else if (isidchar(ch))
                            return getid(c,ch);
                        else {
                            c->t_token[0] = ch;
                            c->t_token[1] = '\0';
                            return ch;
                        }
        }
}

/* getstring - get a string */
static int getstring(BobCompiler *c)
{
    int ch,len=0;
    char *p;

    /* get the string */
    p = c->t_token;
    while ((ch = getch(c)) != EOF && ch != '"') {

        /* get the first byte of the character */
        if ((ch = literalch(c,ch)) == EOF)
            BobParseError(c,"end of file in literal string");

        /* handle one byte characters */
        if (ch <= 0x007f) {
            if ((len += 1) > TKNSIZE)
                BobParseError(c,"string too long");
            *p++ = ch;
        }

        /* handle two byte characters */
        else if (ch <= 0x07ff) {
            if ((len += 2) > TKNSIZE)
                BobParseError(c,"string too long");
            *p++ = 0xc0 | ((ch >> 6) & 0x1f);
            *p++ = 0x80 | ( ch       & 0x3f);
        }

        /* handle three byte characters */
        else if (ch <= 0xffff) {
            if ((len += 3) > TKNSIZE)
                BobParseError(c,"string too long");
            *p++ = 0xe0 | ((ch >> 12) & 0x0f);
            *p++ = 0x80 | ((ch >> 6)  & 0x3f);
            *p++ = 0x80 | ( ch        & 0x3f);
        }

        /* invalid character */
        else
            BobParseError(c,"invalid UTF-8 character");
    }
    if (ch == EOF)
        c->savedChar = EOF;
    *p = '\0';
    return T_STRING;
}

/* getcharacter - get a character constant */
static int getcharacter(BobCompiler *c)
{
    c->t_value = literalch(c,getch(c));
    c->t_token[0] = (char)c->t_value;
    c->t_token[1] = '\0';
    if (getch(c) != '\'')
        BobParseError(c,"Expecting a closing single quote");
    return T_INTEGER;
}

/* CollectHexChar - collect a hex character code */
static int CollectHexChar(BobCompiler *c)
{
    int value,ch;
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return 0;
    }
    value = isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return value;
    }
    return (value << 4) | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
}

/* CollectOctalChar - collect an octal character code */
static int CollectOctalChar(BobCompiler *c,int ch)
{
    int value = ch - '0';
    if ((ch = getch(c)) == EOF || ch < '0' || ch > '7') {
        c->savedChar = ch;
        return value;
    }
    value = (value << 3) | (ch - '0');
    if ((ch = getch(c)) == EOF || ch < '0' || ch > '7') {
        c->savedChar = ch;
        return value;
    }
    return (value << 3) | (ch - '0');
}

/* CollecUnicodeChar - collect a unicode character code */
static int CollecUnicodeChar(BobCompiler *c)
{
    int value,ch;
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return 0;
    }
    value = isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return value;
    }
    value = (value << 4) | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return value;
    }
    value = (value << 4) | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
    if ((ch = getch(c)) == EOF || !isxdigit(ch)) {
        c->savedChar = ch;
        return value;
    }
    return (value << 4) | (isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10);
}

/* literalch - get a character from a literal string */
static int literalch(BobCompiler *c,int ch)
{
    if (ch == '\\')
        switch (ch = getch(c)) {
        case 'b':   ch = '\b'; break;
        case 'f':   ch = '\f'; break;
        case 'n':   ch = '\n'; break;
        case 'r':   ch = '\r'; break;
        case 't':   ch = '\t'; break;
        case 'x':   ch = CollectHexChar(c); break;
        case 'u':   ch = CollecUnicodeChar(c); break;
        case '"':   ch = '"';  break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':   ch = CollectOctalChar(c,ch); break;
        case EOF:   ch = '\\'; c->savedChar = EOF; break;
        }
    return ch;
}

/* getid - get an identifier */
static int getid(BobCompiler *c,int ch)
{
    int len=1,i;
    char *p;

    /* get the identifier */
    p = c->t_token; *p++ = ch;
    while ((ch = getch(c)) != EOF && isidchar(ch)) {
        if (++len > TKNSIZE)
            BobParseError(c,"identifier too long");
        *p++ = ch;
    }
    c->savedChar = ch;
    *p = '\0';

    /* check to see if it is a keyword */
    for (i = 0; ktab[i].kt_keyword != NULL; ++i)
        if (strcmp(ktab[i].kt_keyword,c->t_token) == 0)
            return ktab[i].kt_token;
    return T_IDENTIFIER;
}

/* getnumber - get a number */
static int getnumber(BobCompiler *c,int ch)
{
    char *p = c->t_token;
    int tkn = T_INTEGER;

    /* get the part before the decimal point */
    if (ch != '.') {
        *p++ = ch;
        while ((ch = getch(c)) != EOF && isdigit(ch))
            *p++ = ch;
    }
    
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    /* check for a fractional part */
    if (ch == '.') {
        *p++ = ch;
        while ((ch = getch(c)) != EOF && isdigit(ch))
            *p++ = ch;
        tkn = T_FLOAT;
    }
    
    /* check for an exponent */
    if (ch == 'e' || ch == 'E') {
        *p++ = ch;
        if ((ch = getch(c)) == '+' || ch == '-') {
            *p++ = ch;
            ch = getch(c);
        }
        while (ch != EOF && isdigit(ch)) {
            *p++ = ch;
            ch = getch(c);
        }
        tkn = T_FLOAT;
    }
#endif

    /* terminate the token string */
    c->savedChar = ch;
    *p = '\0';
    
    /* convert the string to a number */
#ifdef BOB_INCLUDE_FLOAT_SUPPORT
    if (tkn == T_FLOAT)
        c->t_fvalue = (BobFloatType)atof(c->t_token);
    else
#endif
        c->t_value = (BobIntegerType)atol(c->t_token);
    
    /* return the BobToken type */
    return tkn;
}

/* getradixnumber - read a number in a specified radix */
static int getradixnumber(BobCompiler *c,int radix)
{
    char *p = c->t_token;
    BobIntegerType val = 0;
    int ch;

    /* get number */
    while ((ch = getch(c)) != BobStreamEOF) {
        if (islower(ch)) ch = toupper(ch);
        if (!isradixdigit(ch,radix))
            break;
        val = val * radix + getdigit(ch);
        *p++ = ch;
    }
    c->savedChar = ch;
    *p = '\0';

    /* convert the string to a number */
    c->t_value = val;
    return T_INTEGER;
}

/* isradixdigit - check to see if a character is a digit in a radix */
static int isradixdigit(int ch,int radix)
{
    switch (radix) {
    case 2:     return ch >= '0' && ch <= '1';
    case 8:     return ch >= '0' && ch <= '7';
    case 10:    return ch >= '0' && ch <= '9';
    case 16:    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F');
    }
    return FALSE; /* never reached */
}

/* getdigit - convert an ascii code to a digit */
static int getdigit(int ch)
{
    return ch <= '9' ? ch - '0' : ch - 'A' + 10;
}

/* skipspaces - skip leading spaces */
static int skipspaces(BobCompiler *c)
{
    int ch;
    while ((ch = getch(c)) != '\0' && isspace(ch))
        ;
    return ch;
}

/* isidchar - is this an identifier character */
static int isidchar(int ch)
{
    return isupper(ch)
        || islower(ch)
        || isdigit(ch)
        || ch == '_';
}

/* getch - get the next character */
static int getch(BobCompiler *c)
{
    int ch;

    /* check for a lookahead character */
    if ((ch = c->savedChar) != '\0') {
        c->savedChar = '\0';
        return ch;
    }

    /* check for a buffered character */
    while (*c->linePtr == '\0') {

        /* check for end of file */
        if (c->atEOF)
            return BobStreamEOF;

        /* read another line */
        else {

            /* read the line */
            for (c->linePtr = c->line;
                 (ch = BobStreamGetC(c->input)) != BobStreamEOF && ch != '\n'; )
                *c->linePtr++ = ch;
            *c->linePtr++ = '\n'; *c->linePtr = '\0';
            c->linePtr = c->line;
            ++c->lineNumber;

            /* check for end of file */
            if (ch < 0)
                c->atEOF = TRUE;
         }
    }

    /* return the current character */
    return *c->linePtr++;
}

/* BobParseError - report an error in the current line */
void BobParseError(BobCompiler *c,char *msg)
{
    c->ic->errorMessage = msg;
    BobCallErrorHandler(c->ic,BobErrSyntaxError,c);
}
