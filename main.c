#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

enum{END, ID, BREAK,CHAR, DOUBLE,ELSE, FOR, IF , INT, RETURN, STRUCT, VOID, WHILE, 
CT_INT, CT_REAL,CT_CHAR, CT_STRING,
COMMA, SEMICOLON, LPAR, RPAR, LBRACKET,RBRACKET, LACC,RACC,
ADD, SUB, MUL, DIV, DOT, AND, OR, NOT, ASSIGN, EQUAL, NOTEQ,LESS,LESSEQ,GREATER,GREATEREQ,
SPACE, LINECOMMENT, COMMENT, UNKNOWN};

#define SAFEALLOC(var,Type)if((var=(Type*)malloc(sizeof(Type)))==NULL)err("not enough memory");

void err(const char *fmt, ...){
    va_list va;
    va_start(va, fmt);
    fprintf(stderr,"error:");
    vfprintf(stderr, fmt, va);
    fputc('\n',stderr);
    exit(-1);
};


typedef struct _Token{
   int code; // codul (numele)
   union{
      char *text; // folosit pentru ID, CT_STRING (alocat dinamic)
      long int i; // folosit pentru CT_INT, CT_CHAR
      double r; // folosit pentru CT_REAL
   };
   int line; // linia din fisierul de intrare
   struct _Token *next; // inlantuire la urmatorul AL
}Token;

void tkerr(const Token *tk,const char *fmt,...)
{
    va_list va;
    va_start(va,fmt);
    fprintf(stderr,"error in line %d: ",tk->line);
    vfprintf(stderr,fmt,va);
    fputc('\n',stderr);
    va_end(va);
    exit(-1);
};

Token *tokens = NULL;
Token *lastToken = NULL;

Token* addTk(int code, int line)
{
    Token *tk;
    SAFEALLOC(tk,Token)
    tk->code=code;
    tk->line=line;
    tk->next=NULL;
    if(lastToken){
        lastToken->next=tk;
    }else{
        tokens=tk;
    }
    
    lastToken=tk;
    return tk;
};

char* createString(char *start, char *end)
{
    char* c;
    size_t len;
    len = end - start;

    
    c = (char*)malloc(len*sizeof(char));

    if(c == NULL){
        printf("Error at createString allocation");
        exit(0);
    }

    strncpy(c,start,len);
    
    return c; 
}
char *pch;


char special_characters[9]= "abfnrtv?";
//!!!!!!!!!!!!!!!!!!--ANALIZATORUL LEXICAL--!!!!!!!!!!!!!!!!!!
int getNextToken(){
   
   Token *tk; 
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
                                                                              pch++;
                                                                              s = 33;
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
               pch++;
               s = 35;
            }
               else s =34;
            
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
                  addTk(COMMENT,line); 
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
            return UNKNOWN;     
   
      }

      printf("%d--- ", s);
      int x;
      scanf("%d", &x);
   
   
   }

}

//!!!!!!!!!!!!!!!!!!--ANALIZATORUL SINTACTIC--!!!!!!!!!!!!!!!!!!

Token *consumedTk;
Token *crtTk;
int unit();
int declStruct();
int declVar();
int typeBase();
int arrayDecl();
int typeName();
int declFunc();
int funcArg();
int stm();
int stmCompound();
int expr();
int exprAssign();
int exprOr();
int exprAnd();
int exprEq();
int exprRel();
int exprAdd();
int exprMul();
int exprCast();
int exprUnary();
int exprPostfix();
int exprPrimary();


int consume( int code ){
	printf("consume(%s)",codeName(code));
	
   if(crtTk -> code == code){
		printf("=> consumat\n");
		crtTk = crtTk -> next;
		return 1;
	}else
   {
		printf("=>altceva\n", codeName(crtTk-> code));;//for debbuging
		return 0;
   }
}

int unit(){
	Token *start=crtTk;
	for(;;){
		if(declStruct()){}
	      else if(declFunc()){}
	      	else if(declVar()){} 
	      		else break;		

	}

	if(consume(END)){
		return 1;
	}

	crtTk = start;

	return 0;
}

int declStruct(){

	Token *start=crtTk; // daca nu se indeplineste totul, nu se consuma nimic atunci
	if(consume(STRUCT))
		if(consume(ID))
			if(consume(LACC))
			{   //while declVar(){}
				for(;;){
					if(declVar()){}
					else break;
				}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						return 1;
					}
				}

			}
		
crtTk = start;	// restaurare pozitie initiala
return 0;
 
}

int declVar(){
   Token *start = crtTk;
   if(typeBase()){
      if(consume(ID)){
         if(arrayDecl()){}
            else {}

         for(;;){
            if(consume(COMMA)){
               if(cosume(ID)){
                   if(arrayDecl()){}
                     else {}
               } else break;
            } else break; 
         }
         
      }
      
      if(consume(SEMICOLON)) { return 1; }
   }

   crtTk = start;
   return 0;

}

int typeBase(){
    
   Token *start = crtTk; 

   if(consume(ID)){ if(consume(ID)) {return 1;}}
      else if(consume(DOUBLE)){ if(consume(ID)) {return 1;}}
         else if(consume(CHAR)){ if(consume(ID)) {return 1;}}
            else if(consume(STRUCT)){ if(consume(ID)) {return 1;}}

   crtTk = start;
   return 0;
}

int arrayDecl(){
   Token *start = crtTk; 

   if(consume(LBRACKET)){
      if(expr()){}
       else {}

       if(consume(RBRACKET)){
          return 1;
       }
   }

   crtTk = start;
   return 0;


}

int typeName(){
   Token *start = crtTk; 

   if(typeBase()){
      if(arrayDecl()){return 1;}
        else return 1;
   }

   crtTk = start;
   return 0;

}

int declFunc(){
   Token *start = crtTk; 

   if(typeBase()){
      if(MUL){}
       else {
          if(VOID){}
       }

       if(consume(ID)){
          if(consume(LPAR)){
            if(funcArg()){
               for(;;){
                  if(consume(COMMA)){
                     if(funcArg()){

                     }
                  }else break;
               }
            }else {}
            if(consume(RPAR)){
               if(stmCompound){return 1;}
            }
          }
       }


   }


   crtTk = start;
   return 0;


}

int funcArg(){
   Token *start = crtTk; 

   if(typeBase()){
      if(consume(ID)){
         if(arrayDecl()){return 1;}
          else {return 1;}
      }
   }

   crtTk = start;
   return 0;

}

int stm(){
   
   Token *start = crtTk; 

   if(stmCompound()) {return 1;}
    else if(consume(IF)){
       if(consume(LPAR)){
          if(expr()){
             if(consume(RPAR)){
                if(stm()){
                   if(consume(ELSE)){
                       if(stm()) return 1; 
                   } else{return 1;}
                }
             }
          }
       }
    }
      else if(consume(WHILE)){
         if(consume(LPAR)){
            if(expr()){
               if(consume(RPAR)){
                  if(stm()) return 1;
               }
            }
         }
      }
         else if(consume(FOR)){
            if(consume(LPAR)){
               if(epxr()){}
                else{}

               if(consume(SEMICOLON)){
                  if(epxr()){}
                   else{}

                   if(consume(SEMICOLON)){
                     if(epxr()){}
                      else{} 
                     

                   if(consume(RPAR)){
                      if(stm()) return 1;
                   } 
                  }  
               } 
            }
         }

            else if(consume(BREAK)){
                if(consume(SEMICOLON)) return 1;
            }
               else if(consume(RETURN)){
                  if(epxr()){}
                   else{}

                   if(consume(SEMICOLON)){ return 1; }
                  
               }

                  else if(expr() && consume(SEMICOLON)) return 1;
                     else if(consume(SEMICOLON)) return 1;
                     
                  

   crtTk = start;
   return 0;

}

int stmCompound(){

   Token *start = crtTk; 

   if(consume(LACC)){
      for(;;){
         if(declVar() || stm()){}
          else break;
      }
      if(consume(RACC)) return 1;

   }

   crtTk = start;
   return 0;
}

int expr(){
    Token *start = crtTk; 

    if(exprAssign()) return 1;
    crtTk = start;
    return 0;
}

int exprAssign(){
   Token *start = crtTk; 
   if(exprUnary()){
      if(consume(ASSIGN)){
         if(exprAssign()) return 1;
      }
   } else if(exprOr()) return 1;

    crtTk = start;
    return 0;
}


int exprOrPrim(){}
int exprAndPrim(){}
int exprEqPrim(){}
int exprRelPrim(){}
int exprAddPrim(){}
int exprMulPrim(){}
int exprPostfixPrim(){}


int exprOr(){
   Token *start = crtTk; 

   if(exprAnd()){
      if(exprOrPrim()) return 1;
   }
   
   crtTk = start;
   return 0;
}

int exprOrPrim(){
   Token *start = crtTk;

   if(consume(OR)){
      if(exprAnd()){
         if(exprOrPrim()) return 1;
      }
   }
   crtTk = start;
   return 1;
   
}

int exprAnd(){
    Token *start = crtTk; 
    
    if(exprEq()){
       if(exprAndPrim()) return 1;
    }
    
    crtTk = start;
    return 0;
}

int exprAndPrim(){
   Token *start = crtTk;

   if(consume(AND)){
       if(exprEq()){
         if(exprAddPrim()) return 1;
       }  
   }

   crtTk = start;
   return 1;
}

int exprEq(){
   Token *start = crtTk; 

   if(exprRel()){
      if(exprEqPrim()) return 1;
   }
  
   crtTk = start;
   return 0; 
}

int exprEqPrim(){
   Token *start = crtTk; 

   if(consume(EQUAL) || consume(NOT)){
      if(exprRel()){
         if(exprEqPrim()) return 1;
      }
   }

   
   crtTk = start;
   return 1; 
}

int exprRel(){
   Token *start = crtTk; 

   if(exprAdd()){
      if(exprRelPrim()) return 1;
   }

   crtTk = start;
   return 0; 
}

int exprRelPrim(){

   Token *start = crtTk; 

   if(consume(LESS) || consume(LESSEQ) || consume(GREATER) || consume(GREATEREQ)){
      if(exprAdd()){
         if(exprRelPrim()) return 1;
      }
   }

   crtTk = start;
   return 1; 

}

int exprAdd(){
   Token *start = crtTk; 

   if(exprMul()){
      if(exprAddPrim()) return 1;
   }   

   crtTk = start;
   return 0;

}

int exprAddPrim(){
   Token *start = crtTk; 

   if(consume(ADD) || consume(SUB)){
      if(exprMul()){
         if(exprAddPrim()) return 1;
      }
   }

   
   crtTk = start;
   return 1; 
}

int exprMul(){
   Token *start = crtTk; 

   if(exprCast()){
      if(exprMulPrim()) return 1;
   }   

   crtTk = start;
   return 0;


}

int exprMulPrim(){
   Token *start = crtTk; 

   if(consume(MUL) || consume(DIV)){
      if(exprCast()){
         if(exprMulPrim()) return 1;
      }
   }

   
   crtTk = start;
   return 1; 
}

exprCast(){
   Token *start = crtTk; 

   if(consume(LPAR)){
      if(typeName()){
         if(consume(RPAR)){
            if(exprCast()) return 1;
         }
      }
   } else if(exprUnary()) return 1;

    
   crtTk = start;
   return 0; 
}

int exprUnary(){
   Token *start = crtTk;
   if(consume(SUB)|| consume(NOT)){
      if(exprUnary()) return 1;
   }else if(exprPostfix()) return 1;

   crtTk = start;
   return 0; 

}

int exprPostfix(){
   Token *start = crtTk;

   if(exprPrimary()){
      if(exprPostfixPrim()) return 1;
   }

   crtTk = start;
   return 0; 
}

int exprPostfixPrim(){
   Token *start = crtTk;

   if(consume(LBRACKET)){
      if(expr()){
         if(consume(RBRACKET)){
            if(exprPostfixPrim()) return 1;
         }
      }
   }else if(consume(DOT)){
      if(consume(ID)){
         if(exprPostfixPrim()) return 1;
      }
   }

   crtTk = start;
   return 0; 
}

int exprPrimary(){
   Token *start = crtTk;

   if(consume(ID)){
      if(consume(LPAR)){
        if(expr()){
           for(;;){
              if(consume(COMMA)){
                 if (expr()){

                 }else break;
              }else break;
           }

        }
        if(consume(RPAR)){ return 1; }
      } else{ return 1;}
   }else if(consume(CT_INT)) return 1;
      else if(consume(CT_REAL)) return 1;
         else if(consume(CT_CHAR)) return 1;
            else if(consume(CT_STRING)) return 1;
               else if(consume(LPAR)){
                  if(expr()){
                     if(consume(RPAR)) return 1;
                  }
               }

   crtTk = start;
   return 0;             

}
 /*
    ANALIZATOR LEXICAL - line 89
    ANALIZATOR SINTACTIC - line 489
    
 */
    
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

   crtTk = tokens;

    while(crtTk != NULL)
    {
       printf("%d\n", crtTk->code);
       crtTk = crtTk->next;
    }
    
   

    
    return 0;

}
