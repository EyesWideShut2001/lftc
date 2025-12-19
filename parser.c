#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "lexer.h"
#include "ad.h"
#include "at.h"
#include "gen.h"

int iTk;
Token *consumed;

void tkerr(const char *fmt, ...)
{
    fprintf(stderr, "error in line %d: ", tokens[iTk].line);
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

bool consume(int code)
{
    if (tokens[iTk].code == code)
    {
        consumed = &tokens[iTk++];
        return true;
    }
    return false;
}

void initBuiltins()
{
    addPredefinedFns();
}

/*** program ::= ( defVar | defFunc | block )* FINISH ***/
bool defVar();
bool defFunc();
bool block();

bool program()
{
    addDomain();
    initBuiltins();

    crtCode = &tMain;
    crtVar = &tBegin;
    Text_write(&tBegin, "#include \"quick.h\"\n\n");
    Text_write(&tMain, "\nint main(){\n");

    for (;;)
    {
        if (defVar())
            continue;
        if (defFunc())
            continue;
        if (block())
            continue;
        break;
    }

    if (!consume(FINISH))
        tkerr("missing FINISH");

    Text_write(&tMain, "return 0;\n}\n");

   FILE *fis = fopen("1.c", "w");
    if (!fis)
    {
        printf("cannot write to file 1.c\n");
        exit(EXIT_FAILURE);
    }
    fwrite(tBegin.buf, sizeof(char), tBegin.n, fis);
    fwrite(tFunctions.buf, sizeof(char), tFunctions.n, fis);
    fwrite(tMain.buf, sizeof(char), tMain.n, fis);
    fclose(fis);

    delDomain();
    return true;
}

/*** defVar ::= VAR ID COLON baseType SEMICOLON ***/
bool baseType();

bool defVar()
{
    int start = iTk;
    if (consume(VAR))
    {
        if (!consume(ID))
            tkerr("missing variable name");
        const char *name = consumed->text;

        if (searchInCurrentDomain(name))
            tkerr("symbol redefinition: %s", name);

        Symbol *s = addSymbol(name, KIND_VAR);
        s->local = crtFn != NULL;

        if (!consume(COLON))
            tkerr("missing ':'");
        if (!baseType())
            tkerr("missing type");
        s->type = ret.type;

        if (!consume(SEMICOLON))
            tkerr("missing ;");

        Text_write(crtVar, "%s %s;\n", cType(ret.type), name);
        return true;
    }
    iTk = start;
    return false;
}

bool baseType()
{
    if (consume(TYPE_INT))
    {
        ret.type = TYPE_INT;
        return true;
    }
    if (consume(TYPE_REAL))
    {
        ret.type = TYPE_REAL;
        return true;
    }
    if (consume(TYPE_STR))
    {
        ret.type = TYPE_STR;
        return true;
    }
    return false;
}

/*** defFunc ::= FUNCTION ID LPAR funcParams? RPAR COLON baseType defVar* block END ***/
bool funcParams();
bool funcParam();

bool defFunc()
{
    int start = iTk;
    if (consume(FUNCTION))
    {
        if (!consume(ID))
            tkerr("missing function name");
        const char *name = consumed->text;

        if (searchInCurrentDomain(name))
            tkerr("symbol redefinition: %s", name);

        crtFn = addSymbol(name, KIND_FN);
        crtFn->args = NULL;

        addDomain();

        crtCode = &tFunctions;
        crtVar = &tFunctions;
        Text_clear(&tFnHeader);
        Text_write(&tFnHeader, "%s(", name);

        if (!consume(LPAR))
            tkerr("missing (");
        funcParams();
        if (!consume(RPAR))
            tkerr("missing )");

        if (!consume(COLON))
            tkerr("missing :");
        if (!baseType())
            tkerr("missing return type");
        crtFn->type = ret.type;

        Text_write(&tFunctions, "\n%s %s){\n", cType(ret.type), tFnHeader.buf);

        while (defVar())
            ; // vars in function
        if (!block())
            tkerr("missing function body");
        if (!consume(END))
            tkerr("missing END");

        Text_write(&tFunctions, "}\n");

        crtCode = &tMain;
        crtVar = &tBegin;
        delDomain();
        crtFn = NULL;

        return true;
    }
    iTk = start;
    return false;
}

bool funcParams()
{
    int start = iTk;
    if (funcParam())
    {
        while (consume(COMMA))
        {
            Text_write(&tFnHeader, ",");
            if (!funcParam())
                tkerr("invalid parameter");
        }
        return true;
    }
    iTk = start;
    return false;
}

bool funcParam()
{
    if (!consume(ID))
        return false;
    const char *name = consumed->text;

    if (searchInCurrentDomain(name))
        tkerr("symbol redefinition: %s", name);

    Symbol *s = addSymbol(name, KIND_ARG);
    Symbol *fa = addFnArg(crtFn, name);

    if (!consume(COLON))
        tkerr("missing :");
    if (!baseType())
        tkerr("missing type");

    s->type = fa->type = ret.type;
    Text_write(&tFnHeader, "%s %s", cType(ret.type), name);
    return true;
}

/*** block ::= instr+ ***/
bool instr();
bool block()
{
    if (!instr())
        return false;
    while (instr())
        ;
    return true;
}

/*** instr ***/
bool expr();

bool instr()
{
    int start = iTk;

    if (expr())
    {
        if (!consume(SEMICOLON))
            tkerr("missing ;");
        Text_write(crtCode, ";\n");
        return true;
    }
    iTk = start;

    if (consume(SEMICOLON))
    {
        Text_write(crtCode, ";\n");
        return true;
    }

    iTk = start;
    if (consume(IF))
    {
        if (!consume(LPAR))
            tkerr("missing ( after IF");

        Text_write(crtCode, "if(");

        if (!expr())
            tkerr("invalid expression in IF");

        if (ret.type == TYPE_STR)
            tkerr("the if condition must have type int or real");

        if (!consume(RPAR))
            tkerr("missing ) after IF condition");

        Text_write(crtCode, "){\n");

        if (!block())
            tkerr("invalid block in IF");

        Text_write(crtCode, "}\n"); // close IF block

        if (consume(ELSE))
        {
            Text_write(crtCode, "else{\n");

            if (!block())
                tkerr("invalid block in ELSE");

            Text_write(crtCode, "}\n"); // close ELSE block
        }

        if (!consume(END))
            tkerr("missing END for IF");

        return true;
    }

    iTk = start;
    if (consume(RETURN))
    {
        Text_write(crtCode, "return ");
        if (!expr())
            tkerr("invalid return expression");
        if (!crtFn)
            tkerr("return outside function");
        if (ret.type != crtFn->type)
            tkerr("wrong return type");
        if (!consume(SEMICOLON))
            tkerr("missing ;");
        Text_write(crtCode, ";\n");
        return true;
    }

    iTk = start;
    if (consume(WHILE))
    {
        Text_write(crtCode, "while(");
        if (!consume(LPAR))
            tkerr("missing (");
        if (!expr())
            tkerr("invalid while condition");
        if (ret.type == TYPE_STR)
            tkerr("while condition must be int or real");
        if (!consume(RPAR))
            tkerr("missing )");
        Text_write(crtCode, "){\n");
        if (!block())
            tkerr("invalid while block");
        if (!consume(END))
            tkerr("missing END");
        Text_write(crtCode, "}\n");
        return true;
    }

    iTk = start;
    return false;
}

/*** expr ::= exprLogic ***/
bool exprLogic();
bool expr() { return exprLogic(); }

/*** exprLogic ::= exprAssign ( ( AND | OR ) exprAssign )* ***/
bool exprAssign();

bool exprLogic()
{
    if (!exprAssign())
        return false;
    while (consume(AND) || consume(OR))
    {
        Ret left = ret;
        if (left.type == TYPE_STR)
            tkerr("invalid && or || operand");
        if (!exprAssign())
            tkerr("invalid operand");
        if (ret.type == TYPE_STR)
            tkerr("invalid && or || operand");
        setRet(TYPE_INT, false);
        Text_write(crtCode, (tokens[iTk - 1].code == AND) ? "&&" : "||");
    }
    return true;
}

/*** exprAssign ::= ID ASSIGN exprComp | exprComp ***/
bool exprComp();

bool exprAssign()
{
    int start = iTk;
    if (consume(ID))
    {
        const char *name = consumed->text;
        if (consume(ASSIGN))
        {
            Text_write(crtCode, "%s=", name);
            if (!exprComp())
                tkerr("invalid assignment");
            Symbol *s = searchSymbol(name);
            if (!s)
                tkerr("undefined symbol: %s", name);
            if (s->kind == KIND_FN)
                tkerr("cannot assign to function");
            if (s->type != ret.type)
                tkerr("type mismatch in assignment");
            ret.lval = false;
            return true;
        }
        iTk = start;
    }
    return exprComp();
}

/*** exprComp ::= exprAdd ( ( LESS | EQUAL ) exprAdd )? ***/
bool exprAdd();

bool exprComp()
{
    if (!exprAdd())
        return false;
    if (consume(LESS) || consume(EQUAL))
    {
        Ret left = ret;
        Text_write(crtCode, (tokens[iTk - 1].code == LESS) ? "<" : "==");
        if (!exprAdd())
            tkerr("invalid comparison");
        if (left.type != ret.type)
            tkerr("comparison operands must have same type");
        setRet(TYPE_INT, false);
    }
    return true;
}

/*** exprAdd ::= exprMul ( ( ADD | SUB ) exprMul )* ***/
bool exprMul();

bool exprAdd()
{
    if (!exprMul())
        return false;
    while (consume(ADD) || consume(SUB))
    {
        Ret left = ret;
        Text_write(crtCode, (tokens[iTk - 1].code == ADD) ? "+" : "-");
        if (!exprMul())
            tkerr("invalid operand");
        if (left.type != ret.type)
            tkerr("operands must have same type");
        ret.lval = false;
    }
    return true;
}

/*** exprMul ::= exprPrefix ( ( MUL | DIV ) exprPrefix )* ***/
bool exprPrefix();

bool exprMul()
{
    if (!exprPrefix())
        return false;
    while (consume(MUL) || consume(DIV))
    {
        Ret left = ret;
        Text_write(crtCode, (tokens[iTk - 1].code == MUL) ? "*" : "/");
        if (!exprPrefix())
            tkerr("invalid operand");
        if (left.type != ret.type)
            tkerr("operands must have same type");
        ret.lval = false;
    }
    return true;
}

/*** exprPrefix ::= SUB factor | NOT factor | factor ***/
bool factor();

bool exprPrefix()
{
    if (consume(SUB))
    {
        Text_write(crtCode, "-");
        if (!factor())
            tkerr("invalid unary -");
        if (ret.type == TYPE_STR)
            tkerr("unary - must be int or real");
        ret.lval = false;
        return true;
    }
    if (consume(NOT))
    {
        Text_write(crtCode, "!");
        if (!factor())
            tkerr("invalid ! operand");
        if (ret.type == TYPE_STR)
            tkerr("! operand must be int or real");
        setRet(TYPE_INT, false);
        return true;
    }
    return factor();
}

/*** factor ***/
bool factor()
{
    if (consume(INT))
    {
        Text_write(crtCode, "%d", consumed->i);
        setRet(TYPE_INT, false);
        return true;
    }
    if (consume(REAL))
    {
        Text_write(crtCode, "%g", consumed->r);
        setRet(TYPE_REAL, false);
        return true;
    }
    if (consume(STR))
    {
        Text_write(crtCode, "\"%s\"", consumed->text);
        setRet(TYPE_STR, false);
        return true;
    }

    if (consume(LPAR))
    {
        Text_write(crtCode, "(");
        if (!expr())
            tkerr("invalid expression");
        if (!consume(RPAR))
            tkerr("missing )");
        Text_write(crtCode, ")");
        return true;
    }

    if (consume(ID))
    {
        Symbol *s = searchSymbol(consumed->text);
        if (!s)
            tkerr("undefined symbol: %s", consumed->text);

        if (consume(LPAR))
        {
            if (s->kind != KIND_FN)
                tkerr("%s is not a function", s->name);
            Symbol *arg = s->args;
            Text_write(crtCode, "%s(", s->name);
            if (expr())
            {
                if (!arg)
                    tkerr("too many arguments");
                if (arg->type != ret.type)
                    tkerr("argument type mismatch");
                arg = arg->next;
                while (consume(COMMA))
                {
                    Text_write(crtCode, ",");
                    if (!expr())
                        tkerr("expected expression");
                    if (!arg)
                        tkerr("too many arguments");
                    if (arg->type != ret.type)
                        tkerr("argument type mismatch");
                    arg = arg->next;
                }
            }
            if (!consume(RPAR))
                tkerr("missing )");
            if (arg)
                tkerr("too few arguments");
            Text_write(crtCode, ")");
            setRet(s->type, false);
            return true;
        }

        if (s->kind == KIND_FN)
            tkerr("function %s can only be called", s->name);

        Text_write(crtCode, "%s", s->name);
        setRet(s->type, true);
        return true;
    }

    return false;
}

void parse()
{
    iTk = 0;
    if (!program())
        tkerr("invalid program");
}
