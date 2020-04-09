
#include "main.h"


void err(const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    fprintf(stderr,"error:");
    vfprintf(stderr, fmt, va);
    fputc('\n',stderr);
    exit(-1);
}


int main(int argc, char **argv){

    
    FILE *f = fopen(argv[1], "rb");
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

    getNextToken();


    return 0;
}