#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <ctype.h>

enum{ID, BREAK, CHAR, DOUBLE, ELSE, FOR, IF , INT , REUTRN, STRUCT, VOID, WHILE, 
CT_INT, CT_REAL,CT_CHAR, CT_STRING,
COMMA, SEMICOLON, LPAR, RPAR, LBRACKET,RBRACKET, LACC,RACC,
ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ,LESS,LESSEQ,GREATER,GREATEREQ,
SPACE, LINECOMMENT, COMMENT, UNKNOWN};

extern char * pch;

#define SAFEALLOC(var,Type)if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

void err(const char *fmt, ...);

typedef struct _Token{
    int code; //codul(numele)
    union{
        char *text; // for ID,CT_STRING 
        long int i; // for CT_INT, CT_CHAR
        double r; // for CT_REAL
    };

    int line; // line from input file
    struct _Token *next;

}Token;