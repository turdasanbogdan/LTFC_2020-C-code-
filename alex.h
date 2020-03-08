#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

char special_characters[9]= "abfnrtv?";
int getState( char *pch){
   
   int s = 0;
   char c;
   for(int i = 0; i< strlen(pch); i++ ){
      c = *(pch+i);
      printf("%c\n",c);     

   

      switch(s){
         case 0:
            if(c =='/') { s = 61; break;}//COMMENT
             if(c == '\0') {printf("END"); s=0; break;}//END
              if(c == 39) { s=67; break; } //CT_CHAR
               if(c == 34) { s=71; break; } //CT_STRING

            break; 
              
         // ---------- COMMENT ----------
         case 61:
            if(c =='/') s=65;
               else if(c =='*') s=62;
                 else { printf("DIV"); s=0;}
            
            break;

         case 62: 
            if(c=='*') s= 63;
               if(c != '*') s=62;
            
            break;

         case 63:
            if(c=='/') { printf("COMMENT\n"); s=0; }  
               else if(c=='*') s=63;
                  else s=62;
            
            break;            
         case 65:
             if(c == '\n' || c == '\r' || c=='\0' ){ printf("COMMENT"); s=0; }
               else s=65;
            
            break;  
         
         // ---------- CT_CHAR -----------

         case 67:
            if(c == '\\') s=70;
              else if((c != '\\' ) && (c != 39)) s=68;
            
            break;
         
         case 68: 
            if(c == 39) {printf("CT_CHAR"); s=0;}
            
            break;
         
         case 70:
            if(strchr(special_characters,c)!=NULL || c == '\\' || c == 34 || c == '\0') s=68;
            
            break;

         // ---------- CT_STRING -----------

         case 71:
            if (c == '\\') s=72;
             else s=73;    
            
            break;

         case 72: 
            if(strchr(special_characters,c)!=NULL || c == '\\' || c == 34 || c == '\0') s=71;
            
            break;

         case 73: 
            if(c == 34) { printf("CT_STRING"); s=0;} 
               else if( c != '\\' && c != 39) s=73;
                else if( c == '\\') s=72;
                 
         
            break;



         default:
            printf("UNKNOWN");     
   
      }

      printf("%d--- ", s);
   
   
   }

}