#include <stdio.h>
#include <stdlib.h>

char * pch;
enum{ID, BREAK, CHAR, DOUBLE, ELSE, FOR, IF , INT , REUTRN, STRUCT, VOID, WHILE, 
CT_INT, CT_REAL,CT_CHAR, CT_STRING,
COMMA, SEMICOLON, LPAR, RPAR, LBRACKET,RBRACKET, LACC,RACC,
ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ,LESS,LESSEQ,GREATER,GREATEREQ,
SPACE, LINECOMMENT, COMMENT};
int main(){
     
    char file_name[32]; 
    
    printf("File name: ");
    scanf("%s",&file_name);
    printf("file name: %s", file_name);
    
    FILE *f = fopen(file_name, "rb");
    if (!f) {
        perror("Eroare la deschiderea fisierului");
        exit(EXIT_FAILURE);
    }

    fseek(f, 0, SEEK_END);
    
    int size = ftell(f);
    
    rewind(f);

    char *buff;
    buff = (char *)malloc(size*sizeof(char));
    if(!buff){
        perror("Eroare la alocarea de memorie pentru buff");
        exit(EXIT_FAILURE);
    }

    int n = fread(buff, 1, size, f);

    pch = buff;
    printf("In fisier avem ceva de genu: %s", pch);
    return 0;
}