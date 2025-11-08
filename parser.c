#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "lexer.h"

int iTk;  
Token *consumed;  

void tkerr(const char *fmt,...){
    fprintf(stderr,"error in line %d: ",tokens[iTk].line);
    va_list va;
    va_start(va,fmt);
    vfprintf(stderr,fmt,va);
    va_end(va);
    fprintf(stderr,"\n");
    exit(EXIT_FAILURE);
}

bool consume(int code){
    if(tokens[iTk].code==code){
        consumed=&tokens[iTk++];
        return true;
    }
    return false;
}

/*** baseType ::= TYPE_INT | TYPE_REAL | TYPE_STR ***/
bool baseType(){
    return consume(TYPE_INT) || consume(TYPE_REAL) || consume(TYPE_STR);
}

/*** funcParam ::= ID COLON baseType ***/
bool funcParam(){
    int start = iTk;
    if(consume(ID)){
        if(consume(COLON)){
            if(baseType()) return true;
        }
    }
    iTk=start;
    return false;
}

/*** funcParams ::= funcParam ( COMMA funcParam )* ***/
bool funcParams(){
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

/*** expr forward declaration ***/
bool expr();

/*** factor ::= INT | REAL | STR | LPAR expr RPAR | ID ( LPAR (expr (COMMA expr)*)? RPAR )? ***/
bool factor(){
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

/*** exprPrefix ::= ( SUB | NOT )? factor ***/
bool exprPrefix(){
    consume(SUB) || consume(NOT);
    return factor();
}

/*** exprMul ::= exprPrefix ( ( MUL | DIV ) exprPrefix )* ***/
bool exprMul(){
    if(!exprPrefix()) return false;
    while(consume(MUL) || consume(DIV)){
        if(!exprPrefix()) tkerr("invalid multiplication operand");
    }
    return true;
}

/*** exprAdd ::= exprMul ( ( ADD | SUB ) exprMul )* ***/
bool exprAdd(){
    if(!exprMul()) return false;
    while(consume(ADD) || consume(SUB)){
        if(!exprMul()) tkerr("invalid operand");
    }
    return true;
}

/*** exprComp ::= exprAdd ( ( LESS | EQUAL ) exprAdd )? ***/
bool exprComp(){
    if(!exprAdd()) return false;
    if(consume(LESS) || consume(EQUAL)){
        if(!exprAdd()) tkerr("invalid comparison");
    }
    return true;
}

/*** exprAssign ::= ( ID ASSIGN )? exprComp ***/
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

/*** exprLogic ::= exprAssign ( ( AND | OR ) exprAssign )* ***/
bool exprLogic(){
    if(!exprAssign()) return false;
    while(consume(AND) || consume(OR)){
        if(!exprAssign()) tkerr("invalid operand in logical expression");
    }
    return true;
}

/*** expr ::= exprLogic ***/
bool expr(){
    return exprLogic();
}

/*** instr ::=  expr? SEMICOLON
| IF LPAR expr RPAR block ( ELSE block )? END
| RETURN expr SEMICOLON
| WHILE LPAR expr RPAR block END
***/
bool block(); // forward

bool instr(){
    int start=iTk;

    // expr? SEMICOLON
    if(expr()){
        if(consume(SEMICOLON)) return true;
        tkerr("missing ; after expression");
    }
    iTk=start;
    if(consume(SEMICOLON)) return true;   // empty statement

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

    // RETURN expr ;
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

/*** block ::= instr+ ***/
bool block(){
    if(!instr()) return false;
    while(instr());
    return true;
}

/*** defVar ::= VAR ID COLON baseType SEMICOLON ***/
bool defVar(){
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

/*** defFunc ::= FUNCTION ID LPAR funcParams? RPAR COLON baseType defVar* block END ***/
bool defFunc(){
    int start=iTk;
    if(consume(FUNCTION)){
        if(!consume(ID)) tkerr("missing function name");
        if(!consume(LPAR)) tkerr("missing ( in function");
        funcParams();  // optional
        if(!consume(RPAR)) tkerr("missing ) in function");

        if(!consume(COLON)) tkerr("missing : in function");
        if(!baseType()) tkerr("missing return type");

        while(defVar()); // zero or more
        if(!block()) tkerr("missing function body");
        if(!consume(END)) tkerr("missing END");
        return true;
    }
    iTk=start;
    return false;
}

/*** program ::= ( defVar | defFunc | block )* FINISH ***/
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

void parse(){
    iTk=0;
    if(!program()) tkerr("invalid program");
}
