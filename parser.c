#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "lexer.h"

int iTk; //index token curent
Token *consumed; 

void tkerr(const char *fmt, ...){ 
    fprintf(stderr,"error in line %d: ",tokens[iTk].line); 
    va_list va;
    va_start(va,fmt);
        vfprintf(stderr,fmt,va);
    va_end(va);
    fprintf(stderr,"\n");
    exit(EXIT_FAILURE);
}

bool consume(int code){
    if(tokens[iTk].code==code){ //citim token curent din vector de tokeni 
        consumed=&tokens[iTk++]; //si consuma daca se potriveste
        return true;
    }
    return false;
}

/*** program ::= ( defVar | defFunc | block )* FINISH ***/
bool defVar();
bool defFunc();
bool block();

bool program(){
    for(;;){
        if(defVar()) continue;
        if(defFunc()) continue;
        if(block()) continue;
        break;
    }
    if(!consume(FINISH)) tkerr("missing FINISH");
    return true;
}

/*** defVar ::= VAR ID COLON baseType SEMICOLON ***/
bool baseType();

bool defVar(){   // var x: int;
    int start=iTk;
    if(consume(VAR)){
        if(consume(ID)){
            if(consume(COLON)){
                if(baseType()){
                    if(consume(SEMICOLON)) return true;
                    tkerr("missing ; in variable definition");
                }
            }
        }
    }
    iTk=start;
    return false;
}

/*** baseType ::= TYPE_INT | TYPE_REAL | TYPE_STR ***/
bool baseType(){
    return consume(TYPE_INT) || consume(TYPE_REAL) || consume(TYPE_STR);  
}

/*** defFunc ::= FUNCTION ID LPAR funcParams? RPAR COLON baseType defVar* block END ***/
bool funcParams();  //   a: int, b: real;

bool defFunc(){   // function f (a: int, b: real) : int  
                    //      return a;
                    //  end
    int start=iTk;
    if(consume(FUNCTION)){
        if(!consume(ID)) tkerr("missing function name");
        if(!consume(LPAR)) tkerr("missing ( in function");
        funcParams(); // optional
        if(!consume(RPAR)) tkerr("missing ) in function");

        if(!consume(COLON)) tkerr("missing : in function");
        if(!baseType()) tkerr("missing return type");

        while(defVar());
        if(!block()) tkerr("missing function body");

        if(!consume(END)) tkerr("missing END");

        return true;
    }
    iTk=start;
    return false;
}

/*** block ::= instr+ ***/
bool instr();

bool block(){
    if(!instr()) return false;
    while(instr());
    return true;
}

/*** funcParams ::= funcParam ( COMMA funcParam )* ***/
bool funcParam(); //   a: int

bool funcParams(){   //   a: int, b: real
    int start=iTk;
    if(funcParam()){
        while(consume(COMMA)){
            if(!funcParam()) tkerr("expected parameter");
        }
        return true;
    }
    iTk=start;
    return false;
}

/*** funcParam ::= ID COLON baseType ***/
bool funcParam(){
    int start=iTk;
    if(consume(ID)){
        if(consume(COLON)){
            if(baseType()) return true;
            else tkerr("missing basetype");
        }
        else tkerr("missing colon");
    }
    iTk=start;
    return false;
}

/*** instr ::= expr? SEMICOLON
    | IF LPAR expr RPAR block ( ELSE block )? END
    | RETURN expr SEMICOLON
    | WHILE LPAR expr RPAR block END
***/
bool expr();

bool instr(){
    int start=iTk;

    // expr? SEMICOLON
    if(expr()){
        if(consume(SEMICOLON)) return true;  // x=10;
        tkerr("missing ; after expression");
    }
    iTk=start;
    if(consume(SEMICOLON)) return true;

    // IF
    iTk=start;
    if(consume(IF)){
        if(!consume(LPAR)) tkerr("missing ( after IF");
        if(!expr()) tkerr("invalid expression in IF");
        if(!consume(RPAR)) tkerr("missing ) in IF");
        if(!block()) tkerr("invalid block in IF");
        if(consume(ELSE)){
            if(!block()) tkerr("invalid block in ELSE");
        }
        if(!consume(END)) tkerr("missing END for IF");
        return true;
    }

    // RETURN
    iTk=start;
    if(consume(RETURN)){
        if(!expr()) tkerr("invalid return expr");
        if(!consume(SEMICOLON)) tkerr("missing ; after RETURN");
        return true;
    }

    // WHILE
    iTk=start;
    if(consume(WHILE)){
        if(!consume(LPAR)) tkerr("missing ( after WHILE");
        if(!expr()) tkerr("invalid WHILE condition");
        if(!consume(RPAR)) tkerr("missing ) after WHILE");
        if(!block()) tkerr("invalid block in WHILE");
        if(!consume(END)) tkerr("missing END for WHILE");
        return true;
    }

    iTk=start;
    return false;
}

/*** expr ::= exprLogic ***/
bool exprLogic();

bool expr(){
    return exprLogic();
}

/*** exprLogic ::= exprAssign ( ( AND | OR ) exprAssign )* ***/
bool exprAssign();

bool exprLogic(){
    if(!exprAssign()) return false;
    while(consume(AND) || consume(OR)){
        if(!exprAssign()) tkerr("invalid operand in logical expression");
    }
    return true;
}

/*** exprAssign ::= ( ID ASSIGN )? exprComp ***/
bool exprComp();

bool exprAssign(){
    int start=iTk;
    if(consume(ID)){
        if(consume(ASSIGN)){
            if(exprComp()) return true;
            tkerr("invalid assignment");
        }
        iTk=start;
    }
    return exprComp();
}

/*** exprComp ::= exprAdd ( ( LESS | EQUAL ) exprAdd )? ***/
bool exprAdd();

bool exprComp(){    // x > 10
    if(!exprAdd()) return false;
    if(consume(LESS) || consume(EQUAL)){
        if(!exprAdd()) tkerr("invalid comparison");
    }
    return true;
}

/*** exprAdd ::= exprMul ( ( ADD | SUB ) exprMul )* ***/
bool exprMul();

bool exprAdd(){
    if(!exprMul()) return false;
    while(consume(ADD) || consume(SUB)){
        if(!exprMul()) tkerr("invalid operand");
    }
    return true;
}

/*** exprMul ::= exprPrefix ( ( MUL | DIV ) exprPrefix )* ***/
bool exprPrefix();

bool exprMul(){
    if(!exprPrefix()) return false;
    while(consume(MUL) || consume(DIV)){
        if(!exprPrefix()) tkerr("invalid multiplication operand");
    }
    return true;
}

/*** exprPrefix ::= ( SUB | NOT )? factor ***/
bool factor();

bool exprPrefix(){
    consume(SUB) || consume(NOT);
    return factor();
}

/*** factor ::= INT
               | REAL
               | STR
               | LPAR expr RPAR
               | ID ( LPAR ( expr ( COMMA expr )* )? RPAR )?
***/
bool factor(){  //punct de intrare pt expresii complexe
    int start=iTk;

    if(consume(INT) || consume(REAL) || consume(STR)) return true;

    if(consume(LPAR)){
        if(expr()){
            if(consume(RPAR)) return true;
            tkerr("missing )");
        }
        tkerr("invalid expression in ( )");
    }

    if(consume(ID)){
        if(consume(LPAR)){
            if(expr()){
                while(consume(COMMA)){
                    if(!expr()) tkerr("expected expression");
                }
            }
            if(!consume(RPAR)) tkerr("missing ) in function call");
        }
        return true;
    }

    iTk=start;
    return false;
}

void parse(){
    iTk=0;
    if(!program()) tkerr("invalid program");
}
