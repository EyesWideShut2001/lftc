#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "lexer.h"
#include "ad.h"

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

void initBuiltins() {
    Symbol *s;

    s = addSymbol("puti", KIND_FN);
    s->type = TYPE_INT;
    addFnArg(s, "x")->type = TYPE_INT;

    s = addSymbol("putr", KIND_FN);
    s->type = TYPE_INT;
    addFnArg(s, "x")->type = TYPE_REAL;

    s = addSymbol("puts", KIND_FN);
    s->type = TYPE_INT;
    addFnArg(s, "x")->type = TYPE_STR;
}


/*** program ::= ( defVar | defFunc | block )* FINISH ***/
bool defVar();
bool defFunc();
bool block();

bool program(){
    addDomain();   // <-- domeniul global
    initBuiltins();  // adaugă builtin-urile în acest domeniu

    for(;;){
        if(defVar()) continue;
        if(defFunc()) continue;
        if(block()) continue;
        break;
    }

    if(!consume(FINISH)) tkerr("missing FINISH");

    delDomain(); // <-- închidere domeniu global
    return true;
}

/*** defVar ::= VAR ID COLON baseType SEMICOLON ***/
bool baseType();

bool defVar(){
    int start=iTk;
    if(consume(VAR)){
        if(consume(ID)){
            const char *name = consumed->text;
            Symbol *s = searchInCurrentDomain(name);
            if(s) tkerr("symbol redefinition: %s", name);
            s = addSymbol(name, KIND_VAR);
            s->local = crtFn != NULL;

            if(consume(COLON)){
                if(baseType()){
                    s->type = ret.type;

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
    if(consume(TYPE_INT)){
        ret.type = TYPE_INT;
        return true;
    }
    if(consume(TYPE_REAL)){
        ret.type = TYPE_REAL;
        return true;
    }
    if(consume(TYPE_STR)){
        ret.type = TYPE_STR;
        return true;
    }
    return false;
}


/*** defFunc ::= FUNCTION ID LPAR funcParams? RPAR COLON baseType defVar* block END ***/
bool funcParams();  //   a: int, b: real;

bool defFunc(){
    int start=iTk;
    if(consume(FUNCTION)){
        if(!consume(ID)) tkerr("missing function name");

        const char *name = consumed->text;
        Symbol *s = searchInCurrentDomain(name);
        if(s) tkerr("symbol redefinition: %s", name);
        crtFn = addSymbol(name, KIND_FN);
        crtFn->args = NULL;

        addDomain(); // domeniul local al funcției

        if(!consume(LPAR)) tkerr("missing ( in function");
        funcParams();
        if(!consume(RPAR)) tkerr("missing ) in function");

        if(!consume(COLON)) tkerr("missing : in function");
        if(!baseType()) tkerr("missing return type");
        crtFn->type = ret.type;

        while(defVar());
        if(!block()) tkerr("missing function body");

        if(!consume(END)) tkerr("missing END");
        
        delDomain();

        crtFn = NULL;

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
        const char *name = consumed->text;
        Symbol *s = searchInCurrentDomain(name);
        if(s) tkerr("symbol redefinition: %s", name);

        s = addSymbol(name, KIND_ARG);
        Symbol *sFnParam = addFnArg(crtFn, name);

        if(consume(COLON)){
            if(baseType()){
                s->type = ret.type;
                sFnParam->type = ret.type;
                return true;
            }
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
        const char *name = consumed->text;
        Symbol *s = searchSymbol(name); // CAUTĂ ÎN TOATE DOMENIILE
        if(!s) tkerr("undefined symbol: %s", name);

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
