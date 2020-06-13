#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/time.h>

#define SAFEALLOC(var,Type) if(((var)=(Type*)malloc(sizeof(Type)))==NULL) myerr("not enough memory");

enum { COMMA, SEMICOLON,
    LPAR, RPAR,
    LBRACKET, RBRACKET,
    LACC, RACC,
    EQUAL, ASSIGN,
    NOTEQ, NOT,
    LESSEQ, LESS,
    GREATEREQ, GREATER,
    ADD, SUB,
    MUL, DIV,
    AND, OR,
    ID, DOT,
    CT_INT, CT_REAL,
    CT_CHAR, CT_STRING,
    STRUCT,DOUBLE,RETURN,
    BREAK,WHILE,
    CHAR,ELSE,VOID,
    FOR,INT,
    IF,
    COMMENT,
    END
    
}; // codurile AL

typedef struct s_Token {
    int code; // codul (numele)
    union
    {
        char *text; // folosit pentru ID, CT_STRING (alocat dinamic)
        //char *c = createString(point,stop);
        // int b = strtol(c,NULL,16); //8 sau 2
        int i; // folosit pentru CT_INT, CT_CHAR
        double r; // folosit pentru CT_REAL
    };
    int line; // linia din fisierul de intrare
    struct s_Token *next;// inlantuire la urmatorul AL
}Token;

//=======GLOBAL VARIABLES==========
char *pStartCh;
Token *tokens;
Token *lastToken;
Token *crtTk;
Token *consumedTk;
int line=0;
char *pCrtCh;
int limit = 0;
char special_characters[9]= "abfnrtv?";

//===========FUNCTIONS=============
struct _Symbol;
typedef struct _Symbol Symbol;
typedef struct{
    Symbol **begin; // the beginning of the symbols, or NULL
    Symbol **end; // the position after the last symbol
    Symbol **after; // the position after the allocated space
}Symbols;

enum{TB_INT,TB_DOUBLE,TB_CHAR,TB_STRUCT,TB_VOID};
typedef struct {
    int typeBase; // TB_*
    Symbol *s; // struct definition for TB_STRUCT
    int nElements; // >0 array of given size, 0=array without size, <0 non array
}Type;

enum{CLS_VAR,CLS_FUNC,CLS_EXTFUNC,CLS_STRUCT};
enum{MEM_GLOBAL,MEM_ARG,MEM_LOCAL};
typedef struct _Symbol{
    const char *name; // a reference to the name stored in a token
    int cls; // CLS_*
    int mem; // MEM_*
    Type type;
    int depth; // 0-global, 1-in function, 2... - nested blocks in function
    union{
        Symbols args; // used only of functions
        Symbols members; // used only for structs
    };
    union{
        void *addr; // vm: the memory address for global symbols
        int offset; // vm: the stack offset for local symbols
    };
}Symbol;
Symbols symbols;

void initSymbols(Symbols *symbols) {
    symbols->begin=NULL;
    symbols->end=NULL;
    symbols->after=NULL;
}

//================Interface =========================


void myerr(const char *fmt, ...);
void printTk(int tkn);

//================GLOBAL VARIABLES===================
int crtDepth=0;
Symbol* crtStruct=NULL;
Symbol* crtFunc=NULL;

//===================VIRTUAL MACHINE=====================
#define STACK_SIZE (32*1024)
char stack[STACK_SIZE];
char *SP; // Stack Pointer
char *stackAfter; // first byte after stack; used for stack limit tests

//DOUBLE
void pushd(double d) {
    if(SP+sizeof(double)>stackAfter) myerr("out of stack");
    *(double*)SP=d;
    SP+=sizeof(double);
}
double popd(){
    SP-=sizeof(double);
    if(SP<stack) myerr("not enough stack bytes for popd");
    return *(double*)SP;
}
//ADDRESS
void pusha(void *a) {
    if(SP+sizeof(void*)>stackAfter) myerr("out of stack");
    *(void**)SP=a;
    SP+=sizeof(void*);
}
void *popa() {
    SP-=sizeof(void*);
    if(SP<stack) myerr("not enough stack bytes for popa");
    return *(void**)SP;
}
//INT
void pushi(int i) {
    if(SP+sizeof(int)>stackAfter) myerr("out of stack");
    *(int*)SP=i;
    SP+=sizeof(int);
}
int popi(){
    SP-=sizeof(int);
    if(SP<stack) myerr("not enough stack bytes for popi");
    return *(int*)SP;
}
//CHARACTER
void pushc(char c) {
    if(SP+sizeof(char)>stackAfter) myerr("out of stack");
    *(char*)SP=c;
    SP+=sizeof(char);
}
char popc(){
    SP-=sizeof(char);
    if(SP<stack) myerr("not enough stack bytes for popc");
    return *(char*)SP;
}

enum{
    O_ADD_C,
    O_ADD_D,
    O_ADD_I,
    O_AND_A,
    O_AND_C,
    O_AND_D,
    O_AND_I,
    O_CALL,
    O_CALLEXT,
    O_CAST_C_D,
    O_CAST_C_I,
    O_CAST_D_C,
    O_CAST_D_I,
    O_CAST_I_C,
    O_CAST_I_D,
    O_DIV_C,
    O_DIV_D,
    O_DIV_I,
    O_DROP,
    O_ENTER,
    O_EQ_A,
    O_EQ_C,
    O_EQ_D,
    O_EQ_I ,
    O_GREATER_C,
    O_GREATER_D,
    O_GREATER_I,
    O_GREATEREQ_C,
    O_GREATEREQ_D,
    O_GREATEREQ_I,
    O_HALT,
    O_INSERT,
    O_JF_A,
    O_JF_C,
    O_JF_D,
    O_JF_I,
    O_JMP,
    O_JT_A,
    O_JT_C,
    O_JT_D,
    O_JT_I,
    O_LESS_C,
    O_LESS_D,
    O_LESS_I ,
    O_LESSEQ_C,
    O_LESSEQ_D,
    O_LESSEQ_I,
    O_LOAD,
    O_MUL_C,
    O_MUL_D,
    O_MUL_I,
    O_NEG_C,
    O_NEG_D,
    O_NEG_I,
    O_NOP,
    O_NOT_A,
    O_NOT_C,
    O_NOT_D,
    O_NOT_I,
    O_NOTEQ_A,
    O_NOTEQ_C,
    O_NOTEQ_D,
    O_NOTEQ_I,
    O_OFFSET,
    O_OR_A,
    O_OR_C,
    O_OR_D,
    O_OR_I,
    O_PUSHFPADDR,
    O_PUSHCT_A,
    O_PUSHCT_C,
    O_PUSHCT_D,
    O_PUSHCT_I,
    O_RET,
    O_STORE,
    O_SUB_C,
    O_SUB_D,
    O_SUB_I
};

void put_i() { printf("#%d\n",popi()); }
void put_s() { printf("#%s\n",(char*)popa()); }
void put_c() { printf("#%c\n",popc()); }
void put_d() { printf("#%lf\n",popd()); }
void get_i() { int i; scanf("%d, ",&i); pushi(i); }
void get_c() { char c; scanf("%d, ",&c); pushc(c); }
void get_d() { double d; scanf("%lf, ", &d); pushd(d); }
void seconds(){ pushi((int) time(NULL)); }
void get_s() {
    char *s;
    char buffer[1024];
    fgets(buffer, sizeof(buffer),stdin);
    buffer[strlen(buffer)-1]=0;
    s = strdup(buffer);
    pusha(s);
}

typedef struct _Instr{
    int opcode; // O_*
    union{
        int i; // int, char
        double d;
        void *addr;
    }args[2];
    struct _Instr *last,*next; // links to last, next instructions
}Instr;
Instr *instructions,*lastInstruction; // double linked list

Instr *createInstr(int opcode) {
    Instr *i;
    SAFEALLOC(i,Instr)
    /*printf("Created: ");
    switch(opcode) {
        case O_ADD_C: printf("O_ADD_C\n"); break;
        case O_ADD_D: printf("O_ADD_D\n"); break;
        case O_ADD_I: printf("O_ADD_I\n"); break;
        case O_AND_A: printf("O_AND_A\n"); break;
        case O_AND_C: printf("O_AND_C\n"); break;
        case O_AND_D: printf("O_AND_D\n"); break;
        case O_AND_I: printf("O_AND_I\n"); break;
        case O_CALL: printf("O_CALL\n"); break;
        case O_CALLEXT: printf("O_CALLEXT\n"); break;
        case O_CAST_C_D: printf("O_CAST_C_D\n"); break;
        case O_CAST_C_I: printf("O_CAST_C_I\n"); break;
        case O_CAST_D_C: printf("O_CAST_D_C\n"); break;
        case O_CAST_D_I: printf("O_CAST_D_I\n"); break;
        case O_CAST_I_C: printf("O_CAST_I_C\n"); break;
        case O_CAST_I_D: printf("O_CAST_I_D\n"); break;
        case O_DIV_C: printf("O_DIV_C\n"); break;
        case O_DIV_D: printf("O_DIV_D\n"); break;
        case O_DIV_I: printf("O_DIV_I\n"); break;
        case O_DROP: printf("O_DROP\n"); break;
        case O_ENTER: printf("O_ENTER\n"); break;
        case O_EQ_A: printf("O_EQ_A\n"); break;
        case O_EQ_C: printf("O_EQ_C\n"); break;
        case O_EQ_D: printf("O_EQ_D\n"); break;
        case O_EQ_I: printf("O_EQ_I\n"); break;
        case O_GREATER_C: printf("O_GREATER_C\n"); break;
        case O_GREATER_D: printf("O_GREATER_D\n"); break;
        case O_GREATER_I: printf("O_GREATER_I\n"); break;
        case O_GREATEREQ_C: printf("O_GREATEREQ_C\n"); break;
        case O_GREATEREQ_D: printf("O_GREATEREQ_D\n"); break;
        case O_GREATEREQ_I: printf("O_GREATEREQ_I\n"); break;
        case O_HALT: printf("O_HALT\n"); break;
        case O_INSERT: printf("O_INSERT\n"); break;
        case O_JF_A: printf("O_JF_A\n"); break;
        case O_JF_C: printf("O_JF_C\n"); break;
        case O_JF_D: printf("O_JF_D\n"); break;
        case O_JF_I: printf("O_JF_I\n"); break;
        case O_JMP: printf("O_JMP\n"); break;
        case O_JT_A: printf("O_JT_A\n"); break;
        case O_JT_C: printf("O_JT_C\n"); break;
        case O_JT_D: printf("O_JT_D\n"); break;
        case O_JT_I: printf("O_JT_I\n"); break;
        case O_LESS_C: printf("O_LESS_C\n"); break;
        case O_LESS_D: printf("O_LESS_D\n"); break;
        case O_LESS_I: printf("O_LESS_I\n"); break;
        case O_LESSEQ_C: printf("O_LESSEQ_C\n"); break;
        case O_LESSEQ_D: printf("O_LESSEQ_D\n"); break;
        case O_LESSEQ_I: printf("O_LESSEQ_I\n"); break;
        case O_LOAD: printf("O_LOAD\n"); break;
        case O_MUL_C: printf("O_MUL_C\n"); break;
        case O_MUL_D: printf("O_MUL_D\n"); break;
        case O_MUL_I: printf("O_MUL_I\n"); break;
        case O_NEG_C: printf("O_NEG_C\n"); break;
        case O_NEG_D: printf("O_NEG_D\n"); break;
        case O_NEG_I: printf("O_NEG_I\n"); break;
        case O_NOP: printf("O_NOP\n"); break;
        case O_NOT_A: printf("O_NOT_A\n"); break;
        case O_NOT_C: printf("O_NOT_C\n"); break;
        case O_NOT_D: printf("O_NOT_D\n"); break;
        case O_NOT_I: printf("O_NOT_I\n"); break;
        case O_NOTEQ_A: printf("O_NOTEQ_A\n"); break;
        case O_NOTEQ_C: printf("O_NOTEQ_C\n"); break;
        case O_NOTEQ_D: printf("O_NOTEQ_D\n"); break;
        case O_NOTEQ_I: printf("O_NOTEQ_I\n"); break;
        case O_OFFSET: printf("O_OFFSET\n"); break;
        case O_PUSHFPADDR: printf("O_PUSHFPADDR\n"); break;
        case O_PUSHCT_A: printf("O_PUSHCT_A\n"); break;
        case O_PUSHCT_C: printf("O_PUSHCT_C\n"); break;
        case O_PUSHCT_D: printf("O_PUSHCT_D\n"); break;
        case O_PUSHCT_I: printf("O_PUSHCT_I\n"); break;
        case O_RET: printf("O_RET\n"); break;
        case O_STORE: printf("O_STORE\n"); break;
        case O_SUB_C: printf("O_SUB_C\n"); break;
        case O_SUB_D: printf("O_SUB_D\n"); break;
        case O_SUB_I: printf("O_SUB_I\n"); break;
        default:
            myerr("invalid opcode: %d",opcode);
    }*/
    i->opcode=opcode;
    return i;
}

//– adauga o instructiune setandu-i si arg[0].addr
Instr *addInstrA(int opcode,void *addr) {
    Instr *i=createInstr(opcode);
    i->next=NULL;
    i->last=lastInstruction;
    i->args[0].addr = addr;
    if(lastInstruction){
        lastInstruction->next=i;
    }else{
        instructions=i;
    }
    lastInstruction=i;
    return i;
}
//– adauga o instructiune setandu-i si arg[0].i
Instr *addInstrI(int opcode,int val) {
    Instr *i=createInstr(opcode);
    i->next=NULL;
    i->last=lastInstruction;
    i->args[0].i = val;
    if(lastInstruction){
        lastInstruction->next=i;
    }else{
        instructions=i;
    }
    lastInstruction=i;
    return i;
}
//– adauga o instructiune setandu-i si arg[0].i, arg[1].i
Instr *addInstrII(int opcode, int val1, int val2) {
    Instr *i=createInstr(opcode);
    i->next=NULL;
    i->last=lastInstruction;
    i->args[0].i = val1;
    i->args[1].i = val2;
    if(lastInstruction){
        lastInstruction->next=i;
    }else{
        instructions=i;
    }
    lastInstruction=i;
    return i;
}
//– sterge toate instructiunile de dupa instructiunea „start”
void deleteInstructionsAfter(Instr *start){
    Instr* iterator = start->next;
    while(iterator!=NULL) {
        Instr* toDelete = iterator;
        iterator = iterator->next;
        free(toDelete);
    }
    start->next=NULL;
    lastInstruction=start;
}

void insertInstrAfter(Instr *after,Instr *i){
    i->next=after->next;
    i->last=after;
    after->next=i;
    if(i->next==NULL)lastInstruction=i;
}

Instr *addInstr(int opcode){
    Instr *i=createInstr(opcode);
    i->next=NULL;
    i->last=lastInstruction;
    if(lastInstruction){
        lastInstruction->next=i;
    }else{
        instructions=i;
    }
    lastInstruction=i;
    return i;
}

Instr *addInstrAfter(Instr *after,int opcode) {
    Instr *i=createInstr(opcode);
    insertInstrAfter(after,i);
    return i;
}

#define GLOBAL_SIZE (32*1024)
char globals[GLOBAL_SIZE];
int nGlobals;

void *allocGlobal(int size) {
    void *p=globals+nGlobals;
    if(nGlobals+size>GLOBAL_SIZE) myerr("insufficient globals space");
    nGlobals+=size;
    return p;
}

Symbol *requireSymbol(Symbols *symbols, const char *name){
    //printf("\nCautam simbol %s\n",name);
    if(symbols->begin == NULL){
        //printf("Begin este NULL\n");
        return NULL;
    }
    Symbol **iterator = symbols->end-1;
    //printf("Begin nu e null\n");
    //printf("Nume: %s\n",(*iterator)->name);
    while(iterator >= (Symbol **) symbols->begin){
        //printf("iterator !=\n");
        if(strcmp((*iterator)->name,name) == 0){
            //printf("Am gasit simbol: %s\n",(*iterator)->name);
            return *iterator;
        }
        //printf("repetam\n");
        iterator--;
    }
    return NULL;
}


//===================CODE GENERATION=====================
int typeFullSize(Type *type);
int typeArgSize(Type *type);
int typeBaseSize(Type *type) {
    int size=0;
    Symbol **is;
    switch(type->typeBase){
        case TB_INT:size=sizeof(int);break;
        case TB_DOUBLE:size=sizeof(double);break;
        case TB_CHAR:size=sizeof(char);break;
        case TB_STRUCT:
            for(is=type->s->members.begin;is!=type->s->members.end;is++){
                size+=typeFullSize(&(*is)->type);
            }
            break;
        case TB_VOID:size=0;break;
        default:
            myerr("invalid typeBase: %d",type->typeBase);
    }
    return size;
}
int typeFullSize(Type *type) {
    return typeBaseSize(type)*(type->nElements>0?type->nElements:1);
}
int typeArgSize(Type *type) {
    if(type->nElements>=0)
        return sizeof(void*);
    return typeBaseSize(type);
}
int offset;
Instr *crtLoopEnd;

//===================/CODE GENERATION=====================
Type createType(int typeBase,int nElements){
    Type t;
    t.typeBase=typeBase;
    t.nElements=nElements;
    return t;
}

typedef union{
    int i; // int, char
    double d; // double
    const char *str; // char[]
}CtVal;

typedef struct{
    Type type; // type of the result
    int isLVal; // if it is a LVal
    int isCtVal; // if it is a constant value (int, real, char, char[])
    CtVal ctVal; // the constat value
}RetVal;

Symbol *addSymbol(Symbols *symbols,const char *name,int cls) {
    Symbol *s;
    if(symbols->end==symbols->after) { // create more room
        int count = (int) (symbols->after - symbols->begin);
        int n=count*2; // double the room
        if(n==0) n=1; // needed for the initial case
        symbols->begin=(Symbol**)realloc(symbols->begin, n*sizeof(Symbol*));
        if(symbols->begin==NULL) myerr("not enough memory");
        symbols->end=symbols->begin+count;
        symbols->after=symbols->begin+n;
    }
    SAFEALLOC(s,Symbol)
    *symbols->end++=s;
    s->name=name;
    s->cls=cls;
    s->depth=crtDepth;
    return s;
}

Symbol *findSymbol(Symbols *symbols, const char *name){
    //printf("\nCautam simbol %s\n",name);
    if(symbols->begin == NULL){
        //printf("Begin este NULL\n");
        return NULL;
    }
    Symbol **iterator = symbols->end-1;
    //printf("Begin nu e null\n");
    //printf("Nume: %s\n",(*iterator)->name);
    while(iterator >= (Symbol **) symbols->begin){
        //printf("iterator !=\n");
        if(strcmp((*iterator)->name,name) == 0){
            //printf("Am gasit simbol: %s\n",(*iterator)->name);
            return *iterator;
        }
        //printf("repetam\n");
        iterator--;
    }
    return NULL;
}

void deleteSymbolsAfter(Symbols *symbols,Symbol *start) {
    Symbol **iterator = symbols->end-1;
    symbols->end--;
    while(*iterator != start){
        //printf("Stergem symbol: %s\n",(*iterator)->name);

        //free(*iterator);
        iterator--;//--;//
    }
    symbols->end ++;//(Symbol **) (iterator + 1);
}

Token *addTk(int code, int line) {
    Token *tk;
    SAFEALLOC(tk,Token)
    tk->code=code;
    tk->line=line;
    tk->next=NULL;
    if(lastToken){
        lastToken->next=tk;
    } else {
        tokens=tk;
    }
    lastToken = tk;
    return tk;
}

void printTokens() {
    Token *atom = tokens;
    while (atom != NULL) {
        if (atom->code == CT_INT) {
            printTk(atom->code);
            printf(":%ld ",atom->i);
        }
        else if (atom->code == CT_REAL) {
            printTk(atom->code);
            printf(":%lf ",atom->r);
        }
        else if (atom->code == ID) {
            printTk(atom->code);
            printf(":%s ",atom->text);
        }
        else if (atom->code == CT_STRING) {
            printTk(atom->code);
            printf(":%s ",atom->text);
        }
        else if (atom->code == CT_CHAR) {
            printTk(atom->code);
            printf(":%c ",(char)atom->i);
        }
        else {
            printTk(atom->code);
        }
        atom = atom->next;
    }
}

char escapeChar(char c){
    switch(c) {
        case 'a' : return '\a';
        case 'b' : return '\b';
        case 'f' : return '\f';
        case 'n' : return '\n';
        case 'r' : return '\r';
        case 't' : return '\t';
        case 'v' : return '\v';
        case '\'' : return '\'';
        case '\?' : return '\?';
        case '\"' : return '\"';
        case '\\' : return '\\';
        case '0' : return '\0';
        default :
            printf("Eroare nu exista char escaped cu %c\n",c);
            exit(0);
    }
}

char* createString(char *start, char *stop) {
    char *c;
    int n;
    n = (int) (stop - start);
    c = (char *)malloc(sizeof(char)*n+1);
    c = strncpy(c,start,sizeof(char)*n);
    c[n+1] = '\0';
    for(int i=0;i<n;i++) {
        if(c[i]=='\\'){
            c[i] = escapeChar(c[i+1]);
            memmove(c+i+1, c+i+2, strlen(c)-i);
        }
    }
    return c;
}

void myerr(const char *fmt, ...) {
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error: ");
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
}

void tkerr(const Token *tk, const char *fmt,...) {
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error in line %d: ",tk->line);
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    fflush(stdout);
    fflush(stderr);
    exit(-1);
}

void translateTkn(Token *t) {
    switch (t->code) {
        case COMMA : printf("COMMA Line: %d\n",t->line); break;
        case SEMICOLON : printf("SEMICOLON Line: %d\n",t->line); break;
        case LPAR : printf("LPAR Line: %d\n",t->line); break;
        case RPAR : printf("RPAR Line: %d\n",t->line); break;
        case LBRACKET : printf("LBRACKET Line: %d\n",t->line); break;
        case RBRACKET : printf("RBRACKET Line: %d\n",t->line); break;
        case LACC : printf("LACC Line: %d\n",t->line); break;
        case RACC : printf("RACC Line: %d\n",t->line); break;
        case EQUAL : printf("EQUAL Line: %d\n",t->line); break;
        case ASSIGN : printf("ASSIGN Line: %d\n",t->line); break;
        case NOTEQ : printf("NOTEQ Line: %d\n",t->line); break;
        case NOT : printf("NOT Line: %d\n",t->line); break;
        case LESSEQ : printf("LESSEQ Line: %d\n",t->line); break;
        case LESS : printf("LESS Line: %d\n",t->line); break;
        case GREATEREQ : printf("GREATEREQ Line: %d\n",t->line); break;
        case GREATER : printf("GREATER Line: %d\n",t->line); break;
        case ADD : printf("ADD Line: %d\n",t->line); break;
        case SUB : printf("SUB Line: %d\n",t->line); break;
        case MUL : printf("MUL Line: %d\n",t->line); break;
        case DIV : printf("DIV Line: %d\n",t->line); break;
        case AND : printf("AND Line: %d\n",t->line); break;
        case OR : printf("OR Line: %d\n",t->line); break;
        case ID : printf("ID: %s Line: %d\n",t->text ,t->line); break;
        case DOT : printf("DOT Line: %d\n",t->line); break;
        case CT_INT : printf("CT_INT: %ld Line: %d\n",t->i, t->line); break;
        case CT_REAL : printf("CT_REAL: %lf Line: %d\n",t->r, t->line); break;
        case CT_CHAR : printf("CT_CHAR: %li Line: %d\n",t->i, t->line); break;
        case CT_STRING : printf("CT_STRING: %s Line: %d\n",t->text, t->line); break;
        case STRUCT : printf("STRUCT Line: %d\n",t->line); break;
        case DOUBLE : printf("DOUBLE Line: %d\n",t->line); break;
        case RETURN : printf("RETURN Line: %d\n",t->line); break;
        case BREAK : printf("BREAK Line: %d\n",t->line); break;
        case WHILE : printf("WHILE Line: %d\n",t->line); break;
        case CHAR : printf("CHAR Line: %d\n",t->line); break;
        case ELSE : printf("ELSE Line: %d\n",t->line); break;
        case VOID : printf("VOID Line: %d\n",t->line); break;
        case FOR : printf("FOR Line: %d\n",t->line); break;
        case INT : printf("INT Line: %d\n",t->line); break;
        case IF : printf("IF Line: %d\n",t->line); break;
        case COMMENT : printf("COMMENT Line: %d\n",t->line); break;
        case END : printf("END Line: %d\n",t->line); break;
            //case NEWLINE : printf("\n"); break;
        default: printf("Eroare tiparire in printTk se cere valoarea %d\n",t->code); break;
    }
}

void printTk(int tkn) {
    switch (tkn) {
        case 0 : printf("COMMA "); break;
        case 1 : printf("SEMICOLON \n"); break;
        case 2 : printf("LPAR "); break;
        case 3 : printf("RPAR \n"); break;
        case 4 : printf("LBRACKET "); break;
        case 5 : printf("RBRACKET "); break;
        case 6 : printf("LACC "); break;
        case 7 : printf("RACC \n"); break;
        case 8 : printf("EQUAL "); break;
        case 9 : printf("ASSIGN "); break;
        case 10 : printf("NOTEQ "); break;
        case 11 : printf("NOT "); break;
        case 12 : printf("LESSEQ "); break;
        case 13 : printf("LESS "); break;
        case 14 : printf("GREATEREQ "); break;
        case 15 : printf("GREATER "); break;
        case 16 : printf("ADD "); break;
        case 17 : printf("SUB "); break;
        case 18 : printf("MUL "); break;
        case 19 : printf("DIV "); break;
        case 20 : printf("AND "); break;
        case 21 : printf("OR "); break;
        case 22 : printf("ID"); break;
        case 23 : printf("DOT "); break;
        case 24 : printf("CT_INT"); break;
        case 25 : printf("CT_REAL"); break;
        case 26 : printf("CT_CHAR"); break;
        case 27 : printf("CT_STRING"); break;
        case 28 : printf("STRUCT "); break;
        case 29 : printf("DOUBLE "); break;
        case 30 : printf("RETURN "); break;
        case 31 : printf("BREAK "); break;
        case 32 : printf("WHILE "); break;
        case 33 : printf("CHAR "); break;
        case 34 : printf("ELSE "); break;
        case 35 : printf("VOID "); break;
        case 36 : printf("FOR "); break;
        case 37 : printf("INT "); break;
        case 38 : printf("IF "); break;
        case 39 : printf("COMMENT "); break;
        case 40 : printf("END "); break;
            //case 40 : printf("\n"); break;
        default: printf("Eroare tiparire in printTk se cere valoarea %d\n",tkn); break;
    }
}

int getNextToken(){
   
   Token *tk; 
   char *pch;
   pch = pCrtCh;
   SAFEALLOC(tk , Token);
   int s = 0, line =0, len;
   char c;
   const char *pStartCh;
// for(int i = 0; i< strlen(pch); i++ )
 for(;;){
      c = *pch;

      printf("%c\n",c);     
        switch(s){
         case 0:
            if( isdigit(c) && c != '0') { pStartCh = pch; s= 11; pch ++; } // CT_INT
               else if( c == '0') {pStartCh = pch; s=13; pch++;} // CT_REAL
                  else if(c =='/') { s = 61; pch++; }//COMMENT
                     else if(c == 39) { s=67; pStartCh = pch; pch++;} //CT_CHAR
                        else if(c == 34) { s=71; pStartCh = pch; pch++;} //CT_STRING
                           else if(isalpha(c)|| c == '_'){
                                 pStartCh = pch;
                                 pch++;
                                 s=31;
                           } //ID
                              else if(isspace(c) || c == "\r" || c == "\t"){
                                 pch++;
                                 printf("space");
                              }
                                 else if(c == 10){
                                    line++;
                                    pch++;
                                 } // NEW_LINE
                                    else if( c == '+'){pch++; tk = addTk(ADD,line);} //ADD
                                       else if( c == '-'){pch++; tk = addTk(SUB,line);} //SUB
                                          else if( c == '.'){pch++; tk = addTk(DOT,line);} //DOT
                                             else if( c == '&'){pch++; s =40;} //AND
                                                else if( c == '|'){pch++; s=42;} //OR
                                                   else if( c == ','){pch++; tk = addTk(COMMA,line);} //COMMA
                                                      else if( c == ';'){pch++; tk = addTk(SEMICOLON,line);} //SEMICOLON
                                                         else if( c == '('){pch++; tk = addTk(LPAR,line);} //LPAR
                                                            else if( c == ')'){pch++; tk = addTk(RPAR,line);} //RPAR
                                                               else if( c == '['){pch++; tk = addTk(LBRACKET,line);} //LBRACKET
                                                                  else if( c == ']'){pch++; tk = addTk(RBRACKET,line);} //RBRACKET
                                                                     else if( c == '{'){pch++; tk = addTk(LACC,line);} //LACC
                                                                        else if( c == '}'){pch++; tk = addTk(RACC,line);} //RACC
                                       
                                                                           else if(c == '='){
                                                                              
                                                                              s = 33;
                                                                              pch++;
                                                                              
                                                                              
                                                                           } // ASSIGN | EQAUL
                                                                              else if( c == '!'){
                                                                                 pch++;
                                                                                 s=44;
                                                                              }//LESSEQ | LESS
                                                                                 else if(c == '<'){
                                                                                    pch++;
                                                                                    s=47;
                                                                                 }//GREATEQ | GREATER
                                                                                    else if(c == '>'){
                                                                                       pch++;
                                                                                       s = 50;
                                                                                    }// NOT | NOTEQ
                                                                                       else if(c == 0) { 
                                                                           
                                                                                          printf("END");
                                                                                          addTk(END,line); 
                                                                                          return END ;
                                                                                          
                                                                                       }//END
                                                                  
                                                                                          else tkerr(addTk(END,line), "caracter invalid");
               
            break; 
         // ---------- CT_INT + CT_REAL ----------
         case 11:
            if(isdigit(c)){pch++;}
               else if(c == 'e' || c=='E') {pch++; s= 21;}
                  else if(c == '.'){pch++; s= 18;}
                     else s=12;
            break;

         case 12:
            tk = addTk(CT_INT, line);
            tk -> i = atoi(createString(pStartCh, pch));
            s = 0;
            break;  

         case 13:
            if(c == 'x') {pch++;s=14;}
               else s = 16;
            break;

         case 14: 
            if(isalnum(c)){pch++; s=15;}
            break;

         case 15:
            if(isalnum(c)){pch++;}
               else s=12;
            break;

         case 16:
            if(isdigit(c) && c <= 7) {pch++;}
               else if(isdigit(c) && c >7) {pch++;s=17;}
                  else s= 12;

            break;

         case 17:
            if(isdigit(c)){pch++;}
               else if(c=='.'){pch++;s=18;} 

            break;

         case 18:
            if(isdigit(c)){pch++;s=19;}

            break;

         case 19:
            if(isdigit(c)){pch++;}
               else if(c == 'e' || c=='E') {pch++; s= 21;}
                  else s= 20;

         case 20:
            tk =addTk(CT_REAL, line);
            tk -> r = atof(createString(pStartCh, pch));

            s=0;
            break;

         case 21: 
            
            if(c == '-' || c=='+'){pch++; s= 23;}
               else if(isdigit(c)) {pch++; s=22;}

            break;

         case 22:

            if(isdigit(c)) {pch++;}
               else s=20;

            break;

         case 23:

            if(isdigit(c)) {pch++; s= 22;}

            break;                                       

         // ---------- ID ----------
         case 31: 
            if(isalnum(c)||c=='_') pch++;
               else s = 32;
            break;

         case 32: 
            len = pch - pStartCh;
            
               if(len==5&&!memcmp(pStartCh,"break",5)) tk = addTk(BREAK,line);
                  else if(len==4&&!memcmp(pStartCh,"char",4)) tk = addTk(CHAR,line); 
                     else if(len==6&&!memcmp(pStartCh,"double",6)) tk = addTk(DOUBLE,line);
                        else if(len==4&&!memcmp(pStartCh,"else",4)) tk = addTk(ELSE,line);
                           else if(len==3&&!memcmp(pStartCh,"for",3)) tk = addTk(FOR,line);
                              else if(len==2&&!memcmp(pStartCh,"if",2)) tk = addTk(IF,line);
                                 else if(len==3&&!memcmp(pStartCh,"int",3)) tk = addTk(INT,line);
                                    else if(len==6&&!memcmp(pStartCh,"return",6)) tk = addTk(RETURN,line);
                                       else if(len==6&&!memcmp(pStartCh,"struct",6)) tk = addTk(STRUCT,line);
                                          else if(len==4&&!memcmp(pStartCh,"void",4)) tk = addTk(VOID,line);
                                             else if(len==5&&!memcmp(pStartCh,"while",5)) tk = addTk(WHILE,line);
                                                else{
                                                   tk =addTk(ID, line);
                                                   tk->text = createString(pStartCh, pch);
                                                }
            s=0;
            printf("%c", c);
            break;     

         // ---------- ASSIGN + EQUAL ---------- 
         case 33:
            if(c == '='){
               s = 35;
               pch++;
            }else s = 34;
            
            
            break;
 
         case 34:
            tk = addTk(ASSIGN, line);
            s=0;
            
            break; 

         case 35:
         
            tk = addTk(EQUAL, line);
            s=0;
            break;

         // ---------- AND + OR ----------
         case 40:
            if(c == '&'){
               pch++;
               s = 41;
            }

            break;

         case 41:
            tk = addTk(AND,line);
            s=0;  
            
            break;
         case 42:
            if(c == '|'){
               pch++;
               s = 43;
            }

            break;

         case 43:
            tk = addTk(OR,line);
            s=0;  
            
            break;

         // ---------- NOT + NOTEQ ----------

         case 44:
            if(c== '='){
               pch++;
               s = 45;
            }   
             else  s = 46;

            break;

         case 45:
            tk = addTk(NOTEQ, line);
            s=0;
            break;

         case 46:
            tk = addTk(NOT, line);
            s=0;
            break; 
            
         // ---------- LESS + LESSEQ ---------- 
         case 47:
            if(c == '='){
               pch++;
               s = 48;
            }  
               else s = 49;

            break;

         case 48:
            tk = addTk(LESSEQ, line);
            s = 0;
            break;

         case 49:
            tk = addTk(LESS, line);
            s=0;
            break;

         // ---------- GREATER + GREATEREQ ---------- 
         case 50:
            if(c == '='){
               pch++;
               s = 51;
            }  
               else s = 52;

            break;

         case 51:
            tk = addTk(GREATEREQ, line);
            s = 0;
            break;

         case 52:
            tk = addTk(GREATER, line);
            s=0;
            break;         

         // ---------- COMMENT ----------
         case 61:
            if(c =='/') { pch++; s=65;}
               else if(c =='*') { pch++ ; s=62;}
                 else { 
                     pch++; 
                     addTk(DIV, line); 
                     s=0; 
                     }
            
            break;

         case 62: 
            if(c=='*') { pch++ ;s= 63;}
               if(c != '*') { pch++ ;s=62;}
            
            break;

         case 63:
            if(c=='/') {
               pch++; 
               addTk(COMMENT, line); 
               s=0; 
               }  
               else if(c=='*') { pch++ ;s=63;}
                  else {pch++; s=62;}
            
            break;            
         case 65:
             if(c == 10 || c == '\r' || c=='\0' ){
                  pch++; 
                  //addTk(COMMENT,line); 
                  s=0; 
               }
               else {pch++;s=65;}
            
            break;  
         
         // ---------- CT_CHAR -----------

         case 67:
            if(c == '\\') { pch ++ ;s=70; }
              else if((c != '\\' ) && (c != 39)) { pch++ ;s=68;}
            
            break;
         
         case 68: 
            if(c == 39){
               pch++;
               tk=addTk(CT_CHAR, line);
               tk->i =c; 
               s=0;
               }
            
            break;
         
         case 70:
            if(strchr(special_characters,c)!=NULL || c == '\\' || c == 34 || c == '\0'){ 
               pch++; 
               s=68;
            }
            
            break;

         // ---------- CT_STRING -----------

         case 71:
            if (c == '\\') { pch++; s=72;}
             else s=73;    
            
            break;

         case 72: 
            if(strchr(special_characters,c)!=NULL || c == '\\' || c == 34 || c == '\0'){
               pch ++;
               s=71;
            }
            
            break;

         case 73: 
            if(c == 34) { 
               pch++;
               tk = addTk(CT_STRING,line); 
               tk->text = createString(pStartCh, pch); 
               s=0;
               } 
               else if( c != '\\' && c != 39) { pch++;s=73;}
                  else if( c == '\\') {pch ++;s=72;}
                 
         
            break;



         default:
            return -1;     
   
      }

    //   printf("%d--- ", s);
    //   int x;
     // scanf("%d", &x);
   
   
   }

}

int consume(int code){
    if(crtTk->code == code){
        consumedTk = crtTk;
        /*if(crtTk != NULL){
         printf("Token consumed: ");
         translateTkn(crtTk);
        }*/
        crtTk = crtTk->next;
        //printf("Next token is: ");
        //translateTkn(crtTk);
        return 1;
    }
    return 0;
}

void cast(Type *dst,Type *src) {

    if(src->nElements>-1){
        if(dst->nElements>-1){
            if(src->typeBase!=dst->typeBase)
                tkerr(crtTk,"an array cannot be converted to an array of another type");
        }else{
            tkerr(crtTk,"an array cannot be converted to a non-array");
        }
    }else{
        if(dst->nElements>-1){
            tkerr(crtTk,"a non-array cannot be converted to an array");
        }
    }
    switch(src->typeBase){
        case TB_CHAR:
        case TB_INT:
        case TB_DOUBLE:
        switch(dst->typeBase){
            case TB_CHAR:
            case TB_INT:
            case TB_DOUBLE:
                return;
        }
        case TB_STRUCT:
            if(dst->typeBase==TB_STRUCT){
                if(src->s!=dst->s)
                    tkerr(crtTk,"a structure cannot be converted to another one");
                return;
            }
    }
    tkerr(crtTk,"incompatible types");
}

Type getArithType(Type *s1,Type *s2) { //char, int double
    Type r;
    r.nElements=-1;
    if(s1->typeBase==TB_DOUBLE || s2->typeBase==TB_DOUBLE) {
        r.typeBase = TB_DOUBLE;
    } else if(s1->typeBase==TB_INT || s2->typeBase==TB_INT) {
        r.typeBase = TB_INT;
    }  
    return r;
}


int unit();
int declStruct();
int declVar();
int typeBase(Type *ret);
int arrayDecl(Type *ret);
int typeName(Type *ret);
int declFuncAux(Type *t);
int declFunc();
int funcArg();
int stm();
int stmCompound();
int expr(RetVal *rv);
int exprAssign(RetVal *rv);
int exprOr(RetVal *rv);
int exprOrPrim(RetVal *rv);
int exprAnd(RetVal *rv);
int exprAndPrim(RetVal *rv);
int exprEq(RetVal *rv);
int exprEqPrim(RetVal *rv);
int exprRel(RetVal *rv);
int exprRelPrim(RetVal *rv);
int exprAdd(RetVal *rv);
int exprAddPrim(RetVal *rv);
int exprMul(RetVal *rv);
int exprMulPrim(RetVal *rv);
int exprCast(RetVal *rv);
int exprUnary(RetVal *rv);
int exprPostfix(RetVal *rv);
int exprPostfixPrim(RetVal *rv);
int exprPrimary(RetVal *rv);


int unit() {
    //printf("unit()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Instr *labelMain=addInstr(O_CALL);
    addInstr(O_HALT);

    for(;;){
        if(declStruct()){continue;}
        if(declFunc()) {continue;}
        if(declVar()) {continue;}
        else break;

    }
    labelMain->args[0].addr=requireSymbol(&symbols,"main")->addr;
    if(consume(END)) {
        return 1;
    } else tkerr(crtTk,"unit: Lipseste END");
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

Instr *getRVal(RetVal *rv) {
    if(rv->isLVal){
        switch(rv->type.typeBase){
            case TB_INT:
            case TB_DOUBLE:
            case TB_CHAR:
            case TB_STRUCT:
                addInstrI(O_LOAD,typeArgSize(&rv->type));
                break;
            default:tkerr(crtTk,"unhandled type: %d",rv->type.typeBase);
        }
    }
    return lastInstruction;
}
void addCastInstr(Instr *after,Type *actualType,Type *neededType) {
    if(actualType->nElements>=0||neededType->nElements>=0)return;
    switch(actualType->typeBase){
        case TB_CHAR:
            switch(neededType->typeBase){
                case TB_CHAR:break;
                case TB_INT:addInstrAfter(after,O_CAST_C_I);break;
                case TB_DOUBLE:addInstrAfter(after,O_CAST_C_D);break;
            }
            break;
        case TB_INT:
            switch(neededType->typeBase){
                case TB_CHAR:addInstrAfter(after,O_CAST_I_C);break;
                case TB_INT:break;
                case TB_DOUBLE:addInstrAfter(after,O_CAST_I_D);break;
            }
            break;
        case TB_DOUBLE:
            switch(neededType->typeBase){
                case TB_CHAR:addInstrAfter(after,O_CAST_D_C);break;
                case TB_INT:addInstrAfter(after,O_CAST_D_I);break;
                case TB_DOUBLE:break;
            }
            break;
    }
}
Instr *createCondJmp(RetVal *rv) {
    if(rv->type.nElements>=0){  // arrays
        return addInstr(O_JF_A);
    } else {  // non-arrays
        getRVal(rv);
        switch(rv->type.typeBase){
            case TB_CHAR:return addInstr(O_JF_C);
            case TB_DOUBLE:return addInstr(O_JF_D);
            case TB_INT:return addInstr(O_JF_I);
            default:return NULL;
        }
    }
}

int declStruct(){
    //printf("declStruct()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Token *tkName;
    if(consume(STRUCT)){
        if(consume(ID)){
            tkName = consumedTk;
            if(consume(LACC)){
                offset=0;
                //printf("Am cautat simbol: %s\n",tkName->text);
                if(findSymbol(&symbols,tkName->text)) {
                    //printf("Gasit: %s\n",findSymbol(&symbols,tkName->text)->name);
                    tkerr(crtTk, "symbol redefinition: %s", tkName->text);
                }

                crtStruct=addSymbol(&symbols,tkName->text,CLS_STRUCT);
                initSymbols(&(crtStruct->members)); /** Changed from &crtStruct->members */
                for(;;){
                    if(declVar()){continue;}
                    else break;
                }
                if(consume(RACC)){
                    if(consume(SEMICOLON)){
                        crtStruct = NULL;
                        return 1;
                    } else tkerr(crtTk,"declStruct: dupa STRUCT ID LACC * RACC => Lipseste ;");
                } else tkerr(crtTk,"declStruct: dupa STRUCT ID LACC * => Lipseste }");
            }
        } else tkerr(crtTk,"declStruct: dupa STRUCT => Lipseste ID");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

void addVar(Token *tkName,Type *t)  {
    Symbol *s;
    if(crtStruct){
        if(findSymbol(&crtStruct->members,tkName->text)) 
            tkerr(crtTk,"symbol redefinition: %s",tkName->text);
        s=addSymbol(&crtStruct->members,tkName->text,CLS_VAR);
    }
    else if(crtFunc){
        s=findSymbol(&symbols,tkName->text);
        if(s&&s->depth==crtDepth)
            tkerr(crtTk,"symbol redefinition: %s",tkName->text);
        s=addSymbol(&symbols,tkName->text,CLS_VAR);
        s->mem=MEM_LOCAL;
    }
    else{
        if(findSymbol(&symbols,tkName->text))
            tkerr(crtTk,"symbol redefinition: %s",tkName->text);
        s=addSymbol(&symbols,tkName->text,CLS_VAR);
        s->mem=MEM_GLOBAL;
    }
    s->type=*t;
    if(crtStruct||crtFunc){
        s->offset=offset;
    }else{
        s->addr=allocGlobal(typeFullSize(&s->type));
    }
    offset+=typeFullSize(&s->type);
}

int declVar(){
    //printf("declVar()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Token *tkName;
    Type t;
    if(typeBase(&t)){
        if(consume(ID)){
            tkName = consumedTk;
            if (arrayDecl(&t)) {}
            else{
                t.nElements=-1;
            }
            addVar(tkName,&t);
            for(;;){
                if(consume(COMMA)){
                    if(consume(ID)){
                        tkName = consumedTk;
                        if (arrayDecl(&t)) {}
                        else {
                            t.nElements=-1;
                        }
                        addVar(tkName,&t);
                    } else tkerr(crtTk,"declVar: dupa typeBase ID arrayDecl? ( COMMA  => Lipseste ID");
                } else break;
            }
            if(consume(SEMICOLON)){
                return 1;
            } else tkerr(crtTk,"declVar: dupa typeBase ID arrayDecl? => Lipseste ;");
        } else tkerr(crtTk,"declVar: dupa typeBase => Lipseste ID");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int typeBase(Type *ret){
    //printf("typeBase()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Token *tkName;
    if(consume(INT)){
        ret->typeBase=TB_INT;
        return 1;
    }
    if(consume(DOUBLE)){
        ret->typeBase=TB_DOUBLE;
        return 1;
    }
    if(consume(CHAR)){
        ret->typeBase=TB_CHAR;
        return 1;
    }
    if(consume(STRUCT)){
        if(consume(ID)){
            tkName = consumedTk;
            Symbol *s=findSymbol(&symbols,tkName->text);
            if(s==NULL) {
                tkerr(crtTk, "undefined symbol: %s", tkName->text);
            }
            else {
                if (s->cls != CLS_STRUCT)
                    tkerr(crtTk, "%s is not a struct", tkName->text);
                ret->typeBase = TB_STRUCT;
                ret->s = s;
            }
            //printf("Am consumat ID in typeBase\n");
            return 1;
        } else tkerr(crtTk,"typeBase: dupa INT | DOUBLE | CHAR | STRUCT => Lipseste ID");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int arrayDecl(Type *ret){
    //printf("arrayDecl()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    RetVal rv;
    Instr *instrBeforeExpr;
    if(consume(LBRACKET)) {
        instrBeforeExpr=lastInstruction;
        if(expr(&rv)) { // if an expression, get its value
            if(!rv.isCtVal) {
                tkerr(crtTk, "the array size is not a constant");
            }
            if(rv.type.typeBase!=TB_INT) {
                tkerr(crtTk, "the array size is not an integer");
            }
            // the "expr" needs only to provide an array size and no code for it
            deleteInstructionsAfter(instrBeforeExpr);
            ret->nElements= (int) rv.ctVal.i;
        } else {
            ret->nElements=0;//array without given size
        }
        if(consume(RBRACKET)){
            return 1;
        } else tkerr(crtTk,"arrayDecl: dupa LBRACKET expr? Lipseste ]");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int typeName(Type *ret){
    //printf("typeName()\n");
    translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(typeBase(ret)){
        if(arrayDecl(ret)){
        } else {
            ret->nElements=-1;
        }
        return 1;
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int declFuncAux(Type *t){
    //printf("declFuncAux()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(typeBase(t)){
        if (consume(MUL)) {
            t->nElements = 0;
        } else {
            t->nElements=-1;
        }
        return 1;
    } else if (consume(VOID)){
        t->typeBase=TB_VOID;
        return 1;
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int declFunc(){
    //printf("declFunc()\n");
    //translateTkn(crtTk);
    Symbol **ps;
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Token *tkName;
    Type t;
    int sizeArgs;
    if(declFuncAux(&t)){
        if(consume(ID)){
            sizeArgs=offset=0;
            tkName = consumedTk;
            if(consume(LPAR)){
                if(findSymbol(&symbols,tkName->text))
                    tkerr(crtTk,"symbol redefinition: %s",tkName->text);
                crtFunc=addSymbol(&symbols,tkName->text,CLS_FUNC);
                initSymbols(&crtFunc->args); /** Changed from &crtStruct->args */
                crtFunc->type=t; /** Changed t is of type Type* */
                crtDepth++;
                if(funcArg()){
                    for(;;){
                        if(consume(COMMA)){
                            if(funcArg()){}
                            else tkerr(crtTk,"declFunc: dupa ( typeBase MUL? | VOID ) ID LPAR ( funcArg ( COMMA => Lipseste argument dupa ,");
                        } else break;
                    }
                }
                //printf("Current token to consume: %s\n",crtTk->code);
                /*crtTk=crtTk->next->next;
                if(crtTk != NULL){
                    printf("RPAR?: ");
                    translateTkn(crtTk);
                }*/
                if(consume(RPAR)){
                    crtDepth--;
                    crtFunc->addr=addInstr(O_ENTER);
                    sizeArgs=offset;
                    //update args offsets for correct FP indexing
                    for(ps=symbols.begin;ps!=symbols.end;ps++){
                        if((*ps)->mem==MEM_ARG){
                            //2*sizeof(void*) == sizeof(retAddr)+sizeof(FP)
                            (*ps)->offset-=sizeArgs+2*sizeof(void*);
                        }
                    }
                    offset=0;
                    if(stmCompound()){
                        deleteSymbolsAfter(&symbols,crtFunc);
                        //before "crtFunc=NULL;"
                        ((Instr*)crtFunc->addr)->args->i =offset;  // setup the ENTER argument
                        if(crtFunc->type.typeBase==TB_VOID){
                            addInstrII(O_RET,sizeArgs,0);
                        }
                        crtFunc=NULL;
                        return 1;
                    }else tkerr(crtTk,"declFunc: dupa ( typeBase MUL? | VOID ) ID LPAR ( funcArg ( COMMA funcArg )* )? RPAR => Lipseste stmCompound");
                } else tkerr(crtTk,"declFunc: dupa ( typeBase MUL? | VOID ) ID LPAR ( funcArg ( COMMA funcArg )* )? =>Lipseste )");
            }
        } else tkerr(crtTk,"declFunc: dupa ( typeBase MUL? | VOID ) => Lipseste ID");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int funcArg(){
   
    Token *tkName;
    Type t;
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(typeBase(&t)){
        if(consume(ID)){
            tkName = consumedTk;
            if(arrayDecl(&t)){}
            else {
                t.nElements=-1;
            }
            Symbol *s=addSymbol(&symbols,tkName->text,CLS_VAR);
            s->mem=MEM_ARG;
            s->type=t; /** Changed t is of type Type* */
            s=addSymbol(&crtFunc->args,tkName->text,CLS_VAR); /** Changed from &crtStruct->args */
            s->mem=MEM_ARG;
            s->type=t; /** Changed t is of type Type* */
            //for each "s" (the one as local var and the one as arg):
            s->offset=offset;
            //only once at the end, after "offset" is used and "s->type" is set
            offset+=typeArgSize(&s->type);
        } else tkerr(crtTk,"funcArg: dupa typeBase => Lipseste ID");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

void appendInstr(Instr* myInstr) {
    myInstr->last=lastInstruction;
    if (lastInstruction!=NULL) {
        lastInstruction->next = myInstr;
    } else {
        instructions = myInstr;
    }
    lastInstruction=myInstr;
    myInstr->next=NULL;
}



int stm(){
    //printf("stm()\n");
    //translateTkn(crtTk);
    Instr *i,*i1,*i2,*i3,*i4,*is,*ib3,*ibs;
    int sizeArgs;
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    RetVal rv,rv1,rv2,rv3;
    if(stmCompound()){return 1;}
    else if(consume(IF)){
        if(consume(LPAR)){
            if(expr(&rv)){
                if(rv.type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"a structure cannot be logically tested");
                if(consume(RPAR)){
                    i1=createCondJmp(&rv);
                    if(stm()){
                        if(consume(ELSE)){
                            i2=addInstr(O_JMP);
                            if(stm()){
                                i1->args[0].addr=i2->next;
                                i1=i2;
                                return 1;
                            } else tkerr(crtTk,"stm: dupa IF LPAR expr RPAR stm ( ELSE => Lipseste stm in ramura ELSE");
                        }
                        i1->args[0].addr=addInstr(O_NOP);
                        return 1;
                    } else tkerr(crtTk,"stm: dupa IF LPAR expr RPAR => Lipseste stm in ramura de IF");
                } else tkerr(crtTk,"stm: dupa IF LPAR expr => Lipseste ) dupa IF");
            } else tkerr(crtTk,"stm: dupa IF LPAR => Lipseste conditia pentru IF");
        } else tkerr(crtTk,"stm: dupa IF => Lipseste ( dupa IF");
    } else if(consume(WHILE)){
        Instr *oldLoopEnd=crtLoopEnd;
        crtLoopEnd=createInstr(O_NOP);
        i1=lastInstruction;
        if(consume(LPAR)){
            if(expr(&rv)){
                if(rv.type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"a structure cannot be logically tested");
                if(consume(RPAR)){
                    i2=createCondJmp(&rv);
                    if(stm()){
                        addInstrA(O_JMP,i1->next);
                        appendInstr(crtLoopEnd);
                        i2->args[0].addr=crtLoopEnd;
                        crtLoopEnd=oldLoopEnd;
                        return 1;
                    } else tkerr(crtTk,"stm: dupa WHILE LPAR expr RPAR => Lipseste stm in WHILE");
                } else tkerr(crtTk,"stm: dupa WHILE LPAR expr => Lipseste ) dupa WHILE");
            } else tkerr(crtTk,"stm: dupa WHILE LPAR => Lipseste conditia pentru WHILE");
        } else tkerr(crtTk,"stm: dupa WHILE => Lipseste ( dupa WHILE");
    } else if(consume(FOR)){
        Instr *oldLoopEnd=crtLoopEnd;
        crtLoopEnd=createInstr(O_NOP);
        if(consume(LPAR)){
            if(expr(&rv1)) {
                if (typeArgSize(&rv1.type))
                    addInstrI(O_DROP, typeArgSize(&rv1.type));
            }
            if(consume(SEMICOLON)){
                i2=lastInstruction; /* i2 is before rv2 */
                if(expr(&rv2)) {
                    i4=createCondJmp(&rv2);
                    if (rv2.type.typeBase == TB_STRUCT)
                        tkerr(crtTk, "a structure cannot be logically tested");
                } else {
                    i4=NULL;
                }
                if(consume(SEMICOLON)) {
                    ib3=lastInstruction; /* ib3 is before rv3 */
                    //printf("#%p\n",lastInstruction);
                            if(expr(&rv3)) {
                                //printf("@%p\n",lastInstruction);
                        if (typeArgSize(&rv3.type))
                            addInstrI(O_DROP, typeArgSize(&rv3.type));
                    }
                    if(consume(RPAR)){
                        ibs=lastInstruction; /* ibs is before stm */
                        if(stm()){
                            // if rv3 exists, exchange rv3 code with stm code: rv3 stm -> stm rv3
                            if(ib3!=ibs){
                                i3=ib3->next;
                                is=ibs->next;
                                ib3->next=is;
                                is->last=ib3;
                                lastInstruction->next=i3;
                                i3->last=lastInstruction;
                                ibs->next=NULL;
                                lastInstruction=ibs;
                            }
                            addInstrA(O_JMP,i2->next);
                            appendInstr(crtLoopEnd);
                            if(i4)i4->args[0].addr=crtLoopEnd;
                            crtLoopEnd=oldLoopEnd;
                            return 1;
                        } else tkerr(crtTk,"stm: dupa FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? RPAR => Lipseste stm in FOR");
                    } else tkerr(crtTk,"stm: dupa FOR LPAR expr? SEMICOLON expr? SEMICOLON expr? => Lipseste ) dupa FOR");
                } else tkerr(crtTk,"stm: dupa FOR LPAR expr? SEMICOLON expr? => Lipseste al doilea ; in stm al FOR");
            } else tkerr(crtTk,"stm: dupa FOR LPAR expr? => Lipseste primul ; in stm al FOR");
        } else tkerr(crtTk,"stm: dupa FOR => Lipseste ( dupa FOR");
    } else if(consume(BREAK)){
        if(consume(SEMICOLON)){
            if(!crtLoopEnd)
                tkerr(crtTk,"break without for or while");
            addInstrA(O_JMP,crtLoopEnd);
            return 1;
        } else tkerr(crtTk,"stm: dupa BREAK => Lipseste ;");
    } else if(consume(RETURN)){
        if(expr(&rv)) {
            i=getRVal(&rv);
            addCastInstr(i,&rv.type,&crtFunc->type);
            if (crtFunc->type.typeBase == TB_VOID)
                tkerr(crtTk, "a void function cannot return a value");
            //printf("crtFunc: %d    rv: %d",crtFunc->type.nElements,rv.type.nElements);
            cast(&crtFunc->type, &rv.type);
        }
        if(consume(SEMICOLON)){
            if(crtFunc->type.typeBase==TB_VOID){
                addInstrII(O_RET,sizeArgs,0);
            }else{
                addInstrII(O_RET,sizeArgs,typeArgSize(&crtFunc->type));
            }
            return 1;
        } else tkerr(crtTk,"stm: dupa RETURN expr? => Lipseste ;");
    } else if(expr(&rv1)) { /**CHANGED rv into rv1*/
        if(typeArgSize(&rv1.type))addInstrI(O_DROP,typeArgSize(&rv1.type));
        if(consume(SEMICOLON)){
            return 1;
        } else tkerr(crtTk,"stm: dupa expr? => Lipseste ;");
    } else if(consume(SEMICOLON)){
        return 1;
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int stmCompound(){
    //printf("stmCompound()\n");
    //translateTkn(crtTk);
    Symbol *start= symbols.end[-1];
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(consume(LACC)){
        crtDepth++;
        for(;;){
            if(declVar()){}
            else if(stm()){}
            else break;
        }
        if(consume(RACC)){
            crtDepth--;
            deleteSymbolsAfter(&symbols,start);
            return 1;
        } else tkerr(crtTk,"stmCompound: dupa LACC ( declVar | stm )* => Lipseste }");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

//expr: exprAssign
//expr(out rv:RetVal): exprAssign:rv ;
int expr(RetVal *rv){
    //printf("expr()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(exprAssign(rv)) {
        return 1;
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

int exprAssign(RetVal *rv){
    //printf("exprAssign()\n");
    //translateTkn(crtTk);
    RetVal rve;
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Instr* i; /**CHANGED*/
    if(exprUnary(rv)){
        if(consume(ASSIGN)){
            if(exprAssign(&rve)){
                if(!rv->isLVal)tkerr(crtTk,"cannot assign to a non-lval");
                if(rv->type.nElements>-1||rve.type.nElements>-1)
                    tkerr(crtTk,"the arrays cannot be assigned");
                cast(&rv->type,&rve.type);
                // before "rv->isCtVal=rv->isLVal=0;"
                i=getRVal(&rve);
                addCastInstr(i,&rve.type,&rv->type);
                //duplicate the value on top before the dst addr
                addInstrII(O_INSERT, sizeof(void*)+typeArgSize(&rv->type),
                           typeArgSize(&rv->type));
                addInstrI(O_STORE,typeArgSize(&rv->type));
                rv->isCtVal=rv->isLVal=0;
                return 1;
            } else tkerr(crtTk,"exprAssign: dupa exprUnary ASSIGN => Lipseste exprAssign");
        }
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    if(exprOr(rv)){
        return 1;
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

int exprOr(RetVal *rv){
    //printf("exprOr()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(exprAnd(rv)){
        if(exprOrPrim(rv)){
            return 1;
        } else tkerr(crtTk,"exprOr: dupa exprAnd => Lipseste exprOrPrim");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

// exprOrPrim: OR exprAnd exprOrPrim | eps
int exprOrPrim(RetVal *rv){
    //printf("exprOrPrim()\n");
    //translateTkn(crtTk);
    RetVal rve;
    Instr *i1,*i2;Type t,t1,t2;
    if(consume(OR)){
        i1=rv->type.nElements<0?getRVal(rv):lastInstruction;
        t1=rv->type;
        if(exprAnd(&rve)){
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                tkerr(crtTk,"a structure cannot be logically tested");
            // after "if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
            //              tkerr(crtTk,"a structure cannot be logically tested");"
            if(rv->type.nElements>=0){      // vectors
                addInstr(O_OR_A);
            }else{  // non-vectors
                i2=getRVal(&rve);t2=rve.type;
                t=getArithType(&t1,&t2);
                addCastInstr(i1,&t1,&t);
                addCastInstr(i2,&t2,&t);
                switch(t.typeBase){
                    case TB_INT:addInstr(O_OR_I);break;
                    case TB_DOUBLE:addInstr(O_OR_D);break;
                    case TB_CHAR:addInstr(O_OR_C);break;
                }
            }
            rv->type=createType(TB_INT,-1);
            rv->isCtVal=rv->isLVal=0;
            if(exprOrPrim(rv)){
                return 1;
            } else tkerr(crtTk,"exprOrPrim: dupa OR exprAnd => Lipseste exprOrPrim");
        } else tkerr(crtTk,"exprOrPrim: dupa OR => Lipseste exprAnd");
    }
    return 1;
}


int exprAnd(RetVal *rv){
    //printf("exprAnd()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(exprEq(rv)){
        if(exprAndPrim(rv)){
            return 1;
        } else tkerr(crtTk,"exprAnd: dupa exprEq => Lipseste exprAndPrim");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

//exprAndPrim: AND exprEq exprAndPrim | eps
int exprAndPrim(RetVal *rv){
    //printf("exprAndPrim()\n");
    //translateTkn(crtTk);
    RetVal rve;
    Instr *i1,*i2;Type t,t1,t2;
    if(consume(AND)){
        i1=rv->type.nElements<0?getRVal(rv):lastInstruction;
        t1=rv->type;
        if(exprEq(&rve)){
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                tkerr(crtTk,"a structure cannot be logically tested");
            // after "if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
            //              tkerr(crtTk,"a structure cannot be logically tested");"
            if(rv->type.nElements>=0){      // vectors
                addInstr(O_AND_A);
            }else{  // non-vectors
                i2=getRVal(&rve);t2=rve.type;
                t=getArithType(&t1,&t2);
                addCastInstr(i1,&t1,&t);
                addCastInstr(i2,&t2,&t);
                switch(t.typeBase){
                    case TB_INT:addInstr(O_AND_I);break;
                    case TB_DOUBLE:addInstr(O_AND_D);break;
                    case TB_CHAR:addInstr(O_AND_C);break;
                }
            }
            rv->type=createType(TB_INT,-1);
            rv->isCtVal=rv->isLVal=0;
            if(exprAndPrim(rv)){
                return 1;
            } else tkerr(crtTk,"exprAndPrim: dupa AND exprEq => Lipseste exprAndPrim");
        } else tkerr(crtTk,"exprAndPrim: dupa AND => Lipseste exprEq");
    }
    return 1;
}


int exprEq(RetVal *rv){
    //printf("exprEq()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(exprRel(rv)){
        if(exprEqPrim(rv)){
            return 1;
        } else tkerr(crtTk,"exprEq: dupa exprRel => Lipseste exprEqPrim");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

//exprEqPrim: (EQUAL|NOTEQ) exprRel exprEqPrim | eps
int exprEqPrim(RetVal *rv){
    //printf("exprEqPrim()\n");
    //translateTkn(crtTk);
    RetVal rve;
    Instr *i1,*i2;Type t,t1,t2;
    Token *tkop;
    if(consume(EQUAL) || consume(NOTEQ)){
        tkop=consumedTk;
        i1=rv->type.nElements<0?getRVal(rv):lastInstruction;
        t1=rv->type;
        if(exprRel(&rve)){
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                tkerr(crtTk,"a structure cannot be compared");
            // after "if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
            //              tkerr(crtTk,"a structure cannot be compared");"
            if(rv->type.nElements>=0){      // vectors
                addInstr(tkop->code==EQUAL?O_EQ_A:O_NOTEQ_A);
            }else{  // non-vectors
                i2=getRVal(&rve);t2=rve.type;
                t=getArithType(&t1,&t2);
                addCastInstr(i1,&t1,&t);
                addCastInstr(i2,&t2,&t);
                if(tkop->code==EQUAL){
                    switch(t.typeBase){
                        case TB_INT:addInstr(O_EQ_I);break;
                        case TB_DOUBLE:addInstr(O_EQ_D);break;
                        case TB_CHAR:addInstr(O_EQ_C);break;
                    }
                }else{
                    switch(t.typeBase){
                        case TB_INT:addInstr(O_NOTEQ_I);break;
                        case TB_DOUBLE:addInstr(O_NOTEQ_D);break;
                        case TB_CHAR:addInstr(O_NOTEQ_C);break;
                    }
                }
            }
            rv->type=createType(TB_INT,-1);
            rv->isCtVal=rv->isLVal=0;
            if(exprEqPrim(rv)){
                return 1;
            } else tkerr(crtTk,"exprEqPrim: dupa (EQUAL|NOTEQ) exprRel => Lipseste exprEqPrim");
        } else tkerr(crtTk,"exprEqPrim: dupa (EQUAL|NOTEQ) => Lipseste exprRel");
    }
    return 1;
}


int exprRel(RetVal *rv){
    //printf("exprRel()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(exprAdd(rv)){
        if(exprRelPrim(rv)){
            return 1;
        } else tkerr(crtTk,"exprRel: dupa exprAdd => Lipseste exprRelPrim");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

//exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | eps
int exprRelPrim(RetVal *rv){
    //printf("exprRelPrim()\n");
    //translateTkn(crtTk);
    RetVal rve;
    Instr *i1,*i2;Type t,t1,t2;
    Token *tkop;
    if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
        tkop=consumedTk;
        i1=getRVal(rv);
        t1=rv->type;
        if(exprAdd(&rve)){
            if(rv->type.nElements>-1||rve.type.nElements>-1)
                tkerr(crtTk,"an array cannot be compared");
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                tkerr(crtTk,"a structure cannot be compared");
            // after "if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)"
            //              "tkerr(crtTk,"a structure cannot be compared");"
            i2=getRVal(&rve);t2=rve.type;
            t=getArithType(&t1,&t2);
            addCastInstr(i1,&t1,&t);
            addCastInstr(i2,&t2,&t);
            switch(tkop->code){
                case LESS:
                    switch(t.typeBase){
                        case TB_INT:addInstr(O_LESS_I);break;
                        case TB_DOUBLE:addInstr(O_LESS_D);break;
                        case TB_CHAR:addInstr(O_LESS_C);break;
                    }
                    break;
                case LESSEQ:
                    switch(t.typeBase){
                        case TB_INT:addInstr(O_LESSEQ_I);break;
                        case TB_DOUBLE:addInstr(O_LESSEQ_D);break;
                        case TB_CHAR:addInstr(O_LESSEQ_C);break;
                    }
                    break;
                case GREATER:
                    switch(t.typeBase){
                        case TB_INT:addInstr(O_GREATER_I);break;
                        case TB_DOUBLE:addInstr(O_GREATER_D);break;
                        case TB_CHAR:addInstr(O_GREATER_C);break;
                    }
                    break;
                case GREATEREQ:
                    switch(t.typeBase){
                        case TB_INT:addInstr(O_GREATEREQ_I);break;
                        case TB_DOUBLE:addInstr(O_GREATEREQ_D);break;
                        case TB_CHAR:addInstr(O_GREATEREQ_C);break;
                    }
                    break;
            }
            rv->type=createType(TB_INT,-1);
            rv->isCtVal=rv->isLVal=0;
            if(exprRelPrim(rv)){
                return 1;
            } else tkerr(crtTk,"exprRelPrim: dupa ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd => Lipseste exprRelPrim");
        } else tkerr(crtTk,"exprRelPrim: dupa ( LESS | LESSEQ | GREATER | GREATEREQ ) => Lipseste exprAdd");
    }
    return 1;
}


int exprAdd(RetVal *rv){
    //printf("exprAdd()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(exprMul(rv)){
        if(exprAddPrim(rv)){
            return 1;
        } else tkerr(crtTk,"exprAdd: dupa exprMul => Lipseste exprAddPrim");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

//exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | eps
int exprAddPrim(RetVal *rv){
    //printf("exprAddPrim()\n");
    //translateTkn(crtTk);
    RetVal rve;
    Instr *i1,*i2;Type t1,t2;
    Token *tkop;
    if(consume(ADD) || consume(SUB)){
        tkop=consumedTk;
        i1=getRVal(rv);t1=rv->type;
        if(exprMul(&rve)){
            if(rv->type.nElements>-1||rve.type.nElements>-1)
                tkerr(crtTk,"an array cannot be added or subtracted");
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                tkerr(crtTk,"a structure cannot be added or subtracted");
            rv->type=getArithType(&rv->type,&rve.type);
            // after "rv->type=getArithType(&rv->type,&rve.type);"
            i2=getRVal(&rve);t2=rve.type;
            addCastInstr(i1,&t1,&rv->type);
            addCastInstr(i2,&t2,&rv->type);
            if(tkop->code==ADD){
                switch(rv->type.typeBase){
                    case TB_INT:addInstr(O_ADD_I);break;
                    case TB_DOUBLE:addInstr(O_ADD_D);break;
                    case TB_CHAR:addInstr(O_ADD_C);break;
                }
            }else{
                switch(rv->type.typeBase){
                    case TB_INT:addInstr(O_SUB_I);break;
                    case TB_DOUBLE:addInstr(O_SUB_D);break;
                    case TB_CHAR:addInstr(O_SUB_C);break;
                }
            }
            rv->isCtVal=rv->isLVal=0;
            if(exprAddPrim(rv)){
                return 1;
            } else tkerr(crtTk,"exprAddPrim: dupa ( ADD | SUB ) exprMul => Lipseste exprAddPrim");
        } else tkerr(crtTk,"exprAddPrim: dupa ( ADD | SUB ) => Lipseste exprMul");
    }
    return 1;
}


int exprMul(RetVal *rv){
    //printf("exprMul()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(exprCast(rv)){
        if(exprMulPrim(rv)){
            return 1;
        } else tkerr(crtTk,"exprMul: dupa exprCast => Lipseste exprMulPrim");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

//exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | eps
int exprMulPrim(RetVal *rv){
    //printf("exprMulPrim()\n");
    //translateTkn(crtTk);
    RetVal rve;
    Instr *i1,*i2;Type t1,t2;
    Token *tkop;
    if(consume(MUL) || consume(DIV)){
        tkop=consumedTk;
        i1=getRVal(rv);t1=rv->type;
        if(exprCast(&rve)){
            if(rv->type.nElements>-1||rve.type.nElements>-1)
                tkerr(crtTk,"an array cannot be multiplied or divided");
            if(rv->type.typeBase==TB_STRUCT||rve.type.typeBase==TB_STRUCT)
                tkerr(crtTk,"a structure cannot be multiplied or divided");
            rv->type=getArithType(&rv->type,&rve.type);
            // after "rv->type=getArithType(&rv->type,&rve.type);"
            i2=getRVal(&rve);t2=rve.type;
            addCastInstr(i1,&t1,&rv->type);
            addCastInstr(i2,&t2,&rv->type);
            if(tkop->code==MUL){
                switch(rv->type.typeBase){
                    case TB_INT:addInstr(O_MUL_I);break;
                    case TB_DOUBLE:addInstr(O_MUL_D);break;
                    case TB_CHAR:addInstr(O_MUL_C);break;
                }
            }else{
                switch(rv->type.typeBase){
                    case TB_INT:addInstr(O_DIV_I);break;
                    case TB_DOUBLE:addInstr(O_DIV_D);break;
                    case TB_CHAR:addInstr(O_DIV_C);break;
                }
            }
            rv->isCtVal=rv->isLVal=0;
            if(exprMulPrim(rv)){
                return 1;
            } else tkerr(crtTk,"exprMulPrim: dupa ( MUL | DIV ) exprCast => Lipseste exprMulPrim");
        } else tkerr(crtTk,"exprMulPrim: dupa ( MUL | DIV ) => Lipseste exprCast");
    }
    return 1;
}


int exprCast(RetVal *rv){
    //printf("exprCast()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Type t;
    RetVal rve;
    if(consume(LPAR)){
        if(typeName(&t)){
            if(consume(RPAR)){
                if(exprCast(&rve)) {
                    cast(&t,&rve.type);
                    // after "cast(&t,&rve.type);"
                    if(rv->type.nElements<0&&rv->type.typeBase!=TB_STRUCT){
                        switch(rve.type.typeBase){
                            case TB_CHAR:
                                switch(t.typeBase){
                                    case TB_INT:addInstr(O_CAST_C_I);break;
                                    case TB_DOUBLE:addInstr(O_CAST_C_D);break;
                                }
                                break;
                            case TB_DOUBLE:
                                switch(t.typeBase){
                                    case TB_CHAR:addInstr(O_CAST_D_C);break;
                                    case TB_INT:addInstr(O_CAST_D_I);break;
                                }
                                break;
                            case TB_INT:
                                switch(t.typeBase){
                                    case TB_CHAR:addInstr(O_CAST_I_C);break;
                                    case TB_DOUBLE:addInstr(O_CAST_I_D);break;
                                }
                                break;
                        }
                    }
                    rv->type=t;
                    rv->isCtVal=rv->isLVal=0;
                    return 1;
                } else tkerr(crtTk,"exprCast: dupa LPAR typeName RPAR => Lipseste exprCast");
            } else tkerr(crtTk,"exprCast: dupa LPAR typeName => Lipseste )");
        }
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    if(exprUnary(rv)){
        return 1;
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int exprUnary(RetVal *rv){
    //printf("exprUnary()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Token *tkop;
    if(consume(SUB) || consume(NOT)){
        tkop = consumedTk;
        if(exprUnary(rv)){
            if(tkop->code==SUB){
                if(rv->type.nElements>=0)
                    tkerr(crtTk,"unary '-' cannot be applied to an array");
                if(rv->type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"unary '-' cannot be applied to a struct");
                // after "if(rv->type.typeBase==TB_STRUCT)
                //           tkerr(crtTk,"unary '-' cannot be applied to a struct");"
                getRVal(rv);
                switch(rv->type.typeBase){
                    case TB_CHAR:addInstr(O_NEG_C);break;
                    case TB_INT:addInstr(O_NEG_I);break;
                    case TB_DOUBLE:addInstr(O_NEG_D);break;
                }
            }else{  // NOT
                if(rv->type.typeBase==TB_STRUCT)
                    tkerr(crtTk,"'!' cannot be applied to a struct");
                // after "if(rv->type.typeBase==TB_STRUCT)
                //          tkerr(crtTk,"'!' cannot be applied to a struct");"
                if(rv->type.nElements<0){
                    getRVal(rv);
                    switch(rv->type.typeBase){
                        case TB_CHAR:addInstr(O_NOT_C);break;
                        case TB_INT:addInstr(O_NOT_I);break;
                        case TB_DOUBLE:addInstr(O_NOT_D);break;
                    }
                }else{
                    addInstr(O_NOT_A);
                }
                rv->type=createType(TB_INT,-1);
            }
            rv->isCtVal=rv->isLVal=0;
            return 1;
        } else tkerr(crtTk,"exprUnary: dupa ( SUB | NOT ) => Lipseste exprUnary");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    if(exprPostfix(rv)){
        return 1;
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


int exprPostfix(RetVal *rv){
    //printf("exprPostfix()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    if(exprPrimary(rv)){
        if(exprPostfixPrim(rv)){
            return 1;
        } else tkerr(crtTk,"exprPostfix: dupa exprPrimary => Lipseste exprPostfixPrim");
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}

//exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim | DOT ID exprPostfixPrim | eps
int exprPostfixPrim(RetVal *rv){
    //printf("exprPostfixPrim()\n");
    //translateTkn(crtTk);
    RetVal rve;
    Token *tkName;
    if(consume(LBRACKET)){
        if(expr(&rve)){
            if(rv->type.nElements<0)tkerr(crtTk,"only an array can be indexed");
            Type typeInt=createType(TB_INT,-1);
            cast(&typeInt,&rve.type);
            rv->type=rv->type;
            rv->type.nElements=-1;
            rv->isLVal=1;
            rv->isCtVal=0;
            if(consume(RBRACKET)){
                addCastInstr(lastInstruction,&rve.type,&typeInt);
                getRVal(&rve);
                if(typeBaseSize(&rv->type)!=1){
                    addInstrI(O_PUSHCT_I,typeBaseSize(&rv->type));
                    addInstr(O_MUL_I);
                }
                addInstr(O_OFFSET);
                if(exprPostfixPrim(rv)){
                    return 1;
                } else tkerr(crtTk,"exprPostfixPrim: dupa LBRACKET expr RBRACKET => Lipseste exprPostfixPrim");
            } else tkerr(crtTk,"exprPostfixPrim: dupa LBRACKET expr => Lipseste ]");
        } else tkerr(crtTk,"exprPostfixPrim: dupa LBRACKET => Lipseste expr");
    }
    if(consume(DOT)){
        if(consume(ID)){
            tkName = consumedTk;
            if(exprPostfixPrim(rv)){
                Symbol *sStruct=rv->type.s;
                Symbol *sMember=findSymbol(&sStruct->members,tkName->text);
                /**CHANGED*/
                if(sMember->offset){
                    addInstrI(O_PUSHCT_I,sMember->offset);
                    addInstr(O_OFFSET);
                }
                if(!sMember)
                    tkerr(crtTk,"struct %s does not have a member %s",sStruct->name,tkName->text);
                rv->type=sMember->type;
                rv->isLVal=1;
                rv->isCtVal=0;
                return 1;
            } else tkerr(crtTk,"exprPostfixPrim: dupa DOT ID => Lipseste exprPostfixPrim");
        } else tkerr(crtTk,"exprPostfixPrim: dupa DOT => Lipseste ID");
    }
    return 1;
}


int exprPrimary(RetVal *rv){
    //printf("exprPrimary()\n");
    //translateTkn(crtTk);
    Token *initialTk = crtTk;
    Instr* startLastInstr = lastInstruction;
    Token *tkName;
    Token *tki;
    Token *tkc;
    Token *tkr;
    Token *tks;
    RetVal arg;
    Instr *i;
    if(consume(ID)){
        tkName = consumedTk;
        Symbol *s=findSymbol(&symbols,tkName->text);
        if(!s) tkerr(crtTk,"undefined symbol %s",tkName->text);
        rv->type=s->type;
        rv->isCtVal=0;
        rv->isLVal=1;
        if(consume(LPAR)){
            Symbol **crtDefArg=s->args.begin;
            if(s->cls!=CLS_FUNC&&s->cls!=CLS_EXTFUNC)
                tkerr(crtTk,"call of the non-function %s",tkName->text);
            if(expr(&arg)){
                if(crtDefArg==s->args.end)tkerr(crtTk,"too many arguments in call");
                cast(&(*crtDefArg)->type,&arg.type);
                //after 1st "cast(&(*crtDefArg)->type,&arg.type);"
                //and before 1st "crtDefArg++;"
                if((*crtDefArg)->type.nElements<0){  //only arrays are passed by addr
                    i=getRVal(&arg);
                }else{
                    i=lastInstruction;
                }
                addCastInstr(i,&arg.type,&(*crtDefArg)->type);
                crtDefArg++;
                for(;;){
                    if(consume(COMMA)){
                        if(expr(&arg)){
                            if(crtDefArg==s->args.end)tkerr(crtTk,"too many arguments in call");
                            cast(&(*crtDefArg)->type,&arg.type);
                            //after 2nd "cast(&(*crtDefArg)->type,&arg.type);"
                            //and before 2nd "crtDefArg++;"
                            if((*crtDefArg)->type.nElements<0){
                                i=getRVal(&arg);
                            }else{
                                i=lastInstruction;
                            }
                            addCastInstr(i,&arg.type,&(*crtDefArg)->type);
                            crtDefArg++;
                        } else tkerr(crtTk,"exprPrimary: dupa ID ( LPAR ( expr ( COMMA => Lipseste expr");
                    } else break;
                }
            }
            if(consume(RPAR)){
                // function call
                i=addInstr(s->cls==CLS_FUNC?O_CALL:O_CALLEXT);
                i->args->addr =s->addr; /**CHANGED from i->addr to i->args->addr*/
                if(crtDefArg!=s->args.end)tkerr(crtTk,"too few arguments in call");
                rv->type=s->type;
                rv->isCtVal=rv->isLVal=0;
                return 1;
            } else tkerr(crtTk,"exprPrimary: dupa ID ( LPAR ( expr ( COMMA expr )* )? => Lipseste )");
        }
        if(s->cls==CLS_FUNC||s->cls==CLS_EXTFUNC)
            tkerr(crtTk,"missing call for function %s",tkName->text);
        // variable
        if(s->depth){
            addInstrI(O_PUSHFPADDR,s->offset);
        }else{
            addInstrA(O_PUSHCT_A,s->addr);
        }
        return 1;
    }
    if(consume(CT_INT)){
        tki = consumedTk;
        rv->type=createType(TB_INT,-1);rv->ctVal.i=tki->i;
        rv->isCtVal=1;rv->isLVal=0;
        addInstrI(O_PUSHCT_I,tki->i);
        return 1;
    }
    if(consume(CT_REAL)){
        tkr = consumedTk;
        rv->type=createType(TB_DOUBLE,-1);rv->ctVal.d=tkr->r;
        rv->isCtVal=1;rv->isLVal=0;
        i=addInstr(O_PUSHCT_D);i->args[0].d=tkr->r;
        return 1;
    }
    if(consume(CT_CHAR)){
        tkc = consumedTk;
        rv->type=createType(TB_CHAR,-1);rv->ctVal.i=tkc->i;
        rv->isCtVal=1;rv->isLVal=0;
        addInstrI(O_PUSHCT_C,tkc->i);
        return 1;
    }
    if(consume(CT_STRING)){
        tks = consumedTk;
        rv->type=createType(TB_CHAR,0);rv->ctVal.str=tks->text;
        rv->isCtVal=1;rv->isLVal=0;
        addInstrA(O_PUSHCT_A,tks->text);
        return 1;
    }
    if(consume(LPAR)){
        if(expr(rv)){
            if(consume(RPAR)){
                return 1;
            } else tkerr(crtTk,"exprPrimary: dupa LPAR expr => Lipseste )");
        }
    }
    crtTk = initialTk;
    deleteInstructionsAfter(startLastInstr);
    return 0;
}


Symbol *addExtFunc(const char *name,Type type,void *addr) {
    Symbol *s=addSymbol(&symbols,name,CLS_EXTFUNC);
    s->type=type;
    s->addr=addr;
    initSymbols(&s->args);
    return s;
}

Symbol *addFuncArg(Symbol *func,const char *name,Type type) {
    Symbol *a=addSymbol(&func->args,name,CLS_VAR);
    a->type=type;
    return a;
}


addDefaultFuncs(){
    Symbol *s,*a;
    s=addExtFunc("put_i",createType(TB_VOID,-1),put_i);
    a=addSymbol(&s->args,"i",CLS_VAR);
    a->type=createType(TB_INT,-1);

    s=addExtFunc("put_s",createType(TB_VOID,-1),put_s);
    a=addSymbol(&s->args,"s",CLS_VAR);
    a->type=createType(TB_CHAR,0);

    s=addExtFunc("put_d",createType(TB_VOID,-1),put_d);
    a=addSymbol(&s->args,"d",CLS_VAR);
    a->type=createType(TB_DOUBLE,-1);

    s=addExtFunc("put_c",createType(TB_VOID,-1),put_c);
    a=addSymbol(&s->args,"c",CLS_VAR);
    a->type=createType(TB_CHAR,-1);

    s=addExtFunc("get_s",createType(TB_VOID,-1),get_s);
    addFuncArg(s,"s",createType(TB_CHAR,0));

    s=addExtFunc("get_i",createType(TB_INT,-1),get_i);

    s=addExtFunc("get_d",createType(TB_DOUBLE,-1),get_d);

    s=addExtFunc("get_c",createType(TB_CHAR,-1),get_c);

    s=addExtFunc("seconds",createType(TB_DOUBLE,-1),seconds);
}

void run(Instr *IP) {
    int iVal1,iVal2;
    double dVal1,dVal2;
    char cVal1, cVal2;
    char *aVal1,*aVal2;
    char *FP=0,*oldSP;
    SP=stack;
    stackAfter=stack+STACK_SIZE;
    while(1){
        for(int aux=0;aux<1000000;aux++){}
        printf("%p/%ld\t",IP,SP-stack);
        switch(IP->opcode) {
            case O_ADD_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("ADD_C\t(%c+%c -> %c)\n", cVal2, cVal1, cVal2 + cVal1);
                pushc(cVal2 + cVal1);
                IP = IP->next;
                break;
            case O_ADD_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("ADD_D\t(%g+%g -> %g)\n", dVal2, dVal1, dVal2 + dVal1);
                pushd(dVal2 + dVal1);
                IP = IP->next;
                break;
            case O_ADD_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("ADD_I\t(%d+%d -> %d)\n", iVal2, iVal1, iVal2 + iVal1);
                pushi(iVal2 + iVal1);
                IP = IP->next;
                break;

            case O_AND_A:
                aVal1 = popa();
                aVal2 = popa();
                printf("AND_A\t(%s==%s -> %d)\n", aVal2, aVal1, aVal2 && aVal1);
                pushi(aVal2 && aVal1);
                IP = IP->next;
                break;
            case O_AND_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("AND_C\t(%c-%c -> %c)\n", cVal2, cVal1, cVal2 && cVal1);
                pushc(cVal2 && cVal1);
                IP = IP->next;
                break;
            case O_AND_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("AND_D\t(%g-%g -> %c)\n", dVal2, dVal1, dVal2 && dVal1);
                pushd(dVal2 && dVal1);
                IP = IP->next;
                break;
            case O_AND_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("AND_I\t(%d-%d -> %c)\n", iVal2, iVal1, iVal2 && iVal1);
                pushi(iVal2 && iVal1);
                IP = IP->next;
                break;


            case O_CALL:
                aVal1=IP->args[0].addr;
                printf("CALL\t%p\n",(void*)aVal1); 
                pusha(IP->next);
                IP=(Instr*)aVal1;
                break;

            case O_CALLEXT:
                printf("CALLEXT\t%p\n",IP->args[0].addr);
                (*(void(*)())IP->args[0].addr)();
                IP=IP->next;
                break;

            case O_CAST_C_D:
                cVal1=popc();
                dVal1=(double)cVal1;
                printf("CAST_C_D\t(%c -> %g)\n",cVal1,dVal1);
                pushd(dVal1);
                IP=IP->next;
                break;
            case O_CAST_C_I:
                cVal1=popc();
                iVal2=(int)cVal1;
                printf("CAST_C_I\t(%d -> %d)\n",cVal1,iVal2);
                pushi(iVal2);
                IP=IP->next;
                break;
            case O_CAST_D_C:
                dVal1 = popd();
                cVal2 = (char)dVal1;
                printf("CAST_D_C\t(%g -> %c)\n", dVal1, cVal2);
                pushd(cVal2);
                IP = IP->next;
                break;
            case O_CAST_D_I:
                dVal1 = popd();
                iVal2 = (int)dVal1;
                printf("CAST_D_I\t(%g -> %d)\n", dVal1, iVal2);
                pushd(iVal2);
                IP = IP->next;
                break;
            case O_CAST_I_C:
                iVal1 = popi();
                cVal2 = (char)iVal1;
                printf("CAST_I_C\t(%d -> %c)\n", iVal1, cVal2);
                pushc(cVal2);
                IP = IP->next;
                break;
            case O_CAST_I_D:
                iVal1=popi();
                dVal1=(double)iVal1;
                printf("CAST_I_D\t(%d -> %g)\n",iVal1,dVal1);
                pushd(dVal1);
                IP=IP->next;
                break;

            case O_DIV_C:
                iVal1 = popc();
                iVal2 = popc();
                printf("DIV_C\t(%c/%c -> %c)\n", iVal2, iVal1, iVal2 / iVal1);
                pushc((char) (iVal2 / iVal1));
                IP = IP->next;
                break;
            case O_DIV_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("DIV_D\t(%g/%g -> %g)\n", dVal2, dVal1, dVal2 / dVal1);
                pushd(dVal2 / dVal1);
                IP = IP->next;
                break;
            case O_DIV_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("DIV_I\t(%d/%d -> %d)\n", iVal2, iVal1, iVal2 / iVal1);
                pushi(iVal2 / iVal1);
                IP = IP->next;
                break;

            case O_DROP:
                iVal1=IP->args[0].i;
                printf("DROP\t%d\n",iVal1);
                if(SP-iVal1<stack) myerr("not enough stack bytes");
                SP-=iVal1;
                IP=IP->next;
                break;

            case O_ENTER:
                iVal1=IP->args[0].i;
                printf("ENTER\t%d\n",iVal1);
                pusha(FP);
                FP=SP;
                SP+=iVal1;
                IP=IP->next;
                break;

            case O_EQ_A:
                aVal1 = (char *)popa();
                aVal2 = (char *)popa();
                printf("EQ_A\t(%s==%s -> %d)\n", aVal2, aVal1, aVal2 == aVal1);
                pushi(aVal2 == aVal1);
                IP = IP->next;
                break;
            case O_EQ_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("EQ_C\t(%c==%c -> %d)\n", cVal2, cVal1, cVal2 == cVal1);
                pushi(cVal2 == cVal1);
                IP = IP->next;
                break;
            case O_EQ_D:
                dVal1=popd();
                dVal2=popd();
                printf("EQ_D\t(%g==%g -> %d)\n",dVal2,dVal1,dVal2 == dVal1);
                pushi(dVal2==dVal1);
                IP=IP->next;
                break;
            case O_EQ_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("EQ_I\t(%d==%d -> %d)\n", iVal2, iVal1, iVal2 == iVal1);
                pushi(iVal2 == iVal1);
                IP = IP->next;
                break;

            case O_GREATER_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("GREATER_C\t(%c>%c -> %d)\n", cVal2, cVal1, cVal2 > cVal1);
                pushi(cVal2 > cVal1);
                IP = IP->next;
                break;
            case O_GREATER_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("GREATER_D\t(%g>%g -> %d)\n", dVal2, dVal1, dVal2 > dVal1);
                pushi(dVal2 > dVal1);
                IP = IP->next;
                break;
            case O_GREATER_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("GREATER_I\t(%d>%d -> %d)\n", iVal2, iVal1, iVal2 > iVal1);
                pushi(iVal2 > iVal1);
                IP = IP->next;
                break;

            case O_GREATEREQ_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("GREATEREQ_C\t(%c>=%c -> %d)\n", cVal2, cVal1, cVal2 >= cVal1);
                pushi(cVal2 >= cVal1);
                IP = IP->next;
                break;
            case O_GREATEREQ_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("GREATEREQ_D\t(%g>=%g -> %d)\n", dVal2, dVal1, dVal2 >= dVal1);
                pushi(dVal2 >= dVal1);
                IP = IP->next;
                break;
            case O_GREATEREQ_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("GREATEREQ_I\t(%d>=%d -> %d)\n", iVal2, iVal1, iVal2 >= iVal1);
                pushi(iVal2 >= iVal1);
                IP = IP->next;
                break;

            case O_HALT:
                printf("HALT\n");
                return;

            case O_INSERT:
                iVal1=IP->args[0].i; // iDst
                iVal2=IP->args[1].i; // nBytes
                printf("INSERT\t%d,%d\n",iVal1,iVal2);
                if(SP+iVal2>stackAfter) myerr("out of stack");
                memmove(SP-iVal1+iVal2,SP-iVal1,iVal1); //make room
                memmove(SP-iVal1,SP+iVal2,iVal2); SP+=iVal2; //dup
                IP=IP->next;
                break;

            case O_JF_A:
                aVal1 = (char *)popa();
                printf("JF\t%p\t(%s)\n", IP->args[0].addr, aVal1);
                IP = aVal1 ? IP->next : (Instr *)IP->args[0].addr;
                break;
            case O_JF_C:
                cVal1 = popc();
                printf("JF\t%p\t(%c)\n", IP->args[0].addr, cVal1);
                IP = cVal1 ? IP->next : (Instr *)IP->args[0].addr;
                break;
            case O_JF_D:
                dVal1 = popd();
                printf("JF\t%p\t(%g)\n", IP->args[0].addr, dVal1);
                IP = dVal1 ? IP->next : (Instr *)IP->args[0].addr;
                break;
            case O_JF_I:
                iVal1 = popi();
                printf("JF\t%p\t(%d)\n", IP->args[0].addr, iVal1);
                IP = iVal1 ? IP->next : (Instr *)IP->args[0].addr;
                break;

            case O_JMP:
                IP = (Instr *)IP->args[0].addr;
                break;

            case O_JT_A:
                aVal1 = (char *)popa();
                printf("JT\t%p\t(%s)\n", IP->args[0].addr, aVal1);
                IP = aVal1 ? (Instr *)IP->args[0].addr : IP->next;
                break;
            case O_JT_C:
                cVal1 = popc();
                printf("JT\t%p\t(%c)\n", IP->args[0].addr, cVal1);
                IP = cVal1 ? (Instr *)IP->args[0].addr : IP->next;
                break;
            case O_JT_D:
                dVal1 = popd();
                printf("JT\t%p\t(%g)\n", IP->args[0].addr, dVal1);
                IP = dVal1 ? (Instr *)IP->args[0].addr : IP->next;
                break;
            case O_JT_I:
                iVal1=popi();
                printf("JT\t%p\t(%d)\n",IP->args[0].addr,iVal1);
                IP=iVal1?IP->args[0].addr:IP->next;
                break;

            case O_LESS_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("LESS_C\t(%c<%c -> %d)\n", cVal2, cVal1, cVal2 < cVal1);
                pushi(cVal2 < cVal1);
                IP = IP->next;
                break;
            case O_LESS_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("LESS_D\t(%g<%g -> %d)\n", dVal2, dVal1, dVal2 < dVal1);
                pushi(dVal2 < dVal1);
                IP = IP->next;
                break;
            case O_LESS_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("LESS_I\t(%d<%d -> %d)\n", iVal2, iVal1, iVal2 < iVal1);
                pushi(iVal2 < iVal1);
                IP = IP->next;
                break;

            case O_LESSEQ_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("LESSEQ_C\t(%c<=%c -> %d)\n", cVal2, cVal1, cVal2 <= cVal1);
                pushi(cVal2 <= cVal1);
                IP = IP->next;
                break;
            case O_LESSEQ_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("LESSEQ_D\t(%g<=%g -> %d)\n", dVal2, dVal1, dVal2 <= dVal1);
                pushi(dVal2 <= dVal1);
                IP = IP->next;
                break;
            case O_LESSEQ_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("LESSEQ_I\t(%d<=%d -> %d)\n", iVal2, iVal1, iVal2 <= iVal1);
                pushi(iVal2 <= iVal1);
                IP = IP->next;
                break;

            case O_LOAD:
                iVal1=IP->args[0].i;
                aVal1=popa(); printf("LOAD\t%d\t(%p)\n",iVal1,(void*)aVal1);
                if(SP+iVal1>stackAfter) myerr("out of stack");
                memcpy(SP,aVal1,iVal1);
                SP+=iVal1;
                IP=IP->next;
                break;

            case O_MUL_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("MUL_C\t(%c*%c -> %c)\n", cVal2, cVal1, cVal2 * cVal1);
                pushc(cVal2 * cVal1);
                IP = IP->next;
                break;
            case O_MUL_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("MUL_D\t(%g*%g -> %g)\n", dVal2, dVal1, dVal2 * dVal1);
                pushd(dVal2 * dVal1);
                IP = IP->next;
                break;
            case O_MUL_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("MUL_I\t(%d*%d -> %d)\n", iVal2, iVal1, iVal2 * iVal1);
                pushi(iVal2 * iVal1);
                IP = IP->next;
                break;

            case O_NEG_C:
                cVal1 = popc();
                printf("NEG_C\t(-%c -> %c)\n", cVal1, 0 - cVal1);
                pushc(0 - cVal1);
                IP = IP->next;
                break;
            case O_NEG_D:
                dVal1 = popd();
                printf("NEG_D\t(-%g -> %g)\n", dVal1, 0 - dVal1);
                pushd(0 - dVal1);
                IP = IP->next;
                break;
            case O_NEG_I:
                iVal1 = popi();
                printf("NEG_I\t(-%d -> %d)\n", iVal1, 0 - iVal1);
                pushi(0 - iVal1);
                IP = IP->next;
                break;

            case O_NOP:
                IP = IP->next;
                break;

            case O_NOT_A:
                aVal1 = (char*)popa();
                printf("NOT_C\t(!%s -> %s)\n", aVal1, !aVal1);
                pushi(!aVal1);
                IP = IP->next;
                break;
            case O_NOT_C:
                cVal1 = popc();
                printf("NOT_C\t(!%c -> %c)\n", cVal1, !cVal1);
                pushc(!cVal1);
                IP = IP->next;
                break;
            case O_NOT_D:
                dVal1 = popd();
                printf("NOT_D\t(!%g -> %d)\n", dVal1, !dVal1);
                pushd(!dVal1);
                IP = IP->next;
                break;
            case O_NOT_I:
                iVal1 = popi();
                printf("NOT_I\t(!%d -> %d)\n", iVal1, !iVal1);
                pushi(!iVal1);
                IP = IP->next;
                break;

            case O_NOTEQ_A:
                aVal1 = (char *)popa();
                aVal2 = (char *)popa();
                printf("NOTEQ_A\t(%s!=%s -> %d)\n", aVal2, aVal1, aVal2 != aVal1);
                pushi(aVal2 != aVal1);
                IP = IP->next;
                break;
            case O_NOTEQ_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("NOTEQ_C\t(%c!=%c -> %d)\n", cVal2, cVal1, cVal2 != cVal1);
                pushi(cVal2 != cVal1);
                IP = IP->next;
                break;
            case O_NOTEQ_D:
                dVal1 = popd();
                dVal2 = popd();
                printf("NOTEQ_D\t(%g!=%g -> %d)\n", dVal2, dVal1, dVal2 != dVal1);
                pushi(dVal2 != dVal1);
                IP = IP->next;
                break;
            case O_NOTEQ_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("NOTEQ_I\t(%d!=%d -> %d)\n", iVal2, iVal1, iVal2 != iVal1);
                pushi(iVal2 != iVal1);
                IP = IP->next;
                break;

            case O_OFFSET:
                iVal1=popi();
                aVal1=popa();
                printf("OFFSET\t(%p+%d -> %p)\n",(void*)aVal1,iVal1,(void*)aVal1+iVal1);
                pusha(aVal1+iVal1);
                IP=IP->next;
                break;

            case O_PUSHFPADDR:
                iVal1=IP->args[0].i;
                printf("PUSHFPADDR\t%d\t(%p)\n",iVal1,(void*)FP+iVal1);
                pusha(FP+iVal1);
                IP=IP->next;
                break;

            case O_PUSHCT_A:
                aVal1=IP->args[0].addr;
                printf("PUSHCT_A\t%p\n",(void*)aVal1);
                pusha(aVal1);
                IP=IP->next;
                break;
            case O_PUSHCT_C:
                cVal1 = (char)IP->args[0].addr;
                printf("PUSHCT_C\t%c\n", cVal1);
                pushc(cVal1);
                IP = IP->next;
                break;
            case O_PUSHCT_D:
                dVal1 = (int)IP->args[0].addr;
                printf("PUSHCT_D\t%g\n", dVal1);
                pushd(dVal1);
                IP = IP->next;
                break;
            case O_PUSHCT_I:
                iVal1 = (int)IP->args[0].addr;
                printf("PUSHCT_I\t%d\n", iVal1);
                pushi(iVal1);
                IP = IP->next;
                break;

            case O_RET:
                iVal1=IP->args[0].i; // sizeArgs
                iVal2=IP->args[1].i; // sizeof(retType)
                printf("RET\t%d,%d\n",iVal1,iVal2);
                oldSP=SP;
                SP=FP;
                FP=popa();
                IP=popa();
                if(SP-iVal1<stack) myerr("not enough stack bytes");
                SP-=iVal1;
                memmove(SP,oldSP-iVal2,iVal2);
                SP+=iVal2;
                break;

            case O_STORE:
                iVal1=IP->args[0].i;
                if(SP-(sizeof(void*)+iVal1)<stack) myerr("not enough stack bytes for STORE");
                aVal1=*(void**)(SP-((sizeof(void*)+iVal1)));
                printf("STORE\t%d\t(%p)\n",iVal1,(void*)aVal1);
                memcpy(aVal1,SP-iVal1,iVal1);
                SP-=sizeof(void*)+iVal1;
                IP=IP->next;
                break;

            case O_SUB_C:
                cVal1 = popc();
                cVal2 = popc();
                printf("SUB_C\t(%c-%c -> %c)\n", cVal2, cVal1, cVal2 - cVal1);
                pushc(cVal2 - cVal1);
                IP = IP->next;
                break;
            case O_SUB_D:
                dVal1=popd(); dVal2=popd();
                printf("SUB_D\t(%g-%g -> %g)\n",dVal2,dVal1,dVal2-dVal1);
                pushd(dVal2-dVal1);
                IP=IP->next;
                break;
            case O_SUB_I:
                iVal1 = popi();
                iVal2 = popi();
                printf("SUB_I\t(%d-%d -> %d)\n", iVal2, iVal1, iVal2 - iVal1);
                pushi(iVal2 - iVal1);
                IP = IP->next;
                break;
            default:
                myerr("invalid opcode: %d",IP->opcode);
        }
    }
}
void printOperations(Instr *IP){
    SP=stack;
    stackAfter=stack+STACK_SIZE;
    while(IP!=NULL){
        switch(IP->opcode) {
            case O_ADD_C: printf("O_ADD_C\n"); break;
            case O_ADD_D: printf("O_ADD_D\n"); break;
            case O_ADD_I: printf("O_ADD_I\n"); break;
            case O_AND_A: printf("O_AND_A\n"); break;
            case O_AND_C: printf("O_AND_C\n"); break;
            case O_AND_D: printf("O_AND_D\n"); break;
            case O_AND_I: printf("O_AND_I\n"); break;
            case O_CALL: printf("O_CALL\n"); break;
            case O_CALLEXT: printf("O_CALLEXT\n"); break;
            case O_CAST_C_D: printf("O_CAST_C_D\n"); break;
            case O_CAST_C_I: printf("O_CAST_C_I\n"); break;
            case O_CAST_D_C: printf("O_CAST_D_C\n"); break;
            case O_CAST_D_I: printf("O_CAST_D_I\n"); break;
            case O_CAST_I_C: printf("O_CAST_I_C\n"); break;
            case O_CAST_I_D: printf("O_CAST_I_D\n"); break;
            case O_DIV_C: printf("O_DIV_C\n"); break;
            case O_DIV_D: printf("O_DIV_D\n"); break;
            case O_DIV_I: printf("O_DIV_I\n"); break;
            case O_DROP: printf("O_DROP\n"); break;
            case O_ENTER: printf("O_ENTER\n"); break;
            case O_EQ_A: printf("O_EQ_A\n"); break;
            case O_EQ_C: printf("O_EQ_C\n"); break;
            case O_EQ_D: printf("O_EQ_D\n"); break;
            case O_EQ_I: printf("O_EQ_I\n"); break;
            case O_GREATER_C: printf("O_GREATER_C\n"); break;
            case O_GREATER_D: printf("O_GREATER_D\n"); break;
            case O_GREATER_I: printf("O_GREATER_I\n"); break;
            case O_GREATEREQ_C: printf("O_GREATEREQ_C\n"); break;
            case O_GREATEREQ_D: printf("O_GREATEREQ_D\n"); break;
            case O_GREATEREQ_I: printf("O_GREATEREQ_I\n"); break;
            case O_HALT: printf("O_HALT\n"); break;
            case O_INSERT: printf("O_INSERT\n"); break;
            case O_JF_A: printf("O_JF_A\n"); break;
            case O_JF_C: printf("O_JF_C\n"); break;
            case O_JF_D: printf("O_JF_D\n"); break;
            case O_JF_I: printf("O_JF_I\n"); break;
            case O_JMP: printf("O_JMP\n"); break;
            case O_JT_A: printf("O_JT_A\n"); break;
            case O_JT_C: printf("O_JT_C\n"); break;
            case O_JT_D: printf("O_JT_D\n"); break;
            case O_JT_I: printf("O_JT_I\n"); break;
            case O_LESS_C: printf("O_LESS_C\n"); break;
            case O_LESS_D: printf("O_LESS_D\n"); break;
            case O_LESS_I: printf("O_LESS_I\n"); break;
            case O_LESSEQ_C: printf("O_LESSEQ_C\n"); break;
            case O_LESSEQ_D: printf("O_LESSEQ_D\n"); break;
            case O_LESSEQ_I: printf("O_LESSEQ_I\n"); break;
            case O_LOAD: printf("O_LOAD\n"); break;
            case O_MUL_C: printf("O_MUL_C\n"); break;
            case O_MUL_D: printf("O_MUL_D\n"); break;
            case O_MUL_I: printf("O_MUL_I\n"); break;
            case O_NEG_C: printf("O_NEG_C\n"); break;
            case O_NEG_D: printf("O_NEG_D\n"); break;
            case O_NEG_I: printf("O_NEG_I\n"); break;
            case O_NOP: printf("O_NOP\n"); break;
            case O_NOT_A: printf("O_NOT_A\n"); break;
            case O_NOT_C: printf("O_NOT_C\n"); break;
            case O_NOT_D: printf("O_NOT_D\n"); break;
            case O_NOT_I: printf("O_NOT_I\n"); break;
            case O_NOTEQ_A: printf("O_NOTEQ_A\n"); break;
            case O_NOTEQ_C: printf("O_NOTEQ_C\n"); break;
            case O_NOTEQ_D: printf("O_NOTEQ_D\n"); break;
            case O_NOTEQ_I: printf("O_NOTEQ_I\n"); break;
            case O_OFFSET: printf("O_OFFSET\n"); break;
            case O_PUSHFPADDR: printf("O_PUSHFPADDR\n"); break;
            case O_PUSHCT_A: printf("O_PUSHCT_A\n"); break;
            case O_PUSHCT_C: printf("O_PUSHCT_C\n"); break;
            case O_PUSHCT_D: printf("O_PUSHCT_D\n"); break;
            case O_PUSHCT_I: printf("O_PUSHCT_I\n"); break;
            case O_RET: printf("O_RET\n"); break;
            case O_STORE: printf("O_STORE\n"); break;
            case O_SUB_C: printf("O_SUB_C\n"); break;
            case O_SUB_D: printf("O_SUB_D\n"); break;
            case O_SUB_I: printf("O_SUB_I\n"); break;
            default:
                myerr("invalid opcode: %d",IP->opcode);
        }
        IP = IP->next;
    }
}
void mvTest() {
    Instr *L1;
    int *v=allocGlobal(sizeof(int));
    addInstrA(O_PUSHCT_A,v);
    addInstrI(O_PUSHCT_I,3);
    addInstrI(O_STORE,sizeof(int));
    L1=addInstrA(O_PUSHCT_A,v);
    addInstrI(O_LOAD,sizeof(int));
    addInstrA(O_CALLEXT,requireSymbol(&symbols,"put_i")->addr);
    addInstrA(O_PUSHCT_A,v);
    addInstrA(O_PUSHCT_A,v);
    addInstrI(O_LOAD,sizeof(int));
    addInstrI(O_PUSHCT_I,1);
    addInstr(O_SUB_I);
    addInstrI(O_STORE,sizeof(int));
    addInstrA(O_PUSHCT_A,v);
    addInstrI(O_LOAD,sizeof(int));
    addInstrA(O_JT_I,L1);
    addInstr(O_HALT);
}

int main(int argc, char **argv) {

    FILE *fin;
    int a;
    char *buff;
    int i=0;
    buff = (char *)malloc(sizeof(char)*10000);
    if ((fin = fopen(argv[1], "rb")) == NULL)
    {
        printf("ERROR opening the file!\n");
        return -1;
    }
    while ( (a=fgetc(fin)) != EOF){
        buff[i++]= (char) a;
    }
    pCrtCh=buff;
    while (getNextToken()!= END) {}
    printf("\n");
    printTokens();
    fclose(fin);
    
    crtTk = tokens;
    initSymbols(&symbols);
    addDefaultFuncs();
    
    mvTest();
    printOperations(instructions);
    if(unit()) {
        puts("Sintaxa corecta\n");
        //Masina Virtuala
        mvTest();
        printOperations(instructions);
        run(instructions);
        return 0;
    }
    tkerr(crtTk,"Syntax error\n");

    return 0;
}

